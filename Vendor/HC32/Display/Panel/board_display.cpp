#include "board_display.h"
#include "mcu_config.h"

/** DMA1 CH0 板级中断跳板；实例只清完成标志并发布完成状态。 */
static void
displayDmaIrq() {
    board_display_spi.handleDmaIrq();
}

// 配置依据：原 lv_port_disp_init / spi_dma_trans 及 SPI.cpp::begin；构造不访问硬件。
static const SpiMaster::DmaConfig displayDma = {
    CM_DMA1,          DMA_CH0,     FCG0_PERIPH_DMA1 | FCG0_PERIPH_AOS,
    DMA_FLAG_TC_CH0,  AOS_DMA1_0,  EVT_SRC_SPI3_SPTI,
    INT_SRC_DMA1_TC0, INT009_IRQn, DDL_IRQ_PRIO_04,
    &displayDmaIrq,
};

SpiMaster board_display_spi({CM_SPI3, FCG1_PERIPH_SPI3, CONFIG_SCREEN_SCK_PIN, CONFIG_SCREEN_MOSI_PIN, -1, GPIO_FUNC_43,
                             GPIO_FUNC_40, GPIO_FUNC_41, SPI_BR_CLK_DIV2, SPI_MD_0, SPI_FIRST_MSB, &displayDma});

#if defined(CONFIG_SCREEN_BLK_PIN)
static constexpr gpio_pin_t displayBacklight = CONFIG_SCREEN_BLK_PIN;
#else
static constexpr gpio_pin_t displayBacklight = -1;
#endif

PanelRm690a0 board_display_panel(board_display_spi,
                                 {CONFIG_SCREEN_CS_PIN, CONFIG_SCREEN_DC_PIN, CONFIG_SCREEN_RST_PIN, displayBacklight});
