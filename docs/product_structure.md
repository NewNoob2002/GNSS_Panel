# 多产品工程结构

同一 HC32F460xE 板卡系列上的三个产品共用一套源码，每次 CMake 配置只构建一个产品（`PRODUCT` 缓存变量）。

| 产品 | 来源分支 | 屏幕 | 版本 | 镜像前缀 | Preset |
|---|---|---|---|---|---|
| X1 | `X1_Panel_dev`（1.30） | 无，按键 + LED | 1.30 | `MCU_LED` | `x1-debug` / `x1-release` |
| X1Pro | `newFrameworkX1pro` | AMOLED 294×126，LVGL | 2.4.4 | `MCU_PRO` | `x1pro-*`（`gcc-*` 为别名） |
| XBase | `newFrameworkXbase`（2.4.6） | 同 X1Pro | 2.4.6 | `MCU_XBASE` | `xbase-*` |

## 目录与依赖

```text
Platform/          MCU 启动、驱动、Arduino 风格外设、共用 HAL（电源策略、充电/电量计、按键、LED、时基、Flash），Config/board_pins.h 为主板引脚
Arduino/           Arduino 兼容核心
Libraries/         第三方与器件库；LVGL/Adafruit 仅带屏产品构建，MultiButton 仅 X1
Shared/Comm/       I2C 从机传输 + 面板帧协议核心（全部产品）
Shared/Display/    带屏产品入口、共享应用的 HC32 平台层 AppPlatform/（入口 + Status/Power/Device 三个 Bridge + 初始化步骤）、显示协议表、显示 HAL、LVGL 内存池 operator new（X1Pro、XBase）
Simulator/App/     共享 LVGL 应用，ATK_RK3506 application/App 的原样副本（UPSTREAM 记录来源提交，scripts/sync_shared_app.sh 同步）
Simulator/         PC 模拟器：共享应用 + AppPlatform + Simulator/HAL；LVGL 用 Libraries/lvgl，配置用自身 lv_conf.h（LV_CONF_PATH 指定）
Products/<产品>/   product.cmake、product_config.h、产品专属源码
```

依赖只能自上而下：`Products` → `Shared` → `Platform`/`Libraries`/`Arduino`。`Platform/` 不包含 LVGL 或 `Simulator/App` 头文件；为此做了以下解耦：

- LVGL 使用 `LV_TICK_CUSTOM` 直接读取 `millis()`，SysTick 不再调用 `lv_tick_inc()`。
- C++ `operator new` 平台默认走 C 堆（弱符号），带屏产品在 `Shared/Display/lvgl_new.cpp` 以强符号改用 LVGL 内存池。
- `HAL::Power_OnCheck(idle)` 由入口传入等待开机期间的刷新函数；显示 DMA 回收 `lv_port_disp_poll()` 由带屏主循环调用。
- `SPI.cpp` 只声明 `spi_dma_trans()`，实现在显示驱动中。

## 产品差异放在哪里

1. **功能开关**：`Products/<产品>/product_config.h` 中的 `PRODUCT_*` 宏，经 `mcu_config.h` 被所有源码看到。带屏产品缺少任一开关时 `mcu_config.h` 直接报错，避免 `#if` 未定义被当作 0。
2. **弱符号扩展点**（`Platform/HAL/HAL.h`）：`Power_ProductInit`、`Power_ProductUpdate`、`Power_AdjustPercent`、`Key_ProductInit`、`Key_ProductUpdate`。默认无动作，产品源码覆盖（X1：电量补偿、MultiButton 按键动作）。
3. **协议表**：帧校验、ACK 编码、“完整读出后才生效”在 `Shared/Comm/panel_protocol.cpp`；产品实现 `panel_protocol_packets()`。X1 见 `Products/X1/x1_protocol.cpp`，X1Pro/XBase 共用 `Shared/Display/display_protocol.cpp`。
4. **产品专属源码**：放在产品目录并在其 `CMakeLists.txt` 中加入 `hc32_product`。

| 开关 | X1 | X1Pro | XBase |
|---|---|---|---|
| `PRODUCT_HAS_DISPLAY` | 0 | 1 | 1 |
| `PRODUCT_LED_BATTERY_BEFORE_POWER_ON` | 0 | 1 | 1 |
| `PRODUCT_MONITOR_PERIOD_MS` | 10 | 15 | 15 |
| `PRODUCT_PROTOCOL_WIFI_CONTROL`（0x0B） | – | 1 | 0 |
| `PRODUCT_PROTOCOL_SATELLITE`（0x0D） | – | 1 | 0 |
| `PRODUCT_PAGE_STAR_MAP` | – | 1 | 0 |
| `PRODUCT_PAGE_BATTERY_WARNING` | – | 1 | 0 |
| `PRODUCT_SHUTDOWN_WIFI_BUTTON` | – | 1 | 0 |
| `PRODUCT_RECORD_SETTINGS` | – | 1 | 0 |

XBase 的差异取自其分支相对分叉点的产品级改动（无星图/过温告警页、关机页无 Wi-Fi、录制类型/间隔不可调、协议无 0x0B/0x0D）。该分支中其余页面改动为代码整理，未合入。

### 新增一个产品

复制最接近的 `Products/<产品>/` 目录，修改 `product.cmake` 与 `product_config.h`，在顶层 `HC32_PRODUCTS` 和 `CMakePresets.json` 中登记；需要不同行为时优先新增开关或扩展点。

## 系统状态与所有权

全部运行状态在 `systemInfo`（`Platform/Config/mcu_define.h` 定义类型，`Platform/State/SystemState.{h,cpp}` 持有存储）。`systemInfo` 对所有模块是 `const`，写入按“谁写”分域：

| 域 | 内容 | 写入者 | 入口 |
|---|---|---|---|
| `board` | 软硬件版本、器件在线（BQ40Z50、MP2762A） | HAL I2C 扫描 | `State::Board()` |
| `power` | 电池采样、开关机与关机流程标志 | HAL 电源策略 | `State::Power()`；其它模块经 `HAL::Power_Shutdown()`、`State::Request_ForcedShutdown()`、`State::Host_*()` 请求 |
| `host` | 主控上报：定位、星图、Wi-Fi、录制、工作模式、NTRIP、主控在线 | 面板协议 | `State::Host()` |
| `request` | 面板请求：工作模式、录制启停/设置、Wi-Fi 开关 | 界面（`DeviceBridge`）、X1 按键、低电策略 | `State::Request_*()`；协议 `State::Confirm_*()` |
| `diag` | 报文计数、I2C 错误 | 通信层 | `State::Diagnostics()` |

- 面板请求与主控上报分开保存。请求待确认时，界面与协议响应用请求值（`State::Effective_Work()/Effective_Record()`）；主控上报照常写入 `host`，不再整帧忽略。
- 协议填充响应时记下 revision，主控完整读出后调用 `Confirm_*()`；期间请求又变化则不确认，下次查询再上报。确认后请求值写入 `host`，直到主控下一次上报。
- 线上字段（`radio_change_flag`、`record_op`、`record_change_flag`、`wifi_change_flag`、`wifi_on_off_set`）由协议从 `request` 组装，报文格式不变。
- `tests/check_state_ownership.py`（ctest `state_ownership`）检查各写入口只在其写入者的文件中调用；新增写入者时同步更新该脚本的表。

## 平台层的可复用组件

| 组件 | 位置 | 作用 |
|---|---|---|
| `HAL::Charger` | `Platform/HAL/Charger.{h,cpp}` | 充电控制接口；`Mp2762Charger`（V1.5 主板）与 `GpioCharger`（V1.3 主板）两个实现，`Charger_Get()` 按 I2C 扫描结果选择。温度保护 `Charger_Protect()` 与电量计读取 `Battery_Sample()` 与芯片无关 |
| `PowerPolicy` | `Platform/HAL/PowerPolicy.h` | 关机时序与低电计时（纯逻辑，固件与模拟器共用） |
| `HoldTimer` | `Platform/HAL/HoldTimer.h` | 按住计时；Function 键 2.5 s 强制关机（`HAL_Key.cpp`，三产品共用） |
| `BlinkLed` | `Platform/HAL/BlinkLed.h` | 固定周期闪烁的指示灯 |
| `StatusBridge` / `PowerBridge` / `DeviceBridge` | `Shared/Display/AppPlatform/` | `systemInfo`/HAL 与共享应用之间各一个方向的转换，状态在对象内 |

### 常见扩展

- **新的主板版本或充电方案**：在 `Charger.cpp` 增加一个 `HAL::Charger` 实现，并在 `Charger_Get()` 中按检测结果返回；电源与 LED 代码不需要改。
- **新的业务数据或命令（带屏）**：共享接口先在 ATK_RK3506 增加（`DeviceSnapshot`/`DeviceRequest`），同步后在 `DeviceBridge::snapshot()/execute()` 中映射：读 `systemInfo.host` 或 `State::Effective_*()`，写经 `State::Request_*()`；主控上报的新字段加到 `HostReport_t` 并在 `display_protocol.cpp` 收发。
- **产品专属按键动作**：实现 `HAL::Key_ProductInit()/Key_ProductUpdate()`；强制关机等共用行为不要复制。

## 产品行为约定

- **外部供电关机驻留**（三产品共用 HAL）：断电延时到期后若 MCU 仍有电，清除关机请求、熄灭电源/功能灯、继续充电，置 `ShutdownComplete`。带屏产品由 AppPlatform 上报待机，共享应用重置页面栈回到 Startup，长按电源键 2 s 后 HAL 原地开机：重置电源策略、清除主控上报数据（电池采样、器件在线与版本保留）、恢复电源保持与指示灯，界面与冷启动一样进入 SystemLoading。不使用软件复位开机：Boot 保留 SWRF 会直接开机，且升级窗口会停留约 1 s。
- **电量计不可用时充电**：电量计从未识别时交由充电芯片保护继续充电；在线但样本失效/过期时禁止充电。
- **关机确认**：主控完整读出含关机标志的状态帧（X1Pro/XBase 为 INFO2 `0x02`，X1 为 CTRL `0x02`）即确认，8 s 后断电；X1Pro/XBase 也接受 `0x05`（n=1）。均未确认时从首次请求起 20 s 兜底。

## X1 移植说明

- 开机长按由共用电源监控处理（2 s，与 X1Pro 和 Boot 仓库 X1 契约一致）；旧版因 MultiButton 10 ms 调用 5 ms 节拍，实际约 4 s。
- 功能键按住 2.5 s 强制关机，三个产品共用 `Platform/HAL/HAL_Key.cpp`（`HoldTimer`，与旧 X1Pro ButtonEvent 500 ms + 10×200 ms 一致）；旧版 X1 计数用 `LONG_PRESS_START`，实际无法触发，已修正。
- 电量补偿参数从旧地址 `0x7A000`（Boot V2 元数据 A）改到 App 参数扇区 `0x7E000`，只写首字，读出值不在 0..9 时视为未保存。旧设备上已保存的补偿值不会迁移；Boot 仓库的 X1 App 在该扇区使用不同的记录格式，两者交替烧录时补偿值会失效。
- 补偿差值 ≥ 10 时不采用（旧版写入 RAM 但不保存）。
- 擦除参数扇区时总线保持约 18 ms，期间中断暂停。
- 参数扇区位于 App 区之外（Boot V2 预留的末尾 8 KiB），不占 App 容量；补偿功能代码约 952 B（补偿逻辑 208 B、Flash 辅助 180 B、EFM 驱动 564 B）。

## Flash 容量

片内 512 KiB：Boot 32 KiB | App 456 KiB（`0x8000..0x79FFF`）| 元数据 A/B 各 8 KiB | App 参数 8 KiB。

| 产品 | Release 占用 / 剩余 | Debug 占用 / 剩余 |
|---|---|---|
| X1 | 25,216 B / 431.4 KiB | 37,488 B / 419.4 KiB |
| X1Pro | 332,048 B / 131.7 KiB | 390,188 B / 75.0 KiB |
| XBase | 330,584 B / 133.2 KiB | 388,580 B / 76.5 KiB |

## 待确认事项

- 主控写入请求后从不读取时，未读响应会阻塞后续请求（含关机/复位），目前没有过期机制。
- X1 未在实板验证；XBase 仅构建、主机测试与模拟器验证。
