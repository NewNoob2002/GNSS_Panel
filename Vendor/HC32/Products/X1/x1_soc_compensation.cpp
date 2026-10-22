// X1 电量补偿（源自 X1_Panel_dev）：部分电池充满时电量计停在 9x%，
// 充电中读数持续 STABLE_TIME_MS 不变即认为已满，记录与 100% 的差值并叠加到显示/上报电量。
#include <HAL.h>
#include "flash.h"

static uint8_t comp_offset;
static bool comp_updated;
static uint16_t last_soc;
static uint32_t stable_timer_ms;

// 补偿值仅接受 0..9：更大的差值说明并非“充满停在 9x%”，不采用也不保存。
static constexpr uint8_t MAX_COMP_OFFSET = 10U;

static uint8_t
read_saved_offset() {
    const uint32_t raw = *(volatile const uint32_t*)APP_PARAM_SECTOR_ADDR;
    // 擦除态或其他格式的数据都视为未保存。
    return raw < MAX_COMP_OFFSET ? static_cast<uint8_t>(raw) : 0U;
}

static bool
save_offset(uint8_t offset) {
    if (FlashEraseSector(APP_PARAM_SECTOR_ADDR) != LL_OK) {
        return false;
    }
    const uint32_t word = offset;
    return FlashWriteData(APP_PARAM_SECTOR_ADDR, reinterpret_cast<const uint8_t*>(&word), sizeof(word)) == LL_OK;
}

void
HAL::Power_ProductInit() {
    comp_offset = read_saved_offset();
    comp_updated = false;
    last_soc = 0;
    stable_timer_ms = 0;
}

// 由 HAL::Power_Update() 每 500 ms 调用。
void
HAL::Power_ProductUpdate() {
    const BatteryInfo_t& battery = systemInfo.power.batteryInfo;
    if (comp_updated) {
        return;
    }
    if (battery.chargeStatus == notCharge || !battery.sample_valid) {
        stable_timer_ms = 0;
        return;
    }
    if (battery.Actual_Percent != last_soc) {
        stable_timer_ms = 0;
        last_soc = battery.Actual_Percent;
        return;
    }
    stable_timer_ms += 500U;
    if (stable_timer_ms < STABLE_TIME_MS) {
        return;
    }
    stable_timer_ms = 0;
    if (battery.Actual_Percent > 100U) {
        return;
    }
    const uint16_t offset = 100U - battery.Actual_Percent;
    if (offset < MAX_COMP_OFFSET) {
        comp_offset = static_cast<uint8_t>(offset);
        comp_updated = save_offset(comp_offset);
        CORE_DEBUG_PRINTF("SOC compensation %u saved: %d", comp_offset, comp_updated);
    }
}

uint16_t
HAL::Power_AdjustPercent(uint16_t actualPercent) {
    return actualPercent + comp_offset;
}
