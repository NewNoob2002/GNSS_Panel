/*********************
 *      INCLUDES
 *********************/
#define DEFAULT_SOURCE /* needed for usleep() */
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#define SDL_MAIN_HANDLED /*To fix SDL's "undefined reference to WinMain" issue*/
#include "lv_drivers/sdl/sdl.h"

#include <cstdlib>
#include <cstring>
#include "App/App.h"
#include "HAL/HAL.h"
#include "elog.h"

/*********************
 *      DEFINES
 *********************/
extern "C" {
extern const lv_img_dsc_t mouse_cursor_icon; /*Declare the image file.*/
}

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void hal_init();

/**********************
 *  STATIC VARIABLES
 **********************/
SystemInfo_t systemInfo;
// BatteryState batteryState;
/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      VARIABLES
 **********************/
extern void StatusBar_Appear(bool en);

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int
main(const int argc, char** argv) {
    /*Initialize LVGL*/
    lv_init();
    /*Initialize the HAL (display, input devices, tick) for LVGL*/
    hal_init();
    memset(&systemInfo, 0, sizeof(systemInfo));
    systemInfo.powerMonitor.batteryInfo.Temp_f = 25.0f;
    systemInfo.positionInfo.satellite_number_used = 32;
    systemInfo.positionInfo.satellite_number_track = 48;
    constexpr char wifi_id[16] = "E1PRO-2124A0078";
    memcpy(systemInfo.wifiInfo.wifi_ssid, wifi_id, 16);
    /* set EasyLogger log format */
    elog_init();
    /* set EasyLogger log format */
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_ALL & ~(ELOG_FMT_FUNC | ELOG_FMT_T_INFO | ELOG_FMT_P_INFO));
    elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_ALL & ~(ELOG_FMT_FUNC | ELOG_FMT_T_INFO | ELOG_FMT_P_INFO));
    /* start EasyLogger */
    elog_start();
    HAL::HAL_Init();
    App_Init();
    std::atexit(App_Deinit);
    // lv_timer_t *timer = lv_timer_create(timer_callback, 50, nullptr);
    // lv_timer_ready(timer);
    // systemInfo.powerMonitor.batteryInfo.chargeStatus = normalCharge;
    systemInfo.powerMonitor.batteryInfo.Percent = 18;
    systemInfo.starMapInfo.numberGPS = 8;
    systemInfo.starMapInfo.numberBDS = 14;
    systemInfo.starMapInfo.numberGLONASS = 6;
    systemInfo.starMapInfo.numberGALILEO = 4;
    systemInfo.starMapInfo.numberSBAS = 2;
    systemInfo.starMapInfo.numberQZSS = 3;
    systemInfo.starMapInfo.numberIRNSS = 1;
    while (true) {
        /* Periodically call the lv_task handler.
         * It could be done in a timer interrupt or an OS task too.*/
        HAL::HAL_Update();
        App_Update();
        lv_timer_handler();
        // if (systemInfo.powerMonitor.PowerKey_PressCount < 100) {
        //     systemInfo.powerMonitor.PowerKey_PressCount++;
        // }
        if (systemInfo.powerMonitor.ShutdownComplete) {
            break;
        }
#if defined(_WIN32)
        Sleep(5);
#else
        usleep(5 * 1000);
#endif
    }
    return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the LVGL graphics
 * library
 */
static void
hal_init() {
    /* Use the 'monitor' driver which creates window on PC's monitor to simulate a display*/
    sdl_init();

    /*Create a display buffer*/
    static lv_disp_draw_buf_t disp_buf1;
    static lv_color_t buf1_1[SDL_HOR_RES * 100];
    lv_disp_draw_buf_init(&disp_buf1, buf1_1, nullptr, SDL_HOR_RES * 100);

    /*Create a display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv); /*Basic initialization*/
    disp_drv.draw_buf = &disp_buf1;
    disp_drv.flush_cb = sdl_display_flush;
    disp_drv.hor_res = SDL_HOR_RES;
    disp_drv.ver_res = SDL_VER_RES;

    lv_disp_t* disp = lv_disp_drv_register(&disp_drv);

    // lv_theme_t *th = lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
    //                                        LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT);
    // lv_disp_set_theme(disp, th);

    lv_group_t* g = lv_group_create();
    lv_group_set_default(g);

    /* Add the mouse as input device
     * Use the 'mouse' driver which reads the PC's mouse*/
    static lv_indev_drv_t indev_drv_1;
    lv_indev_drv_init(&indev_drv_1); /*Basic initialization*/
    indev_drv_1.type = LV_INDEV_TYPE_POINTER;

    /*This function will be called periodically (by the library) to get the mouse position and state*/
    indev_drv_1.read_cb = sdl_mouse_read;
    lv_indev_t* mouse_indev = lv_indev_drv_register(&indev_drv_1);

    static lv_indev_drv_t indev_drv_2;
    lv_indev_drv_init(&indev_drv_2); /*Basic initialization*/
    indev_drv_2.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv_2.read_cb = sdl_keyboard_read;
    lv_indev_t* kb_indev = lv_indev_drv_register(&indev_drv_2);
    lv_indev_set_group(kb_indev, g);

    static lv_indev_drv_t indev_drv_3;
    lv_indev_drv_init(&indev_drv_3); /*Basic initialization*/
    indev_drv_3.type = LV_INDEV_TYPE_ENCODER;
    indev_drv_3.read_cb = sdl_mousewheel_read;
    lv_indev_t* enc_indev = lv_indev_drv_register(&indev_drv_3);
    lv_indev_set_group(enc_indev, g);

    /*Set a cursor for the mouse*/
    lv_obj_t* cursor_obj = lv_img_create(lv_scr_act()); /*Create an image object for the cursor */
    lv_img_set_src(cursor_obj, &mouse_cursor_icon);     /*Set the image source*/
    lv_indev_set_cursor(mouse_indev, cursor_obj);       /*Connect the image  object to the driver*/
}
