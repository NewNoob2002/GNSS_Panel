#include <cassert>
#include "AppLogDisabled.h"

int
main() {
    unsigned evaluated = 0;
    APP_LOG_D("GNSS", "%u", ++evaluated);
    APP_LOG_I("GNSS", "%u", ++evaluated);
    APP_LOG_W("GNSS", "%u", ++evaluated);
    APP_LOG_E("GNSS", "%u", ++evaluated);
    assert(evaluated == 0);
    assert(app_log_get_level() == APP_LOG_OFF);
    assert(app_log_set_level(APP_LOG_INFO));
    assert(!app_log_set_level(static_cast<AppLogLevel>(99)));
    app_log_set_sink(nullptr, nullptr);
    assert(app_log_write(APP_LOG_ERROR, "GNSS", "ignored") == 0);
}
