#ifndef APP_RESOURCE_LOGO_PARTS_H
#define APP_RESOURCE_LOGO_PARTS_H

#include <stdbool.h>
#include "LogoLayout.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 静态图像零件；x/y 为原 Logo 内偏移，group 为入场组号，图像寿命覆盖应用。 */
typedef struct {
    const void* image;
    int16_t x;
    int16_t y;
    uint16_t group;
} logo_part_t;

/** @brief 组时间覆盖；delay_ms=-1、duration_ms=0 时使用播放参数，延迟相对背景淡入结束。 */
typedef struct {
    int32_t delay_ms;
    uint32_t duration_ms;
} logo_group_t;

/** @brief 不可变资源表；非 A8 单零件可关闭着色，高光由各 Logo 配置。 */
typedef struct {
    const logo_part_t* parts;
    const logo_group_t* groups;
    uint16_t part_count;
    uint16_t group_count;
    system_loading_logo_layout_t layout;
    uint16_t width;
    uint16_t height;
    bool recolor;
    bool highlight;
} logo_parts_t;

extern const logo_parts_t img_src_startupLogo_parts;
extern const logo_parts_t img_src_MiddleLogo_parts;
extern const logo_parts_t img_src_RGKLogo_parts;

#ifdef __cplusplus
}
#endif
#endif
