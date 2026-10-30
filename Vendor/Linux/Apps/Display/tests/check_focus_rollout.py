#!/usr/bin/env python3
"""逐字节比较双语页面基线，只允许获批四角及 WorkSettings 四像素特例。"""
import argparse
import csv
import json
from pathlib import Path

ENABLED = {"Dialplate", "WorkSettings", "RecordConfig"}
WIDTH, HEIGHT = 294, 126


def compare(baseline: Path, after: Path) -> list[dict]:
    """返回每张图片的差异计数；白名单外任一字节变化立即失败。"""
    baseline_files = sorted(path.relative_to(baseline) for path in baseline.glob("*/*/*.ppm"))
    after_files = sorted(path.relative_to(after) for path in after.glob("*/*/*.ppm"))
    for directory in ("SystemDash-states",):
        baseline_files.extend(sorted(path.relative_to(baseline) for path in (baseline / directory).glob("*.ppm")))
        after_files.extend(sorted(path.relative_to(after) for path in (after / directory).glob("*.ppm")))
    assert baseline_files and baseline_files == after_files, "双语截图缺失或多出"
    results = []
    for relative in baseline_files:
        state_capture = len(relative.parts) == 2
        if state_capture:
            page, name = relative.parts
            language = "ru" if name.endswith("-ru.ppm") else "en"
        else:
            page, language, name = relative.parts
        original = (baseline / relative).read_bytes()
        updated = (after / relative).read_bytes()
        header = b"P6\n294 126\n255\n"
        assert original.startswith(header) and updated.startswith(header), relative
        original, updated = original[len(header):], updated[len(header):]
        assert len(original) == len(updated) == WIDTH * HEIGHT * 3, relative
        corners = set()
        exception = set()
        if page in ENABLED and name.startswith("focus-"):
            with (baseline / page / language / "focus.csv").open(newline="", encoding="utf-8") as stream:
                row = next(row for row in csv.DictReader(stream) if row["name"] == Path(name).stem)
            with (after / page / language / "focus.csv").open(newline="", encoding="utf-8") as stream:
                actual = next(row for row in csv.DictReader(stream) if row["name"] == Path(name).stem)
            assert row == actual, f"{relative}: 终态聚焦几何改变"
            x1, y1, x2, y2 = (int(row[key]) for key in ("x1", "y1", "x2", "y2"))
            corners = {(x, y) for x in (*range(x1, x1 + 9), *range(x2 - 8, x2 + 1))
                       for y in (*range(y1, y1 + 9), *range(y2 - 8, y2 + 1))}
            # 主控 msg_341b23b58e32 回复批准：基线状态点压在底边上，完整覆盖框修正此遮挡。
            if page == "WorkSettings" and Path(name).stem == "focus-0":
                assert (x1, y1, x2, y2) == (8, 28, 95, 104), relative
                exception = {(x, 103) for x in range(52, 56)}
        elif state_capture:
            geometry = None
            if name.startswith(("WorkSettings-new-", "NetworkBase-new-")):
                geometry = (8, 28, 95, 104)
                exception = {(x, 103) for x in range(52, 56)}
            elif name.startswith(("RecordConfig-new-", "Recording-new-")):
                geometry = (102, 82, 190, 104)
            if geometry:
                x1, y1, x2, y2 = geometry
                corners = {(x, y) for x in (*range(x1, x1 + 9), *range(x2 - 8, x2 + 1))
                           for y in (*range(y1, y1 + 9), *range(y2 - 8, y2 + 1))}
        changed = corner_pixels = special_pixels = outside = max_corner_error = max_special_error = 0
        for pixel in range(WIDTH * HEIGHT):
            offset = pixel * 3
            before, current = original[offset:offset + 3], updated[offset:offset + 3]
            if before == current:
                continue
            changed += 1
            point = pixel % WIDTH, pixel // WIDTH
            error = max(abs(a - b) for a, b in zip(before, current))
            if point in exception:
                special_pixels += 1
                max_special_error = max(max_special_error, error)
            elif point in corners:
                corner_pixels += 1
                max_corner_error = max(max_corner_error, error)
                assert error <= 33, f"{relative}: {point} 通道差 {error} 超过 33"
            else:
                outside += 1
        assert outside == 0, f"{relative}: 白名单外 {outside} 个差异像素"
        results.append({"file": str(relative), "changed_pixels": changed, "corner_pixels": corner_pixels,
                        "max_corner_channel_error": max_corner_error, "exception_pixels": special_pixels,
                        "max_exception_channel_error": max_special_error, "outside_whitelist_pixels": outside})
    return results


def main() -> None:
    """生成可复查 JSON 证明，任何超限差异返回非零退出状态。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("after", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    results = compare(args.baseline, args.after)
    report = {"images": len(results), "outside_whitelist_pixels": 0,
              "max_corner_channel_error": max(result["max_corner_channel_error"] for result in results),
              "exception_pixels": sum(result["exception_pixels"] for result in results), "results": results}
    text = json.dumps(report, ensure_ascii=False, indent=2) + "\n"
    if args.report:
        args.report.write_text(text, encoding="utf-8")
    print(text, end="")


if __name__ == "__main__":
    main()
