#ifndef __SPI_H
#define __SPI_H

#include "Arduino.h"

#ifndef LSBFIRST
#define LSBFIRST 0
#endif
#ifndef MSBFIRST
#define MSBFIRST 1
#endif

#if SPI_CLASS_PIN_DEFINE_ENABLE
#define SS   PA4
#define SCK  PA5
#define MISO PA6
#define MOSI PA7
#endif

#define DATA_SIZE_8BIT                       SPI_DATA_SIZE_8BIT
#define DATA_SIZE_16BIT                      SPI_DATA_SIZE_16BIT

#define SPI_I2S_GET_FLAG(spix, SPI_I2S_FLAG) (spix->SR & SPI_I2S_FLAG)
#define SPI_I2S_RXDATA(spix)                 (spix->DR)
#define SPI_I2S_RXDATA_VOLATILE(spix)        volatile uint16_t vn = SPI_I2S_RXDATA(spix)
#define SPI_I2S_TXDATA(spix, data)           (spix->DR = (data))
#define SPI_I2S_WAIT_RX(spix)                                                                                          \
    do {                                                                                                               \
        while (!SPI_I2S_GET_FLAG(spix, SPI_FLAG_RX_BUF_FULL))                                                          \
            ;                                                                                                          \
    } while (0)
#define SPI_I2S_WAIT_TX(spix)                                                                                          \
    do {                                                                                                               \
        while (!SPI_I2S_GET_FLAG(spix, SPI_FLAG_TX_BUF_EMPTY))                                                         \
            ;                                                                                                          \
    } while (0)
#define SPI_I2S_WAIT_BUSY(spix)                                                                                        \
    do {                                                                                                               \
        while (SPI_I2S_GET_FLAG(spix, SPI_FLAG_IDLE))                                                                  \
            ;                                                                                                          \
    } while (0)

#define SPI_I2S_WAIT_IDLE(spix)                                                                                        \
    do {                                                                                                               \
        while (!SPI_I2S_GET_FLAG(spix, SPI_FLAG_IDLE))                                                                 \
            ;                                                                                                          \
    } while (0)

typedef enum { SPI_MODE0, SPI_MODE1, SPI_MODE2, SPI_MODE3 } SPI_MODE_TypeDef;

typedef enum { SPI_STATE_IDLE, SPI_STATE_READY, SPI_STATE_RECEIVE, SPI_STATE_TRANSMIT, SPI_STATE_TRANSFER } spi_mode_t;

class SPISettings {
  public:
    SPISettings(uint32_t clock, uint16_t bitOrder, uint8_t dataMode) {
        init_AlwaysInline(clock, bitOrder, dataMode, DATA_SIZE_8BIT);
    }

    SPISettings(uint32_t clock, uint16_t bitOrder, uint8_t dataMode, uint32_t dataSize) {
        init_AlwaysInline(clock, bitOrder, dataMode, dataSize);
    }

    SPISettings(uint32_t clock) { init_AlwaysInline(clock, MSBFIRST, SPI_MODE0, DATA_SIZE_8BIT); }

    SPISettings() { init_AlwaysInline(4000000, MSBFIRST, SPI_MODE0, DATA_SIZE_8BIT); }

  private:
    void
    init_MightInline(uint32_t clock, uint16_t bitOrder, uint8_t dataMode, uint32_t dataSize) {
        init_AlwaysInline(clock, bitOrder, dataMode, dataSize);
    }

    void
    init_AlwaysInline(uint32_t clock, uint16_t bitOrder, uint8_t dataMode, uint32_t dataSize)
        __attribute__((__always_inline__)) {
        this->clock = clock;
        this->bitOrder = bitOrder;
        this->dataMode = dataMode;
        this->dataSize = dataSize;
    }

    uint32_t clock;
    uint16_t bitOrder;
    uint8_t dataMode;
    uint32_t dataSize;

    friend class SPIClass;
};

class SPIClass {
  public:
    SPIClass(CM_SPI_TypeDef* spix, gpio_pin_t sclk, gpio_pin_t mosi, gpio_pin_t miso = -1);
    void SPI_Settings(uint32_t master_slave_mode, uint32_t frame_bit_num, uint32_t SPI_MODEx, uint32_t mclk_freq_div,
                      uint32_t first_bit);
    void begin(void);
    void begin(bool enable_dma);
    void begin(uint32_t clock, uint16_t dataOrder, uint16_t dataMode);
    void begin(SPISettings settings);
    void beginSlave(uint32_t bitOrder, uint32_t mode);
    void beginSlave(void);
    void beginTransactionSlave(void);
    void beginTransaction(SPISettings settings);

    void endTransaction(void);
    void end(void);

    void setClock(uint32_t clock);
    void setClockDivider(uint32_t Div);
    void setBitOrder(uint16_t bitOrder);
    void setDataMode(uint8_t dataMode);
    void setDataSize(uint32_t datasize);

    uint16_t read(void);
    void read(uint8_t* buffer, uint32_t length);
    void write(uint8_t data);
    void write(uint8_t* data, uint32_t length);
    void write(uint16_t* data, uint32_t length);
    uint8_t transfer(uint8_t data) const;
    uint16_t transfer16(uint16_t data) const;
    uint8_t transfer(uint8_t* data, uint32_t length) const;
    uint8_t send(uint8_t data);
    uint8_t send(uint8_t* data, uint32_t length);
    uint8_t recv(void);

    CM_SPI_TypeDef*
    getSPI() {
        return SPIx;
    }

  private:
    CM_SPI_TypeDef* SPIx;
    stc_spi_init_t spi_init_struct;
    uint32_t SPI_UNIT_CLOCK;

    gpio_pin_t sclk_pin;
    gpio_pin_t mosi_pin;
    gpio_pin_t miso_pin;

    bool enable_dma;
};

#if SPI_CLASS_1_ENABLE
extern SPIClass SPI;
#endif

#if SPI_CLASS_2_ENABLE
extern SPIClass SPI_2;
#endif

#if SPI_CLASS_3_ENABLE
extern SPIClass SPI_3;
#endif

#endif //__SPI_H