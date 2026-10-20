# Repository Guidelines

## Project Structure & Module Organization

This repository is an HC32F460xE firmware project migrated from Keil MDK to CMake + `arm-none-eabi-gcc`; the Keil project has been removed.

One source tree builds three products on the same MCU and board family: `X1` (no screen), `X1Pro` and `XBase` (AMOLED + LVGL). Each CMake configure builds exactly one product, selected by the `PRODUCT` cache variable.

- `CMakeLists.txt` is the top-level orchestrator only: it selects the product, adds subdirectories and links the final image (`<prefix>_<version>.elf`).
- `Platform/` contains CMSIS, startup, linker script, drivers, the shared HAL (power policy, charger/gauge, LEDs, tick) and support code. It is shared by all products and must not depend on LVGL or `Simulator/App`.
- `Arduino/` provides the Arduino-compatible core.
- `Libraries/` contains third-party and device libraries. Display-only targets (LVGL, Adafruit, ButtonEvent) are built only when the product has a display; `MultiButton` only for X1.
- `Shared/Comm/` is the I2C slave transport and panel frame-protocol core used by every product.
- `Shared/Display/` is shared by X1Pro and XBase: display entry point, display protocol table, display/encoder HAL and the LVGL-backed `operator new`.
- `Simulator/App/` contains LVGL app pages, resources, and app utilities reused by the display products and the PC simulator.
- `Products/<Product>/` holds `product.cmake` (version, image prefix, display flag), `product_config.h` (feature switches included through `mcu_config.h`) and product-only sources (X1: entry point, keys, SOC compensation, protocol table).

Put a difference between products in `product_config.h` as a `PRODUCT_*` switch, or in product-only sources, never as a copy of shared code. Shared code reaches product behavior through those switches or weak `HAL::` hooks (`Power_ProductInit/Update`, `Power_AdjustPercent`). See `docs/product_structure.md`.

## Build, Test, and Development Commands

- `cmake --preset <product>-debug` / `<product>-release` configures one product, where `<product>` is `x1`, `x1pro` or `xbase`; build with `cmake --build --preset <same name>`. Outputs go to `build/<preset>/` (`.elf`, `.map`) and `build/<preset>/generated/` (`.hex`, `.bin`).
- `gcc-debug` / `gcc-release` remain as aliases for X1Pro and provide `build/gcc-debug/compile_commands.json` for clangd; `.clangd` uses `build/x1-debug` for `Products/X1/`.
- `cmake -S Simulator -B build/simulator -DSIM_PRODUCT=X1Pro|XBase` builds the PC simulator for a display product.
- `cmake -S tests -B build/host-tests && cmake --build build/host-tests && ctest --test-dir build/host-tests` runs the host regression tests (protocol, transport, power, per-product tables).

## Coding Style & Naming Conventions

Use C11 and C++17 in the CMake build. Keep CMake modular: add sources and include directories in the owning subdirectory target, expose headers with `PUBLIC` only when dependent modules include them, and avoid adding broad include lists at the top level.

Follow existing naming: lower_snake_case for C-style functions such as `slave_i2c_update`, PascalCase for many C++ classes, and `HAL_` or `HAL::` patterns for hardware abstraction code.

## Testing Guidelines

Host regression tests live in `tests/` (sanitizers enabled). Treat successful GCC builds of every affected product plus the host tests as the minimum validation:

- `cmake --build --preset x1-debug`, `x1pro-debug`, `xbase-debug` (and the matching `-release` presets)
- `ctest --test-dir build/host-tests`

Changes in `Platform/`, `Shared/Comm/` or `Libraries/` affect all three products and must be built for all of them.

For UI or LVGL page changes, also validate the simulator path if it is relevant to the change.

## Commit & Pull Request Guidelines

Recent commits use short imperative summaries, sometimes bilingual, such as `update add linux port` or `Refactor main and hal_init functions for improved readability`. Keep commits focused by subsystem.

Pull requests should include a concise description, build results, linked issue if applicable, and screenshots or recordings for UI-visible LVGL changes.

## Agent-Specific Instructions

Keep vendored-library edits minimal. Do not reformat generated resources or third-party code unless the task explicitly requires it.
