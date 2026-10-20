#include "ResourcePool.h"
#include "Utils/ResourceManager/ResourceManager.h"

static ResourceManager font_;
static ResourceManager image_;

extern "C" {
#define IMPORT_FONT(name)                                                                                              \
    do {                                                                                                               \
        LV_FONT_DECLARE(font_##name)                                                                                   \
        font_.add_resource(#name, (void*)&font_##name);                                                                \
    } while (0)

#define IMPORT_SYMBOL(name)                                                                                            \
    do {                                                                                                               \
        LV_FONT_DECLARE(symbol_##name)                                                                                 \
        font_.add_resource(#name, (void*)&symbol_##name);                                                              \
    } while (0)

#define IMPORT_IMG(name)                                                                                               \
    do {                                                                                                               \
        LV_IMG_DECLARE(img_src_##name)                                                                                 \
        image_.add_resource(#name, (void*)&img_src_##name);                                                            \
    } while (0)

static void resource_init() {
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
    IMPORT_IMG(system_dash_power);
    IMPORT_IMG(system_dash_power_small);
    IMPORT_IMG(system_dash_settings);
    IMPORT_IMG(system_dash_return);
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

void resource_pool::init() {
    resource_init();
    font_.set_default((void*)LV_FONT_DEFAULT);
}

lv_font_t* resource_pool::get_font(const char* name) {
    return static_cast<lv_font_t*>(font_.get_resource(name));
}

const void* resource_pool::get_image(const char* name) {
    return image_.get_resource(name);
}
