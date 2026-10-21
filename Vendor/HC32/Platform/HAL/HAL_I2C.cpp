#include <string.h>
#include "Charger.h"
#include "HAL.h"
#include "SystemState.h"
#include "bq40z50.h"
#include "i2c_master.h"
#include "mp2762a.h"

void
i2c_scan_callback(const uint8_t* e) {
    if (e == nullptr) {
        return;
    }
    uint8_t addr = *e;
    switch (addr) {
        default: {
            CORE_DEBUG_PRINTF("0x%02X - Unknow", addr);
            break;
        }
        case 0x0B: {
            CORE_DEBUG_PRINTF("0x%02X - BQ40Z50 Battery Pack Manager / Fuel gauge", addr);
            State::Board().online_device.bq40z50 = true;
            bq40z50Begin(&g_i2c_main);
            HAL::Battery_Sample(State::Power().batteryInfo);
            break;
        }
        case 0x5C: {
            CORE_DEBUG_PRINTF("0x%02X - MP27692A Power Management / Charger", addr);
            State::Board().online_device.mp2762 = true;
            mp2762aBegin(&g_i2c_main);
            strcpy(State::Board().hardware_version, "V1.5");
            break;
        }
    }
}

bool
HAL::I2C_ProbeDevice(uint8_t address) {
    if (!i2c_master_probe(&g_i2c_main, address)) {
        return false;
    }
    const bool known = (address == 0x0B && systemInfo.board.online_device.bq40z50)
                       || (address == 0x5C && systemInfo.board.online_device.mp2762);
    if (!known) {
        i2c_scan_callback(&address);
    }
    return true;
}

int
HAL::I2C_Scan() {
    if (!i2c_master_init(&g_i2c_main, 100000U)) {
        CORE_DEBUG_PRINTF("I2C: init failed");
        return -1;
    }
    CORE_DEBUG_PRINTF("I2C: device scanning...");
    strcpy(State::Board().software_version, SOFTWARE_VERSION);
    strcpy(State::Board().hardware_version, "V1.3");
    return i2c_master_scan(&g_i2c_main, i2c_scan_callback);
}