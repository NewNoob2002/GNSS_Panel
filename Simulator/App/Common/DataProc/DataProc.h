#pragma once

#include <cstring>
#include <type_traits>
#include "DataProc_Def.h"
#include "Utils/DataCenter/DataCenter.h"

#define DATA_PROC_INIT_DEF(name) bool _DP_##name##_Init(Account* account)

/** Initialize service Accounts after lv_init(), before page Models; only one app instance.
 * Failure rolls back partially initialized providers. No HAL/OS/power operations are performed.
 */
bool DataProc_Init();
/** Call after all page/component Accounts have been destroyed. */
bool DataProc_Deinit();

namespace DataProc {
/** Shared center accessor only; routing is implemented by Utils/DataCenter. */
DataCenter* Center();

/** Validate native payload size and copy, avoiding alignment/aliasing assumptions. */
template <class T> int ReadPayload(const Account::EventParam_t* event, T& value) {
    static_assert(std::is_trivially_copyable<T>::value, "Account payload must be trivially copyable");
    if (!event || !event->data_p)
        return Account::RES_PARAM_ERROR;
    if (event->size != sizeof(T))
        return Account::RES_SIZE_MISMATCH;
    std::memcpy(&value, event->data_p, sizeof(T));
    return Account::RES_OK;
}
} // namespace DataProc
