# P3 屏驱动：实施与验收

日期：2026-09-21。P3 源码实现与离线验证完成；2026-09-24 以 SPI mode 3 完成实屏点亮和测试图目视检查。边缘受外壳遮挡，物理波形、最高速率与供电轨未实测。未烧录或修改 SDK、设备树和启动参数。

本文保留 P3 阶段记录（主机测试当时为 3/3）；后续 P4 页面显示与测试板 evdev 验收见 [P4 记录](P4_APPLICATION.md)。当前源码 `7b49532` 的默认主机测试为 7/7，独立 benchmark 为 1/1；此次文档核对未重新执行 ARM 构建或硬件测试，不改变下文 P3 物理验收的待测边界。

## 计划与结果

| 步骤 | 实现 | 状态 |
| --- | --- | --- |
| 协议提取 | 双重硬复位、原 11 条命令、0x36/0x00、清黑 | 完成；逐项检查命令、参数及 1800 ms 等待 |
| Linux IO | spidev、GPIO 字符设备、硬件 CS、分段与错误清理 | 完成；系统调用替身检查配置、短传输及资源释放 |
| LVGL 显示 port | 126×294 控制器、294×126 UI、ROT_90、RGB565 SWAP=1、双缓冲与同步 flush | 完成；软件屏模型核对颜色、边框、旋转、两像素对齐和局部更新 |
| 测试入口 | 红绿蓝白黑彩条、黄色边框、1 TL / 2 TR / 3 BL / 4 BR 四角标签 | 已目视确认颜色和方向；边缘局部被外壳遮挡 |
| 构建与故障验证 | 主机 CTest、Buildroot ARM32、QEMU | 离线通过，见下方复现 |
| 上板验收 | 运行节点、电气条件、CS/DC 波形、实际速率、退出重启 | mode 3、500 kHz 点亮，退出及再次启动成功；物理波形与供电轨待测 |

## 2026-09-24 实屏测试结果

用户确认测试板与屏共地、IO 电平兼容且异常可断电恢复。板端为 `ATK-DLRK3506J Board`（ARMv7）；`/dev/spidev0.0` 对应 SPI0 CS0，`/dev/gpiochip1` 对应 GPIO1，运行 pinmux 为 SPI0 的 SCLK/MOSI/CS0，GPIO1_C4/C6 空闲；spidev 缓冲为 65535 字节。通过临时 SSH 将 SHA256 为 `1ab54dd557853ce250e017eedf38853beb77436849e44f48f710fef520712da6` 的程序传到板端 `/tmp/display_test`。

运行 `timeout -s TERM 90 /tmp/display_test /dev/spidev0.0 /dev/gpiochip1 500000`，程序退出码 0；用户目视反馈屏幕纯黑、无数据。板端 SPI0.0 统计无错误或超时，首次运行后累计 TX 为 5,421,352 字节（未记录运行前基线，不将累计量全归因于本次测试）。随后以同样速率限时 12 秒诊断运行，TX 增加 149,868 字节、错误仍为 0；运行中 GPIO1_C4/C6 由 `rm690a0` 持有，两次采样均为高，退出后已释放。以上只证明内核接收了传输并申请了 GPIO，不证明屏端实际收到正确波形。

用户进一步明确：这是无背光控制的 AMOLED，SGM3849 负责屏供电；在供电正常、完成初始化后，`0x2C` 写入坐标窗口的像素数据应使屏自行显示。

对照已实屏运行的 `refer/01_lvgl_button/USER/LCD.c` 发现其实际设置 `SPI_MODE_3`，而首测程序为 mode 0；参考代码旁的“CPOL=0, CPHA=0”注释与 mode 3 不符。仅将 SPI 模式改为 mode 3，保持硬件 CS、接线、初始化命令和 500 kHz 不变。Buildroot ARM 产物 SHA256 为 `ed892f86320f6a78aff93bbce5d082d59dbbb5dc211b34e52b3c8c97baee021e`，传至板端 `/tmp/display_test_mode3` 并校验一致。限时 60 秒运行退出码 0；用户目视及照片确认红、绿、蓝、白、黑区域、黄色边框和四角标记方向正确。外壳挡住部分边缘，不能据照片确认每个边缘像素及边框完整性。随后限时 12 秒再次启动，退出码 0；第二次画面未单独拍照。单变量对照支持 mode 0 是本次黑屏原因，但未测屏端时钟与片选波形，也未实测 SGM3849 输出轨。测试结束后关闭临时 SSH 监听和主机 HTTP 服务，保留板端 `/tmp` 测试程序。

## LVGL benchmark 实屏性能（2026-09-24）

使用 `cmake -S application -B out/p3/demo-host -DRK3506_LVGL_DEMO=ON` 配置并以 `cmake --build out/p3/demo-host --target lvgl_demo` 构建主机版；板端需用下文的 Buildroot ARM32 工具链构建，再运行 `lvgl_demo /dev/spidev0.0 /dev/gpiochip1 32000000 4096`。demo 使用 8 ms LVGL 刷新周期、两块各占四分之一帧的绘制缓冲，并在屏幕右下角显示实际刷新 FPS 与进程 CPU 占用率。FPS 按 LVGL 完成有像素刷新回调的次数除以经过的单调时钟时间计算；不是屏幕扫描帧率。CPU 占用率基于进程 CPU 时间，显示值随测试场景变化。

| SPI 请求频率 | 5 秒窗口实测刷新率 | 结果 |
| --- | --- | --- |
| 500 kHz，16 ms 刷新 | 约 0.8 FPS | 有明显擦屏感 |
| 8 MHz，16 ms 刷新 | 约 25 FPS | 画面正常，仍有擦屏感 |
| 16 MHz，16 ms 刷新 | 约 39–40 FPS | 画面正常 |
| 32 MHz，16 ms 刷新 | 约 50–51 FPS | 画面正常 |
| 40 MHz，16 ms 刷新 | 约 50–51 FPS | 画面正常，与 32 MHz 相近 |
| 32 MHz，8 ms 刷新 | 动画场景约 62–64 FPS | 画面正常，用户确认明显更顺畅 |
| 47 MHz，8 ms 刷新 | 程序报告约 75–76 FPS | **屏幕无法点亮，不可使用**；退回 32 MHz 后用户确认恢复正常 |

32 MHz 请求、4096 字节分段、8 ms 刷新周期下，完整 benchmark 运行 **96.932 秒，完成 5,556 次有内容刷新，整轮平均 57.3 FPS**；LVGL 自带分场景加权分数为 **Weighted FPS 112**，两者不是同一个指标。测试程序退出码为 0。各场景 5 秒窗口约 22–94 FPS，CPU 占用约 6–32%，不能用单一场景的 62 FPS 代表全程平均。把分段从 4096 增至 16384 字节、或把绘制缓冲从四分之一帧增至半帧，均未带来明显的实屏改善，因此保留 4096 字节和四分之一帧配置。

板端 SPI0 父时钟显示 187.5 MHz；当前 Rockchip SPI 驱动使用偶数分频。依据驱动公式推算，32 MHz 与 40 MHz 请求均落在 6 分频（约 31.25 MHz），47 MHz 请求落在 4 分频（约 46.875 MHz）。**这些是源码和时钟树推算值，未用示波器测量 SCLK。** 4 分频时内核未报传输错误，仍发生黑屏；后续只采用已目视验证的 32 MHz 请求档。在该次 P3 benchmark 时，P4 页面实屏与按键验证尚未执行；后续结果见 [P4 记录](P4_APPLICATION.md)。

分段与 DMA 记录：板端 SPI0 的 DMA 通道已分配给 TX/RX，且未配置 polling；本版驱动对达到 FIFO 长度的传输自动选择 DMA，因此 4096 字节像素段具备 DMA 条件，小命令走中断。板端 `spidev.bufsiz=65535` 是用户态缓冲上限，不等于建议分段大小。`spidev` 会把传输长度向 DMA 对齐要求上取整，恰好 65535 字节会超过该上限；像素段还要求偶数字节。当前应用允许的分段上限为 32768 字节，而四分之一帧 LVGL 缓冲最多只有 18522 字节数据，增至 65535 不会让单次 flush 发送更多像素。保留 4096 字节默认值，等 P4 实际页面运行后再按场景评估。

## 代码与协议边界

- `application/display/rm690a0.c`：初始化、坐标检查、0x2A/0x2B/0x2C、像素分段。参数越界在发送前拒绝；传输失败使屏状态失效，后续写入被拒绝，必须重新初始化。
- `application/platform/linux_display.c`：SPI mode 3、8 bit、MSB first、硬件 CS；GPIO1 的 DC=22、RESET=20 以同一 line handle 申请。使用目标内核已启用的 GPIO CDEV v1，无新增 libgpiod 依赖。
- `application/display/lv_display.c`：保留原软件旋转、rounder 及两块 9261 像素缓冲。像素已由 LVGL 按线上顺序生成，协议层不再交换字节。不分配全屏影子缓冲。
- `application/display_test.c`：显示测试图、单调时钟、SIGINT/SIGTERM 退出；不接业务、按键或整机电源。
- `tests/`：协议屏模型、逐处故障注入、Linux 系统调用替身。测试使用全屏数组作断言依据，生产驱动没有该数组。

初始化数据依据 HC32 `Libraries/Adafruit-ST7735-Library/Adafruit_ST7789.cpp` 的 `generic_RM690A0`，源提交及调用链见 [迁移摘要](HC32_MIGRATION.md)。新实现未复制 Arduino/HC32 传输代码；原工程未修改。

### CS 与分段

一次命令加参数期间保持 CS；一次窗口更新从 0x2A 开始到最后一块像素发送结束都保持 CS。每个 ioctl 只有一个 transfer，非末段设置 `cs_change=1`，末段设置 0。D/C 仅在前一个同步 ioctl 返回后切换，窗口地址按高字节在前。

依据当前 SDK 的 `kernel-6.1/drivers/spi/spi.c` 中 `spi_transfer_one_message()`、`spi-rockchip.c` 的 `rockchip_spi_set_cs()` 及 `spidev.c` 实现：末 transfer 的 `cs_change` 可保持片选，Rockchip 在 CS 有效期间保留 runtime PM 引用；无载荷、`cs_change=0` 的 transfer 用于显式结束片选。**这些是源码层依据，尚未以逻辑分析仪证明实板波形。**

该实现要求运行期间 **SPI0 总线独占**，包括 CS1 上没有其他活动设备。跨 ioctl 无法锁住内核整条 SPI 总线；另一个设备的事务可能中断 CS 保持。程序对 spidev 文件使用非阻塞 `flock`，只防止遵守同一锁约定的进程重复打开，不等于总线独占。若要共享总线，应改用能够原子管理 D/C 与整个事务的内核驱动，不能仅取消该前置条件。

默认每段最多 4096 字节，整帧 74088 字节与非整行分段已离线核对；无需先修改 P2 内核启动参数。可显式指定 4–32768 的偶数字节上限，必须不超过板端 spidev 缓冲（含内核 DMA 对齐）与控制器限制。32768 仅在实际 `spidev.bufsiz` 等条件已满足时使用，本阶段未修改这些配置。

SPI 短传输视为 EIO；其他 ioctl 错误保留 errno，不重试可能已部分发送的数据。失败时尽力结束 CS，LVGL 仍释放绘制缓冲，然后主循环报错退出；关闭失败也以非零状态返回。正常重启执行完整 reset/init/清黑。同步 ioctl 的传输超时依赖当前内核控制器，未实现额外的用户态硬截止时间。SIGKILL、内核挂死或释放 ioctl 本身失败时不保证 CS 已恢复；需核查总线并按板端恢复路径处理，不能宣称自动恢复通过。

## 构建与离线复现

主机：

```sh
cmake -S application -B out/p3/host -DCMAKE_BUILD_TYPE=Debug
cmake --build out/p3/host -j8
ctest --test-dir out/p3/host --output-on-failure
```

P3 检查点主机 CTest **3/3 通过**：P2 `lvgl_baseline`、`display_protocol`、`linux_display`。检查点 `20f7f1c` 的 [GitHub Host tests](https://github.com/NewNoob2002/ATK_RK3506/actions/runs/35965777501) 同样通过 3/3。协议测试覆盖完整初始化/清黑、4/4096/32768 字节分段、74088 字节整帧、边界与非法输入、初始化和写入每一 IO 步的失败、旋转后颜色/边框与非对齐局部区域。Linux IO 测试覆盖打开失败、锁冲突、GPIO 申请失败、短传输、清理失败及 EINTR 延时；测试不访问真实设备。

ARM 沿用 P2 镜像 `rk3506-sdk:ubuntu20.04-p2`，项目挂载为 `/work`。容器内：

```sh
cd /work
br_host=/work/sdk/atk_dlrk3506_linux6.1_release_v1.3.1_20260326/buildroot/output/alientek_rk3506/host
br_target=/work/sdk/atk_dlrk3506_linux6.1_release_v1.3.1_20260326/buildroot/output/alientek_rk3506/target
cmake -S application -B out/p3/arm \
  -DCMAKE_TOOLCHAIN_FILE="$br_host/share/buildroot/toolchainfile.cmake" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build out/p3/arm -j8
qemu-arm-static -L "$br_target" out/p3/arm/lvgl_baseline
qemu-arm-static -L "$br_target" out/p3/arm/display_protocol_test
qemu-arm-static -L "$br_target" out/p3/arm/linux_display_test
"$br_host/bin/arm-buildroot-linux-gnueabihf-readelf" -h -l -d out/p3/arm/display_test
```

产物：`out/p3/arm/display_test`，Buildroot GCC 12.4.0，ARM32 EABI5 hard-float，加载器 `/lib/ld-linux-armhf.so.3`。QEMU 只证明 ABI、协议与软件错误路径，不能模拟真实 SPI/GPIO 时序或屏幕响应。构建日志在 `out/p3/host-build.log`、`out/p3/arm-build.log`。

## 待上板验收

先按 [硬件定型记录](../board/rk3506j/HARDWARE.md) 确认具体测试板、恢复路径、运行 DTB、电平/共地、供电、GPIO consumer 和 pinmux；确认 SPI0 总线独占。设备节点与 GPIO bank 不能仅凭 `/dev/gpiochip1` 的编号推断。SPI 速率由模块规格或有效硬件依据确认，程序不默认采用旧库的 32 MHz。

仅在上述条件确认且得到目标板操作授权后运行以下入口；参数为板端核实值：

```text
display_test SPI_DEVICE GPIO1_CHIP VERIFIED_SPEED_HZ [EVEN_CHUNK_BYTES]
```

此命令会操作 DC/RESET、执行两次硬复位、清屏并持续显示测试图。不会控制电源、背光或复位 Linux。Ctrl-C/TERM 正常结束并释放资源，保留当前画面，不发送额外关屏命令。

待记录：

1. 初始化波形：两次复位、11 条命令及 0x36，DC/CS 对应关系与实际时钟。
2. 彩条应为红、绿、蓝、白、黑；完整黄色边框，四角依次为左上 1 TL、右上 2 TR、左下 3 BL、右下 4 BR，无裁切/镜像。
3. 大区域分段期间 CS 连续、无丢像素；实际 spidev 缓冲与最大传输限制符合所选参数。
4. 正常退出、再次启动、设备缺失/权限错误、超时后的资源释放与完整重初始化。长时间稳定性归 P6，不能以本次离线检查替代。

完成这些实板项目后才能将项目计划中的 P3 标为“实屏验收通过”。P4 已完成页面显示与测试板导航验收；最新动画与逐页英俄实屏回归列入 P6，真实业务属于 P5。
