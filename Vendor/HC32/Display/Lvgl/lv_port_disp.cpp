#include "board_display.h"
#include <delay.h>
#include "lv_port.h"
#include "mcu_config.h"

static PanelRm690a0& screen = board_display_panel;

// 逻辑横屏宽度为 CONFIG_SCREEN_HOR_RES；42 行使 126 行高的整屏正好分 3 块刷新。
#define SCREEN_BUFFER_ROWS 42U
#define SCREEN_BUFFER_SIZE (CONFIG_SCREEN_HOR_RES * SCREEN_BUFFER_ROWS)
static_assert(SCREEN_BUFFER_SIZE * sizeof(uint16_t) <= UINT16_MAX, "DMA 单次传输计数不能超过 65535");
// 画面在玻璃上的水平平移（逻辑像素，正值向右）。RM690A0 窗口起点与尺寸须按 2 像素对齐（见 my_rounder_cb），
// 偏移为偶数才不破坏对齐；移出玻璃的列不发送，未覆盖的边缘保持初始化时填充的黑色。
static_assert(PRODUCT_DISPLAY_X_SHIFT % 2 == 0, "PRODUCT_DISPLAY_X_SHIFT must be even");
static_assert(PRODUCT_DISPLAY_X_SHIFT > -CONFIG_SCREEN_HOR_RES && PRODUCT_DISPLAY_X_SHIFT < CONFIG_SCREEN_HOR_RES,
              "PRODUCT_DISPLAY_X_SHIFT out of range");

static lv_display_t* display;

/* LVGL 单缓冲渲染，旋转后的数据放入独立的 DMA 发送缓冲。
 * LVGL 自带 sw_rotate 会强制“渲染→旋转→发送”串行执行，这里自行旋转，
 * 让下一块的渲染与上一块的 DMA 发送并行。 */
static uint16_t draw_buf[SCREEN_BUFFER_SIZE] __attribute__((aligned(4)));
static uint16_t tx_buf[SCREEN_BUFFER_SIZE] __attribute__((aligned(4)));
static bool flush_active;
static uint32_t flush_started_ms;
volatile uint32_t display_dma_timeouts;
static uint8_t target_brightness = 0xFF;

#if PRODUCT_DISPLAY_BRIGHTNESS_FADE
static constexpr uint32_t kBrightnessFadeMs = 300U;
static constexpr uint32_t kBrightnessStepMs = 16U;
static constexpr uint32_t kBrightnessFallbackMs = 1000U;
static uint32_t brightness_wait_started_ms;
static uint32_t brightness_started_ms;
static uint32_t brightness_step_ms;
static bool brightness_done;
static bool brightness_started;
static bool brightness_polled;
static bool first_frame_failed;
static bool flush_last;

/** @brief 无定时器分配；首帧失败或 1 s 无完整帧时直接全亮，DMA 必须先回收。 */
static void
brightness_poll() {
    if (brightness_done || flush_active) {
        return;
    }
    const uint32_t now = millis();
    if (first_frame_failed || (!brightness_started && now - brightness_wait_started_ms >= kBrightnessFallbackMs)) {
        screen.setBrightness(0xFF);
        brightness_done = true;
        return;
    }
    if (!brightness_started || now - brightness_step_ms < kBrightnessStepMs) {
        return;
    }
    brightness_step_ms = now;
    const uint32_t elapsed = now - brightness_started_ms;
    if (elapsed >= kBrightnessFadeMs) {
        screen.setBrightness(0xFF);
        brightness_done = true;
    } else {
        const uint32_t value =
            255U * elapsed * (2U * kBrightnessFadeMs - elapsed) / (kBrightnessFadeMs * kBrightnessFadeMs);
        screen.setBrightness(static_cast<uint8_t>(value));
    }
}

/** @brief LVGL 首帧的最后一块已完成移位，之后的轮询才发送亮度命令。 */
static void
first_frame_complete() {
    if (flush_last && !brightness_started && !first_frame_failed) {
        brightness_started = true;
        brightness_started_ms = brightness_step_ms = millis();
    }
}
#endif

/** @brief 将失效区域扩展到面板要求的双像素边界。 */
static void
my_rounder_cb(lv_event_t* event) {
    auto* area = static_cast<lv_area_t*>(lv_event_get_param(event));
    area->x1 = area->x1 & ~1;
    area->x2 = area->x2 | 1;
    area->y1 = area->y1 & ~1;
    area->y2 = area->y2 | 1;
}

static void
wait_flush_idle() {
    while (flush_active) {
        lv_port_disp_poll();
    }
}

// 与 LVGL 8.3 sw_rotate 的 LV_DISPLAY_ROTATION_90 映射一致：逻辑 (x, y) -> 物理 (y, H - 1 - x)。
static void
rotate_90(const uint16_t* src, int32_t w, int32_t h, uint16_t* dst) {
    for (int32_t y = 0; y < h; ++y) {
        uint16_t* out = dst + (uint32_t)(w - 1) * h + y;
        for (int32_t x = 0; x < w; ++x) {
            const uint16_t pixel = *src++;
            // RM690A0 高字节在前；交换只发生在发送缓冲，绘制缓冲保持原生 RGB565。
            *out = static_cast<uint16_t>((pixel << 8) | (pixel >> 8));
            out -= h;
        }
    }
}

static void
disp_flush(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    const auto* color_p = reinterpret_cast<const uint16_t*>(px_map);
    // tx_buf 仍可能被上一块 DMA 读取，必须等发送结束后才能覆写。
    wait_flush_idle();

    const int32_t w = lv_area_get_width(area);
    const int32_t h = lv_area_get_height(area);
    rotate_90(color_p, w, h, tx_buf);
    /** @brief LVGL 在 flush_ready 后可能清除末块标记，须先保存。 */
#if PRODUCT_DISPLAY_BRIGHTNESS_FADE
    flush_last = lv_display_flush_is_last(disp);
#endif
    // 数据已复制到 tx_buf，立即归还绘制缓冲，LVGL 可在 DMA 期间渲染下一块。
    lv_display_flush_ready(disp);

    // 旋转后 tx_buf 每行对应一个逻辑 x（第 0 行为 x2），物理行 = 屏高 - 1 - x - 偏移；只发送落在玻璃内的行。
    const int32_t panel_rows = screen.height();
    int32_t first_row = panel_rows - 1 - area->x2 - PRODUCT_DISPLAY_X_SHIFT;
    int32_t rows = w;
    int32_t skipped = 0;
    if (first_row < 0) {
        skipped = -first_row;
        first_row = 0;
        rows -= skipped;
    }
    if (first_row + rows > panel_rows) {
        rows = panel_rows - first_row;
    }
    if (rows <= 0) {
#if PRODUCT_DISPLAY_BRIGHTNESS_FADE
        first_frame_complete();
#endif
        return; // 整块都在玻璃外
    }

    flush_active = true;
    flush_started_ms = millis();

    // setWindow 不操作 CS；先选中屏幕，再发送窗口命令和像素，完成/超时后才释放 CS。
    screen.select();
    screen.setWindow(area->y1, first_row, area->y1 + h - 1, first_row + rows - 1);
    screen.dataMode();
    if (!board_display_spi.startDmaWrite(tx_buf + (uint32_t)skipped * h, (uint32_t)rows * h * sizeof(uint16_t))) {
        // 静态缓冲和长度已受编译期约束；初始化异常时也必须释放片选，避免卡住下一帧。
        board_display_spi.abort();
        screen.deselect();
        flush_active = false;
#if PRODUCT_DISPLAY_BRIGHTNESS_FADE
        first_frame_failed = true;
#endif
    }
}

void
lv_port_disp_poll() {
#if PRODUCT_DISPLAY_BRIGHTNESS_FADE
    /** @brief App_Init 的建页耗时不计入首帧兜底窗口，首次进入轮询后才计时。 */
    if (!brightness_polled) {
        brightness_wait_started_ms = millis();
        brightness_polled = true;
    }
#endif
    if (flush_active) {
        const bool expired = millis() - flush_started_ms >= 100U;
        // IDLNF 为 1 表示 SPI 仍忙；DMA 结束时最后一个字节可能还在移位。
        if ((board_display_spi.dmaDone() && board_display_spi.isIdle()) || expired) {
            board_display_spi.abort();
            screen.deselect();
            flush_active = false;
            if (expired) {
                ++display_dma_timeouts;
            }
#if PRODUCT_DISPLAY_BRIGHTNESS_FADE
            first_frame_failed = first_frame_failed || expired;
            first_frame_complete();
#endif
        }
    }
#if PRODUCT_DISPLAY_BRIGHTNESS_FADE
    brightness_poll();
#endif
}

void
lv_port_disp_init() {
    // 保留 DMA 先于面板 GPIO/SPI 初始化的顺序；与旧 DMA_Init 失败一样停止初始化。
    if (!board_display_spi.beginDma() || !screen.init()) {
        for (;;) {}
    }
    screen.fill(0x0000);
    screen.enableBacklight();
    // 面板物理竖屏，逻辑横屏旋转在 disp_flush 中完成。
    display = lv_display_create(screen.height(), screen.width());
    if (!display) {
#if PRODUCT_DISPLAY_BRIGHTNESS_FADE
        screen.setBrightness(0xFF);
#endif
        for (;;) {}
    }
    target_brightness = 0xFF;
#if PRODUCT_DISPLAY_BRIGHTNESS_FADE
    brightness_done = brightness_started = brightness_polled = first_frame_failed = flush_last = false;
#endif
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, draw_buf, nullptr, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, disp_flush);
    lv_display_add_event_cb(display, my_rounder_cb, LV_EVENT_INVALIDATE_AREA, nullptr);
}

void
lv_port_reset() {
    // 重新初始化会发送命令，必须先让在途的 DMA 帧结束。
    wait_flush_idle();
    if (!screen.init()) {
        for (;;) {}
    }
    screen.fill(0x0000);
    // 唤醒恢复用户选择，不重复冷启动动画。
#if PRODUCT_DISPLAY_BRIGHTNESS_FADE
    screen.setBrightness(target_brightness);
    brightness_done = true;
#else
    if (target_brightness != 0xFF) {
        screen.setBrightness(target_brightness);
    }
#endif
}

void
lv_port_set_brightness(uint8_t value) {
    wait_flush_idle();
#if PRODUCT_DISPLAY_BRIGHTNESS_FADE
    brightness_done = true;
#endif
    target_brightness = value;
    screen.setBrightness(value);
}

#if defined(__CORE_DEBUG)
/** @brief GDB 手动验证入口；先回收 DMA，再同步设置亮度，不添加面板控制命令。 */
__attribute__((noinline)) void
lv_port_debug_brightness(uint8_t value) {
    lv_port_set_brightness(value);
}
#endif
