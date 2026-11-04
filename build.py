#!/usr/bin/env python3
"""GNSS_Panel 统一编译入口：编译单个或多个固件，输出编译前后信息并保存日志。

目标来自 CMakePresets.json，新增产品或 preset 无需修改本脚本。

    ./build.py                            终端内进入交互菜单，回车可重复上次命令
    ./build.py --list                     列出全部目标
    ./build.py x1pro                      X1Pro Debug
    ./build.py x1pro xbase -c release     两个固件的 Release
    ./build.py hc32 -c all                全部固件的 Debug 与 Release
    ./build.py tests linux-host --test    编译并运行主机测试
    ./build.py x1pro-release -D OEM=demo  向 CMake 透传缓存变量

退出码：全部成功为 0；有目标失败为 1；参数或环境错误为 2；Ctrl+C 中断为 130。
"""

import argparse
import datetime
import hashlib
import json
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
LOG_ROOT = ROOT / "build" / "logs"
KEEP_RUNS = 20  # 只清理本脚本创建的时间戳目录，避免日志无限增长
MAX_ERRORS = 20  # 失败时终端只显示前若干条，完整内容见日志
REPORT_NAME = ".build_report.json"
FIRMWARE_NAME_FILE = "firmware_name.txt"  # 由 Vendor/HC32/CMakeLists.txt 在配置时写入
LAST_COMMAND_NAME = "last_command.json"
OEM_ROOT = ROOT / "App" / "UI" / "Resource" / "oem"

# 没有 preset 的独立 CMake 工程：preset 的 sourceDir 固定为仓库根目录。
STANDALONE = {
    "host-tests": ("tests", "HC32 主机回归测试"),
    "app-core-tests": ("App/tests", "App 无屏核心测试"),
}
# gcc-* 是 x1pro-* 的兼容别名，可显式指定，但不进入分组，否则同一固件会编译两次。
ALIAS_PREFIX = "gcc-"
GROUPS = ("hc32", "simulator", "linux", "tests", "all")
MENU_OPTIONS = {"c": ("--clean", "先清理"), "t": ("--test", "运行测试"),
                "f": ("--fail-fast", "失败即停"), "v": ("--verbose", "完整输出")}
HOST_ARTIFACTS = (
    "bin/main",
    "Vendor/Linux/Apps/Display/rk3506_app",
    "Vendor/Linux/Apps/Survey/survey_app",
)

ANSI = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]")
PROGRESS = re.compile(r"^\[(\d+)/(\d+)\]")
# 同时匹配中文诊断：不能用 LC_ALL=C 统一语言，ccache 会把它计入哈希，
# 导致脚本构建与手动 cmake --build 互不命中。
WARNING = re.compile(r": (warning|警告)[:：] ?")
ERROR = re.compile(r"(: (fatal |致命)?(error|错误)[:：] ?|^FAILED: |undefined reference|^CMake Error"
                   r"|ninja: error|\(Failed\)$)")
TESTS = re.compile(r"\d+% tests passed, .*")
MEMORY = re.compile(r"^\s*(FLASH|RAM):\s+([\d.]+ [KM]?B)\s+([\d.]+ [KM]?B)\s+([\d.]+)%")


def usage_error(message):
    print(f"错误：{message}", file=sys.stderr)
    sys.exit(2)


class Target:
    """一个编译单元：一个 preset 或一个独立工程。"""

    def __init__(self, name, title, group, build_dir, source=None):
        self.name = name
        self.title = title
        self.group = group
        self.build_dir = build_dir
        self.source = source  # None 表示通过 preset 配置


def load_presets():
    """读取非隐藏 preset，并沿 inherits 解析 PLATFORM、PRODUCT 与工具链。"""
    try:
        data = json.loads((ROOT / "CMakePresets.json").read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        usage_error(f"无法读取 CMakePresets.json：{exc}")
    table = {p["name"]: p for p in data["configurePresets"]}

    def resolve(preset):
        merged = {"cacheVariables": {}, "toolchainFile": None}
        parents = preset.get("inherits", [])
        for parent in [parents] if isinstance(parents, str) else reversed(parents):
            base = resolve(table[parent])
            merged["cacheVariables"].update(base["cacheVariables"])
            merged["toolchainFile"] = base["toolchainFile"] or merged["toolchainFile"]
        merged["cacheVariables"].update(preset.get("cacheVariables", {}))
        merged["toolchainFile"] = preset.get("toolchainFile") or merged["toolchainFile"]
        return merged

    targets = {}
    for preset in data["configurePresets"]:
        if preset.get("hidden"):
            continue
        name = preset["name"]
        cache = resolve(preset)["cacheVariables"]
        group = cache.get("PLATFORM", "HC32").lower()
        targets[name] = Target(name, preset.get("displayName", name), group, ROOT / "build" / name)
    for name, (source, title) in STANDALONE.items():
        targets[name] = Target(name, title, "tests", ROOT / "build" / name, source)
    return targets


def firmware_targets(targets):
    return [t for t in targets.values() if t.group == "hc32" and not t.name.startswith(ALIAS_PREFIX)]


def product_names(targets):
    return sorted({t.name.rsplit("-", 1)[0] for t in firmware_targets(targets)})


def expand(names, config, targets):
    """把命令行名称展开为去重后的目标列表，保持输入顺序。"""
    configs = ("debug", "release") if config == "all" else (config,)
    firmware = firmware_targets(targets)
    groups = {
        "hc32": [t.name for t in firmware if t.name.endswith(configs)],
        "simulator": [t.name for t in targets.values() if t.group == "simulator"],
        "linux": [t.name for t in targets.values() if t.group == "linux"],
        "tests": [t.name for t in targets.values() if t.group == "tests"],
    }
    groups["all"] = groups["hc32"] + groups["simulator"] + groups["linux"] + groups["tests"]
    products = product_names(targets)

    selected = []
    for name in names:
        if name in groups:
            selected += groups[name]
        elif name in targets:
            selected.append(name)
        elif name in products:
            selected += [f"{name}-{c}" for c in configs]
        else:
            usage_error(f"未知目标 '{name}'，用 --list 查看可用目标")
    return [targets[n] for n in dict.fromkeys(selected)]


def capture(*command, cwd=ROOT):
    """运行只读查询命令；工具缺失或失败时返回空串，信息展示不应中断编译。"""
    try:
        result = subprocess.run(command, cwd=cwd, capture_output=True, text=True, errors="replace")
    except OSError:
        return ""
    return result.stdout.strip() if result.returncode == 0 else ""


def first_line(text):
    return text.splitlines()[0] if text else "未找到"


def print_environment(targets, args):
    """编译前：记录源码版本与工具链，日志脱离终端后仍能追溯构建环境。"""
    dirty = len(capture("git", "status", "--porcelain").splitlines())
    print("=" * 72)
    print(f"开始时间  {datetime.datetime.now():%Y-%m-%d %H:%M:%S}")
    print(f"源码      {capture('git', 'rev-parse', '--abbrev-ref', 'HEAD')}"
          f" @ {capture('git', 'rev-parse', '--short', 'HEAD')}"
          f"（未提交改动 {dirty} 个文件）")
    print(f"主机      {platform.node()}  {platform.system()} {platform.machine()}"
          f"  {os.cpu_count()} 线程  并行 {args.jobs or '默认'}")
    print(f"CMake     {first_line(capture('cmake', '--version'))}")
    print(f"Ninja     {first_line(capture('ninja', '--version'))}")
    if any(t.group == "hc32" for t in targets):
        print(f"ARM GCC   {first_line(capture('arm-none-eabi-gcc', '--version'))}")
    if any(t.group != "hc32" for t in targets):
        print(f"主机 GCC  {first_line(capture('gcc', '--version'))}")
    print(f"ccache    {first_line(capture('ccache', '--version'))}")
    print(f"目标      {len(targets)} 个：{' '.join(t.name for t in targets)}")
    print("=" * 72)


def ccache_stats(path):
    """汇总本目标编译进程写入的 CCACHE_STATSLOG。

    不用 ccache 的全局计数做差：共享缓存的其他项目会混入，并发清零还会得到负值。
    没有文件表示未调用 ccache（无改动或未安装），返回 None。
    """
    try:
        events = path.read_text(errors="replace").splitlines()
    except OSError:
        return None
    path.unlink()
    return {"hit": events.count("direct_cache_hit") + events.count("preprocessed_cache_hit"),
            "miss": events.count("cache_miss")}


class Step:
    """运行一个子命令，完整输出写入日志，并统计进度、告警和错误。"""

    def __init__(self, log, verbose):
        self.log = log
        self.verbose = verbose
        self.warnings = set()
        self.errors = []
        self.memory = {}
        self.tests = ""
        self.compiled = 0

    def run(self, command, label, env=None):
        self.log.write(f"\n$ {' '.join(map(str, command))}\n")
        self.log.flush()
        started = time.monotonic()
        try:
            process = subprocess.Popen(command, cwd=ROOT, env=env, stdout=subprocess.PIPE,
                                       stderr=subprocess.STDOUT, text=True, errors="replace", bufsize=1)
        except OSError as exc:
            self.errors.append(f"无法启动 {command[0]}：{exc}")
            return False, 0.0
        try:
            for raw in process.stdout:
                line = ANSI.sub("", raw).rstrip()
                self.log.write(line + "\n")
                self.scan(line, label)
        except KeyboardInterrupt:
            # Ctrl+C 同时发给了子进程组：继续读管道等 Ninja 收尾，否则它写满管道会卡住。
            process.communicate()
            self.clear_progress()
            self.log.write("# 用户中断\n")
            raise
        code = process.wait()
        self.clear_progress()
        self.log.write(f"# 退出码 {code}，耗时 {time.monotonic() - started:.1f}s\n")
        self.log.flush()
        return code == 0, time.monotonic() - started

    def scan(self, line, label):
        if WARNING.search(line):
            self.warnings.add(line)
        if ERROR.search(line):
            self.errors.append(line)
        if TESTS.search(line):
            self.tests = line.strip()
        match = MEMORY.match(line)
        if match:
            self.memory[match[1]] = {"used": match[2], "total": match[3], "percent": float(match[4])}
        progress = PROGRESS.match(line)
        if progress:
            self.compiled = int(progress[1])
        if self.verbose:
            print(line, flush=True)
        elif progress and sys.stdout.isatty():
            width = shutil.get_terminal_size().columns - 1
            print(f"\r  {label} {progress[0]}".ljust(width)[:width], end="", flush=True)

    def clear_progress(self):
        if not self.verbose and sys.stdout.isatty():
            print("\r" + " " * (shutil.get_terminal_size().columns - 1) + "\r", end="", flush=True)


def read_cache(build_dir):
    """从 CMakeCache 取实际生效的配置，而不是 preset 的期望值（如 -DOEM 已写入缓存）。"""
    wanted = ("CMAKE_BUILD_TYPE", "PRODUCT", "OEM", "SIM_PRODUCT", "SIM_BACKEND", "HC32_LOG_OUTPUT")
    values = {}
    try:
        for line in (build_dir / "CMakeCache.txt").read_text(errors="replace").splitlines():
            key, sep, value = line.partition("=")
            name = key.split(":")[0]
            if sep and name in wanted and value:
                values[name] = value
    except OSError:
        pass
    return values


def find_artifacts(target):
    """固件名取自配置时导出的清单：版本切换后目录里会残留其他版本，文件时间不可靠。"""
    if target.group == "hc32":
        generated = target.build_dir / "generated"
        try:
            name = (generated / FIRMWARE_NAME_FILE).read_text(encoding="utf-8").strip()
        except OSError:
            return []
        return [target.build_dir / f"{name}.elf", generated / f"{name}.hex", generated / f"{name}.bin"]
    return [target.build_dir / item for item in HOST_ARTIFACTS]


def elf_size(elf, group):
    tool = "arm-none-eabi-size" if group == "hc32" else "size"
    fields = capture(tool, str(elf)).splitlines()
    if len(fields) < 2:
        return None
    text, data, bss = fields[1].split()[:3]
    return {"text": int(text), "data": int(data), "bss": int(bss)}


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_report(build_dir):
    try:
        return json.loads((build_dir / REPORT_NAME).read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return {}


def signed(value):
    return f"{value:+d} 字节" if value else "无变化"


def describe_size(size, previous):
    text = f"text {size['text']}  data {size['data']}  bss {size['bss']}"
    if not previous:
        return text + "（无历史记录）"
    flash = size["text"] + size["data"] - previous["text"] - previous["data"]
    ram = size["data"] + size["bss"] - previous["data"] - previous["bss"]
    return f"{text}（较上次：Flash {signed(flash)}，RAM {signed(ram)}）"


def build_target(target, args, run_dir):
    """配置、编译、（可选）测试单个目标，返回写入汇总的结果。"""
    cmake = ["cmake"]
    configure = cmake + (["--preset", target.name] if target.source is None else
                         ["-S", target.source, "-B", target.build_dir, "-G", "Ninja"])
    configure += [f"-D{item}" for item in args.define]
    build = cmake + ["--build"] + (["--preset", target.name] if target.source is None else [target.build_dir])
    if args.jobs:
        build += ["-j", str(args.jobs)]
    if args.clean:
        build.append("--clean-first")
    test = ["ctest", "--test-dir", target.build_dir, "--output-on-failure"]

    if args.dry_run:
        for command in (configure, build) + ((test,) if args.test else ()):
            print("  $ " + " ".join(map(str, command)))
        return None

    log_path = run_dir / f"{target.name}.log"
    previous = load_report(target.build_dir)
    result = {"target": target.name, "title": target.title, "group": target.group,
              "status": "failed", "log": str(log_path),
              "warnings": 0, "seconds": {}, "artifacts": []}
    started = time.monotonic()
    stats_path = run_dir / f"{target.name}.ccache-stats"

    with log_path.open("w", encoding="utf-8") as log:
        log.write(f"# {target.name}（{target.title}）{datetime.datetime.now():%Y-%m-%d %H:%M:%S}\n")
        step = Step(log, args.verbose)
        ok, result["seconds"]["configure"] = step.run(configure, "配置")
        config = read_cache(target.build_dir)
        result["config"] = config
        if config:
            print("  配置  " + "  ".join(f"{k}={v}" for k, v in config.items()))
        if ok:
            ok, result["seconds"]["build"] = step.run(build, "编译", dict(os.environ, CCACHE_STATSLOG=str(stats_path)))
        cache = ccache_stats(stats_path)
        # 分组里可能混入没有注册测试的目标（如 HC32 固件），这里跳过而非判失败。
        if ok and args.test and (target.build_dir / "CTestTestfile.cmake").exists():
            ok, result["seconds"]["test"] = step.run(test, "测试")
    result["seconds"]["total"] = time.monotonic() - started
    result["warnings"] = len(step.warnings)
    result["steps"] = step.compiled
    result["tests"] = step.tests
    result["cache"] = cache
    result["status"] = "ok" if ok else "failed"

    if ok:
        collect_outputs(target, result, step, previous)
    report_target(result, step, previous)
    if ok:
        (target.build_dir / REPORT_NAME).write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    return result


def collect_outputs(target, result, step, previous):
    artifacts = [path for path in find_artifacts(target) if path.is_file()]
    result["artifacts"] = [{"path": str(path.relative_to(ROOT)), "bytes": path.stat().st_size,
                            "sha256": sha256(path)} for path in artifacts]
    if artifacts:
        result["size"] = elf_size(artifacts[0], target.group)
    # 无改动时不会重新链接，日志里没有内存占用，沿用上次记录。
    result["memory"] = step.memory or previous.get("memory", {})


def report_target(result, step, previous):
    seconds = result["seconds"]
    timing = "  ".join(f"{k} {v:.1f}s" for k, v in seconds.items())
    if result["status"] != "ok":
        print(f"  [失败] {timing}")
        if result["tests"]:
            print(f"    测试：{result['tests']}")
        for line in list(dict.fromkeys(step.errors))[:MAX_ERRORS]:
            print(f"    {line[:200]}")
        print(f"    完整日志：{result['log']}")
        return
    print(f"  [成功] {timing}")
    cache = result["cache"]
    cache_text = ""
    if cache and cache["hit"] + cache["miss"]:
        total = cache["hit"] + cache["miss"]
        cache_text = f"，缓存命中 {cache['hit']}/{total}（{100 * cache['hit'] // total}%）"
    print(f"  编译  {result['steps']} 步，告警 {result['warnings']} 条{cache_text}")
    if result["tests"]:
        print(f"  测试  {result['tests']}")
    if result.get("size"):
        print(f"  体积  {describe_size(result['size'], previous.get('size'))}")
    for region, usage in result.get("memory", {}).items():
        print(f"  {region:<5} {usage['used']} / {usage['total']}（{usage['percent']:.2f}%）")
    for artifact in result["artifacts"]:
        print(f"  产物  {artifact['path']}  {artifact['bytes']} 字节  sha256:{artifact['sha256'][:12]}")
    print(f"  日志  {result['log']}")


def print_summary(results, run_dir, elapsed):
    print("=" * 72)
    print(f"{'目标':<16}{'状态':<6}{'耗时':>6}{'告警':>4}  {'缓存命中':<8}体积")
    for item in results:
        if item["status"] == "skipped":
            print(f"{item['target']:<18}{'跳过':<6}")
            continue
        cache = item.get("cache")
        hits = f"{cache['hit']}/{cache['hit'] + cache['miss']}" if cache and cache["hit"] + cache["miss"] else "-"
        size = item.get("size")
        flash = f"Flash {size['text'] + size['data']}" if size and item["group"] == "hc32" else ""
        state = "成功" if item["status"] == "ok" else "失败"
        print(f"{item['target']:<18}{state:<6}{item['seconds']['total']:>7.1f}s{item['warnings']:>6}  {hits:<12}{flash}")
    failed = [item["target"] for item in results if item["status"] != "ok"]
    print("-" * 72)
    print(f"总耗时 {elapsed:.1f}s；成功 {sum(i['status'] == 'ok' for i in results)}/{len(results)}"
          + (f"；未成功：{' '.join(failed)}" if failed else ""))
    print(f"日志目录 {run_dir}")


def run_order(path):
    """同一秒的后缀按数值比较，否则 _10 会排在 _2 之前。"""
    stamp, _, suffix = path.name.partition("_")
    return stamp, int(suffix or 0)


def prepare_log_dir():
    LOG_ROOT.mkdir(parents=True, exist_ok=True)
    run_dir = LOG_ROOT / f"{datetime.datetime.now():%Y%m%d-%H%M%S}"
    suffix = 1
    while run_dir.exists():  # 同一秒内重复运行时不覆盖上一次日志
        run_dir = LOG_ROOT / f"{run_dir.name.split('_')[0]}_{suffix}"
        suffix += 1
    run_dir.mkdir()
    # 本次目录不参与排序：时钟回拨时它的名字可能最旧，但后续日志都要写在这里。
    history = sorted((p for p in LOG_ROOT.iterdir() if p != run_dir and p.is_dir() and not p.is_symlink()
                      and re.fullmatch(r"\d{8}-\d{6}(_\d+)?", p.name)), key=run_order)
    for old in history[:max(0, len(history) - (KEEP_RUNS - 1))]:
        shutil.rmtree(old, ignore_errors=True)
    latest = LOG_ROOT / "latest"
    if latest.is_symlink():
        latest.unlink()
    latest.symlink_to(run_dir.name)
    return run_dir


def print_list(targets):
    print("目标（CMakePresets.json）：")
    for target in targets.values():
        note = "  [x1pro 兼容别名]" if target.name.startswith(ALIAS_PREFIX) else ""
        print(f"  {target.name:<18}{target.group:<11}{target.title}{note}")
    print(f"产品简写：{' '.join(product_names(targets))}（配合 -c 选择配置）")
    print(f"分组：{' '.join(GROUPS)}")


def command_text(argv):
    return shlex.join(["./build.py", *argv])


def load_last_command():
    try:
        argv = json.loads((LOG_ROOT / LAST_COMMAND_NAME).read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None
    return argv if isinstance(argv, list) and all(isinstance(item, str) for item in argv) else None


def save_last_command(argv):
    (LOG_ROOT / LAST_COMMAND_NAME).write_text(json.dumps(argv, ensure_ascii=False), encoding="utf-8")


def oem_names():
    """只列出 App/oem.cmake 会接受的目录：未生成资源的 OEM 选了也只会配置失败。"""
    return sorted(p.parent.name for p in OEM_ROOT.glob("*/oem.toml")
                  if all((p.parent / "generated" / name).is_file() for name in ("oem_config.h", "oem_logo.c")))


def parse_choice(text, menu, targets):
    """编号换成菜单里的名称，其余按命令行目标校验，返回（有效目标，无法识别的输入）。"""
    known = {*targets, *GROUPS, *product_names(targets)}
    chosen, unknown = [], []
    for token in text.split():
        name = menu[int(token) - 1] if token.isdecimal() and 0 < int(token) <= len(menu) else token
        (chosen if name in known else unknown).append(name)
    return chosen, unknown


def ask(prompt, parse):
    """重复询问直到 parse 返回非 None，无效输入由 parse 负责提示。"""
    while True:
        value = parse(input(prompt).strip())
        if value is not None:
            return value


def prompt_argv(targets):
    """逐项询问并拼成普通命令行参数：后续校验和编译与命令行共用同一条路径。"""
    print("选择目标（Ctrl+C 退出）")
    menu = []
    for group in GROUPS[:-1]:
        print(f"  {group}")
        for target in firmware_targets(targets) if group == "hc32" else targets.values():
            if target.group == group:
                menu.append(target.name)
                print(f"  {len(menu):>3}  {target.name:<18}{target.title}")
    products = product_names(targets)
    print(f"也可输入分组 {' '.join(GROUPS)} 或产品简写 {' '.join(products)}")
    last = load_last_command()
    if last:
        print(f"上次：{command_text(last)}")

    def parse_targets(text):
        if not text and last:
            return last
        chosen, unknown = parse_choice(text, menu, targets)
        if chosen and not unknown:
            return chosen
        print(f"无法识别：{' '.join(unknown)}" if unknown else "请至少选择一个目标")
        return None

    argv = ask(f"目标（编号或名称，空格分隔{'；回车重复上次' if last else ''}）> ", parse_targets)
    if argv is last:
        return argv

    if {*argv} & {*products, "hc32", "all"}:
        configs = {"": [], "d": [], "r": ["-c", "release"], "a": ["-c", "all"]}

        def parse_config(text):
            if text.lower() in configs:
                return configs[text.lower()]
            print("请输入 d、r 或 a")
            return None

        argv = argv + ask("配置 d=debug r=release a=全部（回车 debug）> ", parse_config)

    def parse_options(text):
        keys = "".join(dict.fromkeys(text.lower().replace(" ", "")))
        unknown = [key for key in keys if key != "o" and key not in MENU_OPTIONS]
        if unknown:
            print(f"无法识别：{' '.join(unknown)}")
            return None
        return keys

    names = "  ".join(f"{key}={label}" for key, (_, label) in MENU_OPTIONS.items())
    keys = ask(f"选项 {names}  o=选择 OEM（可多选，回车跳过）> ", parse_options)
    argv = argv + [MENU_OPTIONS[key][0] for key in keys if key != "o"]
    if "o" in keys:
        oems = oem_names()

        def parse_oem(text):
            if not text:
                return []
            name = oems[int(text) - 1] if text.isdecimal() and 0 < int(text) <= len(oems) else text
            if name in oems:
                return ["-D", f"OEM={name}"]
            print(f"无法识别：{text}")
            return None

        listing = "  ".join(f"{index}={name}" for index, name in enumerate(oems, 1))
        argv = argv + ask(f"OEM {listing}（写入构建目录缓存，之后保持生效；回车跳过）> ", parse_oem)
    return argv


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description="编译单个或多个固件，输出编译前后信息并保存日志。",
        epilog="示例：./build.py x1pro xbase -c release；./build.py hc32 -c all；./build.py tests --test",
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("targets", nargs="*", help="preset 名、产品简写或分组（见 --list）")
    parser.add_argument("-c", "--config", choices=("debug", "release", "all"), default="debug",
                        help="产品简写与 hc32 分组使用的配置（默认 debug）")
    parser.add_argument("-l", "--list", action="store_true", help="列出全部目标后退出")
    parser.add_argument("-j", "--jobs", type=int, help="并行编译任务数（默认由 Ninja 决定）")
    parser.add_argument("-D", dest="define", action="append", default=[], metavar="VAR=VALUE",
                        help="透传给 CMake 的缓存变量，会保留在该构建目录，可重复")
    parser.add_argument("--clean", action="store_true", help="先清理目标再编译")
    parser.add_argument("--test", action="store_true", help="编译后运行 ctest（没有测试的目标跳过）")
    parser.add_argument("--fail-fast", action="store_true", help="首个失败后停止，默认继续编译其余目标")
    parser.add_argument("-v", "--verbose", action="store_true", help="实时显示完整编译输出")
    parser.add_argument("--dry-run", action="store_true", help="只显示将执行的命令")
    return parser.parse_args(argv)


def main():
    argv = sys.argv[1:]
    targets = load_presets()
    # 管道和 CI 里没有人应答，无参数时保持列出目标。
    if not argv and sys.stdin.isatty() and sys.stdout.isatty():
        try:
            argv = prompt_argv(targets)
        except (EOFError, KeyboardInterrupt):
            print("\n已取消")
            return 130
        print(f"\n将执行：{command_text(argv)}\n")
    args = parse_args(argv)
    if args.list or not args.targets:
        print_list(targets)
        return 0
    if args.jobs is not None and args.jobs < 1:
        usage_error("-j 必须大于 0")
    selected = expand(args.targets, args.config, targets)
    if not selected:
        usage_error("没有匹配的目标")
    missing = [tool for tool in ("cmake", "ninja") if not shutil.which(tool)]
    if any(t.group == "hc32" for t in selected) and not shutil.which("arm-none-eabi-gcc"):
        missing.append("arm-none-eabi-gcc")
    if missing:
        usage_error(f"缺少工具 {' '.join(missing)}，请检查 PATH")

    print_environment(selected, args)
    run_dir = None
    if not args.dry_run:
        run_dir = prepare_log_dir()
        save_last_command(argv)
    started = time.monotonic()
    results = []
    interrupted = False
    # 目标按顺序编译：Ninja 已占满 CPU，且后续目标能命中前一目标写入的缓存，并行反而都会未命中。
    try:
        for index, target in enumerate(selected, 1):
            print(f"\n[{index}/{len(selected)}] {target.name}（{target.title}）")
            result = build_target(target, args, run_dir)
            if result is None:
                continue
            results.append(result)
            if result["status"] != "ok" and args.fail_fast:
                break
    except KeyboardInterrupt:
        interrupted = True
        print("\n已中断")
    if args.dry_run:
        return 0

    results += [{"target": t.name, "status": "skipped"} for t in selected[len(results):]]
    print()
    print_summary(results, run_dir, time.monotonic() - started)
    summary = {"finished": datetime.datetime.now().isoformat(timespec="seconds"),
               "commit": capture("git", "rev-parse", "HEAD"), "results": results}
    (run_dir / "summary.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8")
    if interrupted:
        return 130
    return 0 if all(item["status"] == "ok" for item in results) else 1


if __name__ == "__main__":
    sys.exit(main())
