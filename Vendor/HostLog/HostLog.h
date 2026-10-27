#pragma once

#include <memory>
#include <spdlog/sinks/sink.h>

// 启动时安装；可注入同步 sink，默认输出带自动终端颜色的 stdout。
void host_log_init(std::shared_ptr<spdlog::sinks::sink> sink = {});
