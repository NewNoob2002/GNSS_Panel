# CMake 分层构建架构

本文定义仓库的构建分层与依赖规则。目标是让每个模块只通过 CMake 目标暴露接口：依赖方向固定，头文件可见性受控，一个模块改动时，从目标图上就能确定会影响哪些模块。阶段 3.3b（组件 CMake 化）和 3.3c（分层目标）按本文实施，`tests/check_layers.py` 负责自动检查。

## 1. 分层

依赖只能从上层指向下层。同层组件之间不得互相依赖；表中列出的除外。

| 层 | 目录 | 目标（ALIAS 命名） | 允许依赖 |
|---|---|---|---|
| L0 选项 | `cmake/` | `proj::options`（语言标准、告警、`-fno-exceptions/-fno-rtti` 等，按平台定义） | — |
| L1 第三方组件 | `External/lvgl`、各平台 `Libraries/` 中的第三方库 | `lvgl::lvgl`、`lvgl::demos`（上游 CMake）、`hc32::segger_rtt`、`hc32::easylogger`、`hc32::multibutton`、`hc32::parser`、`hc32::millis_task` | L0；需要平台端口的库通过 `*_port` INTERFACE 目标注入（如 easylogger 的输出端口） |
| L2 框架 | `Frameworks/*` | `fw::databroker`、`fw::devicemanager`、`fw::resourcemanager`（INTERFACE）、`fw::pagemanager` | L0、L1（只有 `fw::pagemanager` 可以依赖 `lvgl::lvgl`），以及 `app::log`（日志宏桥接，见 4.3） |
| L3 应用 | `App/` | `app::log`、`app::i18n`、`app::core`（无 LVGL）、`app::spring`（无 LVGL）、`app::lv_spring`、`app::ui`（页面、资源、组件） | L0–L2；`app::core` 不得依赖 `lvgl::lvgl`、`fw::pagemanager`；同层弹簧依赖仅 `app::ui → app::lv_spring → app::spring` |
| L4 平台 | `Vendor/<Platform>/` | 见第 2 节 | L0–L3；只依赖本平台目标，不得依赖其他 `Vendor/` 平台，第 3 节的共享子集除外 |
| L5 产品/可执行 | `Vendor/HC32/Products/*`、各平台入口 | 固件 ELF、模拟器、Linux 可执行与测试 | 任意下层 |

## 2. HC32 平台内部分层（L4）

自下而上：

| 子层 | 目录 | 目标 | 允许依赖 |
|---|---|---|---|
| 4.1 芯片 | `Platform/CMSIS`、`Platform/Config/hc32f4xx_conf.h` | `hc32::cmsis`（头文件、启动文件、`system_hc32f460.c`） | L0 |
| 4.2 驱动库 | `Platform/DDL` | `hc32::ddl` | 4.1 |
| 4.3 系统 | `Platform/System` | `hc32::system`（中断、syscalls、`new`、`delay`、调试宏） | 4.1–4.2，`hc32::segger_rtt`、`hc32::easylogger` |
| 4.4 外设 | `Platform/Peripherals` | `hc32::peripherals`（`GpioPin`、`I2cMaster`、`SpiMaster`、GPIO/UART/Flash/DWT/OTS） | 4.1–4.3 |
| 4.5 器件驱动 | `Platform/Devices`（由 `Libraries/Bq40z50`、`Libraries/Mp2762a` 迁入） | `hc32::devices` | 4.4 |
| 4.6 板级 | `Platform/Config`（`board_pins.h`、`board_peripherals`、`mcu_config.h`） | `hc32::board` | 4.4–4.5 |
| 4.7 状态 | `Platform/State` | `hc32::state`（纯 C++，主机可编译） | L0 及其类型定义所需的最小头文件 |
| 4.8 HAL | `Platform/HAL` | `hc32::hal`（电源策略、充电/电量计、按键、LED、时基） | 4.3–4.7，`hc32::millis_task` |
| 4.9 通信 | `Comm/` | `hc32::comm` | 4.7–4.8，`hc32::parser` |
| 4.10 显示 | `Display/{Lvgl,Panel,AppPlatform}` 与入口/协议表 | `hc32::display_port`（Lvgl、Panel）、`hc32::app_platform`（AppPlatform 设备对象）、`hc32::display` | 4.4–4.9，L3 `app::ui`、`lvgl::lvgl` |
| 5 产品 | `Products/*` | `hc32::product_<name>` + ELF | 全部下层 |

迁入 `Platform/Devices` 是为了消除现有的环：HAL 依赖 Bq40z50/Mp2762a，Bq40z50/Mp2762a 又依赖 `Peripherals/I2cMaster`。迁入后，器件驱动位于外设之上、HAL 之下。

## 3. 跨平台共享子集

模拟器 Board 后端与主机测试需要复用 HC32 中可在主机编译的部分，包括 `State`、电源策略（`HAL_Power.cpp`、`PowerPolicy.h`）和 `AppPlatform` 设备对象。这些部分在 `Vendor/HC32/cmake/portable.cmake` 中定义成目标：`hc32::state`、`hc32::power_policy`、`hc32::app_platform`。HC32 固件、`Vendor/Simulator`、`tests/` 都 `include()` 这个文件并链接上述目标，不再各自按路径编译 HC32 源文件。硬件相关部分由各端用自己的实现或 fake 替换，替换方式是链接不同的 INTERFACE/OBJECT 目标，不靠调整 include 路径顺序来覆盖。

Linux 的 `Vendor/Linux/CMakeLists.txt` 用 `LINUX_APP_DISPLAY`、`LINUX_APP_SURVEY`（默认均为 ON）编排两个应用。`Apps/Display/` 保留 RK3506 屏幕程序与原目标名：`Panel/`、`Platform/`、`Backends/Demo/` 属于 L4；入口、诊断、benchmark 和测试属于 L5。SDL Demo 继续链接此处的 `linux::demo_backend`，主机回归通过此处的 `tests/portable.cmake` 复用页面管理测试。

`Apps/Survey/` 是独立逻辑应用（L5），当前只提供一次性启动入口和 CTest 冒烟；仅依赖标准库，不链接 LVGL、共享 App、Frameworks 或 Display。关闭 Display 后可独立构建，先面向 EG25-G、再移植 RK3506。业务模块和待确认边界见 [Survey README](../Vendor/Linux/Apps/Survey/README.md)；两个应用的数据接缝与协议尚未确定。

## 4. 规则

1. **依赖向下、无环**：只能按第 1、2 节的表链接；同层例外必须写进本表。
2. **头文件可见性**：每个目标只把自己的公开头文件目录设为 `PUBLIC`/`INTERFACE`，实现细节用 `PRIVATE`。禁止在全局使用 `include_directories()`，禁止把仓库根目录或上级目录（`..`）作为包含路径。头文件统一按组件目录包含，例如 `#include "DataBroker.h"`。
3. **不跨模块按路径编译**：一个模块需要另一个模块的代码时，链接对方的目标（OBJECT/STATIC/INTERFACE），不得在自己的源文件列表里写 `../Other/xxx.cpp`。
4. **注入点显式化**：平台向上层提供的差异只有三种方式：L0 选项目标、`lvgl::lvgl` 及其 `lv_conf.h` 路径、`*_port` 接口目标。不再依赖父目录预先定义的隐式目标名（现有的 `app_build_options`、`app_lvgl` 改为显式参数或 `proj::options`/`lvgl::lvgl`）。
   - **日志桥接（4.3）**：`Frameworks` 的日志宏通过 `app::log` 输出，这是 L2 → L3 的唯一允许例外。等日志移到更低层（例如独立的 `common::log`）后再取消。
5. **组件自管理**：第三方组件优先使用上游 CMake（LVGL 用 `add_subdirectory`，通过 `LV_BUILD_CONF_PATH` 等选项配置）；没有 CMake 的组件在自身目录维护 `CMakeLists.txt`，父目录只做 `add_subdirectory`。
6. **命名**：每个目标都提供 `<域>::<组件>` 形式的 ALIAS（`proj::`、`lvgl::`、`fw::`、`app::`、`hc32::`、`sim::`、`linux::`），其他目录只链接 ALIAS 名。
7. **可替换头文件唯一提供者**：会被模拟器或测试替身替换的头文件（`HAL.h`、`gpio.h`、`delay.h`、`dwt.h`、`hc32_ll.h`、`platform_debug.h` 等）用尖括号包含，真实版本放在各组件的 `port/` 目录并由 `*_port` 目标提供。每个编译单元对每个这类头文件只能解析到一个包含目录，替换由所链接的目标决定，不依赖包含路径顺序。
8. **自动检查**：`tests/check_layers.py` 通过 CMake File API（codemodel-v2）读取已配置构建目录（`x1pro-debug`、`x1-debug`、`xbase-debug`、`linux-host`、`host-tests`、`simulator-x1pro`）的目标、源文件、包含目录与依赖。目标按定义目录映射到层与 HC32 子层，无法映射的目标直接报错。检查内容包括依赖方向与环、规则 2、3，以及规则 7 的包含歧义。找不到任何构建目录时判为失败。INTERFACE 目标不出现在 codemodel 中，由包含歧义检查间接约束。检查纳入 `tests/` 的 CTest 与 `.pi-orchestrator.json` 验证策略。

## 5. 现状差距与完成情况（2026-10-21，3.3c 实施后）

| 问题 | 位置 | 状态 |
|---|---|---|
| App 各目标把 `App/` 与仓库根 `..` 设为 PUBLIC 包含路径，任意头文件都能互相包含 | `App/CMakeLists.txt` | 已完成（3.3c）：各目标仅公开自有目录，统一按组件名包含 |
| App 依赖父目录预先定义的 `app_build_options`、`app_lvgl` | `App/CMakeLists.txt`、各平台 | 已完成（3.3c）：上层统一链接 `proj::options` 与 `lvgl::lvgl` |
| `Frameworks` 源文件由 `App/CMakeLists.txt` 按路径编译，框架没有自己的目标 | `App/CMakeLists.txt` | 已完成（3.3b/3.3c）：各框架组件维护自有目标与 `fw::*` 别名 |
| HC32 平台只有一个 `hc32_platform_headers`（暴露全部子目录）和一个大 OBJECT 目标 | `Vendor/HC32/Platform/CMakeLists.txt` | 已完成（3.3c）：拆分为 4.1–4.10 子层目标 |
| HAL 依赖 `Libraries/` 中的器件驱动，器件驱动又依赖 Platform 的外设，形成环 | `Platform/HAL/*`、`Libraries/{Bq40z50,Mp2762a,MillisTaskManager}` | 已完成（3.3c）：Bq40z50 与 Mp2762a 迁入 `Platform/Devices/`（4.5 子层），端口通过 INTERFACE 注入 |
| 模拟器按路径编译 HC32 的 `HAL_Power.cpp`、`SystemState.cpp`、`AppPlatform/*.cpp`，并把 HC32 多个目录加入包含路径 | `Vendor/Simulator/CMakeLists.txt` | 已完成（3.3c）：通过 `portable.cmake` 链接便携目标与 `sim::hardware_port` |
| 主机测试按路径引用其他模块源文件约 38 处 | `tests/CMakeLists.txt` | 已完成（3.3c）：主机回归全部通过 `portable.cmake` 实例化目标并链接替身接口 |
| LVGL 源文件在 HC32、模拟器、Linux、tests 四处手写收集 | 3.3b 处理 | 已完成（3.3b）：通过上游 CMake 与 `project_add_lvgl` 统一构建 |
