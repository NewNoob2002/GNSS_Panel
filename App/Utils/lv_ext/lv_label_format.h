/** @file lv_label_format.h 保留整数坐标配置下的应用浮点数值显示。 */
#ifndef APP_LV_LABEL_FORMAT_H
#define APP_LV_LABEL_FORMAT_H

#include <cstdarg>
#include <cstdio>
#include "lvgl.h"

/**
 * @brief 用标准库格式化应用数值，再由 LVGL 复制到标签。
 * @details LV_USE_FLOAT=0 时 v9.6 内置格式化器不支持浮点；256 字节覆盖现有多行数值。
 *          格式化失败时保留原文字，超长结果按缓冲边界截断。
 */
inline void app_label_set_text_fmt(lv_obj_t* label, const char* format, ...) {
    char text[256];
    va_list args;
    va_start(args, format);
    const int length = std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    if (length >= 0)
        lv_label_set_text(label, text);
}

#endif
