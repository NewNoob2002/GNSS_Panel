# Repository Guidelines

## Project Structure & Module Organization

This directory is the desktop SDL simulator for the HC32 display products (X1Pro, XBase). `main.cpp` creates the SDL window and LVGL display, then runs the same code as the firmware: the shared LVGL application in `App/` plus the HC32 platform layer `../Shared/Display/AppPlatform/`. The firmware power sequencing `../Platform/HAL/HAL_Power.cpp` is compiled unchanged (power-on long press, shutdown flow, standby and power-on in place, low-battery shutdown): `HAL/HAL.h` is force-included for that file to hide the firmware `HAL.h`, and `HAL/Arduino.h` + `HAL/HAL_Board.cpp` provide a virtual GPIO array, `millis()` and a no-op charger. Enter / middle click drives the virtual Power key pin, so power-on is decided by the same code as on the board. Battery sampling and charging (`HAL_Battery.cpp`) are not compiled; `main.cpp` sets the battery values and plays the host (reports data, confirms panel requests and shutdown, comes back online 3 s after power-on from standby). LVGL sources are shared with the firmware at `../Libraries/lvgl/`; the simulator uses its own `lv_conf.h` through `LV_CONF_PATH`.

`App/` is a verbatim copy of ATK_RK3506 `application/App` (source commit in `App/UPSTREAM`). Only `App/CMakeLists.txt` (firmware build), `App/UPSTREAM` and `App/.clang-format` are HC32-owned.

## Build, Test, and Development Commands

- `cmake -S Simulator -B build/simulator -DSIM_PRODUCT=X1Pro|XBase` configures the simulator (run from the repository root; SDL2 development files required).
- `cmake --build build/simulator` builds `build/simulator/bin/main` (each build directory has its own binary, so X1Pro and XBase can be built side by side).
- `./build/simulator/bin/main` runs it; the key bindings (focus/press/back/commit and injected HC32 power/battery events) are printed at start.

## Coding Style & Naming Conventions

`App/` follows the upstream style (snake_case, its own formatting); do not reformat it here. HC32-owned files (`main.cpp`, `HAL/`, `../Shared/Display/AppPlatform*`) follow the repository `.clang-format` and C++17.

## Testing Guidelines

`ctest --test-dir build/host-tests` runs `x1pro_display_app_tests` / `xbase_display_app_tests` (shared app + AppPlatform firmware path with the firmware `lv_conf.h`, including the LVGL memory margin) and, when `SIMULATOR_EXECUTABLE` is configured, `simulator_smoke`. For UI changes also run the simulator and exercise the affected pages.

## Agent-Specific Instructions

Do not edit `App/` directly: change ATK_RK3506 `application/App`, commit there, then run `scripts/sync_shared_app.sh <ATK_RK3506 repo> [commit]`. Do not edit the shared `../Libraries/lvgl/` sources unless the task explicitly targets LVGL (changes also affect the firmware).
