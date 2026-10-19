#pragma once

// X1：无屏面板，依靠按键与 LED 交互（源自 X1_Panel_dev 分支 1.30）。

#define PRODUCT_NAME                        "X1"
#define FIRMWARE_NAME                       "X1_Panel"
#define PRODUCT_HAS_DISPLAY                 0

/* X1 协议 INFO 中上报的硬件版本（沿用 X1_Panel_dev） */
#define HW_VERSION                          "V1.4"

/* 未开机时不点亮电量指示 */
#define PRODUCT_LED_BATTERY_BEFORE_POWER_ON 0
/* MultiButton 以该周期调用 button_ticks()，长按 200 次 = 2 s */
#define PRODUCT_MONITOR_PERIOD_MS           10

/* 电量补偿：充电时电量计读数稳定 20 min 不变即记录到 100% 的差值 */
#define STABLE_TIME_MS                      (20 * 60 * 1000)
/* Boot V2 分配给 App 参数的最后一个 8 KiB 扇区；不可使用 0x7A000（Boot 元数据 A） */
#define APP_PARAM_SECTOR_ADDR               0x0007E000UL
