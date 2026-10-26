"""系统状态写入口只能由对应的写入者调用（见 Vendor/HC32/Platform/State/SystemState.h）。

systemInfo 是 const，直接写入在编译期就会失败；本检查补上“谁能拿到可写引用”这一层。
测试代码（tests/）与 PC 模拟器的主控/HAL 模拟不受限制。
"""
import re
import sys
from pathlib import Path

ROOT = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parent.parent
OWNERS = {
    "Board": {"Vendor/HC32/Platform/HAL/HAL_I2C.cpp"},
    "Power": {
        "Vendor/HC32/Platform/HAL/HAL.cpp",
        "Vendor/HC32/Platform/HAL/HAL_I2C.cpp",
        "Vendor/HC32/Platform/HAL/HAL_Power.cpp",
        "Vendor/HC32/Platform/HAL/HAL_Battery.cpp",
    },
    "Host": {
        "Vendor/HC32/Comm/panel_protocol.cpp",
        "Vendor/HC32/Display/display_protocol.cpp",
        "Vendor/HC32/Products/X1/x1_protocol.cpp",
    },
    "Diagnostics": {
        "Vendor/HC32/Comm/panel_protocol.cpp",
        "Vendor/HC32/Comm/slave_i2c.cpp",
        "Vendor/HC32/Display/display_protocol.cpp",
        "Vendor/HC32/Products/X1/x1_protocol.cpp",
    },
}
# 确认面板请求只属于面板协议
OWNERS["Confirm"] = OWNERS["Host"]
EXEMPT = (
    "Vendor/HC32/Platform/State/",
    "tests/",
    "Vendor/Simulator/main.cpp",
    "Vendor/Simulator/HAL/",
)
SOURCES = ("Vendor/HC32", "Vendor/Simulator")

pattern = re.compile(r"State::(Board|Power|Host|Diagnostics|Confirm)_?\w*\(")
violations = []
for top in SOURCES:
    for path in (ROOT / top).rglob("*"):
        if path.suffix not in {".c", ".cpp", ".h"} or "/build/" in str(path):
            continue
        rel = path.relative_to(ROOT).as_posix()
        if rel.startswith(EXEMPT):
            continue
        for number, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
            for match in pattern.finditer(line):
                if rel not in OWNERS[match.group(1)]:
                    violations.append(f"{rel}:{number}: State::{match.group(1)}... 不属于该模块")
if violations:
    print("\n".join(violations))
    sys.exit(1)
print("system state ownership ok")
