#pragma once
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
        || !defined(PRODUCT_SHUTDOWN_WIFI_BUTTON) || !defined(PRODUCT_RECORD_SETTINGS)                                 \
        || !defined(PRODUCT_DISPLAY_X_SHIFT) || !defined(PRODUCT_DISPLAY_BRIGHTNESS_FADE))
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
/* 主板引脚与外设分配（三个产品同一主板） */
#include "board_pins.h"
