# 共享应用架构

HC32、RK3506（`Vendor/Linux`）和 SDL 模拟器（`Vendor/Simulator`）共用本仓库的 `App/`、`Frameworks/`，不从外部仓库同步。架构参照 X-TRACK `dev_lvgl_v9` 的领域节点与页面 MVP，保留本项目的同步调用、板级时序和页面交互。

## 目录与依赖

`UI → Service/DataProc → Service/HAL → Frameworks`。UI 只直接包含领域契约，不能直接包含 `Service/HAL/` 头文件；`tests/check_layers.py` 检查完整路径、缩短路径和裸头文件名。领域状态可按值复用 HAL 类型，不把平台实现引入应用。

- `App.{h,cpp}` 组装资源、DataProc、PageManager、焦点组、StatusBar、BatteryWarning 和 AppNavigator，并按所有权逆序清理。
- `Service/DataProc/` 包含十二个可销毁节点；`Def/DP_*.h` 提供类型化状态、命令、订阅、pull/notify/receive，节点 ID 集中定义于 `Helper/NodeHelper.h`，创建顺序来自 `DataProc_NodeList.inc`。
- `Service/HAL/HAL_Manager.h` 声明平台实现的 `hal::manager()`；`HAL_Def.h` 提供按值、精确尺寸的设备解码；`Def/HAL_*.h` 定义细设备契约，`PowerObject` 负责电源设备解码。
- `Service/i18n/` 保留 TextId 枚举和编译期语言表；`Utils/` 保存定点格式化、ButtonGesture、日志和动画工具。
- `UI/<页面>/`、`UI/StatusBar/`、`UI/BatteryWarning/` 使用 Page/Model/View；`UI/Common/` 保存 Model、样式与请求反馈，`UI/Resource/` 保存静态资源。`UI/_Template/` 编入 Linux 主机测试但不注册。
- `UI/AppFactory` 按 `PageList.inc` 顺序安装九个路由页；StarMap 按 Version 节点的能力过滤。清单同时生成 PageId，外部路由保持 `Pages/<页面>`。

## 构建边界

| 目标 | 职责与依赖 |
|---|---|
| `app::log`、`app::i18n` | 日志与语言；依赖 `proj::options` |
| `app::framework` | 汇总 `fw::databroker`、`fw::devicemanager`、`fw::resourcemanager` |
| `app::core` | DataProc、HAL 公共解码和通用工具；不依赖 LVGL 或 PageManager |
| `fw::pagemanager` / `app_page_manager` | 页面池、生命周期与过渡；依赖 LVGL 和日志 |
| `app::dialplate` | 表盘 View、资源、动画；依赖 LVGL、i18n、ResourceManager 与核心 |
| `app::ui` | App、导航/输入与全部页面 MVP；依赖 core、PageManager、dialplate |
| 平台目标 | 实现 HAL 细设备、显示/输入与入口，链接共享目标 |

父工程提供 `proj::options` 与 `lvgl::lvgl`；没有 LVGL 时仍可构建核心及 `App/tests`。Frameworks 保留各自 CMake 目标和许可，不包含平台分配策略。

## 领域节点与平台设备

| DataProc 节点 | HAL 设备 / 调度 | 状态与命令 |
|---|---|---|
| Power | Power；20 ms | 平台电源状态、待消费导航边沿、Prepare/Cancel/Execute token 与 RequestOn |
| Battery、GNSS | 同名设备；200 ms | 电量/温度/充电与定位/卫星/星座值 |
| Clock | Clock；200 ms | 平台运行时钟、独立 RAM 日期；SetDate 校验 2000..2099 和公历日期，不写 HAL |
| Work、Recorder、WiFi | 同名设备；500 ms | 设置与 pending、录制和主控实际录制、Wi-Fi 回报/请求 |
| Version | Board；1000 ms | 固件/硬件/构建/运行时长与产品能力 |
| Init | Init；按需 | Begin/Record 保存最近一次结果；类型化 info/poll 查询元数据和轮询，不发布中间态 |
| Settings | 无设备 | 当前语言，值变化时发布 |
| Button | 应用线程输入采样 | 去抖、单/双击与 held；每次 Poll 发布一份动作/电平 |
| PageNavi | 无设备 | 非 UI 请求的八项有界 FIFO；以数值 PageId 和已配置上界校验 |

十个领域节点加 Button/PageNavi，共十二个 provider。轮询失败保留最近成功值，稳定值不重发；命令后立即采样 pending。Status/System/Device 聚合节点、200 ms 兼容重发、SystemService、ModelUtils 与粗快照已删除。日期与初始化类型分别由 DP_Clock、DP_Init 持有。

平台持有九个静态 DeviceObject：Power、Battery、GNSS、Clock、Work、Recorder、WiFi、Board、Init。设备寿命覆盖应用，初始化只标记可用；未知命令、错误方向/尺寸和非零尺寸空指针均拒绝，外部缓冲区按值拷贝，不假设对齐。

HC32 的 BusinessDevice 仅负责细领域采样与协议映射，不注册为设备；选项下标和协议码转换只在此处，写入只经 `State::Request_*`，读取有效请求经 `State::Effective_*`，revision 采样与确认仍属于显示协议。StatusDevice 是 InitSteps/Board 的平台能力及初始化辅助对象，不承担聚合发布。Board 模拟器编译同一实现；Linux 和 Demo 模拟器共用 `Backends/Demo` 单一模型，真实 Linux 业务仍待接入。

## 页面与执行时序

Model 继承 DataNode，只订阅所需领域并立即 pull 首值；自有值状态通过 LVGL 9 `lv_subject`/`lv_observer` 通知 View，参数不跨回调保存。Page 负责生命周期、输入转发及同步导航；View 管控件、展示和动画，离页解除 observer 并清理借用对象地址的回调。StatusBar 与 BatteryWarning 自行订阅，App 不再喂状态。StatusBar 保留外观命令入口，无独立 Global 或同名 DataProc provider。

AppNavigator 是 UI 组装对象，其 20 ms LVGL 定时器消费 Power 边沿和 PageNavi 队列，繁忙切页后重试。页面点击仍同步调用 PageManager，以保留导航返回值、拒绝回滚和时序；PageNavi 仅供非 UI 请求。SystemLoading 期间只预加载 Dialplate 与 SaveConfig，每节拍最多新建一页。

SystemLoading 的元数据从 Model 经 `dp::Init::info` 读取后交给 View，检查经 `dp::Init::poll` 执行。Logo 800 ms、50 ms 轮询、单步至少 300 ms、结果至少 150 ms、100% 停留 200 ms、Ready 停留 1000 ms 均保留；平台有界超时之外仍有 `kStepLimitMs = 35000` 兜底。失败记录为红色并继续后续步骤，Pending 不写入已完成结果，Begin 清除上次报告。

PageManager 拥有已注册页面；导航结果必须显式比较 `PageManager::RES_TYPE::OK`（值为 0）。各页保留 260 ms 缓出位移过渡，建页耗时不计入动画；根对象异步删除前清理动画、控件与外部样式引用。双键输入和 SDL 输入各用既有归一化路径，页面成员通过 getter 访问。

所有 DataNode、设备、输入、定时器和 LVGL 操作只在应用线程执行。DataBroker 最多注册 32 个外部节点和 32 个独立时钟定时器；Publish/Notify/Pull 同步完成，禁止中断/后台线程直接分发或保存载荷指针。发布时使用地址加注册序号快照，可在回调中解除订阅或删除订阅者，禁止销毁当前发布者或 Broker。

初始化先注入单调毫秒时钟 `DataProc_Init(clock)`，再创建页面；主循环运行平台更新、`App::update()`（即 DataProc_RunLoop）和 `lv_timer_handler()`。销毁先停止导航定时器、销毁页面和组件，再逆序销毁领域节点；Broker 必须比全部节点和定时器存活更久。HC32 的标准 `new` 仍由平台映射到 LVGL 池，Frameworks 不选择分配器。

资源通过 ResourceManager 静态数组模板查找，不动态注册；未知字体返回 LV_FONT_DEFAULT，未知图片返回空指针，ResourcePool 公开接口不变。LVGL 为 9.6.0，RGB565 原生渲染，HC32/Linux 在发送缓冲区交换字节，仅使用 v9 原生 API。

## 验证

`App/tests` 无 LVGL，覆盖框架生命周期、容量、同步边界、计时器、领域发布/命令、Clock 日期与 Init 查询/留存；`tests` 验证 HC32 协议映射、pending/revision、平台时序与 LVGL 池；Linux 测试验证页面 MVP、导航/输入、双语布局和截图。构建与检查命令见根 `AGENTS.md`；CMake 层级和自有源码包含规则由 `tests/check_layers.py` 检查，HC32 状态单写者由 `check_state_ownership.py` 检查。
