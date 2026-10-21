/*
 * This file is part of the EasyLogger Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2015-04-28
 */

#include <elog.h>

#if defined(_WIN32)
#include <windows.h>
static HANDLE output_lock = NULL;
#elif defined(__linux__)
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
static pthread_mutex_t output_lock = PTHREAD_MUTEX_INITIALIZER;
#else
#include <inttypes.h>
#include <stdio.h>
#include "delay.h"
#if defined(__CORE_DEBUG)
#include "SEGGER_RTT.h"
#endif
//#include "usart.h"
#endif

char buf[32];

/**
 * EasyLogger port initialize
 *
 * @return result
 */
ElogErrCode
elog_port_init(void) {
    ElogErrCode result = ELOG_NO_ERR;
#if defined(_WIN32)
    output_lock = CreateMutex(NULL, FALSE, NULL);
#elif defined(__linux__)
    pthread_mutex_init(&output_lock, NULL);
#elif defined(__CORE_DEBUG)
#ifdef SEGGER_RTT_H
    SEGGER_RTT_Init();
#else
    usart_init();
#endif
#endif //_WIN32
    return result;
}

/**
 * EasyLogger port deinitialize
 *
 */
void
elog_port_deinit(void) {
/* add your code here */
#if defined(_WIN32)
    CloseHandle(output_lock);
#elif defined(__linux__)
    pthread_mutex_destroy(&output_lock);
#else

#endif
}

/**
 * output log port interface
 *
 * @param log output of log
 * @param size log size
 */
void
elog_port_output(const char* log, size_t size) {
    /* add your code here */
#if defined(_WIN32)
    printf("%.*s", (int)size, log);
#elif defined(__linux__)
    fwrite(log, 1, size, stdout);
    fflush(stdout);
#elif defined(__CORE_DEBUG)
#ifdef SEGGER_RTT_H
    SEGGER_RTT_Write(0, log, size);
#else
    usart_write_buffer(log, size);
#endif
#else
    (void)log;
    (void)size;
#endif
}

/**
 * output lock
 */
void
elog_port_output_lock(void) {
    /* add your code here */
#if defined(_WIN32)
    WaitForSingleObject(output_lock, INFINITE);
#elif defined(__linux__)
    pthread_mutex_lock(&output_lock);
#else

#endif
}

/**
 * output unlock
 */
void
elog_port_output_unlock(void) {
    /* add your code here */
#if defined(_WIN32)
    ReleaseMutex(output_lock);
#elif defined(__linux__)
    pthread_mutex_unlock(&output_lock);
#else

#endif
}

/**
 * get current time interface
 *
 * @return current time
 */
const char*
elog_port_get_time(void) {
    /* add your code here */
    //		MakeTimeString(lv_tick_get(), buf, 32);
#if defined(_WIN32)
    static char cur_system_time[24] = {0};
    static SYSTEMTIME currTime;

    GetLocalTime(&currTime);
    snprintf(cur_system_time, 24, "%02d-%02d %02d:%02d:%02d.%03d", currTime.wMonth, currTime.wDay, currTime.wHour,
             currTime.wMinute, currTime.wSecond, currTime.wMilliseconds);

    return cur_system_time;
#elif defined(__linux__)
    static char cur_system_time[24] = {0};
    struct timespec ts;
    struct tm local_time;
    size_t len;
    int milliseconds;

    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &local_time);
    milliseconds = (int)(ts.tv_nsec / 1000000L);
    if (milliseconds < 0) {
        milliseconds = 0;
    } else if (milliseconds > 999) {
        milliseconds = 999;
    }
    len = strftime(cur_system_time, sizeof(cur_system_time), "%m-%d %H:%M:%S", &local_time);
    if (len > 0 && len < sizeof(cur_system_time) - 4) {
        snprintf(cur_system_time + len, sizeof(cur_system_time) - len, ".%03d", milliseconds);
    }

    return cur_system_time;
#else

    snprintf(buf, 32, "%" PRIu32, millis());
    return buf;
#endif
}

/**
 * get current process name interface
 *
 * @return current process name
 */
const char*
elog_port_get_p_info(void) {
    /* add your code here */
#if defined(_WIN32)
    static char cur_process_info[10] = {0};
    snprintf(cur_process_info, 10, "pid:%04ld", GetCurrentProcessId());

    return cur_process_info;

#elif defined(__linux__)
    static char cur_process_info[16] = {0};
    snprintf(cur_process_info, sizeof(cur_process_info), "pid:%04ld", (long)getpid());

    return cur_process_info;

#else
    return "";
#endif
}

/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char*
elog_port_get_t_info(void) {
    /* add your code here */
#if defined(_WIN32)
    static char cur_thread_info[10] = {0};

    snprintf(cur_thread_info, 10, "tid:%04ld", GetCurrentThreadId());

    return cur_thread_info;

#elif defined(__linux__)
    static char cur_thread_info[24] = {0};
    snprintf(cur_thread_info, sizeof(cur_thread_info), "tid:%lu", (unsigned long)pthread_self());

    return cur_thread_info;

#else
    return "";
#endif
}
