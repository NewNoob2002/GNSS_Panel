#pragma once
#include "../support/support.h"
#include "mcu_define.h"
// 产品差异（名称、功能开关、带屏与否）由所选产品目录的 product_config.h 提供，见 Products/<产品>/。
#include "product_config.h"

#if !defined(PRODUCT_NAME) || !defined(PRODUCT_HAS_DISPLAY) || !defined(PRODUCT_LED_BATTERY_BEFORE_POWER_ON)          \
    || !defined(PRODUCT_MONITOR_PERIOD_MS)
#error "product_config.h is missing a required PRODUCT_* setting"
#endif
// 功能开关在 #if 中未定义会被当作 0 而静默关闭，带屏产品必须全部显式给出。
#if PRODUCT_HAS_DISPLAY                                                                                                \
    && (!defined(PRODUCT_PROTOCOL_WIFI_CONTROL) || !defined(PRODUCT_PROTOCOL_SATELLITE)                                \
        || !defined(PRODUCT_PAGE_STAR_MAP) || !defined(PRODUCT_PAGE_BATTERY_WARNING)                                   \
        || !defined(PRODUCT_SHUTDOWN_WIFI_BUTTON) || !defined(PRODUCT_RECORD_SETTINGS))
#error "display product_config.h is missing a feature switch"
#endif

#if defined(__has_include)
#if __has_include("mcu_version_config.h")
#include "mcu_version_config.h"
#endif
#endif
#ifndef SOFTWARE_VERSION
#define SOFTWARE_VERSION "V0.0.0"
#endif
#define SOFTWARE_BUILD_DATE     __DATE__
#define SOFTWARE_BUILD_TIME     __TIME__

/* Number to string macro */
#define _VERSION_NUM_TO_STR_(n) #n
#define VERSION_NUM_TO_STR(n)   _VERSION_NUM_TO_STR_(n)

/* Compiler Version */
#if defined(_MSC_FULL_VER)
#define VERSION_COMPILER "MSVCv" VERSION_NUM_TO_STR(_MSC_FULL_VER)
#elif defined(__ARMCC_VERSION)
#define VERSION_COMPILER "ARMCCv" VERSION_NUM_TO_STR(__ARMCC_VERSION)
#elif defined(__GNUC__)
#define VERSION_COMPILER                                                                                               \
    "GCC"                                                                                                              \
    "v" VERSION_NUM_TO_STR(__GNUC__) "." VERSION_NUM_TO_STR(__GNUC_MINOR__) "." VERSION_NUM_TO_STR(                    \
        __GNUC_PATCHLEVEL__)
#else
#define VERSION_COMPILER "UNKNOW"
#endif
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
/* SPI Class */
#define SPI_CLASS_3_ENABLE     1
#if SPI_CLASS_3_ENABLE
#define SPI_CLASS_3_SPI CM_SPI3
#endif

/* Screen */
// #define CONFIG_SCREEN_CS_PIN   PH2
// #define CONFIG_SCREEN_DC_PIN   PB8
// #define CONFIG_SCREEN_RST_PIN  PB9
// #define CONFIG_SCREEN_SCK_PIN  PB6
// #define CONFIG_SCREEN_MOSI_PIN PB7
// #define CONFIG_SCREEN_BLK_PIN  PB3 // TIM3
// #define CONFIG_SCREEN_SPI      SPI_3
// #define CONFIG_SCREEN_HOR_RES  320
// #define CONFIG_SCREEN_VER_RES  172
//////////////////////////////////////////
#define CONFIG_SCREEN_CS_PIN   PB14
#define CONFIG_SCREEN_DC_PIN   PB1
#define CONFIG_SCREEN_RST_PIN  PB2
#define CONFIG_SCREEN_SCK_PIN  PB15
#define CONFIG_SCREEN_MOSI_PIN PB13
// #define CONFIG_SCREEN_BLK_PIN  PB3 // TIM3
#define CONFIG_SCREEN_SPI      SPI_3
#define CONFIG_SCREEN_HOR_RES  294
#define CONFIG_SCREEN_VER_RES  126

/* DMA definition */
#define DMA_UNIT               (CM_DMA1)
#define DMA_CLK                (FCG0_PERIPH_DMA1 | FCG0_PERIPH_AOS)
#define DMA_TX_CH              (DMA_CH0)
#define DMA_TX_TRIG_CH         (AOS_DMA1_0)

#define SPI_TX_EVT_SRC         (EVT_SRC_SPI3_SPTI)
#endif /* PRODUCT_HAS_DISPLAY */
