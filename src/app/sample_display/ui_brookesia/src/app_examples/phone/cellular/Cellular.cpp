#include <math.h>
#include <vector>
#include <string>
#include "Cellular.hpp"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>
#include <poll.h>
#include <termios.h>
#include <errno.h>

using namespace std;

LV_IMG_DECLARE(img_app_cellular);

#define KEYBOARD_H_PERCENT      65
#define KEYBOARD_FONT           &lv_font_montserrat_48
#define KEYBOARD_SPECIAL_COLOR  lv_color_make(0, 0x99, 0xff)

#define LABEL_PAD               5
#define LABEL_FONT_SMALL        &lv_font_montserrat_34
#define LABEL_FONT_BIG          &lv_font_montserrat_48
#define LABEL_COLOR             lv_color_make(170, 170, 170)
#define LABEL_FORMULA_LEN_MAX   256

#define BUF_SIZE                1024
#define UART_DEVICE_NAME        "/dev/ttyACM0"

static char buf_rx[BUF_SIZE];
static int fd_uart = -1;

lv_obj_t *label_iccid;
lv_obj_t *label_imsi;
lv_obj_t *label_imei;
lv_obj_t *label_modem_fw;

string cellular_arr[4] = {"", "", "", ""};

static void trim_left(char **p)
{
    while (*p && **p == ' ') {
        (*p)++;
    }
}

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

    return sent;
}

static int send_at_cmd(int fd, const char *cmd, char *rx, int rx_size)
{
    int cnt = 0;

    if (fd < 0 || rx_size <= 1) {
        return -1;
    }

    memset(rx, 0, rx_size);
    tcflush(fd, TCIFLUSH);

    if (write_all(fd, cmd, strlen(cmd)) < 0) {
        return -1;
    }

    for (int i = 0; i < 20; i++) {
        int space = rx_size - cnt - 1;
        if (space <= 0) {
            break;
        }

        int ret = read(fd, &rx[cnt], space);
        if (ret > 0) {
            cnt += ret;
            rx[cnt] = '\0';

            printf("uart recv:\n%s\n", rx);

            if (strstr(rx, "\r\nOK\r\n") || strstr(rx, "\nOK\r\n") ||
                strstr(rx, "\r\nERROR\r\n") || strstr(rx, "\nERROR\r\n")) {
                return cnt;
            }
        } else if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("uart read");
            return -1;
        }

        usleep(100000);
    }

    return cnt;
}

static bool parse_iccid(char *rx, string &out)
{
    char *p = strstr(rx, "%XICCID:");
    if (!p) {
        p = strstr(rx, "XICCID:");
    }
    if (!p) {
        return false;
    }

    p = strchr(p, ':');
    if (!p) {
        return false;
    }

    p++;
    trim_left(&p);

    char *end = strpbrk(p, "\r\n");
    if (end) {
        *end = '\0';
    }

    out = p;
    return !out.empty();
}

static bool parse_15_digit_line(char *rx, string &out)
{
    char *saveptr = NULL;
    char *token = strtok_r(rx, "\r\n", &saveptr);

    while (token) {
        if (strlen(token) == 15) {
            out = token;
            return true;
        }
        token = strtok_r(NULL, "\r\n", &saveptr);
    }

    return false;
}

static bool parse_modem_fw(char *rx, string &out)
{
    char *saveptr = NULL;
    char *token = strtok_r(rx, "\r\n", &saveptr);

    while (token) {
        if (strstr(token, "mfw") || strstr(token, "MFW")) {
            out = token;
            return true;
        }
        token = strtok_r(NULL, "\r\n", &saveptr);
    }

    return false;
}

Cellular::Cellular(bool use_status_bar, bool use_navigation_bar):
    ESP_Brookesia_PhoneApp(
        ESP_BROOKESIA_CORE_APP_DATA_DEFAULT("Cellular", &img_app_cellular, true),
        ESP_BROOKESIA_PHONE_APP_DATA_DEFAULT(&img_app_cellular, use_status_bar, use_navigation_bar)
    )
{
}

Cellular::~Cellular()
{
}

void Cellular::btn_clicked_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
    }
}

bool Cellular::run(void)
{
    lv_area_t area = getVisualArea();
    _width = area.x2 - area.x1;
    _height = area.y2 - area.y1;

    static lv_coord_t col_dsc[] = {300, 200, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {80, 80, 80, 80, 80, LV_GRID_TEMPLATE_LAST};

    lv_obj_t *cont = lv_obj_create(lv_scr_act());
    lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);
    lv_obj_set_size(cont, 550, 480);
    lv_obj_center(cont);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);

    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, "ICCID:");
    lv_obj_center(label);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 0, 1);

    label_iccid = lv_label_create(cont);
    lv_label_set_text(label_iccid, "");
    lv_obj_center(label_iccid);
    lv_obj_set_grid_cell(label_iccid, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 0, 1);

    label = lv_label_create(cont);
    lv_label_set_text(label, "IMSI:");
    lv_obj_center(label);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 1, 1);

    label_imsi = lv_label_create(cont);
    lv_label_set_text(label_imsi, "");
    lv_obj_center(label_imsi);
    lv_obj_set_grid_cell(label_imsi, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 1, 1);

    label = lv_label_create(cont);
    lv_label_set_text(label, "IMEI:");
    lv_obj_center(label);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 2, 1);

    label_imei = lv_label_create(cont);
    lv_label_set_text(label_imei, "");
    lv_obj_center(label_imei);
    lv_obj_set_grid_cell(label_imei, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 2, 1);

    label = lv_label_create(cont);
    lv_label_set_text(label, "modem_fw:");
    lv_obj_center(label);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 3, 1);

    label_modem_fw = lv_label_create(cont);
    lv_label_set_text(label_modem_fw, "");
    lv_obj_center(label_modem_fw);
    lv_obj_set_grid_cell(label_modem_fw, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 3, 1);
   if(fd_uart==-1)
   {
    fd_uart = open_uart(UART_DEVICE_NAME);
    }
    if (fd_uart >= 0) {
        if (send_at_cmd(fd_uart, "AT%XICCID\r\n", buf_rx, BUF_SIZE) > 0) {
            parse_iccid(buf_rx, cellular_arr[0]);
        }

        if (send_at_cmd(fd_uart, "AT+CIMI\r\n", buf_rx, BUF_SIZE) > 0) {
            parse_15_digit_line(buf_rx, cellular_arr[1]);
        }

        if (send_at_cmd(fd_uart, "AT+CGSN\r\n", buf_rx, BUF_SIZE) > 0) {
            parse_15_digit_line(buf_rx, cellular_arr[2]);
        }

        if (send_at_cmd(fd_uart, "AT+CGMR\r\n", buf_rx, BUF_SIZE) > 0) {
            parse_modem_fw(buf_rx, cellular_arr[3]);
        }

        //close(fd_uart);
        //fd_uart = -1;
    }

    lv_label_set_text(label_iccid, cellular_arr[0].c_str());
    lv_label_set_text(label_imsi, cellular_arr[1].c_str());
    lv_label_set_text(label_imei, cellular_arr[2].c_str());
    lv_label_set_text(label_modem_fw, cellular_arr[3].c_str());

    return true;
}

bool Cellular::back(void)
{
    notifyCoreClosed();
    return true;
}

bool Cellular::close(void)
{
    if (fd_uart >= 0) {
        //close(fd_uart);
        //fd_uart = -1;
    }

    return true;
}

bool Cellular::init(void)
{
    return true;
}

bool Cellular::pause()
{
    printf("cellular pause before \n");
    printf("cellular pause after \n");
    return true;
}

bool Cellular::resume()
{
    printf("cellular resume \n");
    return true;
}

void Cellular::keyboard_event_cb(lv_event_t *e)
{
}

