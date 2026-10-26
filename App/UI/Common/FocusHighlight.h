/** @file FocusHighlight.h @brief 卡片焦点的透明覆盖边框，限 LVGL 应用线程使用。 */
#pragma once
#include <array>
#include <cstddef>
#include "Utils/Spring/lv_spring.h"

namespace page {
/**
 * @brief 借用 card/control 控件；先于控件或后于控件销毁均安全，空闲与离页无驱动定时器。
 * 参照 smooth_ui_toolkit SmoothSelectorMenu：焦点跟随与按下挤压/松开回弹用弹簧（四轴共用一组参数：位置与尺寸若分速，
 * 贴边控件切换时边缘会越出屏幕）；open()/close() 的面板展开收回改用切屏时长（350 ms）
 * 与缓入缓出曲线：弹簧要么约 150 ms 即盖满显得生硬，要么为凑 350 ms 拖出缓慢的收敛尾段。
 */
class FocusHighlight {
  public:
    using DoneCallback = void (*)(void* user_data);
    ~FocusHighlight() { unbind(); }
    FocusHighlight() = default;
    FocusHighlight(const FocusHighlight&) = delete;
    FocusHighlight& operator=(const FocusHighlight&) = delete;

    /**
     * @brief 显式接管最多六个 card/control 的焦点边框；首次聚焦直接落位，失败撤销绑定。
     * @param shared_layer true 时覆盖框固定在 root，跨父容器切换也保速过渡（控件父容器须静止、不裁剪覆盖框）；
     *        false 时覆盖框跟随控件父容器，随其滚动裁剪与动画，换父容器时直接落位。
     */
    template <std::size_t N>
    bool bind(lv_obj_t* root, const std::array<lv_obj_t*, N>& controls, bool shared_layer = false) {
        static_assert(N <= 6, "焦点控件超出页面容量");
        return bind_controls(root, controls.data(), N, shared_layer);
    }
    /** @brief 恢复 card/control 样式、解除所有事件并删除覆盖框，不删除控件。 */
    void unbind();
    /** @brief 首次进入及恢复时对齐当前焦点，再允许后续保速重定向。 */
    void resume();
    /** @brief 语言或布局变化直接落位；不启动定时器。 */
    void sync();
    /** @brief 离页停止推进，保留过渡所需画面（含已展开的面板），恢复前新焦点直接落位。 */
    void stop();
    /**
     * @brief 从当前焦点控件展开为 fill 色不透明面板，盖住宿主并越出边缘（圆角与边框在屏外），橙框随之淡出；
     *        展开期间冻结焦点组，按切屏时长展开完成后调用 done；收回途中可再次展开。
     *        未绑定、无焦点或已展开时返回 false 且不调用 done。
     */
    bool open(lv_color_t fill, DoneCallback done, void* user_data);
    /** @brief 从展开状态收回到焦点控件，静止后恢复透明边框；未展开时无操作。 */
    void close();
    /** @brief 是否处于展开、已展开或收回中。 */
    bool opened() const { return mode_ != Mode::Focus; }
    lv_obj_t* object() const { return motion_.object(); }
    const app::LvSpring& motion() const { return motion_; }

  private:
    enum class Mode { Focus, Opening, Open, Closing };
    bool bind_controls(lv_obj_t* root, lv_obj_t* const* controls, std::size_t count, bool shared_layer);
    void focus_to(lv_obj_t* object, bool animated, bool layout_only = false);
    void watch_parent(lv_obj_t* parent);
    void visibility();
    void freeze(bool frozen);
    void restore_border();
    void target_of(lv_obj_t* object, lv_obj_t* host, int32_t (&geometry)[4]) const;
    void panel_start(const int32_t (&to)[4]);
    void panel_apply(int32_t step);
    void panel_done();
    void finish_close();
    static void on_panel(lv_anim_t* animation, int32_t step);
    static void on_panel_done(lv_anim_t* animation);
    static constexpr int32_t kPanelSteps = 1024;
    static void on_control(lv_event_t* event);
    static void on_parent(lv_event_t* event);
    app::LvSpring motion_{};
    std::array<lv_obj_t*, 6> controls_{};
    lv_obj_t *focused_ = nullptr, *parent_ = nullptr, *host_ = nullptr;
    bool animate_ = false, aligning_ = false, frozen_ = false;
    Mode mode_ = Mode::Focus;
    DoneCallback done_ = nullptr;
    void* done_user_data_ = nullptr;
    int32_t from_[4]{}, to_[4]{}; // 面板插值起止几何 x/y/w/h（宿主内容区坐标）。
};
} // namespace page
