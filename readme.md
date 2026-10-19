# HC32F460xE Embedded UI Framework

**基于 HC32F460xE 的高性能嵌入式系统框架，集成 Arduino 兼容层、LVGL 界面库及跨平台模拟器。**
![alt text](https://img.shields.io/badge/MCU-HC32F460-blue.svg)

![alt text](https://img.shields.io/badge/UI-LVGL_v8.3.11-orange.svg)

![alt text](https://img.shields.io/badge/IDE-EIDE-green.svg)

![alt text](https://img.shields.io/badge/License-MIT-yellow.svg)

## 📖 项目简介

> 本项目是为小华半导体 (XHSC) HC32F460xE 系列开发的综合性嵌入式软件框架。项目参考了知名开源项目 X-TRACK 的页面管理逻辑，实现了从底层硬件驱动到高层 UI 交互的解耦设计。
> 核心意义：解决了国产 MCU 在高分 UI 场景下开发效率低、底层驱动与业务逻辑耦合严重的痛点

---

## ✨ 核心特性

 > 🚀 高性能 UI 引擎：基于 LVGL 8.x，针对 HC32F460 的 Cortex-M4 内核进行了性能优化，UI 动画丝滑且响应迅速。

> 🏗 现代化页面管理：参考 X-TRACK 的解耦设计，实现了一套基于应用（App）理念的页面切换机制，方便快速扩展功能模块。

> 💻 跨平台开发：提供基于 PC 的 TFT Simulator，支持在电脑端预览和调试 UI 逻辑，极大缩短开发周期。

> 🛠 完善的驱动封装：

> 基于 Arduino API 风格的底层抽象。
> 硬件驱动支持：GPIO, UART, I2C Slave (含特定通信协议), SPI, DMA, Display Control 等。

> ⚡ 低功耗优化：主循环内置 __WFI() 指令与事件驱动逻辑，平衡性能与功耗。

---

## 😄 实物演示

![alt text](https://github.com/NewNoob2002/HC32F460xE_Arduino/blob/master/Images/Image_mainPage.png)

## 📂 项目结构说明

> 项目采用分层架构，确保代码的可移植性和可维护性：

> ├── .cmsis / .eide    # 项目构建与工程配置 (支持 EIDE)

> ├── Arduino           # Arduino API 兼容层抽象实现

> ├── Libraries         # 第三方库 (LVGL, 字体, 图像资源)

> ├── Platform          # 硬件抽象层 (HAL) 与驱动定义，三产品共用

> ├── Shared            # 产品共用业务

> │   ├── Comm          # 与主控的 I2C 从机通信、面板帧协议核心（全部产品）

> │   └── Display       # 带屏产品入口、显示协议表、显示/编码器 HAL（X1Pro、XBase）

> ├── Products          # 产品配置与专属代码：X1（无屏）、X1Pro、XBase

> └── Simulator         # PC 端仿真器环境；Simulator/App 为带屏产品共用的 LVGL 页面

## 🛠 技术深度 (Technical Stack)

> 在本项目中，我完成了以下关键技术任务：

> 硬件抽象层 (HAL) 适配：将小华官方 DDL 库封装为 Arduino 风格接口，通过 HAL::HAL_Init() 统一管理外设生命周期。

> I2C 通信协议开发：实现了 slave_i2c 模块，用于 MCU 与主控或其他外设的可靠数据交换。

> UI 性能调优：在 V2.1.0 版本中重构了框架，优化了 LVGL 的 flush_cb 绘制逻辑，提升了 UI 的帧率和响应速度。

> 页面状态机设计：借鉴 X-TRACK 思想，在 App_Update 中实现了非阻塞的异步任务处理，确保 UI 刷新与后台数据读取互不干扰。

## 致谢

> 感谢[@[FASTSHIFT](https://github.com/FASTSHIFT/X-TRACK)]贡献的参考设计“[[页面生命周期管理](https://github.com/FASTSHIFT/X-TRACK/tree/main/Software/X-Track/USER/App/Utils/PageManager)]

