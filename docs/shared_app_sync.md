# HC32 与 RK3506 共享应用

共享应用唯一源码位于 ATK_RK3506 `application/App`，HC32 的原样副本位于 `Shared/App`，来源提交见 `Shared/App/UPSTREAM`。架构清理分支为双方的 `refactor/shared-app-architecture`。

## 当前边界

共享的 CMake 文件将源码分成以下目标，固件、模拟器和主机测试直接链接这些目标，不再各自递归收集 App 源码：

| 目标 | 职责 | 图形依赖 |
|---|---|---|
| `app_framework` | Account 发布、通知、查询与 DataTimer | 无 |
| `app_core` | System/Device/Status 服务与共享数据 | 无 |
| `app_ui` | 页面、Model、状态栏、输入路由、资源与页面管理 | LVGL |

父工程通过 `app_build_options` 提供工具链/产品编译选项，通过 `app_lvgl` 提供显示库。核心可以单独构建，不包含 HAL、Arduino 或 Linux 设备头文件。完整职责、线程约定和后续阶段见 [共享应用架构](../Shared/App/ARCHITECTURE.md)。

RK 的模拟业务实现位于共享目录之外的 `application/Backends/Demo`。`p4_screen` 当前仍使用这个演示业务后端；真实 Linux 电源、设备业务尚未接入，不能将实屏驱动通过视为业务后端已经完成。

HC32 特有代码继续放在 `Shared/Display/AppPlatform/`：

| 文件 | 职责 |
|---|---|
| `AppPlatform.cpp` | 创建应用、运行核心循环、适配 system_backend、按键输入、日志与致命错误处理 |
| `PowerBridge` | HAL 电源状态转为共享服务状态 |
| `DeviceBridge` | 设备快照、业务命令与主控协议编码转换 |
| `StatusBridge` | 状态栏快照 |
| `InitSteps.cpp` | HC32 初始化检查与主控通信等待 |

充电保护、看门狗、强制断电、电源保持和主控协议确认仍由 HC32 平台处理。本阶段只清理目录、构建和事件调度边界；页面的长按、初始化推进及关机提交规则留待后续服务迁移。

## 事件运行与内存

- DataTimer 采用 X-TRACK 独立时钟的调度方式，保留当前 Account 的注册序号、容量、发布期间销毁订阅者和请求校验保护。
- `DataProc_Init(clock)` 注入单调毫秒时钟；主循环通过 `App::update()` 执行核心循环，之后运行 `lv_timer_handler()`。
- 所有状态在同一个应用线程中访问，Publish/Notify 仍同步分发；中断/后台线程不能直接进入服务，也不能保存同步回调中的临时指针作为异步消息。
- 先销毁页面和组件，再销毁服务。框架使用标准 C++ 分配接口，HC32 的平台 `operator new` 继续使用 LVGL 池。
- 异步有界消息入口、共享电源/初始化服务、去除具体 Model 身份依赖以及真实 Linux 后端属于下一阶段。

## 同步流程

1. 在 ATK_RK3506 修改 `application/App`，运行测试并提交。
2. 在 HC32 执行 `scripts/sync_shared_app.sh <ATK_RK3506 路径> <提交>`。
3. 复核 `Shared/App/UPSTREAM`，运行主机测试、三产品 Debug/Release 构建及两个模拟器检查。

同步包括上游 `CMakeLists.txt`。HC32 仅保留 `UPSTREAM` 和禁止就地格式化的 `.clang-format`；不要直接修改共享副本。RK 演示后端位于共享目录之外，不会同步到 HC32。

## 已有行为契约

- Startup 每次新按压必须重新计满 2 s，快速松开再按下不得累计回退动画残余进度。带屏后端调用 `HAL::Power_RequestOn()`，HAL 接受后发布开机状态；X1 保留原始按键长按路径。
- 普通上电先保持 `POWER_CONTROL_PIN=LOW`，确认后供电并锁存；软件复位立即保持 HIGH，保留 Boot/app_fatal 的恢复路径。
- 外部供电下关机完成进入驻留，清除主控数据与待确认请求后可以原地开机。充电、电池与板卡信息保留。
- 关机确认仍是主控完整读出对应状态帧（或带屏 `0x05`），确认后 8 s 断电、未确认时从首次请求起 20 s 兜底。
- 工作模式、录制、Wi-Fi 的“收到请求”与物理完成分开。保留 revision 和完整读出确认；返回页面或提示超时不清除平台请求。
- 初始化检查仍有界、失败不阻止进入首页；V1.3 无 MP2762A 是正常配置，主控通信等待最多 30 s。

## 验证入口

```sh
# 完全独立于 LVGL 的共享框架与核心
cmake -S Shared/App/tests -B build/shared-core
cmake --build build/shared-core
ctest --test-dir build/shared-core --output-on-failure

# 协议、电源、状态所有权和两显示产品
cmake -S tests -B build/host-tests
cmake --build build/host-tests
ctest --test-dir build/host-tests --output-on-failure
```

硬件兼容性仍需实板检查；主机测试不证明供电时序、显示 DMA、物理按键或 Linux 系统关机已经验收。
