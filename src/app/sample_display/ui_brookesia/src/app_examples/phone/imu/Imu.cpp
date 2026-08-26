#include "Imu.hpp"
#include "imu_iio.h"

#include <stdio.h>

LV_IMG_DECLARE(img_app_imu);

static lv_obj_t *label_accel_x;
static lv_obj_t *label_accel_y;
static lv_obj_t *label_accel_z;
static lv_obj_t *label_gyro_x;
static lv_obj_t *label_gyro_y;
static lv_obj_t *label_gyro_z;
static lv_obj_t *label_status;
static lv_timer_t *timer_imu_monitor;

static void set_value(lv_obj_t *label, double value, const char *unit)
{
    char text[48];

    snprintf(text, sizeof(text), "%.4f %s", value, unit);
    lv_label_set_text(label, text);
}

static void set_group_unavailable(lv_obj_t *x, lv_obj_t *y, lv_obj_t *z)
{
    lv_label_set_text(x, "--");
    lv_label_set_text(y, "--");
    lv_label_set_text(z, "--");
}

static void imu_timer_cb(lv_timer_t *timer)
{
    imu_iio_sample_t sample;
    (void)timer;

    if (imu_iio_read(&sample) != 0) {
        lv_label_set_text(label_status, "IMU read failed");
        return;
    }

    if (sample.has_accel) {
        set_value(label_accel_x, sample.accel_x, "m/s2");
        set_value(label_accel_y, sample.accel_y, "m/s2");
        set_value(label_accel_z, sample.accel_z, "m/s2");
    } else {
        set_group_unavailable(label_accel_x, label_accel_y, label_accel_z);
    }

    if (sample.has_gyro) {
        set_value(label_gyro_x, sample.gyro_x, "rad/s");
        set_value(label_gyro_y, sample.gyro_y, "rad/s");
        set_value(label_gyro_z, sample.gyro_z, "rad/s");
    } else {
        set_group_unavailable(label_gyro_x, label_gyro_y, label_gyro_z);
    }

    lv_label_set_text(label_status, "Running");
}

static void create_data_row(lv_obj_t *parent, int row, const char *title,
                            lv_obj_t **value_label)
{
    lv_obj_t *title_label = lv_label_create(parent);
    lv_label_set_text(title_label, title);
    lv_obj_set_grid_cell(title_label,
                         LV_GRID_ALIGN_START, 0, 1,
                         LV_GRID_ALIGN_CENTER, row, 1);

    *value_label = lv_label_create(parent);
    lv_label_set_text(*value_label, "--");
    lv_obj_set_grid_cell(*value_label,
                         LV_GRID_ALIGN_START, 1, 1,
                         LV_GRID_ALIGN_CENTER, row, 1);
}

Imu::Imu(bool use_status_bar, bool use_navigation_bar) :
    ESP_Brookesia_PhoneApp(
        ESP_BROOKESIA_CORE_APP_DATA_DEFAULT("IMU", &img_app_imu, true),
        ESP_BROOKESIA_PHONE_APP_DATA_DEFAULT(
            &img_app_imu, use_status_bar, use_navigation_bar))
{
}

Imu::~Imu()
{
}

void Imu::btn_clicked_event(lv_event_t *event)
{
    (void)event;
}

bool Imu::run(void)
{
    static lv_coord_t col_dsc[] = {
        300, 200, LV_GRID_TEMPLATE_LAST
    };
    static lv_coord_t row_dsc[] = {
        70, 70, 70, 70, 70, 70, 50, LV_GRID_TEMPLATE_LAST
    };
    lv_area_t area = getVisualArea();
    lv_obj_t *cont;

    _width = area.x2 - area.x1;
    _height = area.y2 - area.y1;

    cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cont, 550, 650);
    lv_obj_center(cont);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);
    lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);

    create_data_row(cont, 0, "Accel X:", &label_accel_x);
    create_data_row(cont, 1, "Accel Y:", &label_accel_y);
    create_data_row(cont, 2, "Accel Z:", &label_accel_z);
    create_data_row(cont, 3, "Gyro X:", &label_gyro_x);
    create_data_row(cont, 4, "Gyro Y:", &label_gyro_y);
    create_data_row(cont, 5, "Gyro Z:", &label_gyro_z);

    label_status = lv_label_create(cont);
    lv_label_set_text(label_status, "Initializing...");
    lv_obj_set_grid_cell(label_status,
                         LV_GRID_ALIGN_START, 0, 2,
                         LV_GRID_ALIGN_CENTER, 6, 1);

    if (imu_iio_init(NULL) != 0) {
        lv_label_set_text(label_status, "IIO IMU not found");
        return true;
    }
    timer_imu_monitor = lv_timer_create(imu_timer_cb, 500, NULL);
    //imu_timer_cb(timer_imu_monitor);
    lv_timer_set_repeat_count(timer_imu_monitor, -1);  
    return true;
}

bool Imu::back(void)
{
    notifyCoreClosed();
    return true;
}

bool Imu::close(void)
{
    if (timer_imu_monitor != NULL) {
        //lv_timer_del(timer_imu_monitor);
        //timer_imu_monitor = NULL;
    }
    imu_iio_deinit();
    return true;
}

bool Imu::init(void)
{
    return true;
}

bool Imu::pause(void)
{
    printf("imu pause before \n");
    //notifyCoreClosed();
    printf("imu pause after \n");
    //if (timer_imu_monitor != NULL)
       // lv_timer_pause(timer_imu_monitor);
    return true;
}

bool Imu::resume(void)
{
    printf("weather resume \n");
    //if (timer_imu_monitor != NULL)
       // lv_timer_resume(timer_imu_monitor);
    return true;
}

void Imu::keyboard_event_cb(lv_event_t *event)
{
    (void)event;
}

