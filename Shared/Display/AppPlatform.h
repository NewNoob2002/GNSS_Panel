#ifndef APP_PLATFORM_H
#define APP_PLATFORM_H

class P4App;

// HC32 平台层：把 systemInfo / HAL 接到共享应用（Shared/App，来源 ATK_RK3506）。
// 均在主循环（LVGL）线程调用。

/** 在 lv_init() 与显示端口初始化之后调用一次：创建共享应用并进入 Startup 待机页。 */
void App_Init();

/** 每轮主循环调用（含等待开机期间）：运行核心定时器、采样两个按键、发布快照与电源事件。 */
void App_Update();

/** 销毁共享应用并清理页面（PC 模拟器退出时调用；固件不退出，不调用）。 */
void App_Deinit();

/** 共享应用实例；App_Init() 之前为 nullptr。PC 模拟器用它注入键盘/鼠标操作。 */
P4App* App_Get();

#endif // APP_PLATFORM_H
