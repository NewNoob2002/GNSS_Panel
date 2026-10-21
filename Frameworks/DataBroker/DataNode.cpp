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
#include "DataNode.h"
#include <algorithm>
#include <cstring>
#include <new>
#include <utility>
#include "DataBroker.h"
#include "DataBrokerLog.h"
#include "DataTimer.h"

DataNode::DataNode(const char* id, DataBroker* broker, void* userData)
    : _ID(id ? id : ""), _broker(broker), _userData(userData), _eventCallback(nullptr), _eventFilterMask(EVENT_ALL),
      _timer(nullptr) {
    _registered = _broker && _broker->add(this);

    DN_LOG_INFO("DataNode[%s] created", _ID);
}

DataNode::~DataNode()
{
    DN_LOG_INFO("DataNode[%s] deleting...", _ID);

    /* Delete timer */
    if (_timer) {
        delete _timer;
        _timer = nullptr;
    }

    // 关系表不分配；注销双方关系，避免发布者销毁后留下悬挂订阅。
    for (auto* sub : _subscribers) {
        if (sub) {
            _broker->remove(&sub->_publishers, this);
        }
    }
    for (auto* pub : _publishers) {
        if (pub) {
            _broker->remove(&pub->_subscribers, this);
        }
    }
    if (_registered) {
        _broker->remove(this);
    }
    DN_LOG_INFO("DataNode[%s] deleted", _ID);
}

const DataNode* DataNode::subscribe(const char* pubID)
{
    if (!_registered || !pubID || !*pubID) {
        return nullptr;
    }
    /* Not allowed to subscribe to itself */
    if (strcmp(pubID, _ID) == 0) {
        DN_LOG_ERROR("DataNode[%s] try to subscribe to it itself", _ID);
        return nullptr;
    }

    /* Check subscribe repeatedly */
    DataNode* pub = _broker->search(&_publishers, pubID);
    if (pub) {
        DN_LOG_WARN("Multi subscribe pub[%s]", pubID);
        return nullptr;
    }

    /* Check node is created */
    pub = _broker->search(pubID);
    if (!pub) {
        DN_LOG_ERROR("pub[%s] was not found", pubID);
        return nullptr;
    }

    /* Add the publisher to the subscription list */
    if (!_broker->append(_publishers, pub)) {
        return nullptr;
    }

    /* Let the publisher add this subscriber */
    if (!_broker->append(pub->_subscribers, this)) {
        _broker->remove(&_publishers, pub);
        return nullptr;
    }

    DN_LOG_INFO("sub[%s] subscribed pub[%s]", _ID, pubID);

    return pub;
}

bool DataNode::unsubscribe(const char* pubID)
{
    if (!_registered || !pubID || !*pubID) {
        return false;
    }
    /* Whether to subscribe to the publisher */
    DataNode* pub = _broker->search(&_publishers, pubID);
    if (!pub) {
        DN_LOG_WARN("sub[%s] was not subscribe pub[%s]", _ID, pubID);
        return false;
    }

    /* Remove the publisher from the subscription list */
    _broker->remove(&_publishers, pub);

    /* Let the publisher remove this subscriber */
    _broker->remove(&pub->_subscribers, this);

    return true;
}

int DataNode::publish(const void* data_p, size_t size)
{
    if (!_registered || !data_p || !size) {
        return RES_PARAM_ERROR;
    }
    int result = RES_OK;
    // 地址加序号快照：回调删除或原址重建后续订阅者时不能再向旧对象分发。
    std::array<std::pair<DataNode*, uint64_t>, 17> snapshot{};
    for (size_t i = 0; i < _subscribers.size(); ++i) {
        auto* sub = _subscribers[i];
        if (sub) {
            snapshot[i] = {sub, sub->_serial};
        }
    }
    for (const auto& entry : snapshot) {
        auto* sub = entry.first;
        if (!sub || !_broker->alive(sub, entry.second)
            || std::find(_subscribers.begin(), _subscribers.end(), sub) == _subscribers.end()) {
            continue;
        }
        EventParam_t param(this, sub, EVENT_PUBLISH, const_cast<void*>(data_p), size);
        result = sendEvent(sub, &param);
        if (result == RES_STOP_PROCESS) {
            break;
        }
    }
    return result;
}

int DataNode::pull(const char* pubID, void* data_p, size_t size)
{
    if (!_registered || !pubID || !*pubID || !data_p || !size) {
        return RES_PARAM_ERROR;
    }
    DataNode* pub = _broker->search(&_publishers, pubID);
    if (!pub) {
        DN_LOG_ERROR("sub[%s] was not subscribe pub[%s]", _ID, pubID);
        return RES_NOT_FOUND;
    }
    return pull(pub, data_p, size);
}

int DataNode::pull(const DataNode* pub, void* data_p, size_t size)
{
    if (!_registered || !pub || !data_p || !size) {
        return RES_PARAM_ERROR;
    }
    if (std::find(_publishers.begin(), _publishers.end(), pub) == _publishers.end()) {
        return RES_NOT_FOUND;
    }

    DN_LOG_INFO("sub[%s] pull << data(%p)[%zu] << pub[%s] ...",
        _ID, data_p, size, pub->_ID);

    EventParam_t param(this, pub, EVENT_PULL, data_p, size);
    int ret = sendEvent((DataNode*)pub, &param);

    DN_LOG_INFO("pull done: %d", ret);
    return ret;
}

int DataNode::notify(const char* pubID, const void* data_p, size_t size)
{
    if (!_registered || !pubID || !*pubID || !data_p || !size) {
        return RES_PARAM_ERROR;
    }
    DataNode* pub = _broker->search(&_publishers, pubID);
    if (!pub) {
        DN_LOG_ERROR("sub[%s] was not subscribe pub[%s]", _ID, pubID);
        return RES_NOT_FOUND;
    }

    return notify(pub, data_p, size);
}

int DataNode::notify(const DataNode* pub, const void* data_p, size_t size)
{
    if (!_registered || !pub || !data_p || !size) {
        return RES_PARAM_ERROR;
    }
    if (std::find(_publishers.begin(), _publishers.end(), pub) == _publishers.end()) {
        return RES_NOT_FOUND;
    }

    DN_LOG_INFO("sub[%s] notify >> data(%p)[%zu] >> pub[%s] ...",
        _ID, data_p, size, pub->_ID);

    EventParam_t param(this, pub, EVENT_NOTIFY, (void*)data_p, size);
    int ret = sendEvent((DataNode*)pub, &param);

    DN_LOG_INFO("send done: %d", ret);

    return ret;
}

void DataNode::setEventCallback(EventCallback_t callback, uint32_t eventFilter)
{
    _eventCallback = callback;
    _eventFilterMask = eventFilter;
}

void DataNode::setEventFilter(uint32_t eventFilter)
{
    _eventFilterMask = eventFilter;
}

int DataNode::onEvent(EventParam_t* param)
{
    if (!_eventCallback) {
        DN_LOG_INFO("DataNode[%s] not register callback", _ID);
        return RES_NO_CALLBACK;
    }

    return _eventCallback(this, param);
}

int DataNode::sendEvent(DataNode* node, EventParam_t* param)
{
    if (!(node->_eventFilterMask & param->event)) {
        return RES_UNSUPPORTED_REQUEST;
    }

    return node->onEvent(param);
}

void DataNode::timerCallbackHandler(DataTimer* timer)
{
    auto instance = (DataNode*)timer->getUserData();

    EventParam_t param(instance, instance, EVENT_TIMER, nullptr, 0);
    instance->sendEvent(instance, &param);
}

void DataNode::startTimer(uint32_t period)
{
    if (!_registered) {
        return;
    }
    if (!period) {
        delete _timer;
        _timer = nullptr;
        return;
    }
    if (_timer) {
        _timer->setPeriod(period);
        _timer->resume();
        return;
    }

    _timer = new (std::nothrow) DataTimer(_broker->timerManager(), timerCallbackHandler, period, this);
    if (!_timer || !_timer->isRegistered()) {
        delete _timer;
        _timer = nullptr;
        DN_LOG_ERROR("DataNode[%s] timer registration failed", _ID);
    }
}

void DataNode::setTimerPeriod(uint32_t period)
{
    startTimer(period);
}

void DataNode::resetTimer()
{
    if (!_timer) {
        DN_LOG_ERROR("Timer not created");
        return;
    }

    _timer->reset();
}

void DataNode::stopTimer()
{
    if (!_timer) {
        return;
    }

    _timer->pause();

    DN_LOG_INFO("DataNode[%s] timer stop", _ID);
}

size_t DataNode::getPublishersNumber()
{
    return std::count_if(_publishers.begin(), _publishers.end(), [](const DataNode* n) { return n != nullptr; });
}

size_t DataNode::getSubscribersNumber()
{
    return std::count_if(_subscribers.begin(), _subscribers.end(), [](const DataNode* n) { return n != nullptr; });
}
