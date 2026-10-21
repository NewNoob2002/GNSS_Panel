/*******************************************************************************
 * Size: 20 px
 * Bpp: 2
 * Opts: --bpp 2 --size 20 --no-compress --stride 1 --align 1 --font Rajdhani-Bold.ttf --symbols 0123456789 --format lvgl -o Rajdhani_Bold_0_9_20.c
 ******************************************************************************/

#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif



#ifndef RAJDHANI_BOLD_0_9_20
#define RAJDHANI_BOLD_0_9_20 1
#endif

#if RAJDHANI_BOLD_0_9_20

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0030 "0" */
    0x1f, 0xfd, 0x1f, 0xff, 0xcf, 0xc1, 0xfb, 0xe0,
    0x3e, 0xf8, 0xf, 0xbe, 0x3, 0xef, 0x80, 0xfb,
    0xe0, 0x3e, 0xf8, 0xf, 0xbe, 0x3, 0xef, 0xc1,
    0xf9, 0xff, 0xfc, 0x2f, 0xfd, 0x0,

    /* U+0031 "1" */
    0xb, 0xd7, 0xfd, 0xbf, 0xd5, 0x3d, 0x3, 0xd0,
    0x3d, 0x3, 0xd0, 0x3d, 0x3, 0xd0, 0x3d, 0x3,
    0xd0, 0x3d, 0x3, 0xd0,

    /* U+0032 "2" */
    0xb, 0xfe, 0xb, 0xff, 0xe3, 0xe0, 0xfc, 0xf4,
    0x2f, 0x14, 0xb, 0xc0, 0x7, 0xe0, 0xb, 0xf0,
    0xb, 0xf0, 0xf, 0xe0, 0xb, 0xe0, 0x3, 0xf0,
    0x0, 0xff, 0xff, 0x3f, 0xff, 0xc0,

    /* U+0033 "3" */
    0xb, 0xfe, 0x2, 0xff, 0xf8, 0x3e, 0xb, 0xc3,
    0xd0, 0x7c, 0x28, 0x7, 0xc0, 0x0, 0xbc, 0x0,
    0xff, 0x40, 0xf, 0xf4, 0x0, 0xb, 0xc3, 0xc0,
    0x7c, 0x3e, 0xb, 0xc3, 0xff, 0xf8, 0xb, 0xfe,
    0x0,

    /* U+0034 "4" */
    0x0, 0x3f, 0x80, 0x2, 0xfe, 0x0, 0xf, 0xf8,
    0x0, 0xbe, 0xe0, 0x3, 0xdb, 0x80, 0x2f, 0x2e,
    0x0, 0xf4, 0xb8, 0xb, 0xc2, 0xe0, 0x3d, 0xb,
    0xc1, 0xff, 0xff, 0xe3, 0xff, 0xff, 0x80, 0x2,
    0xe0, 0x0, 0xb, 0x80,

    /* U+0035 "5" */
    0x3f, 0xff, 0xc3, 0xff, 0xfc, 0x3e, 0x55, 0x3,
    0xe0, 0x0, 0x3e, 0x0, 0x3, 0xff, 0xf0, 0x3f,
    0xff, 0xc2, 0xa0, 0x7c, 0x0, 0x3, 0xc3, 0xd0,
    0x3c, 0x3e, 0x7, 0xc2, 0xff, 0xfc, 0xb, 0xfe,
    0x0,

    /* U+0036 "6" */
    0x2f, 0xff, 0x1f, 0xff, 0xcf, 0xc0, 0x3, 0xe0,
    0x0, 0xf8, 0x0, 0x3f, 0xff, 0x4f, 0xff, 0xf3,
    0xe0, 0x7d, 0xf8, 0xf, 0xbe, 0x3, 0xef, 0xc1,
    0xf6, 0xff, 0xfc, 0x2f, 0xfd, 0x0,

    /* U+0037 "7" */
    0xbf, 0xff, 0x2f, 0xff, 0xc1, 0x56, 0xf0, 0x0,
    0xfc, 0x0, 0x7d, 0x0, 0x3f, 0x0, 0xf, 0x80,
    0xb, 0xc0, 0x3, 0xf0, 0x1, 0xf4, 0x0, 0xfc,
    0x0, 0x7e, 0x0, 0x2f, 0x0, 0x0,

    /* U+0038 "8" */
    0x2f, 0xfd, 0x1f, 0xff, 0xdf, 0xc0, 0xfb, 0xe0,
    0x2f, 0xf8, 0xb, 0xbf, 0x3, 0xe7, 0xff, 0xf1,
    0xff, 0xfc, 0xf8, 0xf, 0xbe, 0x2, 0xff, 0xc0,
    0xfa, 0xff, 0xfd, 0x2f, 0xfd, 0x0,

    /* U+0039 "9" */
    0xb, 0xfe, 0x2, 0xff, 0xfc, 0x3e, 0x7, 0xd3,
    0xd0, 0x3d, 0x3d, 0x3, 0xd3, 0xe0, 0x3d, 0x3f,
    0xff, 0xd0, 0xff, 0xfd, 0x0, 0x7, 0xd0, 0x0,
    0x3d, 0x0, 0x7, 0xd2, 0xff, 0xfc, 0x2f, 0xfe,
    0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 172, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 107, .box_w = 6, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 50, .adv_w = 158, .box_w = 9, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 80, .adv_w = 163, .box_w = 10, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 113, .adv_w = 173, .box_w = 11, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 149, .adv_w = 162, .box_w = 10, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 182, .adv_w = 168, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 212, .adv_w = 138, .box_w = 9, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 242, .adv_w = 173, .box_w = 9, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 272, .adv_w = 168, .box_w = 10, .box_h = 13, .ofs_x = 0, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 48, .range_length = 10, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 2,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif

};

extern const lv_font_t lv_font_montserrat_18;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t font_rajdhaniBold_20 = {
#else
lv_font_t font_rajdhaniBold_20 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 13,          /*The maximum line height required by the font*/
    .base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if font_rajdhaniBold_20*/
