#ifndef KEYBOARD_EXT_H
#define KEYBOARD_EXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdint.h>

int keyboard_ext_init(void);
void keyboard_ext_deinit(void);
void keyboard_ext_read(lv_indev_data_t *data);
const char *keyboard_ext_get_device_path(void);

#ifdef __cplusplus
}
#endif

#endif

