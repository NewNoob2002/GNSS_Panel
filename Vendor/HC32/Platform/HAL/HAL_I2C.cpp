#include <string.h>
#include "Charger.h"
#include <HAL.h>
#include "SystemState.h"
#include "board_peripherals.h"
#include "bq40z50.h"
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
            bq40z50Begin(&board_i2c);
            HAL::Battery_Sample(State::Power().batteryInfo);
            break;
        }
        case 0x5C: {
            CORE_DEBUG_PRINTF("0x%02X - MP27692A Power Management / Charger", addr);
            State::Board().online_device.mp2762 = true;
            mp2762aBegin(&board_i2c);
            strcpy(State::Board().hardware_version, "V1.5");
            break;
        }
    }
}

bool
HAL::I2C_ProbeDevice(uint8_t address) {
    if (!board_i2c.probe(address)) {
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
    if (!board_i2c.begin(100000U)) {
        CORE_DEBUG_PRINTF("I2C: init failed");
        return -1;
    }
    CORE_DEBUG_PRINTF("I2C: device scanning...");
    strcpy(State::Board().software_version, SOFTWARE_VERSION);
    strcpy(State::Board().hardware_version, "V1.3");
    return board_i2c.scan(i2c_scan_callback);
}