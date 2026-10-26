#!/usr/bin/env python3
"""用现有无窗口 LVGL 回归生成文档 PNG；仅依赖 CMake、主机编译器和 Python 标准库。

仓库根目录用法：
    python3 Vendor/Linux/tools/generate_doc_previews.py --build-dir build/doc-previews

原始 PPM 和回归日志保存在构建目录，docs/images 只写入四张 PNG。
"""

import argparse
import struct
import subprocess
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
NATIVE_WIDTH, NATIVE_HEIGHT = 294, 126
SCALE = 2
WIDTH, HEIGHT = NATIVE_WIDTH * SCALE, NATIVE_HEIGHT * SCALE
MARGIN, GAP = 12, 24


def run(*args: str) -> None:
    subprocess.run(args, cwd=ROOT, check=True)


def render(executable: Path, output: Path, *args: str) -> None:
    """回归断言照常执行；失败时输出对应日志路径，避免静默使用旧截图。"""
    log = output.parent / (output.name + ".log")
    with log.open("w") as stream:
        result = subprocess.run(
            [str(executable), str(output), *args],
            cwd=ROOT,
            stdout=stream,
            stderr=subprocess.STDOUT,
            check=False,
        )
    if result.returncode:
        raise RuntimeError(f"{executable.name} 失败（{result.returncode}），见 {log}")


def read_frame(path: Path) -> bytes:
    """读取回归 harness 的 P6 帧，最近邻放大，不插值或改写 UI 内容。"""
    magic, size, maximum, pixels = path.read_bytes().split(b"\n", 3)
    if (
        magic != b"P6"
        or size != b"294 126"
        or maximum != b"255"
        or len(pixels) != NATIVE_WIDTH * NATIVE_HEIGHT * 3
    ):
        raise ValueError(f"截图格式或尺寸异常：{path}")
    rows = []
    for y in range(NATIVE_HEIGHT):
        start = y * NATIVE_WIDTH * 3
        row = b"".join(
            pixels[x : x + 3] * SCALE for x in range(start, start + NATIVE_WIDTH * 3, 3)
        )
        rows.extend([row] * SCALE)
    return b"".join(rows)


def chunk(kind: bytes, data: bytes) -> bytes:
    return (
        struct.pack(">I", len(data))
        + kind
        + data
        + struct.pack(">I", zlib.crc32(kind + data))
    )


def write_png(path: Path, width: int, height: int, pixels: bytes) -> None:
    rows = b"".join(
        b"\0" + pixels[y * width * 3 : (y + 1) * width * 3] for y in range(height)
    )
    png = (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">2I5B", width, height, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(rows, 9))
        + chunk(b"IEND", b"")
    )
    if len(png) >= 100_000:
        raise ValueError(f"文档图片超过 100 KB：{path}")
    path.write_bytes(png)
    print(f"{path.relative_to(ROOT)}: {width}×{height}, {len(png)} B")


def montage(paths: list[Path]) -> tuple[int, int, bytes]:
    if not paths or len(paths) % 2:
        raise ValueError("拼图必须包含完整的两列")
    rows = len(paths) // 2
    width = WIDTH * 2 + MARGIN * 2 + GAP
    height = HEIGHT * rows + MARGIN * 2 + GAP * (rows - 1)
    canvas = bytearray(bytes.fromhex("202020") * width * height)
    for index, path in enumerate(paths):
        pixels = read_frame(path)
        x = MARGIN + index % 2 * (WIDTH + GAP)
        y = MARGIN + index // 2 * (HEIGHT + GAP)
        for row in range(HEIGHT):
            start = ((y + row) * width + x) * 3
            canvas[start : start + WIDTH * 3] = pixels[
                row * WIDTH * 3 : (row + 1) * WIDTH * 3
            ]
    return width, height, bytes(canvas)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=Path("build/doc-previews"))
    args = parser.parse_args()
    build = (ROOT / args.build_dir).resolve()
    run("cmake", "--preset", "linux-host", "-B", str(build), "-DOEM=singularxyz")
    targets = [
        "app_system_dash_test",
        "app_startup_power_test",
        "app_focus_rollout_preview",
    ]
    run("cmake", "--build", str(build), "--parallel", "4", "--target", *targets)
    shots = build / "doc-preview"
    shots.mkdir(exist_ok=True)
    executables = build / "Vendor/Linux/tests"
    render(executables / targets[0], shots / "pages")
    render(executables / targets[1], shots / "startup")
    render(executables / targets[2], shots / "dialplate", "Dialplate", "stills", "en")

    pages = shots / "pages"
    startup = shots / "startup"
    dialplate = shots / "dialplate/focus-3.ppm"
    infos = pages / "SystemInfos-Reopen-en.ppm"
    output = ROOT / "docs/images"
    output.mkdir(exist_ok=True)
    # 开机帧每 100 ms 一张；第 11 帧为逐字浮现完成、尚未开始自检的 Logo。
    overview = [
        startup / "Startup-charging-en.ppm",
        pages / "Boot-frame-en-011.ppm",
        dialplate,
        pages / "StarMap-no-outline-en.ppm",
        pages / "WorkSettings-new-en.ppm",
        pages / "RecordConfig-new-en.ppm",
        infos,
        pages / "SystemDash-hero-focused-60-en.ppm",
    ]
    bilingual = []
    # 每行左英右俄：亮度主卡、待机功率、按住确认窗、工作设置、记录设置。
    for directory, name in [
        (pages, "SystemDash-hero-focused-60"),
        (startup, "Startup-charging"),
        (pages, "Power-Hold"),
        (pages, "WorkSettings-new"),
        (pages, "RecordConfig-new"),
    ]:
        bilingual.extend(
            directory / f"{name}-{language}.ppm" for language in ("en", "ru")
        )
    # 先读完所有帧，再更新文档图片；截图失败不会被误认为成功。
    images = [
        ("ui-pages.png", montage(overview)),
        ("ui-settings-bilingual.png", montage(bilingual)),
        ("ui-dialplate.png", (WIDTH, HEIGHT, read_frame(dialplate))),
        ("ui-device-info.png", (WIDTH, HEIGHT, read_frame(infos))),
    ]
    for name, (width, height, pixels) in images:
        write_png(output / name, width, height, pixels)


if __name__ == "__main__":
    main()
