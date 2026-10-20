#ifndef __SYSTEM_INFOS_MODEL_H
#define __SYSTEM_INFOS_MODEL_H

#include "Common/DataProc/DataProc.h"

namespace Page
{

class SystemInfosModel
{
public:
    void Init();
    void Deinit();

    void SetStatusBarStyle(DataProc::StatusBar_Style_t style) const;

private:
    Account *account = nullptr;

};

}

#endif
