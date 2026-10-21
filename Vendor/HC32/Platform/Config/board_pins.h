#pragma once

// 主板引脚与外设分配：三个产品共用同一主板（V1.3/V1.5 仅充电芯片不同，见 HAL::Charger）。
// 由 mcu_config.h 在读入 product_config.h 之后包含（显示相关部分取决于 PRODUCT_HAS_DISPLAY）。

/*POWER*/
#define POWER_CONTROL_PIN      PB3
#define WATCHDOG_FEED_PIN      PA6
#define WATCHDOG_FEED_TIME     5000

/*Function Key*/
#define FUNCTION_KEY_PIN       PA15

/*power_key_pin*/
#define POWER_KEY_PIN          PA0

/*Status LED*/
#define POWER_LED_PIN          PC13
#define CHARGE_LED_PIN         PH2
#define FUNCTION_LED_PIN       PB5

/*charger_control*/
#define CHARGER_ENABLE_PIN     PA4
#define CHARGER_CTRL_PIN       PB7
#define CHARGER_CTRL_FAST_PIN  PB6
#define CHARGER_SWITCH_PIN     PB10
#define BATTERY_TERMINATE_TEMP (float)45.0f

#define CHARGER_ADC_DETECT_PIN PA1

#define USB_SWITCH_PIN         PB8

#if PRODUCT_HAS_DISPLAY
/* Screen */
// #define CONFIG_SCREEN_CS_PIN   PH2
// #define CONFIG_SCREEN_DC_PIN   PB8
// #define CONFIG_SCREEN_RST_PIN  PB9
// #define CONFIG_SCREEN_SCK_PIN  PB6
// #define CONFIG_SCREEN_MOSI_PIN PB7
// #define CONFIG_SCREEN_BLK_PIN  PB3 // TIM3
// #define CONFIG_SCREEN_HOR_RES  320
// #define CONFIG_SCREEN_VER_RES  172
//////////////////////////////////////////
#define CONFIG_SCREEN_CS_PIN   PB14
#define CONFIG_SCREEN_DC_PIN   PB1
#define CONFIG_SCREEN_RST_PIN  PB2
#define CONFIG_SCREEN_SCK_PIN  PB15
#define CONFIG_SCREEN_MOSI_PIN PB13
// #define CONFIG_SCREEN_BLK_PIN  PB3 // TIM3
#define CONFIG_SCREEN_HOR_RES  294
#define CONFIG_SCREEN_VER_RES  126

// SPI3、DMA1 CH0、AOS 与 IRQ 分配由 Display/board_display.cpp 的板级实例保存。
#endif /* PRODUCT_HAS_DISPLAY */
