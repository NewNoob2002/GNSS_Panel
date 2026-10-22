#include <assert.h>
#include <string.h>
#include "benchmark/lv_demo_benchmark.h"
#include "demo_perf.h"

static uint16_t pixels[294 * 20];
static unsigned flushes;

static void
flush(lv_display_t* driver, const lv_area_t* area, uint8_t* color) {
    (void)area;
    (void)color;
    ++flushes;
    lv_display_flush_ready(driver);
}

int main(void) {
    lv_init();
    lv_display_t* display = lv_display_create(294, 126);
    assert(display);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, pixels, NULL, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
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
    lv_display_delete(display);
    return 0;
}
