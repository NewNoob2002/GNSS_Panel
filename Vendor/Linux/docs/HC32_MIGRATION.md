# HC32 → RK3506J 应用与显示迁移摘要

提取日期：2026-09-18。依据当时工作区源码和用户确认的初始化表；该次提取仅分析与整理文档，没有修改 HC32 工程、复制应用、编译旧工程或操作硬件。本文的 HC32 符号名与八页注册表保留原工程名称，不按 Linux 命名清理反向改写。

后续进度：P2 已建立构建基线，P3 已点亮 RM690A0，P4 已迁入九页 View、状态栏、PageManager 和测试板 evdev。当前 `7b49532` 使用 `page` / `i18n` / `resource_pool` 命名空间及 `App::init()`、`on_input()`、`update_status()`；真实业务仍未接入。最新验收范围与验证边界见 [P4 记录](P4_APPLICATION.md) 及 [项目计划](PROJECT_PLAN.md)。

后续 P1 更新：用户已确认 SCL=GPIO0_C0、SDA=GPIO0_C1、CS=GPIO0_C3、RST=GPIO1_C4、DC=GPIO1_C6。当前接线与设备树冲突结论以 [硬件定型记录](../board/rk3506j/HARDWARE.md) 为准，下面保留旧源码提取依据。

当前范围进一步确认：测试板为 RK3506J 512MB+8GB，屏为 AMOLED，沿用现有 SGM3849 供电电路；供电、初始化及 SPI 写入像素数据后应自行显示，无背光控制。显示移植不增加 BL/PWM/亮度接口，原初始化表不变。量产按键与整机电源相关功能明确延后到量产 PCB 阶段；测试板 evdev 四键后来已在 P4 接入。本文对旧 MCU 职责的分析作为后续参考，不再作为 P1/P2 的前置要求。

## 1. 来源与结论

源工程根目录：

```text
/home/gtc/Desktop/workspace/HC32_PROJ/HC32F460xE_Arduino
```

读取时 HEAD 为 `28e89c1406e2c26ded7b402fa37aadf2c6d9155e`，`git status --short` 无输出。下文 HC32 文件路径相对此目录。RK 文件路径相对本项目 SDK 根目录。

关键结论：

- 应用基线是 `Simulator/App`。HC32 顶层 CMake 实际通过 `add_subdirectory(Simulator/App)` 将其编入固件，因此它不仅是桌面演示。
- 类名虽然为 `Adafruit_ST7789`，实际调用的是 `generic_RM690A0`。用户已确认此表为目标屏初始化。后续按 RM690A0 协议基线命名，不能按库目录名移植标准 ST7735/ST7789 初始化。
- 目标为 RGB565 彩色屏，应用逻辑尺寸 294×126；底层初始化尺寸 126×294，再由 LVGL 软件旋转 90°。
- 两份旧 LVGL 均为 8.3.11。优先冻结旧版本完成迁移验证；RK SDK 的 8.4.0 是后续可评估选项，不能混合不同版本头文件和库。
- 可保留页面、资源、页面管理、数据中心及语言切换；SPI/DMA、输入、tick、电源/复位、业务数据通路需适配 Linux。

## 2. 证据索引

| 结论 | 源文件 / 定位 |
| --- | --- |
| 固件复用 App，C11/C++17 | `CMakeLists.txt`；`Simulator/App/CMakeLists.txt` |
| MCU 程序启动与循环 | `USER/main.cpp:47` |
| 桌面 SDL 主循环、演示数据 | `Simulator/main.cpp` |
| 页面注册、语言切换、复位耦合 | `Simulator/App/App.cpp:56` |
| 实际屏引脚与尺寸 | `Platform/Config/mcu_config.h:86` |
| RM690A0 初始化表与实际调用 | `Libraries/Adafruit-ST7735-Library/Adafruit_ST7789.cpp:79,157` |
| 初始化表解析、第二次硬复位、地址窗口 | `Libraries/Adafruit-ST7735-Library/Adafruit_ST77xx.cpp:95,173` |
| 第一次硬复位、CS/DC、字节发送 | `Libraries/Adafruit-GFX-Library/Adafruit_SPITFT.cpp:536,650,1988,2208,2505` |
| 默认 SPI mode / 偏移初值 | `Libraries/Adafruit-ST7735-Library/Adafruit_ST7789.h` |
| 真正 SPI 寄存器配置、频率设置空实现 | `Platform/Core/src/SPI.cpp:41,121,257` |
| flush、DMA、对齐、缓冲与旋转 | `Libraries/lvgl/porting/lv_port_disp.cpp` |
| 编码器类型输入 | `Libraries/lvgl/porting/lv_port_indev.cpp`；`Platform/HAL/HAL_Key.cpp` |
| 版本与配置差异 | `Libraries/lvgl/lvgl.h`、`Libraries/lvgl/lv_conf.h`、`Simulator/lvgl/lvgl.h`、`Simulator/lv_conf.h` |
| 业务模型与 MCU 标记耦合 | `Platform/Config/mcu_define.h` |
| 原通信及报文更新逻辑 | `USER/inc/slave_i2c.h`、`USER/src/slave_i2c.cpp`、`USER/src/message_decode.cpp` |
| MCU 电源、时钟、看门狗职责 | `Platform/HAL/HAL.cpp`、`HAL_Power.cpp` |
| Linux 日志已有实现 | `Libraries/easylogger/port/elog_port.c` |
| RK spidev 默认消息缓冲 | RK `kernel/drivers/spi/spidev.c:86` |

CodeGraph 查询未能访问源工程索引，本轮结论来自当前源码直接读取及调用点核对，未重建索引。

## 3. 已确定的显示参数

| 参数 | 当前 HC32 实现 | 迁移处理 |
| --- | --- | --- |
| 驱动协议基线 | `generic_RM690A0`，用户确认 | 保留原表，不替换为库中通用 ST7789 表 |
| 应用逻辑尺寸 | 294×126 | 保持原 UI 布局 |
| 初始化尺寸 | `screen.init(126, 294)` | 保持控制器坐标系 |
| 控制器方向 | `setRotation(2)` → `0x36, 0x00` | 与 LVGL 旋转一起复现 |
| 坐标偏移 | 126×294 分支及成员初值使当前 x/y 起点为 0 | 显式配置，避免继承库的隐含初值 |
| LVGL 旋转 | `sw_rotate=1`、`LV_DISP_ROT_90` | 首版保留，避免重复旋转 |
| 像素格式 | RGB565、2 字节/像素 | 不采用单色页打包 |
| MCU 字节序配置 | `LV_COLOR_16_SWAP=1` | 保留线上字节顺序；只能在明确一层交换 |
| SPI | 初始按 mode 0、8 bit、MSB first 提取 | 2026-09-24 实屏对照发现参考 Linux 程序使用 mode 3；RK3506 改为 mode 3 后点亮 |
| 软件期望频率 | 库默认 32 MHz | 不是实测值，不当作屏最大频率 |
| 实际 HC32 配置 | `SPIClass::begin()` 使用 `SPI_BR_CLK_DIV2`；`setClock()` 未实际配置分频 | 实际速率须结合时钟树或测量确认 |
| 显示使能/背光 | 当前 `CONFIG_SCREEN_BLK_PIN` 被注释 | 不按历史 LCD 配置擅自增加背光控制 |
| 刷新区域 | 起点向偶数取整、终点扩展到奇数 | 首版保留已有 2 像素对齐，验证边界；源码未解释硬件原因 |

旧引脚：CS=PB14、D/C=PB1、RESET=PB2、SCK=PB15、MOSI=PB13，SPI_3；这些仅用于理解旧板，不是 RK 引脚编号。新板 D/C=GPIO1_C6、RESET=GPIO1_C4 已由用户确认；屏电气要求及屏端连接器完整线序仍需明确。

## 4. 初始化表完整展开

表格式是：首字节为命令数；每条为命令、参数数量（高位为延时标志）、参数字节、可选延时字节。

`ST_CMD_DELAY=0x80`；解析器把延时字节 `255` 转为 `500 ms`。该标记和延时字节属于宿主解析格式，不发送到屏幕。以下按原程序执行顺序列出，不为未经手册核实的厂商寄存器推测含义。

| 顺序 | 命令 | 数据 | 命令后等待 |
| --- | --- | --- | --- |
| 1 | 0x01 | 无 | 150 ms |
| 2 | 0xFE | 0x01 | 无 |
| 3 | 0x6A | 0x21 | 无 |
| 4 | 0xFE | 0x00 | 无 |
| 5 | 0xC4 | 0x80 | 无 |
| 6 | 0x35 | 0x00 | 无 |
| 7 | 0x51 | 0xFF | 无 |
| 8 | 0x3A | 0x05 | 无 |
| 9 | 0x20 | 无 | 无 |
| 10 | 0x11 | 无 | **500 ms** |
| 11 | 0x29 | 无 | **500 ms** |

原注释将 0x3A/0x05 标为 16 bit，0x20 标为关闭反色。不要套用同文件另一张 ST7789 表中的 0x3A/0x55。表内总等待为 1150 ms。

初始化不止这 11 条。完整已读调用链为：

1. `lv_port_disp_init()` 调用 `screen.init(126,294)`。
2. `commonInit(NULL) → begin() → initSPI()`：建立 SPI、CS/DC；RESET 高 100 ms、低 100 ms、高 200 ms。
3. `displayInit(generic_RM690A0)`：再次 RESET 高 50 ms、低 50 ms、高 150 ms；再执行上述表。
4. `setRotation(2)`：额外发送 0x36，数据 0x00。
5. `fillScreen(ST77XX_BLACK)`：清黑屏。
6. 注册 LVGL 显示及软件旋转。

源码显式等待累计约 1800 ms，另有 SPI 传输与执行耗时。首版以复现旧行为为准；没有屏规格或实测依据时，不直接删掉第二次硬复位、缩短延时。

## 5. 像素写入、缓冲与 SPI 边界

现有刷新链：

```text
LVGL 绘制并执行软件旋转
  → disp_flush(area, color_p)
  → setAddrWindow(x, y, w, h)
      0x2A + [x_start_hi, x_start_lo, x_end_hi, x_end_lo]
      0x2B + [y_start_hi, y_start_lo, y_end_hi, y_end_lo]
      0x2C
  → D/C 高，RGB565 字节经 HC32 DMA 发出
  → DMA 回调 → lv_disp_flush_ready()
```

窗口终点是 `x+w-1`、`y+h-1`；坐标按高字节在前发送。底层是单字节命令加 D/C 的 SPI 路径，不应仅因 RM690A0 名称而擅自套用别处的 QSPI 封装。

重要迁移约束：

- `sendCommand()` 自行控制 CS/DC；`setAddrWindow()` 内部的 `writeCommand()` 要求外部已选中屏幕。旧 `disp_flush()` 在窗口命令之后才显式拉低 CS，DMA 回调中也未显式拉高 CS。新实现必须定义完整事务边界，不沿用对先前 CS 状态的依赖。
- 旧 DMA 及 NVIC 代码属于 HC32，不能复制到 Linux 用户态；RK 首版使用同步 spidev，整块区域实际发送完成后才调用 `lv_disp_flush_ready()`。
- 窗口命令、数据与分段发送期间的 CS 行为要一起设计。先核实模块对片选中断的要求，再选择硬件 CS 或明确配置的 GPIO CS；避免 GPIO 和内核同时控制同一片选。
- RK SDK 的 `spidev.c` 默认 `bufsiz=4096`，`write` 和 message 的累计发送量均受限制。把大缓冲拆成同一个 ioctl 的多个 transfer 不一定绕过总量限制。按实际缓冲/控制器限制分段，保留连续写显存语义，检查返回长度和错误；必要时显式配置缓冲上限。
- 初始化表发出 0x35/0x00，但已读刷新代码没有等待 TE 信号。不能据此宣称旧系统已做 TE 同步，新板是否引出 TE 仍待确认。

按现有宏计算：

| 项目 | 大小 |
| --- | --- |
| 一帧像素 | 294×126 = 37044 像素 |
| RGB565 一帧 | 74088 字节，约 72.35 KiB |
| 每个 MCU 绘制缓冲 | 37044/4 = 9261 像素 = 18522 字节 |
| 两个 MCU 绘制缓冲合计 | 37044 字节，约 36.18 KiB |

原缓冲注释写“10 rows”，实际以宏计算为准。上述仅为绘制缓冲，不包括 LVGL 内存池、旋转临时空间与应用资源。旧 `spi_dma_trans()` 长度形参为 uint16_t，不能直接用于 74088 字节整帧。

纯载荷估算：若实际 SPI 为 32 MHz，全帧约 18.522 ms；若为 8 MHz，约 74.088 ms。这里假设连续满速传输，不含 GPIO/ioctl/旋转/绘制开销，也不是旧板实测帧率。新板先以保守且符合模块规格的频率点亮，再测量状态刷新与页面动画。

## 6. LVGL 两套配置的差别

| 参数 | MCU Libraries/lvgl | 桌面 Simulator |
| --- | --- | --- |
| LVGL | 8.3.11 | 8.3.11 |
| LV_COLOR_DEPTH | 16 | 16 |
| LV_COLOR_16_SWAP | 1 | 0 |
| LV_MEM_SIZE 配置值 | 128 KiB | 1 MiB |
| tick | SysTick 调用 lv_tick_inc(1) | LV_TICK_CUSTOM / SDL_GetTicks() |
| 显示尺寸与旋转 | 126×294 + LVGL ROT_90 | SDL 294×126 |
| 显示后端 | SPI3 + DMA | SDL |
| 输入 | HAL 转为 LV_INDEV_TYPE_ENCODER | SDL 鼠标、键盘、滚轮 |

迁移建议：首版固定旧 8.3.11 源码与必要配置，C11/C++17，单个 UI 主循环；Linux 单调时钟提供 tick。沿用原软件旋转和两块绘制缓冲，先做同步 SPI。未来改硬件旋转或缓冲大小，应独立验证。旧 SDK 配方 8.4.0 与此基线不同，应用可先私有链接旧版本，Buildroot 集成时显式决定版本归属，禁止无意链接 SDK 的另一份 LVGL。

不要直接复制 Simulator/lv_conf.h 当成板端配置：它包含 SDL tick 和不同字节序。显示驱动、LVGL、字体/图片资源必须使用一致的配置编译。彩条检查红/绿/蓝、黑/白；四角编号与非对称图案检查方向、窗口终点和偏移。

## 7. 应用复用范围

`App_Init()` 初始化 DataProc、主题、ResourcePool、状态栏与 PageManager，安装下列 8 页：

| 页面注册名 | 迁移关注点 |
| --- | --- |
| Startup | Logo、动画、输入使能 |
| HardwareCheck | 设备在线判断及原 eg25_board / 30 秒等待逻辑 |
| Dialplate | 主界面、定位与业务状态显示 |
| RecordConfig | 记录参数、操作与确认状态 |
| WorkSettings | 电台等工作参数、修改标志与执行反馈 |
| SystemInfos | 软件/硬件版本及系统信息来源 |
| Shutdown | 关机请求、输入禁用与页面切换 |
| SaveConfig | 原 MCU shared_info、NVIC 复位、电源关闭；不能视作已完成 Linux 持久化 |

状态栏独立创建在 `lv_layer_top()`。I18n 已支持 English/Russian 动态切换；字体、图片、符号和语言包应随应用完整迁移。保留原框架版权及资源来源信息，不重写现有 PageManager/旧消息框架/ResourceManager。

`DataProc/DP_LIST.inc` 当前仅注册 StatusBar 节点；业务状态广泛通过全局 `SystemInfo_t systemInfo` 访问，不能把所有数据流都概括为 旧消息框架消息。

`Simulator/main.cpp` 写入卫星数、Wi-Fi 名称、电量等演示值；`Simulator/HAL` 中多项电源函数为空或模拟实现。它适合 UI 对照，不是目标业务后端。EasyLogger 已有 __linux__ 分支，可评估复用 stdout、pthread 与时间实现。

## 8. 输入与业务接口的迁移

输入虽然注册为 encoder，旧硬件实际上由按键产生：功能键单击产生 -1，双击产生 +1；电源键电平映射 pressed；长按重复计数到 10 设置强制关机标志。功能键采样为低有效，但 `Encoder_GetIsPush()` 返回电源键 HIGH，必须结合原电路确认，不能直接把所有按键统一成低有效。

原数据通路：

```text
旧业务主机 ↔ I²C 从机 0x11 ↔ 报文解析 ↔ systemInfo ↔ 页面/状态栏
                                            ↑
                       本地按键、电池、充电、电源状态
```

`message_decode.cpp` 更新工作模式、卫星/坐标、NTRIP、电台、Wi-Fi、记录信息等；页面设置通过 change_flag/op 等字段与对端交互。去掉 HC32 后需确定业务是否同进程、在 RK 其他进程，或仍在外部设备：分别接入已有函数、既有 IPC 或明确的外部传输。当前不预选新消息协议，不假设 UI 自己实现 GNSS/NTRIP/记录服务。

需要明确替换的旧耦合：

- `mcu_config.h` 把屏引脚、版本和业务结构混在一起；提取所需业务定义，Linux 构建不包含 HC32 外设头。
- `App_Update()` 的外部电源变化分支调用 `NVIC_SystemReset()`。
- `SaveConfig` 写 `shared_info` / boot 命令并复位，不能直接翻译成主机 reboot。
- `LVGL_SIMULATOR` 用于屏蔽真实硬件行为；RK 正式构建不能靠一直定义它来伪装成功。
- 原 HC32 同时管理电源保持、充电器/电量计、按键、LED、看门狗。整板移除后，需明确这些职责由新硬件/系统中谁承担；不等于本轮自动移植全部外设驱动。
- 若后台接入线程，更新通过 UI 主循环交接，明确状态快照/事件归属，避免后台直接修改 LVGL 对象或无同步改 systemInfo。

## 9. 完整迁移参考步骤与阶段验收

以下保留完整迁移目标，不是当前待办清单。屏驱动和应用显示已进入 P3/P4 验收；测试板实体输入已通过 evdev 接入，无按键回归仍可静态选页。量产输入、整机电源/充电/复位适配在量产 PCB 阶段实施，真实业务与 SDK 集成属于 P5。

1. 冻结源版本及资源，补齐新板电气连接、业务提供方、输入/电源职责；已有屏型号和分辨率不再作为未知项反复询问。
2. 保留 Simulator 作原 UI 对照；建立目标 CMake，复用 App、资源与 LVGL 8.3.11，先移除编译对 MCU 头/SDL 的硬依赖。
3. 实现 Linux SPI/GPIO：原初始化表、双重硬复位、0x36/0x00、窗口写入、分段及错误处理。先验证彩条、四角、边框，再接 LVGL。
4. 复现 126×294 控制器坐标和 294×126 UI、RGB565、一次字节交换、软件旋转、输入与 tick；验证原 8 页、状态栏、动画和英俄切换。
5. 接入真实业务数据及设置回执；验证不再因旧 eg25_board 状态卡在检查页；明确重启/关机/保存配置语义。
6. 接入 Buildroot、自启动及目标权限，完成连续运行、重启、设备缺失、短写/传输失败、业务断连和关机流程验收。

当前已确认：五根信号接线、512MB+8GB 测试板、屏现有供电及无亮度控制方式；量产输入/整机电源延期，测试板 evdev 已接入。P2 已按 J/eMMC 非 RGB 基线构建；P3 用户确认 IO 电平兼容及共地，32 MHz 请求档已目视验证，但量化电气参数、物理波形、模块最大速率和完整资料仍缺失。存储介质/容量与部署配置仍需按目标板核实。业务数据提供方在 P5 接入时明确，量产按键/电源在 PCB 输出后另行定型。以上未测项目不记录为已通过。
