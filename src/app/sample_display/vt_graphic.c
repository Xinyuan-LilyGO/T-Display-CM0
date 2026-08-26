#include <sys/ioctl.h>
#include <linux/kd.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include "lvgl/lvgl.h"
#include "vt_graphic.h"
static int tty_fd = -1;


#include <linux/kd.h>
#include <linux/vt.h>
static volatile sig_atomic_t need_redraw = 0;
static volatile sig_atomic_t need_exit = 0;
// 恢复终端为文本模式，程序退出/被杀时自动调用
static void vt_deinit(void)
{
    if (tty_fd > 0) {
        ioctl(tty_fd, KDSETMODE, KD_TEXT);
        close(tty_fd);
        tty_fd = -1;
    }
}
#if 0
// 切回 tty1 时触发：强制全屏重绘 LVGL 界面，解决切回黑屏
static void vt_enter_handler(int sig)
{
    (void)sig;
    lv_obj_invalidate(lv_scr_act());  // 标记全屏为脏区
    lv_refr_now(NULL);                // 立即执行一次完整刷新
}
#endif


static void sig_exit_handler(int sig)
{
    (void)sig;
    need_exit = 1;
}

static void vt_acquire_handler(int sig)
{
    (void)sig;

    if (tty_fd >= 0) {
        ioctl(tty_fd, VT_RELDISP, VT_ACKACQ);
    }

    need_redraw = 1;
}

static void vt_release_handler(int sig)
{
    (void)sig;

    if (tty_fd >= 0) {
        ioctl(tty_fd, VT_RELDISP, 1);
    }
}



/**
 * @brief 终端图形模式初始化
 * 调用时机：lv_init() + 显示驱动初始化完成之后，主循环启动之前
 */
void vt_graphics_init(void)
{
    tty_fd = open("/dev/tty1", O_RDWR);
    if (tty_fd < 0) {
        perror("open /dev/tty1 failed");
        return;
    }

    // 核心：设置终端为图形模式，内核停止所有文本渲染
    // 彻底解决内核日志、服务打印、光标闪烁叠在界面上的问题
    ioctl(tty_fd, KDSETMODE, KD_GRAPHICS);
    /*struct vt_mode mode = {0};
    mode.mode = VT_PROCESS;
    mode.relsig = SIGUSR1;
    mode.acqsig = SIGUSR2;

    if (ioctl(tty_fd, VT_SETMODE, &mode) < 0) {
        perror("VT_SETMODE failed");
    }*/
    // 注册退出保护：正常退出、Ctrl+C、systemctl stop 时自动恢复文本模式
    atexit(vt_deinit);
    signal(SIGTERM, sig_exit_handler);
    signal(SIGINT, sig_exit_handler);
    //signal(SIGUSR1, vt_release_handler);
    //signal(SIGUSR2, vt_acquire_handler);//tty no switch
    
   /* while (1) {
    lv_timer_handler();

    if (vt_need_redraw) {
        vt_need_redraw = 0;
        lv_obj_invalidate(lv_scr_act());
    }

    if (vt_need_exit) {
        vt_graphics_deinit();
        break;
    }

    usleep(5000);
}*/
    
    
    
    
   #if 0 
    signal(SIGTERM, (void (*)(int))vt_deinit);
    signal(SIGINT,  (void (*)(int))vt_deinit);

    // 注册切屏信号：切回 tty1 时自动重绘画面
    signal(SIGUSR2, vt_enter_handler);
   #endif
}
