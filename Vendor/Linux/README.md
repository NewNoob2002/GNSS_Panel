# RK3506J RM690A0 / LVGL 应用迁移

共享应用已拆为独立事件框架、无 LVGL 的 `app_core` 与 UI；架构和无屏测试入口见 [ARCHITECTURE.md](../../App/ARCHITECTURE.md)。平台演示业务实现位于 `Backends/Demo`，由入口工程选择。

目标：移除原 HC32，由 ATK RK3506J Linux 平台直接驱动现有 RM690A0 彩色屏，并迁移 HC32 工程的 294×126、RGB565、LVGL 8.3.11 应用。P3 已在测试板用 SPI mode 3 点亮测试图，颜色和方向经目视确认；LVGL benchmark 在 32 MHz 请求、8 ms 刷新配置下整轮平均 57.3 FPS，详见 [P3 实屏性能记录](docs/P3_DISPLAY.md#lvgl-benchmark-实屏性能2026-09-24)。P4 已完成原 PageManager、状态栏及九个页面的 View 布局迁移，已有 ARM/测试板运行记录；九页限时运行、evdev 四键导航、页面返回及 Debug FPS/CPU/内存监视器已复核。历史基线源码 `7b49532` 的主机默认测试 7/7、独立 benchmark 测试 1/1 通过，SDL 预览构建通过；最新动画、英俄逐页实屏回归及退出/长期稳定性归 P6，不能以旧板端记录代替新提交验证。业务数据与操作进入 P5，`rk3506_app` 当前每 200 ms 更新的是明确标为 `DEMO` 的合成状态。外壳遮挡部分边缘；物理波形、最高速率和 SGM3849 输出轨尚未实测。

当前 `feature/page-ui-optimization` 分支已改为每页 Controller/View/Model、独立 DataNode/DataBroker 和 DataProc 初始化层，增加 SystemSettings、共享日志 API，真正产品的两路 GPIO HIGH/LOW 输入保留接入点，不是键盘 F/P 或 evdev；GPIO line 与有效电平尚未确认。主机 SDL/测试仍用直接操作（方向键/鼠标、Enter、Esc、Ctrl+Enter）；Startup 需 Enter/中键/左键长按 2 秒，短按或失焦取消；Startup 默认隐藏状态栏，待机 10 秒后滑入；SystemLoading 上滑隐藏状态栏，默认 XYZ Logo → 五阶段 System Initialization（旋转齿轮/圆环、Step N/5、进度）→ 完成窗口（全成功为绿色，有失败为橙色）→ Dialplate，全程标 DEMO；按情境使用淡入、水平导航或垂直进出，Back 反向（[启动拼图](out/ui-pages/Boot/current-flow.png)、[动画](out/ui-pages/Boot/boot-en.gif)）。Startup→SystemLoading 逐次替换，Dialplate 为工作根页，不能 Back 回开机页。模拟 Off 未切断电源时重置根页为 Startup，Reboot 返回 Dialplate。StatusBar 沿 HC32 动画上下滑入/滑出，SystemDash 通知隐藏并使用全屏及确认图的灰/橙/红配色。SystemDash 改为 [Power][Settings][Return] 三卡，Return 单击选择即返回上页；图标为原生 40px/24px 图像，避免放大旧 16px 图标。StarMap 去掉黄色焦点外框，保留星座色条与点击返回（[英俄预览](out/ui-pages/SystemDash/three-tile-return-starmap.png)）；SystemSettings 仅语言、禁用 Wi-Fi 与显式 Back，已移除手动日期编辑以简化两键操作（[英俄截图](out/ui-pages/SystemDash/native-icons-settings.png)）。当前测试板改用 V−=Function、V+=Power 的两键操作（单击切焦点/选择、双击返回/提交，Startup 长按 V+ 2 秒）；MENU/ESC 不再参与。电源仍为模拟服务，Wi-Fi 仍禁用；两键版本主机 9/9 测试、ARM 构建及板端 FIFO 定时输入回归通过，已部署并恢复真实 `/dev/input/event0` 输入；V+ 长按启动、V− 双击取消、V+ 双击模拟关机/重启已由板端日志确认，实际两键手感与最新画面仍待操作者复核。历史 SDL smoke 已通过。

当前测试板为用户确认的 RK3506J 512MB+8GB；P1 显示接口已定型，沿用现有屏供电，不增加背光亮度控制。测试板原按键接线 P1_B3 曾与 UART2 TX/sysfs 冲突；当前测试板 `adc-keys` 通过 `/dev/input/event0` 提供 V+、V-、MENU、ESC 四键，已板端验证焦点与页面导航。量产按键和整机电源设计仍在量产 PCB 阶段定型。P2 已使用 J/eMMC 基线完成构建，具体介质与运行配置在板端核实。

- [架构与实施计划](docs/PROJECT_PLAN.md)：模块职责、硬件依据、阶段交付与验收。
- [HC32 迁移摘要](docs/HC32_MIGRATION.md)：实际初始化、旋转/字节序、原应用结构、业务与电源接口的迁移边界。
- [P1 硬件定型记录](board/rk3506j/HARDWARE.md)：用户实接线、候选设备树冲突核查、电气与系统职责待定项。
- [P2 构建基线与复现](docs/P2_BASELINE.md)：SDK 构建结果、Simulator 修复及应用 CMake。
- [P3 屏驱动实施与验收](docs/P3_DISPLAY.md)：协议、SPI/GPIO、测试图、实屏结果与剩余验收项。
- [P4 应用迁移与实屏试运行](docs/P4_APPLICATION.md)：页面路由、状态栏、按键行为及试运行条件。
- [UI 优化与页面结构](docs/UI_PAGES_OPTIMIZATION_PLAN.md)：`feature/page-ui-optimization` 上的 SystemDash 系统菜单、英俄预览、每页 Controller/View/Model、DataNode 数据层、两路 GPIO 输入与 RAM/电源模拟；真实后端及量产 GPIO line/有效电平仍待板端确认。
- [Docker 编译环境](docs/BUILD_ENVIRONMENT.md)：镜像构建、启动、SDK 编译入口与验证边界。
- `Vendor/Linux/`：已有 LVGL 基线、RM690A0/Linux IO、显示测试入口及 [P4 主界面来源记录](../../App/SOURCE.md)；原业务应用正在迁移。
- `Vendor/Linux/tests/`：协议、显示模型、Linux IO/evdev、九页导航与占位状态的主机测试。
- `refer/01_lvgl_button/`：用户提供的已实屏运行参考程序源码；就地构建产物不入版本库。
- `External/lvgl/`：固定原 MCU 的 LVGL 8.3.11。
- `sdk/`：厂商 SDK 工作区，已由现有 `.gitignore` 排除；SDK 改动以补丁保存在主项目。
- `Vendor/Linux/docker/`：Ubuntu 20.04 编译环境与最小验证脚本。
- `.github/workflows/host-tests.yml`：默认应用与 benchmark 分别构建、测试，另检查根工程 SDL 构建、ASan/UBSan、固定 seed 随机状态和正常退出；[P3 检查点运行结果](https://github.com/NewNoob2002/ATK_RK3506/actions/runs/35965777501)仅覆盖旧版 3 项测试。

在仓库根目录运行主机检查：

```sh
cmake --preset linux-host
cmake --build --preset linux-host
ctest --test-dir build/linux-host --output-on-failure
```

benchmark 使用独立构建配置：`cmake -S Vendor/Linux -B build/linux-demo -DCMAKE_BUILD_TYPE=Debug -DRK3506_LVGL_DEMO=ON`，随后构建并运行该目录的 CTest。当前默认配置在 Linux 主机注册 11 项测试，benchmark 注册 1 项；远端 CI 结果需另行核实。

离线单页预览可运行 `build/linux-host/Vendor/Linux/tests/app_navigation_preview Pages/StarMap build/linux-host/star-map.ppm`；不带页面名时检查九页导航。预览不包含真实业务状态或板端刷新性能。九页拼图见 `out/p4/screens.png`（历史本机生成文件，未在此次文档核对中重新生成）。

原始资料保留在 `docs/`；实际 SDK 位于 `sdk/atk_dlrk3506_linux6.1_release_v1.3.1_20260326/`。P3 代码检查点为提交 `20f7f1c`，本地标签为 `p3-display-bringup-2026-09-24`；标签不代表剩余物理验收项已完成。
