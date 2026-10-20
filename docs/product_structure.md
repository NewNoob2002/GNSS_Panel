# 多产品工程结构

同一 HC32F460xE 板卡系列上的三个产品共用一套源码，每次 CMake 配置只构建一个产品（`PRODUCT` 缓存变量）。

| 产品 | 来源分支 | 屏幕 | 版本 | 镜像前缀 | Preset |
|---|---|---|---|---|---|
| X1 | `X1_Panel_dev`（1.30） | 无，按键 + LED | 1.30 | `MCU_LED` | `x1-debug` / `x1-release` |
| X1Pro | `newFrameworkX1pro` | AMOLED 294×126，LVGL | 2.4.4 | `MCU_PRO` | `x1pro-*`（`gcc-*` 为别名） |
| XBase | `newFrameworkXbase`（2.4.6） | 同 X1Pro | 2.4.6 | `MCU_XBASE` | `xbase-*` |

## 目录与依赖

```text
Platform/          MCU 启动、驱动、Arduino 风格外设、共用 HAL（电源策略、充电/电量计、LED、时基、Flash）
Arduino/           Arduino 兼容核心
Libraries/         第三方与器件库；LVGL/Adafruit/ButtonEvent 仅带屏产品构建，MultiButton 仅 X1
Shared/Comm/       I2C 从机传输 + 面板帧协议核心（全部产品）
Shared/Display/    带屏产品入口、显示协议表、显示/编码器 HAL、LVGL 内存池 operator new（X1Pro、XBase）
Simulator/App/     带屏产品共用 LVGL 页面（固件与 PC 模拟器同一份）
Simulator/         PC 模拟器；LVGL 源码直接用 Libraries/lvgl，配置用自身 lv_conf.h（LV_CONF_PATH 指定）
Products/<产品>/   product.cmake、product_config.h、产品专属源码
```

依赖只能自上而下：`Products` → `Shared` → `Platform`/`Libraries`/`Arduino`。`Platform/` 不包含 LVGL 或 `Simulator/App` 头文件；为此做了以下解耦：

- LVGL 使用 `LV_TICK_CUSTOM` 直接读取 `millis()`，SysTick 不再调用 `lv_tick_inc()`。
- C++ `operator new` 平台默认走 C 堆（弱符号），带屏产品在 `Shared/Display/lvgl_new.cpp` 以强符号改用 LVGL 内存池。
- `HAL::Power_OnCheck(idle)` 由入口传入等待开机期间的刷新函数；显示 DMA 回收 `lv_port_disp_poll()` 由带屏主循环调用。
- `SPI.cpp` 只声明 `spi_dma_trans()`，实现在显示驱动中。

## 产品差异放在哪里

1. **功能开关**：`Products/<产品>/product_config.h` 中的 `PRODUCT_*` 宏，经 `mcu_config.h` 被所有源码看到。带屏产品缺少任一开关时 `mcu_config.h` 直接报错，避免 `#if` 未定义被当作 0。
2. **弱符号扩展点**（`Platform/HAL/HAL.h`）：`Display_Sleep`、`Power_ProductInit`、`Power_ProductUpdate`、`Power_AdjustPercent`。默认无动作，产品源码覆盖。
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

## 产品行为约定

- **外部供电关机驻留**（三产品共用 HAL）：断电延时到期后若 MCU 仍有电，清除关机请求、熄灭电源/功能灯、继续充电，置 `ShutdownComplete`。带屏产品据此回到栈底的待机页 Startup（状态栏照常显示，编码器重新启用），长按电源键 2 s 后 HAL 原地开机：重置电源策略、清除主控上报数据（电池采样、器件在线与版本保留）、恢复电源保持与指示灯，界面与冷启动一样进入 HardwareCheck。不使用软件复位开机：Boot 保留 SWRF 会直接开机，且升级窗口会停留约 1 s。
- **电量计不可用时充电**：电量计从未识别时交由充电芯片保护继续充电；在线但样本失效/过期时禁止充电。
- **关机确认**：主控完整读出含关机标志的状态帧（X1Pro/XBase 为 INFO2 `0x02`，X1 为 CTRL `0x02`）即确认，8 s 后断电；X1Pro/XBase 也接受 `0x05`（n=1）。均未确认时从首次请求起 20 s 兜底。

## X1 移植说明

- 开机长按由共用电源监控处理（2 s，与 X1Pro 和 Boot 仓库 X1 契约一致）；旧版因 MultiButton 10 ms 调用 5 ms 节拍，实际约 4 s。
- 功能键长按 2.5 s 强制关机，与 X1Pro（ButtonEvent 500 ms + 10×200 ms）一致；旧版计数用 `LONG_PRESS_START`，实际无法触发，已修正。
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
