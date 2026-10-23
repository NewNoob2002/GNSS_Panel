#include "Node.h"
#include "Def/DP_PageNavi.h"
#include <array>

namespace {
class PageNaviNode final : public dp::ValueNode<dp::PageNaviState> {
  public:
    explicit PageNaviNode(DataBroker* broker) : ValueNode(dp::names::PageNavi, broker) {}
    int onEvent(EventParam_t* event) override {
        if (!event || event->event != EVENT_NOTIFY)
            return ValueNode::onEvent(event);
        dp::PageNaviCommand command;
        const int result = DataProc::ReadPayload(event, command);
        if (result != RES_OK)
            return result;
        using Op = dp::PageNaviOperation;
        switch (command.operation) {
        case Op::Configure:
            if (!command.page || page_count_)
                return RES_PARAM_ERROR;
            page_count_ = command.page;
            return RES_OK;
        case Op::Consume:
            if (command.page || !state_.pending)
                return RES_PARAM_ERROR;
            head_ = (head_ + 1) % queue_.size();
            update(state_.pending - 1);
            return RES_OK;
        case Op::Pop:
            if (command.page)
                return RES_PARAM_ERROR;
            break;
        case Op::Push:
        case Op::Replace:
        case Op::ResetRoot:
        case Op::Preload:
            if (command.page >= page_count_)
                return RES_PARAM_ERROR;
            break;
        default:
            return RES_PARAM_ERROR;
        }
        if (state_.pending == queue_.size())
            return RES_NO_DATA;
        queue_[(head_ + state_.pending) % queue_.size()] = command;
        update(state_.pending + 1);
        return RES_OK;
    }

  private:
    std::array<dp::PageNaviCommand, 8> queue_{};
    size_t head_ = 0;
    uint16_t page_count_ = 0;
    /** @brief 只发布队列快照，绝不在同步 notify 链中执行 UI 导航。 */
    void update(unsigned pending) {
        commit({pending ? queue_[head_] : dp::PageNaviCommand{}, pending});
    }
};
}
DP_CREATE(PageNavi, PageNaviNode)
