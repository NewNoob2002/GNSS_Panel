#pragma once

#include "GpioPin.h"
#include "SpiMaster.h"

/** 当前 126×294 RM690A0 面板；阻塞命令只在主循环且无在途 DMA 时调用，无虚函数与动态分配。 */
class PanelRm690a0 {
  public:
    struct Config {
        gpio_pin_t cs;
        gpio_pin_t dc;
        gpio_pin_t rst;
        gpio_pin_t blk; /**< -1 表示板上没有背光控制引脚。 */
    };

    /** 保存总线引用与板级引脚，不访问硬件；总线须比面板存活更久。 */
    constexpr PanelRm690a0(SpiMaster& spi, Config config)
        : spi(spi), cs(config.cs), dc(config.dc), rst(config.rst), blk(config.blk), hasBacklight(config.blk != -1) {}

    /** 完整重复本地旧库的双复位、初始化表、rotation 2；SPI 初始化失败返回 false，不初始化 DMA。 */
    bool init();
    /** 设置含首尾坐标的物理窗口，调用者先 select；不操作 CS，末尾保持 DC 为数据模式。 */
    void setWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
    /** 同步填充全屏，RGB565 高字节先发；函数自行选中与释放 CS。 */
    void fill(uint16_t color);

    /** 物理宽度固定为 126；逻辑旋转由显示端口完成。 */
    constexpr uint16_t
    width() const {
        return 126;
    }

    /** 物理高度固定为 294，无列/行偏移。 */
    constexpr uint16_t
    height() const {
        return 294;
    }

    /** 拉低 CS，开始窗口/像素事务。 */
    void select();
    /** 拉高 CS；DMA 场景只能在移位完成或 abort 后调用。 */
    void deselect();
    /** 将 DC 置为数据模式，供 DMA 像素发送使用。 */
    void dataMode();
    /** 同步发送完整命令事务；参数在 count>0 时须非空，参数顺序不变。 */
    void sendCommand(uint8_t command, const uint8_t* data = nullptr, size_t count = 0);
    /** 在已选中事务内发送单字节命令，再恢复数据模式，不改变 CS。 */
    void writeCommand(uint8_t command);
    /** 初始化可选 BLK 为输出 LOW；当前板未配置此引脚，复位时不重复调用。 */
    void enableBacklight();

  private:
    /** 发送两个 16 bit 地址，均为高字节先发；对应旧 SPI_WRITE32 编码。 */
    void writeAddress(uint16_t first, uint16_t last);

    SpiMaster& spi;
    const GpioPin cs;
    const GpioPin dc;
    const GpioPin rst;
    const GpioPin blk;
    const bool hasBacklight;
};
