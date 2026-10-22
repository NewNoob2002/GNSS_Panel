# Repository Guidelines

## Project Structure & Module Organization

This repository follows the X-TRACK workspace layout (`App/`, `Frameworks/`, `External/`, `Vendor/`) and builds HC32F460xE firmware (`X1`, `X1Pro`, `XBase`), the unified SDL simulator, and the Linux (RK3506) port from a single source tree.

- `CMakeLists.txt` is the top-level platform selector: it dispatches `add_subdirectory(Vendor/${PLATFORM})` based on the `PLATFORM` cache variable (`HC32`, `Simulator` or `Linux`, defaulting to `HC32`).
- `App/` is the shared application (`app_core`, `app_i18n`, `app_log`, `app_page_manager`, `app_dialplate`, `app_ui`) used by HC32 display firmware, Simulator and Linux. `App/tests/` contains headless core tests that do not depend on LVGL.
- `Frameworks/` 包含跨平台框架（`DataBroker`、`DeviceManager`、`PageManager`、`ResourceManager`），各组件在自身目录定义 CMake 目标及 `fw::` 别名；`App/CMakeLists.txt` 通过 `add_subdirectory` 汇总，ResourceManager 为 INTERFACE，页面管理保留 `app_page_manager` 目标名。
- `External/lvgl/` contains unmodified upstream LVGL v9.6.0 sources (public headers in `include/`, plus `src/`, `lvgl.h`, `lvgl_private.h`, `lv_version.h`, and the demos used by the Linux port) without any platform-specific `lv_conf.h` or porting layer. App and platform targets compile with `LV_DISABLE_API_MAPPING`, so only v9 native API names are allowed.
- `Vendor/HC32/` contains the HC32F460xE MCU firmware:
  - `CMakeLists.txt` orchestrates the HC32 firmware build (`PRODUCT=X1|X1Pro|XBase`) and links `<prefix>_<version>.elf`.
  - `Platform/` contains CMSIS, startup, `DDL/` (XHSC LL drivers), `Peripherals/` native peripherals (`GpioPin`, `I2cMaster`, `SpiMaster`, GPIO, UART, Flash, DWT, OTS), `Devices/` on-board chip drivers (`Bq40z50`, `Mp2762a` moved from Libraries), `System/` runtime and system glue (interrupts, syscalls, `new`, `delay`, debug macros), board configuration and instances (`Config/board_pins.h`, `Config/board_peripherals`), the shared `HAL/` (power policy, charger/gauge, LEDs, tick), and `State/`. It is shared by all HC32 products and must not depend on LVGL or `App/`.
  - `Libraries/` 包含第三方组件（`MillisTaskManager`、`Parser`、`SEGGER_RTT`、`easylogger`、X1 使用的 `MultiButton`），各组件维护自己的 `CMakeLists.txt` 与 `hc32::` 别名。LVGL 由 `cmake/lvgl.cmake` 导入上游构建，平台链接 `lvgl::lvgl`。
  - `Comm/` is the I2C slave transport and panel frame-protocol core (`hc32::comm`, `hc32::protocol`) used by every HC32 product.
  - `Display/` is shared by X1Pro and XBase: display entry point (`hc32::display`), display protocol table (`hc32::display_protocol`), display/key HAL, `AppPlatform/` (`hc32::app_platform`: HC32 platform layer of the shared LVGL app: entry header, status snapshot, two-key input, `Power`/`Device`/`Status` device objects registered with DeviceManager), `Panel/` (RM690A0 panel driver `PanelRm690a0` and display board instances `board_display`), and `Lvgl/` (LVGL `lv_conf.h`, `lv_port*` as `hc32::lvgl_port`, and the LVGL-backed `operator new`).
  - `cmake/portable.cmake` defines portable host-compilable targets (`hc32::state`, `hc32::power_policy`, `hc32::app_platform`, etc.) reused by firmware, Simulator, and tests without cross-module source inclusion.
  - `Products/<Product>/` holds `product.cmake` (version, image prefix, display flag), `product_config.h` (feature switches included through `mcu_config.h`) and product-only sources (X1: entry point, keys, SOC compensation, protocol table).
- `Vendor/Simulator/` is the unified PC SDL2 simulator (`SIM_BACKEND=Board|Demo`, defaulting to `Board` with `SIM_PRODUCT=X1Pro|XBase`), using `Vendor/Simulator/lv_conf.h`.
- `Vendor/Linux/` is the RK3506 Linux display port, demo backend (`Backends/Demo`) and Linux host test suite, using `Vendor/Linux/config/lv_conf.h`.
- `tests/` contains the HC32 host regression tests (protocol, transport, power, per-product tables, display app integration, state ownership, and layer decoupling check via `check_layers.py`).

Runtime state on HC32 lives in `systemInfo`, which is `const` everywhere; each domain has one writer (board: HAL I2C scan, power: HAL power policy, host: panel protocol, request: `State::Request_*()`, diag: comm layer). Write through the `State::` entry points in `Vendor/HC32/Platform/State/SystemState.h` and keep `tests/check_state_ownership.py` in sync (see `docs/product_structure.md`).

Put a difference between HC32 products in `product_config.h` as a `PRODUCT_*` switch, or in product-only sources, never as a copy of shared code. Shared code reaches product behavior through those switches or weak `HAL::` hooks (`Power_ProductInit/Update`, `Power_AdjustPercent`). See `docs/product_structure.md`.

## Build, Test, and Development Commands

- `cmake --preset <product>-debug` / `<product>-release` configures one HC32 product, where `<product>` is `x1`, `x1pro` or `xbase`; build with `cmake --build --preset <same name>`. Outputs go to `build/<preset>/` (`.elf`, `.map`) and `build/<preset>/generated/` (`.hex`, `.bin`).
- `gcc-debug` / `gcc-release` 仍为 X1Pro 的兼容别名；`.clangd` 默认使用 `build/x1pro-debug/compile_commands.json`，`Vendor/HC32/Products/X1/` 使用 `build/x1-debug`。
- `cmake --preset simulator-x1pro` / `simulator-xbase` and `cmake --build --preset <same name>` build the PC simulator with the HC32 virtual board backend (`SIM_BACKEND=Board`).
- `cmake -S . -B build/sim-demo -DPLATFORM=Simulator -DSIM_BACKEND=Demo && cmake --build build/sim-demo` builds the PC simulator with the Linux demo backend (`--random-status [seed]` supported).
- `cmake --preset linux-host && cmake --build --preset linux-host && ctest --test-dir build/linux-host --output-on-failure` builds and tests the Linux port on the host.
- `cmake -S tests -B build/host-tests && cmake --build build/host-tests && ctest --test-dir build/host-tests --output-on-failure` runs the HC32 host regression tests.
- `cmake -S App/tests -B build/app-core-tests && cmake --build build/app-core-tests && ctest --test-dir build/app-core-tests --output-on-failure` runs the standalone headless `App` core tests.

## Coding Style & Naming Conventions

Use C11 and C++17 in the CMake build. Keep CMake modular: add sources and include directories in the owning subdirectory target, expose headers with `PUBLIC` only when dependent modules include them, and avoid adding broad include lists at the top level.

第三方组件优先使用其上游 CMake；未自带 CMake 的组件在自身目录维护 `CMakeLists.txt`，父目录通过 `add_subdirectory` 汇总。分层与目标命名遵循 [docs/build_architecture.md](docs/build_architecture.md)。

Follow existing naming: lower_snake_case for C-style functions such as `slave_i2c_update`, PascalCase for many C++ classes, and `HAL_` or `HAL::` patterns for hardware abstraction code.

## Testing Guidelines

Host regression tests live in `tests/` (sanitizers enabled) and `App/tests/`. Treat successful GCC builds of every affected HC32 product plus the host tests as the minimum validation:

- `cmake --build --preset x1-debug`, `x1pro-debug`, `xbase-debug` (and the matching `-release` presets)
- `ctest --test-dir build/host-tests --output-on-failure`
- `ctest --test-dir build/app-core-tests --output-on-failure`
- `python3 tests/check_layers.py .` (after configuring the build directories it inspects; see `docs/build_architecture.md`)

Changes in `Vendor/HC32/Platform/`, `Vendor/HC32/Comm/` or `Vendor/HC32/Libraries/` affect all three HC32 products and must be built for all of them. Changes in `App/` or `Frameworks/` should also be validated with the simulator and `linux-host` tests.

## Commit & Pull Request Guidelines

Recent commits use short imperative summaries, sometimes bilingual, such as `update add linux port` or `Refactor main and hal_init functions for improved readability`. Keep commits focused by subsystem.

Pull requests should include a concise description, build results, linked issue if applicable, and screenshots or recordings for UI-visible LVGL changes.

## Agent-Specific Instructions

Keep vendored-library edits minimal (`External/lvgl/`, `Vendor/HC32/Libraries/`). Do not reformat generated resources or third-party code unless the task explicitly requires it.

`App/` 与 `Frameworks/` 以本仓库为唯一来源。`app_framework` 使用 DataBroker（最多 16 个 DataNode、32 个独立时钟定时器）、DeviceManager 和 ResourceManager 静态表；同步回调仅在应用线程分发，参数不跨回调保存。LVGL 为 v9.6.0（RGB565 原生渲染，HC32/Linux 在发送缓冲中交换字节）；页面管理使用 `Frameworks/PageManager`，导航结果必须与 `PageManager::RES_TYPE::OK` 比较（OK 为 0），页面成员通过 getter 访问。

平台行为位于 `Vendor/<Platform>`：HC32 `Display/AppPlatform/` 的 `PowerDevice`、`BusinessDevice`、`StatusDevice` 分别注册为 `Power`、`Device`、`Status`；Board 模拟器编译同一份实现，Linux/模拟器 Demo 使用 `Backends/Demo`。`Common/SystemBackend.h` 定义三类设备的值快照和精确尺寸命令；SystemService/Model 的便捷函数通过 DeviceManager 同步 read/write/ioctl。HC32 选项下标与协议码转换只在 BusinessDevice 内，写入只经 `State::Request_*`，revision 采样与确认仍在 `display_protocol.cpp`。资源使用 `Frameworks/ResourceManager/ResourceManager.h` 静态数组模板，`ResourcePool.h` 的公开接口不变。
