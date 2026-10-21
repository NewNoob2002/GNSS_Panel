# Repository Guidelines

## Project Structure & Module Organization

This repository follows the X-TRACK workspace layout (`App/`, `Frameworks/`, `External/`, `Vendor/`) and builds HC32F460xE firmware (`X1`, `X1Pro`, `XBase`), the unified SDL simulator, and the Linux (RK3506) port from a single source tree.

- `CMakeLists.txt` is the top-level platform selector: it dispatches `add_subdirectory(Vendor/${PLATFORM})` based on the `PLATFORM` cache variable (`HC32`, `Simulator` or `Linux`, defaulting to `HC32`).
- `App/` is the shared application (`app_core`, `app_i18n`, `app_log`, `app_page_manager`, `app_dialplate`, `app_ui`) used by HC32 display firmware, Simulator and Linux. `App/tests/` contains headless core tests that do not depend on LVGL.
- `Frameworks/` holds shared frameworks (`DataCenter`, `DataBroker`, `DeviceManager`, `PageManager`, `ResourceManager`). `App/CMakeLists.txt` compiles `DataCenter` from `../Frameworks/DataCenter`.
- `External/lvgl/` contains upstream LVGL 8.3.11 sources (`src/`, `lvgl.h`, `demos/`) without any platform-specific `lv_conf.h` or porting layer.
- `Vendor/HC32/` contains the HC32F460xE MCU firmware:
  - `CMakeLists.txt` orchestrates the HC32 firmware build (`PRODUCT=X1|X1Pro|XBase`) and links `<prefix>_<version>.elf`.
  - `Platform/` contains CMSIS, startup, drivers, the shared HAL (power policy, charger/gauge, LEDs, tick), `State/` and support code. It is shared by all HC32 products and must not depend on LVGL or `App/`.
  - `Arduino/` provides the Arduino-compatible core.
  - `Libraries/` contains HC32 device and third-party libraries (`Bq40z50`, `Mp2762a`, `MillisTaskManager`, `Parser`, `easylogger`, `Adafruit*` for display products, `MultiButton` for X1), and builds `hc32_lvgl` from `External/lvgl/`.
  - `Comm/` is the I2C slave transport and panel frame-protocol core used by every HC32 product.
  - `Display/` is shared by X1Pro and XBase: display entry point, `AppPlatform/` (HC32 platform layer of the shared LVGL app: status snapshot, two-key input, power events, `system_backend`), display protocol table, display/key HAL, LVGL `lv_conf.h` and `lv_port*`, and the LVGL-backed `operator new`.
  - `Products/<Product>/` holds `product.cmake` (version, image prefix, display flag), `product_config.h` (feature switches included through `mcu_config.h`) and product-only sources (X1: entry point, keys, SOC compensation, protocol table).
- `Vendor/Simulator/` is the unified PC SDL2 simulator (`SIM_BACKEND=Board|Demo`, defaulting to `Board` with `SIM_PRODUCT=X1Pro|XBase`), using `Vendor/Simulator/lv_conf.h`.
- `Vendor/Linux/` is the RK3506 Linux display port, demo backend (`Backends/Demo`) and Linux host test suite, using `Vendor/Linux/config/lv_conf.h`.
- `tests/` contains the HC32 host regression tests (protocol, transport, power, per-product tables, display app integration, state ownership).

Runtime state on HC32 lives in `systemInfo`, which is `const` everywhere; each domain has one writer (board: HAL I2C scan, power: HAL power policy, host: panel protocol, request: `State::Request_*()`, diag: comm layer). Write through the `State::` entry points in `Vendor/HC32/Platform/State/SystemState.h` and keep `tests/check_state_ownership.py` in sync (see `docs/product_structure.md`).

Put a difference between HC32 products in `product_config.h` as a `PRODUCT_*` switch, or in product-only sources, never as a copy of shared code. Shared code reaches product behavior through those switches or weak `HAL::` hooks (`Power_ProductInit/Update`, `Power_AdjustPercent`). See `docs/product_structure.md`.

## Build, Test, and Development Commands

- `cmake --preset <product>-debug` / `<product>-release` configures one HC32 product, where `<product>` is `x1`, `x1pro` or `xbase`; build with `cmake --build --preset <same name>`. Outputs go to `build/<preset>/` (`.elf`, `.map`) and `build/<preset>/generated/` (`.hex`, `.bin`).
- `gcc-debug` / `gcc-release` remain as aliases for X1Pro and provide `build/gcc-debug/compile_commands.json` for clangd; `.clangd` uses `build/x1-debug` for `Vendor/HC32/Products/X1/`.
- `cmake --preset simulator-x1pro` / `simulator-xbase` and `cmake --build --preset <same name>` build the PC simulator with the HC32 virtual board backend (`SIM_BACKEND=Board`).
- `cmake -S . -B build/sim-demo -DPLATFORM=Simulator -DSIM_BACKEND=Demo && cmake --build build/sim-demo` builds the PC simulator with the Linux demo backend (`--random-status [seed]` supported).
- `cmake --preset linux-host && cmake --build --preset linux-host && ctest --test-dir build/linux-host --output-on-failure` builds and tests the Linux port on the host.
- `cmake -S tests -B build/host-tests && cmake --build build/host-tests && ctest --test-dir build/host-tests --output-on-failure` runs the HC32 host regression tests.
- `cmake -S App/tests -B build/app-core-tests && cmake --build build/app-core-tests && ctest --test-dir build/app-core-tests --output-on-failure` runs the standalone headless `App` core tests.

## Coding Style & Naming Conventions

Use C11 and C++17 in the CMake build. Keep CMake modular: add sources and include directories in the owning subdirectory target, expose headers with `PUBLIC` only when dependent modules include them, and avoid adding broad include lists at the top level.

Follow existing naming: lower_snake_case for C-style functions such as `slave_i2c_update`, PascalCase for many C++ classes, and `HAL_` or `HAL::` patterns for hardware abstraction code.

## Testing Guidelines

Host regression tests live in `tests/` (sanitizers enabled) and `App/tests/`. Treat successful GCC builds of every affected HC32 product plus the host tests as the minimum validation:

- `cmake --build --preset x1-debug`, `x1pro-debug`, `xbase-debug` (and the matching `-release` presets)
- `ctest --test-dir build/host-tests --output-on-failure`
- `ctest --test-dir build/app-core-tests --output-on-failure`

Changes in `Vendor/HC32/Platform/`, `Vendor/HC32/Comm/` or `Vendor/HC32/Libraries/` affect all three HC32 products and must be built for all of them. Changes in `App/` or `Frameworks/` should also be validated with the simulator and `linux-host` tests.

## Commit & Pull Request Guidelines

Recent commits use short imperative summaries, sometimes bilingual, such as `update add linux port` or `Refactor main and hal_init functions for improved readability`. Keep commits focused by subsystem.

Pull requests should include a concise description, build results, linked issue if applicable, and screenshots or recordings for UI-visible LVGL changes.

## Agent-Specific Instructions

Keep vendored-library edits minimal (`External/lvgl/`, `Vendor/HC32/Libraries/`). Do not reformat generated resources or third-party code unless the task explicitly requires it.

`App/` and `Frameworks/` are maintained directly in this repository as the single source of truth; platform-specific behavior belongs in `Vendor/<Platform>` (`Vendor/HC32/Display/AppPlatform/` for HC32, `Vendor/Simulator/` for the SDL simulator, and `Vendor/Linux/` for the Linux port) behind the shared `system_backend` / `SystemService` / `App` interfaces: `AppPlatform.cpp` (entry, power/business backend), `StatusBridge` (status bar), `PowerBridge` (HAL power events), `DeviceBridge` (device data and business commands, option index <-> host protocol codes), `InitSteps.cpp` (SystemLoading checks).
