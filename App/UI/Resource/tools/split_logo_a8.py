#!/usr/bin/env python3
"""将任意带 layout 的单色 A8 C 资源拆为无损的左→右入场零件表（仅标准库）。

用法：python3 split_logo_a8.py 输入.c或.png 输出.c [--groups '0,1|2|3']
PNG 示例：python3 split_logo_a8.py designer.png ../img/img_src_NewLogo.c --symbol img_src_NewLogo --fit
分组示例：python3 split_logo_a8.py designer.png ../img/img_src_NewLogo.c --groups '0,1|2' --background 000000 --foreground ffffff
注册 NewLogoParts 后，SystemLoadingView 的选择行改为 resource_pool::get_image("NewLogoParts")。
PNG 支持非隔行 Gray/GrayAlpha/RGB/RGBA 的 8/16 位及索引色 1/2/4/8 位（PLTE/tRNS）。
--fit 显式等比缩小超屏图片；16 位取高字节降至 8 位，隔行输入清晰报错。
扫描/压缩过的近似单色 Logo 可加 --max-error N（0..255，默认 20）；工具打印实际最大/平均误差，
仅在最大通道误差不超过 N 时继续。例如：python3 split_logo_a8.py noisy.png output.c --max-error 80。
可选 --timings '组号:延迟ms:时长ms|...' 覆盖相对背景淡入结束的时间；
--no-highlight 关闭高光。默认每个 8 邻域 alpha>24 分量为一组。
弱 alpha 像素归到欧氏距离最近的强像素分量，平距时按左→右顺序选择。

新增 Logo：
1. 将设计师 PNG 或带颜色和位置 layout 的 A8 .c 放进 tools/logos（彩色图保留原格式）。
2. 运行本脚本生成 img/ 下的零件表，必要时传 --groups 和 --timings。
3. 在 ResourcePool 静态表中注册零件表名称；在 View 的宏分支选择它。
4. 运行 test_convert_images_v9.py 和 Linux app_logo_reveal_test 校验还原及终态。
"""
from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from pathlib import Path

import convert_images_v9 as codec
import convert_logo_a8 as logo_converter
import png_logo


@dataclass
class Part:
    """零件的紧裁剪图像、图内偏移及入场组。"""
    image: codec.Image9
    x: int
    y: int
    group: int


def groups_for(count: int, text: str | None) -> list[int]:
    """要求每个分量恰好出现一次，组号连续且无空组。"""
    if text is None:
        return list(range(count))
    groups = [[int(i) for i in group.split(',')] for group in text.split('|')]
    flat = [i for group in groups for i in group]
    if sorted(flat) != list(range(count)):
        raise ValueError('--groups 必须恰好包含每个分量一次')
    return [next(g for g, group in enumerate(groups) if i in group) for i in range(count)]


def split(image: codec.Parsed9, groups: str | None = None) -> list[Part]:
    """提取强 alpha 分量，并按最近强像素分配弱边缘；每个非零字节唯一归属。"""
    if image.cf != 'A8' or image.stride != image.w:
        raise ValueError('输入必须是紧排 A8 图像')
    w, h, data = image.w, image.h, image.data
    remaining = {i for i, a in enumerate(data) if a > 24}
    if not remaining:
        remaining = {i for i, a in enumerate(data) if a}
    components = []
    while remaining:
        seed = min(remaining)
        remaining.remove(seed)
        pending, component = [seed], [seed]
        while pending:
            n = pending.pop()
            x, y = n % w, n // w
            for yy in range(max(0, y - 1), min(h, y + 2)):
                for xx in range(max(0, x - 1), min(w, x + 2)):
                    other = yy * w + xx
                    if other in remaining:
                        remaining.remove(other)
                        pending.append(other)
                        component.append(other)
        components.append(component)
    if not components:
        raise ValueError('输入没有可见像素')
    components.sort(key=lambda c: (min(i % w for i in c), min(i // w for i in c)))
    group_ids = groups_for(len(components), groups)
    owner = {n: g for g, component in enumerate(components) for n in component}
    seeds = [(n % w, n // w, g) for g, component in enumerate(components) for n in component]
    for n, alpha in enumerate(data):
        if alpha and n not in owner:
            x, y = n % w, n // w
            _, owner[n] = min(((x - sx) ** 2 + (y - sy) ** 2, g) for sx, sy, g in seeds)
    parts = []
    for g in range(len(components)):
        pixels = [n for n in owner if owner[n] == g]
        x, y = min(n % w for n in pixels), min(n // w for n in pixels)
        right, bottom = max(n % w for n in pixels) + 1, max(n // w for n in pixels) + 1
        payload = bytes(data[yy * w + xx] if owner.get(yy * w + xx) == g else 0
                        for yy in range(y, bottom) for xx in range(x, right))
        parts.append(Part(codec.Image9(f'{image.symbol}_part{g}', right - x, bottom - y,
                                      'A8', right - x, payload), x, y, group_ids[g]))
    verify(image, parts)
    return parts


def verify(source: codec.Parsed9, parts: list[Part]) -> None:
    """逐字节校验 max 与相加重建，同时拒绝任何非零像素重叠。"""
    total, maximum = bytearray(source.w * source.h), bytearray(source.w * source.h)
    for part in parts:
        for y in range(part.image.h):
            for x in range(part.image.w):
                alpha = part.image.data[y * part.image.w + x]
                n = (part.y + y) * source.w + part.x + x
                if total[n] and alpha:
                    raise ValueError('零件非零像素重叠')
                total[n] += alpha
                maximum[n] = max(maximum[n], alpha)
    if bytes(total) != source.data or maximum != total:
        raise ValueError('零件重建与原图不一致')


def layout_from(text: str, symbol: str) -> tuple[int, int, int, int]:
    """读取现有 C 资源的颜色和画面偏移。"""
    found = re.search(rf'\b{symbol}_layout\s*=\s*\{{([^}}]+)\}}', text)
    if not found:
        raise ValueError('输入缺少同名 layout')
    values = tuple(int(s.strip(), 0) for s in found[1].split(','))
    if len(values) != 4:
        raise ValueError('layout 必须含底色、前景色、x、y')
    return values


def render(source: codec.Parsed9, layout: tuple, parts: list[Part], timings: str = '',
           highlight: bool = True) -> str:
    """生成 v9 零件、组时间覆盖及公开静态表，不编译原整图。"""
    bg, fg, x, y = layout
    count = max(p.group for p in parts) + 1
    overrides = {}
    if timings:
        for entry in timings.split('|'):
            group, delay, duration = map(int, entry.split(':'))
            if not 0 <= group < count or not 0 <= delay <= 60000 or not 1 <= duration <= 60000 or group in overrides:
                raise ValueError('无效或重复的 --timings 组/延迟/时长')
            overrides[group] = (delay, duration)
    text = codec.render_c_file([p.image for p in parts])
    text += '\n#include "UI/Resource/LogoParts.h"\n'
    text += f'const system_loading_logo_layout_t {source.symbol}_layout = {{0x{bg:06x}, 0x{fg:06x}, {x}, {y}}};\n'
    text += f'static const logo_part_t {source.symbol}_pieces[] = {{\n'
    text += ''.join(f'    {{&{p.image.symbol}, {p.x}, {p.y}, {p.group}}},\n' for p in parts)
    text += '};\n'
    text += f'static const logo_group_t {source.symbol}_groups[] = {{\n'
    text += ''.join(f'    {{{overrides.get(g, (-1, 0))[0]}, {overrides.get(g, (-1, 0))[1]}}},\n' for g in range(count))
    text += '};\n'
    text += f'const logo_parts_t {source.symbol}_parts = {{\n'
    text += f'    {source.symbol}_pieces, {source.symbol}_groups, {len(parts)}, {count},\n'
    text += f'    {{0x{bg:06x}, 0x{fg:06x}, {x}, {y}}}, {source.w}, {source.h}, true, {str(highlight).lower()}\n}};\n'
    return text


def main() -> int:
    """解析资源并在写入前验证分组、时间和重建。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    parser.add_argument('--groups')
    parser.add_argument('--timings', default='')
    parser.add_argument('--no-highlight', action='store_true')
    parser.add_argument('--background', help='PNG 底色 RRGGBB')
    parser.add_argument('--foreground', help='PNG 前景色 RRGGBB')
    parser.add_argument('--fit', action='store_true')
    parser.add_argument('--symbol', help='PNG 输出 C 符号，如 img_src_NewLogo')
    parser.add_argument('--max-error', type=int, default=20, help='PNG 近似单色回读最大通道误差，0..255，默认 20')
    args = parser.parse_args()
    try:
        if not 0 <= args.max_error <= 255:
            raise ValueError('--max-error 必须在 0..255 内')
        if args.source.suffix.lower() == '.png':
            def color(value):
                if value is None:
                    return None
                if not re.fullmatch(r'#?[0-9a-fA-F]{6}', value):
                    raise ValueError('颜色必须是 RRGGBB')
                return tuple(bytes.fromhex(value.lstrip('#')))
            bg, fg = color(args.background), color(args.foreground)
            decoded = png_logo.read(args.source, bg, args.fit)
            decoded.symbol = args.symbol or 'img_src_' + re.sub(r'\W', '_', args.source.stem)
            if not re.fullmatch(r'[A-Za-z][A-Za-z0-9_]*', decoded.symbol):
                raise ValueError('无效的 C 符号')
            logo = logo_converter.convert(decoded, background=bg, foreground=fg)
            print(f'PNG RGB565 回读误差：最大 {logo.maximum}/255，平均 {logo.mean:.6f}/255；'
                  f'允许最大 {args.max_error}/255')
            if logo.maximum > args.max_error:
                raise ValueError(f'PNG 单色回读最大误差 {logo.maximum}/255 超过 --max-error {args.max_error}')
            if logo.maximum > 20:
                print('按显式误差门槛继续近似单色转换')
            source, = codec.parse_v9_text(codec.render_c_file([logo.image]))
            layout = (*[int.from_bytes(bytes(c), 'big') for c in (logo.background, logo.foreground)], logo.x, logo.y)
        else:
            if args.background or args.foreground or args.fit or args.symbol or args.max_error != 20:
                raise ValueError('--background/--foreground/--fit/--symbol/--max-error 仅用于 PNG')
            text = args.source.read_text()
            source, = codec.parse_v9_text(text)
            layout = layout_from(text, source.symbol)
        parts = split(source, args.groups)
        output = render(source, layout, parts, args.timings, not args.no_highlight)
        readback = codec.parse_v9_text(output)
        if [p.data for p in readback] != [p.image.data for p in parts]:
            raise ValueError('生成 C 零件回读不一致')
        args.destination.write_text(output)
        print(f'{source.symbol}: {source.w}x{source.h}, {len(parts)} parts, {1 + max(p.group for p in parts)} groups, '
              f'{sum(len(p.image.data) for p in parts)} A8 bytes')
        return 0
    except (OSError, ValueError, codec.ConvertError) as exc:
        parser.exit(1, f'ERROR: {exc}\n')


if __name__ == '__main__':
    raise SystemExit(main())
