# Survey 逻辑与数据处理应用

Survey 独立于屏幕应用，先在旧平台 EG25-G 复刻此前的 Survey 应用，再移植到 RK3506。旧 Survey 源码尚未进入本仓库；当前仅提供启动时打印信息并正常退出的 `survey_app`，CTest 的 `survey_smoke` 覆盖该入口。

规划模块：GNSS 板卡数据交互、Wi-Fi WebServer、蓝牙与手机 App 交互、电台。具体业务实现需以旧 Survey 源码及实际设备接口为依据。

Survey 仅依赖标准库，不链接 LVGL、共享 `App/`、`Frameworks/` 或 `Apps/Display/`；Display 的演示数据后端仍归 Display 所有。与 Display 的数据接缝、通信方式及协议均待定，本轮不预选 IPC、socket 或消息协议。

在仓库根目录独立构建并运行：

```sh
cmake -S Vendor/Linux -B build/linux-survey -DLINUX_APP_DISPLAY=OFF
cmake --build build/linux-survey -j
ctest --test-dir build/linux-survey --output-on-failure
build/linux-survey/Apps/Survey/survey_app
```

入口采用 C++98，避免将显示应用的 C++17 要求带给 EG25-G。EG25-G 的编译器版本、sysroot、libc、ABI、部署及启动方式均待确认；当前不提供推测的交叉工具链配置。主机冒烟通过不代表 EG25-G 或 RK3506 板端已验证。
