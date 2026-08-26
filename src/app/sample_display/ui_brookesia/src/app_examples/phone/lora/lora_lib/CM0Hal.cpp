#include "CM0Hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <linux/gpio.h>
int fd_spi0;
uint32_t spi_speed = 1000000;
#define SPI_DEV    "/dev/spidev0.0"
#define GPIO_CHIP  "/dev/gpiochip0"
#define PIN_RST    7
#define PIN_BUSY   24

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

#define GPIO_READ_VALUE       	_IOW('G', 12, int)
/* ── GPIO 工具 ────────────────────────────────────────────── */
static int open_output(int chip, unsigned offset, const char *label, int init)
{
    struct gpio_v2_line_request r = {0};
    r.offsets[0] = offset; r.num_lines = 1;
    r.config.flags = GPIO_V2_LINE_FLAG_OUTPUT;
    r.config.num_attrs = 1;
    r.config.attrs[0].attr.id     = GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES;
    r.config.attrs[0].attr.values = init;
    r.config.attrs[0].mask        = 1;
    strncpy(r.consumer, label, sizeof(r.consumer) - 1);
    if (ioctl(chip, GPIO_V2_GET_LINE_IOCTL, &r) < 0) { perror(label); return -1; }
    return r.fd;
}

static int open_input(int chip, unsigned offset, const char *label)
{
    struct gpio_v2_line_request r = {0};
    r.offsets[0] = offset; r.num_lines = 1;
    r.config.flags = GPIO_V2_LINE_FLAG_INPUT;
    strncpy(r.consumer, label, sizeof(r.consumer) - 1);
    if (ioctl(chip, GPIO_V2_GET_LINE_IOCTL, &r) < 0) { perror(label); return -1; }
    return r.fd;
}

static void gpio_write(int fd, int val)
{
    struct gpio_v2_line_values v = {  .bits = val ? 1u : 0u,.mask = 1 };
    ioctl(fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &v);
}

static int gpio_read(int fd)
{
    struct gpio_v2_line_values v = {  .bits = 0,.mask = 1 };
    if (ioctl(fd, GPIO_V2_LINE_GET_VALUES_IOCTL, &v) < 0) return -1;
    return (int)(v.bits & 1);
}

/*
 * BUSY 等待（竞态安全版，两步法）：
 *
 * 问题根源（逻辑分析仪已确认）：
 *   Frame1 的 NSS 拉高后，BUSY 响应存在传播延迟（几十 ns ~ 几 µs）。
 *   原代码立即进入轮询，在 BUSY 尚未拉高时读到 0，误判"芯片就绪"，
 *   导致 Frame2 在芯片准备好数据之前就发出，读回数据全为无效值。
 *
 * 修复策略（两步法）：
 *   Step1：先等 BUSY 拉高（wait_high_ms 超时）
 *          — 确认芯片已接收命令，正在处理
 *          — 若超时（脉冲极短已过），跳过不报错，继续 Step2
 *   Step2：再等 BUSY 拉低（timeout_ms 超时）
 *          — 芯片数据准备完毕，可以安全发送 Frame2
 *          — 超时则报错返回 -1
 *
 * 调用约定：
 *   发送前（仅检查低电平）：wait_busy_done(busy_fd, 0, 1000)
 *   Frame1 之后（完整两步）：wait_busy_done(busy_fd, 5, 1000)
 */
static int wait_busy_done(int busy_fd, int wait_high_ms, int timeout_ms)
{
    struct timespec t0, tn;

    /* ── Step 1：等 BUSY 拉高 ─────────────────────────────── */
    clock_gettime(CLOCK_MONOTONIC, &t0);
    while (1) {
        int v = gpio_read(busy_fd);
        if (v < 0)  return -1;
        if (v == 1) break;          /* BUSY 已高，进入 Step2 */

        clock_gettime(CLOCK_MONOTONIC, &tn);
        long ms = (tn.tv_sec  - t0.tv_sec)  * 1000
                + (tn.tv_nsec - t0.tv_nsec) / 1000000;
        if (ms >= wait_high_ms) break;  /* 超时跳过，不报错 */
        /* 紧循环，不加 usleep，捕捉短脉冲 */
    }

    /* ── Step 2：等 BUSY 拉低 ─────────────────────────────── */
    clock_gettime(CLOCK_MONOTONIC, &t0);
    while (1) {
        int v = gpio_read(busy_fd);
        if (v == 0) return 0;       /* BUSY=0，芯片就绪 */
        if (v < 0)  return -1;

        clock_gettime(CLOCK_MONOTONIC, &tn);
        long ms = (tn.tv_sec  - t0.tv_sec)  * 1000
                + (tn.tv_nsec - t0.tv_nsec) / 1000000;
        if (ms >= timeout_ms) {
            fprintf(stderr, "BUSY timeout (%d ms)\n", timeout_ms);
            return -1;
        }
        /* 紧循环，不加 usleep，捕捉短脉冲 */
    }
}

/* ── Status 解码（Table 6-38）────────────────────────────── */
static void print_status(const char *label, uint16_t s)
{
    static const char *cmd[]  = {"CMD_FAIL","CMD_PERR","CMD_OK","CMD_DAT"};
    static const char *mode[] = {"SLEEP","STBY_RC","STBY_XOSC","FS","Rx","Tx"};
    static const char *rst[]  = {"Cleared","POR/BRN","NRESET","RFU"};
    unsigned c = (s>>9)&7, i = (s>>8)&1, r = (s>>4)&0xF, m = s&7;
    printf("%s: 0x%04X  cmd=%-8s  irq=%u  rst=%-7s  mode=%s\n",
           label, s,
           c<4?cmd[c]:"?", i, r<4?rst[r]:"?", m<6?mode[m]:"?");
}

/* ── SPI 单帧传输 ─────────────────────────────────────────── */
static int spi_xfer(int fd, uint32_t speed,
                    const uint8_t *tx, uint8_t *rx, int len)
{
    struct spi_ioc_transfer t = {
        .tx_buf = (unsigned long)tx, .rx_buf = (unsigned long)rx,
        .len = (__u32)len, .speed_hz = speed, .bits_per_word = 8,
    };
    return ioctl(fd, SPI_IOC_MESSAGE(1), &t);
}


//ArduinoHal::ArduinoHal(): RadioLibHal(INPUT, OUTPUT, LOW, HIGH, RISING, FALLING), spi(&RADIOLIB_DEFAULT_SPI), initInterface(true) {}

//ArduinoHal::ArduinoHal(SPIClass& spi, SPISettings spiSettings): RadioLibHal(INPUT, OUTPUT, LOW, HIGH, RISING, FALLING), spi(&spi), spiSettings(spiSettings) {}

//ArduinoHal::ArduinoHal(): RadioLibHal(1, 3, 0, 1, 1, 2), initInterface(true) {}

CM0Hal::CM0Hal():RadioLibHal(GPIO_DM_INPUT, GPIO_DM_OUTPUT, GPIO_WRITE_LOW, GPIO_WRITE_HIGH, GPIO_PE_RISING, GPIO_PE_FALLING) {}

void CM0Hal::init() {
  
    int chip = open(GPIO_CHIP, O_RDWR);
    if (chip < 0) { perror("gpiochip"); return; }

    int rst_fd  = open_output(chip, PIN_RST,  "lora_rst",  1);
    int busy_fd = open_input (chip, PIN_BUSY, "lora_busy");
    if (rst_fd < 0 || busy_fd < 0) return;

    /* ── rst ──────────────────────────────────────────────── */
    gpio_write(rst_fd, 0); usleep(20000);
    gpio_write(rst_fd, 1); usleep(200000);   /* 等芯片启动 */

    /* ── 打开 SPI ──────────────────────────────────────────── */
    fd_spi0 = open(SPI_DEV, O_RDWR);
    if (fd_spi0 < 0) { perror("spi"); return; }
    uint8_t mode = SPI_MODE_0, bits = 8; 
    ioctl(fd_spi0, SPI_IOC_WR_MODE,          &mode);
    ioctl(fd_spi0, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(fd_spi0, SPI_IOC_WR_MAX_SPEED_HZ,  &spi_speed); 
  //test
  #if 0
   /* 发送前确认 BUSY 空闲（跳过 Step1，仅等低电平） */
    if (wait_busy_done(busy_fd, 0, 1000) < 0) return;

    /* Frame 1：3 字节 = Op_H(0x01), Op_L(0x01), Arg0(0x00) */
    uint8_t tx1[2] = {0x01, 0x01}, rx1[2] = {0};
    if (spi_xfer(fd, speed, tx1, rx1, 2) < 0) { perror("frame1"); return; }

    /* Frame1 结束后：两步法等待（先等 BUSY 高最多 5ms，再等拉低最多 1000ms） */
    if (wait_busy_done(busy_fd, 5, 1000) < 0) return;

    /* Frame 2：4 字节，接收 Stat_H Stat_L FWMajor FWMinor */
    uint8_t tx2[4] = {0}, rx2[4] = {0};
    if (spi_xfer(fd, speed, tx2, rx2, 4) < 0) { perror("frame2"); return; }

    /* ── 打印结果 ───────────────────────────────────────────── */
    printf("Frame1 MISO: %02X %02X\n", rx1[0], rx1[1]);
    printf("Frame2 MISO: %02X %02X %02X %02X\n",
           rx2[0], rx2[1], rx2[2], rx2[3]);
    print_status("Frame1 Stat", (uint16_t)(rx1[0]<<8)|rx1[1]);
    print_status("Frame2 Stat", (uint16_t)(rx2[0]<<8)|rx2[1]);
    printf("FW version : v%d.%d (0x%02X.0x%02X)\n",
           rx2[2], rx2[3], rx2[2], rx2[3]);
    
 #endif 
  
  
  
   
}

void CM0Hal::term() {
  //if(initInterface) {
    //spiEnd();
  //}
}

void inline CM0Hal::pinMode(uint32_t pin, uint32_t mode) {
    /*pin_gpio_t pin_opt;
    pin_opt.pin=pin;
    ioctl(gpio_fd, mode, &pin_opt);*/
}

void inline CM0Hal::digitalWrite(uint32_t pin, uint32_t value) {
    /*pin_gpio_t pin_opt;
    pin_opt.pin=pin;
    if(value==GPIO_WRITE_HIGH)
    {
    ioctl(gpio_fd, GPIO_WRITE_HIGH, &pin_opt);
    }
    else if(value==GPIO_WRITE_LOW)
    {
    ioctl(gpio_fd, GPIO_WRITE_LOW, &pin_opt);
    }*/
}

uint32_t inline CM0Hal::digitalRead(uint32_t pin) {
    /*pin_gpio_t pin_opt;
    pin_opt.pin=pin;
    ioctl(gpio_fd, GPIO_READ_VALUE, &pin_opt);
    return pin_opt.val;*/
    return 0;
}

void inline CM0Hal::attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) {
 //empty
}

void inline CM0Hal::detachInterrupt(uint32_t interruptNum) {
 //empty
}

void inline CM0Hal::delay(RadioLibTime_t ms) {
      usleep(ms*1000);
}

void inline CM0Hal::delayMicroseconds(RadioLibTime_t us) {
usleep(us);//1us
//k230_delay_us(us);
//empty
}

RadioLibTime_t inline CM0Hal::millis() {
//empty
struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (RadioLibTime_t)(ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL);
}

RadioLibTime_t inline CM0Hal::micros() {
//empty
 struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (RadioLibTime_t)(ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000ULL);
}

long inline CM0Hal::pulseIn(uint32_t pin, uint32_t state, RadioLibTime_t timeout) {
 //empty
  return 0;
}

void inline CM0Hal::spiBegin() {

  //spi->begin();
}

void inline CM0Hal::spiBeginTransaction() {
  //spi->beginTransaction(spiSettings);
}
void CM0Hal::spiTransfer(uint8_t* out, size_t len, uint8_t* in) {
    spi_xfer(fd_spi0, spi_speed, out, in, len);
}

void inline CM0Hal::spiEndTransaction() {
}

void inline CM0Hal::spiEnd() {
}

void inline CM0Hal::tone(uint32_t pin, unsigned int frequency, RadioLibTime_t duration) {
  
}

void inline CM0Hal::noTone(uint32_t pin) {
 
}

void inline CM0Hal::yield() {
  #if !defined(RADIOLIB_YIELD_UNSUPPORTED)
  //::yield();
  #endif
  //rt_thread_yield();
  //k230_delay_us(1000);
  usleep(1000);
}

uint32_t inline CM0Hal::pinToInterrupt(uint32_t pin) {
  return 0;
}
