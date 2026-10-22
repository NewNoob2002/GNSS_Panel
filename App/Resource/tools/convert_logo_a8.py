#!/usr/bin/env python3
"""单色 Logo 裁剪与 A8 转换，仅使用标准库及现有 v9 编解码器。

用法：python3 App/Resource/tools/convert_logo_a8.py 输入.c 输出.c [--exclude-first-column]
底色取 RGB 众数，前景取离底色最远的原图颜色；投影估计 alpha，>16 的
外接矩形四周留 2 px。回读按 LVGL RGB565 混色计算，最大通道误差 >20 则
失败；彩色 Logo 可显式传入 --keep-rgb565-on-error 保留 RGB565。
仅默认 Logo 显式排除第 0 列转换残留；该列不参与误差验收。
输入可使用 git show 91a38ea:App/Resource/img/文件名.c 导出的原始 C 资源。
"""
from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

import convert_images_v9 as codec

METADATA_MARKER = "/* Logo 布局：底色、前景色及原画面中的裁剪偏移。 */"


@dataclass
class Logo:
    """转换结果，误差以 8 位 RGB 单通道计。"""
    image: codec.Image9
    background: tuple
    foreground: tuple
    x: int
    y: int
    maximum: int
    mean: float


def blend565(foreground: int, background: int, alpha: int) -> int:
    """复现 LVGL 9.6 RGB565 遮罩混色的 5 位 alpha 舍入。"""
    if alpha <= 2:
        return background
    if alpha >= 253:
        return foreground
    mix = (alpha + 4) >> 3
    fg = (foreground | (foreground << 16)) & 0x07E0F81F
    bg = (background | (background << 16)) & 0x07E0F81F
    value = (((((fg - bg) & 0xffffffff) * mix & 0xffffffff) >> 5) + bg) & 0x07E0F81F
    return (value | (value >> 16)) & 0xffff


def convert(source: codec.Image8, exclude_first_column: bool = False) -> Logo:
    """估算单色遮罩并裁剪，拒绝非不透明或无前景的输入。"""
    if not source.opaque:
        raise codec.ConvertError("Logo 输入必须不透明")
    pixels = [tuple(source.rgba[n:n + 3]) for n in range(0, len(source.rgba), 4)]
    retained = [p for n, p in enumerate(pixels) if not exclude_first_column or n % source.w]
    if not retained:
        raise codec.ConvertError("排除残留列后没有像素")
    background = Counter(retained).most_common(1)[0][0]
    foreground = max(retained, key=lambda p: sum((p[k] - background[k]) ** 2 for k in range(3)))
    direction = [foreground[k] - background[k] for k in range(3)]
    denominator = sum(v * v for v in direction)
    if denominator == 0:
        raise codec.ConvertError("Logo 没有可见前景")
    alpha = [max(0, min(255, round(255 * sum((p[k] - background[k]) * direction[k]
                                           for k in range(3)) / denominator))) for p in pixels]
    if exclude_first_column:
        alpha[::source.w] = [0] * source.h
    visible = [(n % source.w, n // source.w) for n, a in enumerate(alpha) if a > 16]
    if not visible:
        raise codec.ConvertError("Logo 前景低于裁剪阈值")
    x = max(int(exclude_first_column), min(p[0] for p in visible) - 2)
    y = max(0, min(p[1] for p in visible) - 2)
    right = min(source.w, max(p[0] for p in visible) + 3)
    bottom = min(source.h, max(p[1] for p in visible) + 3)
    data = bytes(alpha[row * source.w + col] for row in range(y, bottom) for col in range(x, right))
    image = codec.Image9(source.symbol, right - x, bottom - y, "A8", right - x, data)
    # 从生成 C 文本独立回读，避免只验证内存中的待写入结果。
    parsed, = codec.parse_v9_text(codec.render_c_file([image]))
    bg, fg = codec.quantize565(*background), codec.quantize565(*foreground)
    errors = []
    for n, pixel in enumerate(pixels):
        col, row = n % source.w, n // source.w
        if exclude_first_column and col == 0:
            continue
        a = parsed.data[(row - y) * parsed.w + col - x] if x <= col < right and y <= row < bottom else 0
        rgb = codec.rgb565_to_rgb888(blend565(fg, bg, a))
        errors.extend(abs(rgb[k] - pixel[k]) for k in range(3))
    return Logo(image, background, foreground, x, y, max(errors), sum(errors) / len(errors))


def render(logo: Logo) -> str:
    """生成 A8/RGB565 描述符和布局常量，颜色使用 0xRRGGBB。"""
    bg = int.from_bytes(bytes(logo.background), "big")
    fg = int.from_bytes(bytes(logo.foreground), "big")
    return codec.render_c_file([logo.image]) + (f'\n{METADATA_MARKER}\n'
        '#include "Resource/LogoLayout.h"\n'
        f'const system_loading_logo_layout_t {logo.image.symbol}_layout = {{'
        f'0x{bg:06x}, 0x{fg:06x}, {logo.x}, {logo.y}}};\n')


def main() -> int:
    """读取原始 C 数组，超限时失败；仅显式允许彩色回退时保留 RGB565。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--exclude-first-column", action="store_true")
    parser.add_argument("--keep-rgb565-on-error", action="store_true",
                        help="明确允许彩色 Logo 超过误差门槛时保留 RGB565")
    args = parser.parse_args()
    try:
        source, = codec.parse_source_file(args.source)
        logo = convert(source, args.exclude_first_column)
        print(f"{source.symbol}: A8 候选 {logo.image.w}x{logo.image.h} 偏移 ({logo.x},{logo.y}), "
              f"底色 {logo.background}, 前景 {logo.foreground}, 最大误差 {logo.maximum}/255, 均值 {logo.mean:.6f}/255")
        if logo.maximum > 20:
            if not args.keep_rgb565_on_error:
                raise codec.ConvertError("最大误差超过 20/255，拒绝有损单色转换")
            print("超过最大误差 20/255，保留 RGB565")
            logo.image = codec.encode_v9(source.symbol, source.w, source.h, source.rgba, "RGB565")
            logo.x = logo.y = 0
        text = render(logo)
        newline = "\r\n" if args.destination.exists() and b"\r\n" in args.destination.read_bytes() else "\n"
        args.destination.write_bytes(text.replace("\n", newline).encode())
        parsed, = codec.parse_v9_text(args.destination.read_text())
        if parsed.data != logo.image.data or parsed.cf != logo.image.fmt:
            raise codec.ConvertError("磁盘回读不一致")
        return 0
    except (codec.ConvertError, OSError, ValueError) as exc:
        parser.exit(1, f"ERROR: {exc}\n")


if __name__ == "__main__":
    raise SystemExit(main())
