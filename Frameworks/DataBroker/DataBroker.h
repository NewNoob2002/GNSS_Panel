/*
 * MIT License
 * Copyright (c) 2021 - 2024 _VIFEXTech
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
#pragma once
#include "DataNode.h"
#include "DataTimer.h"

/** 同步数据路由；应用线程独占，必须比节点及其定时器长寿。 */
class DataBroker {
    friend class DataNode;
public:
  static constexpr size_t MaxNodes = 16;
  /** 构造值成员，避免在禁用异常的固件上隐式分配失败。 */
  explicit DataBroker(const char* name);
  /** 释放主节点订阅；调用前必须已销毁所有外部节点。 */
  ~DataBroker() = default;
  DataBroker(const DataBroker&) = delete;
  DataBroker& operator=(const DataBroker&) = delete;
  /** 返回外部节点数，不包含主节点。 */
  size_t getNodeNumber() const;

  /** 返回自动订阅所有外部节点的主节点。 */
  DataNode*
  mainNode() {
      return &_mainNode;
  }

  /** 设置毫秒时钟；存在定时器时禁止更换时钟。 */
  bool
  initTimerManager(DataTimerManager::TickFunction_t clock) {
      return _timerManager.setTickCallback(clock);
  }

  /** 处理一次到期事件，不追赶遗漏周期。 */
  uint32_t
  handleTimer() {
      return _timerManager.handler();
  }

  /** 按名称查找注册节点；空参数返回空指针。 */
  DataNode* search(const char* id);

  /** 返回独立于 LVGL 的定时器管理器。 */
  DataTimerManager*
  timerManager() {
      return &_timerManager;
  }

private:
  /** 注册节点并分配不会重复的存活序号。 */
  bool add(DataNode* node);
  /** 注销节点，不释放调用方拥有的对象。 */
  bool remove(DataNode* node);
  /** 从定长关系表中移除节点并保持顺序。 */
  static bool remove(DataNode::DataNodeList_t* list, DataNode* node);
  /** 向关系表加入节点，容量不足返回失败。 */
  static bool append(DataNode::DataNodeList_t& list, DataNode* node);
  /** 在关系表中按名称查找。 */
  static DataNode* search(const DataNode::DataNodeList_t* list, const char* id);
  /** 不解引用失效指针，校验地址与注册序号。 */
  bool alive(const DataNode* node, uint64_t serial) const;
  DataNode::DataNodeList_t _nodePool{};
  uint64_t _nextSerial = 0;
  DataTimerManager _timerManager;
  DataNode _mainNode;
};
