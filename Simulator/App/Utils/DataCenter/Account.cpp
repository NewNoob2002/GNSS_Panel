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
#include "Account.h"
#include <algorithm>
#include <array>
#include <cstring>
#include "DataCenter.h"
#include "DataCenterLog.h"

#define ACCOUNT_DISCARD_READ_DATA 1

/**
  * @brief  Account constructor
  * @param  id:       Unique name
  * @param  center:   Pointer to the data center
  * @param  bufSize:  The length of the data to be cached
  * @param  userData: Point to the address of user-defined data
  * @retval None
  */
Account::Account(const char* id, DataCenter* center, uint32_t bufSize, void* userData) {
    memset(&priv, 0, sizeof(priv));

    ID = id ? id : "";
    Center = center;
    UserData = userData;

    if (!id || !*id || !center)
        return;
    const size_t allocation_bytes = static_cast<size_t>(bufSize) * 2;
    if (allocation_bytes / 2 != bufSize)
        return;
    if (bufSize != 0) {
        uint8_t* buffer = (uint8_t*)lv_mem_alloc(allocation_bytes);

        if (!buffer) {
            DC_LOG_ERROR("Account[%s] buffer malloc failed!", ID);
            return;
        }

        memset(buffer, 0, allocation_bytes);

        uint8_t* buf0 = buffer;
        uint8_t* buf1 = buffer + bufSize;

        PingPongBuffer_Init(&priv.BufferManager, buf0, buf1);
        DC_LOG_INFO("Account[%s] cached %lu x2 bytes", ID, static_cast<unsigned long>(bufSize));
        priv.BufferSize = bufSize;
    }

    registered_ = Center->AddAccount(this) || this == &Center->AccountMain;

    if (registered_)
        DC_LOG_INFO("Account[%s] created", ID);
    else
        DC_LOG_ERROR("Account[%s] registration failed", ID);
}

/**
  * @brief  Account destructor
  * @param  None
  * @retval None
  */
Account::~Account() {
    DC_LOG_INFO("Account[%s] deleting...", ID);

    /* Release cache */
    if (priv.BufferSize) {
        lv_mem_free(priv.BufferManager.buffer[0]);
    }

    /* Delete timer */
    if (priv.timer) {
        lv_timer_del(priv.timer);
        DC_LOG_INFO("Account[%s] task deleted", ID);
    }

    /* Let subscribers unfollow */
    // Unsubscribe erases this vector: a range-for here invalidated its own iterator upstream.
    while (!subscribers.empty()) {
        Account* sub = subscribers.back();
        if (!sub->Unsubscribe(ID))
            subscribers.pop_back();
    }

    /* Ask the publisher to delete this subscriber */
    for (auto iter : publishers) {
        Center->Remove(&iter->subscribers, this);
        DC_LOG_INFO("pub[%s] removed sub[%s]", iter->ID, ID);
    }

    /* Let the data center delete the account */
    if (registered_)
        Center->RemoveAccount(this);
    DC_LOG_INFO("Account[%s] deleted", ID);
}

/**
  * @brief  Subscribe to Publisher
  * @param  pubID: Publisher ID
  * @retval Pointer to publisher
  */
Account* Account::Subscribe(const char* pubID) {
    if (!registered_ || !pubID)
        return nullptr;
    /* Not allowed to subscribe to yourself */
    if (strcmp(pubID, ID) == 0) {
        DC_LOG_ERROR("Account[%s] try to subscribe to it itself", ID);
        return nullptr;
    }

    /* Whether to subscribe repeatedly */
    Account* pub = Center->Find(&publishers, pubID);
    if (pub != nullptr) {
        DC_LOG_ERROR("Multi subscribe pub[%s]", pubID);
        return nullptr;
    }

    /* BWhether the account is created */
    pub = Center->SearchAccount(pubID);
    if (pub == nullptr) {
        DC_LOG_ERROR("pub[%s] was not found", pubID);
        return nullptr;
    }

    /* Add the publisher to the subscription list */
    publishers.push_back(pub);

    /* Let the publisher add this subscriber */
    pub->subscribers.push_back(this);

    DC_LOG_INFO("sub[%s] subscribed pub[%s]", ID, pubID);

    return pub;
}

/**
  * @brief  Unsubscribe from publisher
  * @param  pubID: Publisher ID
  * @retval Return true if unsubscribe is successful
  */
bool Account::Unsubscribe(const char* pubID) {
    /* Whether to subscribe to the publisher */
    Account* pub = Center->Find(&publishers, pubID);
    if (pub == nullptr) {
        DC_LOG_WARN("sub[%s] was not subscribe pub[%s]", ID, pubID);
        return false;
    }

    /* Remove the publisher from the subscription list */
    Center->Remove(&publishers, pub);

    /* Let the publisher add this subscriber */
    Center->Remove(&pub->subscribers, this);

    return true;
}

/**
  * @brief  Submit data to cache
  * @param  data_p: Pointer to data
  * @param  size:   The size of the data
  * @retval Return true if the submission is successful
  */
bool Account::Commit(const void* data_p, uint32_t size) {
    if (!registered_ || !data_p || !size || size != priv.BufferSize) {
        DC_LOG_ERROR("pub[%s] has not cache", ID);
        return false;
    }

    void* wBuf;
    PingPongBuffer_GetWriteBuf(&priv.BufferManager, &wBuf);

    memcpy(wBuf, data_p, size);

    PingPongBuffer_SetWriteDone(&priv.BufferManager);

    DC_LOG_DEBUG("pub[%s] commit data(%p)[%lu] >> data(%p)[%lu] done", ID, data_p, static_cast<unsigned long>(size),
                 wBuf, static_cast<unsigned long>(size));

    return true;
}

/**
  * @brief  Publish data to subscribers
  * @param  None
  * @retval error code
  */
int Account::Publish() {
    int retval = RES_UNKNOW;

    if (priv.BufferSize == 0) {
        DC_LOG_ERROR("pub[%s] has not cache", ID);
        return RES_NO_CACHE;
    }

    void* rBuf;
    if (!PingPongBuffer_GetReadBuf(&priv.BufferManager, &rBuf)) {
        DC_LOG_WARN("pub[%s] data was not commit", ID);
        return RES_NO_COMMITED;
    }

    EventParam_t param;
    param.event = EVENT_PUB_PUBLISH;
    param.tran = this;
    param.recv = nullptr;
    param.data_p = rBuf;
    param.size = priv.BufferSize;

    // Bounded snapshot with registration identities: callbacks may remove/recreate subscribers.
    if (subscribers.size() > DataCenter::MaxAccounts)
        return RES_PARAM_ERROR;
    std::array<std::pair<Account*, uint64_t>, DataCenter::MaxAccounts> snapshot{};
    const auto count = subscribers.size();
    for (size_t i = 0; i < count; ++i)
        snapshot[i] = {subscribers[i], subscribers[i]->registration_serial_};
    for (size_t i = 0; i < count; ++i) {
        Account* sub = snapshot[i].first;
        if (!Center->IsAlive(sub, snapshot[i].second)
            || std::find(subscribers.begin(), subscribers.end(), sub) == subscribers.end())
            continue;
        EventCallback_t callback = sub->priv.eventCallback;

        DC_LOG_DEBUG("pub[%s] publish >> data(%p)[%lu] >> sub[%s]...", ID, param.data_p,
                     static_cast<unsigned long>(param.size), sub->ID);

        if (callback != nullptr) {
            param.recv = sub;
            int ret = callback(sub, &param);

            DC_LOG_DEBUG("publish done: %d", ret);
            if (ret != RES_OK)
                DC_LOG_WARN("Publish %s -> subscriber(%p) rejected: %d", ID, static_cast<void*>(sub), ret);
            retval = ret;
        } else {
            DC_LOG_DEBUG("sub[%s] has no callback", sub->ID);
        }
    }

#if ACCOUNT_DISCARD_READ_DATA
    PingPongBuffer_SetReadDone(&priv.BufferManager);
#endif

    return retval;
}

/**
  * @brief  Pull data from the publisher
  * @param  pubID:  Publisher ID
  * @param  data_p: Pointer to data
  * @param  size:   The size of the data
  * @retval error code
  */
int Account::Pull(const char* pubID, void* data_p, uint32_t size) {
    if (!registered_ || !pubID || !data_p || !size)
        return RES_PARAM_ERROR;
    Account* pub = Center->Find(&publishers, pubID);
    if (pub == nullptr) {
        DC_LOG_ERROR("sub[%s] was not subscribe pub[%s]", ID, pubID);
        return RES_NOT_FOUND;
    }
    return Pull(pub, data_p, size);
}

int Account::Pull(Account* pub, void* data_p, uint32_t size) {
    int retval = RES_UNKNOW;

    if (pub == nullptr) {
        return RES_NOT_FOUND;
    }

    DC_LOG_DEBUG("sub[%s] pull << data(%p)[%lu] << pub[%s] ...", ID, data_p, static_cast<unsigned long>(size), pub->ID);

    EventCallback_t callback = pub->priv.eventCallback;
    if (callback != nullptr) {
        EventParam_t param;
        param.event = EVENT_SUB_PULL;
        param.tran = this;
        param.recv = pub;
        param.data_p = data_p;
        param.size = size;

        int ret = callback(pub, &param);

        DC_LOG_DEBUG("pull done: %d", ret);
        retval = ret;
    } else {
        DC_LOG_DEBUG("pub[%s] has no pull callback, read commit cache...", pub->ID);

        if (pub->priv.BufferSize == size) {
            void* rBuf;
            if (PingPongBuffer_GetReadBuf(&pub->priv.BufferManager, &rBuf)) {
                memcpy(data_p, rBuf, size);
#if ACCOUNT_DISCARD_READ_DATA
                PingPongBuffer_SetReadDone(&pub->priv.BufferManager);
#endif
                DC_LOG_DEBUG("read done");
                retval = 0;
            } else {
                DC_LOG_WARN("pub[%s] data was not commit!", pub->ID);
            }
        } else {
            DC_LOG_ERROR("Data size pub[%s]:%lu != sub[%s]:%lu", pub->ID,
                         static_cast<unsigned long>(pub->priv.BufferSize), this->ID, static_cast<unsigned long>(size));
        }
    }

    return retval;
}

/**
  * @brief  Send a notification to the publisher
  * @param  pubID: Publisher ID
  * @param  data_p: Pointer to data
  * @param  size:   The size of the data
  * @retval error code
  */
int Account::Notify(const char* pubID, const void* data_p, uint32_t size) {
    if (!registered_ || !pubID || !data_p || !size)
        return RES_PARAM_ERROR;
    Account* pub = Center->Find(&publishers, pubID);
    if (pub == nullptr) {
        DC_LOG_ERROR("sub[%s] was not subscribe pub[%s]", ID, pubID);
        return RES_NOT_FOUND;
    }
    return Notify(pub, data_p, size);
}

/**
  * @brief  Send a notification to the publisher
  * @param  pub:    Pointer to publisher
  * @param  data_p: Pointer to data
  * @param  size:   The size of the data
  * @retval error code
  */
int Account::Notify(Account* pub, const void* data_p, uint32_t size) {
    int retval = RES_UNKNOW;

    if (pub == nullptr) {
        return RES_NOT_FOUND;
    }

    DC_LOG_DEBUG("sub[%s] notify >> data(%p)[%lu] >> pub[%s] ...", ID, data_p, static_cast<unsigned long>(size),
                 pub->ID);

    EventCallback_t callback = pub->priv.eventCallback;
    if (callback != nullptr) {
        EventParam_t param;
        param.event = EVENT_NOTIFY;
        param.tran = this;
        param.recv = pub;
        param.data_p = (void*)data_p;
        param.size = size;

        int ret = callback(pub, &param);

        DC_LOG_DEBUG("send done: %d", ret);
        if (ret != RES_OK)
            DC_LOG_WARN("Notify %s -> %s rejected: %d", ID, pub->ID, ret);
        retval = ret;
    } else {
        DC_LOG_WARN("pub[%s] not register callback", pub->ID);
        retval = RES_NO_CALLBACK;
    }

    return retval;
}

/**
  * @brief  Set event callback
  * @param  callback: Callback function pointer
  * @retval None
  */
void Account::SetEventCallback(EventCallback_t callback) {
    priv.eventCallback = callback;
}

/**
  * @brief  Timer callback entry function
  * @param  timer: Pointer to timer
  * @retval None
  */
void Account::TimerCallbackHandler(lv_timer_t* timer) {
    Account* instance = (Account*)(timer->user_data);
    EventCallback_t callback = instance->priv.eventCallback;
    if (callback) {
        EventParam_t param;
        param.event = EVENT_TIMER;
        param.tran = instance;
        param.recv = instance;
        param.data_p = nullptr;
        param.size = 0;

        callback(instance, &param);
    }
}

/**
  * @brief  Set timing period
  * @param  period: Timing period
  * @retval None
  */
void Account::SetTimerPeriod(uint32_t period) {
    if (priv.timer) {
        lv_timer_del(priv.timer);
        priv.timer = nullptr;
    }

    if (period == 0) {
        return;
    }

    priv.timer = lv_timer_create(TimerCallbackHandler, period, this);
}

/**
  * @brief  Set timer enable
  * @param  en: Whether to enable
  * @retval None
  */
void Account::SetTimerEnable(bool en) {
    lv_timer_t* timer = priv.timer;

    if (timer == nullptr) {
        return;
    }

    en ? lv_timer_resume(timer) : lv_timer_pause(timer);
}

/**
  * @brief  Get the number of publishers
  * @retval number of publishers
  */
size_t Account::GetPublishersSize() {
    return publishers.size();
}

/**
  * @brief  Get the number of subscribes
  * @retval number of subscribes
  */
size_t Account::GetSubscribersSize() {
    return subscribers.size();
}
