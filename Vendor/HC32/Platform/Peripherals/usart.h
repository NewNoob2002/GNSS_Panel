#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 调试日志只允许主循环调用；中断调用丢弃，发送超时后返回已写入字节数。
bool usart_init(void);
size_t usart_write_buffer(const uint8_t* buffer, size_t size);
int usart_write(int ch);
#ifdef __cplusplus
}
#endif
