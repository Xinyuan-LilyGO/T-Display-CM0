#include <math.h>
#include <vector>
#include "KeyBoard.hpp"
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
using namespace std;
LV_IMG_DECLARE(img_app_keyboard);
#define KEYBOARD_H_PERCENT      65
#define KEYBOARD_FONT           &lv_font_montserrat_48
#define KEYBOARD_SPECIAL_COLOR  lv_color_make(0, 0x99, 0xff)

#define LABEL_PAD               5
#define LABEL_FONT_SMALL        &lv_font_montserrat_34
#define LABEL_FONT_BIG          &lv_font_montserrat_40
#define LABEL_COLOR             lv_color_make(170, 170, 170)
#define LABEL_FORMULA_LEN_MAX   256

 lv_obj_t * ta_keyboard;    
          
KeyBoard::KeyBoard(bool use_status_bar, bool use_navigation_bar):
    ESP_Brookesia_PhoneApp(
        ESP_BROOKESIA_CORE_APP_DATA_DEFAULT("Keyboard", &img_app_keyboard, true),
        ESP_BROOKESIA_PHONE_APP_DATA_DEFAULT(&img_app_keyboard, use_status_bar, use_navigation_bar)
    )
{
}

KeyBoard::~KeyBoard()
{

}

void KeyBoard::btn_clicked_event(lv_event_t * e)
{

 }
bool KeyBoard::run(void)
{
    lv_area_t area = getVisualArea();
    _width = area.x2 - area.x1;
    _height = area.y2 - area.y1; 
   
    
    #if 1
    int text_width=1000;
    int text_height=560;
    ta_keyboard = lv_textarea_create(lv_scr_act());
    //lv_textarea_set_one_line(ta, true);
    lv_obj_set_size(ta_keyboard, text_width, text_height);                    // 设置大小
    //lv_obj_align(ta_keyboard, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_align(ta_keyboard, LV_ALIGN_CENTER, 0, 0);
    lv_textarea_set_placeholder_text(ta_keyboard, "Please connect the external keyboard for use.Enter your text...");
    //lv_obj_add_event_cb(ta, textarea_event_handler, LV_EVENT_READY, ta);
    lv_obj_add_state(ta_keyboard, LV_STATE_FOCUSED); /*To be sure the cursor is visible*/
   // if(SCREEN_ORIENTATATION == 0)
    { 
    // 旋转45度（450 = 45.0度）
    lv_obj_set_style_transform_angle(ta_keyboard, 2700, 0);
    // 可选：设置旋转中心为控件中心（默认值）
    lv_obj_set_style_transform_pivot_x(ta_keyboard, text_width/2, 0);
    lv_obj_set_style_transform_pivot_y(ta_keyboard, text_height/2, 0);
    }
    lv_obj_set_style_text_font(ta_keyboard, &lv_font_montserrat_38, 0);
    #endif 
    return true;
}

/**
 * @brief The function will be called when the left button of navigate bar is clicked.
 */
bool KeyBoard::back(void)
{
  notifyCoreClosed(); 
    return true;
}

/**
 * @brief The function will be called when app should be closed.
 */
bool KeyBoard::close(void)
{ 
    lv_textarea_set_text(ta_keyboard, "");
    return true;
}

bool KeyBoard::init(void)
{

    return true;
}


 bool KeyBoard::pause()
{

 printf("keyboard pause before \n");
 notifyCoreClosed();
 printf("keyboard pause after \n");
     //ESP_BROOKESIA_LOGD("Pause");

     /* Do some operations here if needed */

     return true;
}

bool KeyBoard::resume()
 {
     //ESP_BROOKESIA_LOGD("Resume");
      printf("keyboard resume \n");
     /* Do some operations here if needed */

     return true;
 }

void KeyBoard::keyboard_event_cb(lv_event_t *e)
{
}
