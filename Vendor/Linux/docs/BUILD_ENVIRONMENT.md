# SDK Docker 编译环境

P2 更新：Dockerfile 已增加 libsdl2-dev，镜像 rk3506-sdk:ubuntu20.04-p2 已构建；实际 kernel/rootfs 与应用验证记录见 [P2 构建基线](P2_BASELINE.md)。下面的构建/启动命令统一使用含 SDL2 的 P2 镜像；第 6 节 v1 验证记录保留为 P0 历史结果。

## 1. 选择与边界

依据本地 SDK 开发手册 V1.5 §1.2–1.4，使用官方 `ubuntu:20.04`、`linux/amd64`，在其上安装手册全部依赖。镜像用于 x86_64 主机上的 ARM32 交叉编译，容器本身不是 ARM 镜像。

基础镜像已拉取；`docker/Dockerfile` 固定本次拉取的 digest：

```text
ubuntu:20.04@sha256:8feb4d8ca5354def3d8fce243717141ce31e2c428701f6682bd2fafe15388214
```

当前复现镜像标签：`rk3506-sdk:ubuntu20.04-p2`；`rk3506-sdk:ubuntu20.04-v1` 是 P0 历史镜像。Dockerfile 固定基础镜像但未冻结 APT 仓库快照，因此重建时软件包补丁版本可能变化；需要长期复现时保留本次镜像及包清单。工具链来自挂载的 SDK，不另下无关版本。宿主已存在的 RK3588/ROS 镜像不替代本项目指定环境。

## 2. 重建镜像

在项目根目录运行：

```sh
docker pull --platform linux/amd64 ubuntu:20.04@sha256:8feb4d8ca5354def3d8fce243717141ce31e2c428701f6682bd2fafe15388214
docker build --platform linux/amd64 \
  --build-arg HOST_UID="$(id -u)" --build-arg HOST_GID="$(id -g)" \
  -t rk3506-sdk:ubuntu20.04-p2 -f docker/Dockerfile docker
```

构建上下文仅为 `docker/`，不会上传整套 SDK。当前本地主机 UID/GID 为 1000:1000，对应容器普通用户 `builder`；其他机器重建时保持 UID/GID 与 SDK 文件所有者一致，避免厂商 `check-sdk.sh` 拒绝编译。不要用 root 编译或对 SDK 批量 chown。若账户没有 Docker socket 访问权限，使用宿主已有授权机制执行 Docker 命令；无需改变 socket 权限。

## 3. 环境检查

在项目根目录执行；只读挂载工程，临时编译文件写容器 `/tmp`：

```sh
docker run --rm --network none \
  --mount "type=bind,source=$PWD,target=/work,readonly" \
  rk3506-sdk:ubuntu20.04-p2 \
  bash /work/docker/check-environment.sh \
  /work/sdk/atk_dlrk3506_linux6.1_release_v1.3.1_20260326
```

检查 Ubuntu 版本、非 root 身份及 SDK 所有者、厂商 SDK 预检、关键宿主工具、宿主 64/32 位程序编译运行、SDK Linux 工具链编译链接，以及 ARM ELF / hard-float 标记。此检查不执行 ARM 程序，不改 SDK 配置，也不编译完整固件。

## 4. 进入 SDK 编译环境

确认主机可用空间至少 100 GB、内存建议至少 8 GB，并为 Docker 镜像、下载缓存和构建产物留余量。SDK 必须通过 bind mount 放在主机 Linux 文件系统中，厂商脚本检查 ext*/f2fs/btrfs；不要把源码放到容器临时 overlay 层。

```sh
docker run --rm -it \
  --mount "type=bind,source=$PWD,target=/work" \
  --workdir /work/sdk/atk_dlrk3506_linux6.1_release_v1.3.1_20260326 \
  rk3506-sdk:ubuntu20.04-p2
```

普通用户、无 `--privileged`、无硬件设备映射。容器退出后 SDK 的下载缓存和输出仍保留在主机 SDK 工作区。镜像已安装工具不受容器删除影响；交互容器内临时安装的工具不会保留，应修改 Dockerfile 后重建。

## 5. 后续 SDK 编译入口

先确认实际板卡存储类型，选择 `device/rockchip/rk3506/` 下 J 型对应配置。不要仅因为 SoC 名称相同，就选择 B 型、AMP、Thunderboot 或不同存储配置。

容器内流程：

```sh
./build.sh help
./build.sh lunch
# 在交互菜单中选择与实板一致的 RK3506J 配置后：
./build.sh kernel
./build.sh rootfs
# 需要完整基线时依手册运行：
./build.sh
```

当前用户确认测试板为 RK3506J 512MB+8GB，P1 将 `22_atk_dlrk3506j_automipi_emmc_defconfig` 定为 P2 工作基线，暂按 8GB eMMC 方案准备；具体存储介质、运行 DTB 和实际容量在板端核实。P2 已选择该配置并完成 kernel/rootfs 编译，2026-09-21 已复核产物，详见 P2 构建基线记录。该非 RGB 基线与已确认的屏控制线无已知静态复用冲突，项目专用 OLED 配置后续建立。编译日志和输出位置以 SDK 控制台实际报告为准。

应用交叉编译使用所选 Buildroot 生成的 host 工具链与 staging/sysroot，或 SDK 导出的开发包；确认目标 libc、动态加载器与 LVGL 编译配置一致。环境检查使用预置 `arm-none-linux-gnueabihf-gcc` 证明其可以运行和链接，不保证其内置 sysroot 与未来 rootfs 完全一致。`arm-none-eabi-gcc` 为裸机用途，不用于 Linux 应用。

变更入口：内核配置通过 `./build.sh kernel-config`，Buildroot 通过 `./build.sh buildroot-config`；按 SDK 流程保存回源 defconfig/fragment 后再导出补丁。应用包变更按已有 `lvgl_demo.mk` 的 local/CMake 方式接入，不手改 `output/build`。

## 6. P0 历史验证记录（v1 镜像）

- 2026-09-18：官方 Ubuntu 20.04 基础镜像拉取成功；上述依赖镜像构建成功。
- 编译镜像本地 ID：`sha256:034f198bc3e067e432cc219d97e93860369184340a3b1f7c8d1aeb4f6917fab4`（本次 BuildKit 导出的 manifest list，未推送远端）。
- 只读挂载、禁用网络的环境检查通过：Ubuntu 20.04、UID/GID 1000、厂商 `check-sdk.sh`、宿主 64/32 位程序编译运行、GCC 10.3.1 ARM 交叉编译链接。产物为 ELF32 ARM / EABI5，VFP 参数传递，解释器 `/lib/ld-linux-armhf.so.3`。
- 当次 P0 环境检查未验证完整 SDK、kernel/rootfs、真实 OLED 或板端运行。后续 P2 已完成 kernel/rootfs 与 ARM 基线构建，P3/P4 已有实屏和导航记录，分别见 [P2](P2_BASELINE.md)、[P3](P3_DISPLAY.md)、[P4](P4_APPLICATION.md)；这不代表完整 `update.img` 或最新提交已通过板端验证。

## 7. 故障定位

| 现象 | 处理 |
| --- | --- |
| Docker socket permission denied | 使用已批准的 Docker 执行权限；不改 socket 为全局可写 |
| SDK owner 不匹配 | 用 SDK 所有者 UID/GID 重建镜像，检查 bind mount 路径 |
| 提示 ext4 / 文件系统不支持 | 检查挂载的主机文件系统；不要绕过厂商检查 |
| APT/源码下载失败 | 检查 Docker 网络、代理、DNS及下载站；保留日志后重试 |
| 缺少宿主命令/头文件 | 按具体错误补 Dockerfile，在容器里修复宿主依赖 |
| 目标程序加载失败 | 检查 ARM32 ABI、解释器和目标 sysroot，不复制宿主库 |
| 某 rootfs 流程要求特权 | 先确认所选构建阶段；本镜像已准备 Buildroot 流程，不自动开启 privileged |
