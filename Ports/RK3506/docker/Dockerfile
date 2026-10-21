FROM ubuntu:20.04@sha256:8feb4d8ca5354def3d8fce243717141ce31e2c428701f6682bd2fafe15388214

ARG DEBIAN_FRONTEND=noninteractive
# SDK 开发手册 V1.5 §1.4，加上构建和诊断所需的基础工具。
RUN apt-get update && apt-get install -y --no-install-recommends \
    git ssh make gcc libssl-dev liblz4-tool expect expect-dev g++ patchelf \
    chrpath gawk texinfo diffstat binfmt-support qemu-user-static live-build \
    bison flex fakeroot cmake gcc-multilib g++-multilib unzip \
    device-tree-compiler ncurses-dev bzip2 expat gpgv2 cpp-aarch64-linux-gnu \
    libgmp-dev libmpc-dev bc python-is-python3 python2 \
    ca-certificates wget curl rsync cpio file locales patch xz-utils \
    build-essential pkg-config python3-pip python3-setuptools \
    libelf-dev zip time libsdl2-dev && \
    locale-gen en_US.UTF-8 && rm -rf /var/lib/apt/lists/*

ARG HOST_UID=1000
ARG HOST_GID=1000
RUN groupadd --gid ${HOST_GID} builder && \
    useradd --uid ${HOST_UID} --gid ${HOST_GID} --create-home --shell /bin/bash builder
ENV LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8
USER builder
WORKDIR /work
CMD ["/bin/bash"]
