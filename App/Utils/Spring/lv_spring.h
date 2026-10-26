/** @file lv_spring.h @brief 只在 LVGL 应用线程调用的弹簧适配器。 */
#pragma once
#include <array>
#include "Spring.h"
#include "lvgl.h"

namespace app {
/** @brief 固定目标曲线（250 ms、bounce 0.15 的归一化形状），duration 结束精确返回 end_value。 */
int32_t lv_anim_path_spring(const lv_anim_t* animation);

/** @brief 将四个弹簧绑定到对象 x/y/w/h；对象和驱动任一先销毁都安全，空闲时无定时器。 */
class LvSpring {
  public:
    enum Axis : unsigned { X, Y, Width, Height };
    /** @brief 静止回调：一次运动结束（含 retarget 时已在目标而直接落位）时调用，调用前定时器已停止。 */
    using SettledCallback = void (*)(void* user_data);
    explicit LvSpring(SpringParams params = SpringParams::from_duration(250, 0.15f));
    ~LvSpring() { unbind(); }
    LvSpring(const LvSpring&) = delete;
    LvSpring& operator=(const LvSpring&) = delete;

    /** @brief 借用对象并安装删除回调；失败返回 false，旧绑定先解除。 */
    bool bind(lv_obj_t* object);
    /** @brief 解除删除回调及定时器，不删除借用的对象。 */
    void unbind();
    /** @brief 四轴直接落位并停止定时器。 */
    void snap(int32_t x, int32_t y, int32_t width, int32_t height);
    /** @brief 保速重定向；定时器分配失败时直接落位并返回 false。 */
    bool retarget(int32_t x, int32_t y, int32_t width, int32_t height);
    /** @brief 停止推进，保留当前对象位置；不触发静止回调。 */
    void stop();
    /** @brief 设置静止回调；回调中可再次 retarget、stop 或解除绑定。 */
    void set_settled_callback(SettledCallback callback, void* user_data) {
        settled_cb_ = callback;
        settled_user_data_ = user_data;
    }
    /** @brief 设置逐帧回调：每次推进并写入对象几何后调用（静止回调之前），user_data 与静止回调共用。 */
    void set_frame_callback(SettledCallback callback) { frame_cb_ = callback; }
    bool running() const { return timer_ != nullptr; }
    lv_obj_t* object() const { return object_; }
    const SpringValue& value(Axis axis) const { return values_[axis]; }

  private:
    static void on_timer(lv_timer_t* timer);
    static void on_delete(lv_event_t* event);
    void apply();
    bool settled() const;
    void notify_settled();
    std::array<SpringValue, 4> values_;
    lv_obj_t* object_ = nullptr;
    lv_timer_t* timer_ = nullptr;
    uint32_t last_tick_ = 0;
    SettledCallback settled_cb_ = nullptr;
    SettledCallback frame_cb_ = nullptr;
    void* settled_user_data_ = nullptr;
};
} // namespace app
