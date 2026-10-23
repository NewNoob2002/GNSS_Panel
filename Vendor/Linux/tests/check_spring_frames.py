#!/usr/bin/env python3
"""独立检查保存的 Spring PPM 帧，逐帧匹配 geometry.csv 并检查边框四条直边。"""
import argparse
import csv
import json
from pathlib import Path

# kAccent 经 RGB565 量化，再按预览的 PPM 整数公式展开。
ACCENT = bytes((255, 36 * 255 // 63, 3 * 255 // 31))
WIDTH, HEIGHT, RADIUS = 294, 126, 8


def check(directory: Path) -> dict:
    """拒绝缺帧、缺边框、坐标不匹配及任一条直边被遮挡。"""
    with (directory / "geometry.csv").open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    assert rows, f"{directory}: geometry.csv 为空"
    files = sorted(directory.glob("frame-*.ppm"))
    assert len(files) == len(rows), f"{directory}: 帧数与 CSV 行数不匹配"
    minimum = WIDTH * HEIGHT
    for index, row in enumerate(rows):
        assert int(row["frame"]) == index and int(row["time_ms"]) == index * 20
        path = directory / f"frame-{index:04d}.ppm"
        with path.open("rb") as stream:
            assert stream.readline() == b"P6\n", path
            assert stream.readline() == b"294 126\n", path
            assert stream.readline() == b"255\n", path
            pixels = stream.read()
        assert len(pixels) == WIDTH * HEIGHT * 3, f"{path}: 像素长度错误"
        points = [(x, y) for y in range(HEIGHT) for x in range(100, WIDTH)
                  if pixels[(y * WIDTH + x) * 3:(y * WIDTH + x + 1) * 3] == ACCENT]
        assert points, f"{path}: 右侧没有任何高亮边框像素"
        bounds = (min(x for x, _ in points), min(y for _, y in points),
                  max(x for x, _ in points), max(y for _, y in points))
        expected = tuple(int(row[key]) for key in ("x1", "y1", "x2", "y2"))
        assert bounds == expected, f"{path}: 可见边框 {bounds} 与 CSV {expected} 不一致"
        x1, y1, x2, y2 = expected
        edges = ([(x, y) for x in range(x1 + RADIUS, x2 - RADIUS + 1) for y in (y1, y2)]
                 + [(x, y) for y in range(y1 + RADIUS, y2 - RADIUS + 1) for x in (x1, x2)])
        assert all(pixels[(y * WIDTH + x) * 3:(y * WIDTH + x + 1) * 3] == ACCENT for x, y in edges), \
            f"{path}: 覆盖框直边被遮挡"
        if "accent_pixels" in row:
            assert int(row["accent_pixels"]) == len(points), f"{path}: 边框像素数量与 CSV 不一致"
        minimum = min(minimum, len(points))
    return {"directory": str(directory), "frames": len(rows), "passed": len(rows),
            "minimum_accent_pixels": minimum, "missing": 0, "geometry_mismatch": 0, "occluded_edges": 0}


def main() -> None:
    """输出机器可读结果，任一断言失败即返回非零状态。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directories", nargs="+", type=Path)
    parser.add_argument("--report", type=Path)
    arguments = parser.parse_args()
    results = [check(directory) for directory in arguments.directories]
    text = json.dumps(results, ensure_ascii=False, indent=2) + "\n"
    if arguments.report:
        arguments.report.write_text(text, encoding="utf-8")
    print(text, end="")


if __name__ == "__main__":
    main()
