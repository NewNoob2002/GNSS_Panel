/*
 * MIT License
 * Copyright (c) 2021 _VIFEXTech
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef P4_PAGE_MANAGER_LOG_H
#define P4_PAGE_MANAGER_LOG_H

#include "Utils/Log/Log.h"
#define PM_LOG_DEBUG(...) APP_LOG_D("PageManager", __VA_ARGS__)
#define PM_LOG_INFO(...)  APP_LOG_I("PageManager", __VA_ARGS__)
#define PM_LOG_WARN(...)  APP_LOG_W("PageManager", __VA_ARGS__)
#define PM_LOG_ERROR(...) APP_LOG_E("PageManager", __VA_ARGS__)

#endif // P4_PAGE_MANAGER_LOG_H
