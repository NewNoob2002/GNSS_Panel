"""系统状态写入口只能由对应的写入者调用（见 Platform/State/SystemState.h）。

systemInfo 是 const，直接写入在编译期就会失败；本检查补上“谁能拿到可写引用”这一层。
测试代码（tests/）与 PC 模拟器的主控/HAL 模拟不受限制。
"""
import re
import sys
from pathlib import Path

ROOT = Path(sys.argv[1]).resolve()
OWNERS = {
    "Board": {"Platform/HAL/HAL_I2C.cpp"},
    "Power": {
        "Platform/HAL/HAL.cpp",
        "Platform/HAL/HAL_I2C.cpp",
        "Platform/HAL/HAL_Power.cpp",
        # 仅 LVGL_SIMULATOR 分支：模拟器中代替 HAL 置位开机
        "Shared/Display/AppPlatform/AppPlatform.cpp",
    },
    "Host": {
        "Shared/Comm/panel_protocol.cpp",
        "Shared/Display/display_protocol.cpp",
        "Products/X1/x1_protocol.cpp",
    },
    "Diagnostics": {
        "Shared/Comm/panel_protocol.cpp",
        "Shared/Comm/slave_i2c.cpp",
        "Shared/Display/display_protocol.cpp",
        "Products/X1/x1_protocol.cpp",
    },
}
# 确认面板请求只属于面板协议
OWNERS["Confirm"] = OWNERS["Host"]
EXEMPT = ("Platform/State/", "tests/", "Simulator/main.cpp", "Simulator/HAL/")
SOURCES = ("Platform", "Shared", "Products", "Simulator")

pattern = re.compile(r"State::(Board|Power|Host|Diagnostics|Confirm)_?\w*\(")
violations = []
for top in SOURCES:
    for path in (ROOT / top).rglob("*"):
        if path.suffix not in {".c", ".cpp", ".h"} or "/build/" in str(path):
            continue
        rel = path.relative_to(ROOT).as_posix()
        if rel.startswith(EXEMPT) or rel.startswith("Simulator/App/"):
            continue
        for number, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
            for match in pattern.finditer(line):
                if rel not in OWNERS[match.group(1)]:
                    violations.append(f"{rel}:{number}: State::{match.group(1)}... 不属于该模块")
if violations:
    print("\n".join(violations))
    sys.exit(1)
print("system state ownership ok")
