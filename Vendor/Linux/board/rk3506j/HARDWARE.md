# P1 硬件与接口定型记录

P1 记录日期：2026-09-18。目标：RK3506J 直接驱动原 HC32 工程使用的 RM690A0 屏。下文 P1 操作范围和未执行声明保留为阶段记录，不代表后续没有上板。

**后续状态同步：** P2 构建基线已完成；P3 已用 mode 3 点亮测试图，32 MHz 请求档 benchmark 已运行，测试板 `spidev.bufsiz=65535`，应用保留 4096 字节分段，无需按 P1 早期设想修改为 32768。P4 已接入并验证测试板 `adc-keys` 的 evdev 四键（该板 `/dev/input/event0`），不使用 GPIO1_B4 改接方案；量产按键与整机电源仍延期。IO 电平兼容及共地由用户在 P3 确认，物理波形、最大可用速率和 SGM3849 输出轨仍未实测。详见 [P3](../../docs/P3_DISPLAY.md) 与 [P4](../../docs/P4_APPLICATION.md)；这些既有结果不表示当前提交已重新上板。

**P1 阶段结论：测试板显示接口已定型，可进入 P2 离线构建准备；P1 本身不代表板端或电气验收通过。** 用户确认测试板为 RK3506J、512MB 内存 + 8GB 存储，屏已按下表接到开发板引出 IO；屏是 AMOLED，由 SGM3849 电路供电，无背光控制；供电、初始化及 SPI 写入像素数据后应自行显示。按键与整机电源相关功能明确延后到量产 PCB 阶段，不再作为本轮显示迁移的阻塞项。

8GB 存储在 P2 暂按 eMMC 方案准备；用户确认的是容量，具体介质和板上运行配置仍需通过板端信息核实。本文未把用户描述的工作方式记录成代理实测结果。

## 1. 已确认的接线

用户确认：P0_C0--SCL、P0_C1--SDA、P0_C3--CS、P1_C4--RST、P1_C6--DC。

JP1 编号依据 docs/ATK-DLRK3506JS V1.0.pdf 第 4 页，适用于该原理图对应的 ATK 底板；若实板为自研板，应保留 SoC 信号对应，重新核对连接器编号。

| 屏标签 | 实际用途 | RK 引脚 | ATK JP1 | GPIO bank 内 offset | 配置归属 |
| --- | --- | --- | --- | --- | --- |
| SCL | SPI SCLK | GPIO0_C0 | 17 | 16 | SPI0 pinctrl，复用功能 2 |
| SDA | SPI MOSI / DIN | GPIO0_C1 | 13 | 17 | SPI0 pinctrl，复用功能 2 |
| CS | SPI CS0，低选中 | GPIO0_C3 | 14 | 19 | 首版沿用 SPI0 硬件 CS0 |
| RST | 屏复位，低有效 | GPIO1_C4 | 12 | 20 | GPIO 输出；后续由显示应用持有 |
| DC | 低为命令，高为数据 | GPIO1_C6 | 10 | 22 | GPIO 输出；后续由显示应用持有 |

SCL/SDA 是屏端标签，此处使用 SPI，不能按 I²C 总线配置。当前接线未包含 MISO；旧程序也未配置屏幕读回。TE 是否引出未确认，首版不把它作为必需信号。GPIO1_A0/A1 等早期候选不再用于本项目。

GPIO offset 是各 bank 内编号：C4=16+4=20，C6=16+6=22。不要将历史全局 GPIO 编号 52/54 当作 gpiochip 内 offset。GPIO1 在本 SDK DTS 中对应 /pinctrl/gpio@ff870000；实际 /dev/gpiochipN 编号和标签需板端核对，不仅凭 N=1 推断。

SPI0 的 DTS 地址为 /spi@ff120000，reg=0 子节点对应首选 spidev0.0；以板上枚举和实际加载 DTB 为准。CS1、MISO 均不接屏。是否移除厂商额外 spidev@1 / MISO pinctrl 留给项目板配置阶段，当前不因未接线而改动其他外设。

## 2. 原理图与设备树证据

以下路径相对 sdk/atk_dlrk3506_linux6.1_release_v1.3.1_20260326/kernel/arch/arm/boot/dts/：

| 文件 | 作用 |
| --- | --- |
| rk3506j-alientek.dtsi | J 板继承 B 板外设；启用 ADC 按键；覆盖 CAN、UART、存储相关配置 |
| rk3506b-alientek.dtsi | SPI0、两个 spidev 节点；删除音频功放控制 GPIO；USB regulator 的控制 GPIO 已注释 |
| rk3506.dtsi → rk3502.dtsi | SPI0 基地址、aliases、DMA 及 GPIO bank |
| rk3506-pinctrl.dtsi | SPI0_C0/C1/C3 的功能 2；RGB888 对 GPIO1_C4/C6 的功能 1 |
| rk3502-evb1-v10.dtsi | 祖先配置中存在 GPIO1_C6 的音频功放控制，须结合后续删除属性判断 |
| rk3506j-alientek-rgb1024x600.dtsi | 启用 rgb888_pins，会占用两根屏控制线 |

不能仅通过搜索发现某个引脚名就判定冲突：引脚组定义可能没有被引用，祖先的 GPIO 属性也可能已删除。本轮通过 CPP 合并 include、DTC 生成 DTB，再读取合并后的属性及启用节点引用进行了核查。

## 3. pinmux 冲突结论

| 核查配置 | SPI0 | GPIO1_C4/C6 | 结论 |
| --- | --- | --- | --- |
| rk3506j-alientek-mipi720x1280-emmc | 启用，C0/C1/C3 为 SPI 功能 2 | 未发现启用的 pinctrl / GPIO 属性占用 | 当前接线的可选软件基线 |
| rk3506j-alientek-mipi720x1280-nand-ubi-ubifs | 同上 | 同上 | NAND 候选，尚未选择 |
| rk3506j-alientek-mipi800x1280-emmc | 同上 | 同上 | 自动 MIPI eMMC 配置的另一候选 DTB |
| rk3506j-alientek-rgb1024x600-emmc | 启用 | rgb888_pins 对 C4/C6 选择功能 1 | **与当前接线冲突，不能直接使用** |

具体结果：

- GPIO1_C4：vcc5v0-otg0-regulator 最终没有 gpio 和 pinctrl-0 属性，板级控制语句被注释；存在 vcc5v0_otg0_en 引脚组定义不等于它已生效。无需为 RST 关闭整个 USB 控制器。
- GPIO1_C6：es8388-sound 最终没有 spk-con-gpio 和 pinctrl-0，已被板级 /delete-property/ 去除。无需为 D/C 关闭整个音频设备。
- RGB：合并树的 /syscon@ff288000/rgb 在三个 MIPI 候选中为 disabled；RGB 候选中为 okay，确实引用了 C4/C6。
- SPI0：对应引脚组同时包含未接屏的 MISO；这不改变 MOSI/SCLK/CS0 的映射。

这些结论仅覆盖列出的未修改 SDK 基线，不覆盖 AMP、Thunderboot、自研 DTB、U-Boot 提前复用、运行时 GPIO consumer 或实际电平。板上不得在 RGB 仍驱动这些脚时再由 GPIO 应用操作 RST/DC。

保持用户现有接线的条件：本项目不采用占用这两脚的 RGB888 配置；未来增加 RGB 屏时必须重新分配控制线，不能仅靠应用层解决。MIPI 不存在上述同一引脚冲突，本轮可保留厂商非 RGB 基线用于构建，不把 MIPI 实屏接入纳入本轮验收。

## 4. 显示软件接口及 P3 实际结果

| 项目 | 决定 | 当前是否已执行 |
| --- | --- | --- |
| 驱动路径 | 应用 RM690A0 协议 → spidev/GPIO → 内核 SPI0 | P3 已实现并运行 |
| SPI 参数 | Mode 3、8 bit、MSB first，实屏测试 500 kHz | mode 3 已点亮；最高速率和物理波形未测 |
| CS | 硬件 CS0；跨 ioctl 保持完整窗口/像素事务，要求 SPI0 总线独占 | P3 测试图可显示；CS 连续性波形尚未测量 |
| RST / DC | GPIO1 offset 20 / 22，物理电平按旧驱动；应用显式设置初值并持有 line | P3 已申请，退出后释放 |
| 初始化 | 用户确认的 11 条 RM690A0 表；255 表示 500 ms；两次硬复位及 0x36/0x00 | P3 已下发并点亮 |
| 方向/颜色 | 控制器 126×294，LVGL 软件旋转 90°，294×126 UI，RGB565 | P3 软件模型与实屏方向/颜色已核对；部分边缘遮挡 |
| 绘制缓冲 | 旧双缓冲，每块 18522 字节 | 已实现 |
| spidev 消息缓冲 | 应用默认 4096 字节分段，允许按条件指定偶数 4–32768 字节 | P3 板端读到 bufsiz=65535；本项目未更改 bootargs，不再要求改为 32768 |
| SPI 速率 | 保留应用配置入口，当前回归沿用 32 MHz 请求档 | P3 目视验证；实际 SCLK 未测，47 MHz 请求档黑屏不可用 |
| 屏供电 | 沿用用户已接好的独立供电电路，应用负责原初始化与像素发送 | 用户确认供电方式，代理未实测 |
| 背光/亮度 | 本轮不提供 PWM、BL/EN GPIO 或亮度调节接口；原表 0x51/0xFF 保留 | 不新增控制驱动或功能 |

实际内核配置若仍为 CONFIG_SPI_SPIDEV=y，则 bufsiz 通过内核启动参数设置，追加到现有 bootargs，不覆盖 root/console/存储参数。最终启动参数可能由 U-Boot 组合，运行时应检查 /proc/cmdline 和 /sys/module/spidev/parameters/bufsiz。

不使用 GPIO hog 长期占用 RST/DC，否则用户态请求可能失败。显示应用负责获取 line 并保持请求有效；退出和异常时如何关屏/释放依据模块及系统关机要求实现。GPIO 使用物理电平还是 active-low 逻辑值必须一致，避免 RST 的低有效被翻转两次。

现有 SDK 提供 libgpiod 1.6.4 配方；诊断工具是否可用须在选定 Buildroot 配置中核实，不能无条件使用 libgpiod 2.x 的 API/命令语法。P3 应用实际使用 GPIO CDEV v1，没有新增 libgpiod 依赖；P1 阶段也未安装额外依赖或编写 GPIO 驱动。

## 5. 屏供电与上板验证边界

用户已确认现有屏供电方式和五根信号接线。本轮复用这套测试连接，不重新设计供电，不把亮度调节列入功能。下面未量化的项目归入 P3 实机操作前的核查记录，不作为 P2 离线构建的前置输入。

| 项目 | 已知 / 待确认 |
| --- | --- |
| 屏类型及连接器 | RM690A0、294×126 应用显示及实际五根信号已确认；完整模块型号和屏端连接器资料尚未归档 |
| 供电 | 用户确认由现有 SGM3849 电路给 AMOLED 供电；不假设由 ATK JP3 直接供电。2026-09-24 黑屏时各输出轨尚未实测 |
| 亮度 | AMOLED 无背光控制；不因初始化表含 0x51 就宣称已有可用亮度控制功能 |
| IO 电平 | P3 用户确认 IO 电平兼容；实际 bank 供电、屏逻辑电压及电平转换的量化记录仍缺失，不能由 3.3V 插针推断 |
| GND | P3 用户确认测试板与屏共地；此项不是代理电气测量 |
| 外部电路 | 原 HC32 移除后供电、复位偏置、ESD/电平转换是否保留未说明 |
| 线长/速率 | 实际线长、模块最大 SPI 频率及信号质量尚无依据 |
| TE | 初始化有 0x35/0x00，但旧 port 不等待 TE；新接线未包含该信号 |

没有模块规格时不指定电源、允许电流或保证某个 SPI 频率。用户确认的供电工作方式与接线足以冻结当前显示接口，不能替代 IO 电平/共地、运行 pinmux 和实际频率验证。本文没有执行上电、复位、GPIO 输出和测量动作。

## 6. 本轮范围与量产阶段职责

| 原 HC32 职责 | 现有依据 | 本轮安排 |
| --- | --- | --- |
| 界面与屏驱动 | 已确认原 App、屏协议和本次信号接线 | RK 直接承担 |
| 功能键、电源键 | 原 encoder 映射及电平检测 | 用户明确延后，量产 PCB 出来后适配，不新增按键驱动 |
| UI 联调输入 | J 型 DTS 已有 ADC 按键 volume/menu/esc | P4 已通过 evdev 接入并板端验证；无按键回归仍可静态选页 |
| 电源保持/断电 | 原 POWER_CONTROL_PIN 直接控制 | 用户明确延后，不移植 GPIO 断电或 MCU 复位动作 |
| 电量/充电 | 原 bq40z50、mp2762 或 GPIO 充电方案 | 随整机电源功能延后，不作为当前屏点亮的依赖 |
| 看门狗/LED | 原 MCU 定时喂狗及状态指示 | 当前只保留现有测试板系统行为，不新增量产板外设移植 |
| 业务状态 | 原 I²C 从机 0x11 与对端交换 systemInfo | P5 再明确数据提供方；不阻塞当前硬件接口与显示构建 |
| 复位/升级标志 | NVIC_SystemReset、shared_info、MCU boot | 当前测试版本不执行此类动作，后续按量产系统语义适配 |

阶段性验收应分别记录“显示/页面通过”“真实业务未接入”“按键/整机电源延后”。测试数据可用于画面回归，但不代表真实业务或已延后功能通过。保留原页面资源与业务定义；当前运行路径不得因缺少 MCU 电源状态、按键或对端在线状态而无法进入显示测试。

## 7. 板型、配置与恢复条件

用户确认测试板为 RK3506J、512MB 内存 + 8GB 存储，屏直接接开发板引出 IO。P2 以 22_atk_dlrk3506j_automipi_emmc_defconfig 作为工作基线，包含已经核查的 720/800 MIPI 两个 DTB；依据是该 SDK 手册将 22 列为 J/eMMC 出厂配置。8GB 暂按 eMMC 判断，具体介质仍待板端枚举确认，不能以容量替代识别。

P1 当次只记录基线决定，未执行 lunch、修改 SDK 或重建固件；后续 P2 构建结果见 [构建基线](../../docs/P2_BASELINE.md)。NAND 与 RGB 配置不作为当前 P2 的默认选择；此前 NAND DTB 核查作为对照保留。

容量相关注意：该 defconfig 有 RK_FLASH_SIZE=2048，但 gen-extra-parts-config.sh 将此选项放在 RK_UBI 条件下；mk-extra-parts.sh 在无法取得特定 UBIFS 分区大小时用它兜底。不能把该旧配置值解释成实板存储只有 2GB，也不因用户提供 8GB 就直接改成 8192。eMMC 使用 parameter-mmc.txt（GPT，userdata:grow）；P2/P3 核对实际存储容量、分区布局、可用 RAM 和保留内存，部署前再确认产物匹配。

保留厂商非 RGB 基线，不为独立 SPI OLED 强行启用原 MIPI 显示应用，也不因本轮无按键而修改系统所有输入节点。

进入部署前应明确实际设备、原厂恢复镜像、串口和厂商恢复入口。P1 当时尚无板端连接方式，未刷写或重启；后续 P3/P4 已通过受控连接部署应用并运行，不等于验收了整机刷写与恢复通路。

## 8. P1 已执行的验证（历史记录）

- 重新目视核对原理图第 4 页：GPIO1_C4=JP1-12，GPIO1_C6=JP1-10，SPI 三线映射与用户描述一致。
- 通过已建 rk3506-sdk:ubuntu20.04-v1 容器，只读挂载项目、禁用网络，将上述四个厂商 DTS 预处理并编译为 DTB；读取合并后的 GPIO、pinctrl 和 status。
- 四个 DTB 均成功生成。原厂基线存在 DTC 警告：三个 MIPI 配置各 23 条输出行，RGB 配置 25 条，包含时钟节点 simple_bus_reg 等。本轮不是无警告的全量 SDK 构建，也未做 DT schema 校验。
- 文件和核查中间结果仅写入 /tmp/rk3506-p1，未修改 SDK。运行 DTB、引脚当前 mux/consumer、实际供电和 OLED 显示均未验证。

## 9. P1 留下的只读板端核查清单

取得目标与连接方式后，先读取实际状态，勿直接使用 gpioset、SPI 测试发送或 GPIO export 切换引脚。

```sh
tr '\0' '\n' < /proc/device-tree/model
cat /proc/cmdline
cat /proc/mtd
ls /dev/spidev* /dev/gpiochip*
cat /sys/module/spidev/parameters/bufsiz
gpiodetect
gpioinfo
```

命令为检查清单，本轮未在板端执行；部分工具或 /proc/mtd 可能不存在，不据单个节点缺失认定存储类型。需结合块设备、实际板卡型号和当前固件判断。若 debugfs 已挂载且可读，再核查对应 pinctrl 的 pinmux-pins；只看 gpioinfo 未被 consumer 占用，不足以证明没有复用外设驱动。

## 10. P1 历史结论与剩余边界

- [x] 显示控制器、尺寸、颜色及旧初始化有源码和用户依据。
- [x] 五根显示信号按用户实接线冻结，并映射到 ATK JP1。
- [x] 候选设备树 pinmux 冲突检查完成，RGB 冲突已明确。
- [x] 沿用 spidev；P3 已以 4096 字节应用分段运行，P1 的 32 KiB 缓冲设想已由实测结果替代。
- [x] 用户确认 RK3506J 512MB+8GB 测试板；以 J/eMMC 非 RGB 配置作为 P2 工作基线。
- [x] 屏沿用现有供电电路，SPI 初始化/传输后点亮，不做背光或亮度控制。
- [x] 用户明确将按键和整机电源功能延后到量产 PCB，不阻塞当前显示移植。
- [x] 电气与运行配置的未实测项、真实业务接入和部署检查已划分到后续阶段。

**P1 测试板显示接口定型已完成；后续 P2 构建与 P3/P4 部分板端验收已执行，量产硬件和完整物理验收仍未完成。** 后续仍要核实存储介质/容量、运行 DTB、GPIO mux/consumer、IO 电平与共地、可用 SPI 频率；部署前确认目标及恢复通路。P1 本身未完成这些实测；后续已确认的测试板运行项见本文开头及 P3/P4。量产按键/整机电源仍未实现，不能与已接入的测试板 evdev 混为一谈。
