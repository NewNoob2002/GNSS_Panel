#pragma once
#include <array>
#include "HAL_Def.h"

namespace hal {
/** @brief 原始定位状态；基站/移动站由 Work 设备独立提供。 */
enum class FixStatus { None, Single, Float, Fixed };
/** @brief GNSS 同步快照；纬经度单位为度，海拔单位 m，无文本或颜色。 */
struct GNSSState {
    bool position_valid = false;
    double latitude = 0, longitude = 0, altitude = 0;
    bool satellites_valid = false;
    unsigned satellites_used = 0, satellites_tracked = 0;
    bool constellations_valid = false;
    std::array<unsigned, 7> constellations{}; ///< GPS/BDS/GLONASS/Galileo/SBAS/QZSS/IRNSS，单位颗。
    FixStatus fix{};
    bool operator==(const GNSSState& v) const {
        return position_valid == v.position_valid && latitude == v.latitude && longitude == v.longitude
            && altitude == v.altitude && satellites_valid == v.satellites_valid && satellites_used == v.satellites_used
            && satellites_tracked == v.satellites_tracked && constellations_valid == v.constellations_valid
            && constellations == v.constellations && fix == v.fix;
    }
    bool operator!=(const GNSSState& v) const { return !(*this == v); }
};
} // namespace hal
