# Simulator Guidelines

## Structure

This is the unified SDL2 simulator (`Vendor/Simulator`) for the repository. It links the shared `../../App` CMake targets (`app_core`, `app_framework`, `app_ui`) and compiles LVGL from `../../External/lvgl` using `Vendor/Simulator/lv_conf.h`.

It supports two backends selected by the `SIM_BACKEND` CMake cache variable:

- `Board` (default): simulates HC32 `X1Pro` or `XBase` (`SIM_PRODUCT=X1Pro|XBase`) using `../HC32/Display/AppPlatform/`, `HAL/` (virtual GPIO, time and no-op charger), and the firmware `../HC32/Platform/HAL/HAL_Power.cpp` through the `sim::hardware_port` target, which supplies `HAL/HAL.h` and simulator definitions without force-including headers.
- `Demo`: links the Linux demo backend (`../Linux/Backends/Demo`) and supports `--random-status [seed]` on the command line.

`main.cpp` owns SDL, pointer/keyboard input, SIGINT/SIGTERM cleanup, CLI argument validation, and backend-specific status/host updates. Board firmware versions are read from the selected product CMake file (`[0-9.]+`, including `3.0.0`); Board and Demo brightness previews share the CIE L* table in `HAL_Board.h`.

Enter/middle click and the Startup pointer target request power-on after a complete 2 s hold. On the `Board` backend, the main loop finishes cold boot with `Power_OnCheck()` once accepted, and standby resume restores the outputs directly.

## Build and Validation

- Configure & build X1Pro (`Board` backend): `cmake --preset simulator-x1pro && cmake --build --preset simulator-x1pro`.
- Configure & build XBase (`Board` backend): `cmake --preset simulator-xbase && cmake --build --preset simulator-xbase`.
- Configure & build `Demo` backend: `cmake -S . -B build/sim-demo -DPLATFORM=Simulator -DSIM_BACKEND=Demo && cmake --build build/sim-demo`.
- Run: `build/simulator-x1pro/bin/main` or `build/sim-demo/bin/main [--random-status 42]`.
- Smoke: `python3 tests/simulator_smoke.py build/simulator-x1pro/bin/main`.
- Host regressions: `ctest --test-dir build/host-tests --output-on-failure`.

SDL2 development files are required. 应用、Frameworks、HAL、Sim 和 LVGL 内部日志统一经 `app::log` → `host::log`/spdlog 同步输出。Debug 默认 DEBUG；Release/RelWithDebInfo/MinSizeRel 默认 INFO，`APP_LOG_COMPILE_LEVEL` 裁掉 DEBUG 且不求值实参，LVGL 内部只保留 WARN 起。`GNSS_LOG_LEVEL=debug|info|warn|error|off` 调整运行时最小级别（只能打开仍编译保留的消息），`GNSS_LOG_COLOR=always|never` 覆盖默认终端自动颜色，例如 `GNSS_LOG_LEVEL=info GNSS_LOG_COLOR=never build/sim-demo/bin/main`。LVGL 的 TRACE/INFO 映射 DEBUG，避免逐对象/逐帧细节混入 INFO；INFO 保留初始化汇总、页面导航和电源/充电变化。CLI 用法、Controls 横幅和随机种子行仍直接输出。策略表见 `docs/build_architecture.md`。

`.clangd` uses `build/simulator-x1pro/compile_commands.json`; configure `simulator-x1pro` before opening simulator sources in the editor.

## Ownership

`App/` and `Frameworks/` are modified directly in this repository; platform-specific behavior belongs in `Vendor/<Platform>` (`Vendor/HC32/Display/AppPlatform/`, `Vendor/Simulator/`, `Vendor/Linux/`). Keep `External/lvgl/` changes scoped to explicit LVGL work.

The simulator uses C11/C++17 and the repository style. Keep board mocks, host simulation and application services separate; all LVGL and application state access occurs on the main thread.
