#!/usr/bin/env python3
"""build.py 回归：python3 -B tests/test_build_script.py。"""
import datetime
import importlib.util
import json
import os
import tempfile
import unittest
from pathlib import Path
from unittest import mock

SCRIPT = Path(__file__).resolve().parent.parent / "build.py"
spec = importlib.util.spec_from_file_location("build_script", SCRIPT)
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)


def frozen_now(moment):
    """固定 datetime.now()，模拟时钟回拨。"""

    class Frozen(datetime.datetime):
        @classmethod
        def now(cls, tz=None):
            return moment

    return mock.patch.object(build.datetime, "datetime", Frozen)


class LogRotationTests(unittest.TestCase):
    """本次目录必须保留，历史目录按时间和数值后缀淘汰。"""

    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        patcher = mock.patch.object(build, "LOG_ROOT", self.root)
        patcher.start()
        self.addCleanup(patcher.stop)

    def test_clock_rollback_keeps_current_run(self):
        """历史目录的时间都更晚时，新目录仍存在且 latest 有效。"""
        for second in range(build.KEEP_RUNS):
            (self.root / f"20261102-1200{second:02d}").mkdir()
        with frozen_now(datetime.datetime(2026, 11, 1, 12, 0, 0)):
            run_dir = build.prepare_log_dir()
        self.assertTrue(run_dir.is_dir())
        self.assertEqual((self.root / "latest").resolve(), run_dir.resolve())
        self.assertEqual(sum(p.is_dir() and not p.is_symlink() for p in self.root.iterdir()), build.KEEP_RUNS)

    def test_same_second_suffix_is_numeric(self):
        """同一秒的目录按数值淘汰，_10 比 _2 新。"""
        for suffix in ("", "_2", "_10"):
            (self.root / f"20261101-120000{suffix}").mkdir()
        with mock.patch.object(build, "KEEP_RUNS", 2), frozen_now(datetime.datetime(2026, 11, 1, 12, 0, 1)):
            build.prepare_log_dir()
        names = sorted(p.name for p in self.root.iterdir() if not p.is_symlink())
        self.assertEqual(names, ["20261101-120000_10", "20261101-120001"])


class ArtifactTests(unittest.TestCase):
    """固件产物以配置时导出的名称为准。"""

    def test_stale_newer_elf_is_ignored(self):
        """版本切回后，残留的新版本 ELF 时间更晚也不能被选中。"""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "generated").mkdir()
            (root / "generated" / build.FIRMWARE_NAME_FILE).write_text("MCU_3.0.0\n", encoding="utf-8")
            (root / "MCU_3.0.0.elf").touch()
            stale = root / "MCU_3.1.0.elf"
            stale.touch()
            os.utime(stale, (4102444800, 4102444800))
            target = build.Target("x", "x", "hc32", root)
            self.assertEqual([p.name for p in build.find_artifacts(target)],
                             ["MCU_3.0.0.elf", "MCU_3.0.0.hex", "MCU_3.0.0.bin"])

    def test_missing_manifest_reports_nothing(self):
        """清单缺失时不猜测产物。"""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "OLD.elf").touch()
            self.assertEqual(build.find_artifacts(build.Target("x", "x", "hc32", root)), [])


class CacheStatsTests(unittest.TestCase):
    """命中率只来自本目标的统计文件。"""

    def test_counts_events_and_removes_file(self):
        """按结果事件计数，其余事件不计入。"""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "stats"
            path.write_text("# a.c\ndirect_cache_hit\nlocal_storage_hit\n# b.c\npreprocessed_cache_hit\n"
                            "# c.c\ncache_miss\ndirect_cache_miss\n# d.c\ncalled_for_preprocessing\n")
            self.assertEqual(build.ccache_stats(path), {"hit": 2, "miss": 1})
            self.assertFalse(path.exists())

    def test_no_compilation_has_no_stats(self):
        """无改动或未安装 ccache 时没有统计文件。"""
        self.assertIsNone(build.ccache_stats(Path("/nonexistent/stats")))


def sample_targets():
    names = ("gcc-debug", "x1pro-debug", "x1pro-release", "xbase-debug", "xbase-release")
    targets = {name: build.Target(name, name, "hc32", Path(name)) for name in names}
    targets["linux-host"] = build.Target("linux-host", "Linux", "linux", Path("linux-host"))
    return targets


class MenuTests(unittest.TestCase):
    """菜单只负责拼出命令行参数。"""

    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        patcher = mock.patch.object(build, "LOG_ROOT", self.root)
        patcher.start()
        self.addCleanup(patcher.stop)

    def answer(self, *replies):
        with mock.patch("builtins.input", side_effect=replies), mock.patch("builtins.print"):
            return build.prompt_argv(sample_targets())

    def test_numbers_map_to_menu_without_alias(self):
        """编号按菜单顺序，兼容别名不进菜单；全是 preset 时不询问配置。"""
        self.assertEqual(self.answer("1 5", ""), ["x1pro-debug", "linux-host"])

    def test_alias_still_accepted_by_name(self):
        self.assertEqual(self.answer("gcc-debug", ""), ["gcc-debug"])

    def test_shorthand_asks_config(self):
        self.assertEqual(self.answer("x1pro hc32", "r", "t c"), ["x1pro", "hc32", "-c", "release", "--test", "--clean"])

    def test_invalid_input_reprompts(self):
        """非法输入不退出，编号越界按未知名称处理。"""
        self.assertEqual(self.answer("99", "nope 1", "1", "x", ""), ["x1pro-debug"])

    def test_oem_option(self):
        with mock.patch.object(build, "oem_names", return_value=["demo", "rgk"]):
            self.assertEqual(self.answer("1", "o", "bad", "2"), ["x1pro-debug", "-D", "OEM=rgk"])

    def test_enter_repeats_last_command(self):
        """回车原样重复，不再追问其他选项。"""
        build.save_last_command(["x1pro-release", "-D", "OEM=a b"])
        self.assertEqual(self.answer(""), ["x1pro-release", "-D", "OEM=a b"])

    def test_enter_without_last_reprompts(self):
        self.assertEqual(self.answer("", "1", ""), ["x1pro-debug"])

    def test_corrupt_last_command_is_ignored(self):
        (self.root / build.LAST_COMMAND_NAME).write_text('{"not": "a list"}', encoding="utf-8")
        self.assertIsNone(build.load_last_command())


class InterruptTests(unittest.TestCase):
    """Ctrl+C 后仍要写出汇总，已完成的结果不能丢。"""

    def test_interrupt_writes_summary(self):
        done = {"target": "x1pro-debug", "group": "hc32", "status": "ok", "seconds": {"total": 1.0}, "warnings": 0}
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with mock.patch.object(build, "LOG_ROOT", root), \
                    mock.patch.object(build.sys, "argv", ["build.py", "x1pro-debug", "xbase-debug", "linux-host"]), \
                    mock.patch.object(build, "load_presets", return_value=sample_targets()), \
                    mock.patch.object(build.shutil, "which", return_value="/usr/bin/tool"), \
                    mock.patch.object(build, "build_target", side_effect=[done, KeyboardInterrupt]), \
                    mock.patch("builtins.print"):
                self.assertEqual(build.main(), 130)
            summary = json.loads((root / "latest" / "summary.json").read_text(encoding="utf-8"))
            self.assertEqual([item["status"] for item in summary["results"]], ["ok", "skipped", "skipped"])
            self.assertEqual(json.loads((root / build.LAST_COMMAND_NAME).read_text(encoding="utf-8")),
                             ["x1pro-debug", "xbase-debug", "linux-host"])


if __name__ == "__main__":
    unittest.main()
