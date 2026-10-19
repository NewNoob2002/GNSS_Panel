#ifndef FLASH_H
#define FLASH_H
#include "hc32_ll.h"

// 片内 Flash 擦写辅助（源自 X1_Panel_dev）。调用方负责地址落在允许的参数区内；
// 擦除/编程期间总线保持，CPU 与中断会暂停约一个扇区擦除时间（实测约 18 ms）。

#ifdef __cplusplus
extern "C" {
#endif

int32_t FlashWriteData(uint32_t Address, const uint8_t* flashdata, uint32_t len);
int32_t FlashEraseSector(uint32_t u32Addr);
void FlashRead(uint32_t Address, unsigned char* pdata, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif
