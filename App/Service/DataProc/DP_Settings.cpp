#include "Node.h"
#include "Def/DP_Settings.h"
namespace {
class SettingsNode final : public dp::ValueNode<dp::SettingsState> {
public:
    explicit SettingsNode(DataBroker* broker) : ValueNode(dp::names::Settings, broker) { state_.language = i18n::get_language(); }
    int onEvent(EventParam_t* event) override {
        if (!event || event->event != EVENT_NOTIFY) return ValueNode::onEvent(event);
        dp::SettingsCommand command;
        const int decoded = DataProc::ReadPayload(event, command);
        if (decoded != RES_OK) return decoded;
        if (command.language != i18n::Language::English && command.language != i18n::Language::Russian) return RES_PARAM_ERROR;
        i18n::set_language(command.language);
        commit({command.language});
        return RES_OK;
    }
};
}
DP_CREATE(Settings, SettingsNode)
