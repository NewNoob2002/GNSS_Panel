#pragma once
#include "Helper/NodeHelper.h"
namespace dp {
/** @brief 类型化应用设置，仅保存当前语言。 */
struct SettingsState {
    i18n::Language language{};
    bool operator==(const SettingsState& v) const { return language == v.language; }
};
struct SettingsCommand { i18n::Language language{}; };
using Settings = Writer<names::Settings, SettingsState, SettingsCommand>;
}
