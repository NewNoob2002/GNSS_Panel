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
#include "DataBroker.h"
#include <algorithm>
#include <cstring>
#include <limits>

DataBroker::DataBroker(const char* name) : _mainNode(name, this) {
    // 主节点仅作观察入口，默认接收成功，避免无业务订阅者时返回无回调错误。
    _mainNode.setEventCallback([](DataNode*, DataNode::EventParam_t*) -> int { return DataNode::RES_OK; });
}

DataNode*
DataBroker::search(const char* id) {
    return search(&_nodePool, id);
}

DataNode*
DataBroker::search(const DataNode::DataNodeList_t* list, const char* id) {
    if (!list || !id || !*id) {
        return nullptr;
    }
    for (auto* node : *list) {
        if (node && std::strcmp(id, node->getID()) == 0) {
            return node;
        }
    }
    return nullptr;
}

bool
DataBroker::add(DataNode* node) {
    if (!node || !node->getID() || !*node->getID()) {
        return false;
    }
    if (node == &_mainNode) {
        node->_serial = ++_nextSerial;
        return true;
    }
    if (getNodeNumber() >= MaxNodes || search(node->getID()) || std::strcmp(node->getID(), _mainNode.getID()) == 0
        || _nextSerial == std::numeric_limits<uint64_t>::max()) {
        return false;
    }
    if (!append(_nodePool, node)) {
        return false;
    }
    node->_serial = ++_nextSerial;
    // 注册期间尚未置 registered；主节点只建立关系，不触发回调。
    append(_mainNode._publishers, node);
    append(node->_subscribers, &_mainNode);
    return true;
}

bool
DataBroker::remove(DataNode* node) {
    return remove(&_nodePool, node);
}

bool
DataBroker::remove(DataNode::DataNodeList_t* list, DataNode* node) {
    auto it = std::find(list->begin(), list->end(), node);
    if (it == list->end()) {
        return false;
    }
    std::move(it + 1, list->end(), it);
    list->back() = nullptr;
    return true;
}

bool
DataBroker::append(DataNode::DataNodeList_t& list, DataNode* node) {
    auto it = std::find(list.begin(), list.end(), nullptr);
    if (it == list.end()) {
        return false;
    }
    *it = node;
    return true;
}

size_t
DataBroker::getNodeNumber() const {
    return std::count_if(_nodePool.begin(), _nodePool.end(), [](const DataNode* n) { return n != nullptr; });
}

bool
DataBroker::alive(const DataNode* node, uint64_t serial) const {
    if (node == &_mainNode) {
        return _mainNode._serial == serial;
    }
    for (auto* live : _nodePool) {
        if (live == node && live && live->_serial == serial) {
            return true;
        }
    }
    return false;
}
