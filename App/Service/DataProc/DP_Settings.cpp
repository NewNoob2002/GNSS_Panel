#include "Node.h"
#include "Def/DP_Settings.h"
#include "HAL/Def/HAL_Board.h"
namespace {
class SettingsNode final : public dp::ValueNode<dp::SettingsState> {
public:
    explicit SettingsNode(DataBroker* broker) : ValueNode(dp::names::Settings, broker) { state_.language = i18n::get_language(); }
    bool init() override {
        hal::BoardState board;
        if (hal::read(hal::manager().getDevice(hal::names::Board), board) != DeviceObject::RES_OK) return false;
        state_.brightness = board.brightness;
        return isRegistered();
    }
    int onEvent(EventParam_t* event) override {
        if (!event || event->event != EVENT_NOTIFY) return ValueNode::onEvent(event);
        dp::SettingsCommand command;
        const int decoded = DataProc::ReadPayload(event, command);
        if (decoded != RES_OK) return decoded;
        if (command.language != i18n::Language::English && command.language != i18n::Language::Russian) return RES_PARAM_ERROR;
        if (command.brightness && (command.brightness < 20 || command.brightness > 100)) return RES_PARAM_ERROR;
        auto next = state_;
        if (command.brightness && command.brightness != state_.brightness) {
            if (hal::write(hal::manager().getDevice(hal::names::Board), hal::BrightnessCommand{command.brightness}) != DeviceObject::RES_OK) return RES_UNKNOWN;
            next.brightness = command.brightness;
        }
        i18n::set_language(command.language);
        next.language = command.language;
        commit(next);
        return RES_OK;
    }
};
}
DP_CREATE(Settings, SettingsNode)
