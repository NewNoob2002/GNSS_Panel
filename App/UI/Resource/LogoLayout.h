#ifndef APP_RESOURCE_LOGO_LAYOUT_H
#define APP_RESOURCE_LOGO_LAYOUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 启动 Logo 布局资源；颜色为 0xRRGGBB，偏移以原始 294×126 画面的像素计。 */
typedef struct {
    uint32_t background;
    uint32_t foreground;
    int16_t x;
    int16_t y;
} system_loading_logo_layout_t;

/** @brief 各 Logo 的静态布局，与同名 lv_image_dsc_t 资源配对。 */
extern const system_loading_logo_layout_t img_src_startupLogo_layout;
extern const system_loading_logo_layout_t img_src_MiddleLogo_layout;
extern const system_loading_logo_layout_t img_src_RGKLogo_layout;

#ifdef __cplusplus
}
#endif

#endif
