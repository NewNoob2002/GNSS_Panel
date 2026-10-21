#include "SpiMaster.h"
#include <gpio.h>

bool
SpiMaster::begin() {
    // 顺序和字段取自原 SPI.cpp 的 begin / SPI_Settings。
    SPI_DeInit(config.regs);
    gpio_set_mode(config.sck, GPIO_OUTPUT_AF_PP, GPIO_LOW);
    gpio_set_mode(config.mosi, GPIO_OUTPUT_AF_PP, GPIO_LOW);
    if (config.miso != -1) {
        gpio_set_mode(config.miso, GPIO_OUTPUT_AF_PP, GPIO_LOW);
    }
    GPIO_SetFunction(config.sck, config.sckFunc);
    GPIO_SetFunction(config.mosi, config.mosiFunc);
    if (config.miso != -1) {
        GPIO_SetFunction(config.miso, config.misoFunc);
    }
    FCG_Fcg1PeriphClockCmd(config.fcg1Clock, ENABLE);
    SPI_Cmd(config.regs, DISABLE);

    stc_spi_init_t spiInit;
    (void)SPI_StructInit(&spiInit);
    spiInit.u32WireMode = SPI_4_WIRE;
    spiInit.u32TransMode = SPI_FULL_DUPLEX;
    spiInit.u32MasterSlave = SPI_MASTER;
    spiInit.u32DataBits = SPI_DATA_SIZE_8BIT;
    spiInit.u32SpiMode = config.mode;
    spiInit.u32BaudRatePrescaler = config.clockDivider;
    spiInit.u32FirstBit = config.firstBit;
    spiInit.u32FrameLevel = SPI_1_FRAME;
    return SPI_Init(config.regs, &spiInit) == LL_OK;
}

bool
SpiMaster::beginDma() {
    // 字段与调用顺序取自原 lv_port_disp.cpp 的 lv_port_disp_init。
    dmaReady = false;
    if (config.txDma == nullptr) {
        return false;
    }
    const DmaConfig& dma = *config.txDma;
    stc_dma_init_t dmaInit;
    FCG_Fcg0PeriphClockCmd(dma.fcg0Clock, ENABLE);
    (void)DMA_StructInit(&dmaInit);
    dmaInit.u32IntEn = DMA_INT_ENABLE;
    dmaInit.u32BlockSize = 1UL;
    dmaInit.u32TransCount = 65535;
    dmaInit.u32DataWidth = DMA_DATAWIDTH_8BIT;
    dmaInit.u32SrcAddrInc = DMA_SRC_ADDR_INC;
    dmaInit.u32DestAddrInc = DMA_DEST_ADDR_FIX;
    dmaInit.u32SrcAddr = 0;
    dmaInit.u32DestAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&config.regs->DR));
    if (DMA_Init(dma.regs, dma.channel, &dmaInit) != LL_OK) {
        return false;
    }
    AOS_SetTriggerEventSrc(dma.triggerChannel, dma.triggerSource);

    stc_irq_signin_config_t irqConfig;
    irqConfig.enIntSrc = dma.interruptSource;
    irqConfig.enIRQn = dma.irq;
    irqConfig.pfnCallback = dma.callback;
    if (INTC_IrqSignIn(&irqConfig) != LL_OK) {
        return false;
    }
    NVIC_ClearPendingIRQ(irqConfig.enIRQn);
    NVIC_SetPriority(irqConfig.enIRQn, dma.priority);
    NVIC_EnableIRQ(irqConfig.enIRQn);
    DMA_Cmd(dma.regs, ENABLE);
    DMA_ChCmd(dma.regs, dma.channel, DISABLE);
    dmaReady = true;
    return true;
}

void
SpiMaster::writeByte(uint8_t data) {
    // 保留原 SPI.cpp transfer 的 DR 读取与 TX/IDLNF 等待；不能用 RX ready 代替移位完成。
    if ((config.regs->CR1 & SPI_CR1_SPE) == 0U) {
        SPI_Cmd(config.regs, ENABLE);
    }
    volatile uint16_t received = config.regs->DR;
    (void)received;
    config.regs->DR = data;
    while ((config.regs->SR & SPI_FLAG_TX_BUF_EMPTY) == 0U) {}
    while (!isIdle()) {}
    (void)config.regs->DR;
}

void
SpiMaster::write(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        writeByte(data[i]);
    }
}

bool
SpiMaster::startDmaWrite(const void* data, size_t len) {
    if (!dmaReady || data == nullptr || len == 0 || len > UINT16_MAX) {
        return false;
    }
    const DmaConfig& dma = *config.txDma;
    transferDone = false;
    // 逐行保留原 lv_port_disp.cpp spi_dma_trans 的硬件启动顺序。
    if (config.regs->CR1 & SPI_CR1_SPE) {
        SPI_Cmd(config.regs, DISABLE);
    }
    DMA_ChCmd(dma.regs, dma.channel, DISABLE);
    DMA_ClearTransCompleteStatus(dma.regs, dma.completeFlag);
    DMA_SetSrcAddr(dma.regs, dma.channel, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data)));
    DMA_SetTransCount(dma.regs, dma.channel, static_cast<uint16_t>(len));
    DMA_ChCmd(dma.regs, dma.channel, ENABLE);
    SPI_Cmd(config.regs, ENABLE);
    return true;
}

bool
SpiMaster::dmaDone() const {
    return transferDone;
}

bool
SpiMaster::isIdle() const {
    return (config.regs->SR & SPI_FLAG_IDLE) == 0U;
}

void
SpiMaster::abort() {
    if (config.txDma != nullptr) {
        DMA_ChCmd(config.txDma->regs, config.txDma->channel, DISABLE);
    }
    SPI_Cmd(config.regs, DISABLE);
}

void
SpiMaster::handleDmaIrq() {
    DMA_ClearTransCompleteStatus(config.txDma->regs, config.txDma->completeFlag);
    transferDone = true;
}
