# LVGL 9.6 与 X-TRACK 框架迁移设计

本文是阶段 3 的设计依据，由两次只读调研（3.0a：LVGL API、资源、内存；3.0b：框架映射、风险）汇总并经主控对照源码复核。目标版本为 LVGL v9.6.0（上游提交 `80ca777`）。标注“已核实”的结论有源码依据；标注“估算”的数字需在对应子任务中实测。

## 1. 已确认的前提

| 事实 | 依据 | 影响 |
|---|---|---|
| v9.6 公共头文件位于 `include/lvgl/`，根目录 `lvgl.h` 只是 `#include "include/lvgl/lvgl.h"` 的转发 | 已核实：`lvgl.h`、`include/lvgl/lvgl.h` | 导入源码时必须带上 `include/`，CMake 加 `External/lvgl/include` |
| `include/lvgl/lvgl.h` 默认包含 `api_map/lv_api_map_v8.h` 至 `v9_5.h`，定义 `LV_DISABLE_API_MAPPING` 才关闭 | 已核实：`include/lvgl/lvgl.h:212-221` | 兼容层不覆盖显示/输入驱动注册与自定义绘制，迁移后对 `App/`、`Vendor/` 定义 `LV_DISABLE_API_MAPPING`，强制使用 v9 原生 API |
| `lv_color_t` 恒为 3 字节 `{blue, green, red}`，无 `.full` | 已核实：`include/lvgl/draw/lv_color.h:109-113` | 所有 RGB565 绘制/发送缓冲改用 `uint16_t`（误用 `lv_color_t` 会多占约 12 KB 且寻址错乱） |
| `LV_COLOR_16_SWAP` 只作兼容保留，v10 删除 | 已核实：`src/core/lv_refr.c:26-27` | 统一按小端 `LV_COLOR_FORMAT_RGB565` 渲染，在 HC32 `rotate_90()` 写入 `tx_buf` 与 Linux `rotate_90()` 时做字节交换 |
| 软件渲染器支持 `RGB565A8`（RGB565 平面在前、A8 平面在后），不支持 `ARGB8565` | 已核实：`lv_conf_template.h:231-261`、`src/draw/sw/lv_draw_sw_img.c:248-373` | v8 `TRUE_COLOR_ALPHA`（逐像素交错 RGB565+A）必须重排为 `RGB565A8` |
| 索引图（I2/I4）在绘制时由 `decode_indexed()` 解码到堆上的 ARGB8888 | 已核实：`src/image/lv_bin_decoder.c:227,275` | `SystemLoading_RGK`（150×70）每次绘制需约 41 KB 堆；索引图全部预转换为直出格式 |
| `LV_DRAW_LAYER_SIMPLE_BUF_SIZE` 默认 24 KiB | 已核实：`lv_conf_template.h:180` | 带整体透明度的容器渲染时按条带分配，HC32 需按实际需要下调并实测 |

## 2. LVGL API 迁移

### 2.1 语义变化（不是简单改名）

| 区域 | v8 | v9.6 | 位置 |
|---|---|---|---|
| 显示注册 | `lv_disp_draw_buf_t`、`lv_disp_drv_t`、`lv_disp_drv_register` | `lv_display_create` + `lv_display_set_color_format` + `lv_display_set_buffers(..., LV_DISPLAY_RENDER_MODE_PARTIAL)` + `lv_display_set_flush_cb`；`flush_cb(lv_display_t*, const lv_area_t*, uint8_t* px_map)` | HC32 `lv_port_disp.cpp`、模拟器 `main.cpp`、Linux `display/lv_display.c`、`tests/display_app_tests.cpp`、Linux 各测试 |
| rounder | `disp_drv.rounder_cb` | 显示对象的 `LV_EVENT_INVALIDATE_AREA` 事件回调，`lv_event_get_param(e)` 取区域 | HC32 `my_rounder_cb`、Linux `rounder` |
| 输入设备 | `lv_indev_drv_t`、`lv_indev_drv_register` | `lv_indev_create` + `lv_indev_set_type` + `lv_indev_set_read_cb`；周期用 `lv_timer_set_period(lv_indev_get_read_timer(indev), …)` | HC32 `lv_port.cpp`、模拟器、Linux |
| 时基 | `LV_TICK_CUSTOM` 宏 | `lv_tick_set_cb(millis)` / `SDL_GetTicks` | 三端初始化 |
| 日志回调 | `void cb(const char*)` | `void cb(lv_log_level_t, const char*)` | HC32 `HAL_Display.cpp` |
| 内存接口 | `lv_mem_alloc/free` | `lv_malloc/free`（无兼容别名） | `lvgl_new.cpp`、`App/Utils/PageManager` |
| 自定义绘制 | `LV_EVENT_DRAW_PART_BEGIN` + `lv_obj_draw_part_dsc_t` + `lv_draw_*(draw_ctx, …)` | `LV_EVENT_DRAW_MAIN`（或 `DRAW_POST`）+ `lv_event_get_layer(e)` + 描述符内含坐标的 `lv_draw_arc/line/rect(layer, …)` | `App/Pages/Dialplate/DialplateView.cpp`（唯一高复杂度页面） |
| 事件目标 | `lv_event_get_target`（C++ 中需强转） | `lv_event_get_target_obj` | 各页面控制器 |
| 坐标类型 | `lv_coord_t` 为 `int16_t` | `int32_t`（`lv_coord_t` 仍保留为 typedef） | 结构体打包、比较时注意 |

### 2.2 机械改名

`lv_img_*` → `lv_image_*`（`lv_img_set_zoom` → `lv_image_set_scale`）、`LV_IMG_DECLARE` → `LV_IMAGE_DECLARE`、`lv_obj_clear_flag` → `lv_obj_remove_flag`、`lv_obj_del(_async)` → `lv_obj_delete(_async)`、`lv_anim_set_time` → `lv_anim_set_duration`、`lv_anim_set_playback_time/delay` → `lv_anim_set_reverse_duration/delay`、`lv_anim_set_ready_cb` → `lv_anim_set_completed_cb`、`lv_scr_act` → `lv_screen_active`、`lv_indev_get_act` → `lv_indev_active`、`lv_disp_*` → `lv_display_*`、`lv_txt_*` → `lv_text_*`、`LV_RES_OK` → `LV_RESULT_OK`。涉及 `App/Pages/*`、`App/Utils/{SettingsStyle,BatteryStyle}.h`、`App/Utils/lv_anim_label/*`（组合封装，非自定义 `lv_obj_class_t`，只有上述改名）、`App/Resource/ResourcePool.cpp`。

## 3. 资源

**图片**（`App/Resource/img/` 19 个）：

| 类别 | 数量 | 当前格式 | 目标格式 |
|---|---|---|---|
| 脚本生成的 UI 图标（`Vendor/Linux/tools/generate_ui_icons.py`） | 12 | `TRUE_COLOR_ALPHA` | 改脚本直接输出 `RGB565A8` |
| 历史图标 stop、ui_work、workmode | 3 | `TRUE_COLOR_ALPHA` | 从现有数组重排为 `RGB565A8` |
| `SystemLoading_XYZ`（150×36，全不透明） | 1 | `TRUE_COLOR_ALPHA` | `RGB565`（省约 5.4 KB） |
| `NationalFlag_EN/RU`（25×18）、`SystemLoading_Neutral`（90×90） | 3 | `INDEXED_4BIT/2BIT`，调色板全不透明 | `RGB565` |
| `SystemLoading_RGK`（150×70，单色多级透明） | 1 | `INDEXED_4BIT` | `RGB565A8`；若 flash 紧张改 `A8` + `image_recolor` |

转换脚本入库、可重复执行，从现有 C 数组解码再编码，并自带逐像素回读校验（不依赖原始 PNG）。`Neutral`、`RGK` 存为 `A8` 可再省约 29 KiB flash，是否采用由 3.6 的体积结果决定。

**字体**（4 个字体 + `symbol_dialplate.c`）：由 lv_font_conv 1.5.3 生成，已带 `#if LVGL_VERSION_MAJOR == 8` 守卫，v9 原生兼容，无需重新生成；顺带删除 `font_rajdhaniBold_20/40.c` 中未使用的 `lv_font_montserrat_18` 声明。

## 4. HC32 内存预算

**测量口径**：`arm-none-eabi-size` 的 `bss` 已包含 16 KiB `.heap_stack`（8 KiB C 堆 + 8 KiB 栈，`startup_hc32f460.S` 的 `Heap_Size`/`Stack_Size`）。真实余量以链接符号为准：`__RamEnd - __StackTop`。X1Pro release（阶段 2 结束）：`__StackTop = 0x20021850`、`__RamEnd = 0x20027000`，余量 **22,448 B**。`docs/migration_plan.md` 的门槛“`size` 的 data+bss ≤ 176 KiB”相当于保留约 12 KiB，继续沿用。

| 项目 | v8（实测） | v9.6（估算） |
|---|---|---|
| `LV_MEM_SIZE` 池 | 96 KiB | 96 KiB，不变 |
| 绘制缓冲 `draw_buf` + 发送缓冲 `tx_buf` | 2 × 24,696 B | 不变（必须 `uint16_t`） |
| LVGL 全局状态与驱动结构 | — | 约 −0.1 KiB（驱动结构移入池） |
| LVGL 池峰值 | 主机 64 位测试 31,606 B | 约 34–38 KiB（主机），HC32 预计 < 32 KiB；索引图预转换后无 41 KB 解码峰值 |
| `lv_obj_t` | 36 B | 44 B（`LV_OBJ_STYLE_CACHE`），约 140 个对象 +1.1 KiB |
| 栈 | — | 渲染调用链约 2.2–2.8 KiB + 中断，8 KiB 足够（需实测栈水印） |
| flash（release） | 336 KiB | 约 +42 KiB（图片约 +36 KiB，LVGL 代码裁剪后 +5–10 KiB）→ 约 382/456 KiB |
| flash（debug） | 约 389 KiB | 约 434/456 KiB，余量偏紧 |

`lv_conf.h` 关键项（HC32）：`LV_USE_STDLIB_MALLOC/STRING/SPRINTF = LV_STDLIB_BUILTIN`、`LV_MEM_SIZE (96U * 1024U)`、`LV_USE_OS LV_OS_NONE`、`LV_COLOR_DEPTH 16`、`LV_DEF_REFR_PERIOD 16`、`LV_DRAW_SW_SUPPORT_*` 只保留 `RGB565`、`RGB565A8`、`A8`、`ARGB8888`（简单图层需要）、`LV_DRAW_SW_COMPLEX 1`、`LV_DRAW_SW_CIRCLE_CACHE_SIZE 4`、`LV_CACHE_DEF_SIZE 0`、`LV_IMAGE_HEADER_CACHE_DEF_CNT 0`、`LV_BIN_DECODER_RAM_LOAD 0`、关闭 observer/obj_id/obj_name/obj_property/float/matrix/vector_graphic/thorvg、控件只开 arc/image/label/animimg、`LV_USE_SYSMON`/`LV_USE_PERF_MONITOR` 仅 debug。`LV_DRAW_LAYER_SIMPLE_BUF_SIZE` 按实测下调。8 KiB C 堆在带屏产品上基本闲置（`operator new` 走 LVGL 池），可在 3.6 缩减以换 RAM。

## 5. X-TRACK 框架映射

### 5.1 PageManager

| 项目 | `App/Utils/PageManager`（现用） | `Frameworks/PageManager` | 处理 |
|---|---|---|---|
| 生命周期 | `on_view_*` 等 snake_case | `onInstalled/onView*` | 页面改名 |
| 项目扩展虚函数 | `on_language_changed`、`on_next_request`、`on_enter_request`、`on_commit_request`、`wants_power_hold`、`on_back_request` | 无 | 补入框架或应用层派生基类 |
| 管理器扩展 | `preload`、`reset_root`、`get_current_page`、`is_switching`、`notify_language_changed` | 无 | 补入框架（`App.cpp` 依赖） |
| 成员访问 | `root`、`page_manager`、`priv.state` 公有 | 私有，`getRoot/getManager`，无 `getState` | 加 `getState()`，页面改用 getter |
| 返回值 | `bool` | `RES_TYPE`，`OK == 0` | 调用处改成 `== RES_TYPE::OK`，否则 `SystemDash` 关机回滚逻辑反转 |
| 析构 | 释放页面池 | **只 `clearStack()`，页面池泄漏**（已核实 `PageManager.cpp:45-48`） | 修复 |
| 出现顺序 | 先清 `HIDDEN` 再 `on_view_will_appear` | **先 `onViewWillAppear` 后清 `HIDDEN`**（已核实 `PageManagerState.cpp:129-135`） | 改回原顺序，否则 `StarMap` 聚焦被跳过 |
| 切换动画 | 复位非动画轴偏移/透明度，`get_style_x/y`，`lv_anim_refr_now()` | 均缺失 | 合入现有修复 |
| 拖拽返回 | `PageManager_Drag.cpp` | 无，改为 `setBackGestureDirection` | 设备为双键无触摸，不移植 |
| 卸载 | 同步删除 | `lv_obj_del_async` | 保留新行为，注意卸载期间不再访问 |
| LVGL 依赖 | v8 API | `lv_malloc/free`、`lvgl/lvgl.h`，仍用部分 v8 别名 | **只能在 LVGL 9 之后切换**，同时把残留 v8 别名改为原生名 |
| 编译告警 | — | `PageFactory.h:39` 未用形参，`App/` 的 `-Werror` 下编译失败 | 修复 |

### 5.2 ResourceManager

`Frameworks/ResourceManager/ResourceManager.h` 是静态数组模板，零堆分配、无 LVGL 依赖。`ResourcePool.h` 接口保持不变，`ResourcePool.cpp` 改为两张静态表（字体、图片）；`ResourceManagerStatic.cpp` 的模板未实例化且未判空，不使用。

### 5.3 DataCenter → DataBroker

语义基本对应（Account ↔ DataNode、Publish/Pull/Notify 同步分发、`EventParam_t` 栈上构造），差异：DataNode 无 PingPongBuffer（`Commit+Publish` 改为 `publish(data, size)`）、无订阅者时 publish 返回 OK、重复订阅返回已有指针、重写 `onEvent` 时默认事件掩码为 0。`App/` 中约 28 个文件、189 处用法，另有 `Vendor/Linux/tests` 中 25 处。

`App/tests/core_tests.cpp` 覆盖的行为在 DataBroker 中**不成立**的项目（需先加固框架）：

1. `DataTimerManager::handler()` 在遍历 `std::list` 时直接调用回调，回调中删除定时器会使用已释放节点（已核实 `DataTimer.cpp:148-156`）。
2. `DataNode::publish()` 先取下一个订阅者再分发，回调中删除该订阅者会使用已释放节点（已核实 `DataNode.cpp:127-142`）。
3. 无定时器容量上限（现为 32）与节点上限（现为 16）、未设时钟时 `getTick()` 空指针调用、新建定时器 `_lastTick` 初值为 0 导致回绕附近立即误触发、`resetTimer()` 实现错误、`subscribe/pull/notify` 缺参数校验、`DataBroker` 用抛异常的 `new`（项目 `-fno-exceptions`）。

处理：以现有 `Frameworks/DataCenter/DataTimer` 的定长表 + 序号快照实现替换 DataBroker 的定时器，publish 改快照遍历，补容量与参数校验，直到 `core_tests` 全部行为在 DataBroker 上通过。

### 5.4 Bridge → DeviceManager

| DeviceObject | 承接 | read | write | ioctl |
|---|---|---|---|---|
| `Power` | `PowerBridge`、`request_power_on`、`execute_power` | 电源状态报告 | 关机/重启动作 | 请求开机 |
| `Device` | `DeviceBridge`、`initial_device`、`execute_device`、`request_wifi` | 设备快照 + Wi-Fi 状态 | 工作/录制请求（选项下标 ↔ 协议码在此转换） | 设置 Wi-Fi |
| `Status` | `StatusBridge`/`DemoStatus`、`InitSteps`、`capabilities` | 状态栏快照 | — | 读能力、轮询初始化步骤 |

约束保持：`DeviceObject` 只由主循环中的 DataNode 回调同步调用，中断只写 I2C 缓冲；HC32 侧只调用 `State::Request_*`/`State::Effective_*`，revision 采样与 `State::Confirm_*` 仍在 `display_protocol.cpp`，满足 `tests/check_state_ownership.py`。`DeviceObject.h` 默认虚函数未用形参（`-Werror` 失败）与 `ioctl` 中 `(IO_Cmd_t*)&cmd` 的严格别名强转需先修正；`ioctl` 要求传入 `size` 与命令定义完全一致。

## 6. 风险（按严重度）

1. DataBroker 定时器与发布遍历的释放后使用 —— 3.1 先加固并用 `core_tests` 证明。
2. HC32 RAM：余量 22 KiB，图片若保留索引格式会在运行时申请 41 KB；LVGL 9 新增开销需逐项实测 —— 3.2 预转换、3.6 调优。
3. PageManager 缺少应用依赖的接口、页面池泄漏、出现顺序与动画回归、返回值语义反转 —— 3.4 先补框架再切换，逐页截图对比。
4. Debug 构建 flash 余量约 22 KiB —— 3.6 评估 `A8` 图片与 LVGL 模块裁剪。
5. `-Werror` 下框架头文件告警、v9.6 `include/` 目录结构 —— 在对应子任务第一步处理。

## 7. 视觉基线

LVGL 8 下各页面截图已用 `app_navigation_preview <Pages/页面名> <输出.ppm>` 生成（`dev2` 提交 `ea2ef78`，Startup、SystemLoading、Dialplate、WorkSettings、RecordConfig、SystemInfos、SystemDash、StarMap、SaveConfig 及默认导航终态，294×126 PPM）。基线文件存放在主控工作机，不入库；阶段 3 的截图对比由主控执行。
