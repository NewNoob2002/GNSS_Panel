#include "TemplateView.h"

using namespace page;

void TemplateView::create(lv_obj_t* root) {
    label_ = lv_label_create(root);
    lv_obj_center(label_);
}

bool TemplateView::bind(lv_subject_t* subject) {
    unbind();
    observer_ = lv_subject_add_observer(subject, on_state, this);
    return observer_ != nullptr;
}

void TemplateView::unbind() {
    lv_observer_delete(observer_);
    observer_ = nullptr;
}

void TemplateView::on_state(lv_observer_t* observer, lv_subject_t* subject) {
    auto* self = static_cast<TemplateView*>(lv_observer_get_user_data(observer));
    const bool recording = *static_cast<const bool*>(lv_subject_get_pointer(subject));
    lv_label_set_text(self->label_, recording ? "Stop recording" : "Start recording");
}

void TemplateView::destroy() {
    unbind();
    label_ = nullptr;
}
