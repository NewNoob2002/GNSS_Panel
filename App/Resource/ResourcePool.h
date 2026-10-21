#pragma once
#include "lvgl.h"
#include "symbol/symbol_unicode.h"

namespace resource_pool {

/** 在 lv_init() 后调用一次，注册静态编译的字体和图片资源。 */
void init();

/** 返回资源池持有的字体；名称缺失时返回 LV_FONT_DEFAULT。 */
lv_font_t* get_font(const char* name);

/** 返回资源池持有的图片；名称缺失时返回 nullptr。 */
const void* get_image(const char* name);

} // namespace resource_pool
