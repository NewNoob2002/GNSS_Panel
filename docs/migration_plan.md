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

设计依据见 `docs/lvgl9_migration.md`。调研结论调整了顺序：不依赖 LVGL 的数据/设备/资源框架先在 LVGL 8 上切换并单独验收；`Frameworks/PageManager` 已使用 v9 接口，只能在 LVGL 9 之后切换。

| # | 内容 | 负责 |
|---|---|---|
| 3.0 | 调研（3.0a/3.0b）与 LVGL 8 视觉基线截图 —— 已完成，见 `docs/lvgl9_migration.md` | argon / 主控 |
| 3.1 ★ | 在 LVGL 8 上加固并切换 `DataBroker`、`DeviceManager`、`ResourceManager`：修复定时器/发布遍历的释放后使用、容量与参数校验、`-Werror` 告警；`App/` 与三端 Bridge 改用新框架；删除 `Frameworks/DataCenter`、`App/Utils/ResourceManager`；`App/tests` 与 Linux 框架测试全部行为在新框架上通过 | sol |
| 3.2 | 图片转换脚本（入库、可重复执行、逐像素回读校验）：v8 数组 → v9 `lv_image_dsc_t`（`RGB565A8`/`RGB565`，无索引格式）；更新 `generate_ui_icons.py`；产物在 3.3 一并接入 | argon（与 3.1 并行） |
| 3.3 ★ | 切换 LVGL v9.6.0：导入 `External/lvgl`（含 `include/`），重建三份 `lv_conf.h`，HC32/模拟器/Linux 显示、输入、时基端口，`lvgl_new.cpp`，`App/` 全部 API 迁移（含 Dialplate 自定义绘制）与现有 `App/Utils/PageManager` 的最小 v9 适配，接入 3.2 的资源；`App/`、`Vendor/` 定义 `LV_DISABLE_API_MAPPING` | sol（sonnet 复核） |
| 3.4 ★ | 加固并切换 `Frameworks/PageManager`：补齐应用依赖的接口与虚函数、页面池释放、出现顺序、动画复位、返回值语义、告警；迁移全部页面；删除 `App/Utils/PageManager` | sol（sonnet 复核） |
| 3.5 | 主控截图对比（与 LVGL 8 基线逐页）与视觉/交互差异修正 | 主控 / sol |
| 3.6 ★ | HC32 内存与性能调优：`LV_DRAW_LAYER_SIMPLE_BUF_SIZE`、C 堆缩减、`LV_MEM_SIZE`、图片 `A8` 取舍；记录 `lv_mem` 峰值、栈水印、帧率与 debug/release 体积 | sol |

进度：3.0 调研与基线完成（`5c000cd`）；3.1 `4784fa7` 已合入（9 页渲染与 LVGL 8 基线逐字节一致，X1Pro/XBase release 334488/1344/169460，RAM 余量 21,776 B）。

阶段 3 额外验收：截图与 3.0 基线逐页对比；HC32 RAM 以链接符号 `__RamEnd - __StackTop` 为准，余量不少于 8 KiB；`App/` 不再出现 LVGL 8 专有符号；`Frameworks/` 只剩被使用的模块。

实机清单：阶段 2 清单全部重复；每个页面进入/返回/动画；长时间运行（≥ 30 分钟）无内存泄漏（`lv_mem` 监视）；刷新流畅度不低于 LVGL 8 版本。

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
