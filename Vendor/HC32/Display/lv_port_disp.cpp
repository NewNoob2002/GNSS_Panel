#include "board_display.h"
#include "delay.h"
#include "lv_port.h"
#include "mcu_config.h"

static PanelRm690a0& screen = board_display_panel;

// 逻辑横屏宽度为 CONFIG_SCREEN_HOR_RES；42 行使 126 行高的整屏正好分 3 块刷新。
#define SCREEN_BUFFER_ROWS 42U
#define SCREEN_BUFFER_SIZE (CONFIG_SCREEN_HOR_RES * SCREEN_BUFFER_ROWS)
static_assert(SCREEN_BUFFER_SIZE * sizeof(lv_color_t) <= UINT16_MAX, "DMA 单次传输计数不能超过 65535");
// 画面在玻璃上的水平平移（逻辑像素，正值向右）。RM690A0 窗口起点与尺寸须按 2 像素对齐（见 my_rounder_cb），
// 偏移为偶数才不破坏对齐；移出玻璃的列不发送，未覆盖的边缘保持初始化时填充的黑色。
static_assert(PRODUCT_DISPLAY_X_SHIFT % 2 == 0, "PRODUCT_DISPLAY_X_SHIFT must be even");
static_assert(PRODUCT_DISPLAY_X_SHIFT > -CONFIG_SCREEN_HOR_RES && PRODUCT_DISPLAY_X_SHIFT < CONFIG_SCREEN_HOR_RES,
              "PRODUCT_DISPLAY_X_SHIFT out of range");

static lv_disp_drv_t disp_drv; /*Descriptor of a display driver*/

/* LVGL 单缓冲渲染，旋转后的数据放入独立的 DMA 发送缓冲。
 * LVGL 自带 sw_rotate 会强制“渲染→旋转→发送”串行执行，这里自行旋转，
 * 让下一块的渲染与上一块的 DMA 发送并行。 */
static lv_disp_draw_buf_t draw_buf_dsc;
static lv_color_t draw_buf[SCREEN_BUFFER_SIZE] __attribute__((aligned(4)));
static lv_color_t tx_buf[SCREEN_BUFFER_SIZE] __attribute__((aligned(4)));
static bool flush_active;
static uint32_t flush_started_ms;
volatile uint32_t display_dma_timeouts;

void
my_rounder_cb(lv_disp_drv_t* disp_drv, lv_area_t* area) {
    (void)disp_drv;
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

// 与 LVGL 8.3 sw_rotate 的 LV_DISP_ROT_90 映射一致：逻辑 (x, y) -> 物理 (y, H - 1 - x)。
static void
rotate_90(const lv_color_t* src, lv_coord_t w, lv_coord_t h, lv_color_t* dst) {
    for (lv_coord_t y = 0; y < h; ++y) {
        lv_color_t* out = dst + (uint32_t)(w - 1) * h + y;
        for (lv_coord_t x = 0; x < w; ++x) {
            *out = *src++;
            out -= h;
        }
    }
}

static void
disp_flush(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p) {
    // tx_buf 仍可能被上一块 DMA 读取，必须等发送结束后才能覆写。
    wait_flush_idle();

    const lv_coord_t w = lv_area_get_width(area);
    const lv_coord_t h = lv_area_get_height(area);
    rotate_90(color_p, w, h, tx_buf);
    // 数据已复制到 tx_buf，立即归还绘制缓冲，LVGL 可在 DMA 期间渲染下一块。
    lv_disp_flush_ready(disp_drv);

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
        return; // 整块都在玻璃外
    }

    flush_active = true;
    flush_started_ms = millis();

    // setWindow 不操作 CS；先选中屏幕，再发送窗口命令和像素，完成/超时后才释放 CS。
    screen.select();
    screen.setWindow(area->y1, first_row, area->y1 + h - 1, first_row + rows - 1);
    screen.dataMode();
    if (!board_display_spi.startDmaWrite(tx_buf + (uint32_t)skipped * h, (uint32_t)rows * h * sizeof(lv_color_t))) {
        // 静态缓冲和长度已受编译期约束；初始化异常时也必须释放片选，避免卡住下一帧。
        board_display_spi.abort();
        screen.deselect();
        flush_active = false;
    }
}

void
lv_port_disp_poll() {
    if (!flush_active) {
        return;
    }
    const bool expired = millis() - flush_started_ms >= 100U;
    // IDLNF 为 1 表示 SPI 仍忙；DMA 结束时最后一个字节可能还在移位。
    if ((board_display_spi.dmaDone() && board_display_spi.isIdle()) || expired) {
        board_display_spi.abort();
        screen.deselect();
        flush_active = false;
        if (expired) {
            ++display_dma_timeouts;
        }
    }
}

void
lv_port_disp_init() {
    // 保留 DMA 先于面板 GPIO/SPI 初始化的顺序；与旧 DMA_Init 失败一样停止初始化。
    if (!board_display_spi.beginDma() || !screen.init()) {
        for (;;) {}
    }
    screen.fill(0x0000);
    screen.enableBacklight();
    lv_disp_draw_buf_init(&draw_buf_dsc, draw_buf, NULL, SCREEN_BUFFER_SIZE);
    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    lv_disp_drv_init(&disp_drv); /*Basic initialization*/

    /*Set up the functions to access to your display*/

    /*面板物理竖屏，LVGL 使用横屏逻辑坐标，旋转在 disp_flush() 中完成。*/
    disp_drv.hor_res = screen.height();
    disp_drv.ver_res = screen.width();

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush;

    disp_drv.rounder_cb = my_rounder_cb;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}

void
lv_port_reset() {
    // 重新初始化会发送命令，必须先让在途的 DMA 帧结束。
    wait_flush_idle();
    if (!screen.init()) {
        for (;;) {}
    }
    screen.fill(0x0000);
}
