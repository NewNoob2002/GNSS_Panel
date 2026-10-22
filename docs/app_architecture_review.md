# App / Frameworks 架构复审与重构方案（阶段 3.7）

参考：FASTSHIFT/X-TRACK `dev_lvgl_v9`，提交 `28b0e90`（`Firmware/X-TRACK/App`、`Frameworks`）。对比对象：`dev2` `2906f1e`。
目标：整体架构参照 X-TRACK，组件在其基础上优化、扩展；界面外观与交互不变（九页截图逐字节一致），HC32 RAM/flash 预算不倒退。

## 1. 复审结论（按影响排序）

| # | 问题 | 现状（证据） | 参考做法 |
|---|---|---|---|
| F1 | **App 单体** | `App.cpp` 364 行同时负责入口、两键手势、导航定时器、开机/关机/待机状态机、预加载、语言广播、状态栏喂数据 | `App_CreateContext` 只组装 DataBroker、DataProc、PageManager、StatusBar，用 `Global` 节点广播生命周期事件；导航、电源、按键由各自的 DataProc 节点负责 |
| F2 | **数据粗粒度且由平台推送** | 只有 `Status`/`System`/`Device` 三个大快照（`DeviceSnapshot` 40+ 字段）；平台调用 `DeviceService::update()` 等推送整份快照，任一字段变化所有订阅者都要刷新 | 按领域拆成独立节点（Power、GNSS、Clock、Recorder、Storage…），节点自己通过 `HAL::Manager()->getDevice()` 访问设备，定时或事件驱动 `publish` |
| F3 | **按发送方名字授权** | 14 处 `sender_is(event, "SystemDashModel")` 等字符串比较，权限与页面类名耦合 | 命令由节点按语义和自身状态校验，不关心发送方是哪个页面 |
| F4 | **页面 MVP 不完整** | Model 不是 DataNode，`model_utils` 写死订阅三个快照；Page 直接操作 View 控件（5 个页面共 45 处 `view_.ui.*`）；数据变化用 `std::function` 回调 | Model 继承 `DataNode` 只订阅所需节点，通过 `EventListener` 通知 Page；View 按 `MSG_ID` 分发消息、用 `Binding` 读写数据；Page 只做转发与导航 |
| F5 | **StatusBar 被动** | 数据靠 `App::update_status()` 从外部喂入 | StatusBar 是完整的 Page/Model/View，自行订阅数据 |
| F6 | **页面注册手工** | `App::init` 中逐个 `register_page`，StarMap 等按能力条件硬编码 | `AppFactory` + `APP_DESCRIPTOR_DEF` 自注册，`_Template` 作为新页面模板 |
| F7 | **平台契约混杂** | `SystemBackend.h` 把电源、业务、状态栏、能力、初始化塞进三个粗设备，并夹带 ioctl 元数据；HAL 定义放在 `App/Common` | `Service/HAL/Def/HAL_xxx.h` 每个设备一个头文件，DeviceManager 注册，平台按设备实现 |
| F8 | **目录不对应分层** | `Common/`、`Pages/`、`Utils/` 混放服务、HAL 契约、页面与样式 | `Config/`、`Service/{DataProc,HAL,i18n}`、`UI/{各页面,StatusBar,Resource,_Template}`、`Utils/` |

保留或已优于参考的部分：
- **Frameworks**：DataBroker、DeviceManager、PageManager、ResourceManager 已对齐参考，并做过加固（释放后使用、容量与参数校验、RES_TYPE 语义）。只需随节点拆分调整容量。
- **i18n**：`TextId` 枚举加编译期表，比参考的 `lv_i18n`（yml + 运行时字符串查找）更省 flash、类型更安全。保留，迁到 `Service/i18n`。
- **定点格式化、Logo 布局资源、样式工具**：保留，按新目录归位。

参考组件里需要修正后再采用的部分：
- `Utils/Binding/Binding.h`：第一个构造函数中的 `Binding(setFunc, getFunc, ctx);` 只构造了一个临时对象，回调指针未设置。
- `Utils/lv_msg`：LVGL 9 已内置其后继 `lv_subject`/`lv_observer`（见第 4 节决策 D1）。

## 2. 目标结构

```
App/
  App.{h,cpp}            AppContext：组装与生命周期（Global 事件），不含业务
  Config/Config.h        产品无关的应用配置
  Service/
    DataProc/            DP_<Name>.cpp + Def/DP_<Name>.h + Helper/<Name>_Helper.*，DataProc_NodeList.inc
    HAL/                 HAL_Def.h + Def/HAL_<Device>.h（设备命令与数据结构），平台实现在 Vendor/
    i18n/                现有 TextId 表
  UI/
    AppFactory.*, PageList.inc, Page.h, _Template/
    <Page>/              <Page>.{h,cpp}（Presenter）、<Page>Model.*（DataNode）、<Page>View.*
    StatusBar/、BatteryWarning/（同样 MVP）
    Resource/            ResourcePool 与生成资源
  Utils/                 动画标签、FixedFormat、ButtonGesture、Log 等（UI 专用样式工具在 UI/Common）
```

依赖规则（写入 `check_layers.py` 的目录映射）：UI → Service/DataProc → Service/HAL（仅头文件契约）→ Frameworks；Vendor 实现 Service/HAL 设备并注册到 DeviceManager；UI 不得直接包含 Service/HAL。

## 3. DataProc 节点划分（初稿）

| 节点 | 来源字段（现有结构） | 设备 / 驱动方式 | 命令 |
|---|---|---|---|
| Global | 新增 | — | 生命周期事件（INIT_FINISHED、APP_STARTED、RUN_LOOP…） |
| PageNavi | `App::on_navigation` | PageManager | PUSH / POP / RESET_ROOT |
| Button | `App::on_key`、`ButtonGesture` | 平台按键设备 | 发布单击/双击/长按手势 |
| Power | `SystemState` 电源字段、`PowerReport` | HAL_Power | POWER_ON、SHUTDOWN、REBOOT；发布 powered_on/shutting_down/standby/cause |
| Battery | `StatusSnapshot`/`DeviceSnapshot` 电量、电压、温度、充电 | HAL_Battery | — |
| GNSS | 位置、卫星数、各星座 | HAL_GNSS | — |
| Clock | 时间、日期 | HAL_Clock | — |
| Work | 工作模式、电台协议/信道、NTRIP | HAL_Work | APPLY（替代 `DeviceCommand::ApplyWork`） |
| Recorder | 录制状态、类型、间隔、文件名、空间 | HAL_Recorder | START / STOP |
| WiFi | Wi-Fi 可用、开关、STA/AP、SSID、IP | HAL_WiFi | SET |
| Version | 固件、硬件、构建、运行时长、产品能力 | HAL_Board | — |
| Init | 初始化步骤与结果 | HAL_Board（步骤元数据与轮询） | BEGIN / RECORD |
| Settings | 语言 | 平台存储（可选） | SET_LANGUAGE |
| StatusBar | 状态栏可见性与样式 | — | 页面设置状态栏外观（参考同名节点） |

节点数约 14 个，加上 9 个页面 Model、StatusBar 和 BatteryWarning，共约 25 个，DataBroker `MaxNodes` 需从 16 提高到 32。每个 DataNode 的内存成本要在 HC32 上实测（带屏产品的 `new` 走 LVGL 池）。

## 4. 设计决策（2026-10-22 用户确认）

- **D1 View 数据分发**：用 LVGL 9 内置的 `lv_subject`/`lv_observer`，替代参考的 `lv_msg` + `Binding`（官方后继，无需移植维护）。release 需开启 `LV_USE_OBSERVER`，flash 增量在 3.7e 实测。参考的 `Binding`、`lv_msg` 不移植。
- **D2 数据驱动方式**：节点经 DeviceManager 从细粒度设备拉取（定时或平台事件触发），只在数据变化时 `publish`；平台不再推送整份快照。HC32 侧写入仍只经 `State::Request_*`。
- **D3 目录重排**：按第 2 节一次性重排（`git mv` 保留历史），之后各步只改内容。
- **页面注册**：参考的 `APP_DESCRIPTOR_DEF` 依赖静态对象自注册，在静态库中会被链接器丢弃；改为 `UI/PageList.inc` 显式清单（与 DataProc 节点清单同一写法），条件页面（如 StarMap）由安装判断函数过滤。

## 5. 分步实施（每步独立验收）

| 步 | 内容 | 行为变化 |
|---|---|---|
| 3.7a | 目录重排；`AppFactory` + `UI/PageList.inc`；`_Template` | 无 |
| 3.7b | `Service/HAL/Def` 细粒度设备契约；HC32、Linux Demo、Board 模拟器三端实现（与旧粗设备并存） | 无 |
| 3.7c | DataProc 拆分为第 3 节节点；`MaxNodes` → 32；删除 `*Service::update` 推送与按名字授权 | 无（数据路径改变） |
| 3.7d | App 瘦身为 AppContext；按键 → Button 节点；导航 → PageNavi；电源状态机迁入 DP_Power | 无 |
| 3.7e | 页面统一 MVP：Model 继承 DataNode 只订阅所需节点，View 经 D1 机制更新，Page 不再直接操作控件；StatusBar、BatteryWarning 自治 | 无 |
| 3.7f | 删除 `SystemBackend`/`SystemService`/`model_utils` 等旧路径；测试迁移；`AGENTS.md`、`App/ARCHITECTURE.md` 更新 | 无 |

每步验收：6 个固件 preset、host-tests、app-core-tests、linux-host、三个模拟器冒烟、`check_layers.py`、`check_state_ownership.py`；九页截图与 3.6 后基线逐字节一致；X1Pro/XBase release 体积与 RAM 余量报告（RAM 余量不少于 8 KiB，主机 LVGL 池峰值不明显上升）。模拟器交互由用户对比确认。
