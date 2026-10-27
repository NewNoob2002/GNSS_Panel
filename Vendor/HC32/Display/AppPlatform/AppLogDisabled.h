#pragma once
#undef APP_LOG_COMPILE_LEVEL
#define APP_LOG_COMPILE_LEVEL 4
#include "Log.h"

// 发布固件及 NONE 模式不求值日志参数，也不引入格式化实现。
#undef APP_LOG_D
#undef APP_LOG_I
#undef APP_LOG_W
#undef APP_LOG_E
#define APP_LOG_D(...) ((void)0)
#define APP_LOG_I(...) ((void)0)
#define APP_LOG_W(...) ((void)0)
#define APP_LOG_E(...) ((void)0)
