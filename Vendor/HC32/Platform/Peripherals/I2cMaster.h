#pragma once

#include <stddef.h>
#include <stdint.h>
#include "pinmap.h"

/** 同步阻塞、不可重入，仅供主循环使用；每次 LL 等待上限为 0x40000。 */
class I2cMaster {
  public:
    struct Config {
        CM_I2C_TypeDef* regs;
        uint32_t fcg1Clock;
        gpio_pin_t scl;
        gpio_pin_t sda;
        uint16_t sclFunc;
        uint16_t sdaFunc;
    };

    using ScanCallback = void (*)(const uint8_t* address);

    /** 配置须对应有效的外设、时钟与引脚；构造仅保存配置，不访问硬件。 */
    explicit constexpr I2cMaster(Config config) : config(config) {}

    /** clockHz 为 1..400000 Hz；初始化或事务失败返回 false。 */
    bool begin(uint32_t clockHz);
    /** 关闭外设并清除就绪状态；可重复调用，不切换时钟门控或引脚配置。 */
    void end();

    /** 返回最近一次 begin 是否成功；end 或无效频率会清除此状态。 */
    bool
    isReady() const {
        return initialized;
    }

    /** addr 为 7 位地址；缓冲区非空、len 为 1..UINT32_MAX，写完发送 STOP。 */
    bool write(uint8_t addr, const uint8_t* data, size_t len);
    /** 发送 reg、重复 START 后读取；失败时 buf 可能已部分更新，仅成功后可使用。 */
    bool writeRead(uint8_t addr, uint8_t reg, uint8_t* buf, size_t len);
    /** 探测 0..0x7f 地址，完整执行地址发送与 STOP；未就绪或事务失败返回 false。 */
    bool probe(uint8_t addr);
    /** 扫描 1..0x7e 并返回在线数。每个地址回调一次，离线传空指针，在线指针仅在回调内有效。 */
    int scan(ScanCallback callback);

  private:
    /** 启动事务并发送写地址；失败时按原实现恢复总线。 */
    bool start(uint8_t addr);
    /** 发送数据；失败时恢复总线并保留事务错误。 */
    bool send(const uint8_t* data, size_t len);
    /** 发送 STOP、恢复 ACK，并按事务状态执行 SW reset 后关闭外设。 */
    bool finish();

    const Config config;
    bool initialized = false;
    int32_t transactionStatus = LL_ERR;
};
