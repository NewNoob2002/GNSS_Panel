#!/usr/bin/env python3
"""
CMake layering and decoupling verification tool using CMake File API (codemodel-v2).

Validates architectural compliance according to docs/build_architecture.md:
1. Target directory classification via DIRECTORY_LAYER_MAP (unmapped directories cause errors).
2. Rule 2: Header visibility (targets must not declare includes outside their definition or build directory).
3. Rule 3: No cross-module source compilation (target sources must reside within its definition directory).
4. Dependency direction (L5 -> L4 -> L3 -> L2 -> L1 -> L0) and HC32 sublayer ordering (4.10 -> 4.1).
5. Documented exceptions: Rule 4.3 log bridge, *_port injection, hc32::cmsis private header dependencies.
6. Cyclic dependency detection (DAG verification).
"""

import argparse
import glob
import json
import os
import re
import subprocess
import sys
from typing import Dict, List, Optional, Set, Tuple

# 层级定义 (L0 -> L5)
LAYER_L0 = 0  # 选项与外部系统库 (proj::options, m, pthread)
LAYER_L1 = 1  # 第三方库 (LVGL, SEGGER_RTT, easylogger, Parser, MillisTaskManager)
LAYER_L2 = 2  # 跨平台框架 (DataBroker, DeviceManager, ResourceManager, PageManager)
LAYER_L3 = 3  # 共享应用 (app::log, app::i18n, app::core, app::dialplate, app::ui)
LAYER_L4 = 4  # 平台层与硬件端口替身 (HC32, Linux, Simulator, test fakes)
LAYER_L5 = 5  # 产品可执行目标与测试可执行

# HC32 子层定义 (4.1 -> 4.10)
SUBLAYER_CMSIS = 1
SUBLAYER_DDL = 2
SUBLAYER_SYSTEM = 3
SUBLAYER_PERIPHERALS = 4
SUBLAYER_DEVICES = 5
SUBLAYER_BOARD = 6
SUBLAYER_STATE = 7
SUBLAYER_HAL = 8
SUBLAYER_COMM = 9
SUBLAYER_DISPLAY = 10

SYSTEM_LIBS = {
    "m", "pthread", "dl", "rt", "SDL2", "gcc_s", "gcc_s_asneeded", "c", "stdc++",
    "PkgConfig::SDL2", "SDL2::SDL2"
}

# 目录前缀映射表 (与 docs/build_architecture.md 严格对应，最长前缀优先匹配)
DIRECTORY_LAYER_MAP: List[Tuple[str, Tuple[int, Optional[int]]]] = [
    # L0 编译选项目标
    ("cmake", (LAYER_L0, None)),

    # L1 第三方库与移植目标
    ("External/lvgl", (LAYER_L1, None)),
    ("Vendor/HC32/Libraries", (LAYER_L1, None)),

    # L2 跨平台框架
    ("Frameworks", (LAYER_L2, None)),

    # L3 共享应用及测试
    ("App/tests", (LAYER_L5, None)),
    ("App", (LAYER_L3, None)),

    # L4 HC32 子层 (4.1 -> 4.10)
    ("Vendor/HC32/Platform/CMSIS", (LAYER_L4, SUBLAYER_CMSIS)),
    ("Vendor/HC32/Platform/DDL", (LAYER_L4, SUBLAYER_DDL)),
    ("Vendor/HC32/Platform/System", (LAYER_L4, SUBLAYER_SYSTEM)),
    ("Vendor/HC32/Platform/Peripherals", (LAYER_L4, SUBLAYER_PERIPHERALS)),
    ("Vendor/HC32/Platform/Devices", (LAYER_L4, SUBLAYER_DEVICES)),
    ("Vendor/HC32/Platform/Config", (LAYER_L4, SUBLAYER_BOARD)),
    ("Vendor/HC32/Platform/State", (LAYER_L4, SUBLAYER_STATE)),
    ("Vendor/HC32/Platform/HAL", (LAYER_L4, SUBLAYER_HAL)),
    ("Vendor/HC32/Comm", (LAYER_L4, SUBLAYER_COMM)),
    ("Vendor/HC32/Display", (LAYER_L4, SUBLAYER_DISPLAY)),
    ("Vendor/HC32/Platform", (LAYER_L4, None)),

    # L5 HC32 产品与固件可执行目标
    ("Vendor/HC32/Products", (LAYER_L5, None)),
    ("Vendor/HC32", (LAYER_L5, None)),

    # Linux 平台目标与测试
    ("Vendor/Linux/platform/linux", (LAYER_L5, None)),
    ("Vendor/Linux/demo", (LAYER_L5, None)),
    ("Vendor/Linux/tests", (LAYER_L5, None)),
    ("Vendor/Linux/platform", (LAYER_L4, None)),
    ("Vendor/Linux/display", (LAYER_L4, None)),
    ("Vendor/Linux/Backends/Demo", (LAYER_L4, None)),
    ("Vendor/Linux", (LAYER_L5, None)),

    # Simulator 平台目标
    ("Vendor/Simulator", (LAYER_L4, None)),

    # 主机回归测试目标
    ("tests", (LAYER_L5, None)),
]


def map_directory_to_layer(rel_dir: str, target_name: str, target_type: str) -> Tuple[Optional[int], Optional[int]]:
    """
    Map target definition directory to architectural layer and optional HC32 sublayer.

    @param rel_dir Directory relative to repository root.
    @param target_name Target name string.
    @param target_type CMake target type (EXECUTABLE, STATIC_LIBRARY, etc.).
    @return (layer, sublayer) tuple, or (None, None) if directory is unmapped.
    """
    norm_dir = rel_dir.replace("\\", "/").rstrip("/")
    if norm_dir == "":
        norm_dir = "."

    # 选项目标与外部系统链接库归 L0
    if target_name in SYSTEM_LIBS or target_name.startswith("-l") or target_name in ("proj::options", "project_options"):
        return LAYER_L0, None

    # 可执行目标与运行工具归 L5
    if target_type in ("EXECUTABLE", "UTILITY"):
        # 仍须校验其定义目录是否在合法前缀下
        matched = False
        for prefix, _ in DIRECTORY_LAYER_MAP:
            if norm_dir == prefix or norm_dir.startswith(prefix + "/"):
                matched = True
                break
        if not matched:
            return None, None
        return LAYER_L5, None

    # 按目录映射表匹配最长前缀
    for prefix, (layer, sublayer) in DIRECTORY_LAYER_MAP:
        if norm_dir == prefix or norm_dir.startswith(prefix + "/"):
            return layer, sublayer

    return None, None


def get_target_def_dir(tgt: dict, cm_source: str, repo_root: str) -> str:
    """
    Retrieve the exact directory where the target was defined.

    @param tgt Codemodel target JSON dict.
    @param cm_source Root source directory of the codemodel configuration.
    @param repo_root Absolute path to repository root.
    @return Absolute path to target's definition directory.
    """
    bg = tgt.get("backtraceGraph", {})
    bt_idx = tgt.get("backtrace")
    if bt_idx is not None and "nodes" in bg and bt_idx < len(bg["nodes"]):
        node = bg["nodes"][bt_idx]
        file_idx = node.get("file")
        if file_idx is not None and "files" in bg and file_idx < len(bg["files"]):
            fpath = bg["files"][file_idx]
            if not os.path.isabs(fpath):
                fpath = os.path.normpath(os.path.join(cm_source, fpath))
            return os.path.dirname(fpath)

    src_dir = tgt.get("paths", {}).get("source", ".")
    if not os.path.isabs(src_dir):
        src_dir = os.path.normpath(os.path.join(cm_source, src_dir))
    return src_dir


def check_parent_includes(repo_root: str) -> list[str]:
    """扫描自有 C/C++ 源码的父目录包含，返回带文件和行号的规则 2 错误。

    @param repo_root 仓库根目录。
    @return 违规消息列表；第三方实现与既有生成模板回退分支不参与检查。
    """
    errors = []
    excluded = {"Vendor/HC32/Libraries", "Vendor/HC32/Platform/DDL", "Vendor/HC32/Platform/CMSIS"}
    extensions = (".c", ".h", ".cpp", ".hpp", ".cc", ".hh", ".cxx", ".hxx", ".inc")
    for component in ("App", "Frameworks", "Vendor"):
        for root, dirs, files in os.walk(os.path.join(repo_root, component)):
            dirs[:] = sorted(d for d in dirs if d not in ("External", "build", ".git")
                             and os.path.relpath(os.path.join(root, d), repo_root).replace(os.sep, "/") not in excluded)
            for name in sorted(files):
                if not name.endswith(extensions):
                    continue
                filepath = os.path.join(root, name)
                rel = os.path.relpath(filepath, repo_root).replace(os.sep, "/")
                with open(filepath, encoding="utf-8", errors="replace") as fp:
                    content = fp.read()
                for match in re.finditer(r'^[ \t]*#[ \t]*include[ \t]*"([^"\n]+)"', content, flags=re.M):
                    include = match.group(1)
                    if "../" not in include:
                        continue
                    # 上游风格资源模板在 LV_BUILD_TEST 下回退到相邻 lvgl.h；正式构建不走此分支。
                    # 只豁免 img_src_*.c 中紧接该条件的既有精确路径，其他父目录包含仍报错。
                    if (rel.startswith("App/Resource/img/img_src_") and rel.endswith(".c")
                            and include == "../lvgl.h"
                            and re.search(r"#elif\s+defined\(LV_BUILD_TEST\)\s*$", content[:match.start()])):
                        continue
                    line = content.count("\n", 0, match.start()) + 1
                    errors.append(f'{rel}:{line}: 违规 (规则 2): 禁止父目录包含 #include "{include}"')
    return errors


def check_static_rules(repo_root: str) -> List[str]:
    """
    静态检查全局 include_directories() 与源码父目录包含。

    @param repo_root Absolute path to repository root.
    @return List of error message strings.
    """
    errors = check_parent_includes(repo_root)
    for root, _, files in os.walk(repo_root):
        if any(ignored in root for ignored in ("External", "build", ".git")):
            continue
        for f in files:
            if f == "CMakeLists.txt" or f.endswith(".cmake"):
                filepath = os.path.join(root, f)
                rel = os.path.relpath(filepath, repo_root)
                with open(filepath, "r", encoding="utf-8", errors="replace") as fp:
                    content = fp.read()
                for m in re.finditer(r"(^|[^\w])include_directories\s*\(", content):
                    line_no = content[:m.start()].count("\n") + 1
                    errors.append(f"{rel}:{line_no}: 违规 (规则 2): 禁止在全局使用 include_directories()")
    return errors


def ensure_file_api_reply(bdir: str, repo_root: str) -> Optional[dict]:
    """
    Ensure CMake File API codemodel-v2 query is written and reply loaded.

    @param bdir Build directory path.
    @param repo_root Absolute repository root.
    @return Codemodel-v2 JSON dictionary or None on failure.
    """
    bdir_abs = os.path.abspath(bdir)
    query_dir = os.path.join(bdir_abs, ".cmake", "api", "v1", "query")
    os.makedirs(query_dir, exist_ok=True)
    query_file = os.path.join(query_dir, "codemodel-v2")
    with open(query_file, "w") as fp:
        pass

    # 重新配置构建目录以生成最新 codemodel-v2 回复
    proc = subprocess.run(["cmake", bdir_abs], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    if proc.returncode != 0:
        print(f"  [WARN] 重新配置 {bdir} 失败: {proc.stderr.strip()}")
        return None

    reply_dir = os.path.join(bdir_abs, ".cmake", "api", "v1", "reply")
    idx_files = sorted(glob.glob(os.path.join(reply_dir, "index-*.json")))
    if not idx_files:
        return None

    with open(idx_files[-1], "r", encoding="utf-8") as fp:
        idx = json.load(fp)

    reply_map = idx.get("reply", {})
    if "codemodel-v2" not in reply_map:
        return None

    cm_rel = reply_map["codemodel-v2"]["jsonFile"]
    with open(os.path.join(reply_dir, cm_rel), "r", encoding="utf-8") as fp:
        return json.load(fp)


def check_build_directory(bdir: str, repo_root: str) -> List[str]:
    """
    Validate architectural rules for a single build directory using File API codemodel-v2.

    @param bdir Build directory path.
    @param repo_root Absolute repository root path.
    @return List of error message strings.
    """
    bname = os.path.basename(os.path.normpath(bdir))
    bdir_abs = os.path.abspath(bdir)
    cm = ensure_file_api_reply(bdir, repo_root)
    if not cm:
        return [f"[{bname}] 无法获取 codemodel-v2 File API 数据"]

    reply_dir = os.path.join(bdir_abs, ".cmake", "api", "v1", "reply")
    cm_source = cm["paths"]["source"]
    errors = []

    configs = cm.get("configurations", [])
    if not configs:
        return [f"[{bname}] codemodel 中未找到有效构建配置"]

    config = configs[0]
    tgt_refs = config.get("targets", [])

    # 加载全部目标 JSON
    tgt_map: Dict[str, dict] = {}
    for t_ref in tgt_refs:
        t_json = os.path.join(reply_dir, t_ref["jsonFile"])
        with open(t_json, "r", encoding="utf-8") as fp:
            t = json.load(fp)
        tgt_map[t["id"]] = t

    # 1. 目标归层与规则 2 / 规则 3 校验
    tgt_classification: Dict[str, Tuple[Optional[int], Optional[int]]] = {}

    for t in tgt_map.values():
        t_name = t.get("name", "")
        t_type = t.get("type", "")
        def_dir = get_target_def_dir(t, cm_source, repo_root)
        rel_def = os.path.relpath(def_dir, repo_root)

        layer, sublayer = map_directory_to_layer(rel_def, t_name, t_type)
        if layer is None:
            errors.append(f"[{bname}] 违规: 目标 {t_name} 的定义目录 '{rel_def}' 不在分层目录映射表中")
            continue

        tgt_classification[t["id"]] = (layer, sublayer)

        # External 上游第三方组件豁免内部检查
        if rel_def.startswith("External"):
            continue

        bg = t.get("backtraceGraph", {})

        # 规则 3：目标源文件必须在其定义目录内 (构建生成文件除外)
        for s in t.get("sources", []):
            s_path = s["path"]
            s_abs = s_path if os.path.isabs(s_path) else os.path.normpath(os.path.join(def_dir, s_path))
            if s.get("isGenerated", False) or s_abs.endswith((".o", ".obj")):
                continue
            if os.path.commonpath([bdir_abs, s_abs]) == bdir_abs:
                continue
            if os.path.commonpath([def_dir, s_abs]) != def_dir:
                errors.append(f"[{bname}] 违规 (规则 3): 目标 {t_name} 包含定义目录之外的源文件: {s_path}")

        # 规则 2：目标自身声明的包含目录必须在其定义目录内或构建目录内
        for cg in t.get("compileGroups", []):
            for inc in cg.get("includes", []):
                inc_path = os.path.normpath(inc["path"])
                bt_idx = inc.get("backtrace")
                if bt_idx is not None and "nodes" in bg and bt_idx < len(bg["nodes"]):
                    node = bg["nodes"][bt_idx]
                    cmd = bg["commands"][node.get("command", 0)] if "command" in node else ""
                    # 仅检查由本目标直接通过 target_include_directories 声明的路径 (排除 target_link_libraries 传递)
                    if cmd in ("target_include_directories", "include_directories"):
                        in_def = (os.path.commonpath([def_dir, inc_path]) == def_dir)
                        in_bld = (os.path.commonpath([bdir_abs, inc_path]) == bdir_abs)
                        if not in_def and not in_bld:
                            errors.append(f"[{bname}] 违规 (规则 2): 目标 {t_name} 自身声明了定义目录之外的包含路径: {inc_path}")

    # 2. 依赖环路检测 (DAG 校验)
    adj: Dict[str, Set[str]] = {}
    for t in tgt_map.values():
        t_name = t["name"]
        for dep in t.get("dependencies", []):
            dst = tgt_map.get(dep["id"])
            if dst:
                adj.setdefault(t_name, set()).add(dst["name"])

    visited: Dict[str, int] = {}
    path: List[str] = []
    cycle_found: List[List[str]] = []

    def dfs(node: str):
        visited[node] = 1
        path.append(node)
        for neighbor in adj.get(node, set()):
            if visited.get(neighbor, 0) == 1:
                cycle_found.append(list(path) + [neighbor])
            elif visited.get(neighbor, 0) == 0:
                dfs(neighbor)
        path.pop()
        visited[node] = 2

    for n in list(adj.keys()):
        if visited.get(n, 0) == 0:
            dfs(n)

    for cyc in cycle_found:
        errors.append(f"[{bname}] 依赖环路: {' -> '.join(cyc)}")

    # 3. 依赖方向与分层规则校验
    for t in tgt_map.values():
        src_id = t["id"]
        src_name = t["name"]
        if src_id not in tgt_classification:
            continue
        src_layer, src_sub = tgt_classification[src_id]

        for dep in t.get("dependencies", []):
            dst = tgt_map.get(dep["id"])
            if not dst:
                continue
            dst_id = dst["id"]
            dst_name = dst["name"]
            if dst_id not in tgt_classification:
                continue
            dst_layer, dst_sub = tgt_classification[dst_id]

            # 强规则：app::core 不得依赖 lvgl 或 fw::pagemanager
            if src_name in ("app_core", "app::core"):
                if "lvgl" in dst_name.lower():
                    errors.append(f"[{bname}] 违规: app::core 依赖了 LVGL: {dst_name}")
                if "pagemanager" in dst_name.lower() or dst_name == "app_page_manager":
                    errors.append(f"[{bname}] 违规: app::core 依赖了 fw::pagemanager: {dst_name}")

            # 特例 1：规则 4.3 日志桥接 (fw::* -> app::log 为允许的唯一 L2 -> L3 依赖)
            if src_layer == LAYER_L2 and dst_name in ("app_log", "app::log"):
                continue

            # *_port 与测试替身均为 INTERFACE 目标，不出现在 codemodel 中，不需要按名字放行；
            # 它们是否重复提供同名头文件由下方的包含歧义检查负责。

            # 特例 2：hc32::cmsis 对 config/ddl 头的私有依赖 (docs/build_architecture.md 表 2)
            if "cmsis" in src_name and any(k in dst_name for k in ("config", "ddl", "board")):
                continue

            # 编译选项与系统库 (L0) 允许被任意层依赖
            if dst_layer == LAYER_L0:
                continue

            # 产品/可执行目标 (L5) 允许依赖任意下层
            if src_layer == LAYER_L5:
                continue

            # 跨层依赖方向校验 (只能从上层依赖下层)
            if src_layer < dst_layer:
                errors.append(f"[{bname}] 层间违规: L{src_layer} ({src_name}) 反向依赖了 L{dst_layer} ({dst_name})")

            # HC32 平台内部子层顺序校验 (只能从较高子层依赖较低子层)
            if src_layer == LAYER_L4 and dst_layer == LAYER_L4:
                if src_sub is not None and dst_sub is not None:
                    if src_sub < dst_sub:
                        errors.append(f"[{bname}] HC32 子层违规: 子层 {src_sub} ({src_name}) 依赖了较高子层 {dst_sub} ({dst_name})")

    # 4. 可替换头文件唯一性校验 (每个编译单元对每个可替换头文件只能有唯一提供者)
    REPLACEABLE_HEADERS = [
        "HAL.h",
        "gpio.h",
        "delay.h",
        "dwt.h",
        "hc32_ll.h",
        "platform_debug.h",
        "mcu_config.h",
        "board_pins.h",
        "ots.h",
    ]

    cc_file = os.path.join(bdir_abs, "compile_commands.json")
    if os.path.exists(cc_file):
        try:
            with open(cc_file, "r", encoding="utf-8") as fp:
                cmds = json.load(fp)
            for cmd in cmds:
                fpath = cmd.get("file", "")
                args = cmd.get("command", "").split()
                inc_dirs = []
                i = 0
                while i < len(args):
                    arg = args[i]
                    if arg.startswith("-I") or arg.startswith("-isystem"):
                        d = arg[2:] if arg.startswith("-I") else arg[8:]
                        if not d and i + 1 < len(args):
                            i += 1
                            d = args[i]
                        inc_dirs.append(os.path.normpath(d))
                    i += 1
                for h in REPLACEABLE_HEADERS:
                    matching = [d for d in inc_dirs if os.path.exists(os.path.join(d, h))]
                    if len(matching) > 1:
                        errors.append(
                            f"[{bname}] 违规 (包含歧义): 编译单元 {os.path.relpath(fpath, repo_root)} "
                            f"对可替换头文件 '{h}' 存在多个提供者: {matching}"
                        )
        except Exception as e:
            errors.append(f"[{bname}] 解析 compile_commands.json 失败: {e}")
    else:
        for t in tgt_map.values():
            t_name = t.get("name", "")
            for cg in t.get("compileGroups", []):
                inc_dirs = [os.path.normpath(inc["path"]) for inc in cg.get("includes", [])]
                for h in REPLACEABLE_HEADERS:
                    matching = [d for d in inc_dirs if os.path.exists(os.path.join(d, h))]
                    if len(matching) > 1:
                        errors.append(
                            f"[{bname}] 违规 (包含歧义): 目标 {t_name} "
                            f"对可替换头文件 '{h}' 存在多个提供者: {matching}"
                        )

    return errors


def main():
    """CLI entry point for architecture layer validation."""
    parser = argparse.ArgumentParser(description="Check CMake build layering and decoupling rules via File API.")
    parser.add_argument("repo_root", nargs="?", default=".", help="Root of repository")
    parser.add_argument("--build-dir", help="Specific build directory to inspect")
    args = parser.parse_args()

    repo_root = os.path.abspath(args.repo_root)
    all_errors = []

    print(f"=== 开始 CMake 架构分层静态检查 ({repo_root}) ===")
    static_errors = check_static_rules(repo_root)
    if static_errors:
        for err in static_errors:
            print(f"  [ERROR] {err}")
        all_errors.extend(static_errors)
    else:
        print("  [PASS] 静态源码规则 2 全局包含与父目录包含校验通过 (0 违规)")

    print("\n=== 开始 CMake File API 目标与依赖模型校验 (codemodel-v2) ===")
    build_dirs = []
    if args.build_dir:
        build_dirs.append(args.build_dir)
    else:
        for p in (
            "build/x1pro-debug",
            "build/x1-debug",
            "build/xbase-debug",
            "build/linux-host",
            "build/host-tests",
            "build/simulator-x1pro",
        ):
            full = os.path.join(repo_root, p)
            if os.path.exists(full):
                build_dirs.append(full)

    if not build_dirs:
        # 没有可检查的构建目录时不能视为通过，否则规则会被静默跳过
        print("  [ERROR] 未检测到已配置的 build 目录，无法执行 File API 校验。")
        all_errors.append("缺少已配置的 build 目录")
    else:
        for bdir in build_dirs:
            bname = os.path.basename(bdir)
            b_errors = check_build_directory(bdir, repo_root)
            if b_errors:
                for err in b_errors:
                    print(f"  [ERROR] {err}")
                all_errors.extend(b_errors)
            else:
                print(f"  [PASS] {bname}: File API 校验通过 (目标分层、源文件归属、规则 2/3、无环)")

    if all_errors:
        print(f"\n[FAIL] 架构检查未通过，共发现 {len(all_errors)} 处违规。")
        sys.exit(1)
    else:
        print("\n[SUCCESS] 全部架构分层检查通过！")
        sys.exit(0)


if __name__ == "__main__":
    main()
