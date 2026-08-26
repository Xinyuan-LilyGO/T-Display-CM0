#include <math.h>
#include <vector>
#include "Weather.hpp"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>
#include <poll.h>
#include <linux/i2c-dev.h>
#define AHT20_ADDR      0x38
#define I2C_DEV_PATH    "/dev/i2c-1"
using namespace std;

LV_IMG_DECLARE(img_app_weather);

static int i2c_fd=-1;
// 打开I2C
int aht20_init(void)
{
    i2c_fd = open(I2C_DEV_PATH, O_RDWR);
    if (i2c_fd < 0)
    {
        perror("open i2c failed");
        return -1;
    }
    if (ioctl(i2c_fd, I2C_SLAVE, AHT20_ADDR) < 0)
    {
        perror("set slave addr failed");
        close(i2c_fd);
        return -1;
    }

    // AHT20 初始化命令
    unsigned char cmd[3] = {0xBE, 0x08, 0x00};
    write(i2c_fd, cmd, 3);
    usleep(100000);
    return 0;
}

// 读取温湿度
int aht20_read(float *temp, float *humi)
{
    if(i2c_fd < 0) return -1;

    // 触发测量
    unsigned char trig[3] = {0xAC, 0x33, 0x00};
    write(i2c_fd, trig, 3);
    usleep(80000);

    unsigned char buf[6] = {0};
    if(read(i2c_fd, buf, 6) != 6)
        return -1;

    // 状态位判断
    if((buf[0] & 0x80) != 0)
        return -1;

    // 湿度计算
    uint32_t h_raw = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | (buf[3] >> 4);
    *humi = h_raw * 100.0f / 1048576.0f;

    // 温度计算
    uint32_t t_raw = ((uint32_t)(buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | buf[5];
    *temp = t_raw * 200.0f / 1048576.0f - 50.0f;

    return 0;
}

void aht20_deinit(void)
{
    if(i2c_fd >= 0)
        close(i2c_fd);
}
lv_obj_t * label_temperature;
lv_obj_t * label_humidity;

lv_timer_t * timer_weather_monitor=NULL;
char aht20_data_tmp[20]={0};
Weather::Weather(bool use_status_bar, bool use_navigation_bar):
    ESP_Brookesia_PhoneApp(
        ESP_BROOKESIA_CORE_APP_DATA_DEFAULT("Weather", &img_app_weather, true),
        ESP_BROOKESIA_PHONE_APP_DATA_DEFAULT(&img_app_weather, use_status_bar, use_navigation_bar)
    )
{
}

Weather::~Weather()
{

}
void Weather::custom_timer_cb_weather_monitor(lv_timer_t * timer) {
uint32_t * user_data = (uint32_t*)timer->user_data;

 float temperature, humidity;
 if(0 == aht20_read(&temperature, &humidity))
        {
            printf("temperature:%.2f ℃  Humidity:%.2f %%RH\n", temperature, humidity);
        }
    memset(aht20_data_tmp,0,20);
    sprintf(aht20_data_tmp, "%f deg C", temperature);
    lv_label_set_text(label_temperature, aht20_data_tmp);
    memset(aht20_data_tmp,0,20);
    sprintf(aht20_data_tmp, "%f %%", humidity);
    lv_label_set_text(label_humidity, aht20_data_tmp);
  
}
void Weather::btn_clicked_event(lv_event_t * e)
{

 }
bool Weather::run(void)
{
    lv_area_t area = getVisualArea();
    _width = area.x2 - area.x1;
    _height = area.y2 - area.y1; 
    
    
    
    static lv_coord_t col_dsc[] = {300, 200, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {80,80,80,80,80,80,80,80,80, LV_GRID_TEMPLATE_LAST};

    /*Create a container with grid*/
    lv_obj_t * cont = lv_obj_create(lv_scr_act());
    lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);
    lv_obj_set_size(cont, 550, 850);
    lv_obj_center(cont);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);

lv_obj_t * label = lv_label_create(cont);
        lv_label_set_text(label, "Temperature:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 0, 1);


 label_temperature = lv_label_create(cont);
        lv_label_set_text(label_temperature, "");
        lv_obj_center(label_temperature);
lv_obj_set_grid_cell(label_temperature, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 0, 1);


label = lv_label_create(cont);
        lv_label_set_text(label, "Humidity:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 1, 1);


 label_humidity = lv_label_create(cont);
        lv_label_set_text(label_humidity, "");
        lv_obj_center(label_humidity);
lv_obj_set_grid_cell(label_humidity, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 1, 1);
  
   
    //init aht20 i2c
       aht20_init();
static uint32_t user_data = 35; 
timer_weather_monitor = lv_timer_create(custom_timer_cb_weather_monitor, 5000, &user_data);
lv_timer_set_repeat_count(timer_weather_monitor, -1); 

    return true;
}

/**
 * @brief The function will be called when the left button of navigate bar is clicked.
 */
bool Weather::back(void)
{
  notifyCoreClosed(); 
    return true;
}

/**
 * @brief The function will be called when app should be closed.
 */
bool Weather::close(void)
{ 
    //flag_running=0;
    //pthread_join(pthread_handle_uart_rx, NULL);
    //pthread_mutex_destroy(&mutex_uart_rx);
    aht20_deinit();
    return true;
}

bool Weather::init(void)
{

    return true;
}


 bool Weather::pause()
{

 printf("weather pause before \n");
 //notifyCoreClosed();
 printf("weather pause after \n");
     //ESP_BROOKESIA_LOGD("Pause");

     /* Do some operations here if needed */

     return true;
}

bool Weather::resume()
 {
     //ESP_BROOKESIA_LOGD("Resume");
      printf("weather resume \n");
     /* Do some operations here if needed */

     return true;
 }

void Weather::keyboard_event_cb(lv_event_t *e)
{
}
