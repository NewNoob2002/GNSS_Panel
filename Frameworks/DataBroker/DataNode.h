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
#ifndef __DATA_NODE_H
#define __DATA_NODE_H

#include <array>
#include <cstddef>
#include <cstdint>

class DataBroker;
class DataTimer;

class DataNode {
public:
    /* Event type enumeration */
    typedef enum {
        EVENT_NONE = 0, /* No event */
        EVENT_PUBLISH = 1 << 0, /* Publisher posted information */
        EVENT_PULL = 1 << 1, /* Subscriber data pull request */
        EVENT_NOTIFY = 1 << 2, /* Subscribers send notifications to publishers */
        EVENT_TIMER = 1 << 3, /* Timer event */
        EVENT_ALL = 0xFFFFFFFF /* All Event */
    } EventCode_t;

    /* Error type enumeration */
    typedef enum {
        RES_OK = 0,
        RES_UNKNOWN = -1,
        RES_SIZE_MISMATCH = -2,
        RES_UNSUPPORTED_REQUEST = -3,
        RES_NO_CALLBACK = -4,
        RES_NO_DATA = -5,
        RES_NOT_FOUND = -6,
        RES_PARAM_ERROR = -7,
        RES_STOP_PROCESS = -8,
    } ResCode_t;

    /* Event parameter structure */
    typedef struct EventParam {
        EventParam(const DataNode* _tran, const DataNode* _recv, EventCode_t _event, void* _data_p, size_t _size)
            : tran(_tran)
            , recv(_recv)
            , event(_event)
            , data_p(_data_p)
            , size(_size)
        {
        }
        const DataNode* tran; /* Pointer to sender */
        const DataNode* recv; /* Pointer to receiver */
        EventCode_t event; /* Event type */
        void* data_p; /* Pointer to data */
        size_t size; /* The size of the data */
    } EventParam_t;

    /* Event callback function pointer */
    typedef int (*EventCallback_t)(DataNode* node, EventParam_t* param);

    using DataNodeList_t = std::array<DataNode*, 17>;

  public:
    /**
     * @brief  DataNode constructor
     * @param  id:       Unique name
     * @param  broker:   Pointer to the data broker
     * @param  userData: Point to the address of user-defined data
     */
    DataNode(
        const char* id,
        DataBroker* broker,
        void* userData = nullptr);

    /**
     * @brief  DataNode destructor
     */
    virtual ~DataNode();
    DataNode(const DataNode&) = delete;
    DataNode& operator=(const DataNode&) = delete;

    /** 查询注册是否成功；重复名称、空名称或超过 16 个节点时失败。 */
    bool
    isRegistered() const {
        return _registered;
    }

    /**
     * @brief  Subscribe to Publisher
     * @param  pubID: Publisher _ID
     * @retval Pointer to publisher
     */
    const DataNode* subscribe(const char* pubID);

    /**
     * @brief  Unsubscribe from publisher
     * @param  pubID: Publisher _ID
     * @retval Return true if unsubscribe is successful
     */
    bool unsubscribe(const char* pubID);

    /**
     * @brief  Get the number of publishers
     * @retval number of publishers
     */
    size_t getPublishersNumber();

    /**
     * @brief  Get the number of subscribes
     * @retval number of subscribes
     */
    size_t getSubscribersNumber();

    /**
     * @brief  同步发布调用方持有的数据，回调返回后不保存参数。
     * @note   回调可删除订阅者，禁止销毁当前发布者或其 broker。
     * @param  data_p: Pointer to data
     * @param  size:   The size of the data
     * @retval error code
     */
    int publish(const void* data_p, size_t size);

    /**
     * @brief  Pull data from the publisher
     * @param  pubID:  Publisher _ID
     * @param  data_p: Pointer to data
     * @param  size:   The size of the data
     * @retval error code
     */
    int pull(const char* pubID, void* data_p, size_t size);

    /**
     * @brief  Send notification to the publisher
     * @param  pub:    Publisher node pointer
     * @param  data_p: Pointer to data
     * @param  size:   The size of the data
     * @retval error code
     */
    int pull(const DataNode* pub, void* data_p, size_t size);

    /**
     * @brief  Send a notification to the publisher
     * @param  pubID: Publisher _ID
     * @param  data_p: Pointer to data
     * @param  size:   The size of the data
     * @retval error code
     */
    int notify(const char* pubID, const void* data_p, size_t size);

    /**
     * @brief  Send a notification to the publisher
     * @param  pub:    Pointer to publisher
     * @param  data_p: Pointer to data
     * @param  size:   The size of the data
     * @retval error code
     */
    int notify(const DataNode* pub, const void* data_p, size_t size);

    /**
     * @brief  Set event callback
     * @param  callback: Callback function pointer
     * @param  eventFilter: Event filter mask
     */
    void setEventCallback(EventCallback_t callback, uint32_t eventFilter = EVENT_ALL);

    /**
     * @brief  Set event filter
     * @param  eventFilter: Event filter mask
     */
    void setEventFilter(uint32_t eventFilter);

    /**
     * @brief  Event handler
     * @param  param: Event parameter structure
     * @retval error code
     */
    virtual int onEvent(EventParam_t* param);

    /**
     * @brief  Start timer
     * @param  period: Timer period
     */
    void startTimer(uint32_t period);

    /**
     * @brief  Change timer period
     * @param  period: Timer period
     */
    void setTimerPeriod(uint32_t period);

    /**
     * @brief  Reset timer
     */
    void resetTimer();

    /**
     * @brief  Stop timer
     */
    void stopTimer();

    /**
     * @brief  Set user-defined data
     * @param  userData: Point to the address of user-defined data
     */
    void setUserData(void* userData)
    {
        _userData = userData;
    }

    /**
     * @brief  Get user-defined data
     * @retval Point to the address of user-defined data
     */
    void* getUserData()
    {
        return _userData;
    }

    /**
     * @brief  Get node ID
     * @retval Node ID
     */
    const char* getID() const
    {
        return _ID;
    }

private:
  // 禁用异常的固件使用定长关系表，地址加序号校验发布期间的节点寿命。
  friend class DataBroker;
  bool _registered = false;
  uint64_t _serial = 0;
  DataNodeList_t _publishers{};  /* Followed publishers */
  DataNodeList_t _subscribers{}; /* Followed subscribers */

  const char* _ID;     /* Node ID */
  DataBroker* _broker; /* Pointer to the data broker */
  void* _userData;

  EventCallback_t _eventCallback;
  uint32_t _eventFilterMask;
  DataTimer* _timer;

private:
    /**
     * @brief  Timer callback entry function
     * @param  timer: Pointer to timer
     */
    static void timerCallbackHandler(DataTimer* timer);

    /**
     * @brief  Send event to the receiver
     * @param  node:  Receiver node pointer
     * @param  param: Event parameter structure
     * @retval error code
     */
    static int sendEvent(DataNode* node, EventParam_t* param);
};

#endif
