#pragma once
#include <stdint.h>

/** 拦截 DR 写入以记录线上字节；读取保持原 transfer 的寄存器访问形状。 */
struct FakeSpiData {
    void operator=(uint32_t value);
    operator uint32_t() const;
};

struct CM_SPI_TypeDef {
    uint32_t CR1 = 0;
    uint32_t SR = 1;
    FakeSpiData DR;
};

struct CM_DMA_TypeDef {};

extern CM_SPI_TypeDef testSpi;
extern CM_DMA_TypeDef testDma;
#define CM_SPI3 (&testSpi)
#define CM_DMA1 (&testDma)

enum en_functional_state_t { DISABLE, ENABLE };

enum en_event_src_t { EVT_SRC_SPI3_SPTI = 37 };

enum en_int_src_t { INT_SRC_DMA1_TC0 = 45 };

enum IRQn_Type { INT009_IRQn = 9 };

constexpr int LL_OK = 0;
constexpr int LL_ERR = -1;
constexpr uint32_t SPI_CR1_SPE = 1;
constexpr uint32_t SPI_FLAG_TX_BUF_EMPTY = 1;
constexpr uint32_t SPI_FLAG_IDLE = 2;
constexpr uint32_t FCG1_PERIPH_SPI3 = 3;
constexpr uint32_t FCG0_PERIPH_DMA1 = 4;
constexpr uint32_t FCG0_PERIPH_AOS = 8;
constexpr uint32_t GPIO_FUNC_43 = 43;
constexpr uint32_t GPIO_FUNC_40 = 40;
constexpr uint32_t GPIO_FUNC_41 = 41;
constexpr uint32_t SPI_BR_CLK_DIV2 = 2;
constexpr uint32_t SPI_MD_0 = 0;
constexpr uint32_t SPI_FIRST_MSB = 1;
constexpr uint32_t SPI_4_WIRE = 4;
constexpr uint32_t SPI_FULL_DUPLEX = 5;
constexpr uint32_t SPI_MASTER = 6;
constexpr uint32_t SPI_DATA_SIZE_8BIT = 8;
constexpr uint32_t SPI_1_FRAME = 1;
constexpr uint32_t DMA_CH0 = 0;
constexpr uint32_t DMA_FLAG_TC_CH0 = 1;
constexpr uint32_t AOS_DMA1_0 = 7;
constexpr uint32_t DDL_IRQ_PRIO_04 = 4;
constexpr uint32_t DMA_INT_ENABLE = 1;
constexpr uint32_t DMA_DATAWIDTH_8BIT = 8;
constexpr uint32_t DMA_SRC_ADDR_INC = 1;
constexpr uint32_t DMA_DEST_ADDR_FIX = 0;

struct stc_spi_init_t {
    uint32_t u32WireMode, u32TransMode, u32MasterSlave, u32DataBits, u32SpiMode;
    uint32_t u32BaudRatePrescaler, u32FirstBit, u32FrameLevel;
};

struct stc_dma_init_t {
    uint32_t u32IntEn, u32BlockSize, u32TransCount, u32DataWidth, u32SrcAddrInc, u32DestAddrInc;
    uint32_t u32SrcAddr, u32DestAddr;
};

struct stc_irq_signin_config_t {
    en_int_src_t enIntSrc;
    IRQn_Type enIRQn;
    void (*pfnCallback)();
};

void SPI_DeInit(CM_SPI_TypeDef* regs);
void SPI_Cmd(CM_SPI_TypeDef* regs, en_functional_state_t state);
int SPI_StructInit(stc_spi_init_t* config);
int SPI_Init(CM_SPI_TypeDef* regs, const stc_spi_init_t* config);
void FCG_Fcg1PeriphClockCmd(uint32_t clock, en_functional_state_t state);
void FCG_Fcg0PeriphClockCmd(uint32_t clock, en_functional_state_t state);
int DMA_StructInit(stc_dma_init_t* config);
int DMA_Init(CM_DMA_TypeDef* regs, uint8_t channel, const stc_dma_init_t* config);
void DMA_Cmd(CM_DMA_TypeDef* regs, en_functional_state_t state);
void DMA_ChCmd(CM_DMA_TypeDef* regs, uint8_t channel, en_functional_state_t state);
void DMA_ClearTransCompleteStatus(CM_DMA_TypeDef* regs, uint32_t flag);
void DMA_SetSrcAddr(CM_DMA_TypeDef* regs, uint8_t channel, uint32_t address);
void DMA_SetTransCount(CM_DMA_TypeDef* regs, uint8_t channel, uint16_t count);
void AOS_SetTriggerEventSrc(uint32_t channel, en_event_src_t source);
int INTC_IrqSignIn(const stc_irq_signin_config_t* config);
void NVIC_ClearPendingIRQ(IRQn_Type irq);
void NVIC_SetPriority(IRQn_Type irq, uint32_t priority);
void NVIC_EnableIRQ(IRQn_Type irq);
