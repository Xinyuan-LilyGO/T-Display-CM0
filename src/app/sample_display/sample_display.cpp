#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <stdbool.h>
#include "lvgl/lvgl.h"
#include "port/lv_port_disp.h"
#include "port/lv_port_indev.h"
#include "lvgl/src/hal/lv_hal_tick.h"
#include "esp_brookesia.hpp"
#include <math.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>
#include "vt_graphic.h"
/* These are built-in app examples in `esp-brookesia` library */
#include "app_examples/phone/simple_conf/src/phone_app_simple_conf.hpp"
#include "app_examples/phone/complex_conf/src/phone_app_complex_conf.hpp"
#include "app_examples/phone/squareline/src/phone_app_squareline.hpp"
#include "app_examples/phone/calculator/Calculator.hpp"
#include "app_examples/phone/game_2048/Game_2048.hpp"
#include "app_examples/phone/weather/Weather.hpp"
#include "app_examples/phone/lora/Lora.hpp"
#include "app_examples/phone/music/Music.hpp"
#include "app_examples/phone/camera/Camera.hpp"
#include "app_examples/phone/battery/Battery.hpp"
#include "app_examples/phone/keyboard/KeyBoard.hpp"
#include "app_examples/phone/gnss/Gnss.hpp"
#include "app_examples/phone/cellular/Cellular.hpp"
#include "app_examples/phone/imu/Imu.hpp"
#define USING_FAN
#define USING_WEATHER
#define USING_BATTERY
#define USING_KEYBOARD
#define USING_LTE_GNSS
static  int gpio_chip0_fd;
struct gpio_v2_line_request gpio_req_fan_ctl;
struct gpio_v2_line_values line_values;

struct gpio_v2_line_request gpio_req_nrf9151_ctl;
float get_cpu_temperature(void)
{
    FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (fp == NULL) {
        return -1.0f;
    }

    int temp_mill;
    fscanf(fp, "%d", &temp_mill);
    fclose(fp);

    // 转摄氏度
    return temp_mill / 1000.0f;
}
#if defined(USING_BATTERY)  
static int read_power_supply_bq25896(const char *prop, long *value) {
    char path[256];
    snprintf(path, sizeof(path),
             "/sys/class/power_supply/bq25890-charger-0/%s", prop);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    fscanf(f, "%ld", value);
    fclose(f);
    return 0;
}
static int read_power_supply_bq27220(const char *prop, long *value) {
    char path[256];
    snprintf(path, sizeof(path),
             "/sys/class/power_supply/bq27220-0/%s", prop);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    fscanf(f, "%ld", value);
    fclose(f);
    return 0;
}
#endif














#if 1
static void on_clock_update_timer_cb(struct _lv_timer_t *t)
{
    time_t now;
    struct tm timeinfo;
    bool is_time_pm = false;
    ESP_Brookesia_Phone *phone = (ESP_Brookesia_Phone *)t->user_data;

    time(&now);
    //printf("当前系统时间戳：%lld（应与at_ts一致)\n", (long long)now);
    localtime_r(&now, &timeinfo);
    is_time_pm = (timeinfo.tm_hour >= 12);

    /* Since this callback is called from LVGL task, it is safe to operate LVGL */
    // Update clock on "Status Bar"
    ESP_BROOKESIA_CHECK_FALSE_EXIT(
        phone->getHome().getStatusBar()->setClock(timeinfo.tm_hour, timeinfo.tm_min, is_time_pm),
        "Refresh status bar failed"
    );
   #if defined(USING_BATTERY)
    long bq27220_soc;  
    if (read_power_supply_bq27220("capacity", &bq27220_soc) == 0)
    {
      ESP_BROOKESIA_CHECK_FALSE_EXIT(
        phone->getHome().getStatusBar()->setBatteryPercent(false,bq27220_soc),
        "Refresh status bar failed"
    ); //true
    }  
     
    #endif
    //temperature control fan
    #if defined(USING_FAN)
    //read temperature if temperature>60 open fan else close fan
     float cpu_temp = get_cpu_temperature();
     //printf("cpu_temp:%.2f\n",cpu_temp);
     if(gpio_chip0_fd>0)
     {
     
    if(cpu_temp>50)//50
      {
        memset(&line_values, 0, sizeof(line_values));
        line_values.mask = 1;          // 操作第0条线（对应 offsets[0]）
        line_values.bits = 1;        // 电平值
        ioctl(gpio_req_fan_ctl.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &line_values);
      }
    else if(cpu_temp<40)
      {
        memset(&line_values, 0, sizeof(line_values));
        line_values.mask = 1;          // 操作第0条线（对应 offsets[0]）
        line_values.bits = 0;        // 电平值
        ioctl(gpio_req_fan_ctl.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &line_values);
      }
    }
    #endif 
}
#endif
void user_gui_init()
{
 printf("user_gui_init\n");
#if 1
    lv_disp_t*disp=lv_disp_get_default();
 /* Create a phone object */
    ESP_Brookesia_Phone *phone = new ESP_Brookesia_Phone(disp);
    ESP_BROOKESIA_CHECK_NULL_EXIT(phone, "Create phone failed");

    /* Try using a stylesheet that corresponds to the resolution */
    ESP_Brookesia_PhoneStylesheet_t *stylesheet;
   if(SCREEN_ORIENTATATION == 1)
    { 
    stylesheet = new ESP_Brookesia_PhoneStylesheet_t ESP_BROOKESIA_PHONE_1232_568_DARK_STYLESHEET();
    }
    else
    {
     stylesheet = new ESP_Brookesia_PhoneStylesheet_t ESP_BROOKESIA_PHONE_568_1232_DARK_STYLESHEET();
    }
        ESP_BROOKESIA_CHECK_NULL_EXIT(stylesheet, "Create stylesheet failed");
        ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->addStylesheet(stylesheet), "Add stylesheet failed");
        ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->activateStylesheet(stylesheet), "Activate stylesheet failed");
        delete stylesheet;

   lv_indev_t * indev_touch= lv_indev_get_next(NULL);
    /* Configure and begin the phone */
    ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->setTouchDevice(indev_touch), "Set touch device failed");
    
    ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->begin(), "Begin failed");

    /* Install apps */
 
    PhoneAppSquareline *app_squareline = new PhoneAppSquareline();
    ESP_BROOKESIA_CHECK_NULL_EXIT(app_squareline, "Create app squareline failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_squareline) >= 0), "Install app squareline failed");
    Calculator *app_calculator = new Calculator();
    ESP_BROOKESIA_CHECK_NULL_EXIT(app_calculator, "Create app calculator failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_calculator) >= 0), "Install app calculator failed");
    Game2048 *app_game2048 = new Game2048();
    ESP_BROOKESIA_CHECK_NULL_EXIT(app_game2048, "Create app game2048 failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_game2048) >= 0), "Install app game2048 failed");
 
 #if defined(USING_WEATHER)    
    Weather *app_weather = new Weather();
    ESP_BROOKESIA_CHECK_NULL_EXIT(app_weather, "Create app weather failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_weather) >= 0), "Install app weather failed");
    #endif
    
    
    Lora *app_lora = new Lora();
    ESP_BROOKESIA_CHECK_NULL_EXIT(app_lora, "Create app lora failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_lora) >= 0), "Install app lora failed");
    
    #if defined(USING_KEYBOARD)    
    KeyBoard *app_keyboard = new KeyBoard();
    ESP_BROOKESIA_CHECK_NULL_EXIT(app_keyboard, "Create app keyboard failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_keyboard) >= 0), "Install app keyboard failed");
    #endif
 
    Music *app_music = new Music();
    ESP_BROOKESIA_CHECK_NULL_EXIT(app_music, "Create app music failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_music) >= 0), "Install app music failed");
    Camera *app_camera = new Camera();
    ESP_BROOKESIA_CHECK_NULL_EXIT(app_camera, "Create app camera failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_camera) >= 0), "Install app camera failed");
    #if defined(USING_BATTERY)    
    Battery *app_battery = new Battery();
    ESP_BROOKESIA_CHECK_NULL_EXIT(app_battery, "Create app battery failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_battery) >= 0), "Install app battery failed");
    #endif
    #if defined(USING_LTE_GNSS)
    Gnss *app_gnss = new Gnss();
    ESP_BROOKESIA_CHECK_NULL_EXIT(app_gnss, "Create app gnss failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_gnss) >= 0), "Install app gnss failed");

    Cellular *app_cellular = new Cellular();
    ESP_BROOKESIA_CHECK_NULL_EXIT(app_cellular, "Create app cellular failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_cellular) >= 0), "Install app cellular failed");
   #endif
   
    Imu *app_imu = new Imu();
    ESP_BROOKESIA_CHECK_NULL_EXIT(app_imu, "Create app imu failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_imu) >= 0), "Install app imu failed");

   
   
   
   
#if 0
    AppSettings *app_setting = new AppSettings();
    ESP_BROOKESIA_CHECK_NULL_EXIT(app_setting, "Create app setting failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_setting) >= 0), "Install app setting failed");
   
    Clock *app_clock = new Clock();
    ESP_BROOKESIA_CHECK_NULL_EXIT(app_clock, "Create app clock failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_clock) >= 0), "Install app clock failed");

    Artboard *app_artboard = new Artboard();
    ESP_BROOKESIA_CHECK_NULL_EXIT(app_artboard, "Create app artboard failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_artboard) >= 0), "Install app artboard failed");
#endif


 // 1. 打开GPIO芯片（原生 = gpiochip0）
     // -------------------------- 1. 打开 GPIO 芯片 --------------------------
    gpio_chip0_fd = open("/dev/gpiochip0", O_RDWR);
    #if defined(USING_FAN)
   if (gpio_chip0_fd>0)
    {
        memset(&gpio_req_fan_ctl, 0, sizeof(gpio_req_fan_ctl));
    	gpio_req_fan_ctl.offsets[0]              = 17;
    	gpio_req_fan_ctl.num_lines               = 1;
    	gpio_req_fan_ctl.config.flags            = GPIO_V2_LINE_FLAG_OUTPUT;  // 输出模式
    	strncpy(gpio_req_fan_ctl.consumer, "fan_ctl", sizeof(gpio_req_fan_ctl.consumer) - 1);
        ioctl(gpio_chip0_fd, GPIO_V2_GET_LINE_IOCTL, &gpio_req_fan_ctl);  
        memset(&line_values, 0, sizeof(line_values));
        line_values.mask = 1;          // 操作第0条线（对应 offsets[0]）
        line_values.bits = 0;        // 电平值
        ioctl(gpio_req_fan_ctl.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &line_values);
    }  
   #endif
   
   #if 1
   if (gpio_chip0_fd>0)//nrF9151_en
    {
        memset(&gpio_req_nrf9151_ctl, 0, sizeof(gpio_req_nrf9151_ctl));
    	gpio_req_nrf9151_ctl.offsets[0]              = 12;//gpio
    	gpio_req_nrf9151_ctl.num_lines               = 1;
    	gpio_req_nrf9151_ctl.config.flags            = GPIO_V2_LINE_FLAG_OUTPUT;  // 输出模式
    	strncpy(gpio_req_nrf9151_ctl.consumer, "nrf9151_ctl", sizeof(gpio_req_nrf9151_ctl.consumer) - 1);
        ioctl(gpio_chip0_fd, GPIO_V2_GET_LINE_IOCTL, &gpio_req_nrf9151_ctl);  
        memset(&line_values, 0, sizeof(line_values));
        line_values.mask = 1;          // 操作第0条线（对应 offsets[0]）
        line_values.bits = 1;        // 电平值
        ioctl(gpio_req_nrf9151_ctl.fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &line_values);
    }  
   #endif
   
   
   
   
   
   
   
   //control
   // gpio_chip1_fd = open("/dev/gpiochip0", O_RDWR);
   
   
   
   
   
   
   
   
    /* Create a timer to update the clock */
    ESP_BROOKESIA_CHECK_NULL_EXIT(lv_timer_create(on_clock_update_timer_cb, 1000, phone), "Create clock update timer failed");

#endif
    
    printf("user_gui_init after\n");
}




//
int main(int argc, char **argv)
{
    printf("hello world ,lvgl version:%d.%d.%d\n",lv_version_major(),lv_version_minor(),lv_version_patch());
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    vt_graphics_init();
    user_gui_init();

   

         
          

  

    #if 0
    
    lv_obj_t * label;

    lv_obj_t * btn1 = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -40);
    lv_obj_remove_flag(btn1, LV_OBJ_FLAG_PRESS_LOCK);

    label = lv_label_create(btn1);
    lv_label_set_text(label, "Button");
    lv_obj_center(label);

    lv_obj_t * btn2 = lv_button_create(lv_screen_active());
    lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_height(btn2, LV_SIZE_CONTENT);

    label = lv_label_create(btn2);
    lv_label_set_text(label, "Toggle");
    lv_obj_center(label);   
    #endif

#if 0
    // 1. 获取当前活动屏幕
lv_obj_t *scr = lv_scr_act();
// 2. 获取宽、高
lv_coord_t scr_w = lv_obj_get_width(scr);
lv_coord_t scr_h = lv_obj_get_height(scr);

// 3. 打印输出（lv_printf / printf 都可以）
printf("屏幕宽度 = %d px\n", (int)scr_w);
printf("屏幕高度 = %d px\n", (int)scr_h);
// 1. 创建一个按钮对象
    lv_obj_t *btn = lv_btn_create(lv_scr_act());

    // 2. 设置按钮位置：屏幕左上角 (x=0, y=0)
    lv_obj_set_pos(btn, 50, 50);

    // 3. 设置按钮大小（可自己改）
    lv_obj_set_size(btn, 800, 400);

    // 4. 设置按钮背景色 = 纯红色
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF0000), 0);

    // 可选：给按钮加文字（方便看效果）
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "RED BTN");
    lv_obj_center(label); // 文字居中
#endif
while(1)
    {
    lv_tick_inc(1);
    lv_task_handler();
    usleep(1000);//500
    }
	
    return 0;
}



















