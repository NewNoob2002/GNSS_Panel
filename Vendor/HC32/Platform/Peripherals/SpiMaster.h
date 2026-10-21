#pragma once

#include <stddef.h>
#include <stdint.h>
#include "pinmap.h"

/** SPI 主机；无动态分配，不可重入；主循环持有总线，ISR 只发布 DMA 完成标志。 */
class SpiMaster {
  public:
    struct DmaConfig {
        CM_DMA_TypeDef* regs;
        uint8_t channel;
        uint32_t fcg0Clock;
        uint32_t completeFlag;
        uint32_t triggerChannel;
        en_event_src_t triggerSource;
        en_int_src_t interruptSource;
        IRQn_Type irq;
        uint32_t priority;
        void (*callback)(); /**< 板级无参数跳板，必须调用所属实例的 handleDmaIrq。 */
    };

    struct Config {
        CM_SPI_TypeDef* regs;
        uint32_t fcg1Clock;
        gpio_pin_t sck;
        gpio_pin_t mosi;
        gpio_pin_t miso; /**< -1 表示不使用 MISO。 */
        uint16_t sckFunc;
        uint16_t mosiFunc;
        uint16_t misoFunc;
        uint32_t clockDivider;
        uint32_t mode;
        uint32_t firstBit;
        const DmaConfig* txDma; /**< 可为空；非空时配置须与实例同寿命。 */
    };

    /** 保存有效的板级配置；不访问硬件，实例不可复制以免共享 DMA 状态。 */
    explicit constexpr SpiMaster(Config config) : config(config) {}

    SpiMaster(const SpiMaster&) = delete;
    SpiMaster& operator=(const SpiMaster&) = delete;

    /** 复用原 SPI begin 的寄存器顺序，初始化 8 bit 主机并保持禁用；失败返回 false。 */
    bool begin();
    /** 单独初始化可选 TX DMA，允许板级在面板 GPIO/SPI 初始化前调用；无配置或 LL 失败返回 false。 */
    bool beginDma();
    /** 同步发送一个字节并等待移位完成；沿用原 transfer 的阻塞等待，仅限硬件正常且无在途 DMA。 */
    void writeByte(uint8_t data);
    /** 同步逐字节发送；data 在 len>0 时须非空，调用者先结束在途 DMA，等待条件同 writeByte。 */
    void write(const uint8_t* data, size_t len);
    /** 非空缓冲、长度 1..65535；成功后由 DMA 持有缓冲直到完成且 isIdle，或 abort 返回。 */
    bool startDmaWrite(const void* data, size_t len);
    /** 返回 ISR 发布的完成标志；新 DMA 传输前复位，完成不代表末字节已移出。 */
    bool dmaDone() const;
    /** IDLNF 清零才表示移位结束；不读取或改变 DMA 状态。 */
    bool isIdle() const;
    /** 依次禁用 DMA 通道与 SPI；由主循环在完成或超时时回收总线，不操作片选。 */
    void abort();
    /** 仅供所属 DMA 中断调用：清 TC 并置位完成标志，不等待、不操作 GPIO。 */
    void handleDmaIrq();

  private:
    const Config config;
    bool dmaReady = false;
    volatile bool transferDone = false;
};
