#include "ResourcePool.h"
#include "ResourceManager.h"
#include "LogoParts.h"
#include <cstring>

extern "C" {
LV_FONT_DECLARE(font_oswaldBold_12);
LV_FONT_DECLARE(font_oswaldBold_18);
LV_FONT_DECLARE(font_rajdhaniBold_20);
LV_FONT_DECLARE(font_rajdhaniBold_40);
LV_FONT_DECLARE(symbol_dialplate);
LV_IMAGE_DECLARE(img_src_battery);
LV_IMAGE_DECLARE(img_src_battery_info);
LV_IMAGE_DECLARE(img_src_record);
LV_IMAGE_DECLARE(img_src_clock);
LV_IMAGE_DECLARE(img_src_mode);
LV_IMAGE_DECLARE(img_src_settings);
LV_IMAGE_DECLARE(img_src_menu);
LV_IMAGE_DECLARE(img_src_ui_info);
LV_IMAGE_DECLARE(img_src_ui_work);
LV_IMAGE_DECLARE(img_src_ui_record);
LV_IMAGE_DECLARE(img_src_ui_record_large);
LV_IMAGE_DECLARE(img_src_ui_settings);
LV_IMAGE_DECLARE(img_src_ui_settings_large);
LV_IMAGE_DECLARE(img_src_ui_base_large);
LV_IMAGE_DECLARE(img_src_ui_rover_large);
LV_IMAGE_DECLARE(img_src_ui_network);
LV_IMAGE_DECLARE(img_src_ui_network_large);
LV_IMAGE_DECLARE(img_src_ui_protocol);
LV_IMAGE_DECLARE(img_src_ui_channel);
LV_IMAGE_DECLARE(img_src_ui_clock);
LV_IMAGE_DECLARE(img_src_ui_apply);
LV_IMAGE_DECLARE(img_src_ui_back);
LV_IMAGE_DECLARE(img_src_ui_play);
LV_IMAGE_DECLARE(img_src_ui_stop);
LV_IMAGE_DECLARE(img_src_ui_power);
LV_IMAGE_DECLARE(img_src_ui_power_large);
LV_IMAGE_DECLARE(img_src_ui_wifi);
LV_IMAGE_DECLARE(img_src_ui_location);
LV_IMAGE_DECLARE(img_src_ui_sd_card);
LV_IMAGE_DECLARE(img_src_ui_satellite);
LV_IMAGE_DECLARE(img_src_ui_satellite_large);
LV_IMAGE_DECLARE(img_src_ui_antenna);
LV_IMAGE_DECLARE(img_src_ui_rover);
LV_IMAGE_DECLARE(img_src_ui_base);
LV_IMAGE_DECLARE(img_src_start);
LV_IMAGE_DECLARE(img_src_stop);
LV_IMAGE_DECLARE(img_src_map_location);
LV_IMAGE_DECLARE(img_src_storage);
LV_IMAGE_DECLARE(img_src_system_info);
LV_IMAGE_DECLARE(img_src_wifi);
LV_IMAGE_DECLARE(img_src_workmode);
LV_IMAGE_DECLARE(img_src_up);
LV_IMAGE_DECLARE(img_src_down);
LV_IMAGE_DECLARE(img_src_reset);
LV_IMAGE_DECLARE(img_src_back);
LV_IMAGE_DECLARE(img_src_shutdown);
LV_IMAGE_DECLARE(img_src_system_dash_power);
LV_IMAGE_DECLARE(img_src_system_dash_power_small);
LV_IMAGE_DECLARE(img_src_system_dash_settings);
LV_IMAGE_DECLARE(img_src_system_dash_return);
LV_IMAGE_DECLARE(img_src_ntrip);
LV_IMAGE_DECLARE(img_src_NationalFlag_EN);
LV_IMAGE_DECLARE(img_src_NationalFlag_RU);
}
namespace {
/** 静态资源名使用字符串内容比较，空名称按未找到处理。 */
bool same_name(const char* a, const char* b) { return a && b && std::strcmp(a, b) == 0; }
using Fonts = ResourceManager<const char*, const lv_font_t*>;
using Images = ResourceManager<const char*, const void*>;
// 使用 X-TRACK 静态表，避免重复初始化和字符串/map 动态分配。
Fonts::KEY_PAIR fonts[] = {
    {"oswaldBold_12", &font_oswaldBold_12},
    {"oswaldBold_18", &font_oswaldBold_18},
    {"rajdhaniBold_20", &font_rajdhaniBold_20},
    {"rajdhaniBold_40", &font_rajdhaniBold_40},
    {"dialplate", &symbol_dialplate},
};
Images::KEY_PAIR images[] = {
    {"battery", &img_src_battery},
    {"battery_info", &img_src_battery_info},
    {"record", &img_src_record},
    {"clock", &img_src_clock},
    {"mode", &img_src_mode},
    {"settings", &img_src_settings},
    {"menu", &img_src_menu},
    {"ui_info", &img_src_ui_info},
    {"ui_work", &img_src_ui_work},
    {"ui_record", &img_src_ui_record},
    {"ui_record_large", &img_src_ui_record_large},
    {"ui_settings", &img_src_ui_settings},
    {"ui_settings_large", &img_src_ui_settings_large},
    {"ui_base_large", &img_src_ui_base_large},
    {"ui_rover_large", &img_src_ui_rover_large},
    {"ui_network", &img_src_ui_network},
    {"ui_network_large", &img_src_ui_network_large},
    {"ui_protocol", &img_src_ui_protocol},
    {"ui_channel", &img_src_ui_channel},
    {"ui_clock", &img_src_ui_clock},
    {"ui_apply", &img_src_ui_apply},
    {"ui_back", &img_src_ui_back},
    {"ui_play", &img_src_ui_play},
    {"ui_stop", &img_src_ui_stop},
    {"ui_power", &img_src_ui_power},
    {"ui_power_large", &img_src_ui_power_large},
    {"ui_wifi", &img_src_ui_wifi},
    {"ui_location", &img_src_ui_location},
    {"ui_sd_card", &img_src_ui_sd_card},
    {"ui_satellite", &img_src_ui_satellite},
    {"ui_satellite_large", &img_src_ui_satellite_large},
    {"ui_antenna", &img_src_ui_antenna},
    {"ui_rover", &img_src_ui_rover},
    {"ui_base", &img_src_ui_base},
    {"start", &img_src_start},
    {"stop", &img_src_stop},
    {"map_location", &img_src_map_location},
    {"storage", &img_src_storage},
    {"system_info", &img_src_system_info},
    {"wifi", &img_src_wifi},
    {"workmode", &img_src_workmode},
    {"up", &img_src_up},
    {"down", &img_src_down},
    {"reset", &img_src_reset},
    {"back", &img_src_back},
    {"shutdown", &img_src_shutdown},
    {"system_dash_power", &img_src_system_dash_power},
    {"system_dash_power_small", &img_src_system_dash_power_small},
    {"system_dash_settings", &img_src_system_dash_settings},
    {"system_dash_return", &img_src_system_dash_return},
    {"ntrip", &img_src_ntrip},
    {"NationalFlag_EN", &img_src_NationalFlag_EN},
    {"NationalFlag_RU", &img_src_NationalFlag_RU},
    {"oemLogoParts", &oem_logo_parts},
};
Fonts font_(fonts, sizeof(fonts) / sizeof(fonts[0]), same_name, LV_FONT_DEFAULT);
Images image_(images, sizeof(images) / sizeof(images[0]), same_name, nullptr);
}

/** 静态资源表无需运行期注册，保留原初始化入口。 */
void resource_pool::init() {}
/** 查找字体；未知名称返回 LV_FONT_DEFAULT，保持原接口。 */
lv_font_t* resource_pool::get_font(const char* name) {
    return const_cast<lv_font_t*>(*font_.get(name));
}
/** 查找图片；未知名称返回空指针。 */
const void* resource_pool::get_image(const char* name) { return *image_.get(name); }
