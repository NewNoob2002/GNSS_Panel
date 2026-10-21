# P4 页面与按键迁移

2026-09-24 完成 P4 离线切片；2026-09-28 在 RK3506J 测试板完成按键、修正版九页运行及 Debug 监视器复核，P4 显示与测试板导航验收按既有记录关闭。本文历史用法核对至源码 `7b49532`；后续两键更新见下节。源代码来源及改动边界见 [SOURCE.md](../../../App/SOURCE.md)。

## 开发分支提交前复审

复审基线为 `a063f10`，相对 `master` 的 5 个提交重点核对页面生命周期、状态发布、两键输入、初始化报告、模拟电源流程和构建入口；未发现阻断问题。工作区另有 11 个文件将显示驱动类型统一为 `Rm690a0` / `RmIo`，声明、定义、C/C++ 调用点及测试已同步，不改变运行行为。

本次重新验证：默认 Debug CTest **9/9**、根工程 SDL 的 ASan/UBSan/泄漏检查 CTest **9/9**、benchmark CTest **1/1**；SDL dummy-video 随机模式 seed=42 运行 12 秒后 SIGTERM 正常退出，无应用 ERROR 或检查器错误。Buildroot 工具链在既有 Docker 镜像内完成 ARM Release 的 `rk3506_app`、`button_probe`、`display_test` 构建，开启 Debug monitor。产物位于 `out/final-review-default/`、`out/final-review-host/`、`out/final-review-demo/`、`out/final-review-arm/`。

11 个改动源文件的 clang-format 检查及 `git diff --check` 通过。对三个显示模块运行 clang-tidy，仍存在既有有符号位运算、窄化转换、其他结构体命名和 POSIX 宏告警；本次重命名没有新增告警，不能称为全仓静态检查无告警。本轮未部署或操作测试板，最新实屏、物理按键手感及长期稳定性仍未验证。

## 当前工作区收尾验证

已复核状态发布、电池动画生命周期、两键输入、初始化报告、信息页滚动及 CMake 拆分。独立构建验证：默认 `application/` 入口 Debug CTest **9/9**；根工程 Debug/SDL 在 ASan、UBSan 和泄漏检查下 **9/9**；benchmark 配置 **1/1**。带检查器的 SDL dummy-video 随机模式（seed=42）运行 12 秒后 SIGTERM 正常退出，无检查器错误或状态发布失败。`git diff --check` 通过。

使用既有 `rk3506-sdk:ubuntu20.04-p2` 镜像及 Buildroot GCC 12.4.0 工具链，ARM Release 的 `rk3506_app`、`button_probe` 编译通过，启用 `RK3506_DEBUG_MONITOR`；ELF32 ARM hard-float ABI、解释器和依赖库与目标 sysroot 匹配。产物及日志分别保存在 `out/workspace-review-default/`、`out/workspace-review-host/`、`out/workspace-review-demo/`、`out/workspace-review-arm/`。

GitHub Actions 的 [Host tests](../.github/workflows/host-tests.yml) 保留默认应用和 benchmark 两项矩阵检查，新增根工程 SDL 构建、ASan/UBSan CTest，以及 dummy-video 下 seed=42 的 12 秒随机状态与 SIGTERM 正常退出检查。SDL 检查同时拒绝检查器错误和应用 ERROR 日志；远端运行结果以对应提交的 Actions 状态为准。

本轮未部署或操作测试板；后文保留的既有板端日志不代表最新产物已通过硬件验证。

## 当前两键测试板操作

`rk3506_app` 与 `button_probe` 统一映射 V− 为 Function、V+ 为 Power；MENU/ESC 和重复事件忽略。evdev 按下/松开传给 `App::on_key()`，主循环用单调毫秒调用 `poll_keys()`：

| 按钮动作 | 操作 |
| --- | --- |
| V− 单击 | 下一焦点，循环 |
| V− 双击 | 返回 / 取消 |
| V+ 单击 | 选择 |
| V+ 双击 | 提交；Power 弹窗中确认选中的 Shutdown / Reboot |
| Startup 长按 V+ 约 2 秒 | 启动演示；短按、松开或取消不启动 |

沿用 20 ms 去抖及 250 ms 双击窗口。Power 弹窗默认选中 Cancel，先用 V− 移到 Shutdown 或 Reboot，再双击 V+。旧四键适配只有 `KEY_ENTER` 提供 Commit，而本板的四键没有该键，所以无法确认关机/重启。当前两键适配复用同一 Commit 路径；关机/重启仍是模拟：SaveConfig 完成后 Off 回 Startup、Reboot 回 Dialplate，不改变 Linux 电源状态。

两键改动主机 CTest 9/9、ARM Release 构建通过；UART 部署的两个程序 SHA256 与主机一致，旧程序已备份。板端以临时 FIFO 提供定时 ARM32 evdev 事件，已验证 V+ 短按不启动/长按启动、单击 Power 不提交、V− 双击取消、模拟 Off/Reboot 各执行一次；退出码 0，SPI 错误/超时均为 0。随后恢复 `/dev/input/event0` 并保持应用运行，实际按键手感和最新画面仍需现场复核。主机/板端日志位于 `out/deploy-two-button/`，不入版本库。真实产品 GPIO line/有效电平和电源时序仍待定型。

## 当前信息与初始化演示

Startup 的 FW/HW/BAT、SystemInfos 六张信息卡及 StarMap 星座计数已填合理演示值，页面标 `DEMO`；如固件 1.0.0、电量 85%、电压 7.60 V、经纬度 31.2304 N / 121.4737 E、IPv4 192.168.4.2、存储 1.2/8 GB。不是实测值，上板接入后由对应 Model 数据更新。

Startup 最新主机样式采用 GNSS RTK 标题/DEMO 徽章、两张 FW/HW 卡和带图标/电量条的电池卡；右侧 106×94 的完整 Start 卡为长按目标，显示电源图标、居中进度环、Start/Запуск 与 Hold 2s/Удерж. 2 с。复用既有 PageStyle、字体及图像，无新增依赖或资源。顶部仍预留 26px，空闲 10 秒状态栏覆盖标题区，主体不位移；2 秒长按、释放取消及开机流程保持原行为。主机 Debug/SDL 构建、CTest **9/9** 和 SDL 原生指针事件检查通过，检查在提示文字处按下能推进进度、提前释放回到 0%。英俄/空闲/按下拼图见 `out/host-startup-style/preview.png`，六条主动 LSP 探测超时，结论不确定。本次样式未 ARM 构建或部署。

SystemLoading 按 **Logo 800ms → 五节点各 600ms → 100% 200ms → 完成窗口 1s → Dialplate** 执行。配置/服务/GNSS/网络/收尾均记录 `begin` 和 `OK`/`FAILED`，各窗口切换也有日志。网络预留节点固定演示失败：当前标题、进度与节点红色，失败节点在后续阶段及完成窗口保持红色；最终显示 `Started with warnings`，仍进入工作页。离页取消计时器/动画，重入清空失败状态。实际平台初始化尚未接入，未来操作应有界并保留日志/继续执行规则。

本轮主机 CTest **9/9** 与 ARM Release 构建通过；英俄布局、单步日志顺序、全成功/多失败着色及重入回归通过。板端临时 FIFO 定时事件验证网络失败后仍执行收尾并进入 Dialplate，同时复查两键取消与模拟 Off/Reboot；测试退出 0，SPI 错误/超时 0。产物 SHA256 `598cdb0072fa4e20f811c2929f8f4c572fac330e667114e9fdde7d16836a483b`，上版 `/tmp/rk3506_app.previous-1791429508` 已保留；已恢复真实 `/dev/input/event0`，当前 PID 1396，日志 `/tmp/p4-live-loading.log`。记录及最新拼图在 `out/deploy-loading/`。LSP 三条主动探测超时，结论不确定；现场画面、物理手感和长期稳定性仍需操作者复核。

## 初始化结果查询（主机验证，尚未部署）

`Pages/SystemInfos` 是固定 294×126 的滚动视口，纵向排列 Work → GPS → Wi-Fi → Battery → Storage → System 六张信息卡；焦点变化滚动当前页，不切换路由子页。按最新要求，初始化标题与五条结果行已删除。System 卡仍显示 `Init errors`，统计最近初始化报告的失败数。Next/Down/Function 单击按可见顺序前进，Previous/Up 后退，两端循环；Power 单击或 Function 双击返回。仅此页开启竖直滚动，焦点通过 LVGL 原生 scroll-to-view 显示完整条目。

结果由现有 System 服务保存在固定大小的 `SystemState` 中，通过页面 Model 的 Account 订阅/拉取；SystemLoading 卸载、SystemInfos 离开再进入或切换语言后仍可查看同一报告。开始新的初始化尝试时清空旧报告；未执行的节点保持 NOT RUN。仅保留当前进程的最近一次尝试，每节点详情上限 95 字节；不是历史日志浏览器，也不写文件或存储。服务拒绝错误来源、越界节点、无效状态及未终止消息。

最新根工程 Debug/SDL 构建与主机 CTest **9/9** 通过，回归覆盖六卡顺序、首个 Next/Previous、正向滚动、两端循环、英俄布局、初始化错误数及返回。额外临时 SDL fixture 向真实事件队列注入 Down/Up，并使用生产 `platform/sdl/main.cpp` 事件循环，确认 Down: Work → GPS、Up: GPS → Work 和完整六卡循环。此前仍运行的 `out/host-init-info/application/simulator` 是导航修正前的旧二进制，本轮已在此路径重建并重启。验证记录/键盘 fixture 位于 `out/host-info-clean/`。`out/host-info-scroll/` 和旧 diagnostics 截图保留为历史；本轮未 ARM 构建或部署板端。

以下“已接入”及 2026-09-28 内容保留旧四键版本的阶段记录。

## 已接入

- 原 `PageManager` 的页面栈、生命周期和切换逻辑，原 `DialplateView`、资源、状态栏布局及数字组件。状态栏由 `StatusBarState` 更新，无有效值时隐藏卫星数、时间和电量；当前 `rk3506_app` 每 200 ms 用 `DemoStatus` 更新合成数值和 Wi-Fi/录制图标，定位文案保留 `DEMO`。这些变化不是硬件或业务数据；SDL 预览也接入此生产者，并提供可选随机状态模式（见下文）。
- 九个页面的 View 布局已迁入：主界面、工作设置、记录配置、系统信息、关机、星图、启动、系统加载（原 `HardwareCheck`）、保存配置。中间页现名 `Pages/SystemLoading`：没有真实硬件检查结果时不称“硬件检查”。主界面五个按钮沿用旧页面名路由；无按键时可静态选页。未知数值显示为 `N/A` 或 `-`；业务动作尚未接入。
- 测试板原外接按键位于 P1_B3，曾与 UART2 TX/sysfs 冲突；当前输入代码从 Linux evdev 读取 `KEY_VOLUMEUP` / `KEY_VOLUMEDOWN` / `KEY_MENU` / `KEY_ESC`：V+/V-/ESC 只处理按下，分别移到下一焦点、上一焦点、返回；MENU 处理按下和松开，在普通页面确认，在 Startup/Shutdown 支持约 2 秒长按演示。重复事件忽略。此测试板已确认 `adc-keys` 对应 `/dev/input/event0`；旧 GPIO1_B4 方案不再是应用输入入口。
- `rk3506_app` 复用 P3 的 SPI mode 3 屏驱动和 LVGL display port。`--display-only --page Pages/NAME` 跳过按键设备并选择单页；省略 `--page` 从 Startup 待机页开始。SIGINT/SIGTERM 退出，释放 evdev、GPIO、SPI 和 LVGL 资源。状态栏业务数据未接入，屏上显示的是演示 UI。`rk3506_app` 当前使用 `LOAD_ANIM_MOVE_LEFT`、320 ms、`lv_anim_path_ease_in_out` 作为默认导航动画，各页按启动、菜单或返回情境覆盖；历史无动画及 250 ms ease-out 配置不能用于判断当前行为。

## 当前源码验证

源码 `7b49532` 的主机默认配置 CTest **7/7 通过**：`lvgl_baseline`、`display_protocol`、`linux_display`、`app_navigation_preview`、`app_status`、`linux_button`、`app_dialplate_preview`；独立 benchmark 配置 CTest **1/1 通过**：`lvgl_demo_smoke`。导航测试覆盖九页流程、长按取消、焦点/返回、语言切换及两处布局回归；`app_status` 只检查合成快照，不证明真实数据或状态栏长期更新正确。SDL `simulator` 构建通过。

既有记录中 ARM32 Buildroot 工具链编译了 `rk3506_app`、`button_probe` 和测试程序，两个按键测试在 QEMU 通过；此次文档核对没有重新执行 ARM 构建或板端测试。`out/p4/screens.png` 是本机生成的九页软件预览，未在此次核对中重新生成，不能代替最新源码或实屏观察。CI 已配置默认应用和 benchmark 两种构建；未核实 `7b49532` 的远端运行结果。

## 2026-09-28 实板按键试运行（历史记录）

以下保留当次版本和操作结果；“当时尚未确认”的事项不能直接当作当前进度，验收结论见本文末尾。

通过 `/dev/ttyACM0`（1500000，未触发复位）确认 `ATK-DLRK3506J Board`、Linux 6.1.118；板端 `/tmp/button_probe`、`/tmp/rk3506_app` 与本机 ARM 构建 SHA256 一致。`/proc/bus/input/devices` 中 `adc-keys` 为 `/dev/input/event0`，已有 evtest 记录四键按下/松开。限时运行 `/tmp/button_probe /dev/input/event0`，操作者各按一次，依次采到 `V+ next-focus`、`V- previous-focus`、`MENU confirm`、`ESC back`，采集结束后进程已停止。

以 SPI mode 3、32 MHz 请求档限时运行 `/tmp/rk3506_app /dev/spidev0.0 /dev/gpiochip1 32000000 /dev/input/event0`；日志记录焦点动作、MENU 进入 SystemInfos/RecordConfig/WorkSettings、ESC 返回 Dialplate。第二次专项运行记录 `V+ → MENU → Pages/Shutdown → ESC → Pages/Dialplate`，操作者确认关机页面实际显示且可返回。测试程序已退出；退出后屏幕保留最后画面，不代表应用仍响应按键。当次产物的页面加载动画配置为 `LOAD_ANIM_NONE`，进入/退出无动画是当时的预期行为；当前源码已恢复动画。当次尚未验收英俄切换、状态栏生命周期、实际业务操作及长时间运行。

随后用相同的板端产物以 `--display-only --page Pages/NAME` 顺序显示九页，每页限时 12 秒，均正常退出。操作者发现 RecordConfig 时钟图标偏离右侧选项栏、SaveConfig 品牌标志侵入状态栏；其他页面未报告异常。已在本机将时钟图标对齐到右侧选项栏、把 SaveConfig 标志和文案下移到 26 px 状态栏以下；主机 CTest 6/6 通过，SDL 1:1 本机预览经操作者确认这两处位置符合预期。修正版已通过串口 1500000 部署到 `/tmp/rk3506_app` 和 `/tmp/button_probe`，SHA256 与 ARM 构建一致；九页修正版均在 32 MHz 请求档限时运行并以 0 退出。截至该次逐页运行记录，仍缺少操作者对 RecordConfig、SaveConfig 两页修正版的逐页目视确认；不能仅以进程退出成功证明布局正确。后续 P4 关闭记录见本文末尾，完整逐页英俄目视回归仍另列 P6。

## 本机 SDL 页面预览

安装 SDL2 开发库后，在仓库根目录运行统一模拟器（`SIM_BACKEND=Demo`）：

```sh
cmake -S . -B build/sim-demo -DPLATFORM=Simulator -DSIM_BACKEND=Demo
cmake --build build/sim-demo
build/sim-demo/bin/main
# 持续随机状态验证；seed 可省略（默认 1），相同 seed 可复现。
build/sim-demo/bin/main --random-status 42
```

随机模式每 3 秒生成新的 0%～100% 电量、充电状态、卫星数、Wi-Fi 与录制状态；电压按演示范围 6.80～8.40 V 随电量变化。状态每 200 ms 经 `App → StatusService/DataCenter → Model → View` 发布，沿用 SDL/LVGL 主循环线程。Startup 和 StatusBar 共用带槽框及电池正极端头的样式：充电时槽框和填充均为绿色，1.2 秒从空槽增长到满槽、停留 0.3 秒后从空槽重新开始，数字仍显示实际电量；停止充电后恢复实际电量宽度及四级颜色，数据无效时清空槽并取消动画。重复发布和充电期间实际电量变化不重置循环。Startup 空闲 10 秒后显示 StatusBar，可同时观察两处。页面导航与长按操作由用户控制，Q 退出；数据属于 DEMO，不代表电池测量或充放电模型。`app_system_dash` 回归强制检查 0/9/10/19/20/49/50/100/101% 边界，运行 seed=42 的 128 组随机状态，检查两处颜色、实际数值、从空到满的循环、无效数据及页面离开/返回；`app_status` 检查 StatusBar 在充电时销毁/重建和取消动画。

窗口为与原 Simulator 相同的 294×126、1:1 像素尺寸，复用同一 `App` 与 LVGL 配置，不触碰板端设备。启动显示 Startup：鼠标左键、Enter 或中键 **按住约 2 秒**，经过 SystemLoading（Logo、五阶段初始化及完成窗口，约 5 秒）进入主表盘。主表盘的电源按钮进入 SystemDash；选择 Power 后弹出 Cancel / Shutdown / Reboot，默认 Cancel。移动焦点到目标动作后用 Ctrl+Enter 提交，SaveConfig 演示结束后 Shutdown 回 Startup、Reboot 回 Dialplate。鼠标左键点击可操作按钮、中键确认焦点、右键返回、滚轮移动焦点；键盘方向键移动焦点、短按 Enter 确认、Esc 返回、Q 退出。工作设置/记录配置的方向按钮可预览选项；SystemInfos 六卡按焦点滚动并可点击返回，StarMap 同样可聚焦返回；SystemDash 的 Settings 进入语言设置。**整个开关机流程只演示页面**，不执行真实硬件检查、配置保存、系统断电或重启；未接入业务后端的模式、录制、Wi-Fi 按钮仍禁用。页面只通过焦点按钮导航，不另设翻页快捷键。页面管理器沿用 [X-TRACK](https://github.com/FASTSHIFT/X-TRACK/tree/main/Software/X-Track) 同系生命周期/路由机制。SDL 窗口不能替代真实屏幕遮挡和时序验收。

## 最新版本实屏回归

P4 已有显示与导航验收记录；以下用于新产物回归，不要求重新采用 GPIO1_B4 改接方案。逐页检查布局和外壳遮挡可使用 `--display-only`，无需按键设备。执行前确认目标、操作授权、电平/共地及 SPI0 独占条件。

回归沿用已目视验证的 SPI mode 3、32 MHz 请求档和经目标系统核实的设备节点，限时运行：

```text
timeout -s TERM 90 ./rk3506_app --display-only --page Pages/StarMap /dev/spidev0.0 /dev/gpiochip1 32000000
```

这会操作 RST/DC、重新初始化并写屏。验证按键前，先在目标系统通过 `/proc/bus/input/devices` 与受控按键事件确认正确的 `/dev/input/eventN` 和 V+/V− 键码，不要假设 `event0` 或沿用旧 `/dev/gpiochip1` 输入方式。限时运行 `timeout -s TERM 30 ./button_probe /dev/input/eventN`，检查 V+ 的 Power press/release、V− 的 Function press/release，MENU/ESC 和重复事件应忽略；再用 `timeout -s TERM 90 ./rk3506_app /dev/spidev0.0 /dev/gpiochip1 32000000 /dev/input/eventN` 检查单击焦点/选择、双击返回/提交及 Startup 长按启动。若板上没有相应 evdev 键码，需要先核实接线及内核输入配置，不能仅靠应用编译结果判定按键已接通。

P3 benchmark 在此请求档正常显示并完成整轮测试，但其帧率不能代替 P4 页面实测。最新产物仍需核对画面、动画、刷新时长和输入响应；47 MHz 请求档曾无法点亮屏幕，不能作为提速方案。物理 CS/DC/SCLK 波形尚未测量。

## 已记录的 P4 验收与剩余项

既有 P4 显示与测试板导航验收记录已关闭：修正版已重新部署；九页均以 32 MHz 请求档运行退出；操作者已现场验证 V+/V-/MENU/ESC 导航、页面返回及 Debug 监视器显示的 FPS、CPU、内存指标均正常。运行日志记录了启动、加载、关机、保存配置和多页返回路径；另有一次 LVGL `active screen was deleted` 警告，需在 P6 稳定性阶段复现并判断是否为退出清理时序。该记录不是 `7b49532` 的新硬件验证，也不证明逐页英俄像素布局、长期状态更新或最新恢复动画已通过实屏验收。这些回归与退出/重启、10 次启动、8 小时稳定性检查列入 P6。实际业务数据与设置回执属于 P5；量产 PCB 的按键/电源方案仍待定型。
