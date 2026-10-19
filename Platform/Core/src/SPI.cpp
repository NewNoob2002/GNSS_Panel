#include "SPI.h"

// SPI DMA 发送由显示驱动实现（Libraries/lvgl/porting），只有 begin(true) 的实例会调用。
extern "C" void spi_dma_trans(void* buf, uint16_t len);

SPIClass::SPIClass(CM_SPI_TypeDef* spix, gpio_pin_t sclk, gpio_pin_t mosi, gpio_pin_t miso)
    : SPIx(spix), sclk_pin(sclk), mosi_pin(mosi), miso_pin(miso), enable_dma(false) {
    memset(&spi_init_struct, 0, sizeof(spi_init_struct));
}

void
SPIClass::SPI_Settings(uint32_t master_slave_mode, uint32_t frame_bit_num, uint32_t SPI_MODEx, uint32_t mclk_freq_div,
                       uint32_t first_bit) {
    SPI_Cmd(SPIx, DISABLE);

    uint32_t SPI_MODE;
    switch (SPI_MODEx) {
        case 0: SPI_MODE = SPI_MD_0; break;
        case 1: SPI_MODE = SPI_MD_1; break;
        case 2: SPI_MODE = SPI_MD_2; break;
        case 3: SPI_MODE = SPI_MD_3; break;
        default: return;
    }

    SPI_StructInit(&spi_init_struct);
    spi_init_struct.u32WireMode = SPI_4_WIRE;
    spi_init_struct.u32TransMode = SPI_FULL_DUPLEX;
    spi_init_struct.u32MasterSlave = master_slave_mode;
    spi_init_struct.u32DataBits = frame_bit_num;
    spi_init_struct.u32SpiMode = SPI_MODE;
    spi_init_struct.u32BaudRatePrescaler = mclk_freq_div;
    spi_init_struct.u32FirstBit = first_bit;
    spi_init_struct.u32FrameLevel = SPI_1_FRAME;
    SPI_Init(SPIx, &spi_init_struct);

    //    SPI_Cmd(SPIx, ENABLE);
}

void
SPIClass::begin(void) {
    SPI_DeInit(SPIx);

    pinMode(sclk_pin, OUTPUT_AF_PP);
    pinMode(mosi_pin, OUTPUT_AF_PP);
    if (miso_pin != -1) {
        pinMode(miso_pin, OUTPUT_AF_PP);
    }

    uint16_t sclk_function = GPIO_FUNC_0;
    uint16_t mosi_function = GPIO_FUNC_0;
    uint16_t miso_function = GPIO_FUNC_0;
    if (SPIx == CM_SPI1 || SPIx == CM_SPI3) {
        sclk_function = GPIO_FUNC_43;
        mosi_function = GPIO_FUNC_40;
        miso_function = GPIO_FUNC_41;
        if (SPIx == CM_SPI1) {
            SPI_UNIT_CLOCK = FCG1_PERIPH_SPI1;
        } else {
            SPI_UNIT_CLOCK = FCG1_PERIPH_SPI3;
        }
    } else if (SPIx == CM_SPI2 || SPIx == CM_SPI4) {
        sclk_function = GPIO_FUNC_47;
        mosi_function = GPIO_FUNC_44;
        miso_function = GPIO_FUNC_45;
        if (SPIx == CM_SPI2) {
            SPI_UNIT_CLOCK = FCG1_PERIPH_SPI2;
        } else {
            SPI_UNIT_CLOCK = FCG1_PERIPH_SPI4;
        }
    }
    GPIO_SetFunction(sclk_pin, sclk_function);
    GPIO_SetFunction(mosi_pin, mosi_function);
    if (miso_pin != -1) {
        GPIO_SetFunction(miso_pin, miso_function);
    }

    FCG_Fcg1PeriphClockCmd(SPI_UNIT_CLOCK, ENABLE);
    SPI_Settings(SPI_MASTER, SPI_DATA_SIZE_8BIT, SPI_MODE0, SPI_BR_CLK_DIV2, SPI_FIRST_MSB);
}

void
SPIClass::begin(bool enable_dma) {
    this->enable_dma = enable_dma;
    begin();
}

void
SPIClass::begin(uint32_t clock, uint16_t dataOrder, uint16_t dataMode) {
    begin();
    setClock(clock);
    setBitOrder(dataOrder);
    setDataMode(dataMode);
    SPI_Cmd(SPIx, ENABLE);
}

void
SPIClass::begin(SPISettings settings) {
    begin();
    setClock(settings.clock);
    setBitOrder(settings.bitOrder);
    setDataMode(settings.dataMode);
    SPI_Cmd(SPIx, ENABLE);
}

void
SPIClass::beginSlave(void) {
    begin();
    //    SPI_Settings(
    //        SPI_MODE_SLAVE,
    //        SPI_FRAME_8BIT,
    //        SPI_MODE0,
    //        SPI_CS_SOFTWARE_MODE,
    //        SPI_MCLK_DIV_16,
    //        SPI_FIRST_BIT_MSB
    //    );
    //    spi_enable(SPIx, TRUE);
}

void
SPIClass::end(void) {
    SPI_Cmd(SPIx, DISABLE);
}

void
SPIClass::setClock(uint32_t clock) {
    if (clock == 0) {
        return;
    }

    //    static const spi_mclk_freq_div_type mclk_freq_div_map[] =
    //    {
    //        SPI_MCLK_DIV_2,
    //        SPI_MCLK_DIV_2,
    //        SPI_MCLK_DIV_4,
    //        SPI_MCLK_DIV_8,
    //        SPI_MCLK_DIV_16,
    //        SPI_MCLK_DIV_32,
    //        SPI_MCLK_DIV_64,
    //        SPI_MCLK_DIV_128,
    //        SPI_MCLK_DIV_256,
    //        SPI_MCLK_DIV_512,
    //        SPI_MCLK_DIV_1024,
    //    };
    //    const uint8_t mapSize = sizeof(mclk_freq_div_map) / sizeof(mclk_freq_div_map[0]);
    //    uint32_t clockDiv = SPI_Clock / clock;
    //    uint8_t mapIndex = 0;

    //    while(clockDiv > 1)
    //    {
    //        clockDiv = clockDiv >> 1;
    //        mapIndex++;
    //    }

    //    if(mapIndex >= mapSize)
    //    {
    //        mapIndex = mapSize - 1;
    //    }

    //    spi_init_struct.mclk_freq_division = mclk_freq_div_map[mapIndex];
    //    spi_init(SPIx, &spi_init_struct);
    //    spi_enable(SPIx, TRUE);
}

void
SPIClass::setClockDivider(uint32_t Div) {
    ((void)Div);
    //    if(Div == 0)
    //    {
    //        Div = 1;
    //    }
    // #if SPI_CLASS_AVR_COMPATIBILITY_MODE
    //    setClock(16000000 / Div); // AVR:16MHz
    // #else
    //    setClock(SPI_Clock / Div);
    // #endif
}

void
SPIClass::setBitOrder(uint16_t bitOrder) {
    ((void)bitOrder);
    //    spi_init_struct.first_bit_transmission = (bitOrder == MSBFIRST) ? SPI_FIRST_BIT_MSB : SPI_FIRST_BIT_LSB;
    //    spi_init(SPIx, &spi_init_struct);
    //    spi_enable(SPIx, TRUE);
}

/*  Victor Perez. Added to test changing datasize from 8 to 16 bit modes on the fly.
 *   Input parameter should be SPI_CR1_DFF set to 0 or 1 on a 32bit word.
 *
 */
void
SPIClass::setDataSize(uint32_t datasize) {
    ((void)datasize);
    //    spi_init_struct.frame_bit_num = (spi_frame_bit_num_type)datasize;
    //    spi_init(SPIx, &spi_init_struct);
    //    spi_enable(SPIx, TRUE);
}

void
SPIClass::setDataMode(uint8_t dataMode) {
    ((void)dataMode);
    /* Notes.  As far as I can tell, the AVR numbers for dataMode appear to match the numbers required by the STM32

    From the AVR doc http://www.atmel.com/images/doc2585.pdf section 2.4

    SPI Mode    CPOL    CPHA    Shift SCK-edge  Capture SCK-edge
    0           0       0       Falling         Rising
    1           0       1       Rising          Falling
    2           1       0       Rising          Falling
    3           1       1       Falling         Rising


    On the STM32 it appears to be

    bit 1 - CPOL : Clock polarity
        (This bit should not be changed when communication is ongoing)
        0 : CLK to 0 when idle
        1 : CLK to 1 when idle

    bit 0 - CPHA : Clock phase
        (This bit should not be changed when communication is ongoing)
        0 : The first clock transition is the first data capture edge
        1 : The second clock transition is the first data capture edge

    If someone finds this is not the case or sees a logic error with this let me know ;-)
     */
    //    spi_clock_polarity_type clock_polarity;
    //    spi_clock_phase_type clock_phase;
    //    spi_enable(SPIx, FALSE);

    //    switch(dataMode)
    //    {
    //    case 0:
    //        clock_polarity = SPI_CLOCK_POLARITY_LOW;
    //        clock_phase = SPI_CLOCK_PHASE_1EDGE;
    //        break;
    //    case 1:
    //        clock_polarity = SPI_CLOCK_POLARITY_LOW;
    //        clock_phase = SPI_CLOCK_PHASE_2EDGE;
    //        break;
    //    case 2:
    //        clock_polarity = SPI_CLOCK_POLARITY_HIGH;
    //        clock_phase = SPI_CLOCK_PHASE_1EDGE;
    //        break;
    //    case 3:
    //        clock_polarity = SPI_CLOCK_POLARITY_HIGH;
    //        clock_phase = SPI_CLOCK_PHASE_2EDGE;
    //        break;
    //    default:
    //        return;
    //    }

    //    spi_init_struct.clock_polarity = clock_polarity;
    //    spi_init_struct.clock_phase = clock_phase;
    //    spi_init(SPIx, &spi_init_struct);
    //    spi_enable(SPIx, TRUE);
}

void
SPIClass::beginTransaction(SPISettings settings) {
    SPISettings(settings.clock, settings.bitOrder, settings.dataMode);

    setClock(settings.clock);
    setBitOrder(settings.bitOrder);
    setDataMode(settings.dataMode);
    setDataSize(settings.dataSize);

    SPI_Cmd(SPIx, ENABLE);
}

void
SPIClass::beginTransactionSlave(void) {
    beginSlave();
}

void
SPIClass::endTransaction(void) {
    SPI_Cmd(SPIx, DISABLE);
}

uint16_t
SPIClass::read(void) {
    SPI_I2S_WAIT_RX(SPIx);
    return (uint16_t)(SPI_I2S_RXDATA(SPIx));
}

void
SPIClass::read(uint8_t* buf, uint32_t len) {
    if (len == 0) {
        return;
    }

    SPI_I2S_RXDATA_VOLATILE(SPIx);
    (void(vn));
    SPI_I2S_TXDATA(SPIx, 0x00FF);

    while ((--len)) {
        SPI_I2S_WAIT_TX(SPIx);
        noInterrupts();
        SPI_I2S_TXDATA(SPIx, 0x00FF);
        SPI_I2S_WAIT_RX(SPIx);
        *buf++ = (uint8_t)SPI_I2S_RXDATA(SPIx);
        interrupts();
    }
    SPI_I2S_WAIT_RX(SPIx);
    *buf++ = (uint8_t)SPI_I2S_RXDATA(SPIx);
}

void
SPIClass::write(uint8_t data) {
    if (enable_dma) {
        spi_dma_trans(&data, 1);
    } else {
        if (!(SPIx->CR1 & SPI_CR1_SPE)) {
            SPI_Cmd(SPIx, ENABLE);
        }
        SPI_I2S_TXDATA(SPIx, data);
        SPI_I2S_WAIT_TX(SPIx);
        SPI_I2S_WAIT_BUSY(SPIx);
    }
}

void
SPIClass::write(uint8_t* data, uint32_t length) {
    if (enable_dma) {
        spi_dma_trans(data, length);
    } else {
        if (!(SPIx->CR1 & SPI_CR1_SPE)) {
            SPI_Cmd(SPIx, ENABLE);
        }
        while (length--) {
            SPI_I2S_WAIT_TX(SPIx);
            SPI_I2S_TXDATA(SPIx, *data++);
        }
        SPI_I2S_WAIT_TX(SPIx);
        SPI_I2S_WAIT_BUSY(SPIx);
    }
}

void
SPIClass::write(uint16_t* data, uint32_t length) {
    if (enable_dma) {
        spi_dma_trans(data, length);
    } else {
        if (!(SPIx->CR1 & SPI_CR1_SPE)) {
            SPI_Cmd(SPIx, ENABLE);
        }
        while (length--) {
            SPI_I2S_WAIT_TX(SPIx);
            SPI_I2S_TXDATA(SPIx, *data++);
        }
        SPI_I2S_WAIT_TX(SPIx);
        SPI_I2S_WAIT_BUSY(SPIx);
    }
}

uint8_t
SPIClass::transfer(uint8_t wr_data) const {

    if (enable_dma) {
        spi_dma_trans(&wr_data, 1);
    } else {
        if (!(SPIx->CR1 & SPI_CR1_SPE)) {
            SPI_Cmd(SPIx, ENABLE);
        }
        SPI_I2S_RXDATA_VOLATILE(SPIx);
        (void(vn));
        SPI_I2S_TXDATA(SPIx, wr_data);
        SPI_I2S_WAIT_TX(SPIx);
        SPI_I2S_WAIT_BUSY(SPIx);
        return (uint8_t)SPI_I2S_RXDATA(SPIx);
    }
    return wr_data;
}

uint16_t
SPIClass::transfer16(uint16_t wr_data) const {
    if (!(SPIx->CR1 & SPI_CR1_SPE)) {
        SPI_Cmd(SPIx, ENABLE);
    }
    SPI_I2S_RXDATA_VOLATILE(SPIx);
    (void(vn));
    SPI_I2S_TXDATA(SPIx, wr_data);
    SPI_I2S_WAIT_TX(SPIx);
    SPI_I2S_WAIT_BUSY(SPIx);
    return (uint16_t)SPI_I2S_RXDATA(SPIx);
}

uint8_t
SPIClass::transfer(uint8_t* data, uint32_t length) const {
    if (enable_dma) {
        spi_dma_trans(data, length);
    } else {
        if (!(SPIx->CR1 & SPI_CR1_SPE)) {
            SPI_Cmd(SPIx, ENABLE);
        }
        while (length--) {
            SPI_I2S_WAIT_TX(SPIx);
            SPI_I2S_TXDATA(SPIx, *data++);
        }
        SPI_I2S_WAIT_TX(SPIx);
        SPI_I2S_WAIT_BUSY(SPIx);
    }
    return length;
}

uint8_t
SPIClass::send(uint8_t data) {
    this->write(data);
    return 1;
}

uint8_t
SPIClass::send(uint8_t* buf, uint32_t len) {
    this->write(buf, len);
    return len;
}

uint8_t
SPIClass::recv(void) {
    return this->read();
}

#if SPI_CLASS_1_ENABLE
SPIClass SPI(SPI_CLASS_1_SPI);
#endif

#if SPI_CLASS_2_ENABLE
SPIClass SPI_2(SPI_CLASS_2_SPI);
#endif

#if SPI_CLASS_3_ENABLE
SPIClass SPI_3(SPI_CLASS_3_SPI, CONFIG_SCREEN_SCK_PIN, CONFIG_SCREEN_MOSI_PIN);
#endif
