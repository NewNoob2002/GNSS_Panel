#ifndef APP_COMMON_LOGO_REVEAL_H
#define APP_COMMON_LOGO_REVEAL_H

#include "lvgl.h"
#include "UI/Resource/LogoParts.h"

namespace app {
/** @brief UI 线程的静态 Logo 播放器；借用表，拥有容器，单动画驱动全部零件与窄高光。 */
class LogoReveal {
  public:
    struct Timeline {
        uint32_t background_ms = 150;
        uint32_t group_interval_ms = 50;
        uint32_t group_duration_ms = 250;
        int32_t rise_px = 6;
        bool highlight = true;
        uint32_t highlight_start_ms = 900;
        uint32_t highlight_duration_ms = 300;
        uint32_t total_ms = 1300;
    };
    LogoReveal() = default;
    ~LogoReveal();
    LogoReveal(const LogoReveal&) = delete;
    LogoReveal& operator=(const LogoReveal&) = delete;
    /** @brief 创建固定位置零件；parent 和资源表必须有效且比本组件存活更久。 */
    void create(lv_obj_t* parent, const logo_parts_t& logo, const Timeline& timeline);
    /** @brief 从黑色背景重播；分配动画失败时直接应用静止终态。 */
    void play();
    /** @brief 删除借用 this 的动画，供离页及缓存页停止使用。 */
    void stop();
    /** @brief 停止并删除容器及子对象；重复调用安全，必须早于 parent 删除。 */
    void destroy();
    lv_obj_t* container() const { return container_; }

  private:
    static void tick(void* self, int32_t ms);
    static void draw_highlight(lv_event_t* event);
    void apply(uint32_t ms);
    const logo_parts_t* logo_{};
    Timeline timeline_{};
    lv_obj_t* container_{};
    int32_t highlight_x_ = -22;
    bool highlighting_ = false;
};
} // namespace app
#endif
