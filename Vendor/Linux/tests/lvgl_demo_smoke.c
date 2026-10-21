#include <assert.h>
#include <string.h>
#include "benchmark/lv_demo_benchmark.h"
#include "demo_perf.h"

static lv_color_t pixels[294 * 20];
static unsigned flushes;

static void flush(lv_disp_drv_t* driver, const lv_area_t* area, lv_color_t* color) {
    (void)area;
    (void)color;
    ++flushes;
    lv_disp_flush_ready(driver);
}

int main(void) {
    lv_init();
    lv_disp_draw_buf_t draw;
    lv_disp_draw_buf_init(&draw, pixels, NULL, 294 * 20);
    lv_disp_drv_t driver;
    lv_disp_drv_init(&driver);
    driver.hor_res = 294;
    driver.ver_res = 126;
    driver.draw_buf = &draw;
    driver.flush_cb = flush;
    lv_disp_t* display = lv_disp_drv_register(&driver);
    assert(display);

    lv_demo_benchmark();
    struct demo_perf perf;
    demo_perf_start(&perf, display, 0);
    lv_refr_now(display);
    assert(flushes);
    assert(perf.updates > 0);
    demo_perf_update(&perf, 5000);

    const char* text = lv_label_get_text(perf.label);
    assert(strstr(text, "FPS") && strstr(text, "CPU"));
    assert(!strstr(text, "0.0 FPS"));
    demo_perf_finish(&perf, 5000);
    lv_disp_remove(display);
    return 0;
}
