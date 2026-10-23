#include <cassert>
#include <delay.h>
#include <iostream>
#include <string>
#include <vector>
#include "board_display.h"
#include "lv_port.h"
#include "mcu_config.h"

CM_SPI_TypeDef testSpi;
CM_DMA_TypeDef testDma;
extern volatile uint32_t display_dma_timeouts;

struct Event {
    std::string op;
    uint32_t a = 0;
    uint32_t b = 0;

    /** 比较可观测的硬件调用、字节和控制线事件。 */
    bool
    operator==(const Event& other) const {
        return op == other.op && a == other.a && b == other.b;
    }
};

static std::vector<Event> trace;
static void (*dmaIrq)();
static uint32_t nowMs;
static uint32_t pollStep;
static uint32_t sourceAddress;
static uint16_t transferCount;
static int dmaResult = LL_OK;
static int irqResult = LL_OK;
static int spiResult = LL_OK;
static const uint8_t* dmaPixels;
static bool completeDmaImmediately;

extern "C" bool __real__ZN9SpiMaster13startDmaWriteEPKvm(SpiMaster*, const void*, size_t);

/** @brief 在地址截为 MCU 的 32 位寄存器前观察真实 DMA 发送数据，不改变传输流程。 */
extern "C" bool
__wrap__ZN9SpiMaster13startDmaWriteEPKvm(SpiMaster* spi, const void* data, size_t size) {
    dmaPixels = static_cast<const uint8_t*>(data);
    const bool started = __real__ZN9SpiMaster13startDmaWriteEPKvm(spi, data, size);
    if (started && completeDmaImmediately) {
        testSpi.SR &= ~SPI_FLAG_IDLE;
        dmaIrq();
    }
    return started;
}

/** 追加硬件事件，不模拟发送耗时。 */
static void
record(const char* op, uint32_t a = 0, uint32_t b = 0) {
    trace.push_back({op, a, b});
}

/** DR 写入是线上可观测字节，SPI 必须已使能。 */
void
FakeSpiData::operator=(uint32_t value) {
    assert(testSpi.CR1 & SPI_CR1_SPE);
    record("byte", value);
}

/** 原 transfer 会读 DR；测试不使用接收结果。 */
FakeSpiData::
operator uint32_t() const {
    return 0;
}

/** 记录 SPI 复位并恢复默认关闭状态。 */
void
SPI_DeInit(CM_SPI_TypeDef* regs) {
    assert(regs == CM_SPI3);
    regs->CR1 = 0;
    record("spi-deinit");
}

/** 模拟 SPI 使能位，供阻塞发送和 DMA 顺序断言。 */
void
SPI_Cmd(CM_SPI_TypeDef* regs, en_functional_state_t state) {
    assert(regs == CM_SPI3);
    regs->CR1 = state == ENABLE ? SPI_CR1_SPE : 0;
    record("spi", state);
}

/** 记录 SPI 默认结构初始化。 */
int
SPI_StructInit(stc_spi_init_t* config) {
    *config = {};
    record("spi-struct");
    return LL_OK;
}

/** 核对旧 SPI_Settings 的全部显式字段。 */
int
SPI_Init(CM_SPI_TypeDef* regs, const stc_spi_init_t* config) {
    assert(regs == CM_SPI3 && regs->CR1 == 0);
    assert(config->u32WireMode == SPI_4_WIRE && config->u32TransMode == SPI_FULL_DUPLEX);
    assert(config->u32MasterSlave == SPI_MASTER && config->u32DataBits == SPI_DATA_SIZE_8BIT);
    assert(config->u32SpiMode == SPI_MD_0 && config->u32BaudRatePrescaler == SPI_BR_CLK_DIV2);
    assert(config->u32FirstBit == SPI_FIRST_MSB && config->u32FrameLevel == SPI_1_FRAME);
    record("spi-init");
    return spiResult;
}

/** 记录 SPI 时钟门控。 */
void
FCG_Fcg1PeriphClockCmd(uint32_t clock, en_functional_state_t state) {
    record("fcg1", clock, state);
}

/** 记录 DMA/AOS 时钟门控。 */
void
FCG_Fcg0PeriphClockCmd(uint32_t clock, en_functional_state_t state) {
    record("fcg0", clock, state);
}

/** 核对 DMA 默认结构初始化在显式字段赋值之前。 */
int
DMA_StructInit(stc_dma_init_t* config) {
    *config = {};
    record("dma-struct");
    return LL_OK;
}

/** 核对旧端口 DMA_Init 的全部字段和目的寄存器。 */
int
DMA_Init(CM_DMA_TypeDef* regs, uint8_t channel, const stc_dma_init_t* config) {
    assert(regs == CM_DMA1 && channel == DMA_CH0);
    assert(config->u32IntEn == DMA_INT_ENABLE && config->u32BlockSize == 1);
    assert(config->u32TransCount == 65535 && config->u32DataWidth == DMA_DATAWIDTH_8BIT);
    assert(config->u32SrcAddrInc == DMA_SRC_ADDR_INC && config->u32DestAddrInc == DMA_DEST_ADDR_FIX);
    assert(config->u32SrcAddr == 0);
    assert(config->u32DestAddr == static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&testSpi.DR)));
    record("dma-init");
    return dmaResult;
}

/** 记录 DMA 单元使能。 */
void
DMA_Cmd(CM_DMA_TypeDef* regs, en_functional_state_t state) {
    assert(regs == CM_DMA1);
    record("dma", state);
}

/** 记录 DMA 通道使能和禁用。 */
void
DMA_ChCmd(CM_DMA_TypeDef* regs, uint8_t channel, en_functional_state_t state) {
    assert(regs == CM_DMA1 && channel == DMA_CH0);
    record("channel", state);
}

/** 记录清 TC；ISR 不得夹带 SPI/GPIO 动作。 */
void
DMA_ClearTransCompleteStatus(CM_DMA_TypeDef* regs, uint32_t flag) {
    assert(regs == CM_DMA1 && flag == DMA_FLAG_TC_CH0);
    record("tc");
}

/** 保存低 32 bit 总线源地址，与固件的 DMA 地址宽度一致。 */
void
DMA_SetSrcAddr(CM_DMA_TypeDef* regs, uint8_t channel, uint32_t address) {
    assert(regs == CM_DMA1 && channel == DMA_CH0);
    sourceAddress = address;
    record("source");
}

/** 保存单次传输字节数。 */
void
DMA_SetTransCount(CM_DMA_TypeDef* regs, uint8_t channel, uint16_t count) {
    assert(regs == CM_DMA1 && channel == DMA_CH0);
    transferCount = count;
    record("count", count);
}

/** 核对 AOS 目标与 SPI TX 触发源。 */
void
AOS_SetTriggerEventSrc(uint32_t channel, en_event_src_t source) {
    assert(channel == AOS_DMA1_0 && source == EVT_SRC_SPI3_SPTI);
    record("aos");
}

/** 捕获板级跳板并验证中断映射。 */
int
INTC_IrqSignIn(const stc_irq_signin_config_t* config) {
    assert(config->enIntSrc == INT_SRC_DMA1_TC0 && config->enIRQn == INT009_IRQn);
    dmaIrq = config->pfnCallback;
    record("irq-signin");
    return irqResult;
}

/** 核对清 pending 的 IRQ。 */
void
NVIC_ClearPendingIRQ(IRQn_Type irq) {
    assert(irq == INT009_IRQn);
    record("irq-clear");
}

/** 核对 DMA 中断优先级。 */
void
NVIC_SetPriority(IRQn_Type irq, uint32_t priority) {
    assert(irq == INT009_IRQn && priority == DDL_IRQ_PRIO_04);
    record("irq-priority");
}

/** 核对使能的 IRQ。 */
void
NVIC_EnableIRQ(IRQn_Type irq) {
    assert(irq == INT009_IRQn);
    record("irq-enable");
}

/** 记录引脚方向及默认电平；模式与电平编码为两个参数。 */
void
gpio_set_mode(gpio_pin_t pin, gpio_mode_t mode, uint8_t level) {
    record("mode", pin, static_cast<uint32_t>(mode) * 2 + level);
}

/** 记录控制线电平。 */
void
gpio_write(gpio_pin_t pin, uint32_t level) {
    record("gpio", pin, level);
}

/** 记录引脚复用选择。 */
void
GPIO_SetFunction(gpio_pin_t pin, uint16_t function) {
    record("function", pin, function);
}

/** 记录显式延时，同时推进虚拟时间。 */
void
delay_ms(uint32_t ms) {
    record("delay", ms);
    nowMs += ms;
}

/** 可在轮询中推进时间以验证 reset 等待旧帧超时，以及毫秒计数回绕。 */
uint32_t
millis() {
    const uint32_t result = nowMs;
    nowMs += pollStep;
    return result;
}

/** 逐事件比对，错误时打印首个不一致位置便于定位时序差异。 */
static void
expectTrace(const std::vector<Event>& expected) {
    for (size_t i = 0; i < expected.size() && i < trace.size(); ++i) {
        if (!(expected[i] == trace[i])) {
            std::cerr << "event " << i << ": expected " << expected[i].op << '/' << expected[i].a << '/'
                      << expected[i].b << ", got " << trace[i].op << '/' << trace[i].a << '/' << trace[i].b << '\n';
            assert(false);
        }
    }
    assert(expected.size() == trace.size());
}

/** 独立列出旧库的完整上电轨迹，包括窗口、清屏全部 74088 字节与控制线切换。 */
static std::vector<Event>
expectedPanelInit(bool dmaInit) {
    std::vector<Event> expected;
    if (dmaInit) {
        expected = {{"fcg0", FCG0_PERIPH_DMA1 | FCG0_PERIPH_AOS, ENABLE},
                    {"dma-struct"},
                    {"dma-init"},
                    {"aos"},
                    {"irq-signin"},
                    {"irq-clear"},
                    {"irq-priority"},
                    {"irq-enable"},
                    {"dma", ENABLE},
                    {"channel", DISABLE}};
    }
    const std::vector<Event> setup = {
        {"mode", PB14, GPIO_OUTPUT * 2},
        {"gpio", PB14, 1},
        {"mode", PB1, GPIO_OUTPUT * 2},
        {"gpio", PB1, 1},
        {"spi-deinit"},
        {"mode", PB15, GPIO_OUTPUT_AF_PP * 2},
        {"mode", PB13, GPIO_OUTPUT_AF_PP * 2},
        {"function", PB15, 43},
        {"function", PB13, 40},
        {"fcg1", FCG1_PERIPH_SPI3, ENABLE},
        {"spi", DISABLE},
        {"spi-struct"},
        {"spi-init"},
        {"mode", PB2, GPIO_OUTPUT * 2},
        {"gpio", PB2, 1},
        {"delay", 100},
        {"gpio", PB2, 0},
        {"delay", 100},
        {"gpio", PB2, 1},
        {"delay", 200},
        {"gpio", PB2, 1},
        {"delay", 50},
        {"gpio", PB2, 0},
        {"delay", 50},
        {"gpio", PB2, 1},
        {"delay", 150},
    };
    expected.insert(expected.end(), setup.begin(), setup.end());

    struct Command {
        uint8_t command;
        std::vector<uint8_t> data;
        uint32_t delay;
    };

    const Command commands[] = {
        {0x01, {}, 150},
        {0xFE, {0x01}, 0},
        {0x6A, {0x21}, 0},
        {0xFE, {0x00}, 0},
        {0xC4, {0x80}, 0},
        {0x35, {0x00}, 0},
        {0x51, {PRODUCT_DISPLAY_BRIGHTNESS_FADE ? 0x00 : 0xFF}, 0},
        {0x3A, {0x05}, 0},
        {0x20, {}, 0},
        {0x11, {}, 500},
        {0x29, {}, 500},
        {0x36, {0x00}, 0},
    };
    bool first = true;
    for (const auto& command : commands) {
        expected.push_back({"gpio", PB14, 0});
        expected.push_back({"gpio", PB1, 0});
        if (first) {
            expected.push_back({"spi", ENABLE});
            first = false;
        }
        expected.push_back({"byte", command.command});
        expected.push_back({"gpio", PB1, 1});
        for (uint8_t value : command.data) {
            expected.push_back({"byte", value});
        }
        expected.push_back({"gpio", PB14, 1});
        if (command.delay != 0) {
            expected.push_back({"delay", command.delay});
        }
    }
    const std::vector<Event> window = {
        {"gpio", PB14, 0}, {"gpio", PB1, 0}, {"byte", 0x2A},   {"gpio", PB1, 1}, {"byte", 0},      {"byte", 0},
        {"byte", 0},       {"byte", 0x7D},   {"gpio", PB1, 0}, {"byte", 0x2B},   {"gpio", PB1, 1}, {"byte", 0},
        {"byte", 0},       {"byte", 1},      {"byte", 0x25},   {"gpio", PB1, 0}, {"byte", 0x2C},   {"gpio", PB1, 1},
    };
    expected.insert(expected.end(), window.begin(), window.end());
    expected.insert(expected.end(), 126U * 294U * 2U, {"byte", 0});
    expected.push_back({"gpio", PB14, 1});
    return expected;
}

#if PRODUCT_DISPLAY_BRIGHTNESS_FADE
/** @brief 当前 trace 中全部线上字节，供亮度命令断言使用。 */
static std::vector<uint32_t>
bytes() {
    std::vector<uint32_t> result;
    for (const auto& event : trace) {
        if (event.op == "byte") {
            result.push_back(event.a);
        }
    }
    return result;
}

/** @brief 0x51 命令的完整事务；首字节发送前 SPI 已被 DMA 回收关闭。 */
static std::vector<Event>
brightnessTrace(uint8_t value) {
    return {{"gpio", PB14, 0}, {"gpio", PB1, 0}, {"spi", ENABLE},  {"byte", 0x51},
            {"gpio", PB1, 1},  {"byte", value},  {"gpio", PB14, 1}};
}
#endif

/** 用真实 LVGL 注册的驱动执行 flush，验证窗口和 DMA，不运行 UI 定时器。 */
static void
flush(lv_area_t area) {
    static uint16_t pixels[294U * 42U];
    for (unsigned i = 0; i < 294U * 42U; ++i) {
        pixels[i] = (i & 1U) ? 0x07E0 : 0xF800;
    }
    auto* display = lv_display_get_default();
    lv_display_get_flush_cb(display)(display, &area, reinterpret_cast<uint8_t*>(pixels));
}

/** 验证上电与复位总线一致，DMA 完成/忙状态/超时/回绕，以及两产品偏移和裁剪。 */
int
main() {
    assert(trace.empty() && testSpi.CR1 == 0); // 静态构造未访问硬件。
    lv_init();
    lv_port_disp_init();
    expectTrace(expectedPanelInit(true));
    assert(nowMs == 1800 && board_display_panel.width() == 126 && board_display_panel.height() == 294);
    auto* display = lv_display_get_default();
    assert(lv_display_get_horizontal_resolution(display) == 294 && lv_display_get_vertical_resolution(display) == 126);
    lv_area_t rounded = {3, 5, 8, 10};
    lv_display_send_event(display, LV_EVENT_INVALIDATE_AREA, &rounded);
    assert(rounded.x1 == 2 && rounded.y1 == 4 && rounded.x2 == 9 && rounded.y2 == 11);

    trace.clear();
    flush({0, 0, 293, 41});
    assert(transferCount == (294U - PRODUCT_DISPLAY_X_SHIFT) * 42U * 2U);
    const uint32_t clippedSource = sourceAddress;
    const std::vector<Event> dmaStart = {
        {"spi", DISABLE},         {"channel", DISABLE}, {"tc"},          {"source"},
        {"count", transferCount}, {"channel", ENABLE},  {"spi", ENABLE},
    };
    const std::vector<Event> windowStart = {
        {"gpio", PB14, 0}, {"gpio", PB1, 0}, {"byte", 0x2A},
        {"gpio", PB1, 1},  {"byte", 0},      {"byte", 0},
        {"byte", 0},       {"byte", 41},     {"gpio", PB1, 0},
        {"byte", 0x2B},    {"gpio", PB1, 1}, {"byte", 0},
        {"byte", 0},       {"byte", 1},      {"byte", 0x25 - PRODUCT_DISPLAY_X_SHIFT},
        {"gpio", PB1, 0},  {"byte", 0x2C},   {"gpio", PB1, 1},
        {"gpio", PB1, 1},
    };
    auto expectedStart = windowStart;
    expectedStart.insert(expectedStart.end(), dmaStart.begin(), dmaStart.end());
    expectTrace(expectedStart);

    trace.clear();
    testSpi.SR |= SPI_FLAG_IDLE;
    dmaIrq();
    assert(board_display_spi.dmaDone());
    expectTrace({{"tc"}});
    trace.clear();
    lv_port_disp_poll();
    assert(trace.empty()); // DMA 已结束但最后字节仍在移位，不能释放 CS。
    testSpi.SR &= ~SPI_FLAG_IDLE;
    lv_port_disp_poll();
    expectTrace({{"channel", DISABLE}, {"spi", DISABLE}, {"gpio", PB14, 1}});
    assert(display_dma_timeouts == 0);

    trace.clear();
    flush({0, 0, 1, 1});
    assert(transferCount == 8);
    // 与 v8 交换格式完全一致：旋转后绿色、绿色、红色、红色，RGB565 高字节在前。
    const uint8_t expectedPixels[] = {0x07, 0xE0, 0x07, 0xE0, 0xF8, 0x00, 0xF8, 0x00};
    for (size_t i = 0; i < sizeof(expectedPixels); ++i) {
        assert(dmaPixels[i] == expectedPixels[i]);
    }
    assert(!board_display_spi.dmaDone());
    assert(clippedSource == sourceAddress + PRODUCT_DISPLAY_X_SHIFT * 42U * 2U);
    trace.clear();
    nowMs += 99;
    lv_port_disp_poll();
    assert(trace.empty());
    ++nowMs;
    lv_port_disp_poll();
    auto timeout_trace = std::vector<Event>{{"channel", DISABLE}, {"spi", DISABLE}, {"gpio", PB14, 1}};
#if PRODUCT_DISPLAY_BRIGHTNESS_FADE
    const auto full_brightness = brightnessTrace(0xFF);
    timeout_trace.insert(timeout_trace.end(), full_brightness.begin(), full_brightness.end());
#endif
    expectTrace(timeout_trace);
    assert(display_dma_timeouts == 1);

    trace.clear();
    nowMs = UINT32_MAX - 49U;
    flush({0, 0, 1, 1});
    trace.clear();
    nowMs += 100;
    lv_port_disp_poll();
    expectTrace({{"channel", DISABLE}, {"spi", DISABLE}, {"gpio", PB14, 1}});
    assert(display_dma_timeouts == 2);

    if (PRODUCT_DISPLAY_X_SHIFT != 0) {
        trace.clear();
        flush({292, 0, 293, 1});
        assert(trace.empty()); // X1Pro 右缘整块落到玻璃外。
    }
    trace.clear();
    flush({0, 0, 1, 1});
    trace.clear();
    pollStep = 100;
    lv_port_reset();
    pollStep = 0;
    auto expected = expectedPanelInit(false);
    expected.insert(expected.begin(), {{"channel", DISABLE}, {"spi", DISABLE}, {"gpio", PB14, 1}});
#if PRODUCT_DISPLAY_BRIGHTNESS_FADE
    const auto reset_brightness = brightnessTrace(0xFF);
    /** @brief fill 后 SPI 仍使能，亮度命令不重复使能。 */
    auto reset_trace = reset_brightness;
    reset_trace.erase(reset_trace.begin() + 2);
    expected.insert(expected.end(), reset_trace.begin(), reset_trace.end());
#endif
    expectTrace(expected);
    assert(display_dma_timeouts == 3);

    trace.clear();
    board_display_panel.fill(0x1234);
    size_t pixelStart = trace.size() - 1U - 126U * 294U * 2U;
    for (size_t i = pixelStart; i + 1 < trace.size() - 1; i += 2) {
        assert((trace[i] == Event{"byte", 0x12}) && (trace[i + 1] == Event{"byte", 0x34}));
    }
    trace.clear();
    PanelRm690a0 withBacklight(board_display_spi, {PB14, PB1, PB2, PB4});
    withBacklight.enableBacklight();
    expectTrace({{"mode", PB4, GPIO_OUTPUT * 2}, {"gpio", PB4, 0}});

#if PRODUCT_DISPLAY_BRIGHTNESS_FADE
    /** @brief 首帧未发送时不亮；用实际 LVGL 三块刷新验证最后一块完成才启动。 */
    lv_display_delete(display);
    lv_port_disp_init();
    display = lv_display_get_default();
    trace.clear();
    nowMs += 1500; /** @brief 模拟显示端口初始化后 App_Init 耗时 1.5 s。 */
    lv_port_disp_poll();
    assert(bytes().empty());
    nowMs += 900; /** @brief 首帧较晚完成时，随后渐亮仍须完整运行 300 ms。 */
    completeDmaImmediately = true;
    lv_refr_now(display);
    lv_port_disp_poll();
    completeDmaImmediately = false;
    trace.clear();
    for (uint32_t elapsed = 16; elapsed <= 304; elapsed += 16) {
        nowMs += 16;
        lv_port_disp_poll();
        const uint32_t value = elapsed >= 300 ? 255 : 255U * elapsed * (600U - elapsed) / 90000U;
        assert((bytes() == std::vector<uint32_t>{0x51, value}));
        assert(elapsed >= 300 || (value > 0 && value < 255));
        trace.clear();
    }
    lv_port_disp_poll();
    assert(trace.empty());
    /** @brief 无任何刷新（也无动画/定时器）时，1 s 截止直接全亮。 */
    lv_display_delete(display);
    lv_port_disp_init();
    display = lv_display_get_default();
    trace.clear();
    nowMs += 1500;
    lv_port_disp_poll(); /** @brief 无刷新兜底从首次轮询起算，初始化后的等待不影响截止。 */
    assert(bytes().empty());
    nowMs += 999;
    lv_port_disp_poll();
    assert(bytes().empty());
    ++nowMs;
    lv_port_disp_poll();
    assert((bytes() == std::vector<uint32_t>{0x51, 0xFF}));
#endif

    trace.clear();
    uint8_t byte = 0;
    assert(!board_display_spi.startDmaWrite(nullptr, 1));
    assert(!board_display_spi.startDmaWrite(&byte, 0));
    assert(!board_display_spi.startDmaWrite(&byte, 65536));
    assert(trace.empty());
    static uint8_t maxTransfer[UINT16_MAX];
    assert(board_display_spi.startDmaWrite(maxTransfer, sizeof(maxTransfer)));
    assert(transferCount == UINT16_MAX);
    board_display_spi.abort();
    dmaResult = LL_ERR;
    assert(!board_display_spi.beginDma());
    assert(!board_display_spi.startDmaWrite(&byte, 1));
    dmaResult = LL_OK;
    irqResult = LL_ERR;
    assert(!board_display_spi.beginDma());
    irqResult = LL_OK;
    spiResult = LL_ERR;
    assert(!board_display_panel.init());
    spiResult = LL_OK;

    SpiMaster withoutDma(
        {CM_SPI3, FCG1_PERIPH_SPI3, PA1, PA2, PA3, 17, 18, 19, SPI_BR_CLK_DIV2, SPI_MD_0, SPI_FIRST_MSB, nullptr});
    trace.clear();
    assert(!withoutDma.beginDma() && !withoutDma.startDmaWrite(&byte, 1));
    assert(trace.empty());
    assert(withoutDma.begin());
    expectTrace({{"spi-deinit"},
                 {"mode", PA1, GPIO_OUTPUT_AF_PP * 2},
                 {"mode", PA2, GPIO_OUTPUT_AF_PP * 2},
                 {"mode", PA3, GPIO_OUTPUT_AF_PP * 2},
                 {"function", PA1, 17},
                 {"function", PA2, 18},
                 {"function", PA3, 19},
                 {"fcg1", FCG1_PERIPH_SPI3, ENABLE},
                 {"spi", DISABLE},
                 {"spi-struct"},
                 {"spi-init"}});
    lv_deinit();
}
