# 固件可靠性修复与验证

本轮保留现有裸机主循环、MillisTaskManager、LVGL 8.3.11 和页面框架，集中修复通信、内存、电源与生命周期问题。没有修改相邻 Bootloader 工作区及其已有未提交改动。

## 与当前 Boot 的同步

参考相邻 `HC32F460xE_Bootloader/docs/i2c_slave_contract.md` 及 `Drivers/BSP/Src/bsp_i2c_slave.c`：

- I²C1 地址 `0x11`、400 kHz、PA3/PA2 保持不变，EEI 优先级与 Boot 对齐为 9。
- 在释放地址匹配之前装入首字节；清除地址阶段残留 STOP/NACK。
- 地址匹配与 STOP 都清除 START 锁存，防止正常事务结束后硬件 BUSY 残留。
- NACK、仲裁失败、STOP 独立处理；以实际已传输字节与最终 STOP 判定响应完成。
- 短读回滚响应游标；未读响应不会被新请求覆盖，恢复也保留已发布响应。
- 仅活动事务或硬件 BUSY 两秒无进展才恢复；空闲总线和未读响应不触发周期性复位。
- 恢复时注销旧 IRQ、清 RX 和解析状态、重新配置外设并保留原中断屏蔽状态；失败后按两秒间隔重试。
- 半帧保留跨 STOP 的解析状态，250 ms 无新字节后丢弃。
- Linux 事务契约仍为写 STOP、处理延时、独立读；不支持即时 repeated-start 响应。

App Flash 范围已改为 `[0x8000, 0x7A000)`，与 Boot 的 32 KiB 分区、元数据 A/B 和尾部参数区相容。
当前 Boot 依据软件复位标志进入升级窗口；App 删除了无效、地址不固定且会被 BSS 清零的旧 `shared_info` 命令。

## 其他修复

- 自定义解析器支持零负载、检查固定帧头长度及最大帧；Debug 和 Release 均拒绝坏 CRC。
- 解析器释放与 `new[]` 配对；日志使用固定大小缓冲和固定 `%s` 格式，避免二次格式化输入。
- 响应发布前预留所有权；编码时主控开始空读的情况延后发布，避免业务修改后丢失响应。
- 电台、记录、Wi-Fi 通知在完整读出后确认；发送期间状态有变化时保留新通知。
- 校验位置数值、布尔值及帧长度；限制版本字段复制和界面中的定长字符串显示；IP 使用无符号字节。
- `0x04=1` 在响应完整读出后进入非阻塞的软件复位延时，继续服务通信和看门狗。
- 关机确认：主控完整读出含关机标志的 `0x02`（INFO2），或发送 `0x05=1` 且 ACK 完整读出，均进入八秒断电延时；首次关机请求起二十秒兜底，重复确认不延长，强制关机可立即执行。（`0x02` 确认为 2026-10-19 按旧版 X1Pro/XBase 语义恢复。）
- 电源控制独立于 LVGL 动画回调；开机长按由 HAL 时间判定，UI 显示和页面切换跟随状态。
- 低电量计时从 LED 更新迁入电源策略，按实际连续时间判断；采样无效、充电或条件恢复时重置；包含 0% 电量。
- BQ/MP 寄存器读取显式检查完整字节数，失败不发布部分结果；采样状态与最后成功时间分开记录；避免零容量除法。
- I²C2 的错误出口统一 STOP、ACK 恢复及外设关闭；`I2cMaster` 的 `write`/`writeRead`/`probe` 以 `bool` 返回事务是否完整成功。
- 充电控制读失败时不继续读改写寄存器；GPIO 充电方案补充过温后的恢复路径，样本失效时禁止充电。
- `_sbrk` 改用真正的堆起点与上限，检查扩展和回收越界；C++ 普通 `new` 失败终止，nothrow 路径显式返回空指针。
- 堆栈段声明为可分配 NOBITS、链接为 NOLOAD 并 KEEP，构建报告计入全部 8 KiB 堆与 8 KiB 栈，断言防止预留被回收。
- `SystemInit` 在 C++ 构造之前初始化 FPU/向量表；`micros()` 不再恒为零，微秒延时使用无符号回绕计算。
- 显示驱动：LVGL 单绘制缓冲 + 旋转后的 DMA 发送缓冲（各 42 行，整屏 3 块），在 `disp_flush()` 内自行旋转以并行渲染与发送，取代 LVGL `sw_rotate` 的串行流程；`LV_MEM_SIZE` 由 128 KiB 降至 96 KiB。DMA 完成后等待 SPI 移位结束，100 ms 超时可释放等待并计数。实测 30 帧全屏重绘由 1626 ms 降至 578 ms。
- 修复 Startup 的 DELETE 事件读取已删控件造成退出挂起；模拟器显式先清页面再析构 DataCenter，避免静态销毁顺序访问失效账户。
- 温度警告只在显示值变化时刷新文本；移除数据模型对 UI 的未使用类型依赖。
- 编译数据库显式提供 ARM GCC 系统头目录，补充仅编辑器使用的第三方头包含上下文。

## 软件检查

```sh
cmake --preset gcc-debug
cmake --build --preset gcc-debug
cmake --preset gcc-release
cmake --build --preset gcc-release
cmake --preset simulator-x1pro
cmake --build --preset simulator-x1pro -j4
cmake -S tests -B build/host-tests -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DSIMULATOR_EXECUTABLE="$PWD/build/simulator-x1pro/bin/main"
cmake --build build/host-tests -j4
ctest --test-dir build/host-tests --output-on-failure
```

本机 Debug、Release、模拟器构建通过。完整 Release 重编译保留 3 条 LVGL `lv_tlsf.c` 的未使用参数告警，未修改第三方代码消除它们。

主机测试使用 AddressSanitizer/UndefinedBehaviorSanitizer：

| 测试 | 覆盖 |
| --- | --- |
| firmware_tests | 零负载、坏 CRC、最大帧、错误后恢复、短读、NACK+STOP、空闲不恢复、BUSY/事务停滞恢复、恢复失败重试、响应保留、复位/关机确认、设备读取失败、禁充恢复、电源超时与 tick 回绕 |
| clock_tests | SysTick 分段读取、待处理 Tick、原中断屏蔽值、时间回绕、非 1 ms Tick |
| heap_tests | 堆边界、分配失败不移动边界、负增长、INT_MIN/INT_MAX |
| simulator_smoke | SDL dummy 模式启动、SIGTERM、页面栈清理、正常退出 |

四组测试通过。主机测试保证断言启用，不用于替代硬件中断、电气和传输时序验收。

## 资源

| 配置 | Flash 链接报告 | 主 RAM 布局，包含堆和栈 | 预留区域之外主 RAM |
| --- | ---: | ---: | ---: |
| Debug | 389084 B / 456 KiB | 173792 B / 188 KiB | 18720 B |
| Release | 331360 B / 456 KiB | 171224 B / 188 KiB | 21288 B |

128 KiB 的 LVGL 池还供 C++ 对象使用，其内部峰值和碎片不能从链接报告推导；栈的运行峰值也未做全场景测量。

## 板上验证记录

完整原始 Flash 已保存为忽略目录 `build/validation/flash-before.bin`，SHA-256：
`82381a1c71cea5ec4df845cab1f33e43705073a60e8a3146d029574281c55c86`。

最终使用 J-Link CE `63728769`、HC32F460xE、SWD 4 MHz、正常复位后仅下载 App BIN 到 `0x8000`。不整片擦除、不写 Boot、ICG、元数据和参数。

初次 pyOCD 内置算法及中途更换的 SAM-ICE 编程均失败，日志不计作成功；已检查部分写入状态，并用最终 J-Link 完成修复。Debug 最终 Commander 下载及全 BIN verify 通过，随后独立会话再次逐字节比较 App、Boot 与尾部保留区。

调试启动采用软件复位后设置 App 向量/MSP/PC，保留软件复位原因；先关闭 CPACR 验证 App 能自行初始化 FPU。它验证 App 启动与运行，不等于重新完成上电按键、Boot 升级交接和断电保持验收。当前 NRST 引脚复位没有观察到实际重启，因此不把该路径计为通过。

Debug 连续不暂停 CPU 的观察窗口中，完整 I²C 响应从 34 增至 43，恢复计数保持 1；CFSR/HFSR 均为 0，DMA 超时为 0，BQ、MP 和 Linux 主控在线，电池采样有效。此前调试暂停造成的恢复不计为空闲误恢复。
调试器注入事务无进展状态后，恢复计数 1→2，完整读计数 46→47，重新初始化成功且无核心故障。此为软件状态故障注入，未模拟物理 SCL/SDA 拉低。

Release 已通过 Commander 下载、全 BIN verify 和独立会话逐字节回读，Boot/尾部保留区不变。连续不暂停 CPU 的观察窗口中，完整响应 79→88，恢复计数保持 1，CFSR/HFSR 与 DMA 超时均为 0；BQ、MP 和主控均在线，电池采样有效。最终保持 Release 运行。

| 镜像 | BIN SHA-256 |
| --- | --- |
| Debug | `f707c8c182063b5a891c9baf34a0d08caa528907015d08a0e64e841bd7b9c58a` |
| Release，当前部署 | `9b609ee0b5871c0ae29e09ca80a9099cdbe4789db0ae66e8cb4abb9e7dd6441d` |

详细运行快照为 `build/validation/debug-final.json` 和 `release-final.json`，原始下载与运行日志保存在同目录。初次启动采样因 Linux 尚未开始轮询而失败的日志也保留；后续等待主控启动后重新采样，不把初次失败写成通过。

## 尚未覆盖

- 真实电池过温、低电量持续三分钟、`0x05` 完成确认后的电气断电时序及充电参数校准。
- 物理总线卡死、拔插、长时间压力及所有 UI 页面/语言的人工视觉验收。
- 已规划的整体目录搬迁、LVGL 两份源码合并、RTOS 引入和完整 SPI Arduino API 重构不在本轮可靠性修复中。
