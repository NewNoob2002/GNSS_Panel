# 发布说明

## 3.0.0

适用产品：X1Pro、XBase；X1 仍为 1.30。以下内容依据本次工作区相对 `HEAD` 的改动与最近五次提交核对，其中 `ebfa490b`、`0e1d31ef`、`d6ec2d2f` 的切屏与入口展开一并纳入本版。

### 开机与页面过渡

- Startup 待机页到 SystemLoading 加载页使用 300 ms `FADE_THROUGH`：全屏单色遮罩逐渐盖住待机页，加载页根保持隐藏，到黑色终态后显示加载页并移除遮罩；页面根不移动，不做整页透明度混合，不分配离屏图层。
- PageManager 新增 `setFadeThroughColor`，默认黑色；纯色过渡不受 `setScrimOpa(0)` 影响。管理器、遮罩或页面根删除时清理过渡，避免留下遮罩或隐藏页面。
- Logo 动画与完成计时从 `onViewDidAppear` 开始，保留完整 1300 ms Logo 展示；`onViewWillAppear` 只准备黑底和初态，自检与完成停留时间保持原约定。
- PageManager 支持四向 `PUSH_*` 视差推入：新页完整移动，旧页只移动屏幕距离的 1/3。`PUSH_*` 与 `OVER_*` 在露出的旧页条带上叠加压暗遮罩，结束即删除，遮罩深度可由 `setScrimOpa` 配置。

### 首页入口与返回

- 首页焦点框支持按压挤压、松开回弹，并从入口展开成不透明页面背景；展开曲线总时长 500 ms，在约 350 ms 盖满屏幕时导航到子页，剩余几何动画在首页隐藏后结束。经过入口展开的导航使用 `NONE`，页面自身保留 `PUSH_LEFT` 供其他导航路径使用。
- 返回首页时保留满屏面板，等子页卸载、切换结束后的首帧刷新完成通知（`LV_EVENT_REFR_READY`）再启动收回，保证可见的完整 350 ms。收回填充淡出、橙框淡入；慢卸载和慢首帧不占动画时间，离页/卸载取消通知，换焦点仍可中断收回。

### 系统页与屏幕亮度

- SystemDash 改为左侧 88×77 px 亮度主卡、右侧三行网格：语言、Wi-Fi、电源与图标返回。主卡使用独立的 `ui_sun_large` 大太阳和五段短线，亮起 1..5 段对应 20%..100%；英俄语言表新增 Brightness / Яркость。
- 点击亮度循环 20%、40%、60%、80%、100%；焦点链按亮度、语言、Wi-Fi、电源、返回排列。Wi-Fi 不可控时仍显示当前状态，整行文字与图标置灰并退出焦点链，能力变化不重建控件。
- 关机/重启按住确认窗采用动作色细边框、较亮文字及聚焦/按压底色，按钮底部 3 px 进度轨道取代整块填充。重启按平台能力显示，HC32 产品不提供重启；长按确认仍为 2 s，提前松开取消。
- 设置链路为 `dp::Settings → Board → HAL::Display_SetBrightness → DCS 0x51`。Settings 状态增加亮度，Board 增加精确尺寸 `BrightnessCommand` 并只接受五档；语言命令的亮度 0 表示保持当前亮度，写入失败不提交新的设置状态。
- HC32 与 SDL Board/Demo 共用 `HAL_Board.h` 中的 CIE 1976 L* 五档表：`0x33 / 0x53 / 0x7F / 0xB8 / 0xFF`，最低值为 `0x33`，采用 L* 等距后反算亮度的非线性映射。开机默认 100%，仅存 RAM，不写 Flash；手动设置先回收显示 DMA 并终止开机渐亮，屏幕复位恢复所选值。Linux Demo 仅保留模拟设置值。

### 待机页充电功率

- 本机 BQ40Z50 新增 `AverageCurrent(0x0B)` 有符号平均电流读取，单位 mA，充电为正；HAL 电池采样、HC32/Board 与 Demo 细设备、Battery 领域状态增加 `current_ma/current_valid`，不增加新的状态写入者。
- Startup 充电时显示 CHG；电池、电流均有效且平均电流 ≥ 50 mA 时，旁边显示一位小数功率 W，使用本机电压乘电流计算流入电池的功率。其余情况回退两位小数电压，电池样本无效显示 BAT / N/A；MP2762A 不参与功率计算。
- 电流单独读取失败不使温度/SOC/电压样本失效；核心样本失败或过期时电流失效。领域保留原始读数，Startup Model 按最终文本去重，功率使用有界电压校验与 64 位整数乘法。详见[待机页充电功率](battery_charging_power.md)。

### 兼容性与回归

版本仍由产品 `product.cmake` 提供，固件和 Board 模拟器生成 `SOFTWARE_VERSION "V3.0.0"`；模拟器 `[0-9.]+` 正则可解析三段版本。运行时 16 字节和 INFO1 的 10 字节软件版本字段均可容纳该字符串，面板帧布局与命令未改变，无须修改版本消费代码。

- 新增 `app_startup_power_test`，覆盖功率、49/50 mA 阈值、异常电压/电流回退、文本去重与英俄布局；`app_panel_return_test` 覆盖慢卸载/首帧后的收回几何、透明度及可见时长。
- 扩展 PageManager 纯色过渡与销毁清理、SystemDash 亮度/Wi-Fi/确认窗、HAL 精确尺寸与亮度参数、领域变化发布、BQ40Z50 有符号读取与失败/过期回归；`app_transition_bench` 增加纯色过渡与静态重绘对比，仅手动运行，不注册 CTest。

本次使用默认 OEM `singularxyz`，按对应 preset 配置到独立目录并构建：

| 配置 | 产物 / 结果 |
|---|---|
| `x1pro-release`，`build/wJ-x1pro` | `MCU_PRO_3.0.0.elf`，text/data/bss = 309380 / 520 / 168700 B |
| `xbase-release`，`build/wJ-xbase` | `MCU_XBASE_3.0.0.elf`，text/data/bss = 308936 / 520 / 168700 B |
| `simulator-x1pro`，`build/wJ-sim` | `bin/main` 构建通过，生成头文件为 `V3.0.0`；SDL dummy 启动/清理回归通过 |
| `linux-host`，`build/wJ-linux-host` | 构建通过，CTest 32/32 |
| HC32 主机回归，`build/wJ-host-tests` | 构建通过，CTest 20/20（含模拟器冒烟、状态归属与分层检查） |
| 共享核心，`build/wJ-app-core-tests` | 构建通过，CTest 4/4 |
| `python3 tests/check_layers.py .` | 默认六个已配置构建目录的 File API 与静态分层检查通过 |

本版未刷写硬件，未测量 HC32 上的新过渡帧率、实际亮度观感与充电功率精度；历史迁移/可靠性记录中的实机结果不代表这些新增行为已上板验收。
