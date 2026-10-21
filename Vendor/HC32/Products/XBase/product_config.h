#pragma once

// XBase：与 X1Pro 同一硬件，功能裁剪版（依据 newFrameworkXbase 分支 2.4.6）。

#define PRODUCT_NAME                        "XBase"
#define FIRMWARE_NAME                       "PRO_Panel"
#define PRODUCT_HAS_DISPLAY                 1

#define PRODUCT_LED_BATTERY_BEFORE_POWER_ON 1
#define PRODUCT_MONITOR_PERIOD_MS           15

/* XBase 主控不发送 0x0B / 0x0D */
#define PRODUCT_PROTOCOL_WIFI_CONTROL       0
#define PRODUCT_PROTOCOL_SATELLITE          0

/* 无星图、无电池过温告警页、关机页无 Wi-Fi 按钮；录制类型/间隔不可调 */
#define PRODUCT_PAGE_STAR_MAP               0
#define PRODUCT_PAGE_BATTERY_WARNING        0
#define PRODUCT_SHUTDOWN_WIFI_BUTTON        0
#define PRODUCT_RECORD_SETTINGS             0

/* 画面在屏幕玻璃上的水平偏移（见 X1Pro）；XBase 外壳不遮挡。 */
#define PRODUCT_DISPLAY_X_SHIFT             0

// 2.4.5 关闭 MIDDLE_LOGO_USE
// 2.4.6 注释外部供电变化后的复位逻辑
