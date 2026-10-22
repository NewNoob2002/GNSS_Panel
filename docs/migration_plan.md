# 迁移计划：阶段 2–4

阶段 1（X-TRACK 工作区布局，LVGL 8 不变）已在 `dev2` 完成，验收基线见文末。本文是后续阶段的任务契约：每个子任务给出负责模型、交付物和验收门槛。执行代理只修改工作区，不提交；验收通过后由主控在 `dev2` 本地提交，一个子任务一个提交。

## 通用约束（所有执行代理必读）

- 先读 `AGENTS.md`、`docs/product_structure.md` 与本文对应子任务；不越出子任务范围，不做无关重构或格式化。
- `systemInfo` 单写者规则、`State::` 入口、`tests/check_state_ownership.py` 必须保持一致。
- 板级时序（电源保持、强制断电、充电保护、Startup 长按、I2C 从机应答）是已在实机验证的行为，迁移时逐行保留语义；有疑问写入交付说明，不要猜。
- 注释用中文，必要处保留英文术语；硬件相关改动注明依据（数据手册章节或被替换代码的出处）。
- 不新增依赖、不改工具链；`External/lvgl/` 只允许整体替换为上游 tag，不做本地修改。
- 结束时输出：改动文件清单、实际运行过的命令及结果、未验证项与原因。不得声称未运行的验证。

## 验收门槛（每个子任务都执行）

1. `x1/x1pro/xbase` 的 `-debug` 与 `-release` 六个 preset 全部编译通过，无新增警告。
2. `build/host-tests`、`build/app-core-tests` 测试通过；涉及 `App/`、`Frameworks/` 时再加 `simulator-x1pro/xbase` 编译与 `linux-host` 测试。
3. 体积：记录三产品 release 的 `text/data/bss`。应用区上限 `0x72000`（456 KiB，起始 `0x8000`，前面是 bootloader），RAM 188 KiB；flash 余量不少于 16 KiB，静态 RAM（data+bss）不超过 176 KiB，给栈留空间。
4. 主控审阅 diff；高风险子任务（标记 ★）再由独立审阅模型复核。
5. 阶段末由用户按实机检查清单上板确认。

## 模型分工

| 模型 | 用途 |
|---|---|
| `codex/gpt-6.1-sol`（high） | 外设驱动、显示端口、框架替换、内存调优等需要判断的工作（★ 任务） |
| `antigravity/claude-sonnet-5-5` | 规则明确的批量迁移（按页面迁移 API）；审阅 sol 完成的 ★ 任务 diff |
| `antigravity/gemini-4-argon` | 大上下文机械工作：上游源码替换、CMake 清理、资源格式转换、全仓清点、重命名 |

注：2026-10-21 起 `gemini-4-argon` 配额用尽（约 2026-10-28 恢复），期间其任务改由 `antigravity/claude-sonnet-5-5` 执行。
| `antigravity/gemini-3.8-flash` | 文档同步、清点类只读任务 |

## 阶段 2：HC32 去 Arduino 化

目标：删除 `Vendor/HC32/Arduino/` 与 `Adafruit*` 库，HC32 代码直接使用 `Platform/`（LL 驱动 + `gpio.h`/`delay.h`）上的原生 HAL。外部行为与时序不变，flash 应下降。

| # | 内容 | 负责 |
|---|---|---|
| 2.1 ★ | 用原生 C 接口（如 `i2c_master_*`）替换 `Platform/Core` 的 `TwoWire`，迁移 `HAL_I2C`、`Bq40z50`、`Mp2762a`；保留超时、总线恢复与扫描语义。`SPIClass` 仍被 Adafruit 使用，放到 2.3 | sol |
| 2.2 ★ | `Platform/HAL`、`Comm`、`Display`、`Products/X1`、`MillisTaskManager`、`easylogger` 中的 `pinMode/digitalWrite/digitalRead/millis/delay` 等改为 `gpio.h`/`delay.h` 接口；去掉 `Arduino.h` 包含；与 2.2b 合并执行 | sol |
| 2.2b ★ | 通用外设 class 化：`I2cMaster`（替换 2.1 的 `i2c_master_*`）、`GpioPin`；驱动只含外设操作，外设号与引脚由板级配置实例化；无虚函数、无动态分配、静态构造不碰硬件 | sol |
| 2.3 ★ | `SpiMaster` 类替换 `SPIClass`，面板驱动类替换 `Adafruit_ST7789/GFX/BusIO`（同 2.2b 原则）：初始化序列、MADCTL、偏移逐项照搬原库（注明出处），保留现有 DMA 刷新与旋转 | sol |
| 2.4 | 删除 `Arduino/`、`Adafruit*` 目录与 `hc32_arduino*` 目标，更新 `AGENTS.md`、`docs/product_structure.md`、`readme.md` | argon |
| 2.5 ★ | 删除未参与任何构建的文件（依据全目标全新构建的编译器依赖）与编译文件中的 Arduino 衍生死代码，修正失效路径 | sol |
| 2.6 | `Vendor/HC32` 按职责重组：`Platform/{DDL,Peripherals,System}`（取代 `Drivers`、`Core`、`support`），SEGGER RTT 移入 `Libraries/`，`Display/{Panel,Lvgl,AppPlatform}`；只移动文件并同步 CMake/测试/文档，固件体积不变 | argon |

进度（2026-10-21）：2.1 `7889f10`、2.2+2.2b `e523fa1`、2.3 `2b8a027`、2.4 `d7209d4`、2.5 `978f77a`、2.6 `c369003` 已合入 `dev2`，编译与主机测试验收通过，**待实机清单确认**。release 体积（text/data/bss，2.6 后）：X1 25680/136/19136，X1Pro 336412/860/169268，XBase 335964/860/169268；相对阶段 1 基线，带屏产品 flash 约 −6.0 KiB、静态 RAM −80 B；X1Pro RAM 余量（`__RamEnd - __StackTop`）22,448 B。

阶段 2 额外验收：`rg -n 'Arduino\.h|TwoWire|SPIClass|Adafruit|pinMode|digitalWrite' Vendor/HC32` 无结果（第三方示例除外）。

实机清单：三产品上电/长按开机/强制断电；X1Pro/XBase 显示无偏移、无撕裂、颜色正确；按键与长按；I2C 主机侧电量计/充电芯片读数；面板 I2C 从机与主机通信正常；X1 LED 与按键。

## 阶段 3：LVGL v9.6.0 + 全量 X-TRACK 框架

目标：三端（HC32、模拟器、Linux）切到 LVGL v9.6.0；页面层改用 `Frameworks/PageManager`、`ResourceManager`，数据层改用 `DataBroker`，平台 Bridge 改为 `DeviceManager` 设备对象。界面外观与交互不变。

设计依据见 `docs/lvgl9_migration.md`；构建分层见 `docs/build_architecture.md`。调研结论调整了顺序：不依赖 LVGL 的数据/设备/资源框架先在 LVGL 8 上切换并单独验收；`Frameworks/PageManager` 已使用 v9 接口，只能在 LVGL 9 之后切换。

| # | 内容 | 负责 |
|---|---|---|
| 3.0 | 调研（3.0a/3.0b）与 LVGL 8 视觉基线截图 —— 已完成，见 `docs/lvgl9_migration.md` | argon / 主控 |
| 3.1 ★ | 在 LVGL 8 上加固并切换 `DataBroker`、`DeviceManager`、`ResourceManager`：修复定时器/发布遍历的释放后使用、容量与参数校验、`-Werror` 告警；`App/` 与三端 Bridge 改用新框架；删除 `Frameworks/DataCenter`、`App/Utils/ResourceManager`；`App/tests` 与 Linux 框架测试全部行为在新框架上通过 | sol |
| 3.2 | 图片转换脚本（入库、可重复执行、逐像素回读校验）：v8 数组 → v9 `lv_image_dsc_t`（`RGB565A8`/`RGB565`，无索引格式）；更新 `generate_ui_icons.py`；产物在 3.3 一并接入 | argon（与 3.1 并行） |
| 3.3 ★ | 切换 LVGL v9.6.0：导入 `External/lvgl`（含 `include/`），重建三份 `lv_conf.h`，HC32/模拟器/Linux 显示、输入、时基端口，`lvgl_new.cpp`，`App/` 全部 API 迁移（含 Dialplate 自定义绘制）与现有 `App/Utils/PageManager` 的最小 v9 适配，接入 3.2 的资源；`App/`、`Vendor/` 定义 `LV_DISABLE_API_MAPPING` | sol（sonnet 复核） |
| 3.3b | 组件统一由 CMake 管理：导入 LVGL 上游 `CMakeLists.txt` 与 `env_support/cmake`，各平台通过 `add_subdirectory` + `LV_BUILD_CONF_PATH` 使用上游 `lvgl` 目标（demos 用上游 `lvgl_demos`），删除手写的 LVGL 源文件收集；`Vendor/HC32/Libraries/*`、`Frameworks/*` 每个组件在自身目录维护 `CMakeLists.txt`，父目录只 `add_subdirectory`；固件体积与截图不变 | sol（3.4 之后） |
| 3.3c ★ | 按 `docs/build_architecture.md` 实施 CMake 分层：L0 选项 / L1 第三方 / L2 框架 / L3 应用 / L4 平台（HC32 内部 cmsis→ddl→system→peripherals→devices→board→state→hal→comm→display）/ L5 产品；目标只公开自身头文件、带 `<域>::<组件>` ALIAS；Bq40z50/Mp2762a 迁入 `Platform/Devices` 消除环；模拟器与主机测试经 `portable.cmake` 链接 HC32 共享目标；新增 `tests/check_layers.py` 自动校验依赖图并纳入验证策略；体积与截图不变 | sol（3.3b 之后） |
| 3.4 ★ | 加固并切换 `Frameworks/PageManager`：补齐应用依赖的接口与虚函数、页面池释放、出现顺序、动画复位、返回值语义、告警；迁移全部页面；删除 `App/Utils/PageManager` | sol（sonnet 复核） |
| 3.5 | 主控截图对比（与 LVGL 8 基线逐页）与视觉/交互差异修正 | 主控 / sol |
| 3.6 ★ | HC32 内存与性能调优：`LV_DRAW_LAYER_SIMPLE_BUF_SIZE`、C 堆缩减、`LV_MEM_SIZE`、图片 `A8` 取舍；记录 `lv_mem` 峰值、栈水印、帧率与 debug/release 体积 | sol |
| 3.7 ★ | App/Frameworks 按 X-TRACK 架构重构（复审与方案见 `docs/app_architecture_review.md`）：3.7a 目录重排与 AppFactory、3.7b 细粒度 HAL 设备契约与三端实现、3.7c DataProc 领域节点、3.7d App 瘦身（Global/Button/PageNavi/Power 节点）、3.7e 页面 MVP 与 LVGL observer、3.7f 删除旧路径与文档；界面与交互不变 | Pi（Orca） |

进度：3.0 调研与基线完成（`5c000cd`）；3.1 `4784fa7` 已合入（9 页渲染与 LVGL 8 基线逐字节一致，X1Pro/XBase release 334488/1344/169460，RAM 余量 21,776 B）。3.2 `c3f99d7`（57 张图转换脚本，逐像素回读通过）、3.3 `9ef6766`（三端 LVGL v9.6.0；release X1Pro flash 415,496 B、余量 51,448 B，debug 余量 14,140 B，RAM 余量 22,128 B；9 页布局/颜色/图片与基线一致，差异仅为渐变量化与抗锯齿；主机测试 LVGL 池峰值 70,376 B）已合入。3.4 `30b7d04`（切换到 `Frameworks/PageManager`，九页与 3.3 截图逐字节一致，X1Pro release flash 414,784 B、debug 余量 15,380 B）已合入。3.3b `87835be`（LVGL 使用上游 CMake，组件各自维护 CMakeLists；体积与截图完全不变）已合入；`555178a` 修复 3.4 引入的页面日志级别回归（模拟器冒烟失败），并把模拟器冒烟纳入验证策略。3.3c `2a36f3a`（CMake 分层与 `*_port` 端口目标，取消头文件宏注入；`tests/check_layers.py` 基于 File API 校验分层、规则 2/3 与可替换头文件包含歧义；六个固件 release 体积、九页截图与 3.3b 逐字节一致，debug 仅因 `__FILE__` 路径变长 +4/+8 B）已合入。3.6 已合入：3.6a `64ab973`（启动 Logo 裁剪为 A8 + 底色容器重新着色，RGKLogo 彩色保留 RGB565；SystemLoading 除删除的残留列外最大误差 17/255，其余八页逐字节一致；`check_layers.py` 新增源码 `../` 包含检查）、3.6b `f879485`（去掉浮点 printf，改用 `app::format_fixed`，|值|<1e15 内与 newlib 逐字符一致；默认字体改内置 Unscii 8；关闭 grid；默认主题因截图差异保留；debug 栈水印 `lv_port_debug_stack_used()`）、集成修复 `fdee9d8`。release 体积 X1Pro 332,444/1,304/168,780、XBase 332,004/1,304/168,780（相对 3.3c text −80,664 B，低于 LVGL 8 基线），RAM 余量 22,496 B；主机 LVGL 池峰值 X1Pro 70,336 B / XBase 66,896 B，`LV_DRAW_SW_LAYER_SIMPLE_BUF_SIZE` 8–24 KiB 峰值不变，保持 24576；C 堆（8 KiB）与图层缓冲待实机数据再定。3.5 完成：当前 `dev2` 与 LVGL 8 基线（`bed321a`）逐页对比，StarMap/SystemInfos 逐字节一致，其余页面差异 ≤ 9/255（渐变量化与抗锯齿），SystemLoading 仅为 3.6a 的 Logo 变化，默认导航终态的差异为 ROVER 卡片 1.2 s 循环动画的截图相位不同（动画代码未变）；用户以 LVGL 8（`4784fa7`）与当前模拟器并排操作对比交互。对比中发现并修复 3.3c 引入的模拟器回归 `19fabe0`（版本号回退 V0.0.0、应用日志重复 `[HAL]` 前缀、App_Update 误走固件双键轮询），冒烟测试新增前缀检查。**阶段 3 软件部分完成，待实机清单确认。**

阶段 3 额外验收：截图与 3.0 基线逐页对比；HC32 RAM 以链接符号 `__RamEnd - __StackTop` 为准，余量不少于 8 KiB；`App/` 不再出现 LVGL 8 专有符号；`Frameworks/` 只剩被使用的模块。

实机清单：阶段 2 清单全部重复；每个页面进入/返回/动画；长时间运行（≥ 30 分钟）无内存泄漏（`lv_mem` 监视）；刷新流畅度不低于 LVGL 8 版本。3.6 补充（debug 固件，主循环安全断点）：`p lv_port_debug_stack_used()` 得 MSP 栈高水位（余量 = 8192 − 返回值）；`p lv_global.tlsf_state.max_used` 得 LVGL 池峰值；`lv_sysmon_performance_dump(0)` 读 FPS/渲染/flush 耗时；`p (char*)_sbrk(0) - (char*)&__HeapBase` 采样 newlib 堆提交量。场景：冷启动到齿轮动画、九页反复切换、SystemInfos 滚动、录制/Wi-Fi 请求、BatteryWarning、休眠唤醒，各 ≥ 60 s。

## 阶段 4：全量验证并重命名为 GNSS_Panel

| # | 内容 | 负责 |
|---|---|---|
| 4.1 | 全量验证矩阵（全部 preset、全部测试、体积报告、截图对比），汇总实机结果 | 主控 + 用户 |
| 4.2 | CMake `project()`、文档、`proj_hc32.code-workspace` 等文本中的名称改为 GNSS_Panel；固件镜像前缀不变 | flash |
| 4.3 | 仓库目录 `HC32F460xE_Arduino` → `GNSS_Panel`、GitHub 远程仓库改名并更新 remote；执行前再次确认 | 主控 |

## 阶段 1 验收基线（2026-10-21，`dda2da1`）

全部 preset、模拟器、`linux-host`、host-tests、app-core-tests 通过。release 体积：

| 产品 | text | data | bss |
|---|---|---|---|
| X1 | 25920 | 148 | 19128 |
| X1Pro | 342488 | 852 | 169356 |
| XBase | 342048 | 852 | 169356 |

X1Pro/XBase 静态 RAM 已用约 166 KiB（其中 `LV_MEM_SIZE` 96 KiB），阶段 3 的 RAM 预算是主要风险。
