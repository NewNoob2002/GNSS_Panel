# 共享应用架构

HC32、RK3506（`Vendor/Linux`）与 PC 模拟器（`Vendor/Simulator`）使用同一份 `App/`、`Frameworks/` 源码和 CMake 文件，本仓库为唯一来源。事件/设备框架与服务可独立于 LVGL 构建；平台通过 `Service/HAL/SystemBackend.h` 的 DeviceManager 设备契约实现业务操作，UI 通过 Model 发送命令并接收快照。

## 目录结构

- `Service/DataProc/` 保存 DataProc 与暂存的 SystemService，`Service/HAL/` 保存 SystemBackend 设备契约，`Service/i18n/` 保存多语言文本；原有无 LVGL 源码仍归属 `app::core` 与 `app::i18n`。
- `UI/<页面>/`、`UI/StatusBar/`、`UI/BatteryWarning/` 保存页面和组件，`UI/Resource/` 保存资源与生成工具，`UI/Common/` 保存 UI 样式、ModelUtils 和 RequestFeedback。
- `Utils/FixedFormat/`、`Utils/ButtonGesture/` 与原有 Log、动画工具保存通用功能；`tests/` 位置不变。
- `UI/AppFactory::installAll(PageManager&)` 按 `UI/PageList.inc` 的 `PAGE_DEF(Name, InstallPredicate)` 顺序创建页面并移交所有权，StarMap 按 `SystemService::snapshot().star_map` 过滤；注册失败立即释放当前实例。路由名仍为 `Pages/<页面>`。
- `UI/Page.h` 汇总页面公共包含；`UI/_Template/` 提供最小 Page/Model/View 模板，通过 Linux 主机测试构建编译，但不加入页面清单或固件。

## 构建边界

| 目标 | 内容 | 允许的依赖 |
|---|---|---|
| `app::log` / `app_log` | 统一日志格式化与级别定义 | `proj::options` |
| `app::i18n` / `app_i18n` | 多语言文本支持 | `proj::options` |
| `fw::databroker`, `fw::devicemanager` | DataBroker、DataNode、DataTimer、DeviceManager | `proj::options`、`app::log` |
| `fw::resourcemanager` | 静态资源表模板（INTERFACE） | `proj::options` |
| `app::core` / `app_core` | System/Device/Status 服务、共享数据定义 | 事件框架、日志、`app::i18n`、`proj::options`（不得依赖 LVGL 与 `fw::pagemanager`） |
| `fw::pagemanager` / `app_page_manager` | Frameworks/PageManager 页面池、路由、生命周期与动画 | `lvgl::lvgl`、`app::log`、`proj::options` |
| `app::dialplate` / `app_dialplate` | 表盘与资源支持 | `lvgl::lvgl`、`app::i18n`、`proj::options`、`fw::resourcemanager`、`app::core` |
| `app::ui` / `app_ui` | 页面、Model、输入路由、资源 | `app::core`、`fw::pagemanager`、`app::dialplate` |
| 平台目标 | 设备与业务后端、显示和输入端口、程序入口 | 共享接口及各自平台库 |

父工程通过 `proj::options` 提供编译契约，通过 `lvgl::lvgl` 提供 UI 所需的 LVGL。没有 `lvgl::lvgl` 时只构建框架与核心。Linux 的 `Vendor/Linux/Backends/Demo` 属于平台演示后端，不在共享 `App/` 中；实屏程序当前仍选择该演示后端，真实 Linux 业务尚未接入。

## 执行和生命周期

- 在应用线程调用 `DataProc_Init(clock)`，注入单调毫秒时钟，再创建页面。核心不需要 `lv_init()`。
- 主循环依次执行平台更新、`DataProc_RunLoop()`（或 `App::update()`）和 `lv_timer_handler()`。框架处理 DataNode 的业务定时事件，LVGL 处理界面定时器及动画。
- `DataTimer` 参照 X-TRACK `dev_lvgl_v9` 的独立时钟调度方式，实现有界注册、tick 回绕和回调期间销毁保护。框架最多注册 32 个定时器和 16 个外部节点；节点注册结果通过 `isRegistered()` 查询。定时器创建失败由 DataNode 记录错误。
- Publish/Notify 仍是同步分发，所有服务状态只能由应用线程访问。中断或后台线程不能直接进入它，也不能把现有同步参数指针当作异步消息保存。
- DataBroker 的节点关系和定时器注册使用值成员定长表，不依赖异常分配；DataNode 定时器使用 `new (std::nothrow)` 并检查结果。框架使用标准 C++ 分配接口；HC32 在平台目标中提供的 `operator new` 继续选择 LVGL 池。分配策略不再写进框架。
- 先销毁页面/组件 DataNode，再调用 `DataProc_Deinit()`；DataBroker 必须比全部 DataNode 和定时器存活更久。节点发布使用地址与注册序号快照，允许回调中删除自身订阅或其他订阅者；原地址重建对象不接收旧快照。禁止在回调内销毁当前发布者或 DataBroker。重复注册失败、重复订阅返回空指针，入口校验指针与尺寸。

独立验证（不查找或链接 LVGL）：

```sh
cmake -S App/tests -B build/app-core-tests
cmake --build build/app-core-tests
ctest --test-dir build/app-core-tests --output-on-failure
```

测试入口为 `App/tests`。测试覆盖独立服务初始化、DataNode 定时事件、回绕不追赶、暂停/重置/恢复、回调推进时间后重置后续定时器、定时器及节点容量、重复注册/订阅、入口校验、回调销毁定时器和发布期间删除/原址重建订阅者；另验证设备命令精确尺寸和按值解码。

## 设备与资源边界

三端都通过 DeviceManager 注册三个静态 DeviceObject。`SystemBackend.cpp` 统一执行命令检查和按值拷贝，平台只实现类型明确的同步操作。设备初始化仅标记可用，不改变硬件状态；设备由平台持有，寿命覆盖服务和页面。设备接口继承者与核心统一禁用 RTTI。

| 注册名 | read（尺寸必须完全一致） | write | ioctl |
|---|---|---|---|
| `Power` | `PowerReport` | `PowerAction`（Off/Reboot） | `kRequestPowerOn`：无参数，RES_OK 表示即时确认 |
| `Device` | `DeviceReport`（DeviceSnapshot + Wi-Fi 实际/等待状态） | `DeviceRequest`（ApplyWork/StartRecord/StopRecord） | `kSetWifi`：bool 输入 |
| `Status` | `StatusSnapshot` | 不支持 | `kCapabilities`、`kInitializationInfo`、`kPollInitialization` |

ioctl 编码包含方向、类型（P/D/S）、编号与 `sizeof(载荷)`；未知命令、错误尺寸、非零尺寸的空指针均拒绝。初始化元数据查询不触发探测，轮询结果的详情数组由调用方拥有，名称仅引用平台静态字符串。HC32 的 I2C 探测与主控等待仍在 StatusDevice/InitSteps 中；PowerDevice 保留原 HAL 开关机时序；BusinessDevice 内完成索引与协议码转换，只经 `State::Request_*` 和 `State::Effective_*` 处理请求，revision 采样与确认仍由显示协议拥有。Board 模拟器复用这些对象；DemoStatus 是 Demo 的 Status 设备，确定性/随机采样和初始化耗时保持不变。

`ResourcePool.cpp` 使用 `Frameworks/ResourceManager/ResourceManager.h` 的字体、图片静态表，无堆分配；未知字体返回 LV_FONT_DEFAULT，未知图片返回空指针。`ResourcePool.h` 接口保持不变。LVGL 已升级为 9.6.0；`app_page_manager` 统一编译加固后的 `Frameworks/PageManager`。

页面管理器拥有注册成功的 PageBase 实例；析构时卸载全部页面并释放页面池。根对象异步删除前停止页面动画、清理子对象和外部样式引用，不再引用页面实例。`preload` 只创建可缓存页面；`resetRoot` 清空导航历史后进入指定根页。路由返回 `RES_TYPE`，调用方必须显式比较 `RES_TYPE::OK`（值为 0）；App 对外的导航接口仍返回 bool。生命周期和语言/输入回调统一用 lowerCamelCase，根对象、管理器和状态通过 getter 访问；语言/输入扩展直接放在 PageBase，使管理器与输入路由可以统一分发，无需应用转发基类。

页面切换保持各页 260 ms 缓出滑动；先建页再刷新动画时基，缓存页出现前先取消隐藏，进入时复位未动画轴与透明度。双键设备不启用返回手势，也不保留旧指针拖拽跟随动画；模拟器使用 Esc、右键或页面返回控件，框架仍提供可选 `setBackGestureDirection`。

## 后续阶段

本阶段只清理模块、构建和调度边界，保留现有业务行为。后续按以下顺序迁移：

1. 增加拥有消息数据的有界事件入口，明确请求接收、设备接收与实际完成的区别，保留 HC32 revision/确认契约。
2. 将长按与电源编排从页面移到共享服务；电源保持、强制断电、充电保护仍由平台可靠执行。
3. 将初始化推进和业务超时移出页面/平台设备，并移除服务对具体 Model 名称和选项下标的依赖。
4. 提供真实 Linux 业务适配，统一三端事件回放测试。

页面动画、导航和多语言呈现继续属于 UI；页面管理使用 Frameworks/PageManager，已验证的板级时序保持不变。
