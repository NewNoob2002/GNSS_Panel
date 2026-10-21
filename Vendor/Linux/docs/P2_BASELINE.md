# P2 构建基线

构建日期：2026-09-18；复核日期：2026-09-21。P2 离线构建基线已完成；未烧录、未操作 GPIO、未验证实屏。

## 基线与结果

| 项目 | 配置/结果 |
| --- | --- |
| SDK | atk_dlrk3506_linux6.1_release_v1.3.1_20260326 |
| SDK 配置 | 22_atk_dlrk3506j_automipi_emmc_defconfig；512MB+8GB 测试板暂按 eMMC，部署前核实介质 |
| kernel | **通过**，6.1.118；两个 J/MIPI/eMMC DTB、zImage、resource.img、zboot.img |
| rootfs | **通过**，Buildroot alientek_rk3506，ext4，BusyBox init；2026-09-18 18:05（北京时间）完成，9 月 21 日只读完整性检查通过 |
| 原 Simulator | **补丁后通过编译**；SDL dummy 后端运行 15 秒，八页注册、Startup 活动及退出清理有日志 |
| 本项目应用 CMake | **主机端通过**；独立链接 LVGL 8.3.11，旋转后逻辑 294×126、RGB565 SWAP=1、离线 flush 与内存检查 |
| ARM 应用 | **通过**，Buildroot GCC 12.4.0/glibc 2.38；ELF32 ARM hard-float；QEMU 用户态检查通过 |

原厂 SDK 配置和源码未为本项目修改；spidev.bufsiz、屏幕 GPIO 和自启动未在 P2 加入。仅选择配置并生成构建产物。当前应用可执行目标是 lvgl_baseline，不是已迁移的业务程序，原页面 Linux 适配属于 P4。

构建前 kernel/device/Buildroot 子仓无本地修改，来源提交：

~~~text
kernel-6.1     979562a89a7a60edc76144d32c2a792d84deae5f
buildroot      ae9c7eeacc01f156f55a4db0f677a2f0d8bee4f1
device/rockchip 6ae29ad44801837a32dccea67f5278e45ac47756
HC32           28e89c1406e2c26ded7b402fa37aadf2c6d9155e
~~~

## Docker 与 SDK 复现

P2 在原 Dockerfile 增加 libsdl2-dev，以构建旧 Simulator。新镜像标签 rk3506-sdk:ubuntu20.04-p2，保留 v1 镜像；已构建 ID：

~~~text
sha256:3c7aa411003582e2a5f3902bcd55be64858c7b7a378349900c5c4da1c8bc2158
~~~

包清单和项目检查日志位于 out/p2/（不入版本库）。外层 rootfs.log 在会话中断后截断，完整 SDK 构建日志位于 SDK/output/sessions/2026-09-18_07-25-08/，以其 build.log 与 br.log 为准。Dockerfile 固定 Ubuntu 基础 digest，APT 包版本不冻结，需长期复现时保存镜像及包清单。

在项目根目录启动容器：

~~~sh
docker build -t rk3506-sdk:ubuntu20.04-p2 docker
docker run --rm -it --cpuset-cpus 0-7 \
  -v "$PWD:/work" -w /work \
  rk3506-sdk:ubuntu20.04-p2 bash
~~~

0-7 是本次主机的 CPU 限制；其他机器按可用 CPU 调整。容器内普通用户 builder，UID/GID 1000；不加 privileged 或设备映射。下面命令均在容器中执行：

~~~sh
cd /work/sdk/atk_dlrk3506_linux6.1_release_v1.3.1_20260326
./build.sh 22_atk_dlrk3506j_automipi_emmc_defconfig
./build.sh kernel
./build.sh rootfs
~~~

同一 SDK 的阶段顺序执行，避免多个 build.sh 同时改变配置/会话。kernel 使用预置 GCC 10.3.1；rootfs 的实际配置是内部构建 GCC 12.4.0/glibc 工具链。应用使用 Buildroot 生成的 CMake toolchainfile，匹配 rootfs 的 ABI、头文件和库。

内核产物：SDK/kernel-6.1/zboot.img（output/firmware/boot.img 为其链接）。本次 SHA256：

~~~text
zboot.img c3d26405ee849ccc8d58c0b6f597dd1d177a7def1af77c84d32a7e594f339883
zImage    c208fcdc23073a6990f96b28974b4b5e619b288bb8f4364b10f777a7830f6465
~~~

时间戳等构建元数据可能使重建哈希不同。该 boot.img 不等于全套烧录固件；P2 未构建/验收 U-Boot、recovery 或 update.img。

## 原 Simulator 对照

原工程保持只读。其 CMake 会写入源码树内 build/，因此必须在独立副本中构建。以下在**宿主项目根目录**运行，目标目录需是尚不存在的新路径：

~~~sh
hc32_source=/home/gtc/Desktop/workspace/HC32_PROJ/HC32F460xE_Arduino
reference=out/p2/hc32-reference-new
mkdir -p "$reference/Platform/HAL" "$reference/Libraries"
rsync -a --exclude build --exclude .git "$hc32_source/Simulator" "$reference/"
cp "$hc32_source/CMakeLists.txt" "$reference/"
cp -a "$hc32_source/Platform/Config" "$hc32_source/Platform/support" "$reference/Platform/"
cp "$hc32_source/Platform/HAL/CommonMacro.h" "$reference/Platform/HAL/"
cp -a "$hc32_source/Libraries/easylogger" "$reference/Libraries/"
patch -d "$reference" -p1 < patches/hc32/0001-simulator-hal-include.patch
~~~

补丁修复两处编译依赖：Page.h 在模拟器配置下选择 Simulator/HAL；模拟器 HAL 使用现有 EasyLogger 提供调试宏，并保留原公共宏依赖。未改页面业务、资源或 LVGL 内核；原样工程不能据此报告为无补丁通过。

容器中构建：

~~~sh
cd /work
cmake -S out/p2/hc32-reference-new/Simulator -B out/p2/simulator-new -DCMAKE_BUILD_TYPE=Debug
cmake --build out/p2/simulator-new -j4
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  timeout --signal=TERM --kill-after=3 15s \
  out/p2/hc32-reference-new/Simulator/build/bin/main
~~~

持续事件循环被 timeout 正常结束时返回 124；崩溃、断言和 SDL 初始化错误不能视作通过。本次日志为 out/p2/simulator-smoke.log，无窗口检查不覆盖视觉效果、全部页面切换或语言切换。Simulator 使用其自己的配置（1 MiB 内存池、SDL tick、SWAP=0），不能与目标配置混用。

## 本项目 LVGL 基线

源代码与配置归属：

- application/CMakeLists.txt：C11/C++17、私有 lvgl8311 静态库、离线检查目标。
- application/config/lv_conf.h：原 MCU 配置，未修改。
- application/baseline.c：无设备 IO 的基础绘制检查；小缓冲用于触发多次 flush，不是实屏 port。
- third_party/lvgl/：原 MCU 的 lvgl.h/src；来源和校验见 SOURCE.md。

主机端（容器内）：

~~~sh
cd /work
cmake -S application -B out/p2/lvgl-host -DCMAKE_BUILD_TYPE=Debug
cmake --build out/p2/lvgl-host -j4
(cd out/p2/lvgl-host && ctest --output-on-failure)
~~~

目标端构建入口（Buildroot 工具链生成后在同一挂载路径的容器内执行）：

~~~sh
cd /work
br_host=/work/sdk/atk_dlrk3506_linux6.1_release_v1.3.1_20260326/buildroot/output/alientek_rk3506/host
cmake -S application -B out/p2/lvgl-arm \
  -DCMAKE_TOOLCHAIN_FILE="$br_host/share/buildroot/toolchainfile.cmake" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build out/p2/lvgl-arm -j4
file out/p2/lvgl-arm/lvgl_baseline
"$br_host/bin/arm-buildroot-linux-gnueabihf-readelf" -h -A -l -d out/p2/lvgl-arm/lvgl_baseline
qemu-arm-static -L "$br_host/arm-buildroot-linux-gnueabihf/sysroot" out/p2/lvgl-arm/lvgl_baseline
~~~

QEMU 用户态执行仅检查目标 ABI、加载器和基础程序运行；不能验证 SPI/GPIO/板端调度或显示效果。

## 已完成的应用验证记录

主机端 CTest 1/1 通过，42 次 flush，报告可用 LVGL 内存 124912 字节。ARM 程序使用生成的 Buildroot 工具链编译，QEMU 运行返回 0，42 次 flush，可用内存 127468 字节。两者结构体/指针大小不同，空闲量不要求相同；单次检查不表示长期无泄漏。

ARM ELF：EABI5、v7、VFPv4/NEON、VFP 寄存器传参；动态加载器 /lib/ld-linux-armhf.so.3。NEEDED 为 libm.so.6、libc.so.6、ld-linux-armhf.so.3，均存在于目标 rootfs 的 target/lib。链接命令只使用本项目 liblvgl8311.a 和目标系统库，未混入 SDK LVGL 8.4.0。详细记录 out/p2/lvgl-arm.log。

配置快照：out/p2/sdk.config、kernel.config、buildroot.config；镜像包清单 out/p2/docker-packages.tsv。Simulator 启动日志有原配置主动启用内存/对象/样式断言的提示，不能用于性能比较。

## 2026-09-21 收尾核验

- SDK 内部 build.log 明确记录 build_buildroot 和 build_rootfs succeeded；.stamp_build_finish 时间为 2026-09-18 18:05:50 +0800。Docker 已无该构建容器，无需重新编译。
- output/firmware/rootfs.img 指向 buildroot/output/alientek_rk3506/images/rootfs.ext2；文件名虽为 ext2，实际为 **ext4**，rootfs.ext4 是同文件的链接。文件大小 415236096 字节（396 MiB）。
- e2fsck -fn 只读检查五个阶段均通过：6312 个文件，84658/101376 个块；记录见 out/p2/rootfs-check.log。未挂载镜像或修改文件系统。
- debugfs 只读检查镜像中的 /lib/modules，版本为 6.1.118，与构建内核一致。
- 使用最终 Buildroot target/ 中的库执行 ARM 基线，QEMU 返回 0，42 次 flush，LVGL 内存检查通过；见 out/p2/lvgl-rootfs-check.log。该检查不等同于板端启动或实屏验收。
- kernel、Buildroot、device/rockchip 三个子仓检查均无源码修改；原厂内核哈希与此前一致。

rootfs.ext2 SHA256：

~~~text
4995f41e4efe24e564decb70819e6e444490907cb66d00a07b9a76301509f719
~~~

内核与 rootfs 校验值一并保存在 out/p2/artifacts.sha256。SDK 还生成 cpio、squashfs、tar、ubi 等格式；本阶段以配置选用的 ext4 镜像为核验对象，不代表其他存储方案已验收。

SDK 的 ldconfig 阶段提示缺少可选 /etc/ld.so.conf，并跳过非 ELF 的 libstdc++ GDB Python 辅助文件；阶段最终成功，已用最终目标库验证当前 ARM 基线的动态加载。未据此宣称所有原厂服务都已通过运行测试。

后续进入 P3 的 RM690A0/spidev/GPIO 显示实现；原页面迁移属于 P4，业务与自启动属于 P5。当前没有完整 update.img，也未执行烧录。
