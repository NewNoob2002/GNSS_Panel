#include "flash.h"

// 等待 EFM 就绪的上限；正常情况下上一次擦写结束即就绪。
#define FLASH_READY_SPINS 1000000UL

static int32_t
flash_wait_ready(void) {
    for (uint32_t i = 0; i < FLASH_READY_SPINS; ++i) {
        if (SET == EFM_GetStatus(EFM_FLAG_RDY)) {
            return LL_OK;
        }
    }
    return LL_ERR_NOT_RDY;
}

int32_t
FlashWriteData(uint32_t u32Addr, const uint8_t* pu8Buff, uint32_t u32Len) {
    if ((pu8Buff == NULL) || (u32Len == 0U) || ((u32Addr + u32Len) > (EFM_BASE + EFM_END_ADDR + 1UL))) {
        return LL_ERR_INVD_PARAM;
    }
    if (0UL != (u32Addr % 4U)) {
        return LL_ERR_ADDR_ALIGN;
    }
    if (flash_wait_ready() != LL_OK) {
        return LL_ERR_NOT_RDY;
    }
    EFM_FWMC_Cmd(ENABLE);
    EFM_SetBusStatus(EFM_BUS_HOLD);
    const int32_t result = EFM_Program(u32Addr, (uint8_t*)pu8Buff, u32Len);
    EFM_SetBusStatus(EFM_BUS_RELEASE);
    // 编程结束后恢复只读，避免误写。
    EFM_FWMC_Cmd(DISABLE);
    return result;
}

int32_t
FlashEraseSector(uint32_t u32Addr) {
    if (flash_wait_ready() != LL_OK) {
        return LL_ERR_NOT_RDY;
    }
    EFM_FWMC_Cmd(ENABLE);
    EFM_SetBusStatus(EFM_BUS_HOLD);
    const int32_t result = EFM_SectorErase(u32Addr);
    EFM_SetBusStatus(EFM_BUS_RELEASE);
    EFM_FWMC_Cmd(DISABLE);
    return result;
}

void
FlashRead(uint32_t Address, unsigned char* pdata, uint32_t len) {
    const unsigned char* pageaddr = (const unsigned char*)(Address);
    for (uint32_t i = 0; i < len; i++) {
        pdata[i] = pageaddr[i];
    }
}
