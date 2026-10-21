#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 板级总线由本模块持有；调用方只借用指针，不复制或释放。 */
typedef struct i2c_master i2c_master_t;
extern i2c_master_t g_i2c_main; /* I2C2，SCL=PA9，SDA=PA8。 */

/* 每个扫描地址均回调一次；离线时 address 为 NULL，在线指针仅在回调期间有效。 */
typedef void (*i2c_master_scan_callback_t)(const uint8_t* address);

/* 以下操作同步阻塞、不可重入，仅供主循环使用；每次 LL 等待上限为 0x40000。
 * clock_hz 范围为 1..400000 Hz；失败返回 false，事务失败后恢复并关闭外设。
 */
bool i2c_master_init(i2c_master_t* bus, uint32_t clock_hz);
void i2c_master_deinit(i2c_master_t* bus);

/* addr 为 7 位地址（0..0x7f）；缓冲区非空，len 为 1..UINT32_MAX。
 * 写入发送整个缓冲区后 STOP；读操作发送 reg、重复 START，再接收并 STOP。
 * 读失败时缓冲区可能已部分更新，调用方只能在成功后使用数据。
 */
bool i2c_master_write(i2c_master_t* bus, uint8_t addr, const uint8_t* data, size_t len);
bool i2c_master_write_read(i2c_master_t* bus, uint8_t addr, uint8_t reg, uint8_t* buf, size_t len);
bool i2c_master_probe(i2c_master_t* bus, uint8_t addr);

/* 扫描 1..0x7e，返回在线数量；callback 可为 NULL，未初始化时所有地址均离线。 */
int i2c_master_scan(i2c_master_t* bus, i2c_master_scan_callback_t callback);

#ifdef __cplusplus
}
#endif
