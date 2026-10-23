#include "Dialplate.h"
#include "Log.h"
#include "PageManager.h"
#include "UI/PageId.h"
#include "UI/Common/PageUtils.h"
#include "UI/Common/PageStyle.h"

using namespace page;

void Dialplate::onInstalled() {
    setLoadAnimType(PAGE_ANIM::OVER_TOP);
    setLoadAnimTime(style::kPageTransitionMs);
    setLoadAnimPath(lv_anim_path_ease_out);
}

std::array<lv_obj_t*, 5> Dialplate::controls() const {
    return view_.controls(model_->state().star_map);
}

void Dialplate::onViewLoad() {
    model_ = std::make_unique<DialplateModel>();
    if (!model_->init())
        app_fatal("Dialplate", "Model initialization failed");
    view_.create(getRoot());
    utils::attach_controls(controls(), clicked, this);
    entrance_pending_ = true;
}

void Dialplate::onViewWillAppear() {
    model_->set_status_bar(true);
    if (!view_.bind(model_->subject()))
        app_fatal("Dialplate", "View binding failed");
    utils::focus_controls(controls(), last_focus_ ? last_focus_ : view_.default_focus());
    view_.resume_focus();
    if (entrance_pending_) {
        // Returning from a child slides the page back already; replaying the entrance would double the work.
        entrance_pending_ = false;
        view_.appear_anim_start();
    }
    view_.anim_start();
}

void Dialplate::onViewWillDisappear() {
    view_.anim_stop();
    view_.unbind();
    last_focus_ = utils::leave_focus_group();
}

void Dialplate::onViewUnload() {
    view_.destroy();
    model_.reset();
    last_focus_ = nullptr;
}

void Dialplate::clicked(lv_event_t* event) {
    auto* self = static_cast<Dialplate*>(lv_event_get_user_data(event));
    auto* button = lv_event_get_current_target_obj(event);
    PageId destination = PageId::Invalid;
    switch (self->view_.control(button)) {
    case DialplateView::Control::Work: destination = PageId::WorkSettings; break;
    case DialplateView::Control::Record: destination = PageId::RecordConfig; break;
    case DialplateView::Control::Menu: destination = PageId::SystemInfos; break;
    case DialplateView::Control::Power: destination = PageId::SystemDash; break;
    case DialplateView::Control::Satellite: destination = PageId::StarMap; break;
    case DialplateView::Control::None: break;
    }
    if (destination != PageId::Invalid
        && self->getManager()->push(page::name(destination)) != PageManager::RES_TYPE::OK)
        APP_LOG_W("Dialplate", "navigation rejected");
}
