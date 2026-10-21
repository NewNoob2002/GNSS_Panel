# HC32 与 RK3506 共用 LVGL 应用：同步设计

状态：方向与下列三项已由用户确认（2026-10-19）。当前同步的上游提交见 `Simulator/App/UPSTREAM`（ATK_RK3506 分支 `feature/platform-hooks`，基于 `b2b5f1f` 与修复分支 `fix/review-findings`）。

## 已确定的方向

- 唯一源码：ATK_RK3506 `application/App`。HC32 的 `Simulator/App` 改为它的原样副本，不再按 HC32 风格改写。
- HC32 现有业务（工作模式/电台/NTRIP、记录配置与启停、真实开关机与待机、X1Pro 电池过温警告、主控确认关机）接入共享页面的 Model/Service 接口，不在页面里直接读写 `systemInfo`。
- 两平台都用 RK 的两键语义：Function 单击=下一项、双击=返回；Power 单击=选择、双击=提交、长按 2 s=开机。HC32 保留 Function 长按强制关机。
- 两个仓库都修改：共享接口先在 RK 仓库落地，再复制到 HC32。

## 共享层与平台层的边界

共享 App 目前已经平台无关：输入经 `P4App::on_key()/on_key_at()/poll_keys()`，状态经 `StatusService::update()`，电源经 `SystemService::power()`。需要补的是“平台决定、界面跟随”的几个点：

| 内容 | RK3506（现状/模拟） | HC32（平台实现） | 共享层需要新增 |
|---|---|---|---|
| 状态栏数据 | `DemoStatus` 每 200 ms | 由 `systemInfo` 组装快照，200 ms 发布 | 快照增加电池温度（含有效位） |
| 开机 | Startup 自身 2 s 计时后进入 SystemLoading | HAL 读 Power 键原始电平判定 2 s，置 `panel_power_on` | `SystemState.powered_on`；Startup 的长按只做显示并发出开机请求，`powered_on` 变为 true 时才切页。RK 模拟后端收到请求立即置位 |
| 用户关机 | SystemDash → 弹窗 → SaveConfig → 模拟 `power(Off)` | `power(Off)` 调 `HAL::Power_Shutdown(false)` | 无 |
| 外部关机（主控、低电、强制关机） | 无 | HAL 置位 `LinuxPowerOff/LowBatteryPowerOff/Force_ShutDown` | `SystemState` 增加关机原因与“正在关机”；P4App 收到后推入 SaveConfig，SaveConfig 显示原因文案（HC32 现有 4 条） |
| 关机完成后待机 | SaveConfig 计时结束后 `reset_root("Pages/Startup")` | HAL 断电延时到期、外部供电仍在时进入驻留（`ShutdownComplete`） | `SystemState.standby`；由平台通知后 `reset_root`，不由 SaveConfig 计时决定 |
| 重启 | 模拟 | 不支持 | `SystemState.reboot_available`；为 false 时 SystemDash 隐藏 Reboot |
| 初始化步骤 | 5 步 DEMO（模拟不同耗时） | 真实检查：版本信息、重新探测电量计与充电芯片、主控供电、等待主控通信（≤30 s） | 后端非阻塞轮询 `poll_initialization_step()`，每步耗时即实际检查时间；失败不阻止开机 |
| 设备数据（定位、卫星、星图、工作模式、记录、Wi-Fi、电池、版本） | `SimulatedSystem` 的固定数据 | 由 `systemInfo` 组装 `DeviceSnapshot`，每 500 ms 检查、内容变化才发布 | Account “Device”（`DeviceService::update/snapshot`），页面只显示选项索引 |
| 工作模式/电台/NTRIP | 模拟后端只改快照 | `execute_device(ApplyWork)` 经 `State::Request_Work()` 记录面板请求，面板协议上报（`radio_change_flag`）并在主控读出后确认 | `DeviceRequest`（只接受 WorkSettingsModel 发送、索引越界拒绝） |
| 记录配置与启停 | 同上 | `execute_device(StartRecord/StopRecord)` 经 `State::Request_Record()`（协议上报 `record_op`/`record_change_flag`）；XBase 不改类型/间隔 | 同上（只接受 RecordConfigModel） |
| Wi-Fi 开关 | 无后端，禁用 | X1Pro：`request_wifi()` 经 `State::Request_Wifi()`（协议上报 `wifi_on_off_set` + `wifi_change_flag`）；主控回报一致前显示 “...”，15 s 无回报放弃等待 | `SystemCommand::SetWifi`、`SystemState.wifi_pending`、`SystemService::set_wifi()` |
| 电池过温警告 | 无 | X1Pro 顶层横幅，65 °C 出现、60 °C 消失 | 共享横幅组件，按产品开关启用 |
| Model 初始化失败 | `std::abort()` | 单片机上会直接卡死 | 改为可替换的致命错误钩子（RK 默认 abort，HC32 记录后复位） |

平台实现放在各自仓库的 App 之外（HC32：`Shared/Display/`；RK：`application/`），共享 App 只声明接口。

## HC32 侧落地

- `Simulator/App` 换成共享副本，固件与 PC 模拟器继续共用同一份。旧页面（Shutdown、HardwareCheck 等）随之删除，由 SystemDash、SystemLoading 替代。
- 输入：`Shared/Display/HAL_Key.cpp` 改为每轮采样两个 GPIO 电平，调用 `on_key()` + `poll_keys(millis())`；不再注册 LVGL 编码器。
- 编译：共享 App 使用 GNU C++17 扩展（指定初始化器），与 RK 的 `CMAKE_CXX_STANDARD 17` 一致；需要确认 arm-none-eabi-gcc 在当前选项下无告警。
- 产品差异（X1Pro/XBase、品牌 Logo、RGK）继续用 `PRODUCT_*` 开关；共享 App 只读编译期选项，不出现产品名。

## 资源评估（已测）

- LVGL 内存池：在 RK 主机测试中把 `LV_MEM_SIZE` 逐级降低，56 KB 时 9 项测试全部通过，48 KB 时导航测试因分配失败卡住；共享 App 的 LVGL 对象峰值在 48–56 KB。HC32 为 96 KB，但 HC32 的 C++ `new` 也走这个池（`lvgl_new.cpp`），页面、Account 及其双缓冲需另计，在固件上实测。
- `LV_USE_LINE` / `LV_USE_TABLE`：共享 App 未使用，HC32 无需打开。
- Flash（已测）：X1Pro release 294776 B → 321204 B（换成共享应用，+26 KB）→ 327212 B（接入设备数据与业务命令，456 KB 的 70%）；XBase release 326668 B；静态 RAM X1Pro release 169752 B。
- LVGL 池（已测，HC32 固件配置，主机测试走完全部业务流程）：峰值 X1Pro 约 30 KB、XBase 约 28 KB / 96 KB。

## 进度

1. **已完成**（ATK_RK3506 分支 `feature/platform-hooks`）：共享接口（开机/待机/平台关机与原因/重启能力/StarMap 与记录设置能力/致命错误钩子/电池温度与过温横幅/初始化步骤与标题由后端提供）；RK 模拟后端行为不变；HC32 的无损压缩 Logo 并入上游；`uint32_t` 日志格式修正。主机 CTest 10/10（含 ASan/UBSan）。
2. **已完成**（HC32 分支 `feature/shared-app`）：`Simulator/App` 换成上游副本；`Shared/Display/AppPlatform` 实现平台层（状态快照、两键电平、HAL 电源事件、`system_backend`、`app_fatal` 软件复位）；去掉 LVGL 编码器输入；PC 模拟器改为共享应用 + AppPlatform + 模拟 HAL。六个固件预设构建无告警；主机测试含 `x1pro/xbase_display_app_tests`（固件 lv_conf，LVGL 池峰值 X1Pro 约 28 KB / XBase 约 26 KB，96 KB 池，主机 64 位指针下测得，偏保守）。
3. **已完成**（RK `551f3e6` + HC32 `AppPlatform`）：设备数据与业务命令。共享层新增 Device Account、`DeviceRequest`、`request_wifi`/`initial_device`/`execute_device` 后端接口；Startup 固件/硬件版本、SystemInfos、StarMap、Dialplate 显示真实数据；WorkSettings、RecordConfig、SystemSettings Wi-Fi 经 Model 发命令。HC32 把选项索引映射为主控编码（协议 `{1,2,4,5,9,10,13,16}`、记录类型 `{1,2}`、间隔 `{0xff,0x00,0x01,0x02,0x04,0x18}`，与旧页面相同）。主机测试覆盖编码映射、三种工作目标、CUSTOM 信道拒绝、记录启停、Wi-Fi 等待与超时、V1.3 主板充电步骤。尚未上板验证。
4. **已完成**：`scripts/sync_shared_app.sh <ATK_RK3506 仓库> [提交]` 按提交导出 `application/App`，保留 HC32 自有的 `CMakeLists.txt`、`UPSTREAM`、`.clang-format`。

## 同步流程

- 改共享应用：在 ATK_RK3506 修改 `application/App` 并提交（主机测试通过），再在 HC32 运行同步脚本，构建六个预设与主机测试。
- HC32 特有行为只放在 `Shared/Display/AppPlatform/`（实现 `system_backend`、调用 `SystemService`/`P4App`），不改 `Simulator/App`：`AppPlatform.cpp` 是入口与电源/业务后端，`StatusBridge`、`PowerBridge`、`DeviceBridge` 各管一个方向的转换（状态在对象内，`App_Init` 时整体重建），`InitSteps.cpp` 是 SystemLoading 的检查步骤。新增业务数据时扩展 `DeviceBridge::snapshot()/execute()` 与 `display_protocol.cpp`。
- `Simulator/App/.clang-format` 关闭 HC32 侧格式化，避免与上游格式冲突。

## 已确认的细节

- 开机由平台决定、界面跟随：Startup 长按只显示进度并发出开机请求；平台确认后发布 `powered_on`，由 P4App 切到 SystemLoading。
- HC32 不提供重启：`reboot_available = false`，SystemDash 电源弹窗只有 Shutdown；长按 Power 2 s 进度满 100% 确认，短按或提前松开关闭弹窗。运行中的 Power 长按只由界面处理（HAL 只在未开机时判定长按开机），两者不冲突。
- HC32 初始化步骤（`InitSteps.cpp`）非阻塞轮询，失败只显示、不阻塞进入首页：
  1. 版本信息：只检查编译期写入的产品与固件版本。
  2. 电量计：在加载页用 `HAL::I2C_ProbeDevice(0x0B)` 重新探测 BQ40Z50。
  3. 充电控制：重新探测 MP2762A（0x5C）。开机扫描判为 V1.5 而现在不应答判失败；V1.3 无该芯片（GPIO 控制充电）判通过。重新探测只补登记迟到上线的器件，不清除在线标志（清除电量计标志会解除充电温度保护，清除充电芯片标志会在运行中切换充电方式）。
  4. 主控供电：`POWER_CONTROL_PIN` 为高。
  5. 主控通信：收到第一帧有效协议帧（`host.online`）即通过，最长等 30 s，超时标红后进入首页。
  等待期间主循环照常运行：Function 长按 2.5 s 强制关机、主控请求关机、低电量关机都由 HAL 独立处理，加载页会切到关机页。
  主板有两个版本：V1.5 带 MP2762A，V1.3 没有（GPIO 控制充电）。版本由开机时 `HAL::I2C_Scan` 是否找到 0x5C 推断；加载页再次探测，因此开机后才失效的 MP2762A 会判失败，但开机扫描前就已失效的 V1.5 仍会被当作 V1.3。

## 与旧 HC32 界面的行为差异

- 记录：开始与停止都是在录制按钮上单击 Power（Select），双击（Commit）同样有效；旧界面是长按开始。
- 状态栏 SD 图标随主控回报的录制状态点亮（`StatusBridge` 读 `systemInfo.host.record.record_status`，主控确认前不变）；旧界面从不点亮。
- Wi-Fi：旧界面切换后无限等待主控回报；现在 15 s 后结束等待（设定值保留，主控之后仍可应用）。

## 实施要点

- 平台事件（开机确认、外部关机、进入待机）由 P4App 统一处理导航，并推迟到 Account 回调返回之后（`lv_async_call`），避免在页面自身的 Notify 调用链里卸载该页面。
- 用户关机仍保持 RK 现有流程：SaveConfig 进度结束前可取消，结束时提交执行。执行后：Reboot 回到工作根页；Off 不再由 SaveConfig 自行切页，而是等平台发布“进入待机”后回到 Startup（RK 模拟后端立即发布，行为不变；HC32 由 HAL 断电延时结束后发布）。
- 外部关机（主控、低电、强制关机）不可取消：平台发布关机原因后 P4App 推入 SaveConfig，SaveConfig 显示原因文案，不提交 Execute。
