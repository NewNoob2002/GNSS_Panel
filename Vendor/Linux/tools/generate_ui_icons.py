#!/usr/bin/env python3
"""重绘共享 UI 原生尺寸图标，需要已有的 ImageMagick，不在固件运行时缩放。

仓库根目录用法：
    python3 Vendor/Linux/tools/generate_ui_icons.py             # 默认 v9 A8，写入正式 img
    python3 Vendor/Linux/tools/generate_ui_icons.py --v9        # 与默认行为相同
    python3 Vendor/Linux/tools/generate_ui_icons.py --v9 --format RGB565A8 --output-dir build/icons-v9-rgb
    python3 Vendor/Linux/tools/generate_ui_icons.py --v9 --output-dir build/icons-v9
    uv run Vendor/Linux/tools/generate_ui_icons.py --only sun  # 仅生成太阳的 16/32 px 资源

v9 复用 App/UI/Resource/tools/convert_images_v9.py 的编码和回读校验；
保留覆盖率、墨量及半透明像素比例自检。默认保留 A8 遮罩，颜色由主题 recolor 提供；
显式 RGB565A8 使用白色平面。
"""
from pathlib import Path
import argparse
import math
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3] / "App/UI/Resource/img"
FILL = 'fill="white" stroke="none"'


def rects(*items: tuple) -> str:
    """按像素网格填充的矩形 (x, y, w, h)，16 px 下边缘无灰阶。"""
    return "".join(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" {FILL}/>' for x, y, w, h in items)


def gear(teeth: int = 8, outer: float = 7.5, root: float = 5.6, hole: float = 2.3) -> str:
    """实心齿轮加中心孔；16 px 下描边齿轮的齿与内圈会糊成一片。"""
    points = []
    for i in range(teeth):
        center = 2 * math.pi * i / teeth - math.pi / 2
        half = math.pi / teeth / 2
        for angle, radius in ((center - half * 1.25, root), (center - half * 0.8, outer),
                              (center + half * 0.8, outer), (center + half * 1.25, root)):
            points.append(f"{8 + radius * math.cos(angle):.3f} {8 + radius * math.sin(angle):.3f}")
    path = "M" + "L".join(points) + "Z"
    path += f"M{8 + hole} 8a{hole} {hole} 0 1 0 {-2 * hole} 0a{hole} {hole} 0 1 0 {2 * hole} 0Z"
    return f'<path d="{path}" fill-rule="evenodd" {FILL}/>'


def wifi(arcs: int = 3) -> str:
    """以 (8,14) 为圆心的同心弧加 2x2 圆点；弧间留 1.5 px，16 px 下不粘连。参数保留给信号强度/连接动画变体。"""
    path = ""
    for radius in (4.5, 8, 11.5)[:arcs]:
        dx, dy = radius * math.sin(math.radians(42)), radius * math.cos(math.radians(42))
        path += f"M{8 - dx:.3f} {14 - dy:.3f}A{radius} {radius} 0 0 1 {8 + dx:.3f} {14 - dy:.3f}"
    return f'<path d="{path}" stroke-linecap="round"/>' + rects((7, 13, 2, 2))


# 原创 16x16 矢量路径；info 的三横线为现有 Device Info 图标的视觉基准。
# 默认 2 单位描边、方头、斜接：整数坐标上的横竖线正好覆盖整像素，只有斜线与弧线保留抗锯齿。
SATELLITE = ('<g transform="rotate(45 8 8)">' + rects((6, 5.5, 4, 5), (0, 6, 4.5, 4), (11.5, 6, 4.5, 4),
                                                     (4.5, 7.5, 1.5, 1), (10, 7.5, 1.5, 1))
             + '</g><path d="M1.5 11a3.5 3.5 0 0 0 3.5 3.5" stroke-width="1.5" stroke-linecap="round"/>')
# RTK 用户熟悉的 GNSS 接收机外形：半椭圆天线罩加整像素底板。基准站架在三脚架上，移动站装在对中杆上并挂手簿。
RECEIVER = f'<path d="M3 5a5 4 0 0 1 10 0z" {FILL}/>' + rects((2, 5, 12, 1))
BASE = RECEIVER + rects((7, 6, 2, 3)) + '<path d="M8 9l-5 6M8 9l5 6M8 9v6" stroke-linecap="round"/>'
ROVER = RECEIVER + rects((7, 6, 2, 8), (11, 8, 3, 5), (9, 10, 2, 1)) + f'<path d="M7 14h2l-1 2z" {FILL}/>'
# 名称 -> {像素尺寸: 路径}；16 px 与 32 px 按各自网格单独设计，32 px 输出名带 _large。
PATHS = {
    "info": {16: '<path d="M2 3h12M2 8h12M2 13h12"/>'},
    "work": {16: rects((1, 2, 14, 2), (4, 1, 4, 4), (1, 7, 14, 2), (9, 6, 4, 4), (1, 12, 14, 2), (5, 11, 4, 4))},
    "record": dict.fromkeys((16, 32), '<path d="M2 2h9l3 3v9H2z"/>' + rects((5, 3, 5, 3), (5, 10, 6, 2))),
    "settings": dict.fromkeys((16, 32), gear()),
    "sun": dict.fromkeys((16, 32), f'<circle cx="8" cy="8" r="3" {FILL}/>'
                         '<path d="M8 1v1M8 14v1M1 8h1M14 8h1M3 3l.75.75M12.25 12.25L13 13'
                         'M3 13l.75-.75M12.25 3.75L13 3" stroke-linecap="round"/>'),
    "base": dict.fromkeys((16, 32), BASE),
    "rover": dict.fromkeys((16, 32), ROVER),
    # 首页电台卡只画天线本体，收发波形由页面在两侧绘制，二者不重叠。
    "antenna": {16: '<path d="M8 6v8M8 8l-4 6M8 8l4 6" stroke-linecap="round" stroke-linejoin="round"/>'
                    f'<circle cx="8" cy="4" r="2" {FILL}/>'},
    "satellite": dict.fromkeys((16, 32), SATELLITE),
    "network": dict.fromkeys((16, 32), '<circle cx="8" cy="8" r="6"/><path d="M2 8h12"/>'
                                         '<ellipse cx="8" cy="8" rx="2" ry="6"/>'),
    "protocol": {16: '<path d="M1 8h3l2-5 3 10 2-5h4" stroke-linecap="butt" stroke-linejoin="round"/>'},
    "channel": {16: rects((2, 1, 2, 14), (1, 4, 4, 4), (7, 1, 2, 14), (6, 9, 4, 4), (12, 1, 2, 14), (11, 6, 4, 4))},
    "clock": {16: '<circle cx="8" cy="8" r="6"/><path d="M8 5v3h3" stroke-linecap="butt"/>'},
    "apply": {16: '<path d="M2 8l4 4 8-8" stroke-linecap="round" stroke-linejoin="round"/>'},
    "back": {16: '<path d="M7 3L2 8l5 5M2 8h12" stroke-linejoin="round"/>'},
    "play": {16: f'<path d="M4 2v12l9-6z" {FILL}/>'},
    "stop": {16: f'<rect x="3" y="3" width="10" height="10" {FILL}/>'},
    "power": dict.fromkeys((16, 32), '<path d="M8 1v6" stroke-linecap="butt"/><path d="M4.5 3.5a6 6 0 1 0 7 0" stroke-linecap="round"/>'),
    "wifi": {16: wifi()},
    # 状态栏原字体符号的替代：图片对象可直接做 recolor/opa/zoom/换 src 动画。
    "location": {16: f'<path d="M14 2L2 7.5l5.5 1 1 5.5z" {FILL.replace("stroke=\"none\"", "")} stroke-width="1.5" '
                     'stroke-linejoin="round"/>'},
    "sd_card": {16: '<path d="M6 2h6v12H4V4z"/>' + rects((7, 4, 1, 3), (9, 4, 1, 3))},
}
SUPERSAMPLE = 8
# 覆盖率低于 LOW 的淡边清零、高于 HIGH 的补满，只在两者之间保留过渡，避免小尺寸发虚。
LOW, HIGH = 48, 208
# 45° 小块拼成的图形在 16 px 用更窄的过渡带，近似像素画的整齐台阶，否则各块会糊成团。
SHARP = {"ui_satellite": (110, 146), "ui_sun": (110, 146)}


def render(path: str, size: int, low: int = LOW, high: int = HIGH) -> bytes:
    """Render and validate native alpha coverage. 渲染并校验原生尺寸覆盖率。"""
    svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{size * SUPERSAMPLE}" height="{size * SUPERSAMPLE}" '
           'viewBox="0 0 16 16"><g fill="none" stroke="white" stroke-width="2" stroke-linecap="square" '
           f'stroke-linejoin="miter">{path}</g></svg>')
    # -scale 按整数倍做盒式平均，即真实像素覆盖率；-resize 的 Lanczos 会产生振铃和外溢灰边。
    coverage = subprocess.run(["magick", "-background", "none", "svg:-", "-alpha", "extract",
                               "-scale", f"{size}x{size}!", "-depth", "8", "gray:-"], input=svg.encode(),
                              stdout=subprocess.PIPE, check=True).stdout
    pixels = bytes(0 if value <= low else 255 if value >= high else round((value - low) * 255 / (high - low))
                   for value in coverage)
    assert len(pixels) == size * size and max(pixels) == 255
    assert 0.1 < sum(pixels) / (255 * size * size) < 0.85
    return pixels


def main() -> None:
    """Generate v9 icons after pixel checks. 像素自检后输出正式 v9 资源。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--v9", action="store_true", default=True, help="兼容显式开关；默认输出 v9")
    parser.add_argument("--format", choices=("A8", "RGB565A8"), help="v9 目标格式（默认 A8）")
    parser.add_argument("--output-dir", type=Path, help="输出目录，默认 App/UI/Resource/img")
    parser.add_argument("--only", choices=PATHS, help="仅写入指定图标的全部原生尺寸，其他资源仅校验")
    args = parser.parse_args()
    v9_format = args.format or "A8"
    output = (args.output_dir or ROOT).resolve()
    sys.path.insert(0, str(ROOT.parent / "tools"))
    from convert_images_v9 import Image8, encode_v9, render_c_file, verify_text

    images = {}
    pending = {}
    for name, variants in PATHS.items():
        for size, path in variants.items():
            key = f"ui_{name}" + ("_large" if size == 32 else "")
            pixels = render(path, size, *SHARP.get(key, (LOW, HIGH)))
            images[key] = pixels
            rgba = b"".join(bytes((255, 255, 255, alpha)) for alpha in pixels)
            symbol = f"img_src_{key}"
            encoded = encode_v9(symbol, size, size, rgba, v9_format)
            source = render_c_file([encoded])
            original = Image8(ROOT / f"{symbol}.c", symbol, "LV_IMG_CF_ALPHA_8BIT",
                              size, size, len(pixels), rgba)
            errors = verify_text([original], source)
            if errors:
                raise RuntimeError("; ".join(errors))
            if not args.only or name == args.only:
                pending[f"{symbol}.c"] = (source, original)
    # 首页四个图标必须同尺寸且具有接近基准的有效墨量，避免再出现过小、发灰的入口。
    reference = sum(images["ui_info"])
    for name in ("work", "record", "settings"):
        ratio = sum(images[f"ui_{name}"]) / reference
        assert 0.65 < ratio < 1.8, (name, ratio)
    # 清晰度：半透明像素只占有效像素的少数，防止渲染链路退化回模糊缩放。
    for key, pixels in images.items():
        partial = sum(1 for value in pixels if 0 < value < 255)
        assert partial <= 0.5 * sum(1 for value in pixels if value), (key, partial)
    output.mkdir(parents=True, exist_ok=True)
    for filename, (source, original) in pending.items():
        target = output / filename
        newline = "\r\n" if target.exists() and b"\r\n" in target.read_bytes() else "\n"
        target.write_bytes(source.replace("\n", newline).encode("utf-8"))
        if args.v9:
            errors = verify_text([original], target.read_bytes().decode("utf-8"))
            if errors:
                raise RuntimeError("; ".join(errors))
    mode = f"v9 {v9_format}" if args.v9 else "alpha"
    print(f"Generated {len(pending)} and checked {len(images)} native-size {mode} icons.")


if __name__ == "__main__":
    main()
