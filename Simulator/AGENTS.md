# Simulator Guidelines

## Structure

This is the SDL simulator for HC32 X1Pro/XBase. It links the same `../Shared/App` CMake targets as firmware and host tests: portable `p4_core`/`p4_data_center` plus LVGL UI `p4_app`. Shared sources come from ATK_RK3506 `application/App`; the pinned commit is in `../Shared/App/UPSTREAM`.

`main.cpp` owns SDL, pointer/keyboard input and simulated host data. `HAL/` provides virtual GPIO, time and a no-op charger. The firmware `../Platform/HAL/HAL_Power.cpp` runs unchanged, with `HAL/HAL.h` force-included to replace hardware declarations. Battery sampling is simulated; GPIO power sequencing and shutdown policy use the firmware implementation.

Enter/middle click and the Startup pointer target request power-on after a complete 2 s hold. The main loop finishes cold boot with `Power_OnCheck()` once accepted. Standby resume restores the outputs directly. `App_Update()` runs the independent core timers, then LVGL runs its UI timers and animations.

## Build and Validation

- Configure: `cmake -S Simulator -B build/simulator-x1pro -DSIM_PRODUCT=X1Pro` (use XBase and a separate directory for that product).
- Build: `cmake --build build/simulator-x1pro`.
- Run: `build/simulator-x1pro/bin/main`.
- Smoke: `python3 tests/simulator_smoke.py build/simulator-x1pro/bin/main`.
- Host regressions: `ctest --test-dir build/host-tests`.

SDL2 development files are required. `.clangd` uses `build/simulator-x1pro/compile_commands.json`; configure the simulator before opening its sources in the editor. Firmware and simulator link the same LVGL sources with separate configuration targets.

## Ownership

Do not edit `../Shared/App` directly except `UPSTREAM` and `.clang-format`. Change and commit ATK_RK3506 `application/App`, then run `scripts/sync_shared_app.sh`; the CMake file is shared too. HC32 behavior belongs in `../Shared/Display/AppPlatform/`. Keep vendor LVGL changes scoped to explicit LVGL work.

The simulator uses C11/C++17 and the repository style. Keep board mocks, host simulation and application services separate; all LVGL and application state access occurs on the main thread.
