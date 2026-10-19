#include "ResourcePool.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "lvgl/src/widgets/lv_img.h"

static ResourceManager Font_;
static ResourceManager Image_;

extern "C" {
#define IMPORT_FONT(name)                                                                                              \
    do {                                                                                                               \
        LV_FONT_DECLARE(font_##name)                                                                                   \
        Font_.AddResource(#name, (void*)&font_##name);                                                                 \
    } while (0)

#define IMPORT_SYMBOL(name)                                                                                            \
    do {                                                                                                               \
        LV_FONT_DECLARE(symbol_##name)                                                                                 \
        Font_.AddResource(#name, (void*)&symbol_##name);                                                               \
    } while (0)

#define IMPORT_IMG(name)                                                                                               \
    do {                                                                                                               \
        LV_IMG_DECLARE(img_src_##name)                                                                                 \
        Image_.AddResource(#name, (void*)&img_src_##name);                                                             \
    } while (0)

static void
Resource_Init() {
    /* Import Fonts */
    IMPORT_FONT(oswaldBold_12);
    IMPORT_FONT(oswaldBold_18);
    IMPORT_FONT(rajdhaniBold_20);
    IMPORT_FONT(rajdhaniBold_40);
    /* Import Fonts */
    IMPORT_SYMBOL(statusbar);
    IMPORT_SYMBOL(dialplate);
    // /* Import Images */
    IMPORT_IMG(battery);
    IMPORT_IMG(battery_info);
    IMPORT_IMG(record);
    IMPORT_IMG(clock);
    IMPORT_IMG(mode);
    IMPORT_IMG(settings);
    IMPORT_IMG(menu);
    IMPORT_IMG(start);
    IMPORT_IMG(stop);
    IMPORT_IMG(satellite_small);
    IMPORT_IMG(satellite_big);
    IMPORT_IMG(map_location);
    IMPORT_IMG(storage);
    IMPORT_IMG(system_info);
    IMPORT_IMG(wifi);
    IMPORT_IMG(workmode);
    IMPORT_IMG(up);
    IMPORT_IMG(down);
    IMPORT_IMG(reset);
    IMPORT_IMG(back);
    IMPORT_IMG(shutdown);
    IMPORT_IMG(rover);
    IMPORT_IMG(base);
    IMPORT_IMG(ntrip);

    IMPORT_IMG(NationalFlag_EN);
    IMPORT_IMG(NationalFlag_RU);
#if defined(RGK_LOGO_USE)
    IMPORT_IMG(RGKLogo);
#elif defined(MIDDLE_LOGO_USE)
    IMPORT_IMG(MiddleLogo);
#else
    IMPORT_IMG(startupLogo);
#endif // RGK_LOGO_USE
}
} /* extern "C" */

void
ResourcePool::Init() {
    Resource_Init();
    Font_.SetDefault((void*)LV_FONT_DEFAULT);
}

lv_font_t*
ResourcePool::GetFont(const char* name) {
    return static_cast<lv_font_t*>(Font_.GetResource(name));
}

const void*
ResourcePool::GetImage(const char* name) {
    return Image_.GetResource(name);
}
