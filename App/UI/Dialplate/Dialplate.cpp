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
    setLoadAnimPath(style::page_transition_path);
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
    view_.close_focus(); // 从入口展开返回：面板仍盖着首页，收回到原入口。
    if (entrance_pending_) {
        // Returning from a child slides the page back already; replaying the entrance would double the work.
        entrance_pending_ = false;
        view_.appear_anim_start();
    }
    view_.anim_start();
}

void Dialplate::onViewWillDisappear() {
    // 其他导航抢先离页时放弃尚未执行的入口导航。
    lv_async_call_cancel(navigate_entry, this);
    pending_entry_ = PageId::Invalid;
    view_.anim_stop();
    view_.unbind();
    last_focus_ = utils::leave_focus_group();
}

void Dialplate::onViewUnload() {
    lv_async_call_cancel(navigate_entry, this);
    pending_entry_ = PageId::Invalid;
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
    if (destination != PageId::Invalid)
        self->open_entry(destination);
}

void Dialplate::open_entry(PageId destination) {
    if (pending_entry_ != PageId::Invalid || getManager()->isSwitching())
        return;
    pending_entry_ = destination;
    if (view_.open_focus(on_entry_open, this))
        return;
    // 无覆盖框时退回页面自身的切屏动画。
    pending_entry_ = PageId::Invalid;
    if (getManager()->push(page::name(destination)) != PageManager::RES_TYPE::OK)
        APP_LOG_W("Dialplate", "navigation rejected");
}

void Dialplate::on_entry_open(void* user_data) {
    // 在弹簧定时器回调之外导航：PageManager 切屏会刷新动画时基，不能在动画/定时器回调中重入。
    if (lv_async_call(navigate_entry, user_data) != LV_RESULT_OK)
        navigate_entry(user_data);
}

void Dialplate::navigate_entry(void* user_data) {
    auto* self = static_cast<Dialplate*>(user_data);
    const auto destination = self->pending_entry_;
    self->pending_entry_ = PageId::Invalid;
    if (destination == PageId::Invalid)
        return;
    // 面板已盖满且与目标页背景同色：直接切换，目标页控件自行依次浮现；返回同样 NONE，由首页收回面板。
    if (self->getManager()->pushWithAnim(page::name(destination), PAGE_ANIM::NONE) != PageManager::RES_TYPE::OK) {
        APP_LOG_W("Dialplate", "navigation rejected");
        self->view_.close_focus();
    }
}
