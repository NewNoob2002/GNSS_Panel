# 共享应用架构：第一阶段

HC32、RK3506（`Vendor/Linux`）与 PC 模拟器（`Vendor/Simulator`）使用同一份 `App/`、`Frameworks/` 源码和 CMake 文件，本仓库为唯一来源。框架与服务可独立于 LVGL 构建；平台通过 `Common/SystemBackend.h` 实现业务操作，UI 通过 Model 发送命令并接收快照。

## 构建边界

| 目标 | 内容 | 允许的依赖 |
|---|---|---|
| `app_framework` | Account、DataCenter、DataTimer、消息缓冲 | 标准 C/C++、`app_log` |
| `app_core` | System/Device/Status 服务、共享数据定义 | 事件框架、日志、无图形依赖的 i18n |
| `app_ui` | 页面、Model、输入路由、页面管理、资源 | `app_core`、LVGL |
| 平台目标 | 设备与业务后端、显示和输入端口、程序入口 | 共享接口及各自平台库 |

父工程通过 `app_build_options` 提供编译选项，通过 `app_lvgl` 提供 UI 所需的 LVGL。没有 `app_lvgl` 时只构建框架与核心。Linux 的 `Vendor/Linux/Backends/Demo` 属于平台演示后端，不在共享 `App/` 中；实屏程序当前仍选择该演示后端，真实 Linux 业务尚未接入。

## 执行和生命周期

- 在应用线程调用 `DataProc_Init(clock)`，注入单调毫秒时钟，再创建页面。核心不需要 `lv_init()`。
- 主循环依次执行平台更新、`DataProc_RunLoop()`（或 `App::update()`）和 `lv_timer_handler()`。框架处理 Account 的业务定时事件，LVGL 处理界面定时器及动画。
- `DataTimer` 参照 X-TRACK `dev_lvgl_v9` 的独立时钟调度方式，实现有界注册、tick 回绕和回调期间销毁保护。框架最多注册 32 个定时器；注册失败由 Account 记录错误。
- Publish/Notify 仍是同步分发，所有服务状态只能由应用线程访问。中断或后台线程不能直接进入它，也不能把现有同步参数指针当作异步消息保存。
- 框架使用标准 C++ 分配接口；HC32 在平台目标中提供的 `operator new` 继续选择 LVGL 池。分配策略不再写进框架。
- 先销毁页面/组件 Account，再调用 `DataProc_Deinit()`；DataCenter 必须比全部 Account 和定时器存活更久。已有发布期间删除订阅者、重复注册与请求校验保持不变。

独立验证（不查找或链接 LVGL）：

```sh
cmake -S App/tests -B build/app-core-tests
cmake --build build/app-core-tests
ctest --test-dir build/app-core-tests --output-on-failure
```

测试入口为 `App/tests`。测试覆盖独立服务初始化、Account 定时事件、回绕、暂停/恢复、注册容量，以及回调销毁自己和其他定时器。

## 后续阶段

本阶段只清理模块、构建和调度边界，保留现有业务行为。后续按以下顺序迁移：

1. 增加拥有消息数据的有界事件入口，明确请求接收、设备接收与实际完成的区别，保留 HC32 revision/确认契约。
2. 将长按与电源编排从页面移到共享服务；电源保持、强制断电、充电保护仍由平台可靠执行。
3. 将初始化推进和业务超时移出页面/Bridge，并移除服务对具体 Model 名称和选项下标的依赖。
4. 提供真实 Linux 业务适配，统一三端事件回放测试。

页面动画、导航和多语言呈现继续属于 UI。X-TRACK LVGL 9 的 PageManager/设备实现不在本阶段整体替换，保留当前 LVGL 8 与已验证的板级时序。
