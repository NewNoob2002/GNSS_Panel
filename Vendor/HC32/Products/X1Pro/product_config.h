#pragma once

// X1Pro：带屏（AMOLED 294x126）面板，完整功能基线。XBase 在此基础上裁剪。

#define PRODUCT_NAME                        "X1Pro"
#define FIRMWARE_NAME                       "PRO_Panel"
#define PRODUCT_HAS_DISPLAY                 1

/* 未开机时电源灯是否指示电量（关机驻留态始终熄灭） */
#define PRODUCT_LED_BATTERY_BEFORE_POWER_ON 1
/* 按键/关机监控任务周期 */
#define PRODUCT_MONITOR_PERIOD_MS           15

/* 协议：0x0B 面板请求开关 Wi-Fi；0x0D 星图卫星数 */
#define PRODUCT_PROTOCOL_WIFI_CONTROL       1
#define PRODUCT_PROTOCOL_SATELLITE          1

/* 页面功能 */
#define PRODUCT_PAGE_STAR_MAP               1
#define PRODUCT_PAGE_BATTERY_WARNING        1
#define PRODUCT_SHUTDOWN_WIFI_BUTTON        1
#define PRODUCT_RECORD_SETTINGS             1

/* 画面在屏幕玻璃上整体右移的像素数（偶数）：X1Pro 外壳遮挡屏幕左缘。
 * 右侧移出玻璃的部分被裁掉；各页面右侧约 6 px 为背景留白，不裁到内容。 */
#define PRODUCT_DISPLAY_X_SHIFT             4
/** @brief 首帧完成后渐亮；上板验证 DCS 0x51 后可按产品关闭。 */
#define PRODUCT_DISPLAY_BRIGHTNESS_FADE     1

// 2.0 修复X1pro充电问题， 增加强制关机机制防止卡死无法关机
// 2.0.1 增加关机充电提示和开机按键提示动画，增加设备信息页面
// 2.1.0 使用新框架，优化响应速度和UI动画帧率
// 2.2.1 增加不同关机情况的调试信息，需要使用新boot
// 2.3.0 优化lvgl画面撕裂的情况
// 2.3.3 修复电台配置界面显示不对应
// 2.3.5 修复4G设置不成功
// 2.4.0 增加静态记录采集设置
// 2.4.1 增加英俄语言动态切换，完善Simulator独立编译，优化页面定时器和资源释放
// 2.4.3 关机页增加 Wi-Fi 设置，电池温度达到 65 ℃ 时告警
