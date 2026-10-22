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
    lv_group_set_default(nullptr);
    lv_group_delete(group);
    lv_display_delete(display);
    lv_deinit();
}
