/**
 * @file lv_port_indev_templ.c
 *
 */

/*Copy this file as "lv_port_indev.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include "sys/ioctl.h"
#include <unistd.h>
#include <linux/input.h>
#include <dirent.h>
#include "../keyboard_ext.h"




/*********************
 *      DEFINES
 *********************/
static int touch_fd = -1;
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void touchpad_init(void);
static void touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
static bool touchpad_is_pressed(void);
static void touchpad_get_xy(lv_coord_t * x, lv_coord_t * y);


static void keypad_init(void);
static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
static uint32_t keypad_get_key(void);
/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t * indev_keypad;
/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    /**
     * Here you will find example implementation of input devices supported by LittelvGL:
     *  - Touchpad
     *  - Mouse (with cursor support)
     *  - Keypad (supports GUI usage only with key)
     *  - Encoder (supports GUI usage only with: left, right, push)
     *  - Button (external buttons to press points on the screen)
     *
     *  The `..._read()` function are only examples.
     *  You should shape them according to your hardware
     */

    static lv_indev_drv_t indev_drv;
    static lv_indev_drv_t indev_drv2;
     /*------------------
     * Keypad
     * -----------------*/

    /*Initialize your keypad or keyboard if you have*/
    keypad_init();
    
    /*Register a keypad input device*/
    lv_indev_drv_init(&indev_drv2);
    indev_drv2.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv2.read_cb = keypad_read;
    indev_keypad = lv_indev_drv_register(&indev_drv2);
    
    /*------------------
     * Touchpad
     * -----------------*/

    /*Initialize your touchpad if you have*/
    touchpad_init();

    /*Register a touchpad input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);
    
    
    
     lv_group_t * group = lv_group_create();
     //lv_group_add_obj(group, lv_scr_act()); 
     lv_group_set_default(group);	
     lv_indev_set_group(indev_keypad, group);     
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
// 自动查找触摸设备
static int find_touch_device(void) {
    DIR *dir = opendir("/dev/input");
    if (!dir) return -1;

    struct dirent *entry;
    char path[256];
    char name[256];

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "event", 5) != 0) continue;

        snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) > 0) {
            // 匹配常见的触摸设备名称
            if (strstr(name, "Touch") || strstr(name, "touch") || 
                strstr(name, "FT5406") || strstr(name, "ADS7846") ||
                strstr(name, "XPT2046") || strstr(name, "Goodix")) {
                printf("找到触摸设备: %s (%s)\n", path, name);
                closedir(dir);
                return fd;
            }
        }

        close(fd);
    }

    closedir(dir);
    return -1;
}

/*------------------
 * Touchpad
 * -----------------*/

/*Initialize your touchpad*/
static void touchpad_init(void)
{
    /*Your code comes here*/
    // 自动查找触摸设备
    touch_fd = find_touch_device();
    if (touch_fd < 0) {
        fprintf(stderr, "警告: 未找到触摸设备，触摸功能将不可用\n");
        return;
    }
}

/*Will be called by the library to read the touchpad*/
static void touchpad_read(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
    static lv_coord_t last_x = 0;
    static lv_coord_t last_y = 0;
    static bool pressed = false;
    const int SCREEN_WIDTH  = 568;
    const int SCREEN_HEIGHT = 1232;
    const int TOUCH_X_MAX = 1024;  // 上面命令读出的ABS_X最大值
    const int TOUCH_Y_MAX = 2400;  // 上面命令读出的ABS_Y最大值
    struct input_event ev;
    while (read(touch_fd, &ev, sizeof(ev)) == sizeof(ev)) {
        if (ev.type == EV_ABS) {
            if (ev.code == ABS_X) {
                last_x = ev.value;
            } else if (ev.code == ABS_Y) {
                last_y = ev.value;
            }
        } else if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
            pressed = ev.value != 0;
        }
    }
     // 1. 把触摸原始坐标缩放到屏幕像素坐标
    int screen_x = (last_x * SCREEN_WIDTH) / TOUCH_X_MAX;
    int screen_y = (last_y * SCREEN_HEIGHT) / TOUCH_Y_MAX;

    // 2. 裁剪到屏幕有效范围内（防止越界）
    if (screen_x < 0) screen_x = 0;
    if (screen_x >= SCREEN_WIDTH) screen_x = SCREEN_WIDTH - 1;
    if (screen_y < 0) screen_y = 0;
    if (screen_y >= SCREEN_HEIGHT) screen_y = SCREEN_HEIGHT - 1;

    data->state = pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    data->point.x = screen_x;
    data->point.y = screen_y;
   //if (pressed) {
   // printf("📌 触摸中 | X = %d, Y = %d\n", last_x, last_y);
   // }
}
void lv_port_indev_deinit(void) {
    if (touch_fd >= 0) close(touch_fd);
}


#if 1  

/*------------------
 * Keypad
 * -----------------*/

/*Initialize your keypad*/
static void keypad_init(void)
{

    if(keyboard_ext_init() < 0) {
        return;
    }
    /*Your code comes here*/
    //tca8418_init(); 
    //control_keyboard_backlight(20000,5);
}

/*Will be called by the library to read the mouse*/
static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static uint32_t last_key = 0;

    /*Get the current x and y coordinates*/
    //mouse_get_xy(&data->point.x, &data->point.y);
     //touchpad_get_xy(&data->point.x, &data->point.y);
    /*Get whether the a key is pressed and save the pressed key*/
   // data->key = last_key;
    //(void)indev_drv;
    keyboard_ext_read(data);  
}
#endif
#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
