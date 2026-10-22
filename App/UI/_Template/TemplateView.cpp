#include "TemplateView.h"

using namespace page;

void TemplateView::create(lv_obj_t* root) {
    label_ = lv_label_create(root);
    lv_label_set_text(label_, "Template");
    lv_obj_center(label_);
}

void TemplateView::destroy() {
    label_ = nullptr;
}
