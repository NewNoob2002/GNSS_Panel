# Repository Guidelines

## Project Structure & Module Organization

This repository is the desktop LVGL simulator for the HC32F460xE Arduino UI project. `main.cpp` initializes LVGL, SDL, EasyLogger, and the application loop. Application code lives under `App/`: page implementations are in `App/Pages/<PageName>/`, shared data processing is in `App/Common/`, reusable framework code is in `App/Utils/`, and generated fonts/images/symbols are in `App/Resource/`. Hardware simulation shims are in `HAL/`. LVGL sources are shared with the firmware at `../Libraries/lvgl/` (selected via `LV_CONF_PATH`, so the simulator's own `lv_conf.h` is used, not the firmware's `Libraries/lvgl/lv_conf.h`), and display/input drivers are under `lv_drivers/`. Configuration headers `lv_conf.h` and `lv_drv_conf.h` control LVGL and driver features.

## Build, Test, and Development Commands

- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug` configures the simulator build.
- `cmake --build build --config Debug` compiles the `main` executable.
- `cmake --build build --target run` launches the simulator via the CMake `run` target.

On Windows, `SDL2_ROOT` defaults to `E:/SDL2-2.32.10` and can be overridden during CMake configuration. On Linux, install SDL2 development files first, for example `sudo apt install libsdl2-dev`. Build output is placed in `bin/`.

## Coding Style & Naming Conventions

Use C11 for C sources and C++17 for C++ sources. Match the existing style: 4-space indentation, braces on function definitions on the next line, and short section comments for LVGL-style files. Page modules follow the pattern `<Page>.cpp`, `<Page>.h`, optional `<Page>Model.*`, and `<Page>View.*`. Prefer PascalCase for page and class names, snake_case for C-style LVGL callbacks or driver functions, and descriptive resource names such as `img_src_wifi.c`.

## Testing Guidelines

There is no dedicated automated test suite in this simulator. Before submitting changes, run a clean CMake build and manually smoke-test `bin/main`: confirm startup, page navigation, resource rendering, and any modified HAL or data-processing behavior. For UI changes, include screenshots or a short description of the exercised screens in the review.

## Commit & Pull Request Guidelines

Recent commits use short summaries such as `update`, `Refactor ...`, and Chinese descriptions of configuration or driver changes. Prefer a concise imperative summary with a clear scope, for example `Refactor StatusBar update flow` or `更新SDL配置路径`. Pull requests should explain the user-visible change, list build/manual test results, note any SDL or toolchain assumptions, and link related issues when available. Avoid mixing unrelated LVGL vendor updates with application changes.

## Agent-Specific Instructions

Do not edit the shared `../Libraries/lvgl/` or `lv_drivers/` sources (LVGL changes also affect the firmware) unless the task explicitly targets driver or vendor behavior. Keep generated resource files in `App/Resource/` synchronized with their source assets, and avoid broad formatting-only changes in third-party directories.
