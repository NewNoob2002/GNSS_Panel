#include <cassert>
#include <string>
#include "GpioPin.h"

static stc_gpio_init_t lastConfig;
static uint8_t lastPort;
static uint16_t lastMask;
static bool level;
static std::string trace;

/** 记录端口和位掩码，以便检查引脚表寻址。 */
static void
capture(uint8_t port, uint16_t mask, char event) {
    lastPort = port;
    lastMask = mask;
    trace += event;
}

/** 模拟 hc32_ll_gpio.c 的默认字段，保留所有未由模式修改的配置。 */
int32_t
GPIO_StructInit(stc_gpio_init_t* config) {
    *config = {};
    trace += 'C';
    return LL_OK;
}

/** 在一次调用内接收初始电平和方向，检测额外电平写入。 */
int32_t
GPIO_Init(uint8_t port, uint16_t mask, const stc_gpio_init_t* config) {
    capture(port, mask, 'I');
    lastConfig = *config;
    level = config->u16PinState != 0;
    return LL_OK;
}

/** 读取独立的输入电平。 */
en_pin_state_t
GPIO_ReadInputPins(uint8_t port, uint16_t mask) {
    capture(port, mask, 'R');
    return level ? PIN_SET : PIN_RESET;
}

/** 模拟 GPIO 置高。 */
void
GPIO_SetPins(uint8_t port, uint16_t mask) {
    capture(port, mask, 'H');
    level = true;
}

/** 模拟 GPIO 置低。 */
void
GPIO_ResetPins(uint8_t port, uint16_t mask) {
    capture(port, mask, 'L');
    level = false;
}

/** 模拟 GPIO 输出翻转。 */
void
GPIO_TogglePins(uint8_t port, uint16_t mask) {
    capture(port, mask, 'T');
    level = !level;
}

/** 核对通过引脚表进行的复用设置。 */
void
GPIO_SetFunc(uint8_t port, uint16_t mask, uint16_t function) {
    capture(port, mask, 'F');
    assert(function == 51);
}

/** 比对原数字 IO 实现的所有保留模式、默认电平及类包装行为。 */
int
main() {
    struct ModeCase {
        gpio_mode_t mode;
        uint16_t dir, drive, pull, attr, type;
    };

    const ModeCase cases[] = {
        {GPIO_INPUT, PIN_DIR_IN, PIN_LOW_DRV, PIN_PU_OFF, PIN_ATTR_DIGITAL, PIN_OUT_TYPE_CMOS},
        {GPIO_INPUT_PULLUP, PIN_DIR_IN, PIN_LOW_DRV, PIN_PU_ON, PIN_ATTR_DIGITAL, PIN_OUT_TYPE_CMOS},
        {GPIO_INPUT_ANALOG, PIN_DIR_IN, PIN_LOW_DRV, PIN_PU_OFF, PIN_ATTR_ANALOG, PIN_OUT_TYPE_CMOS},
        {GPIO_OUTPUT, PIN_DIR_OUT, PIN_HIGH_DRV, PIN_PU_ON, PIN_ATTR_DIGITAL, PIN_OUT_TYPE_CMOS},
        {GPIO_OUTPUT_OPEN_DRAIN, PIN_DIR_OUT, PIN_LOW_DRV, PIN_PU_OFF, PIN_ATTR_DIGITAL, PIN_OUT_TYPE_NMOS},
        {GPIO_OUTPUT_AF_PP, PIN_DIR_IN, PIN_HIGH_DRV, PIN_PU_OFF, PIN_ATTR_DIGITAL, PIN_OUT_TYPE_CMOS},
        {GPIO_OUTPUT_AF_OD, PIN_DIR_IN, PIN_HIGH_DRV, PIN_PU_OFF, PIN_ATTR_DIGITAL, PIN_OUT_TYPE_CMOS},
    };
    constexpr GpioPin power(PB3);
    for (const auto& test : cases) {
        for (bool initial : {false, true}) {
            trace.clear();
            power.setMode(test.mode, initial);
            assert(trace == "CI" && lastPort == GPIO_PORT_B && lastMask == (1U << 3));
            assert(lastConfig.u16PinState == initial);
            assert(lastConfig.u16PinDir == test.dir && lastConfig.u16PinDrv == test.drive);
            assert(lastConfig.u16PullUp == test.pull && lastConfig.u16PinAttr == test.attr);
            assert(lastConfig.u16PinOutputType == test.type);
            assert(!lastConfig.u16Latch && !lastConfig.u16Invert && !lastConfig.u16ExtInt);
        }
    }
    trace.clear();
    gpio_set_mode(PB3, static_cast<gpio_mode_t>(7), GPIO_LOW);
    assert(trace == "C");
    trace.clear();
    power.setMode(GPIO_OUTPUT);
    assert(!power.read());
    power.write(true);
    assert(power.read());
    power.toggle();
    assert(!power.read());
    gpio_write(PB3, 2U);
    assert(!power.read());
    assert(trace == "CIRHRTRLR");

    GPIO_SetFunction(PA9, 51);
    assert(lastPort == GPIO_PORT_A && lastMask == (1U << 9));
    // 全表的端口/位号与枚举保持一一对应；ADC/TimerA 表内容在迁移时与 HEAD 逐字核验。
    for (int pin = 0; pin < BOARD_NR_GPIO_PINS; ++pin) {
        const bool portH = pin >= PH0;
        const int expectedPort = portH ? GPIO_PORT_H : pin / 16;
        const int expectedBit = portH ? pin - PH0 : pin % 16;
        assert(PIN_MAP[pin].port == expectedPort && PIN_MAP[pin].bit_pos == expectedBit);
    }
}
