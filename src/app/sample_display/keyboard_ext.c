#include "keyboard_ext.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define INPUT_DEVICES_FILE "/proc/bus/input/devices"
#define TARGET_INPUT_NAME  "tca8418"
#define FALLBACK_INPUT_DEV "/dev/input/event0"

#define BACKLIGHT_PWM_CHIP      "/sys/class/pwm/pwmchip0"
#define BACKLIGHT_PWM_CHANNEL   "0"
#define BACKLIGHT_PWM_PERIOD_NS 1000000U
#define BACKLIGHT_PWM_DUTY_ON   100000U
#define BACKLIGHT_PWM_DUTY_OFF  0U

static int kb_fd = -1;
static int shift_down = 0;
static int caps_on = 0;
static int fn_down = 0;

static uint32_t last_key = 0;
static lv_indev_state_t last_state = LV_INDEV_STATE_REL;
static char kb_device_path[64];

static void input_block_clear(int *name_matched, char *event_name, size_t event_size)
{
    *name_matched = 0;
    if(event_size > 0) {
        event_name[0] = '\0';
    }
}

static int input_block_ready(int name_matched, const char *event_name)
{
    return name_matched && event_name[0] != '\0';
}

static int copy_event_name(const char *line, char *event_name, size_t event_size)
{
    const char *p = strstr(line, "event");
    size_t len = 0;

    if(!p || event_size == 0) {
        return 0;
    }

    while(p[len] && p[len] != ' ' && p[len] != '\t' && p[len] != '\n') {
        if(len + 1 >= event_size) {
            return 0;
        }
        len++;
    }

    memcpy(event_name, p, len);
    event_name[len] = '\0';
    return 1;
}

static int find_tca8418_event(char *path, size_t path_size)
{
    FILE *fp = fopen(INPUT_DEVICES_FILE, "r");
    char line[256];
    char event_name[32];
    int name_matched = 0;

    if(!fp) {
        return -1;
    }

    input_block_clear(&name_matched, event_name, sizeof(event_name));

    while(fgets(line, sizeof(line), fp)) {
        if(line[0] == '\n' || line[0] == '\r') {
            if(input_block_ready(name_matched, event_name)) {
                snprintf(path, path_size, "/dev/input/%s", event_name);
                fclose(fp);
                return 0;
            }
            input_block_clear(&name_matched, event_name, sizeof(event_name));
            continue;
        }

        if(strstr(line, "N: Name=\"") && strstr(line, TARGET_INPUT_NAME)) {
            name_matched = 1;
        }
        else if(strstr(line, "H: Handlers=")) {
            copy_event_name(line, event_name, sizeof(event_name));
        }
    }

    if(input_block_ready(name_matched, event_name)) {
        snprintf(path, path_size, "/dev/input/%s", event_name);
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return -1;
}

static int write_all(int fd, const void *buf, size_t len)
{
    const char *p = buf;

    while(len > 0) {
        ssize_t n = write(fd, p, len);
        if(n < 0) {
            if(errno == EINTR) {
                continue;
            }
            return -1;
        }
        p += n;
        len -= (size_t)n;
    }

    return 0;
}

static int write_text_file(const char *path, const char *value)
{
    int fd = open(path, O_WRONLY);

    if(fd < 0) {
        perror(path);
        return -1;
    }

    if(write_all(fd, value, strlen(value)) < 0) {
        perror(path);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static int write_uint_file(const char *path, unsigned int value)
{
    char text[32];

    snprintf(text, sizeof(text), "%u", value);
    return write_text_file(path, text);
}

static int backlight_pwm_export(void)
{
    char path[128];
    char pwm_path[128];

    snprintf(pwm_path, sizeof(pwm_path), "%s/pwm%s",
             BACKLIGHT_PWM_CHIP, BACKLIGHT_PWM_CHANNEL);
    if(access(pwm_path, F_OK) == 0) {
        return 0;
    }

    snprintf(path, sizeof(path), "%s/export", BACKLIGHT_PWM_CHIP);
    if(write_text_file(path, BACKLIGHT_PWM_CHANNEL) < 0) {
        if(errno == EBUSY) {
            return 0;
        }
        return -1;
    }

    usleep(100000);
    return 0;
}

static int backlight_pwm_enable(int enable)
{
    char path[128];

    snprintf(path, sizeof(path), "%s/pwm%s/enable",
             BACKLIGHT_PWM_CHIP, BACKLIGHT_PWM_CHANNEL);
    return write_uint_file(path, enable ? 1U : 0U);
}

static int backlight_pwm_set_period(unsigned int period_ns)
{
    char path[128];

    snprintf(path, sizeof(path), "%s/pwm%s/period",
             BACKLIGHT_PWM_CHIP, BACKLIGHT_PWM_CHANNEL);
    return write_uint_file(path, period_ns);
}

static int backlight_pwm_set_duty(unsigned int duty_ns)
{
    char path[128];

    snprintf(path, sizeof(path), "%s/pwm%s/duty_cycle",
             BACKLIGHT_PWM_CHIP, BACKLIGHT_PWM_CHANNEL);
    return write_uint_file(path, duty_ns);
}

static void keyboard_backlight_set(int enabled)
{
    unsigned int duty_ns = enabled ? BACKLIGHT_PWM_DUTY_ON : BACKLIGHT_PWM_DUTY_OFF;

    if(backlight_pwm_set_duty(duty_ns) < 0) {
        return;
    }

    (void)backlight_pwm_enable(enabled);
}

static void backlight_pwm_init(void)
{
    if(backlight_pwm_export() < 0) {
        printf("[keyboard_ext] backlight PWM export failed\n");
        return;
    }

    (void)backlight_pwm_enable(0);
    (void)backlight_pwm_set_duty(0);

    if(backlight_pwm_set_period(BACKLIGHT_PWM_PERIOD_NS) < 0) {
        printf("[keyboard_ext] backlight PWM period setup failed\n");
        return;
    }

    if(backlight_pwm_set_duty(BACKLIGHT_PWM_DUTY_ON) < 0) {
        printf("[keyboard_ext] backlight PWM duty setup failed\n");
        return;
    }

    if(backlight_pwm_enable(1) < 0) {
        printf("[keyboard_ext] backlight PWM enable failed\n");
        return;
    }

    printf("[keyboard_ext] backlight PWM: %s/pwm%s, period %u ns\n",
           BACKLIGHT_PWM_CHIP, BACKLIGHT_PWM_CHANNEL, BACKLIGHT_PWM_PERIOD_NS);
}

static int handle_fn_action_key(int code, int value)
{
    if(!fn_down || value != 1) {
        return 0;
    }

    switch(code) {
    case KEY_F4:
        keyboard_backlight_set(0);
        return 1;
    case KEY_F5:
        keyboard_backlight_set(1);
        return 1;
    default:
        return 0;
    }
}

static uint32_t alpha_key(int code)
{
 
    int upper;
    char ch;

    switch(code) {
    case KEY_A: ch = 'a'; break;
    case KEY_B: ch = 'b'; break;
    case KEY_C: ch = 'c'; break;
    case KEY_D: ch = 'd'; break;
    case KEY_E: ch = 'e'; break;
    case KEY_F: ch = 'f'; break;
    case KEY_G: ch = 'g'; break;
    case KEY_H: ch = 'h'; break;
    case KEY_I: ch = 'i'; break;
    case KEY_J: ch = 'j'; break;
    case KEY_K: ch = 'k'; break;
    case KEY_L: ch = 'l'; break;
    case KEY_M: ch = 'm'; break;
    case KEY_N: ch = 'n'; break;
    case KEY_O: ch = 'o'; break;
    case KEY_P: ch = 'p'; break;
    case KEY_Q: ch = 'q'; break;
    case KEY_R: ch = 'r'; break;
    case KEY_S: ch = 's'; break;
    case KEY_T: ch = 't'; break;
    case KEY_U: ch = 'u'; break;
    case KEY_V: ch = 'v'; break;
    case KEY_W: ch = 'w'; break;
    case KEY_X: ch = 'x'; break;
    case KEY_Y: ch = 'y'; break;
    case KEY_Z: ch = 'z'; break;
    default:
        return 0;
    }

    upper = shift_down ^ caps_on;
    if(upper) {
        ch = (char)(ch + 'A' - 'a');
    }

    return (uint32_t)ch;
}

static uint32_t number_key(int code)
{
    if(shift_down) {
        switch(code) {
        case KEY_1: return '!';
        case KEY_2: return '@';
        case KEY_3: return '#';
        case KEY_4: return '$';
        case KEY_5: return '%';
        case KEY_6: return '^';
        case KEY_7: return '&';
        case KEY_8: return '*';
        case KEY_9: return '(';
        case KEY_0: return ')';
        default: return 0;
        }
    }

    if(code >= KEY_1 && code <= KEY_9) {
        return (uint32_t)('1' + (code - KEY_1));
    }

    if(code == KEY_0) {
        return '0';
    }

    return 0;
}

static uint32_t fn_key(int code)
{
    switch(code) {
    case KEY_Q: return '\'';
    case KEY_W: return '-';
    case KEY_E: return '_';
    case KEY_R: return '+';
    case KEY_T: return '=';
    case KEY_Y: return '\\';
    case KEY_U: return '|';
    case KEY_I: return ':';
    case KEY_O: return ';';
    case KEY_P: return '"';
    case KEY_A: return '~';
    case KEY_S: return '[';
    case KEY_D: return ']';
    case KEY_F: return '{';
    case KEY_G: return '}';
    case KEY_H: return '\'';
    case KEY_J: return '`';
    case KEY_K: return '/';
    case KEY_L: return '?';
    case KEY_B: return '.';
    case KEY_N: return '<';
    case KEY_M: return '>';
    case KEY_DELETE: return LV_KEY_BACKSPACE;
    default: return 0;
    }
}

static uint32_t normal_key(int code)
{
    uint32_t key;

    key = alpha_key(code);
    if(key) {
        return key;
    }

    key = number_key(code);
    if(key) {
        return key;
    }

    switch(code) {
    case KEY_SPACE: return ' ';
    case KEY_ENTER: return LV_KEY_ENTER;
    case KEY_ESC: return LV_KEY_ESC;
    case KEY_BACKSPACE: return LV_KEY_BACKSPACE;
    case KEY_DELETE: return LV_KEY_DEL;
    case KEY_TAB: return shift_down ? LV_KEY_PREV : LV_KEY_NEXT;
    case KEY_LEFT: return LV_KEY_LEFT;
    case KEY_RIGHT: return LV_KEY_RIGHT;
    case KEY_UP: return LV_KEY_UP;
    case KEY_DOWN: return LV_KEY_DOWN;
    case KEY_MINUS: return shift_down ? '_' : '-';
    case KEY_EQUAL: return shift_down ? '+' : '=';
    case KEY_LEFTBRACE: return shift_down ? '{' : '[';
    case KEY_RIGHTBRACE: return shift_down ? '}' : ']';
    case KEY_BACKSLASH: return shift_down ? '|' : '\\';
    case KEY_SEMICOLON: return shift_down ? ':' : ';';
    case KEY_APOSTROPHE: return shift_down ? '"' : '\'';
    case KEY_GRAVE: return shift_down ? '~' : '`';
    case KEY_COMMA: return shift_down ? '<' : ',';
    case KEY_DOT: return shift_down ? '>' : '.';
    case KEY_SLASH: return shift_down ? '?' : '/';
    default: return 0;
    }
}

static uint32_t translate_key(int code)
{
    uint32_t key;

    if(fn_down) {
        key = fn_key(code);
        if(key) {
            return key;
        }
    }

    return normal_key(code);
}

int keyboard_ext_init(void)
{
    if(kb_fd >= 0) {
        return 0;
    }

    if(find_tca8418_event(kb_device_path, sizeof(kb_device_path)) < 0) {
        snprintf(kb_device_path, sizeof(kb_device_path), "%s", FALLBACK_INPUT_DEV);
    }

    kb_fd = open(kb_device_path, O_RDONLY | O_NONBLOCK);
    if(kb_fd < 0) {
        perror("open keyboard input");
        return -1;
    }

    printf("[keyboard_ext] input device: %s\n", kb_device_path);
    backlight_pwm_init();
    return 0;
}

void keyboard_ext_deinit(void)
{
    keyboard_backlight_set(0);

    if(kb_fd >= 0) {
        close(kb_fd);
        kb_fd = -1;
    }
}

const char *keyboard_ext_get_device_path(void)
{
    return kb_device_path;
}

void keyboard_ext_read(lv_indev_data_t *data)
{
    struct input_event ev;

    data->key = last_key;
    data->state = last_state;

    if(kb_fd < 0) {
        return;
    }

    while(read(kb_fd, &ev, sizeof(ev)) == sizeof(ev)) {
        uint32_t key;

        if(ev.type != EV_KEY) {
            continue;
        }

        if(ev.code == KEY_LEFTSHIFT || ev.code == KEY_RIGHTSHIFT) {
            shift_down = ev.value != 0;
            continue;
        }

        if(ev.code == KEY_FN) {
            fn_down = ev.value != 0;
            continue;
        }

        if(ev.code == KEY_CAPSLOCK && ev.value == 1) {
            caps_on = !caps_on;
            continue;
        }

        if(handle_fn_action_key(ev.code, ev.value)) {
            continue;
        }

        key = translate_key(ev.code);
        if(!key) {
            continue;
        }

        if(ev.value == 1 || ev.value == 2) {
            last_key = key;
            last_state = LV_INDEV_STATE_PR;
            data->key = last_key;
            data->state = last_state;
            return;
        }

        if(ev.value == 0) {
            last_key = key;
            last_state = LV_INDEV_STATE_REL;
            data->key = last_key;
            data->state = last_state;
            return;
        }
    }
}

