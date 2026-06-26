/*
 * LR2021 SPI 驱动 - GetVersion 示例（含 BUSY 竞态修复）
 *
 * 修复内容：
 *   1. wait_busy_done：先等 BUSY 拉高，再等拉低，避免竞态条件
 *   2. Frame1：按数据手册 5.4.1.2 补齐 Arg0，改为 3 字节
 *
 * 引脚：RST=GPIO7  BUSY=GPIO24  CS=GPIO8
 *       SCLK=GPIO11  MOSI=GPIO10  MISO=GPIO9
 * 编译：gcc -O2 -o lora lr2021_spi_fixed.c
 */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <linux/gpio.h>

#define SPI_DEV    "/dev/spidev0.0"
#define GPIO_CHIP  "/dev/gpiochip0"
#define PIN_RST    7
#define PIN_BUSY   24

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
    struct gpio_v2_line_values v = { .mask = 1, .bits = val ? 1u : 0u };
    ioctl(fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &v);
}

static int gpio_read(int fd)
{
    struct gpio_v2_line_values v = { .mask = 1, .bits = 0 };
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
        .len = len, .speed_hz = speed, .bits_per_word = 8,
    };
    return ioctl(fd, SPI_IOC_MESSAGE(1), &t);
}

/* ══════════════════════════════════════════════════════════ */
int main(void)
{
    int chip = open(GPIO_CHIP, O_RDWR);
    if (chip < 0) { perror("gpiochip"); return -1; }

    int rst_fd  = open_output(chip, PIN_RST,  "lora_rst",  1);
    int busy_fd = open_input (chip, PIN_BUSY, "lora_busy");
    if (rst_fd < 0 || busy_fd < 0) return -1;

    /* ── 复位 ──────────────────────────────────────────────── */
    gpio_write(rst_fd, 0); usleep(20000);
    gpio_write(rst_fd, 1); usleep(200000);   /* 等芯片启动 */

    /* ── 打开 SPI ──────────────────────────────────────────── */
    int fd = open(SPI_DEV, O_RDWR);
    if (fd < 0) { perror("spi"); return -1; }
    uint8_t mode = SPI_MODE_0, bits = 8; uint32_t speed = 1000000;
    ioctl(fd, SPI_IOC_WR_MODE,          &mode);
    ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ,  &speed);

    /* ── GetVersion 两帧协议（Section 5.4.1.2）────────────────
     *
     * Frame1  MOSI: Op[15:8]=0x01, Op[7:0]=0x01, Arg0=0x00  (3字节)
     *         NSS 拉高 → BUSY 拉高 → BUSY 拉低（芯片就绪）
     * Frame2  MOSI: 0x00 × 4
     *         MISO: Stat_H  Stat_L  FWMajor  FWMinor
     *
     * 修复：Frame1 由原来的 2 字节改为 3 字节（补齐 Arg0）。
     * ───────────────────────────────────────────────────────── */

    /* 发送前确认 BUSY 空闲（跳过 Step1，仅等低电平） */
    if (wait_busy_done(busy_fd, 0, 1000) < 0) goto out;

    /* Frame 1：3 字节 = Op_H(0x01), Op_L(0x01), Arg0(0x00) */
    uint8_t tx1[2] = {0x01, 0x01}, rx1[2] = {0};
    if (spi_xfer(fd, speed, tx1, rx1, 2) < 0) { perror("frame1"); goto out; }

    /* Frame1 结束后：两步法等待（先等 BUSY 高最多 5ms，再等拉低最多 1000ms） */
    if (wait_busy_done(busy_fd, 5, 1000) < 0) goto out;

    /* Frame 2：4 字节，接收 Stat_H Stat_L FWMajor FWMinor */
    uint8_t tx2[4] = {0}, rx2[4] = {0};
    if (spi_xfer(fd, speed, tx2, rx2, 4) < 0) { perror("frame2"); goto out; }

    /* ── 打印结果 ───────────────────────────────────────────── */
    printf("Frame1 MISO: %02X %02X\n", rx1[0], rx1[1]);
    printf("Frame2 MISO: %02X %02X %02X %02X\n",
           rx2[0], rx2[1], rx2[2], rx2[3]);
    print_status("Frame1 Stat", (uint16_t)(rx1[0]<<8)|rx1[1]);
    print_status("Frame2 Stat", (uint16_t)(rx2[0]<<8)|rx2[1]);
    printf("FW version : v%d.%d (0x%02X.0x%02X)\n",
           rx2[2], rx2[3], rx2[2], rx2[3]);

out:
    close(fd);
    close(busy_fd);
    close(rst_fd);
    close(chip);
    return 0;
}
