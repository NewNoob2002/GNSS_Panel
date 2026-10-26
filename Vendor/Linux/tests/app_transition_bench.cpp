/**
 * @file app_transition_bench.cpp
 * @brief 主机切屏渲染基准（不注册为 ctest）：首页往返工作/系统/信息页，逐帧（16 ms）计时 lv_timer_handler（含渲染）
 *        并统计刷新像素。耗时只用于同一台机器、同一构建类型下比较切屏方案或 LVGL 配置，不代表 MCU 实际帧时间；
 *        实机帧率用调试固件的 LV_USE_PERF_MONITOR 确认。用法：app_transition_bench [轮数，默认 30]。
 */
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "App.h"
#include "DemoStatus.h"
#include "DomainTestInput.hpp"
#define CHECK(x)                                                                                                       \
    do {                                                                                                               \
        if (!(x)) {                                                                                                    \
            std::fprintf(stderr, "check failed: %s\n", #x);                                                            \
            std::abort();                                                                                              \
        }                                                                                                              \
    } while (0)
constexpr int W = 294, H = 126;
static uint16_t draw_pixels[W * 42];
static uint64_t flushed;

static void
flush(lv_display_t* d, const lv_area_t* a, uint8_t*) {
    flushed += lv_area_get_size(a);
    lv_display_flush_ready(d);
}

static double
frame() { // one 16 ms period
    lv_tick_inc(16);
    DataProc_RunLoop();
    auto t0 = std::chrono::steady_clock::now();
    lv_timer_handler();
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::micro>(t1 - t0).count();
}

int
main(int argc, char** argv) {
    lv_init();
    auto* display = lv_display_create(W, H);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, draw_pixels, nullptr, sizeof(draw_pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    demo::model().power = {};
    App app;
    CHECK(app.init());
    for (int i = 0; i < 40; ++i) {
        frame();
    }
    test_input::status(DemoStatus::sample(16000));
    CHECK(app.show_page("Pages/Dialplate"));
    for (int i = 0; i < 200; ++i) {
        frame();
    }
    const int rounds = argc > 1 ? std::atoi(argv[1]) : 30;
    // 每次进入/返回各计 26 帧（约 420 ms，覆盖 350 ms 切屏与弹簧收敛），各轮取中位数。
    const auto run = [&](const char* label, auto enter, auto leave, int visits) {
        std::vector<double> totals;
        uint64_t px = 0;
        int frames = 0;
        for (int r = 0; r < rounds; ++r) {
            double total = 0;
            flushed = 0;
            frames = 0;
            for (int v = 0; v < visits; ++v) {
                enter(v);
                for (int i = 0; i < 26; ++i, ++frames) {
                    total += frame();
                }
                leave();
                for (int i = 0; i < 26; ++i, ++frames) {
                    total += frame();
                }
            }
            totals.push_back(total);
            px = flushed;
        }
        std::sort(totals.begin(), totals.end());
        std::printf(
            "%-26s frames/round=%d median_us_per_frame=%.1f min=%.1f flushed_px_per_round=%llu (%.1f screens)\n", label,
            frames, totals[totals.size() / 2] / frames, totals[0] / frames, (unsigned long long)px, px / double(W * H));
    };
    const char* pages[] = {"Pages/WorkSettings", "Pages/SystemDash", "Pages/SystemInfos"};
    run("page anim, 3 pages", [&](int v) { CHECK(app.show_page(pages[v])); }, [&] { CHECK(app.back()); }, 3);
    run(
        "page anim, SystemInfos", [&](int) { CHECK(app.show_page("Pages/SystemInfos")); }, [&] { CHECK(app.back()); },
        1);
    // 首页默认焦点为信息入口：确认键走入口展开（NONE 切屏），返回再收回。
    run(
        "entry expand, SystemInfos", [&](int) { app.on_input(App::InputAction::Confirm); },
        [&] { app.on_input(App::InputAction::Back); }, 1);
}
