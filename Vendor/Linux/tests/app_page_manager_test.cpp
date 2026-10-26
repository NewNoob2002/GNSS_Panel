#include <array>
#include <cassert>
#include <cstring>
#include "PageManager.h"

namespace {
using Result = PageManager::RES_TYPE;

struct Counts {
    unsigned destroyed = 0;
    unsigned unloaded = 0;
    unsigned appeared = 0;
    unsigned languages = 0;
    unsigned stale_callbacks = 0;
};

class ProbePage final : public PageBase {
  public:
    /** @brief Track externally owned counters across page destruction. */
    explicit ProbePage(Counts& counts) : counts_(counts) {}

    /** @brief Record destruction even for pages that were never loaded. */
    ~ProbePage() override { ++counts_.destroyed; }

    /** @brief Register a root callback whose context must not outlive this page. */
    void
    onViewLoad() override {
        lv_obj_add_event_cb(getRoot(), on_root_delete, LV_EVENT_DELETE, this);
    }

    /** @brief Focus a visible root to verify cached and preloaded appearance order. */
    void
    onViewWillAppear() override {
        assert(!lv_obj_is_hidden(getRoot()));
        lv_group_remove_all_objs(lv_group_get_default());
        lv_group_add_obj(lv_group_get_default(), getRoot());
        lv_group_focus_obj(getRoot());
        assert(lv_group_get_focused(lv_group_get_default()) == getRoot());
        ++counts_.appeared;
    }

    /** @brief Count exactly one unload for each loaded root. */
    void
    onViewUnload() override {
        ++counts_.unloaded;
    }

    /** @brief Count language notifications delivered to loaded pages. */
    void
    onLanguageChanged() override {
        ++counts_.languages;
    }

  private:
    /** @brief Detect a deferred root callback that still retains its former page. */
    static void
    on_root_delete(lv_event_t* event) {
        auto* page = static_cast<ProbePage*>(lv_event_get_user_data(event));
        ++page->counts_.stale_callbacks;
    }

    Counts& counts_;
};

/** @brief Deliver animation frames and deferred deletion on the LVGL thread. */
void
advance(unsigned milliseconds = 400) {
    for (unsigned elapsed = 0; elapsed < milliseconds; elapsed += 10) {
        lv_tick_inc(10);
        lv_timer_handler();
    }
}

/** @brief Complete display refreshes without accessing any physical hardware. */
void
flush(lv_display_t* display, const lv_area_t*, uint8_t*) {
    lv_display_flush_ready(display);
}

/** @brief Exercise ownership, cached focus, parameter resizing and navigation guards. */
void
check_manager() {
    Counts root_counts, cached_counts, preload_counts, idle_counts;
    auto* screen = lv_screen_active();
    const auto children = lv_obj_get_child_count(screen);
    {
        PageManager manager;
        auto* root = new ProbePage(root_counts);
        auto* cached = new ProbePage(cached_counts);
        auto* preloaded = new ProbePage(preload_counts);
        cached->setCacheEnable(true);
        assert(manager.registerPage(root, "Root") == Result::OK);
        assert(manager.registerPage(cached, "Cached") == Result::OK);
        assert(manager.registerPage(preloaded, "Preload") == Result::OK);
        assert(manager.registerPage(new ProbePage(idle_counts), "Idle") == Result::OK);
        assert(manager.registerPage(nullptr, "Invalid") == Result::ERR_PARAM);
        assert(manager.push(nullptr) == Result::ERR_PARAM);
        assert(manager.pop() == Result::ERR_OPERATION);
        assert(manager.backHome() == Result::ERR_OPERATION);
        assert(manager.getCurrentPage() == nullptr);
        manager.setGlobalLoadAnim(PAGE_ANIM::MOVE_LEFT, 260, lv_anim_path_ease_out);
        assert(manager.push("Root") == Result::OK);
        assert(manager.isSwitching());
        assert(manager.push("Cached") == Result::ERR_BUSY);
        assert(manager.resetRoot("Preload") == Result::ERR_BUSY);
        advance();
        assert(root->getState() == PageBase::STATE::ACTIVITY);
        assert(manager.preload("Preload") == Result::OK);
        assert(lv_obj_is_hidden(preloaded->getRoot()));
        manager.notifyLanguageChanged();
        assert(root_counts.languages == 1 && preload_counts.languages == 1);
        assert(cached_counts.languages == 0 && idle_counts.languages == 0);

        uint8_t small = 19;
        PageBase::PARAM param{&small, sizeof(small)};
        assert(manager.push("Cached", &param) == Result::OK);
        advance();
        assert(manager.pop() == Result::OK);
        advance();
        assert(cached->getRoot() && lv_obj_is_hidden(cached->getRoot()));
        std::array<uint8_t, 64> large{};
        large.fill(0xa7);
        param = {large.data(), large.size()};
        assert(manager.push("Cached", &param) == Result::OK);
        advance();
        assert(cached_counts.appeared == 2);
        std::array<uint8_t, 64> copied{};
        assert(!cached->getParam(nullptr, copied.size()));
        assert(cached->getParam(copied.data(), copied.size()));
        assert(copied == large);
        assert(!cached->getParam(copied.data(), copied.size()));
        assert(manager.pop() == Result::OK);
        advance();
        assert(manager.push("Cached", &param) == Result::OK);
        advance();
        assert(manager.pop() == Result::OK);
        advance();
        large.fill(0xb5);
        assert(manager.push("Cached", &param) == Result::OK);
        advance();
        assert(manager.pop() == Result::OK);
        advance();
        param = {&small, sizeof(small)};
        assert(manager.push("Cached", &param) == Result::OK);
        advance();
        uint8_t result = 0;
        assert(cached->getParam(&result, sizeof(result)) && result == small);
        assert(manager.pop() == Result::OK);
        advance();
        PageBase::PARAM invalid{nullptr, 4};
        assert(manager.push("Cached", &invalid) == Result::ERR_PARAM);
        assert(manager.getCurrentPage() == root && manager.getStackDepth() == 1);
        assert(!manager.isSwitching());
        assert(manager.resetRoot("Missing") == Result::ERR_NOT_FOUND);
        assert(manager.getCurrentPage() == root);
        assert(manager.push("Preload") == Result::OK);
        advance();
        assert(preload_counts.appeared == 1);
        assert(manager.backHome() == Result::OK);
        advance();
        assert(manager.getCurrentPage() == root && manager.getStackDepth() == 1);
        assert(manager.resetRoot("Root") == Result::OK);
        advance();
        assert(manager.pop() == Result::ERR_OPERATION);
        assert(manager.uninstall("Cached") == Result::OK);
        assert(cached_counts.destroyed == 1 && cached_counts.unloaded == 1);
        assert(manager.push("Cached") == Result::ERR_NOT_FOUND);
        // 析构时还存在未结束的动画，回调不得访问已释放的页面实例。
        assert(manager.push("Preload") == Result::OK);
    }
    advance();
    assert(root_counts.destroyed == 1 && root_counts.unloaded == 2);
    assert(preload_counts.destroyed == 1 && preload_counts.unloaded == 2);
    assert(idle_counts.destroyed == 1 && idle_counts.unloaded == 0);
    assert(root_counts.stale_callbacks == 0 && cached_counts.stale_callbacks == 0
           && preload_counts.stale_callbacks == 0);
    assert(lv_obj_get_child_count(screen) == children);
}
/** @brief Return the screen child that is neither page root (the transition scrim), or nullptr. */
lv_obj_t*
find_scrim(lv_obj_t* screen, const lv_obj_t* first, const lv_obj_t* second) {
    for (uint32_t i = 0; i < lv_obj_get_child_count(screen); ++i) {
        auto* child = lv_obj_get_child(screen, static_cast<int32_t>(i));
        if (child != first && child != second) {
            return child;
        }
    }
    return nullptr;
}

/** @brief |actual - expected| within a few pixels/opacity steps of animation timer granularity. */
bool
near(int32_t actual, int32_t expected, int32_t tolerance = 6) {
    return actual >= expected - tolerance && actual <= expected + tolerance;
}

/** @brief PUSH_* trails the covered page by a third under a scrim limited to the uncovered strip. */
void
check_push_scrim() {
    Counts first_counts, second_counts;
    auto* screen = lv_screen_active();
    const auto children = lv_obj_get_child_count(screen);
    {
        PageManager manager;
        auto* first = new ProbePage(first_counts);
        auto* second = new ProbePage(second_counts);
        assert(manager.registerPage(first, "First") == Result::OK);
        assert(manager.registerPage(second, "Second") == Result::OK);
        manager.setScrimOpa(LV_OPA_50);
        manager.setGlobalLoadAnim(PAGE_ANIM::PUSH_LEFT, 260, lv_anim_path_linear);
        assert(manager.push("First") == Result::OK);
        advance();
        assert(lv_obj_get_child_count(screen) == children + 1); // The first push has nothing to cover.

        constexpr int32_t width = 294;
        assert(manager.push("Second") == Result::OK);
        auto* scrim = find_scrim(screen, first->getRoot(), second->getRoot());
        assert(scrim && lv_obj_get_index(scrim) + 1 == lv_obj_get_index(second->getRoot()));
        assert(lv_obj_get_index(first->getRoot()) < lv_obj_get_index(scrim));
        advance(130);
        lv_obj_update_layout(screen);
        const auto covering = lv_obj_get_x(second->getRoot());
        assert(near(covering, width / 2));
        assert(near(lv_obj_get_x(first->getRoot()), -width / 3 / 2)); // Parallax: a third of the distance.
        assert(lv_obj_get_x(scrim) == 0 && near(lv_obj_get_width(scrim), covering, 1)); // Follows the edge.
        assert(near(lv_obj_get_style_bg_opa(scrim, LV_PART_MAIN), LV_OPA_50 / 2));
        advance();
        assert(!find_scrim(screen, first->getRoot(), second->getRoot())); // Deleted when the switch ends.
        assert(lv_obj_get_x(first->getRoot()) == -width / 3 && lv_obj_get_x(second->getRoot()) == 0);

        assert(manager.pop() == Result::OK);
        advance(130);
        lv_obj_update_layout(screen);
        scrim = find_scrim(screen, first->getRoot(), second->getRoot());
        assert(scrim && lv_obj_get_index(scrim) + 1 == lv_obj_get_index(second->getRoot()));
        assert(near(lv_obj_get_x(second->getRoot()), width / 2) && near(lv_obj_get_x(first->getRoot()), -width / 6));
        assert(near(lv_obj_get_width(scrim), lv_obj_get_x(second->getRoot()), 1));
        assert(near(lv_obj_get_style_bg_opa(scrim, LV_PART_MAIN), LV_OPA_50 / 2)); // Lifts while uncovering.
        advance();
        assert(!find_scrim(screen, first->getRoot(), second->getRoot()));
        assert(lv_obj_get_x(first->getRoot()) == 0);

        // MOVE_* keeps both pages fully visible, and a zero opacity disables the scrim.
        manager.setGlobalLoadAnim(PAGE_ANIM::MOVE_LEFT, 260, lv_anim_path_linear);
        assert(manager.push("Second") == Result::OK);
        assert(!find_scrim(screen, first->getRoot(), second->getRoot()));
        advance();
        assert(manager.pop() == Result::OK);
        advance();
        manager.setScrimOpa(LV_OPA_TRANSP);
        manager.setGlobalLoadAnim(PAGE_ANIM::OVER_TOP, 260, lv_anim_path_linear);
        assert(manager.push("Second") == Result::OK);
        assert(!find_scrim(screen, first->getRoot(), second->getRoot()));
        advance();
        assert(manager.pop() == Result::OK);
        advance();
        manager.setScrimOpa(LV_OPA_50);
        assert(manager.push("Second") == Result::OK); // OVER_* dims the static page beneath as well.
        assert(find_scrim(screen, first->getRoot(), second->getRoot()));
        advance(100);
    } // Destroying the manager mid-transition must not leave a scrim or page root behind.
    advance();
    assert(lv_obj_get_child_count(screen) == children);
}

/** @brief pushWithAnim belongs to the stack entry: it drives entering and leaving, and a plain push resets it. */
void
check_push_with_anim() {
    Counts first_counts, second_counts, third_counts;
    auto* screen = lv_screen_active();
    const auto children = lv_obj_get_child_count(screen);
    {
        PageManager manager;
        auto* first = new ProbePage(first_counts);
        auto* second = new ProbePage(second_counts);
        auto* third = new ProbePage(third_counts);
        assert(manager.registerPage(first, "First") == Result::OK);
        assert(manager.registerPage(second, "Second") == Result::OK);
        assert(manager.registerPage(third, "Third") == Result::OK);
        manager.setScrimOpa(LV_OPA_50);
        manager.setGlobalLoadAnim(PAGE_ANIM::PUSH_LEFT, 260, lv_anim_path_linear);
        assert(manager.push("First") == Result::OK);
        advance();

        assert(manager.pushWithAnim("Second", PAGE_ANIM::NONE) == Result::OK);
        assert(!find_scrim(screen, first->getRoot(), second->getRoot())); // NONE never covers.
        advance(20);
        assert(!manager.isSwitching() && lv_obj_get_x(second->getRoot()) == 0);
        assert(lv_obj_get_x(first->getRoot()) == 0); // No parallax offset left on the page beneath.

        // A child with the page's own type in between does not lose Second's override.
        assert(manager.push("Third") == Result::OK);
        advance(130);
        assert(lv_obj_get_x(third->getRoot()) > 0 && lv_obj_get_x(third->getRoot()) < 294);
        advance();
        assert(manager.pop() == Result::OK);
        advance();
        assert(manager.pop() == Result::OK); // Leaves Second the way it entered: at once.
        advance(20);
        assert(!manager.isSwitching() && manager.getCurrentPage() == first);
        assert(lv_obj_get_x(first->getRoot()) == 0);

        assert(manager.push("Second") == Result::OK); // Plain push: the page's own (global) type again.
        advance(130);
        assert(manager.isSwitching() && lv_obj_get_x(second->getRoot()) > 0);
        advance();
        assert(manager.pop() == Result::OK);
        advance();
    }
    advance();
    assert(lv_obj_get_child_count(screen) == children);
}
} // namespace

/** @brief Run the framework regression with a headless LVGL display. */
int
main() {
    lv_init();
    auto* display = lv_display_create(294, 126);
    assert(display);
    std::array<uint16_t, 294 * 20> pixels{};
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, pixels.data(), nullptr, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    auto* group = lv_group_create();
    lv_group_set_default(group);
    check_manager();
    check_push_scrim();
    check_push_with_anim();
    lv_group_set_default(nullptr);
    lv_group_delete(group);
    lv_display_delete(display);
    lv_deinit();
}
