/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

/*********************
 *      DEFINES
 *********************/
static int fb_fd = -1;
static uint8_t *fb_base = NULL;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static lv_disp_drv_t disp_drv;

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//        const lv_area_t * fill_area, lv_color_t color);
void lcd_show_color(const lv_area_t * area, void * px_map_color);
/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*-----------------------------
     * Create a buffer for drawing
     *----------------------------*/

    /**
     * LVGL requires a buffer where it internally draws the widgets.
     * Later this buffer will passed to your display driver's `flush_cb` to copy its content to your display.
     * The buffer has to be greater than 1 display row
     *
     * There are 3 buffering configurations:
     * 1. Create ONE buffer:
     *      LVGL will draw the display's content here and writes it to your display
     *
     * 2. Create TWO buffer:
     *      LVGL will draw the display's content to a buffer and writes it your display.
     *      You should use DMA to write the buffer's content to the display.
     *      It will enable LVGL to draw the next part of the screen to the other buffer while
     *      the data is being sent form the first buffer. It makes rendering and flushing parallel.
     *
     * 3. Double buffering
     *      Set 2 screens sized buffers and set disp_drv.full_refresh = 1.
     *      This way LVGL will always provide the whole rendered screen in `flush_cb`
     *      and you only need to change the frame buffer's address.
     */

    /* Example for 1) */
    //static lv_disp_draw_buf_t draw_buf_dsc_1;
    //static lv_color_t buf_1[MY_DISP_HOR_RES * 10];                          /*A buffer for 10 rows*/
    //lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, MY_DISP_HOR_RES * 10);   /*Initialize the display buffer*/

    /* Example for 2) */
    //static lv_disp_draw_buf_t draw_buf_dsc_2;
    //static lv_color_t buf_2_1[MY_DISP_HOR_RES * 10];                        /*A buffer for 10 rows*/
   // static lv_color_t buf_2_2[MY_DISP_HOR_RES * 10];                        /*An other buffer for 10 rows*/
    //lv_disp_draw_buf_init(&draw_buf_dsc_2, buf_2_1, buf_2_2, MY_DISP_HOR_RES * 10);   /*Initialize the display buffer*/

    /* Example for 3) also set disp_drv.full_refresh = 1 below*/
    //static lv_disp_draw_buf_t draw_buf_dsc_3;
    //static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*A screen sized buffer*/
    //static lv_color_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*Another screen sized buffer*/
   // lv_disp_draw_buf_init(&draw_buf_dsc_3, buf_3_1, buf_3_2,MY_DISP_HOR_RES * MY_DISP_VER_RES);   /*Initialize the display buffer*/
   
    // 分配LVGL显示缓冲区（使用1/10屏幕大小的行缓冲区，性能最佳）
    static lv_disp_draw_buf_t disp_buf;
    uint32_t buf_size = vinfo.xres * vinfo.yres / 10;
    static lv_color_t *buf1 = NULL;
    static lv_color_t *buf2 = NULL;

    buf1 = malloc(buf_size * sizeof(lv_color_t));
    buf2 = malloc(buf_size * sizeof(lv_color_t));
    if (!buf1 || !buf2) {
        fprintf(stderr, "无法分配显示缓冲区\n");
        exit(EXIT_FAILURE);
    }

    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, buf_size);
    
    
    // static lv_disp_draw_buf_t draw_buf_dsc_3;
   // static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*A screen sized buffer*/
   // static lv_color_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*Another screen sized buffer*/
   // lv_disp_draw_buf_init(&draw_buf_dsc_3, buf_3_1, buf_3_2,MY_DISP_HOR_RES * MY_DISP_VER_RES)
    
    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

   // 注册显示驱动
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.hor_res = vinfo.xres;
    disp_drv.ver_res = vinfo.yres;

    lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    /*You code here*/
    // 打开帧缓冲设备
    fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd < 0) {
        perror("无法打开/dev/fb0");
        exit(EXIT_FAILURE);
    }

    // 获取屏幕信息
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("无法获取可变屏幕信息");
        exit(EXIT_FAILURE);
    }

    if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        perror("无法获取固定屏幕信息");
        exit(EXIT_FAILURE);
    }

    // 打印屏幕信息（调试用）
    printf("屏幕信息:\n");
    printf("  分辨率: %dx%d\n", vinfo.xres, vinfo.yres);
    printf("  虚拟分辨率: %dx%d\n", vinfo.xres_virtual, vinfo.yres_virtual);
    printf("  颜色深度: %d位\n", vinfo.bits_per_pixel);
    printf("  行长度: %d字节\n", finfo.line_length);

    // 映射帧缓冲到用户空间
    size_t fb_size = finfo.line_length * vinfo.yres_virtual;
    fb_base = mmap(NULL, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fb_base == MAP_FAILED) {
        perror("无法映射帧缓冲");
        exit(EXIT_FAILURE);
    }

    // 清空屏幕
    memset(fb_base, 0, fb_size);
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

void lcd_show_color(const lv_area_t * area, void * px_map_color)
{
printf("lcd_show_color,x1:%d,y1:%d,x2:%d,y2:%d\n",area->x1,area->y1,area->x2,area->y2);
}





/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
static void disp_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p)
{
//printf("disp_flush,%d\n",disp_flush_enabled==true?1:0);
    if(disp_flush_enabled) {
        /*The most simple case (but also the slowest) to put all pixels to the screen one-by-one*/
        uint32_t width = area->x2 - area->x1 + 1;
        uint32_t height = area->y2 - area->y1 + 1;

     uint8_t *dest = fb_base +
                        (area->y1 + vinfo.yoffset) * finfo.line_length +
                        (area->x1 + vinfo.xoffset) * 2;

        for (uint32_t y = 0; y < height; y++) {
            memcpy(dest, color_p, width * 2);
            dest     += finfo.line_length;
            color_p  += width;
        }
    }
    else
    {
    printf("disp_flush_enabled,false\n");
    }

    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
    lv_disp_flush_ready(drv);
}

void lv_port_disp_deinit(void) {
    if (fb_base) munmap(fb_base, finfo.line_length * vinfo.yres_virtual);
    if (fb_fd >= 0) close(fb_fd);
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
