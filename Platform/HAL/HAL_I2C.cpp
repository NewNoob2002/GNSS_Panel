#include "Charger.h"
#include "HAL.h"
#include "Wire.h"
#include "bq40z50.h"
#include "mp2762a.h"

void
i2c_scan_callback(void* e) {
    if (e == nullptr) {
        return;
    }
    uint8_t addr = *(uint8_t*)e;
    switch (addr) {
        default: {
            CORE_DEBUG_PRINTF("0x%02X - Unknow", addr);
            break;
        }
        case 0x0B: {
            CORE_DEBUG_PRINTF("0x%02X - BQ40Z50 Battery Pack Manager / Fuel gauge", addr);
            systemInfo.online_device.bq40z50 = true;
            bq40z50Begin(&Wire);
            HAL::Battery_Sample(systemInfo.powerMonitor.batteryInfo);
            break;
        }
        case 0x5C: {
            CORE_DEBUG_PRINTF("0x%02X - MP27692A Power Management / Charger", addr);
            systemInfo.online_device.mp2762 = true;
            mp2762aBegin(&Wire);
            strcpy(systemInfo.hardware_version, "V1.5");
            break;
        }
    }
}

int
HAL::I2C_Scan() {
    if (!Wire.begin()) {
        CORE_DEBUG_PRINTF("I2C: init failed");
        return -1;
    }
    CORE_DEBUG_PRINTF("I2C: device scanning...");
    strcpy(systemInfo.software_version, SOFTWARE_VERSION);
    strcpy(systemInfo.hardware_version, "V1.3");
    return Wire.scanDeivces(i2c_scan_callback);
}