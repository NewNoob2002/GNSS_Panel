# 待机页充电功率

Startup 的电池标题在充电时显示 `CHG`，右侧读数在电量样本有效、电量计电流有效、电流不少于 50 mA，且电压处于 (0, 65.535] V 时显示一位小数功率（例如 `12.5W`）。未达到条件时仍显示两位小数电压；电池样本无效时显示 `BAT` / `N/A`。英文与俄文使用相同的数值、单位和布局。本功能纳入 [3.0.0 发布说明](release_notes.md)。

## 数据来源与单位

功率表示流入电池的功率。电压与电流都来自本机 BQ40Z50；MP2762A 仅继续提供已有的充电状态，不参与功率计算。

- [TI bq40z50 Technical Reference，SLUUA43A](https://www.ti.com/lit/pdf/sluua43)，第 12.10、12.11、12.12 节（第 111 页）：`Voltage()` 为 0x09、mV；`Current()` 为 0x0A；`AverageCurrent()` 为 0x0B，类型 I2，单位 mA。已核对该公开原文。
- [Smart Battery Data Specification 1.1](https://sbs-forum.org/specs/sbdat110.pdf)，第 5.1.11、5.1.12 节（第 24 页）：电流为有符号值，充电为正，放电为负；AverageCurrent 是滚动平均。选择 AverageCurrent 利用电量计自身平滑，接入充电后可能需要等待平均值越过阈值。

驱动以小端补码读出 `int16_t`，失败保持调用者输出不变。`Battery_Sample()` 为该读数维护独立的 `current_valid`：电流读取失败不使温度/SOC/电压样本失效，也不改变温度保护策略；核心样本失败或超过 10 秒没有有效样本时电流失效。数据仍由现有电源域写入，不增加新的状态写入口。

## 显示链路

`BQ40Z50 → HAL::Battery_Sample → systemInfo.power.batteryInfo → BusinessDevice::battery → hal::BatteryState → dp::Battery → StartupModel → StartupView`。

X1Pro/XBase 在面板 MCU 上直接读取电池状态。`display_protocol.cpp` / X1 协议向主控报告已有电池字段，Startup 不通过该协议接收电池数据，因此本次不改变帧长度、命令或协议版本。Board 模拟器通过虚拟电池状态提供电流，C 键切换充电时采用 1600 mA / -350 mA；Linux / Simulator Demo 也提供同样的合理正负电流。

应用保留领域原始读数。Startup 将电压恢复为整数 mV，使用 64 位乘法计算整数 mW，再舍入为 0.1 W；有限电压范围检查避免 NaN/Inf 和浮点转整数溢出。StartupModel 与 View 共用最终文本生成函数，读数变化不足以改变显示文本时不重新通知 View。

## 回归验证

- `tests/firmware_tests.cpp`：fake I2C 覆盖 AverageCurrent 命令、小端有符号解码、int16 边界、空指针、总线/单寄存器读取失败、恢复和样本过期。
- `tests/display_app_tests.cpp`：X1Pro/XBase 本地电池电流映射；`tests/check_state_ownership.py` 检查状态写入者。
- `App/tests/domain_node_tests.cpp`：电流与有效位变化单独发布，重复样本不发布；`hal_device_checks.h` 覆盖新增字段的相等比较。
- `Vendor/Linux/Apps/Display/tests/app_startup_power_test.cpp`：实际 Startup 页面验证充电功率、49/50 mA 阈值、负值/无效值回退、停止充电、整数边界、非有限电压、文本变化去重和英俄文布局，并在传入目录输出 PPM 截图。

主机测试与编译不能替代实机验证；BQ40Z50 采样时序、校准精度和实际充电曲线尚需在硬件上确认。
