#include "bq40z50.h"

I2cMaster* bq40z50I2c = nullptr;

bool
bq40z50Begin(I2cMaster* i2cBus) {
    if (i2cBus == nullptr) {
        return false;
    }
    bq40z50I2c = i2cBus;
    return true;
}

bool
bq40z50ReadRegister16Checked(uint8_t reg, uint16_t* value) {
    uint8_t bytes[2] = {};
    if (!value || !bq40z50I2c || !bq40z50I2c->writeRead(BQ40Z50_DEVICE_ADDRESS, reg, bytes, 2)) {
        return false;
    }
    *value = static_cast<uint16_t>((static_cast<uint16_t>(bytes[1]) << 8) | bytes[0]);
    return true;
}

bool
bq40z50ReadAverageCurrentChecked(int16_t* current_ma) {
    uint16_t raw;
    if (!current_ma || !bq40z50ReadRegister16Checked(BQ40Z50_AVERAGE_CURRENT, &raw)) {
        return false;
    }
    // 按二进制补码解码，避免超范围 unsigned -> signed 转换依赖编译器实现。
    *current_ma =
        static_cast<int16_t>(raw <= 0x7fffU ? static_cast<int32_t>(raw) : static_cast<int32_t>(raw) - 0x10000);
    return true;
}

float
bq40z50getRelativeStateOfCharge_float() {
    uint16_t remaining, full;
    if (!bq40z50ReadRegister16Checked(BQ40Z50_REMAINING_CAPACITY, &remaining)
        || !bq40z50ReadRegister16Checked(BQ40Z50_FULL_CHARGE_CAPACITY, &full) || full == 0U) {
        return -1.0f;
    }
    return static_cast<float>(remaining) / static_cast<float>(full) * 100.0f;
}
