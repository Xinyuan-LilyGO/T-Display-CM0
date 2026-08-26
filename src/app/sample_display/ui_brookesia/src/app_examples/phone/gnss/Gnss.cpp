#include <math.h>
#include <vector>
#include <string>
#include "Gnss.hpp"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <poll.h>
#include <termios.h>
#include <errno.h>
#include <time.h>

using namespace std;

LV_IMG_DECLARE(img_app_gnss);

#define KEYBOARD_H_PERCENT      65
#define KEYBOARD_FONT           &lv_font_montserrat_48
#define KEYBOARD_SPECIAL_COLOR  lv_color_make(0, 0x99, 0xff)

#define LABEL_PAD               5
#define LABEL_FONT_SMALL        &lv_font_montserrat_34
#define LABEL_FONT_BIG          &lv_font_montserrat_48
#define LABEL_COLOR             lv_color_make(170, 170, 170)
#define LABEL_FORMULA_LEN_MAX   256

#define BUF_SIZE                1024
#define LINE_BUF_SIZE           512
#define UART_DEVICE_NAME        "/dev/ttyACM0"

static char buf_rx[BUF_SIZE];
static int fd_uart = -1;
static int flag_running = 1;
static pthread_t pthread_handle_uart_rx = 0;
static pthread_cond_t cond;
static pthread_mutex_t mutex_uart_rx;
static int pthread_objects_inited = 0;
static pthread_t pthread_handle_locate = 0;
static int locate_thread_started = 0;
static int locate_thread_running = 0;
static pthread_mutex_t locate_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t uart_write_mutex = PTHREAD_MUTEX_INITIALIZER;

lv_obj_t *label_latitude;
lv_obj_t *label_longitude;
lv_obj_t *label_altitude;
lv_obj_t *label_satellite_speed;
lv_obj_t *label_satellite_date;
lv_obj_t *label_satellite_time;
lv_obj_t *btn_locate = NULL;
lv_obj_t *label_btn_locate = NULL;

struct gnss_fix_t {
    string latitude;
    string longitude;
    string altitude;
    string speed;
    string utc;
};

static pthread_mutex_t gnss_fix_mutex = PTHREAD_MUTEX_INITIALIZER;
static gnss_fix_t pending_fix;
static int pending_fix_ready = 0;
static int locating_active = 0;
static int locating_has_fix = 0;
static lv_timer_t *gnss_ui_timer = NULL;

static int open_uart(const char *dev)
{
    int fd = open(dev, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("open uart");
        return -1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        //close(fd);
        return -1;
    }

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
#ifdef CRTSCTS
    tty.c_cflag &= ~CRTSCTS;
#endif
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(ICRNL | INLCR | IGNCR);
    tty.c_oflag &= ~OPOST;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        //close(fd);
        return -1;
    }

    return fd;
}

static void set_modem_lines(int fd)
{
    int status = 0;

    if (ioctl(fd, TIOCMGET, &status) == 0) {
        status |= TIOCM_DTR;
        status |= TIOCM_RTS;
        ioctl(fd, TIOCMSET, &status);
    }
}

static int write_all(int fd, const char *data, int len)
{
    int sent = 0;

    while (sent < len) {
        int ret = write(fd, data + sent, len - sent);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("uart write");
            return -1;
        }
        sent += ret;
    }

    tcdrain(fd);
    return sent;
}

static int send_uart_cmd(const char *cmd, int delay_ms)
{
    if (fd_uart < 0) {
        return -1;
    }

    pthread_mutex_lock(&uart_write_mutex);
    set_modem_lines(fd_uart);

    int len = strlen(cmd);
    int sent = write_all(fd_uart, cmd, len);
    pthread_mutex_unlock(&uart_write_mutex);

    if (sent >= 0) {
        printf("uart send %d/%d bytes: %s", sent, len, cmd);
    }

    if (delay_ms > 0) {
        usleep(delay_ms * 1000);
    }

    return sent;
}

static void update_system_time_from_utc(const char *utc_text)
{
    int year, mon, day, hour, min, sec;

    if (sscanf(utc_text, "%d-%d-%d %d:%d:%d",
               &year, &mon, &day, &hour, &min, &sec) != 6) {
        return;
    }

    struct tm at_time;
    memset(&at_time, 0, sizeof(at_time));
    at_time.tm_year = year - 1900;
    at_time.tm_mon = mon - 1;
    at_time.tm_mday = day;
    at_time.tm_hour = hour;
    at_time.tm_min = min;
    at_time.tm_sec = sec;
    at_time.tm_isdst = -1;

    time_t at_ts = mktime(&at_time);
    if (at_ts < 0) {
        return;
    }

    struct timespec ts;
    ts.tv_sec = at_ts;
    ts.tv_nsec = 0;

    if (clock_settime(CLOCK_REALTIME, &ts) != 0) {
        perror("clock_settime");
    }
}

static void set_locate_button_enabled(bool enabled)
{
    if (!btn_locate) {
        return;
    }

    if (enabled) {
        lv_obj_clear_state(btn_locate, LV_STATE_DISABLED);
        lv_obj_add_flag(btn_locate, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(btn_locate, lv_color_hex(0x2b7bf6), LV_PART_MAIN);

        if (label_btn_locate) {
            lv_label_set_text(label_btn_locate, "Locate");
        }
    } else {
        lv_obj_add_state(btn_locate, LV_STATE_DISABLED);
        lv_obj_clear_flag(btn_locate, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(btn_locate, lv_color_hex(0xc4c4cf), LV_PART_MAIN);

        if (label_btn_locate) {
            lv_label_set_text(label_btn_locate, "Locating");
        }
    }
}

static void gnss_ui_timer_cb(lv_timer_t *timer)
{
    gnss_fix_t fix;
    char text[32];
    int has_fix = 0;
    int is_locating = 0;
    int has_located = 0;

    pthread_mutex_lock(&gnss_fix_mutex);
    has_fix = pending_fix_ready;
    is_locating = locating_active;
    has_located = locating_has_fix;
    if (has_fix) {
        fix = pending_fix;
        pending_fix_ready = 0;
    }
    pthread_mutex_unlock(&gnss_fix_mutex);

    if (btn_locate) {
        if (is_locating) {
            set_locate_button_enabled(false);
        } else if (has_located) {
            set_locate_button_enabled(true);
        }
    }

    if (!has_fix) {
        return;
    }

    lv_label_set_text(label_latitude, fix.latitude.c_str());
    lv_label_set_text(label_longitude, fix.longitude.c_str());

    snprintf(text, sizeof(text), "%s m", fix.altitude.c_str());
    lv_label_set_text(label_altitude, text);

    snprintf(text, sizeof(text), "%s m/s", fix.speed.c_str());
    lv_label_set_text(label_satellite_speed, text);

    if (fix.utc.length() >= 19) {
        lv_label_set_text(label_satellite_date, fix.utc.substr(0, 10).c_str());
        lv_label_set_text(label_satellite_time, fix.utc.substr(11, 8).c_str());
    }

    set_locate_button_enabled(true);
}

static void parse_xgps_line(char *line)
{
    pthread_mutex_lock(&gnss_fix_mutex);
    if (!locating_active || locating_has_fix) {
        pthread_mutex_unlock(&gnss_fix_mutex);
        return;
    }
    pthread_mutex_unlock(&gnss_fix_mutex);

    if (!strstr(line, "#XGPS:")) {
        return;
    }

    char *payload = strstr(line, "#XGPS:");
    payload += strlen("#XGPS:");

    string gnss_arr[7] = {"", "", "", "", "", "", ""};
    char *saveptr = NULL;
    char *token = strtok_r(payload, ",", &saveptr);
    int i = 0;

    while (token && i < 7) {
        gnss_arr[i] = token;
        token = strtok_r(NULL, ",", &saveptr);
        i++;
    }

    if (i < 7) {
        printf("invalid #XGPS line: %s\n", line);
        return;
    }

    if (gnss_arr[0].empty() || gnss_arr[1].empty() ||
        gnss_arr[2].empty() || gnss_arr[4].empty() ||
        gnss_arr[6].length() < 20) {
        printf("waiting valid #XGPS fix: %s\n", line);
        return;
    }

    gnss_fix_t fix;
    fix.latitude = gnss_arr[0];
    fix.longitude = gnss_arr[1];
    fix.altitude = gnss_arr[2];
    fix.speed = gnss_arr[4];

    if (gnss_arr[6].length() >= 20) {
        string utc = gnss_arr[6];
        if (!utc.empty() && utc[0] == '"') {
            utc = utc.substr(1);
        }

        fix.utc = utc;
        update_system_time_from_utc(utc.substr(0, 19).c_str());
    }

    pthread_mutex_lock(&gnss_fix_mutex);
    pending_fix = fix;
    pending_fix_ready = 1;
    locating_active = 0;
    locating_has_fix = 1;
    pthread_mutex_unlock(&gnss_fix_mutex);

    send_uart_cmd("AT#XGPS=0\r\n", 0);
}

static void *locate_cmd_thread_entry(void *parameter)
{
    if (fd_uart >= 0) {
        tcflush(fd_uart, TCIFLUSH);
    }

    send_uart_cmd("AT#XGPS=0\r\n", 2000);
    send_uart_cmd("AT+CFUN=0\r\n", 2000);
    send_uart_cmd("AT%XSYSTEMMODE=0,0,1,0\r\n", 2000);
    send_uart_cmd("AT+CFUN=31\r\n", 2000);
    send_uart_cmd("AT#XGPS=1,0,0,0\r\n", 2000);

    pthread_mutex_lock(&locate_mutex);
    locate_thread_running = 0;
    pthread_mutex_unlock(&locate_mutex);

    return NULL;
}

void *Gnss::thread_uart_rx_entry(void *parameter)
{
    struct pollfd fds[1];
    char line_buf[LINE_BUF_SIZE];
    int line_len = 0;

    memset(line_buf, 0, sizeof(line_buf));
    memset(fds, 0, sizeof(fds));

    fds[0].fd = fd_uart;
    fds[0].events = POLLIN;

    printf("[app] gnss uart poll start\n");
    flag_running = 1;

    while (flag_running) {
        int ready = poll(fds, 1, 500);
        if (ready == 0) {
            continue;
        }
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("uart poll");
            break;
        }

        if (!(fds[0].revents & POLLIN)) {
            continue;
        }

        int ret = read(fd_uart, buf_rx, BUF_SIZE - 1);
        if (ret <= 0) {
            if (ret < 0 && errno != EINTR) {
                perror("uart read");
            }
            continue;
        }

        buf_rx[ret] = '\0';
        printf("uart recv %d bytes:\n%s\n", ret, buf_rx);

        for (int i = 0; i < ret; i++) {
            char ch = buf_rx[i];

            if (ch == '\r') {
                continue;
            }

            if (ch == '\n') {
                line_buf[line_len] = '\0';
                if (line_len > 0) {
                    printf("gnss line: %s\n", line_buf);
                    parse_xgps_line(line_buf);
                }
                line_len = 0;
                memset(line_buf, 0, sizeof(line_buf));
                continue;
            }

            if (line_len < LINE_BUF_SIZE - 1) {
                line_buf[line_len++] = ch;
            } else {
                line_len = 0;
                memset(line_buf, 0, sizeof(line_buf));
            }
        }
    }

    return NULL;
}

Gnss::Gnss(bool use_status_bar, bool use_navigation_bar):
    ESP_Brookesia_PhoneApp(
        ESP_BROOKESIA_CORE_APP_DATA_DEFAULT("Gnss", &img_app_gnss, true),
        ESP_BROOKESIA_PHONE_APP_DATA_DEFAULT(&img_app_gnss, use_status_bar, use_navigation_bar)
    )
{
}

Gnss::~Gnss()
{
}

void Gnss::btn_clicked_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        pthread_mutex_lock(&gnss_fix_mutex);
        if (locating_active) {
            pthread_mutex_unlock(&gnss_fix_mutex);
            return;
        }
        locating_active = 1;
        locating_has_fix = 0;
        pending_fix_ready = 0;
        pthread_mutex_unlock(&gnss_fix_mutex);

        lv_label_set_text(label_latitude, "");
        lv_label_set_text(label_longitude, "");
        lv_label_set_text(label_altitude, "");
        lv_label_set_text(label_satellite_speed, "");
        lv_label_set_text(label_satellite_date, "");
        lv_label_set_text(label_satellite_time, "");

        set_locate_button_enabled(false);

        pthread_mutex_lock(&locate_mutex);
        if (!locate_thread_running) {
            if (locate_thread_started) {
                pthread_join(pthread_handle_locate, NULL);
                pthread_handle_locate = 0;
                locate_thread_started = 0;
            }
            locate_thread_running = 1;
            locate_thread_started = 1;
            pthread_create(&pthread_handle_locate, NULL, locate_cmd_thread_entry, NULL);
        } else {
            pthread_mutex_lock(&gnss_fix_mutex);
            locating_active = 0;
            pthread_mutex_unlock(&gnss_fix_mutex);
            set_locate_button_enabled(true);
        }
        pthread_mutex_unlock(&locate_mutex);
    }
}

bool Gnss::run(void)
{
    lv_area_t area = getVisualArea();
    _width = area.x2 - area.x1;
    _height = area.y2 - area.y1;

    static lv_coord_t col_dsc[] = {300, 200, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {80, 80, 80, 80, 80, 80, 80, LV_GRID_TEMPLATE_LAST};

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);
    lv_obj_set_size(cont, 550, 600);
    lv_obj_center(cont);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);

    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, "latitude:");
    lv_obj_center(label);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 0, 1);

    label_latitude = lv_label_create(cont);
    lv_label_set_text(label_latitude, "");
    lv_obj_center(label_latitude);
    lv_obj_set_grid_cell(label_latitude, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 0, 1);

    label = lv_label_create(cont);
    lv_label_set_text(label, "longitude:");
    lv_obj_center(label);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 1, 1);

    label_longitude = lv_label_create(cont);
    lv_label_set_text(label_longitude, "");
    lv_obj_center(label_longitude);
    lv_obj_set_grid_cell(label_longitude, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 1, 1);

    label = lv_label_create(cont);
    lv_label_set_text(label, "altitude:");
    lv_obj_center(label);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 2, 1);

    label_altitude = lv_label_create(cont);
    lv_label_set_text(label_altitude, "");
    lv_obj_center(label_altitude);
    lv_obj_set_grid_cell(label_altitude, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 2, 1);

    label = lv_label_create(cont);
    lv_label_set_text(label, "speed:");
    lv_obj_center(label);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 3, 1);

    label_satellite_speed = lv_label_create(cont);
    lv_label_set_text(label_satellite_speed, "");
    lv_obj_center(label_satellite_speed);
    lv_obj_set_grid_cell(label_satellite_speed, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 3, 1);

    label = lv_label_create(cont);
    lv_label_set_text(label, "date(utc):");
    lv_obj_center(label);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 4, 1);

    label_satellite_date = lv_label_create(cont);
    lv_label_set_text(label_satellite_date, "");
    lv_obj_center(label_satellite_date);
    lv_obj_set_grid_cell(label_satellite_date, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 4, 1);

    label = lv_label_create(cont);
    lv_label_set_text(label, "time(utc):");
    lv_obj_center(label);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 5, 1);

    label_satellite_time = lv_label_create(cont);
    lv_label_set_text(label_satellite_time, "");
    lv_obj_center(label_satellite_time);
    lv_obj_set_grid_cell(label_satellite_time, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 5, 1);

    btn_locate = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn_locate, btn_clicked_event, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn_locate, LV_ALIGN_CENTER, -20, 360);
    lv_obj_set_style_radius(btn_locate, LV_PCT(20), LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_locate, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_locate, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);

    label_btn_locate = lv_label_create(btn_locate);
    lv_label_set_text(label_btn_locate, "Locate");
    lv_obj_center(label_btn_locate);

    gnss_ui_timer = lv_timer_create(gnss_ui_timer_cb, 200, NULL);

    printf("[app] open gnss uart: %s\n", UART_DEVICE_NAME);
     if(fd_uart==-1)
   {
    fd_uart = open_uart(UART_DEVICE_NAME);
    }
    if (fd_uart >= 0) {
        pthread_mutex_init(&mutex_uart_rx, NULL);
        pthread_cond_init(&cond, NULL);
        pthread_objects_inited = 1;
        pthread_create(&pthread_handle_uart_rx, NULL, thread_uart_rx_entry, NULL);
    }

    return true;
}

bool Gnss::back(void)
{
    notifyCoreClosed();
    return true;
}

bool Gnss::close(void)
{
    flag_running = 0;

    if (gnss_ui_timer) {
        lv_timer_del(gnss_ui_timer);
        gnss_ui_timer = NULL;
    }

    if (pthread_handle_uart_rx) {
        pthread_join(pthread_handle_uart_rx, NULL);
        pthread_handle_uart_rx = 0;
    }

    if (locate_thread_started) {
        pthread_join(pthread_handle_locate, NULL);
        pthread_handle_locate = 0;
        locate_thread_started = 0;
        locate_thread_running = 0;
    }

    if (pthread_objects_inited) {
        pthread_mutex_destroy(&mutex_uart_rx);
        pthread_cond_destroy(&cond);
        pthread_objects_inited = 0;
    }

    if (fd_uart >= 0) {
        //close(fd_uart);
        //fd_uart = -1;
    }

    return true;
}

bool Gnss::init(void)
{
    return true;
}

bool Gnss::pause()
{
    printf("gnss pause before \n");
    printf("gnss pause after \n");
    return true;
}

bool Gnss::resume()
{
    printf("gnss resume \n");
    return true;
}

void Gnss::keyboard_event_cb(lv_event_t *e)
{
}

