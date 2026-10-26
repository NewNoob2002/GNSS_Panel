#pragma once
#include "Helper/NodeHelper.h"
namespace dp {
/** @brief 当前语言与本次运行的屏幕亮度。 */
struct SettingsState {
    i18n::Language language{};
    uint8_t brightness = 100;
    bool operator==(const SettingsState& v) const { return language == v.language && brightness == v.brightness; }
};
struct SettingsCommand {
    i18n::Language language{};
    uint8_t brightness = 0; ///< 0 保持当前亮度；其余值须为 20..100。
};
using Settings = Writer<names::Settings, SettingsState, SettingsCommand>;
}
