#ifndef LV_PORT_H
#define LV_PORT_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif
void lv_port_init();

void lv_port_disp_init();
void lv_port_disp_poll();

void lv_port_reset();
#ifdef __cplusplus
}
#endif

#endif