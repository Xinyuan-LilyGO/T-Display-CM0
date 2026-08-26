#include <math.h>
#include <vector>
#include "Music.hpp"
//#include "../../../../../audio/audio_aio.h"
#include "ui_MusicPlayer.h"
using namespace std;
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
/* ioctl */
#define	GPIO_DM_OUTPUT           _IOW('G', 0, int)
#define	GPIO_DM_INPUT            _IOW('G', 1, int)
#define	GPIO_DM_INPUT_PULL_UP    _IOW('G', 2, int)
#define	GPIO_DM_INPUT_PULL_DOWN  _IOW('G', 3, int)
#define	GPIO_WRITE_LOW           _IOW('G', 4, int)
#define	GPIO_WRITE_HIGH          _IOW('G', 5, int)

#define	GPIO_PE_RISING           _IOW('G', 7, int)
#define	GPIO_PE_FALLING          _IOW('G', 8, int)
#define	GPIO_PE_BOTH             _IOW('G', 9, int)
#define	GPIO_PE_HIGH             _IOW('G', 10, int)
#define	GPIO_PE_LOW              _IOW('G', 11, int)
LV_IMG_DECLARE(img_app_music);

#define KEYBOARD_H_PERCENT      65
#define KEYBOARD_FONT           &lv_font_montserrat_48
#define KEYBOARD_SPECIAL_COLOR  lv_color_make(0, 0x99, 0xff)

#define LABEL_PAD               5
#define LABEL_FONT_SMALL        &lv_font_montserrat_34
#define LABEL_FONT_BIG          &lv_font_montserrat_48
#define LABEL_COLOR             lv_color_make(170, 170, 170)
#define LABEL_FORMULA_LEN_MAX   256



Music::Music(bool use_status_bar, bool use_navigation_bar):
    ESP_Brookesia_PhoneApp(
        ESP_BROOKESIA_CORE_APP_DATA_DEFAULT("Music", &img_app_music, true),
        ESP_BROOKESIA_PHONE_APP_DATA_DEFAULT(&img_app_music, use_status_bar, use_navigation_bar)
    )
{
}

Music::~Music()
{

}


bool Music::run(void)
{
    lv_area_t area = getVisualArea();
    _width = area.x2 - area.x1;
    _height = area.y2 - area.y1;
    //////
    ui_MusicPlayer_screen_init();
    return true;
}

/**
 * @brief The function will be called when the left button of navigate bar is clicked.
 */
bool Music::back(void)
{
    notifyCoreClosed();

    return true;
}

/**
 * @brief The function will be called when app should be closed.
 */
bool Music::close(void)
{
   music_play_close(); 
    return true;
}

bool Music::init(void)
{

    return true;
}

void Music::keyboard_event_cb(lv_event_t *e)
{
}
