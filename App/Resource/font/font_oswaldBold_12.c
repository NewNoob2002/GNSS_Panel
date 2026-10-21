/*******************************************************************************
 * Size: 12 px
 * Bpp: 2
 * Opts: --bpp 2 --size 12 --no-compress --stride 1 --align 1 --font Oswald-Bold.ttf --range 32-126,160-255,338,339,376,1024-1104,8226,8230,8482 --format lvgl -o font_oswaldBold_12.c
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



#ifndef FONT_OSWALDBOLD_12
#define FONT_OSWALDBOLD_12 1
#endif

#if FONT_OSWALDBOLD_12

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */

    /* U+0021 "!" */
    0x75, 0xd3, 0x4c, 0x30, 0xc3, 0x0, 0x75, 0xd0,

    /* U+0022 "\"" */
    0xb7, 0x29, 0xc9, 0x61, 0x14,

    /* U+0023 "#" */
    0xd, 0xd0, 0xcd, 0xc, 0xd7, 0xff, 0x2d, 0xc2,
    0x9c, 0x7f, 0xf3, 0xa8, 0x37, 0x43, 0x34,

    /* U+0024 "$" */
    0x5, 0x2, 0xf8, 0xbf, 0xcb, 0x5d, 0xbc, 0x3,
    0xf0, 0xf, 0xc0, 0x3d, 0xb2, 0xe7, 0xfd, 0x2f,
    0x80, 0x10, 0x0, 0x0,

    /* U+0025 "%" */
    0x2f, 0x43, 0x0, 0x77, 0xc6, 0x0, 0x72, 0xcd,
    0x0, 0x72, 0xcc, 0x0, 0x76, 0xd8, 0x0, 0x3f,
    0xa5, 0xfc, 0x0, 0x33, 0x9d, 0x0, 0x73, 0x8e,
    0x0, 0xa3, 0xdd, 0x0, 0xc1, 0xf8,

    /* U+0026 "&" */
    0x1f, 0x40, 0xeb, 0x3, 0x5c, 0xf, 0xe0, 0x1f,
    0x0, 0xb8, 0xc7, 0xb7, 0x2c, 0xbc, 0x76, 0xf0,
    0xbd, 0xd0,

    /* U+0027 "'" */
    0xba, 0x94,

    /* U+0028 "(" */
    0xe, 0x2d, 0x3c, 0x38, 0x38, 0x38, 0x38, 0x38,
    0x38, 0x3c, 0x2d, 0xe,

    /* U+0029 ")" */
    0xb0, 0x78, 0x2c, 0x2c, 0x2d, 0x2d, 0x2d, 0x2d,
    0x2c, 0x2c, 0x78, 0xb0,

    /* U+002A "*" */
    0xd, 0x1f, 0xe2, 0xf0, 0xec, 0x0, 0x0,

    /* U+002B "+" */
    0xd, 0x0, 0xd0, 0xbf, 0xc0, 0xd0, 0xd, 0x0,

    /* U+002C "," */
    0x75, 0xd2, 0x4,

    /* U+002D "-" */
    0x0, 0xbe, 0x0,

    /* U+002E "." */
    0x75, 0xd0,

    /* U+002F "/" */
    0x2, 0x40, 0xc0, 0x70, 0x28, 0xc, 0x3, 0x1,
    0x80, 0x90, 0x30, 0x18, 0x0,

    /* U+0030 "0" */
    0x1f, 0xc3, 0xfe, 0x78, 0xf7, 0x8f, 0x78, 0xf7,
    0x8f, 0x78, 0xf7, 0x8f, 0x3f, 0xe1, 0xfc,

    /* U+0031 "1" */
    0x2e, 0xbe, 0x6e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
    0x1e, 0x1e,

    /* U+0032 "2" */
    0x1f, 0x83, 0xfe, 0x74, 0xf6, 0x1e, 0x3, 0xd0,
    0xb8, 0xf, 0x3, 0xd0, 0x7e, 0x9b, 0xfe,

    /* U+0033 "3" */
    0x2f, 0x87, 0xfe, 0xb4, 0xf0, 0x1e, 0xb, 0xc0,
    0x2e, 0x60, 0xf7, 0x8e, 0x7f, 0xe2, 0xf8,

    /* U+0034 "4" */
    0x7, 0xc0, 0x3f, 0x1, 0xfc, 0xa, 0xb0, 0x36,
    0xc2, 0xcf, 0x4b, 0xff, 0x5, 0xf4, 0x2, 0xc0,
    0xb, 0x0,

    /* U+0035 "5" */
    0x7f, 0xd7, 0xe8, 0x74, 0x7, 0xa8, 0x7f, 0xd6,
    0x5e, 0x0, 0xe7, 0x4e, 0x7e, 0xd2, 0xf8,

    /* U+0036 "6" */
    0x1f, 0xc3, 0xee, 0x78, 0x97, 0x80, 0x7f, 0xd7,
    0xef, 0x78, 0xf7, 0x8f, 0x3e, 0xe1, 0xfc,

    /* U+0037 "7" */
    0xff, 0xda, 0xf0, 0x78, 0x2d, 0xf, 0x3, 0xc1,
    0xe0, 0xb4, 0x3c, 0xf, 0x0,

    /* U+0038 "8" */
    0x2f, 0x83, 0xed, 0x79, 0xe3, 0x9d, 0x2f, 0xc3,
    0xfd, 0x74, 0xeb, 0x4f, 0x7e, 0xe2, 0xf8,

    /* U+0039 "9" */
    0x1f, 0x83, 0xee, 0x79, 0xfb, 0x8f, 0x79, 0xf3,
    0xff, 0x5, 0xf2, 0x5f, 0x3e, 0xe2, 0xf8,

    /* U+003A ":" */
    0x38, 0xe0, 0x0, 0x38, 0xe0,

    /* U+003B ";" */
    0x38, 0xe0, 0x0, 0x0, 0xe3, 0x86, 0x10,

    /* U+003C "<" */
    0x0, 0x2, 0xd7, 0xd1, 0xf4, 0x1f, 0x40, 0x40,

    /* U+003D "=" */
    0x7f, 0x80, 0x3, 0xf8, 0x0,

    /* U+003E ">" */
    0x0, 0x1e, 0x1, 0xf4, 0x7d, 0x7d, 0x4, 0x0,

    /* U+003F "?" */
    0x2f, 0x4b, 0xfc, 0xb6, 0xd0, 0x2c, 0x3, 0xc0,
    0xf0, 0x1d, 0x0, 0x40, 0x1d, 0x2, 0xd0,

    /* U+0040 "@" */
    0x1, 0xbf, 0x80, 0xb, 0x80, 0xb0, 0x1d, 0x7b,
    0x6c, 0x38, 0xfb, 0x5c, 0x36, 0xc7, 0x1c, 0x73,
    0xcb, 0x2c, 0x72, 0xdf, 0x38, 0x70, 0xfb, 0xe0,
    0x38, 0x0, 0x0, 0x1e, 0x1, 0x0, 0x7, 0xff,
    0x40, 0x0, 0x0, 0x0,

    /* U+0041 "A" */
    0xf, 0x80, 0x3f, 0x1, 0xfc, 0xb, 0xf0, 0x2e,
    0xd0, 0xf7, 0x83, 0xde, 0x1f, 0xfc, 0x78, 0xf2,
    0xd2, 0xd0,

    /* U+0042 "B" */
    0x7f, 0xd1, 0xf7, 0xc7, 0xcb, 0x5f, 0x3c, 0x7f,
    0xe1, 0xf7, 0xd7, 0xc7, 0x9f, 0x1e, 0x7e, 0xf5,
    0xff, 0x80,

    /* U+0043 "C" */
    0x1f, 0xd0, 0xfb, 0xc7, 0xcf, 0x5f, 0x28, 0x7c,
    0x1, 0xf0, 0x7, 0xca, 0x1f, 0x3d, 0x3e, 0xf0,
    0x7f, 0x40,

    /* U+0044 "D" */
    0x7f, 0xd1, 0xfb, 0xc7, 0xcf, 0x5f, 0x3d, 0x7c,
    0xf5, 0xf3, 0xd7, 0xcf, 0x5f, 0x3d, 0x7e, 0xf1,
    0xff, 0x40,

    /* U+0045 "E" */
    0x7f, 0xc7, 0xd4, 0x7c, 0x7, 0xc0, 0x7f, 0x87,
    0xd0, 0x7c, 0x7, 0xc0, 0x7d, 0x47, 0xfc,

    /* U+0046 "F" */
    0x7f, 0xdf, 0x57, 0xc1, 0xf0, 0x7f, 0x9f, 0x47,
    0xc1, 0xf0, 0x7c, 0x1f, 0x0,

    /* U+0047 "G" */
    0x1f, 0xd0, 0xfb, 0xc7, 0xcf, 0x5f, 0x18, 0x7c,
    0x1, 0xf7, 0xd7, 0xcb, 0x5f, 0x2d, 0x3e, 0xf4,
    0x7e, 0xd0,

    /* U+0048 "H" */
    0x7c, 0xb9, 0xf2, 0xe7, 0xcb, 0x9f, 0x2e, 0x7e,
    0xf9, 0xff, 0xe7, 0xcb, 0x9f, 0x2e, 0x7c, 0xb9,
    0xf2, 0xe0,

    /* U+0049 "I" */
    0x7d, 0xf7, 0xdf, 0x7d, 0xf7, 0xdf, 0x7d, 0xf0,

    /* U+004A "J" */
    0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d,
    0x7d, 0xf8,

    /* U+004B "K" */
    0x7c, 0xb5, 0xf3, 0xc7, 0xde, 0x1f, 0xf0, 0x7f,
    0xc1, 0xff, 0x7, 0xed, 0x1f, 0x7c, 0x7c, 0xf1,
    0xf2, 0xe0,

    /* U+004C "L" */
    0x7c, 0x7, 0xc0, 0x7c, 0x7, 0xc0, 0x7c, 0x7,
    0xc0, 0x7c, 0x7, 0xc0, 0x7d, 0x47, 0xfc,

    /* U+004D "M" */
    0x3c, 0x2e, 0x3d, 0x3e, 0x3e, 0x3e, 0x3f, 0x7e,
    0x3f, 0xbe, 0x3b, 0xee, 0x77, 0xee, 0x77, 0xde,
    0x76, 0xce, 0x72, 0xcf,

    /* U+004E "N" */
    0x74, 0xf7, 0xcf, 0x7d, 0xf7, 0xef, 0x7f, 0xf7,
    0xbf, 0x7b, 0xf7, 0x9f, 0x78, 0xf7, 0x8b,

    /* U+004F "O" */
    0x1f, 0xd0, 0xfb, 0xc7, 0xcf, 0x5f, 0x3d, 0x7c,
    0xf5, 0xf3, 0xd7, 0xcf, 0x5f, 0x3d, 0x3e, 0xf0,
    0x7f, 0x40,

    /* U+0050 "P" */
    0x7f, 0xe1, 0xfb, 0xd7, 0xcb, 0x5f, 0x1e, 0x7c,
    0xb5, 0xff, 0xc7, 0xd4, 0x1f, 0x0, 0x7c, 0x1,
    0xf0, 0x0,

    /* U+0051 "Q" */
    0x1f, 0xd0, 0xfb, 0xc7, 0xcf, 0x5f, 0x3d, 0x7c,
    0xf5, 0xf3, 0xd7, 0xcf, 0x5f, 0x3d, 0x3e, 0xf0,
    0x7f, 0x40, 0xf, 0x0, 0x4,

    /* U+0052 "R" */
    0x7f, 0xe1, 0xf7, 0xd7, 0xc7, 0x9f, 0x2e, 0x7f,
    0xf1, 0xfb, 0x87, 0xcf, 0x1f, 0x3c, 0x7c, 0xf5,
    0xf2, 0xe0,

    /* U+0053 "S" */
    0x1f, 0x87, 0xfd, 0x78, 0xd7, 0xc0, 0x2f, 0x0,
    0xbc, 0x2, 0xe7, 0x5f, 0x3f, 0xe1, 0xf8,

    /* U+0054 "T" */
    0xff, 0xcb, 0xf8, 0x1f, 0x1, 0xf0, 0x1f, 0x1,
    0xf0, 0x1f, 0x1, 0xf0, 0x1f, 0x1, 0xf0,

    /* U+0055 "U" */
    0x7c, 0xb5, 0xf2, 0xd7, 0xcb, 0x5f, 0x2d, 0x7c,
    0xb5, 0xf2, 0xd7, 0xcb, 0x5f, 0x3d, 0x3e, 0xf0,
    0x7f, 0x40,

    /* U+0056 "V" */
    0xb0, 0xf1, 0xd3, 0xc7, 0x4e, 0xe, 0x74, 0x3e,
    0xd0, 0xfb, 0x2, 0xfc, 0x7, 0xe0, 0xf, 0x80,
    0x3d, 0x0,

    /* U+0057 "W" */
    0xb2, 0xcf, 0x2d, 0xb3, 0xc7, 0x7c, 0xf1, 0xdf,
    0x78, 0x3b, 0xee, 0xf, 0xef, 0x43, 0xf7, 0xd0,
    0xf9, 0xf0, 0x2e, 0x3c, 0xb, 0x4f, 0x0,

    /* U+0058 "X" */
    0xb4, 0xf0, 0xf7, 0x43, 0xfc, 0x7, 0xf0, 0xf,
    0x40, 0x7d, 0x2, 0xf8, 0xf, 0xf0, 0x76, 0xe2,
    0xc3, 0xc0,

    /* U+0059 "Y" */
    0xf5, 0xe7, 0xad, 0x3f, 0xc3, 0xf8, 0x2f, 0x40,
    0xf0, 0xf, 0x0, 0xf0, 0xf, 0x0, 0xf0,

    /* U+005A "Z" */
    0xbf, 0xc6, 0xe0, 0xb4, 0x3c, 0x1f, 0xb, 0x43,
    0xc1, 0xf0, 0xb9, 0x6f, 0xf0,

    /* U+005B "[" */
    0x7d, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70,
    0x70, 0x70, 0x70, 0x7d,

    /* U+005C "\\" */
    0x60, 0xc, 0x2, 0x40, 0x60, 0xc, 0x3, 0x0,
    0xa0, 0xc, 0x3, 0x0, 0x90,

    /* U+005D "]" */
    0xbd, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d,
    0x1d, 0x1d, 0x1d, 0xbd,

    /* U+005E "^" */
    0xf, 0x2, 0xf4, 0x3b, 0x87, 0x1c,

    /* U+005F "_" */
    0x0, 0x3f, 0xd0, 0x0,

    /* U+0060 "`" */
    0x10, 0x3c, 0x1c,

    /* U+0061 "a" */
    0x2f, 0x47, 0xbc, 0x53, 0xc2, 0xfc, 0xb3, 0xcb,
    0x7c, 0x7f, 0xc0,

    /* U+0062 "b" */
    0x78, 0x7, 0x80, 0x78, 0x7, 0xfc, 0x7a, 0xe7,
    0x9e, 0x79, 0xe7, 0x9e, 0x7a, 0xd7, 0xfc,

    /* U+0063 "c" */
    0x2f, 0x47, 0xbc, 0xb6, 0xcb, 0x40, 0xb6, 0xc7,
    0xbc, 0x2f, 0x40,

    /* U+0064 "d" */
    0x2, 0xd0, 0x2d, 0x2, 0xd3, 0xfd, 0x7a, 0xdb,
    0x6d, 0xb6, 0xdb, 0x6d, 0x7a, 0xd3, 0xfd,

    /* U+0065 "e" */
    0x2f, 0x47, 0xbc, 0xb6, 0xcb, 0xfc, 0xb5, 0x87,
    0xbc, 0x2f, 0x80,

    /* U+0066 "f" */
    0x2e, 0x3d, 0x3c, 0xfe, 0x7c, 0x3c, 0x3c, 0x3c,
    0x3c, 0x3c,

    /* U+0067 "g" */
    0x0, 0x0, 0xbe, 0xc7, 0xad, 0x1e, 0xb4, 0x3f,
    0xc1, 0xd0, 0x7, 0xfd, 0xd, 0x7c, 0xb9, 0xf0,
    0xff, 0x0,

    /* U+0068 "h" */
    0x78, 0x7, 0x80, 0x78, 0x7, 0xfc, 0x7e, 0xe7,
    0x9e, 0x79, 0xe7, 0x9e, 0x79, 0xe7, 0x9e,

    /* U+0069 "i" */
    0x78, 0x40, 0x1e, 0x79, 0xe7, 0x9e, 0x79, 0xe0,

    /* U+006A "j" */
    0x1e, 0x4, 0x0, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
    0x1e, 0x1e, 0x2e, 0x3c,

    /* U+006B "k" */
    0x78, 0x1, 0xe0, 0x7, 0x80, 0x1e, 0x7c, 0x7b,
    0xc1, 0xfe, 0x7, 0xf8, 0x1e, 0xf0, 0x7a, 0xd1,
    0xe3, 0xc0,

    /* U+006C "l" */
    0x79, 0xe7, 0x9e, 0x79, 0xe7, 0x9e, 0x79, 0xe0,

    /* U+006D "m" */
    0x7f, 0xef, 0x1e, 0xba, 0xd7, 0x9e, 0x75, 0xe7,
    0x9d, 0x79, 0xe7, 0x5e, 0x79, 0xd7, 0x9e, 0x74,

    /* U+006E "n" */
    0x7f, 0xc7, 0xae, 0x79, 0xe7, 0x9e, 0x79, 0xe7,
    0x9e, 0x79, 0xe0,

    /* U+006F "o" */
    0x2f, 0x87, 0xbc, 0xb6, 0xdb, 0x6d, 0xb6, 0xd7,
    0xbc, 0x2f, 0x80,

    /* U+0070 "p" */
    0x7f, 0xc7, 0xae, 0x79, 0xe7, 0x9e, 0x79, 0xe7,
    0xad, 0x7f, 0xc7, 0x80, 0x78, 0x0,

    /* U+0071 "q" */
    0x3f, 0xd7, 0xad, 0xb6, 0xdb, 0x6d, 0xb6, 0xd7,
    0xad, 0x3f, 0xd0, 0x2d, 0x2, 0xd0,

    /* U+0072 "r" */
    0x7b, 0x5f, 0xd7, 0x81, 0xe0, 0x78, 0x1e, 0x7,
    0x80,

    /* U+0073 "s" */
    0x2f, 0x2e, 0xe7, 0x80, 0xbc, 0xb, 0xad, 0xf2,
    0xf4,

    /* U+0074 "t" */
    0x3c, 0x3c, 0xff, 0x7c, 0x3c, 0x3c, 0x3c, 0x3e,
    0x2f,

    /* U+0075 "u" */
    0x7a, 0xd7, 0xad, 0x7a, 0xd7, 0xad, 0x7a, 0xd7,
    0xad, 0x3e, 0xd0,

    /* U+0076 "v" */
    0xf3, 0xed, 0xe7, 0xb4, 0xfd, 0x3f, 0xf, 0xc2,
    0xe0,

    /* U+0077 "w" */
    0xb3, 0x6e, 0xde, 0xe7, 0xbb, 0x8f, 0xfd, 0x3e,
    0xf0, 0xf3, 0xc3, 0xcf, 0x0,

    /* U+0078 "x" */
    0xb6, 0xc3, 0xf4, 0x2f, 0x2, 0xf0, 0x3f, 0x7,
    0xb8, 0xb3, 0xc0,

    /* U+0079 "y" */
    0xf2, 0xcb, 0x3c, 0x77, 0x83, 0xf4, 0x2f, 0x1,
    0xf0, 0xf, 0x1, 0xe0, 0x38, 0x0,

    /* U+007A "z" */
    0x7f, 0x47, 0xc0, 0xf0, 0xb4, 0x3c, 0x1f, 0x4b,
    0xf4,

    /* U+007B "{" */
    0xf, 0xb, 0x82, 0xc0, 0xb0, 0x3c, 0x1e, 0x3,
    0xc0, 0xb0, 0x2c, 0xb, 0x2, 0xe0, 0x3c,

    /* U+007C "|" */
    0x34, 0xd3, 0x4d, 0x34, 0xd3, 0x4d, 0x34, 0xd3,
    0x4d,

    /* U+007D "}" */
    0xb8, 0x3c, 0x2c, 0x2c, 0x1d, 0xf, 0x1e, 0x2c,
    0x2c, 0x2c, 0x3c, 0xb8,

    /* U+007E "~" */
    0x0, 0x3, 0xd8, 0x6b, 0xc0, 0x0,

    /* U+00A0 " " */

    /* U+00A1 "¡" */
    0x75, 0xd0, 0xc, 0x30, 0xc3, 0x1c, 0x76, 0xd0,

    /* U+00A2 "¢" */
    0x4, 0x0, 0x40, 0x2f, 0x87, 0x7c, 0xb6, 0xcb,
    0x40, 0xb6, 0xc7, 0x7c, 0x2f, 0x80, 0x40, 0x4,
    0x0,

    /* U+00A3 "£" */
    0x1f, 0x83, 0xfd, 0x3c, 0xd3, 0xc0, 0x7e, 0xf,
    0xf4, 0x3c, 0x3, 0xc0, 0x3e, 0x9f, 0xfd,

    /* U+00A4 "¤" */
    0x0, 0x0, 0x69, 0x42, 0x5, 0x4, 0x8, 0x10,
    0x20, 0x81, 0x41, 0xa5, 0x0, 0x0,

    /* U+00A5 "¥" */
    0xf1, 0xeb, 0x6d, 0x7a, 0xc3, 0xfc, 0x3f, 0x82,
    0xf4, 0x3f, 0xc0, 0xf0, 0x3f, 0xc0, 0xf0,

    /* U+00A6 "¦" */
    0x77, 0x77, 0x70, 0x7, 0x77, 0x77,

    /* U+00A7 "§" */
    0x0, 0x2, 0xf4, 0x7a, 0xc7, 0x80, 0x3f, 0x47,
    0x7c, 0x3a, 0xc1, 0xf4, 0x2, 0xc7, 0xac, 0x2f,
    0x40,

    /* U+00A8 "¨" */
    0x72, 0x6c, 0xe0,

    /* U+00A9 "©" */
    0x1, 0xa9, 0x0, 0x2b, 0xf6, 0x2, 0x3d, 0xf1,
    0x15, 0xf3, 0x88, 0x47, 0xc0, 0x11, 0x1f, 0x28,
    0x45, 0x3d, 0xf2, 0x8, 0xbf, 0x44, 0x8, 0x1,
    0x80, 0x5, 0x54, 0x0,

    /* U+00AA "ª" */
    0x0, 0xb, 0xd7, 0xbc, 0x1f, 0x3b, 0xec, 0xf7,
    0xfc, 0x0,

    /* U+00AB "«" */
    0x0, 0x0, 0xc9, 0x35, 0xcb, 0x34, 0x39, 0xc0,
    0xc9, 0x0, 0x0,

    /* U+00AC "¬" */
    0x0, 0x7, 0xfd, 0x0, 0xd0, 0xd, 0x0, 0x0,

    /* U+00AE "®" */
    0x2, 0xa9, 0x0, 0x20, 0x6, 0x2, 0x2f, 0xc6,
    0x8, 0xb3, 0x4c, 0x52, 0xfc, 0x21, 0x4b, 0xb0,
    0x82, 0x2d, 0xd3, 0x8, 0x62, 0x58, 0x8, 0x1,
    0x80, 0xa, 0xa4, 0x0,

    /* U+00AF "¯" */
    0x7e,

    /* U+00B0 "°" */
    0x1f, 0x83, 0x4d, 0x30, 0xa3, 0x4e, 0x1f, 0xc0,
    0x0,

    /* U+00B1 "±" */
    0xd, 0x0, 0xd0, 0xbf, 0xc0, 0xd0, 0xd, 0xb,
    0xfc, 0x0, 0x0,

    /* U+00B2 "²" */
    0x1f, 0xe, 0xe0, 0x38, 0x2c, 0x2d, 0xf, 0xf0,

    /* U+00B3 "³" */
    0x2f, 0xd, 0xe0, 0x38, 0x2d, 0x37, 0x8b, 0xd0,

    /* U+00B4 "´" */
    0x14, 0x3c, 0x30,

    /* U+00B5 "µ" */
    0xb6, 0xdb, 0x6d, 0xb6, 0xdb, 0x6d, 0xb6, 0xdb,
    0xbd, 0xbf, 0xdb, 0x40, 0xb4, 0x0,

    /* U+00B6 "¶" */
    0x3f, 0xdb, 0xd9, 0x7d, 0x92, 0xd9, 0xd, 0x90,
    0xd9, 0xd, 0x90, 0xd9, 0xd, 0x90, 0xd9, 0x4,
    0x50,

    /* U+00B7 "·" */
    0x72, 0xd0,

    /* U+00B8 "¸" */
    0x0, 0x30, 0x1d, 0x3c,

    /* U+00B9 "¹" */
    0x2d, 0x3d, 0xd, 0xd, 0xd, 0xd,

    /* U+00BA "º" */
    0x2f, 0x1d, 0xe7, 0x39, 0xce, 0x73, 0x8f, 0xd0,
    0x0,

    /* U+00BB "»" */
    0x0, 0x6, 0x30, 0x36, 0xc1, 0xcd, 0x36, 0xc5,
    0x30, 0x0, 0x0,

    /* U+00BC "¼" */
    0x4, 0x1, 0x7, 0xc0, 0xd0, 0x2c, 0x1c, 0x1,
    0xc2, 0x40, 0x1c, 0x30, 0x41, 0xc9, 0x3c, 0x0,
    0xcb, 0xc0, 0x24, 0xdc, 0x3, 0x2f, 0xd0, 0x90,
    0x1c,

    /* U+00BD "½" */
    0x0, 0x0, 0x1, 0xf0, 0x24, 0xb, 0xc0, 0xc0,
    0xb, 0x9, 0x0, 0x2c, 0x30, 0x0, 0xb2, 0x4f,
    0x80, 0xc, 0xbb, 0x0, 0x90, 0x2c, 0x3, 0x3,
    0xc0, 0x24, 0x2f, 0xc0, 0x0, 0x0, 0x0,

    /* U+00BE "¾" */
    0x2f, 0x2, 0x80, 0xdd, 0xc, 0x0, 0x34, 0x60,
    0x3, 0xc3, 0x0, 0x23, 0x98, 0x0, 0xfd, 0xd2,
    0xc0, 0x7, 0x1f, 0x0, 0x34, 0xec, 0x1, 0xcb,
    0xf4, 0x9, 0x1, 0xc0,

    /* U+00BF "¿" */
    0xb, 0x0, 0xb4, 0x1, 0x0, 0xb0, 0x1f, 0x3,
    0xc0, 0x74, 0xb, 0x5d, 0x7f, 0xd2, 0xf8,

    /* U+00C0 "À" */
    0x1d, 0x0, 0x38, 0x0, 0x10, 0x3, 0xe0, 0xf,
    0xc0, 0x7f, 0x2, 0xfc, 0xb, 0xb4, 0x3d, 0xe0,
    0xf7, 0x87, 0xff, 0x1e, 0x3c, 0xb4, 0xb4,

    /* U+00C1 "Á" */
    0x2, 0xc0, 0xd, 0x0, 0x10, 0x3, 0xe0, 0xf,
    0xc0, 0x7f, 0x2, 0xfc, 0xb, 0xb4, 0x3d, 0xe0,
    0xf7, 0x87, 0xff, 0x1e, 0x3c, 0xb4, 0xb4,

    /* U+00C2 "Â" */
    0xf, 0x40, 0x7b, 0x0, 0x4, 0x3, 0xe0, 0xf,
    0xc0, 0x7f, 0x2, 0xfc, 0xb, 0xb4, 0x3d, 0xe0,
    0xf7, 0x87, 0xff, 0x1e, 0x3c, 0xb4, 0xb4,

    /* U+00C3 "Ã" */
    0x9, 0x80, 0xab, 0x0, 0x0, 0x3, 0xe0, 0xf,
    0xc0, 0x7f, 0x2, 0xfc, 0xb, 0xb4, 0x3d, 0xe0,
    0xf7, 0x87, 0xff, 0x1e, 0x3c, 0xb4, 0xb4,

    /* U+00C4 "Ä" */
    0x28, 0xd0, 0xf3, 0x40, 0x0, 0x3, 0xe0, 0xf,
    0xc0, 0x7f, 0x2, 0xfc, 0xb, 0xb4, 0x3d, 0xe0,
    0xf7, 0x87, 0xff, 0x1e, 0x3c, 0xb4, 0xb4,

    /* U+00C5 "Å" */
    0xa, 0x40, 0x32, 0x0, 0xa4, 0x3, 0xe0, 0xf,
    0xc0, 0x7f, 0x2, 0xfc, 0xb, 0xb4, 0x3d, 0xe0,
    0xf7, 0x87, 0xff, 0x1e, 0x3c, 0xb4, 0xb4,

    /* U+00C6 "Æ" */
    0x0, 0x3f, 0xd0, 0xb, 0xe4, 0x0, 0xfc, 0x0,
    0x1f, 0xd0, 0x3, 0xef, 0x80, 0x7a, 0xd0, 0xb,
    0x6c, 0x0, 0xff, 0xc0, 0x2e, 0x6e, 0x43, 0xc2,
    0xfd,

    /* U+00C7 "Ç" */
    0x1f, 0xd0, 0xfb, 0xc7, 0xcf, 0x5f, 0x28, 0x7c,
    0x1, 0xf0, 0x7, 0xcb, 0x1f, 0x3d, 0x3e, 0xf0,
    0x7f, 0x40, 0x34, 0x0, 0x34, 0x3, 0xc0,

    /* U+00C8 "È" */
    0x3c, 0x0, 0xd0, 0x4, 0x7, 0xfc, 0x7d, 0x47,
    0xc0, 0x7c, 0x7, 0xf8, 0x7d, 0x7, 0xc0, 0x7c,
    0x7, 0xd4, 0x7f, 0xc0,

    /* U+00C9 "É" */
    0x3, 0x80, 0x70, 0x5, 0x7, 0xfc, 0x7d, 0x47,
    0xc0, 0x7c, 0x7, 0xf8, 0x7d, 0x7, 0xc0, 0x7c,
    0x7, 0xd4, 0x7f, 0xc0,

    /* U+00CA "Ê" */
    0xf, 0x2, 0xb4, 0x10, 0x7, 0xfc, 0x7d, 0x47,
    0xc0, 0x7c, 0x7, 0xf8, 0x7d, 0x7, 0xc0, 0x7c,
    0x7, 0xd4, 0x7f, 0xc0,

    /* U+00CB "Ë" */
    0x35, 0xc3, 0x6c, 0x0, 0x7, 0xfc, 0x7d, 0x47,
    0xc0, 0x7c, 0x7, 0xf8, 0x7d, 0x7, 0xc0, 0x7c,
    0x7, 0xd4, 0x7f, 0xc0,

    /* U+00CC "Ì" */
    0x3c, 0x1c, 0x4, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f,
    0x1f, 0x1f, 0x1f, 0x1f, 0x1f,

    /* U+00CD "Í" */
    0x1e, 0x2c, 0x10, 0x7c, 0x7c, 0x7c, 0x7c, 0x7c,
    0x7c, 0x7c, 0x7c, 0x7c, 0x7c,

    /* U+00CE "Î" */
    0x1f, 0xe, 0xd1, 0x0, 0x7c, 0x1f, 0x7, 0xc1,
    0xf0, 0x7c, 0x1f, 0x7, 0xc1, 0xf0, 0x7c, 0x1f,
    0x0,

    /* U+00CF "Ï" */
    0x32, 0x9d, 0xb0, 0x0, 0x7c, 0x1f, 0x7, 0xc1,
    0xf0, 0x7c, 0x1f, 0x7, 0xc1, 0xf0, 0x7c, 0x1f,
    0x0,

    /* U+00D0 "Ð" */
    0x7f, 0xd1, 0xfb, 0xc7, 0xcf, 0x5f, 0x3d, 0xfc,
    0xf5, 0xf3, 0xd7, 0xcf, 0x5f, 0x3d, 0x7e, 0xf1,
    0xff, 0x40,

    /* U+00D1 "Ñ" */
    0x9, 0x81, 0xac, 0x0, 0x7, 0x4f, 0x7c, 0xf7,
    0xdf, 0x7e, 0xf7, 0xff, 0x7b, 0xf7, 0xbf, 0x79,
    0xf7, 0x8f, 0x78, 0xb0,

    /* U+00D2 "Ò" */
    0x1e, 0x0, 0x2c, 0x0, 0x10, 0x7, 0xf4, 0x3e,
    0xf1, 0xf3, 0xd7, 0xcf, 0x5f, 0x3d, 0x7c, 0xf5,
    0xf3, 0xd7, 0xcf, 0x4f, 0xbc, 0x1f, 0xd0,

    /* U+00D3 "Ó" */
    0x2, 0xd0, 0xe, 0x0, 0x10, 0x7, 0xf4, 0x3e,
    0xf1, 0xf3, 0xd7, 0xcf, 0x5f, 0x3d, 0x7c, 0xf5,
    0xf3, 0xd7, 0xcf, 0x4f, 0xbc, 0x1f, 0xd0,

    /* U+00D4 "Ô" */
    0xb, 0x80, 0x3b, 0x0, 0x44, 0x7, 0xf4, 0x3e,
    0xf1, 0xf3, 0xd7, 0xcf, 0x5f, 0x3d, 0x7c, 0xf5,
    0xf3, 0xd7, 0xcf, 0x4f, 0xbc, 0x1f, 0xd0,

    /* U+00D5 "Õ" */
    0x9, 0x90, 0x7b, 0x0, 0x0, 0x7, 0xf4, 0x3e,
    0xf1, 0xf3, 0xd7, 0xcf, 0x5f, 0x3d, 0x7c, 0xf5,
    0xf3, 0xd7, 0xcf, 0x4f, 0xbc, 0x1f, 0xd0,

    /* U+00D6 "Ö" */
    0x18, 0x90, 0xb3, 0x80, 0x0, 0x7, 0xf4, 0x3e,
    0xf1, 0xf3, 0xd7, 0xcf, 0x5f, 0x3d, 0x7c, 0xf5,
    0xf3, 0xd7, 0xcf, 0x4f, 0xbc, 0x1f, 0xd0,

    /* U+00D7 "×" */
    0x0, 0x3, 0x5c, 0x2f, 0x41, 0xf4, 0x36, 0xc0,
    0x0,

    /* U+00D8 "Ø" */
    0x0, 0x10, 0x7f, 0x83, 0xef, 0x1f, 0x3d, 0x7d,
    0xf5, 0xfb, 0xd7, 0xdf, 0x5f, 0x3d, 0x7c, 0xf4,
    0xfb, 0xc2, 0xfd, 0x4, 0x0,

    /* U+00D9 "Ù" */
    0x1e, 0x0, 0x2c, 0x0, 0x10, 0x1f, 0x2d, 0x7c,
    0xb5, 0xf2, 0xd7, 0xcb, 0x5f, 0x2d, 0x7c, 0xb5,
    0xf2, 0xd7, 0xcf, 0x4f, 0xbc, 0x1f, 0xd0,

    /* U+00DA "Ú" */
    0x2, 0xd0, 0xe, 0x0, 0x10, 0x1f, 0x2d, 0x7c,
    0xb5, 0xf2, 0xd7, 0xcb, 0x5f, 0x2d, 0x7c, 0xb5,
    0xf2, 0xd7, 0xcf, 0x4f, 0xbc, 0x1f, 0xd0,

    /* U+00DB "Û" */
    0xb, 0x80, 0x3b, 0x0, 0x44, 0x1f, 0x2d, 0x7c,
    0xb5, 0xf2, 0xd7, 0xcb, 0x5f, 0x2d, 0x7c, 0xb5,
    0xf2, 0xd7, 0xcf, 0x4f, 0xbc, 0x1f, 0xd0,

    /* U+00DC "Ü" */
    0x1c, 0x90, 0xb3, 0x80, 0x0, 0x1f, 0x2d, 0x7c,
    0xb5, 0xf2, 0xd7, 0xcb, 0x5f, 0x2d, 0x7c, 0xb5,
    0xf2, 0xd7, 0xcf, 0x4f, 0xbc, 0x1f, 0xd0,

    /* U+00DD "Ý" */
    0x3, 0x80, 0x70, 0x1, 0xf, 0x5e, 0x7a, 0xd3,
    0xfc, 0x3f, 0x82, 0xf4, 0xf, 0x0, 0xf0, 0xf,
    0x0, 0xf0, 0xf, 0x0,

    /* U+00DE "Þ" */
    0x7c, 0x1, 0xf9, 0x7, 0xff, 0x1f, 0x2e, 0x7c,
    0x79, 0xfb, 0xd7, 0xfe, 0x1f, 0x0, 0x7c, 0x1,
    0xf0, 0x0,

    /* U+00DF "ß" */
    0x1f, 0x80, 0xf7, 0x87, 0x8e, 0x1e, 0x74, 0x7b,
    0xc1, 0xe7, 0xc7, 0x8f, 0x1e, 0x3c, 0x79, 0xf1,
    0xef, 0x0,

    /* U+00E0 "à" */
    0x3c, 0x1, 0xc0, 0x4, 0x2, 0xf4, 0x7b, 0xc5,
    0x3c, 0x2f, 0xcb, 0x3c, 0xb7, 0xc7, 0xfc,

    /* U+00E1 "á" */
    0x3, 0x80, 0xb0, 0x4, 0x2, 0xf4, 0x7b, 0xc5,
    0x3c, 0x2f, 0xcb, 0x3c, 0xb7, 0xc7, 0xfc,

    /* U+00E2 "â" */
    0xf, 0x2, 0xb4, 0x10, 0x2, 0xf4, 0x7b, 0xc5,
    0x3c, 0x2f, 0xcb, 0x3c, 0xb7, 0xc7, 0xfc,

    /* U+00E3 "ã" */
    0x2e, 0x82, 0x64, 0x0, 0x2, 0xf4, 0x7b, 0xc5,
    0x3c, 0x2f, 0xcb, 0x3c, 0xb7, 0xc7, 0xfc,

    /* U+00E4 "ä" */
    0x36, 0xc3, 0x58, 0x0, 0x2, 0xf4, 0x7b, 0xc5,
    0x3c, 0x2f, 0xcb, 0x3c, 0xb7, 0xc7, 0xfc,

    /* U+00E5 "å" */
    0xa, 0x2, 0x60, 0xa, 0x2, 0xf4, 0x7b, 0xc5,
    0x3c, 0x2f, 0xcb, 0x3c, 0xb7, 0xc7, 0xfc,

    /* U+00E6 "æ" */
    0x2f, 0xbc, 0x7b, 0xee, 0x13, 0xdd, 0x1f, 0xe0,
    0x77, 0xc9, 0xbb, 0xee, 0x3e, 0xbc,

    /* U+00E7 "ç" */
    0x2f, 0x47, 0xbc, 0xb6, 0xcb, 0x40, 0xb6, 0xc7,
    0xbc, 0x2f, 0x40, 0x90, 0x2, 0xc0, 0xb4,

    /* U+00E8 "è" */
    0x3c, 0x1, 0xc0, 0x4, 0x2, 0xf4, 0x7b, 0xcb,
    0x6c, 0xbf, 0xcb, 0x58, 0x7b, 0xc2, 0xf8,

    /* U+00E9 "é" */
    0x3, 0x80, 0xb0, 0x4, 0x2, 0xf4, 0x7b, 0xcb,
    0x6c, 0xbf, 0xcb, 0x58, 0x7b, 0xc2, 0xf8,

    /* U+00EA "ê" */
    0xf, 0x2, 0xb4, 0x10, 0x2, 0xf4, 0x7b, 0xcb,
    0x6c, 0xbf, 0xcb, 0x58, 0x7b, 0xc2, 0xf8,

    /* U+00EB "ë" */
    0x36, 0xc3, 0x58, 0x0, 0x2, 0xf4, 0x7b, 0xcb,
    0x6c, 0xbf, 0xcb, 0x58, 0x7b, 0xc2, 0xf8,

    /* U+00EC "ì" */
    0x3c, 0x1c, 0x4, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
    0x1e, 0x1e,

    /* U+00ED "í" */
    0x1d, 0x2c, 0x10, 0x78, 0x78, 0x78, 0x78, 0x78,
    0x78, 0x78,

    /* U+00EE "î" */
    0x1e, 0xe, 0xc1, 0x0, 0x78, 0x1e, 0x7, 0x81,
    0xe0, 0x78, 0x1e, 0x7, 0x80,

    /* U+00EF "ï" */
    0x73, 0xdc, 0xa0, 0x0, 0x78, 0x1e, 0x7, 0x81,
    0xe0, 0x78, 0x1e, 0x7, 0x80,

    /* U+00F0 "ð" */
    0x1f, 0x50, 0x7c, 0x5, 0xd2, 0xfe, 0x3e, 0xe7,
    0x9e, 0x79, 0xe7, 0x9e, 0x3e, 0xd1, 0xf8,

    /* U+00F1 "ñ" */
    0x2e, 0xc2, 0x64, 0x0, 0x7, 0xfc, 0x7a, 0xe7,
    0x9e, 0x79, 0xe7, 0x9e, 0x79, 0xe7, 0x9e,

    /* U+00F2 "ò" */
    0x3c, 0x1, 0xd0, 0x4, 0x2, 0xf8, 0x7b, 0xcb,
    0x6d, 0xb6, 0xdb, 0x6d, 0x7b, 0xc2, 0xf8,

    /* U+00F3 "ó" */
    0x3, 0x80, 0x70, 0x4, 0x2, 0xf8, 0x7b, 0xcb,
    0x6d, 0xb6, 0xdb, 0x6d, 0x7b, 0xc2, 0xf8,

    /* U+00F4 "ô" */
    0xf, 0x2, 0xb4, 0x10, 0x2, 0xf8, 0x7b, 0xcb,
    0x6d, 0xb6, 0xdb, 0x6d, 0x7b, 0xc2, 0xf8,

    /* U+00F5 "õ" */
    0x2e, 0xc2, 0x64, 0x0, 0x2, 0xf8, 0x7b, 0xcb,
    0x6d, 0xb6, 0xdb, 0x6d, 0x7b, 0xc2, 0xf8,

    /* U+00F6 "ö" */
    0x36, 0xc3, 0x5c, 0x0, 0x2, 0xf8, 0x7b, 0xcb,
    0x6d, 0xb6, 0xdb, 0x6d, 0x7b, 0xc2, 0xf8,

    /* U+00F7 "÷" */
    0x0, 0x0, 0xd0, 0x4, 0xb, 0xfc, 0x4, 0x0,
    0xd0, 0x4, 0x0,

    /* U+00F8 "ø" */
    0x0, 0x42, 0xf8, 0x7b, 0xcb, 0x7d, 0xbe, 0xdb,
    0xad, 0x7b, 0xc3, 0xf8, 0x10, 0x0, 0x0,

    /* U+00F9 "ù" */
    0x3c, 0x0, 0xd0, 0x4, 0x7, 0xad, 0x7a, 0xd7,
    0xad, 0x7a, 0xd7, 0xad, 0x7a, 0xd3, 0xed,

    /* U+00FA "ú" */
    0x3, 0xc0, 0x70, 0x1, 0x7, 0xad, 0x7a, 0xd7,
    0xad, 0x7a, 0xd7, 0xad, 0x7a, 0xd3, 0xed,

    /* U+00FB "û" */
    0xf, 0x2, 0xf8, 0x0, 0x47, 0xad, 0x7a, 0xd7,
    0xad, 0x7a, 0xd7, 0xad, 0x7a, 0xd3, 0xed,

    /* U+00FC "ü" */
    0x39, 0xc3, 0x5c, 0x0, 0x7, 0xad, 0x7a, 0xd7,
    0xad, 0x7a, 0xd7, 0xad, 0x7a, 0xd3, 0xed,

    /* U+00FD "ý" */
    0x7, 0x40, 0xb0, 0x4, 0xf, 0x2c, 0xb3, 0xc7,
    0x78, 0x3f, 0x42, 0xf0, 0x1f, 0x0, 0xf0, 0x1e,
    0x3, 0x80,

    /* U+00FE "þ" */
    0x78, 0x7, 0x80, 0x78, 0x7, 0xfc, 0x7a, 0xd7,
    0x9d, 0x79, 0xd7, 0x9d, 0x7a, 0xd7, 0xfc, 0x78,
    0x7, 0x80,

    /* U+00FF "ÿ" */
    0x73, 0xc7, 0x28, 0x0, 0xf, 0x2c, 0xb3, 0xc7,
    0x78, 0x3f, 0x42, 0xf0, 0x1f, 0x0, 0xf0, 0x1e,
    0x3, 0x80,

    /* U+0152 "Œ" */
    0x1f, 0xff, 0x4f, 0x7e, 0x47, 0xcf, 0x41, 0xe3,
    0xd0, 0x78, 0xff, 0x1e, 0x3e, 0x47, 0x8f, 0x41,
    0xf3, 0xd0, 0x3d, 0xf9, 0x7, 0xff, 0xd0,

    /* U+0153 "œ" */
    0x2f, 0x7d, 0x1e, 0xfb, 0xcb, 0x6f, 0xf2, 0xdb,
    0x44, 0xb6, 0xdf, 0x1e, 0xfb, 0xc2, 0xf7, 0xd0,

    /* U+0178 "Ÿ" */
    0x35, 0xc3, 0x6c, 0x0, 0xf, 0x5e, 0x7a, 0xd3,
    0xfc, 0x3f, 0x82, 0xf4, 0xf, 0x0, 0xf0, 0xf,
    0x0, 0xf0, 0xf, 0x0,

    /* U+0400 "Ѐ" */
    0x3c, 0x0, 0xd0, 0x4, 0x7, 0xfc, 0x7d, 0x47,
    0xc0, 0x7c, 0x7, 0xf8, 0x7d, 0x7, 0xc0, 0x7c,
    0x7, 0xd4, 0x7f, 0xc0,

    /* U+0401 "Ё" */
    0x35, 0xc3, 0x6c, 0x0, 0x7, 0xfc, 0x7d, 0x47,
    0xc0, 0x7c, 0x7, 0xf8, 0x7d, 0x7, 0xc0, 0x7c,
    0x7, 0xd4, 0x7f, 0xc0,

    /* U+0402 "Ђ" */
    0xff, 0xc0, 0x6f, 0x80, 0x1e, 0x0, 0x1e, 0x0,
    0x1f, 0xf8, 0x1f, 0x7d, 0x1e, 0x2e, 0x1e, 0x2e,
    0x1e, 0x3d, 0x1e, 0x78,

    /* U+0403 "Ѓ" */
    0x7, 0x80, 0xb0, 0x4, 0x7, 0xfd, 0x7e, 0x87,
    0xc0, 0x7c, 0x7, 0xc0, 0x7c, 0x7, 0xc0, 0x7c,
    0x7, 0xc0, 0x7c, 0x0,

    /* U+0404 "Є" */
    0x1f, 0xd0, 0xfb, 0xc7, 0xcf, 0x5f, 0x0, 0x7f,
    0x41, 0xf0, 0x7, 0xc6, 0x1f, 0x3d, 0x3e, 0xf0,
    0x7f, 0x40,

    /* U+0405 "Ѕ" */
    0x1f, 0x87, 0xfd, 0x78, 0xd7, 0xc0, 0x2f, 0x0,
    0xbc, 0x2, 0xe7, 0x5f, 0x3f, 0xe1, 0xf8,

    /* U+0406 "І" */
    0x7d, 0xf7, 0xdf, 0x7d, 0xf7, 0xdf, 0x7d, 0xf0,

    /* U+0407 "Ї" */
    0x72, 0x9c, 0xf0, 0x0, 0x7c, 0x1f, 0x7, 0xc1,
    0xf0, 0x7c, 0x1f, 0x7, 0xc1, 0xf0, 0x7c, 0x1f,
    0x0,

    /* U+0408 "Ј" */
    0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d, 0x3d,
    0xbd, 0xf8,

    /* U+0409 "Љ" */
    0xf, 0xfc, 0x0, 0x3e, 0xf0, 0x0, 0xf3, 0xc0,
    0x3, 0xcf, 0x0, 0x1e, 0x3d, 0x0, 0x78, 0xff,
    0x81, 0xe3, 0xdf, 0x4b, 0x8f, 0x2d, 0x7d, 0x3e,
    0xf3, 0xe0, 0xff, 0x40,

    /* U+040A "Њ" */
    0x7c, 0xb4, 0x7, 0xcb, 0x40, 0x7c, 0xb4, 0x7,
    0xcb, 0x40, 0x7e, 0xfe, 0x7, 0xff, 0xfc, 0x7c,
    0xb6, 0xe7, 0xcb, 0x5e, 0x7c, 0xbb, 0xd7, 0xcb,
    0xf8,

    /* U+040B "Ћ" */
    0xff, 0xc0, 0xbf, 0x80, 0x1e, 0x0, 0x1e, 0x0,
    0x1f, 0xf8, 0x1f, 0x7d, 0x1e, 0x2e, 0x1e, 0x2e,
    0x1e, 0x2e, 0x1e, 0x2e,

    /* U+040C "Ќ" */
    0x2, 0xc0, 0xe, 0x0, 0x10, 0x1f, 0x2e, 0x7c,
    0xf1, 0xf7, 0x87, 0xed, 0x1f, 0xf0, 0x7f, 0xc1,
    0xfb, 0x87, 0xdf, 0x1f, 0x3c, 0x7c, 0xb8,

    /* U+040D "Ѝ" */
    0xf, 0x0, 0x1c, 0x0, 0x10, 0x1d, 0x1d, 0x74,
    0xb5, 0xd3, 0xd7, 0x6f, 0x5e, 0xfd, 0x7f, 0x75,
    0xf9, 0xd7, 0xc7, 0x5e, 0x1d, 0x74, 0x74,

    /* U+040E "Ў" */
    0x18, 0xc0, 0x3e, 0xb, 0x4b, 0x5e, 0x3c, 0x3c,
    0xf0, 0xf3, 0x82, 0xde, 0x7, 0xb4, 0xf, 0xc0,
    0x3f, 0x0, 0xb8, 0x2, 0xd0, 0x2f, 0x1, 0xf4,
    0x0,

    /* U+040F "Џ" */
    0x7c, 0xb5, 0xf2, 0xd7, 0xcb, 0x5f, 0x2d, 0x7c,
    0xb5, 0xf2, 0xd7, 0xcb, 0x5f, 0x2d, 0x7e, 0xf5,
    0xff, 0xd0, 0x34, 0x0, 0xd0,

    /* U+0410 "А" */
    0xf, 0x80, 0x3f, 0x1, 0xfc, 0xb, 0xf0, 0x2e,
    0xd0, 0xf7, 0x83, 0xde, 0x1f, 0xfc, 0x78, 0xf2,
    0xd2, 0xd0,

    /* U+0411 "Б" */
    0x7f, 0xe1, 0xfa, 0x47, 0xc0, 0x1f, 0x0, 0x7c,
    0x1, 0xff, 0x47, 0xdf, 0x1f, 0x3c, 0x7d, 0xf1,
    0xff, 0x40,

    /* U+0412 "В" */
    0x7f, 0xd1, 0xf7, 0xc7, 0xcb, 0x5f, 0x3c, 0x7f,
    0xe1, 0xf7, 0xd7, 0xc7, 0x9f, 0x1e, 0x7e, 0xf5,
    0xff, 0x80,

    /* U+0413 "Г" */
    0x7f, 0xd7, 0xe8, 0x7c, 0x7, 0xc0, 0x7c, 0x7,
    0xc0, 0x7c, 0x7, 0xc0, 0x7c, 0x7, 0xc0,

    /* U+0414 "Д" */
    0xb, 0xfc, 0x2, 0xef, 0x0, 0xb3, 0xc0, 0x3c,
    0xf0, 0xf, 0x3c, 0x3, 0xcf, 0x0, 0xe3, 0xc0,
    0xb4, 0xf0, 0x7e, 0xbe, 0x3f, 0xff, 0xcf, 0x0,
    0x73, 0x80, 0x1c,

    /* U+0415 "Е" */
    0x7f, 0xc7, 0xd4, 0x7c, 0x7, 0xc0, 0x7f, 0x87,
    0xd0, 0x7c, 0x7, 0xc0, 0x7d, 0x47, 0xfc,

    /* U+0416 "Ж" */
    0xb4, 0xe2, 0xc7, 0x8e, 0x3c, 0x3c, 0xe7, 0x82,
    0xde, 0xb0, 0x1e, 0xef, 0x0, 0xff, 0xe0, 0x1e,
    0xef, 0x3, 0xce, 0xb4, 0x38, 0xe3, 0xcb, 0xe,
    0x2c,

    /* U+0417 "З" */
    0x2f, 0x87, 0xfe, 0xb4, 0xf0, 0x1e, 0xf, 0xc0,
    0x2e, 0x64, 0xf7, 0x8f, 0x3f, 0xe1, 0xf8,

    /* U+0418 "И" */
    0x74, 0x75, 0xd2, 0xd7, 0x4f, 0x5d, 0xbd, 0x7b,
    0xf5, 0xfd, 0xd7, 0xe7, 0x5f, 0x1d, 0x78, 0x75,
    0xd1, 0xd0,

    /* U+0419 "Й" */
    0xc, 0x90, 0x2f, 0x7, 0x47, 0x5d, 0x2d, 0x74,
    0xf5, 0xdb, 0xd7, 0xbf, 0x5f, 0xdd, 0x7e, 0x75,
    0xf1, 0xd7, 0x87, 0x5d, 0x1d,

    /* U+041A "К" */
    0x7c, 0xb9, 0xf3, 0xc7, 0xde, 0x1f, 0xb4, 0x7f,
    0xc1, 0xff, 0x7, 0xee, 0x1f, 0x7c, 0x7c, 0xf1,
    0xf2, 0xe0,

    /* U+041B "Л" */
    0xf, 0xfc, 0xf, 0xbc, 0xf, 0x3c, 0x1f, 0x3c,
    0x1e, 0x3c, 0x1e, 0x3c, 0x2e, 0x3c, 0x2d, 0x3c,
    0xbc, 0x3c, 0xf8, 0x3c,

    /* U+041C "М" */
    0x3c, 0x2e, 0x3d, 0x3e, 0x3e, 0x3e, 0x3f, 0x7e,
    0x3f, 0xbe, 0x3b, 0xee, 0x77, 0xee, 0x77, 0xde,
    0x76, 0xce, 0x72, 0xcf,

    /* U+041D "Н" */
    0x7c, 0xb9, 0xf2, 0xe7, 0xcb, 0x9f, 0x2e, 0x7e,
    0xf9, 0xff, 0xe7, 0xcb, 0x9f, 0x2e, 0x7c, 0xb9,
    0xf2, 0xe0,

    /* U+041E "О" */
    0x1f, 0xd0, 0xfb, 0xc7, 0xcf, 0x5f, 0x3d, 0x7c,
    0xf5, 0xf3, 0xd7, 0xcf, 0x5f, 0x3d, 0x3e, 0xf0,
    0x7f, 0x40,

    /* U+041F "П" */
    0x7f, 0xf9, 0xfb, 0xe7, 0xcb, 0x9f, 0x2e, 0x7c,
    0xb9, 0xf2, 0xe7, 0xcb, 0x9f, 0x2e, 0x7c, 0xb9,
    0xf2, 0xe0,

    /* U+0420 "Р" */
    0x7f, 0xe1, 0xfb, 0xd7, 0xcb, 0x5f, 0x1e, 0x7c,
    0xb5, 0xff, 0xc7, 0xd4, 0x1f, 0x0, 0x7c, 0x1,
    0xf0, 0x0,

    /* U+0421 "С" */
    0x1f, 0xd0, 0xfb, 0xc7, 0xcf, 0x5f, 0x28, 0x7c,
    0x1, 0xf0, 0x7, 0xca, 0x1f, 0x3d, 0x3e, 0xf0,
    0x7f, 0x40,

    /* U+0422 "Т" */
    0xff, 0xcb, 0xf8, 0x1f, 0x1, 0xf0, 0x1f, 0x1,
    0xf0, 0x1f, 0x1, 0xf0, 0x1f, 0x1, 0xf0,

    /* U+0423 "У" */
    0xb4, 0xb5, 0xe3, 0xc3, 0xcf, 0xf, 0x38, 0x2d,
    0xe0, 0x7b, 0x40, 0xfc, 0x3, 0xf0, 0xb, 0x80,
    0x2d, 0x2, 0xf0, 0x1f, 0x40,

    /* U+0424 "Ф" */
    0x0, 0x80, 0x0, 0x74, 0x2, 0xff, 0xe1, 0xf7,
    0xad, 0x75, 0xd7, 0x9d, 0x75, 0xe7, 0x5d, 0x79,
    0xe7, 0x6d, 0x3f, 0xff, 0x0, 0x78, 0x0, 0x1d,
    0x0,

    /* U+0425 "Х" */
    0xb4, 0xf0, 0xf7, 0x43, 0xfc, 0x7, 0xf0, 0xf,
    0x40, 0x7d, 0x2, 0xf8, 0xf, 0xf0, 0x76, 0xe2,
    0xc3, 0xc0,

    /* U+0426 "Ц" */
    0x3c, 0x78, 0x3c, 0x78, 0x3c, 0x78, 0x3c, 0x78,
    0x3c, 0x78, 0x3c, 0x78, 0x3c, 0x78, 0x3c, 0x78,
    0x3d, 0xbd, 0x3f, 0xfe, 0x0, 0xe, 0x0, 0xa,

    /* U+0427 "Ч" */
    0x78, 0xb9, 0xe2, 0xe7, 0x8b, 0x9e, 0x2e, 0x7c,
    0xb8, 0xbf, 0xe0, 0xb, 0x80, 0x2e, 0x0, 0xb8,
    0x2, 0xe0,

    /* U+0428 "Ш" */
    0x7c, 0xf3, 0xc7, 0xcf, 0x3c, 0x7c, 0xf3, 0xc7,
    0xcf, 0x3c, 0x7c, 0xf3, 0xc7, 0xcf, 0x3c, 0x7c,
    0xf3, 0xc7, 0xcf, 0x3c, 0x7e, 0xfb, 0xc7, 0xff,
    0xfc,

    /* U+0429 "Щ" */
    0x3c, 0xf3, 0xc0, 0xf3, 0xcf, 0x3, 0xcf, 0x3c,
    0xf, 0x3c, 0xf0, 0x3c, 0xf3, 0xc0, 0xf3, 0xcf,
    0x3, 0xcf, 0x3c, 0xf, 0x3c, 0xf0, 0x3d, 0xf7,
    0xe0, 0xff, 0xff, 0xc0, 0x0, 0x7, 0x0, 0x0,
    0xc,

    /* U+042A "Ъ" */
    0xff, 0x0, 0x6f, 0x0, 0x1f, 0x0, 0x1f, 0x0,
    0x1f, 0x0, 0x1f, 0xf4, 0x1f, 0x7c, 0x1f, 0x3c,
    0x1f, 0x7c, 0x1f, 0xf4,

    /* U+042B "Ы" */
    0x7c, 0x3, 0xc7, 0xc0, 0x3c, 0x7c, 0x3, 0xc7,
    0xc0, 0x3c, 0x7c, 0x3, 0xc7, 0xfd, 0x3c, 0x7d,
    0xf3, 0xc7, 0xcf, 0x3c, 0x7d, 0xf3, 0xc7, 0xfd,
    0x3c,

    /* U+042C "Ь" */
    0x7c, 0x1, 0xf0, 0x7, 0xc0, 0x1f, 0x0, 0x7c,
    0x1, 0xff, 0x47, 0xdf, 0x1f, 0x3c, 0x7d, 0xf1,
    0xff, 0x40,

    /* U+042D "Э" */
    0x1f, 0xd0, 0xfb, 0xc7, 0x8f, 0x40, 0x3d, 0x7,
    0xf4, 0x3, 0xd2, 0x4f, 0x5e, 0x3d, 0x3e, 0xf0,
    0x7f, 0x40,

    /* U+042E "Ю" */
    0x7c, 0x3f, 0x47, 0xcf, 0x7c, 0x7c, 0xf2, 0xd7,
    0xcf, 0x2d, 0x7f, 0xf2, 0xd7, 0xef, 0x2d, 0x7c,
    0xf2, 0xd7, 0xcf, 0x2c, 0x7c, 0xfb, 0xc7, 0xc3,
    0xf4,

    /* U+042F "Я" */
    0x1f, 0xf8, 0xfb, 0xe7, 0xcb, 0x9f, 0x2e, 0x3e,
    0xf8, 0x3f, 0xe1, 0xeb, 0x8f, 0x6e, 0x7c, 0xba,
    0xe2, 0xe0,

    /* U+0430 "а" */
    0x2f, 0x47, 0xbc, 0x53, 0xc2, 0xfc, 0xb3, 0xcb,
    0x7c, 0x7f, 0xc0,

    /* U+0431 "б" */
    0x0, 0x41, 0xfc, 0x39, 0x7, 0x0, 0x7f, 0x87,
    0xac, 0x76, 0xd7, 0x6d, 0x76, 0xd7, 0xac, 0x2f,
    0x80,

    /* U+0432 "в" */
    0x7f, 0xc7, 0x9d, 0x79, 0xd7, 0xfc, 0x79, 0xe7,
    0x9e, 0x7f, 0xc0,

    /* U+0433 "г" */
    0x7f, 0x1e, 0x47, 0x81, 0xe0, 0x78, 0x1e, 0x7,
    0x80,

    /* U+0434 "д" */
    0xb, 0xf0, 0xf, 0xb0, 0xe, 0xb0, 0xe, 0xb0,
    0x1d, 0xb0, 0x3c, 0xb4, 0xbf, 0xfc, 0xb0, 0x1c,
    0xb0, 0x1c,

    /* U+0435 "е" */
    0x2f, 0x47, 0xbc, 0xb6, 0xcb, 0xfc, 0xb5, 0x87,
    0xbc, 0x2f, 0x80,

    /* U+0436 "ж" */
    0xb3, 0x8e, 0x77, 0x9c, 0x3b, 0xbc, 0x2f, 0xf4,
    0x3b, 0xbc, 0x77, 0x9c, 0xb3, 0x8e,

    /* U+0437 "з" */
    0x2f, 0x47, 0x7c, 0x13, 0xc0, 0xf4, 0x12, 0xc7,
    0x7c, 0x3f, 0x40,

    /* U+0438 "и" */
    0x71, 0xe7, 0x3e, 0x77, 0xe7, 0xfe, 0x7d, 0xe7,
    0xce, 0x74, 0xe0,

    /* U+0439 "й" */
    0x25, 0xc1, 0xf4, 0x0, 0x7, 0x1e, 0x73, 0xe7,
    0x7e, 0x7f, 0xe7, 0xde, 0x7c, 0xe7, 0x4e,

    /* U+043A "к" */
    0x79, 0xf1, 0xeb, 0x47, 0xbc, 0x1f, 0xe0, 0x7b,
    0xc1, 0xeb, 0x47, 0x8f, 0x0,

    /* U+043B "л" */
    0x1f, 0xf0, 0x7b, 0xc1, 0xdf, 0x7, 0x7c, 0x2c,
    0xf1, 0xf3, 0xcf, 0x4f, 0x0,

    /* U+043C "м" */
    0x74, 0x75, 0xe2, 0xd7, 0xdf, 0x5f, 0xfd, 0x7b,
    0xb5, 0xcc, 0xd7, 0x3, 0x40,

    /* U+043D "н" */
    0x79, 0xe7, 0x9e, 0x79, 0xe7, 0xfe, 0x79, 0xe7,
    0x9e, 0x79, 0xe0,

    /* U+043E "о" */
    0x2f, 0x87, 0xbc, 0xb6, 0xdb, 0x6d, 0xb6, 0xd7,
    0xbc, 0x2f, 0x80,

    /* U+043F "п" */
    0x7f, 0xe7, 0xae, 0x79, 0xe7, 0x9e, 0x79, 0xe7,
    0x9e, 0x79, 0xe0,

    /* U+0440 "р" */
    0x7f, 0xc7, 0xae, 0x79, 0xe7, 0x9e, 0x79, 0xe7,
    0xad, 0x7f, 0xc7, 0x80, 0x78, 0x0,

    /* U+0441 "с" */
    0x2f, 0x47, 0xbc, 0xb6, 0xcb, 0x40, 0xb6, 0xc7,
    0xbc, 0x2f, 0x40,

    /* U+0442 "т" */
    0xff, 0x9b, 0x82, 0xd0, 0xb4, 0x2d, 0xb, 0x42,
    0xd0,

    /* U+0443 "у" */
    0xf2, 0xcb, 0x3c, 0x77, 0x83, 0xf4, 0x2f, 0x1,
    0xf0, 0xf, 0x1, 0xe0, 0x38, 0x0,

    /* U+0444 "ф" */
    0x2, 0xc0, 0x2, 0xc0, 0x2, 0xc0, 0x1f, 0xf8,
    0x7a, 0xde, 0xb6, 0xcf, 0xb6, 0xcf, 0xb6, 0xcf,
    0x7a, 0xde, 0x1f, 0xf8, 0x2, 0xc0, 0x2, 0xc0,

    /* U+0445 "х" */
    0xb6, 0xc3, 0xf4, 0x2f, 0x2, 0xf0, 0x3f, 0x7,
    0xb8, 0xb3, 0xc0,

    /* U+0446 "ц" */
    0x79, 0xe1, 0xe7, 0x87, 0x9e, 0x1e, 0x78, 0x79,
    0xe1, 0xeb, 0x87, 0xff, 0x80, 0xe, 0x0, 0x38,

    /* U+0447 "ч" */
    0xb6, 0xdb, 0x6d, 0xb6, 0xd3, 0xfd, 0x2, 0xd0,
    0x2d, 0x2, 0xd0,

    /* U+0448 "ш" */
    0x76, 0xcf, 0x76, 0xcf, 0x76, 0xcf, 0x76, 0xcf,
    0x76, 0xcf, 0x7b, 0xdf, 0x7f, 0xff,

    /* U+0449 "щ" */
    0x76, 0xcf, 0x1d, 0xb3, 0xc7, 0x6c, 0xf1, 0xdb,
    0x3c, 0x76, 0xcf, 0x1e, 0xb7, 0xc7, 0xff, 0xfc,
    0x0, 0xb, 0x0, 0x1, 0xc0,

    /* U+044A "ъ" */
    0xfd, 0x1, 0xb4, 0x2, 0xd0, 0xb, 0xf8, 0x2d,
    0xb4, 0xb6, 0xd2, 0xfe, 0x0,

    /* U+044B "ы" */
    0x78, 0xb, 0x5e, 0x2, 0xd7, 0x80, 0xb5, 0xfe,
    0x2d, 0x79, 0xdb, 0x5e, 0x76, 0xd7, 0xf8, 0xb4,

    /* U+044C "ь" */
    0x78, 0x7, 0x80, 0x78, 0x7, 0xf8, 0x79, 0xd7,
    0x9d, 0x7f, 0x80,

    /* U+044D "э" */
    0x2f, 0x47, 0xbc, 0x62, 0xd1, 0xfd, 0x12, 0xd7,
    0xbc, 0x2f, 0x40,

    /* U+044E "ю" */
    0x78, 0xbd, 0x7a, 0xdf, 0x7a, 0xcf, 0x7f, 0xcf,
    0x7a, 0xcf, 0x7a, 0xef, 0x78, 0xfd,

    /* U+044F "я" */
    0x2f, 0xe7, 0x5e, 0xb5, 0xe3, 0xfe, 0x3d, 0xe7,
    0x9e, 0xb5, 0xe0,

    /* U+0450 "ѐ" */
    0x3c, 0x1, 0xc0, 0x4, 0x2, 0xf4, 0x7b, 0xcb,
    0x6c, 0xbf, 0xcb, 0x58, 0x7b, 0xc2, 0xf8,

    /* U+2022 "•" */
    0x2f, 0x4f, 0xf7, 0xfc, 0xfe, 0x5, 0x0,

    /* U+2026 "…" */
    0xb3, 0xce, 0xb3, 0xce,

    /* U+2122 "™" */
    0x7f, 0x74, 0xd1, 0xd7, 0x9d, 0xc, 0x3e, 0xd0,
    0xc3, 0xfd, 0xc, 0x3a, 0xd0, 0xc3, 0x5d, 0x0,
    0x0, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 49, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 49, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 8, .adv_w = 69, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 13, .adv_w = 103, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 28, .adv_w = 93, .box_w = 6, .box_h = 13, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 48, .adv_w = 193, .box_w = 12, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 78, .adv_w = 109, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 96, .adv_w = 33, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 98, .adv_w = 65, .box_w = 4, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 110, .adv_w = 63, .box_w = 4, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 122, .adv_w = 81, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 129, .adv_w = 86, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 137, .adv_w = 46, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 140, .adv_w = 62, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 143, .adv_w = 47, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 145, .adv_w = 81, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 158, .adv_w = 106, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 173, .adv_w = 74, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 183, .adv_w = 99, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 198, .adv_w = 99, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 213, .adv_w = 101, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 231, .adv_w = 98, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 246, .adv_w = 103, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 261, .adv_w = 84, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 274, .adv_w = 100, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 289, .adv_w = 103, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 304, .adv_w = 53, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 309, .adv_w = 55, .box_w = 3, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 316, .adv_w = 76, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 324, .adv_w = 85, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 329, .adv_w = 76, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 337, .adv_w = 93, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 352, .adv_w = 182, .box_w = 12, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 388, .adv_w = 106, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 406, .adv_w = 113, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 424, .adv_w = 108, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 442, .adv_w = 113, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 460, .adv_w = 86, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 475, .adv_w = 83, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 488, .adv_w = 112, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 506, .adv_w = 117, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 524, .adv_w = 58, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 532, .adv_w = 67, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 542, .adv_w = 109, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 560, .adv_w = 85, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 575, .adv_w = 135, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 595, .adv_w = 108, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 610, .adv_w = 113, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 628, .adv_w = 110, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 646, .adv_w = 113, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 667, .adv_w = 115, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 685, .adv_w = 99, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 700, .adv_w = 85, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 715, .adv_w = 113, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 733, .adv_w = 101, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 751, .adv_w = 134, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 774, .adv_w = 99, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 792, .adv_w = 95, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 807, .adv_w = 83, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 820, .adv_w = 64, .box_w = 4, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 832, .adv_w = 81, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 845, .adv_w = 62, .box_w = 4, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 857, .adv_w = 91, .box_w = 6, .box_h = 4, .ofs_x = 0, .ofs_y = 6},
    {.bitmap_index = 863, .adv_w = 71, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 867, .adv_w = 59, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 7},
    {.bitmap_index = 870, .adv_w = 88, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 881, .adv_w = 97, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 896, .adv_w = 90, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 907, .adv_w = 97, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 922, .adv_w = 90, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 933, .adv_w = 61, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 943, .adv_w = 96, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 961, .adv_w = 98, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 976, .adv_w = 51, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 984, .adv_w = 52, .box_w = 4, .box_h = 12, .ofs_x = -1, .ofs_y = -2},
    {.bitmap_index = 996, .adv_w = 98, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1014, .adv_w = 53, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1022, .adv_w = 145, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1038, .adv_w = 97, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1049, .adv_w = 93, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1060, .adv_w = 97, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1074, .adv_w = 97, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1088, .adv_w = 74, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1097, .adv_w = 81, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1106, .adv_w = 67, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1115, .adv_w = 97, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1126, .adv_w = 81, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1135, .adv_w = 115, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1148, .adv_w = 85, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1159, .adv_w = 86, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1173, .adv_w = 75, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1182, .adv_w = 72, .box_w = 5, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1197, .adv_w = 50, .box_w = 3, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1206, .adv_w = 73, .box_w = 4, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1218, .adv_w = 92, .box_w = 6, .box_h = 4, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 1224, .adv_w = 49, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1224, .adv_w = 46, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1232, .adv_w = 88, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1249, .adv_w = 94, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1264, .adv_w = 107, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1278, .adv_w = 91, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1293, .adv_w = 42, .box_w = 2, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1299, .adv_w = 91, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1316, .adv_w = 82, .box_w = 5, .box_h = 2, .ofs_x = 0, .ofs_y = 8},
    {.bitmap_index = 1319, .adv_w = 170, .box_w = 11, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1347, .adv_w = 84, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 1357, .adv_w = 94, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1368, .adv_w = 96, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 1376, .adv_w = 171, .box_w = 11, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1404, .adv_w = 71, .box_w = 4, .box_h = 1, .ofs_x = 0, .ofs_y = 9},
    {.bitmap_index = 1405, .adv_w = 104, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 1414, .adv_w = 86, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1425, .adv_w = 91, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 1433, .adv_w = 91, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 1441, .adv_w = 59, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 7},
    {.bitmap_index = 1444, .adv_w = 94, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1458, .adv_w = 100, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1475, .adv_w = 43, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 1477, .adv_w = 68, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1481, .adv_w = 72, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 1487, .adv_w = 83, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 1496, .adv_w = 95, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1507, .adv_w = 164, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1532, .adv_w = 172, .box_w = 11, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1563, .adv_w = 177, .box_w = 11, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1591, .adv_w = 93, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1606, .adv_w = 106, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1629, .adv_w = 106, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1652, .adv_w = 106, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1675, .adv_w = 106, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1698, .adv_w = 106, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1721, .adv_w = 106, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1744, .adv_w = 136, .box_w = 10, .box_h = 10, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1769, .adv_w = 108, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 1792, .adv_w = 86, .box_w = 6, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1812, .adv_w = 86, .box_w = 6, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1832, .adv_w = 86, .box_w = 6, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1852, .adv_w = 86, .box_w = 6, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1872, .adv_w = 58, .box_w = 4, .box_h = 13, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1885, .adv_w = 58, .box_w = 4, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1898, .adv_w = 58, .box_w = 5, .box_h = 13, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1915, .adv_w = 58, .box_w = 5, .box_h = 13, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 1932, .adv_w = 108, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1950, .adv_w = 108, .box_w = 6, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1970, .adv_w = 113, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1993, .adv_w = 113, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2016, .adv_w = 113, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2039, .adv_w = 113, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2062, .adv_w = 113, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2085, .adv_w = 93, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 2094, .adv_w = 110, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 2115, .adv_w = 113, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2138, .adv_w = 113, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2161, .adv_w = 113, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2184, .adv_w = 113, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2207, .adv_w = 95, .box_w = 6, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2227, .adv_w = 108, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2245, .adv_w = 104, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2263, .adv_w = 88, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2278, .adv_w = 88, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2293, .adv_w = 88, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2308, .adv_w = 88, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2323, .adv_w = 88, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2338, .adv_w = 88, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2353, .adv_w = 130, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2367, .adv_w = 90, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 2382, .adv_w = 90, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2397, .adv_w = 90, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2412, .adv_w = 90, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2427, .adv_w = 90, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2442, .adv_w = 54, .box_w = 4, .box_h = 10, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 2452, .adv_w = 54, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2462, .adv_w = 54, .box_w = 5, .box_h = 10, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 2475, .adv_w = 54, .box_w = 5, .box_h = 10, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 2488, .adv_w = 100, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2503, .adv_w = 97, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2518, .adv_w = 93, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2533, .adv_w = 93, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2548, .adv_w = 93, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2563, .adv_w = 93, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2578, .adv_w = 93, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2593, .adv_w = 88, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 2604, .adv_w = 92, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2619, .adv_w = 97, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2634, .adv_w = 97, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2649, .adv_w = 97, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2664, .adv_w = 97, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2679, .adv_w = 86, .box_w = 6, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2697, .adv_w = 95, .box_w = 6, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2715, .adv_w = 86, .box_w = 6, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 2733, .adv_w = 138, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2756, .adv_w = 135, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2772, .adv_w = 95, .box_w = 6, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2792, .adv_w = 86, .box_w = 6, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2812, .adv_w = 86, .box_w = 6, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2832, .adv_w = 129, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2852, .adv_w = 89, .box_w = 6, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2872, .adv_w = 106, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2890, .adv_w = 99, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2905, .adv_w = 53, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2913, .adv_w = 53, .box_w = 5, .box_h = 13, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 2930, .adv_w = 62, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2940, .adv_w = 170, .box_w = 11, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2968, .adv_w = 160, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2993, .adv_w = 129, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3013, .adv_w = 110, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3036, .adv_w = 113, .box_w = 7, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3059, .adv_w = 106, .box_w = 7, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 3084, .adv_w = 115, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 3105, .adv_w = 106, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3123, .adv_w = 106, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3141, .adv_w = 113, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3159, .adv_w = 89, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3174, .adv_w = 133, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 3201, .adv_w = 86, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3216, .adv_w = 153, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3241, .adv_w = 101, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3256, .adv_w = 113, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3274, .adv_w = 113, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3295, .adv_w = 110, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3313, .adv_w = 126, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3333, .adv_w = 135, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3353, .adv_w = 117, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3371, .adv_w = 113, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3389, .adv_w = 116, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3407, .adv_w = 110, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3425, .adv_w = 108, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3443, .adv_w = 85, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3458, .adv_w = 106, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 3479, .adv_w = 146, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3504, .adv_w = 99, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3522, .adv_w = 130, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 3546, .adv_w = 117, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3564, .adv_w = 158, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3589, .adv_w = 170, .box_w = 11, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 3622, .adv_w = 121, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3642, .adv_w = 160, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3667, .adv_w = 107, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3685, .adv_w = 110, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3703, .adv_w = 158, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3728, .adv_w = 116, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3746, .adv_w = 88, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3757, .adv_w = 93, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3774, .adv_w = 97, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3785, .adv_w = 68, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3794, .adv_w = 118, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 3812, .adv_w = 90, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3823, .adv_w = 127, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3837, .adv_w = 86, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3848, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3859, .adv_w = 98, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3874, .adv_w = 98, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3887, .adv_w = 109, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3900, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3913, .adv_w = 101, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3924, .adv_w = 93, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3935, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3946, .adv_w = 97, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 3960, .adv_w = 90, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3971, .adv_w = 78, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 3980, .adv_w = 86, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 3994, .adv_w = 134, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 4018, .adv_w = 85, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 4029, .adv_w = 105, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 4045, .adv_w = 95, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 4056, .adv_w = 137, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 4070, .adv_w = 143, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 4091, .adv_w = 107, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 4104, .adv_w = 142, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 4120, .adv_w = 94, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 4131, .adv_w = 91, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 4142, .adv_w = 135, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 4156, .adv_w = 99, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 4167, .adv_w = 90, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 4182, .adv_w = 92, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 4189, .adv_w = 127, .box_w = 8, .box_h = 2, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 4193, .adv_w = 167, .box_w = 10, .box_h = 7, .ofs_x = 0, .ofs_y = 3}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_3[] = {
    0x0, 0x1, 0x26
};

static const uint16_t unicode_list_5[] = {
    0x0, 0x4, 0x100
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 160, .range_length = 13, .glyph_id_start = 96,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 174, .range_length = 82, .glyph_id_start = 109,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 338, .range_length = 39, .glyph_id_start = 191,
        .unicode_list = unicode_list_3, .glyph_id_ofs_list = NULL, .list_length = 3, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    },
    {
        .range_start = 1024, .range_length = 81, .glyph_id_start = 194,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 8226, .range_length = 257, .glyph_id_start = 275,
        .unicode_list = unicode_list_5, .glyph_id_ofs_list = NULL, .list_length = 3, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Map glyph_ids to kern left classes*/
static const uint8_t kern_left_class_mapping[] =
{
    0, 0, 0, 1, 0, 0, 0, 2,
    1, 0, 0, 3, 0, 4, 5, 4,
    6, 7, 8, 9, 0, 10, 11, 12,
    13, 14, 15, 16, 16, 0, 0, 0,
    0, 0, 17, 18, 19, 20, 21, 22,
    23, 0, 0, 24, 25, 26, 27, 0,
    20, 28, 20, 29, 30, 31, 32, 33,
    34, 35, 36, 37, 0, 38, 0, 0,
    0, 0, 39, 40, 41, 42, 43, 44,
    45, 46, 0, 0, 47, 48, 46, 46,
    49, 40, 50, 51, 52, 53, 54, 55,
    56, 57, 58, 59, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 60, 61, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 62, 0, 0, 0, 63, 17,
    17, 17, 17, 17, 64, 0, 19, 21,
    21, 21, 21, 0, 0, 0, 0, 20,
    0, 20, 20, 20, 20, 20, 0, 20,
    32, 32, 32, 32, 36, 65, 66, 39,
    39, 39, 39, 39, 39, 43, 41, 67,
    43, 43, 43, 0, 0, 0, 0, 0,
    46, 49, 49, 49, 49, 49, 0, 49,
    54, 54, 54, 54, 58, 49, 58, 21,
    43, 36, 68, 68, 0, 69, 70, 71,
    72, 72, 73, 0, 74, 0, 75, 72,
    0, 72, 76, 77, 78, 69, 0, 68,
    75, 78, 72, 72, 75, 72, 72, 72,
    79, 72, 80, 70, 81, 0, 0, 82,
    0, 72, 72, 0, 0, 72, 0, 79,
    79, 72, 83, 84, 85, 0, 86, 87,
    88, 0, 0, 0, 0, 0, 0, 0,
    89, 0, 90, 91, 92, 93, 90, 94,
    0, 0, 0, 0, 95, 0, 96, 90,
    90, 0, 87, 0, 0, 0
};

/*Map glyph_ids to kern right classes*/
static const uint8_t kern_right_class_mapping[] =
{
    0, 0, 0, 1, 0, 0, 0, 2,
    1, 0, 0, 3, 0, 4, 5, 6,
    7, 8, 9, 10, 0, 11, 0, 12,
    13, 12, 14, 15, 15, 0, 0, 0,
    0, 0, 16, 17, 18, 17, 17, 17,
    18, 17, 17, 19, 17, 17, 20, 17,
    18, 17, 21, 17, 22, 23, 24, 25,
    26, 27, 28, 29, 0, 30, 0, 0,
    0, 0, 31, 0, 32, 32, 32, 33,
    34, 0, 35, 36, 0, 0, 37, 37,
    32, 38, 39, 37, 40, 41, 42, 43,
    44, 45, 46, 47, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 48, 49, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 50, 0,
    0, 48, 51, 0, 0, 0, 52, 16,
    16, 16, 16, 16, 16, 53, 54, 17,
    17, 17, 17, 17, 17, 17, 17, 0,
    17, 18, 18, 18, 18, 18, 0, 18,
    24, 24, 24, 24, 28, 17, 55, 56,
    31, 31, 31, 31, 31, 31, 57, 32,
    32, 32, 32, 0, 35, 35, 35, 32,
    37, 58, 32, 32, 32, 32, 0, 32,
    42, 42, 42, 42, 46, 0, 46, 18,
    59, 28, 60, 60, 61, 60, 0, 62,
    60, 60, 63, 64, 60, 65, 60, 60,
    0, 60, 66, 60, 60, 60, 67, 60,
    68, 0, 60, 60, 60, 69, 60, 60,
    0, 60, 60, 0, 70, 0, 0, 71,
    60, 72, 60, 60, 73, 60, 60, 0,
    60, 0, 74, 0, 0, 0, 75, 76,
    77, 0, 0, 0, 0, 78, 0, 0,
    79, 0, 80, 76, 81, 82, 76, 83,
    0, 84, 0, 0, 85, 0, 0, 0,
    0, 0, 76, 0, 0, 86
};

/*Kern values between classes*/
static const int8_t kern_class_values[] =
{
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -13,
    0, -3, -7, 0, -3, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -3, -8,
    0, -10, 0, 0, 0, 0, -8, -7,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -14, -3, 0, -3,
    -8, -8, -8, 0, 0, 0, -7, 0,
    0, -13, 0, 0, 0, 0, 0, 0,
    0, -3, 0, -8, 0, 0, -8, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -7, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -7, 0, 0, 0, -7, 0,
    0, 0, 0, -7, 0, 0, -7, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -5, 0,
    0, 0, 0, 0, 0, 0, -3, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -5, 0, 0, 0, 0, 0,
    0, 0, -3, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -2, -3, 0, 0, -3, 0,
    -11, -3, -9, -7, 0, -10, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -5, 0, -1, -3, 0, -5,
    0, 0, 0, 0, 0, 0, 0, -3,
    0, 0, 0, 0, 0, -2, -11, 0,
    0, 0, -11, 0, 0, 0, 0, -11,
    0, 0, -11, 0, 0, 0, 0, 0,
    0, 0, 0, -4, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -8, 0,
    -4, -2, -3, -8, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -8, 0, 0, 0,
    -8, 0, 0, 0, 0, -8, -3, 0,
    -8, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -3,
    0, 0, -3, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -10, 0, 0,
    0, 0, 0, 0, -10, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -3, 0, 0, -10, -10,
    -10, 0, 0, -5, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -10, 0, 0, -10, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -2, -3,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -3, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -3, 0,
    0, 0, -2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -3, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -2, -2, 0, 0, -2, -2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -2, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -4, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -3, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -13, 0, -3, -5, 0, -9, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -13, 0,
    0, 0, -13, 0, 0, 0, 0, -13,
    0, 0, -13, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -13, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, 0, 0, -2, -9, -2,
    -7, -4, 0, -8, 0, 0, 0, 0,
    -3, 0, 0, 0, 0, 0, 0, 0,
    -2, 0, 0, -4, 0, -4, 0, -14,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -7, 0,
    0, -3, 0, -3, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -4, 0, -4, -1,
    -4, -5, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -3, 0, 0, -2,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -2, -1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -5, 0, 0, -3, 0, -3,
    0, 0, 0, 0, 0, 0, -2, 0,
    0, -2, 0, 0, -2, 0, 0, 0,
    -2, 0, -2, 0, -3, -3, -2, -7,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, 0, 0, 0, -7, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -2, 0,
    0, -16, 0, -16, 0, 0, 0, 0,
    0, 0, 0, 0, -3, -6, 0, 0,
    -4, 0, 0, -1, 0, 0, 0, 0,
    0, 0, 0, 0, -4, -2, 0, 0,
    0, 0, 0, 0, -2, -2, 0, 0,
    0, 0, 0, 0, -3, 0, 0, 0,
    0, 0, -17, 0, 0, 0, 0, -2,
    -2, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -5, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -3, 0, -2, -1, 0, -3,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -7, 0, -7,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, 0, 0, 0, -5, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -5, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -4, 0, 0, -4, -4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -3,
    0, 0, 0, 0, 0, 0, -3, 0,
    -3, -2, 0, -5, 0, -7, 0, -6,
    0, 0, 0, 0, 0, -4, 0, 0,
    -3, -3, -3, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 0,
    -21, 0, -8, 0, 0, 0, -7, 0,
    0, 0, -2, 0, 0, 0, 0, -2,
    0, 0, -3, -2, -13, -3, -13, -9,
    0, -16, 0, -16, 0, 0, -3, 0,
    0, 0, 0, 0, 0, 0, -5, 0,
    0, -6, 0, -11, 0, -22, 0, -15,
    0, 0, 0, -2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, -1, 0, -2, 0, 0, -2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -13, -10, -16,
    -14, 0, 0, 0, -1, 0, -2, 0,
    0, -9, 0, 0, -6, 0, 0, -2,
    0, 0, 0, 0, -3, 0, -1, -5,
    -7, -4, 0, -5, 0, 0, 0, 0,
    0, -5, 0, 0, 0, 0, -2, 0,
    -3, 0, 0, 0, 0, -13, -25, 0,
    0, -7, -4, -4, -4, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -3,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, 0, -2, -1, -2, -1,
    -2, -1, 0, -4, 0, -5, -2, -4,
    0, 0, 0, 0, 0, 0, -4, 0,
    0, 0, 0, 0, 0, -2, -3, 0,
    0, 0, 0, 0, 0, -2, 0, -2,
    -4, -4, -4, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -3, 0, 0, 0,
    0, 0, 0, 0, 0, -2, 0, 0,
    0, -2, 0, -3, 0, 0, -2, 0,
    -2, -2, 0, -7, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -2, 0, -11,
    -8, -11, -10, 0, 0, 0, 0, 0,
    0, 0, -13, -9, 0, -2, -5, -1,
    -2, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -13, -10, -4, -11, 0, -1,
    -10, -10, 0, -12, -2, -8, 0, -10,
    -8, -9, -8, -1, -8, 0, 0, -7,
    -17, -2, 0, 1, -10, -10, -10, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -3, 0, -3,
    -5, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -7, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -11, -4, -11, 0, 0,
    0, 0, 0, 0, 0, 0, -5, -7,
    0, -2, -1, -2, -2, -1, 0, 0,
    0, 0, 0, 0, 0, 0, -9, -7,
    0, -8, 0, 0, -5, -5, -6, -6,
    0, -4, -2, -4, -5, -3, -8, -3,
    0, 0, 0, 0, -12, -2, 0, -9,
    -7, -7, -7, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -7, -2, -7, -7, 0, 0, 0,
    0, 0, 0, 0, -5, -4, 0, 0,
    0, 0, 0, -1, 0, 0, 0, 0,
    0, 0, 0, -3, -4, -3, -2, -5,
    0, 0, -3, -2, 0, -1, 0, 0,
    0, 0, 0, 0, 0, -1, 0, 0,
    0, 0, -11, 0, 0, -4, -3, -3,
    -3, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -5, 0,
    -3, 0, 0, -2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -3, 0, 0,
    -3, -3, 0, 0, 0, 0, 0, 0,
    0, -3, 0, -2, -1, 0, 0, 0,
    0, 0, -2, -2, -1, 0, -5, -4,
    0, -6, 0, -5, 0, 0, 0, 0,
    0, -3, 0, 0, -2, -2, -2, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -15, -8, -15,
    -8, -3, 0, 0, -5, -2, 0, 0,
    -11, -8, 0, -3, -1, -2, -3, -2,
    0, 0, 0, 0, 0, 0, 0, 0,
    -12, -12, -7, -12, 0, 0, -11, -9,
    -2, -10, 0, -10, 0, -7, -8, -6,
    -7, -4, 0, 0, -3, 0, -16, -3,
    -1, -12, -12, -12, -12, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, 0, -2, -1, 0, 0,
    0, 0, 0, 0, 0, 0, -2, -2,
    0, 0, 0, 0, 0, 0, -2, 0,
    0, 0, 0, 0, 0, 0, 0, -2,
    0, 0, 0, 0, 0, -2, 0, -2,
    -2, -2, -2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -3,
    0, 0, -3, -5, -10, -5, -1, -8,
    0, -10, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -3, 0, 0, 0, 0,
    0, 0, -10, -5, 0, 0, -10, 0,
    0, 0, 0, -10, 0, 0, -10, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -7, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -10, 0, -8, -3, 0, -11,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -8, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -10, 0, -6, -3, -2, -12, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -1, 0,
    0, -1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -8, 0, 0, 0, 0, 0, -2, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -8, 0,
    -4, -1, 0, -9, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 9, 0, 5, 5,
    0, 5, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -10, 0, -5, -1, 0, -12,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, 1, -6, 0, 1, 1, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -3, 0,
    0, 0, -1, -2, 0, 0, -2, 0,
    0, 0, 0, 5, 0, 0, 0, 0,
    2, -1, 0, 2, 0, 3, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -9, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -10, 0, -6, -3,
    0, -11, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -3, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -7, 0, -3, -2, 0, -7,
    0, 0, 0, -3, 0, -2, 0, 0,
    0, 0, -3, -2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -3, -3, -3, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    6, 0, 3, 3, 0, 4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -8, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -10, 0,
    -6, -3, -2, -12, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -1, 0, 0, -1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -3, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -10, 0, -1, 0,
    0, -7, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -18,
    0, -18, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -5, 0, 0, -5, 0,
    0, 0, -8, 0, 0, 0, -5, -5,
    -4, 0, -3, 0, 0, -2, 0, 0,
    0, 0, -4, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -3, 0, -1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -7, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -9, 0, -5, 0, 0, -11, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -7, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -1, -1, 0,
    -3, 0, 0, -5, 0, 0, 0, 0,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -5, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -10, 0, -4, -1,
    0, -10, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -8,
    0, -8, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -5, 0, 0, -5, 0,
    0, 0, -11, 0, -4, 0, -5, -7,
    -4, 0, -2, 0, 0, -2, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -4, 0, -4,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -3, 0, 0, -6, 0, 0, 0,
    -10, 0, -4, 0, -4, -7, -3, 0,
    -2, 0, 0, -2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -8, 0,
    -5, 0, 0, -8, 0, 0, 0, -2,
    0, 0, 0, 0, 0, 0, -2, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, -3, -2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -7, 0, -7, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -5, 0, 0,
    -5, 0, 0, 0, -11, 0, -4, 0,
    -5, -7, -4, 0, -2, 0, 0, -2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -2, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -8, 0, -4, 0, 0, -7,
    0, 0, 0, -1, 0, -2, 0, 0,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -1, -1, -1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -1, 0, 0, 0,
    0, 0, 0, 0, -2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -3, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -8, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -8, 0, 0, 0, -8, 0,
    0, 0, 0, -8, 0, 0, -8, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -7, 0, 0, 0, 0, -8,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -7, 0, 0, 0, -7, 0, 0, 0,
    0, -7, 0, 0, -7, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -13, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -2, 0, 0, 0, -2,
    -9, -2, -7, -4, 0, -8, 0, 0,
    0, 0, -3, 0, 0, 0, 0, 0,
    0, 0, -2, 0, 0, -4, 0, -4,
    0, -14, 0, 0, 0, 0, 0, -2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -2,
    0, 0, 0, 0, 0, 0, -2, 0,
    0, 0, -4, -2, -2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -10, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -4, 0, -3, -1,
    -1, -7, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -1, -1, 0,
    -1, -1, -1, -1, -1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -10, 0, -5, -1, 0, -12,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -3, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, -11, -8, -11, -10, 0,
    0, 0, 0, 0, 0, 0, -13, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -8, 0, 0, -7, 0, 0, 0, 0,
    0, 0, 0, -1, 0, 0, -5, 0,
    0, -14, 0, 0, 0, 0, 0, 0,
    0, -13, 0, -10, 0, 0, -10, -10,
    0, -10, -8, 0, 0, 0, -3, 0,
    0, -2, 0, -2, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -3, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -7, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -2,
    0, -3, 0, 0, 0, -2, 0, 0,
    0, 0, -2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -1, -2,
    0, 0, -1, 0, 0, 0, 0, -1,
    0, 0, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -7, 0, -7, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -6, 0,
    0, 0, 0, -5, 0, -11, -9, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -5, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -4, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -3,
    0, 0, -3, 0, 0, 0, 0, 0,
    0, 0, -13, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -9, -2,
    0, 0, -9, 0, 0, 0, 0, -9,
    0, -4, -1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -7, 0,
    0, -3, 0, -3, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -4, 0, 0, 0, -4, 0,
    0, 0, 0, -4, -4, 0, -4, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -5, 0, 0, -3,
    0, -3, 0, 0, 0, 0, 0, 0,
    -2, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -7, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -2, 0, -2, 0, -2, -2, 0, 0,
    0, -2, -3, 0, -2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -13, -10, -16,
    -14, 0, 0, 0, -1, 0, -2, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -5,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, -13, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -2,
    -6, -7, 0, -9, -16, -1, -10, 0,
    -3, 0, 0, -7, 0, -4, 0, 0,
    -4, 0, 0, 0, -2, 0, 0, -3,
    0, -2, 0, -11, -8, -11, -10, 0,
    0, 0, 0, 0, 0, 0, -13, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -8, 0, 0, -7, 0, 0, 0, 0,
    0, 0, 0, -1, 0, 0, -5, 0,
    0, -9, 0, 0, 0, 0, 0, 0,
    0, -13, 0, -10, 0, 0, -10, -10,
    0, -10, -8, 0, 0, 0, 0, 0,
    -5, 0, -3, 0, 0, -2, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -3, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -3, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, -2, 0, 0, -2, 0, 0, -6,
    0, 0, 0, 0, -7, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -10, 0, 0, 0, -10, 0, 0, 0,
    0, -10, 0, 0, -10, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -8, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -10, 0,
    0, 0, -10, 0, 0, 0, 0, -10,
    -2, 0, -10, 0, -7, 0, 0, -2,
    0, 0, 0, 0, -1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -10, 0, 0, 0, -10, 0, 0, 0,
    0, -10, 0, 0, -10, 0, 0, 0,
    0, 0, 0, 0, 0, 0, -2, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -1, 0, 0, 0, 0,
    -2, 0, 0, 0, 0, 0, 0, 0,
    -8, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -10, 0, 0, 0,
    -10, 0, 0, 0, 0, -10, -2, 0,
    -10, 0, -4, 0, 0, 0, 0, 0,
    -2, 0, -1, 0, 0, 0, -8, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -10, 0, 0, 0, -10, 0,
    0, 0, 0, -10, -2, 0, -10, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, 0, 0, 0, -8, 0, 0, 0,
    0, 0, -2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -8, 0, 0, 0, -8, 0, 0, 0,
    0, -8, 0, 0, -8, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, -8,
    -2, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -4, 0, -4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -10, 0, -6, 0,
    -10, -3, 0, 0, 0, -10, -4, 0,
    -10, -2, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -8, 0, 0, 0, -8, 0,
    0, 0, 0, -8, 0, 0, -8, 0,
    0, -2, 0, 0, -2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, -6, 0, 0, -8,
    -11, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -9, 0, 0, -7, -10, 0
};


/*Collect the kern class' data in one place*/
static const lv_font_fmt_txt_kern_classes_t kern_classes =
{
    .class_pair_values   = kern_class_values,
    .left_class_mapping  = kern_left_class_mapping,
    .right_class_mapping = kern_right_class_mapping,
    .left_class_cnt      = 96,
    .right_class_cnt     = 86,
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
    .kern_dsc = &kern_classes,
    .kern_scale = 16,
    .cmap_num = 6,
    .bpp = 2,
    .kern_classes = 1,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif

};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t font_oswaldBold_12 = {
#else
lv_font_t font_oswaldBold_12 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 16,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
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



#endif /*#if FONT_OSWALDBOLD_12*/
