#ifndef RK3506_DEMO_PERF_H
#define RK3506_DEMO_PERF_H

#include <stdint.h>
#include "lvgl.h"

struct demo_perf {
    lv_obj_t* label;
    uint64_t updates;
    uint64_t pixels;
    uint64_t total_updates;
    uint64_t start_wall_ms;
    uint64_t last_wall_ms;
    uint64_t last_cpu_ns;
};

/* 订阅显示事件，不替换 benchmark 自己的监控回调。 */
void demo_perf_start(struct demo_perf* perf, lv_display_t* display, uint64_t wall_ms);
void demo_perf_update(struct demo_perf* perf, uint64_t wall_ms);
void demo_perf_finish(const struct demo_perf* perf, uint64_t wall_ms);

#endif
