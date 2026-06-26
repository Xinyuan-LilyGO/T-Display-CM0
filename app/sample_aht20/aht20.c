#define _DEFAULT_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#define AHT20_ADDR      0x38
#define I2C_DEV_PATH    "/dev/i2c-1"

static int i2c_fd = -1;

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

int main(void)
{
    float t, h;
    if(aht20_init() < 0)
    {
        printf("AHT20 init fail\n");
        return -1;
    }

    while(1)
    {
        if(0 == aht20_read(&t, &h))
        {
            printf("Temp:%.2f ℃  Humidity:%.2f %%RH\n", t, h);
        }
        else
        {
            printf("read err\n");
        }
        sleep(1);
    }

    aht20_deinit();
    return 0;
}
