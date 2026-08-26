#include <math.h>
#include <vector>
#include "Battery.hpp"
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
//#include "../../../../../fpioa/rt_fpioa.h"
using namespace std;

LV_IMG_DECLARE(img_app_battery);
#define KEYBOARD_H_PERCENT      65
#define KEYBOARD_FONT           &lv_font_montserrat_48
#define KEYBOARD_SPECIAL_COLOR  lv_color_make(0, 0x99, 0xff)

#define LABEL_PAD               5
#define LABEL_FONT_SMALL        &lv_font_montserrat_34
#define LABEL_FONT_BIG          &lv_font_montserrat_48
#define LABEL_COLOR             lv_color_make(170, 170, 170)
#define LABEL_FORMULA_LEN_MAX   256

#define USING_BQ25896
#define USING_BQ27220

lv_obj_t * label_bq25896_vbus;
lv_obj_t * label_bq25896_vsys;
lv_obj_t * label_bq25896_vbat;
lv_obj_t * label_bq25896_charge_current;
lv_obj_t * label_bq25896_charge_status;

lv_obj_t * label_bq25896_precharge_currentl_limit;
lv_obj_t * label_bq25896_fast_charge_currentl_limit;
lv_obj_t * label_bq25896_terminate_current_limit;
lv_obj_t * label_bq25896_charge_voltage_limit;



lv_obj_t * label_bq27220_full_charge_capacity;
lv_obj_t * label_bq27220_design_capacity;
lv_obj_t * label_bq27220_voltage;
lv_obj_t * label_bq27220_current;


lv_obj_t * label_bq27220_temperature_c;
lv_obj_t * label_bq27220_remaining_capacity;
lv_obj_t * label_bq27220_soc;
lv_obj_t * label_bq27220_soh;
lv_obj_t * label_bq27220_bat_state;


static int read_power_supply_bq25896_str(const char *prop, char *buf, size_t buf_size) {
    char path[256];
    snprintf(path, sizeof(path),
             "/sys/class/power_supply/bq25890-charger-0/%s", prop);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    if (!fgets(buf, buf_size, f)) {
        fclose(f);
        return -1;
    }
    // 去掉末尾换行符
    buf[strcspn(buf, "\n")] = '\0';
    fclose(f);
    return 0;
}

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
static int read_power_supply_bq27220_str(const char *prop, char *buf, size_t buf_size) {
    char path[256];
    snprintf(path, sizeof(path),
             "/sys/class/power_supply/bq27220-0/%s", prop);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    if (!fgets(buf, buf_size, f)) {
        fclose(f);
        return -1;
    }
    // 去掉末尾换行符
    buf[strcspn(buf, "\n")] = '\0';
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




lv_timer_t * timer_battery_capacity_monitor=NULL;
lv_timer_t * timer_battery_charge_monitor=NULL;
char bq25896_data[20]={0};
char bq27220_data_s[20]={0};
char status[64];
Battery::Battery(bool use_status_bar, bool use_navigation_bar):
    ESP_Brookesia_PhoneApp(
        ESP_BROOKESIA_CORE_APP_DATA_DEFAULT("Battery", &img_app_battery, true),
        ESP_BROOKESIA_PHONE_APP_DATA_DEFAULT(&img_app_battery, use_status_bar, use_navigation_bar)
    )
{
}

Battery::~Battery()
{

}
void Battery::custom_timer_cb_battery_charge_monitor(lv_timer_t * timer) {
uint32_t * user_data = (uint32_t*)timer->user_data;
#if defined(USING_BQ25896)
 long value;


// VBAT (µV → mV)
if (read_power_supply_bq25896("voltage_now", &value) == 0)
 {
memset(bq25896_data,0,20);
sprintf(bq25896_data, "%ldmV", value / 1000); 
lv_label_set_text(label_bq25896_vbat, bq25896_data);
}

 // 充电电流 (µA → mA)
    if (read_power_supply_bq25896("current_now", &value) == 0)
    {
    memset(bq25896_data,0,20);
    sprintf(bq25896_data, "%ldmA",  value / 1000); 
    lv_label_set_text(label_bq25896_charge_current, bq25896_data);
    }

    // 预充电流限制 (µA → mA)
    if (read_power_supply_bq25896("precharge_current", &value) == 0)
    {  
    //# Precharge Current Limit   Default: 128mA (0001)
    memset(bq25896_data,0,20);
    sprintf(bq25896_data, "%ldmA", value / 1000); 
    lv_label_set_text(label_bq25896_precharge_currentl_limit, bq25896_data);  
    }

    // 快充电流限制 (µA → mA)
    if (read_power_supply_bq25896("constant_charge_current_max", &value) == 0)
    { 
    memset(bq25896_data,0,20);
    sprintf(bq25896_data, "%ldmA", value / 1000); 
    lv_label_set_text(label_bq25896_fast_charge_currentl_limit, bq25896_data);  
    }
    // 终止电流限制 (µA → mA)
    if (read_power_supply_bq25896("charge_term_current", &value) == 0)
    {
      //Termination Current Limit   Default: 256mA (0011)
    memset(bq25896_data,0,20);
    sprintf(bq25896_data, "%ldmA", value / 1000); 
    lv_label_set_text(label_bq25896_terminate_current_limit, bq25896_data);  
    }

    // 充电电压上限 (µV → mV)
    if (read_power_supply_bq25896("constant_charge_voltage_max", &value) == 0)
    {
    // Charge Voltage Limit   Default: 4.208V (010111)
    memset(bq25896_data,0,20);
    sprintf(bq25896_data, "%ldmV", value / 1000); 
    lv_label_set_text(label_bq25896_charge_voltage_limit, bq25896_data);   
    }
     memset(status,0,64);
  if (read_power_supply_bq25896_str("status", status, sizeof(status)) == 0)
  {
      //printf("charge_status: %s\n", status);
  	memset(bq25896_data,0,20);
  	sprintf(bq25896_data, status);   
    lv_label_set_text(label_bq25896_charge_status, bq25896_data);
  }
    /*bq25896_read_reg(fd_i2c0, 0x0F,&reg_data,1);
    reg_data=reg_data&(~(1<<7));
    //printf("sys voltage:%dmV\n", reg_data*20+2304);
    memset(bq25896_data,0,20);
    sprintf(bq25896_data, "%dmV", reg_data*20+2304);
    lv_label_set_text(label_bq25896_vsys, bq25896_data);
    

    
    
    
    bq25896_read_reg(fd_i2c0, 0x11,&reg_data,1);
    reg_data=reg_data&(~(1<<7));
    //printf("vbus:%dmV\n", reg_data*100+2600);
     memset(bq25896_data,0,20);
    sprintf(bq25896_data, "%dmV", reg_data*100+2600);
    lv_label_set_text(label_bq25896_vbus, bq25896_data);
    */ 

#endif




}

void Battery::custom_timer_cb_battery_capacity_monitor(lv_timer_t * timer) {
uint32_t * user_data = (uint32_t*)timer->user_data;
#if defined(USING_BQ27220)
 long value;
if (read_power_supply_bq27220("charge_full", &value) == 0)
{
    memset(bq27220_data_s,0,20);
    sprintf(bq27220_data_s, "%ldmAh", value / 1000); 
    lv_label_set_text(label_bq27220_full_charge_capacity, bq27220_data_s);
}

    if (read_power_supply_bq27220("charge_full_design", &value) == 0)
    {
    memset(bq27220_data_s,0,20);
    sprintf(bq27220_data_s, "%ldmAh", value / 1000); 
    lv_label_set_text(label_bq27220_design_capacity, bq27220_data_s); 
    }
 

    if (read_power_supply_bq27220("voltage_now", &value) == 0)
    {
     memset(bq27220_data_s,0,20);
    sprintf(bq27220_data_s, "%ldmV", value / 1000); 
    lv_label_set_text(label_bq27220_voltage, bq27220_data_s); 
    
    }

    if (read_power_supply_bq27220("current_now", &value) == 0)
    {
    memset(bq27220_data_s,0,20);
    sprintf(bq27220_data_s, "%ldmA", value / 1000); 
    lv_label_set_text(label_bq27220_current, bq27220_data_s);  
    }
        

    if (read_power_supply_bq27220("temp", &value) == 0)
    {
    memset(bq27220_data_s,0,20);
    sprintf(bq27220_data_s, "%.2fC", value/10.0); 
    lv_label_set_text(label_bq27220_temperature_c, bq27220_data_s); 
 
    }

    if (read_power_supply_bq27220("charge_now", &value) == 0)
    {
       memset(bq27220_data_s,0,20);
    sprintf(bq27220_data_s, "%ldmAh", value / 1000); 
    lv_label_set_text(label_bq27220_remaining_capacity, bq27220_data_s);

    }
    if (read_power_supply_bq27220("capacity", &value) == 0)
    {
      memset(bq27220_data_s,0,20);
    sprintf(bq27220_data_s, "%ld%%", value); 
    lv_label_set_text(label_bq27220_soc, bq27220_data_s);    
    }
    memset(status,0,64);
if (read_power_supply_bq27220_str("health", status, sizeof(status)) == 0)
  { 
     memset(bq27220_data_s,0,20);
    sprintf(bq27220_data_s, "%s", status); 
    lv_label_set_text(label_bq27220_soh, bq27220_data_s); 
  }
memset(status,0,64);
 if (read_power_supply_bq27220_str("status", status, sizeof(status)) == 0)
  { 
      memset(bq27220_data_s,0,20);
    sprintf(bq27220_data_s, "%s", status); 
    lv_label_set_text(label_bq27220_bat_state, bq27220_data_s); 
  }   
   
#endif
}




void Battery::btn_clicked_event(lv_event_t * e)
{

 }
bool Battery::run(void)
{
    lv_area_t area = getVisualArea();
    _width = area.x2 - area.x1;
    _height = area.y2 - area.y1; 
    
     /*Create a Tab view object*/
    lv_obj_t * tabview;
    tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 80);

    /*Add 3 tabs (the tabs are page (lv_page) and can be scrolled*/
    lv_obj_t * tab_charge = lv_tabview_add_tab(tabview, "Charge");
    lv_obj_t * tab_coulometer = lv_tabview_add_tab(tabview, "Coulometer");
    lv_obj_t * label;
    static lv_coord_t col_dsc[] = {300, 200, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {80,80,80,80,80,80,80,80,80, LV_GRID_TEMPLATE_LAST};
#if defined(USING_BQ25896)
    /*Create a container with grid*/
    lv_obj_t * cont = lv_obj_create(tab_charge);
    lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);
    lv_obj_set_size(cont, 550, 850);
    lv_obj_center(cont);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);

 label = lv_label_create(cont);
        lv_label_set_text(label, "VBUS:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 0, 1);


 label_bq25896_vbus = lv_label_create(cont);
        lv_label_set_text(label_bq25896_vbus, "");
        lv_obj_center(label_bq25896_vbus);
lv_obj_set_grid_cell(label_bq25896_vbus, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 0, 1);


label = lv_label_create(cont);
        lv_label_set_text(label, "VSYS:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 1, 1);


 label_bq25896_vsys = lv_label_create(cont);
        lv_label_set_text(label_bq25896_vsys, "");
        lv_obj_center(label_bq25896_vsys);
lv_obj_set_grid_cell(label_bq25896_vsys, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 1, 1);
  
  
label = lv_label_create(cont);
        lv_label_set_text(label, "VBAT:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 2, 1);


 label_bq25896_vbat = lv_label_create(cont);
        lv_label_set_text(label_bq25896_vbat, "");
        lv_obj_center(label_bq25896_vbat);
lv_obj_set_grid_cell(label_bq25896_vbat, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 2, 1);  
  
  
 label = lv_label_create(cont);
        lv_label_set_text(label, "charge_current:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 3, 1);


 label_bq25896_charge_current = lv_label_create(cont);
        lv_label_set_text(label_bq25896_charge_current, "");
        lv_obj_center(label_bq25896_charge_current);
lv_obj_set_grid_cell(label_bq25896_charge_current, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 3, 1);   
  
  
  
  label = lv_label_create(cont);
        lv_label_set_text(label, "charge_status:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 4, 1);


 label_bq25896_charge_status = lv_label_create(cont);
        lv_label_set_text(label_bq25896_charge_status, "");
        lv_obj_center(label_bq25896_charge_status);
lv_obj_set_grid_cell(label_bq25896_charge_status, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 4, 1);
  
 
   label = lv_label_create(cont);
        lv_label_set_text(label, "precharge_current_limit:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 5, 1);


 label_bq25896_precharge_currentl_limit = lv_label_create(cont);
        lv_label_set_text(label_bq25896_precharge_currentl_limit, "");
        lv_obj_center(label_bq25896_precharge_currentl_limit);
lv_obj_set_grid_cell(label_bq25896_precharge_currentl_limit, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 5, 1); 
  
  
     label = lv_label_create(cont);
        lv_label_set_text(label, "fast_charge_current_limit:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 6, 1);


 label_bq25896_fast_charge_currentl_limit = lv_label_create(cont);
        lv_label_set_text(label_bq25896_fast_charge_currentl_limit, "");
        lv_obj_center(label_bq25896_fast_charge_currentl_limit);
lv_obj_set_grid_cell(label_bq25896_fast_charge_currentl_limit, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 6, 1); 
  
  label = lv_label_create(cont);
        lv_label_set_text(label, "terminate_current_limit:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 7, 1);


 label_bq25896_terminate_current_limit = lv_label_create(cont);
        lv_label_set_text(label_bq25896_terminate_current_limit, "");
        lv_obj_center(label_bq25896_terminate_current_limit);
lv_obj_set_grid_cell(label_bq25896_terminate_current_limit, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 7, 1); 
  
   label = lv_label_create(cont);
        lv_label_set_text(label, "charge_voltage_limit:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 8, 1);


 label_bq25896_charge_voltage_limit = lv_label_create(cont);
        lv_label_set_text(label_bq25896_charge_voltage_limit, "");
        lv_obj_center(label_bq25896_charge_voltage_limit);
lv_obj_set_grid_cell(label_bq25896_charge_voltage_limit, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 8, 1);    
  #endif
  ////////////////////////////////////////////////////
  
  #if defined(USING_BQ27220)
  /*Create a container with grid*/
    static lv_coord_t col_dsc1[] = {300, 200, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc1[] = {80,80,80,80,80,80,80,80,80, LV_GRID_TEMPLATE_LAST};
    lv_obj_t * cont1 = lv_obj_create(tab_coulometer);
    lv_obj_set_style_grid_column_dsc_array(cont1, col_dsc1, 0);
    lv_obj_set_style_grid_row_dsc_array(cont1, row_dsc1, 0);
    lv_obj_set_size(cont1, 550, 850);
    lv_obj_center(cont1);
    lv_obj_set_layout(cont1, LV_LAYOUT_GRID);

label = lv_label_create(cont1);
        lv_label_set_text(label, "full_charge_capacity:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 0, 1);


 label_bq27220_full_charge_capacity = lv_label_create(cont1);
        lv_label_set_text(label_bq27220_full_charge_capacity, "");
        lv_obj_center(label_bq27220_full_charge_capacity);
lv_obj_set_grid_cell(label_bq27220_full_charge_capacity, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 0, 1);


label = lv_label_create(cont1);
        lv_label_set_text(label, "design_capacity:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 1, 1);


 label_bq27220_design_capacity = lv_label_create(cont1);
        lv_label_set_text(label_bq27220_design_capacity, "");
        lv_obj_center(label_bq27220_design_capacity);
lv_obj_set_grid_cell(label_bq27220_design_capacity, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 1, 1);
  
  
label = lv_label_create(cont1);
        lv_label_set_text(label, "voltage:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 2, 1);


 label_bq27220_voltage = lv_label_create(cont1);
        lv_label_set_text(label_bq27220_voltage, "");
        lv_obj_center(label_bq27220_voltage);
lv_obj_set_grid_cell(label_bq27220_voltage, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 2, 1);  
  
  
 label = lv_label_create(cont1);
        lv_label_set_text(label, "current:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 3, 1);


 label_bq27220_current = lv_label_create(cont1);
        lv_label_set_text(label_bq27220_current, "");
        lv_obj_center(label_bq27220_current);
lv_obj_set_grid_cell(label_bq27220_current, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 3, 1);   
  
  
  
  label = lv_label_create(cont1);
        lv_label_set_text(label, "temperature_c:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 4, 1);


 label_bq27220_temperature_c = lv_label_create(cont1);
        lv_label_set_text(label_bq27220_temperature_c, "");
        lv_obj_center(label_bq27220_temperature_c);
lv_obj_set_grid_cell(label_bq27220_temperature_c, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 4, 1);
  
 
   label = lv_label_create(cont1);
        lv_label_set_text(label, "remaining_capacity:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 5, 1);


 label_bq27220_remaining_capacity = lv_label_create(cont1);
        lv_label_set_text(label_bq27220_remaining_capacity, "");
        lv_obj_center(label_bq27220_remaining_capacity);
lv_obj_set_grid_cell(label_bq27220_remaining_capacity, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 5, 1); 
  
  
     label = lv_label_create(cont1);
        lv_label_set_text(label, "soc:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 6, 1);


 label_bq27220_soc = lv_label_create(cont1);
        lv_label_set_text(label_bq27220_soc, "");
        lv_obj_center(label_bq27220_soc);
lv_obj_set_grid_cell(label_bq27220_soc, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 6, 1); 
  
  label = lv_label_create(cont1);
        lv_label_set_text(label, "soh:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 7, 1);


 label_bq27220_soh = lv_label_create(cont1);
        lv_label_set_text(label_bq27220_soh, "");
        lv_obj_center(label_bq27220_soh);

lv_obj_set_grid_cell(label_bq27220_soh, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 7, 1); 
  
   label = lv_label_create(cont1);
        lv_label_set_text(label, "bat_state:");
        lv_obj_center(label);
lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                             LV_GRID_ALIGN_STRETCH, 8, 1);


 label_bq27220_bat_state = lv_label_create(cont1);
        lv_label_set_text(label_bq27220_bat_state, "");
        lv_obj_center(label_bq27220_bat_state);
lv_obj_set_grid_cell(label_bq27220_bat_state, LV_GRID_ALIGN_STRETCH, 1, 1,
                             LV_GRID_ALIGN_STRETCH, 8, 1);
  #endif
  
   ////////////////////////////////////////// 
      
  static uint32_t user_data = 35; 
#if defined(USING_BQ25896)
timer_battery_charge_monitor = lv_timer_create(custom_timer_cb_battery_charge_monitor, 2000, &user_data);
lv_timer_set_repeat_count(timer_battery_charge_monitor, -1); 
#endif



#if defined(USING_BQ27220)
    //BQ27220
    timer_battery_capacity_monitor= lv_timer_create(custom_timer_cb_battery_capacity_monitor, 2000, &user_data);
    lv_timer_set_repeat_count(timer_battery_capacity_monitor, -1);  
#endif
        
#if 0
    lv_obj_t *label = lv_label_create(lv_scr_act());
    ESP_BROOKESIA_CHECK_NULL_RETURN(label, false, "Create label failed");
    lv_label_set_text(label, "Top Middle");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_font(label, LV_FONT_DEFAULT, 0);

    label = lv_label_create(lv_scr_act());
    ESP_BROOKESIA_CHECK_NULL_RETURN(label, false, "Create label failed");
    lv_label_set_text(label, "Bottom Middle");
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_font(label, LV_FONT_DEFAULT, 0);

    label = lv_label_create(lv_scr_act());
    ESP_BROOKESIA_CHECK_NULL_RETURN(label, false, "Create label failed");
    lv_label_set_text(label, "Left Middle");
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_font(label, LV_FONT_DEFAULT, 0);

    label = lv_label_create(lv_scr_act());
    ESP_BROOKESIA_CHECK_NULL_RETURN(label, false, "Create label failed");
    lv_label_set_text(label, "Right Middle");
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_font(label, LV_FONT_DEFAULT, 0);

    label = lv_label_create(lv_scr_act());
    ESP_BROOKESIA_CHECK_NULL_RETURN(label, false, "Create label failed");
    lv_label_set_text(label, "Simple Conf");
    lv_obj_center(label);
    lv_obj_set_style_text_font(label, LV_FONT_DEFAULT, 0);
#endif
    return true;
}

/**
 * @brief The function will be called when the left button of navigate bar is clicked.
 */
bool Battery::back(void)
{
  notifyCoreClosed(); 
    return true;
}

/**
 * @brief The function will be called when app should be closed.
 */
bool Battery::close(void)
{ 
    //flag_running=0;
    //pthread_join(pthread_handle_uart_rx, NULL);
    //pthread_mutex_destroy(&mutex_uart_rx);
    return true;
}

bool Battery::init(void)
{

    return true;
}


 bool Battery::pause()
{

 printf("battery pause before \n");
 //notifyCoreClosed();
 printf("battery pause after \n");
     //ESP_BROOKESIA_LOGD("Pause");

     /* Do some operations here if needed */

     return true;
}

bool Battery::resume()
 {
     //ESP_BROOKESIA_LOGD("Resume");
      printf("battery resume \n");
     /* Do some operations here if needed */

     return true;
 }

void Battery::keyboard_event_cb(lv_event_t *e)
{
}
