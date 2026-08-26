#pragma once
#include <cstdint>

/**
 * 摄像头 DRM 预览配置
 * 所有字段均有默认值，业务层只改需要的参数即可
 */
struct PreviewConfig {
    uint32_t cam_w    = 1920;  // 摄像头采集宽度
    uint32_t cam_h    = 1080;  // 摄像头采集高度
    int      disp_x   = 0;     // 显示区域左上角 x（屏幕像素坐标）
    int      disp_y   = 0;     // 显示区域左上角 y
    uint32_t disp_w   = 560;   // 显示区域宽度（硬件缩放到此尺寸）
    uint32_t disp_h   = 992;   // 显示区域高度
    uint32_t rotation = 0;     // 旋转角度：0 / 90 / 180 / 270
                                // 注意：90/270 时图像宽高互换，
                                //       建议同步交换 disp_w / disp_h
    // 源图像裁剪区域（16.16 定点，0=使用全图，由 preview_fill 自动计算）
    uint32_t src_x = 0;
    uint32_t src_y = 0;
    uint32_t src_w = 0;
    uint32_t src_h = 0;
};
static inline PreviewConfig preview_fit(
    uint32_t cam_w, uint32_t cam_h,
    int area_x, int area_y, uint32_t area_w, uint32_t area_h,
    uint32_t rotation = 0)
{
    uint32_t src_w = (rotation == 90 || rotation == 270) ? cam_h : cam_w;
    uint32_t src_h = (rotation == 90 || rotation == 270) ? cam_w : cam_h;
    uint32_t fit_w = area_w;
    uint32_t fit_h = fit_w * src_h / src_w;
    if (fit_h > area_h) { fit_h = area_h; fit_w = fit_h * src_w / src_h; }
    PreviewConfig cfg;
    cfg.cam_w = cam_w; cfg.cam_h = cam_h; cfg.rotation = rotation;
    cfg.disp_w = fit_w; cfg.disp_h = fit_h;
    cfg.disp_x = area_x + (int)(area_w - fit_w) / 2;
    cfg.disp_y = area_y + (int)(area_h - fit_h) / 2;
    return cfg;
}

static inline PreviewConfig preview_fill(
    uint32_t cam_w, uint32_t cam_h,
    int disp_x, int disp_y, uint32_t disp_w, uint32_t disp_h,
    uint32_t rotation = 0)
{
    uint32_t src_w = (rotation == 90 || rotation == 270) ? cam_h : cam_w;
    uint32_t src_h = (rotation == 90 || rotation == 270) ? cam_w : cam_h;
    uint32_t crop_w = src_w;
    uint32_t crop_h = crop_w * disp_h / disp_w;
    if (crop_h > src_h) { crop_h = src_h; crop_w = crop_h * disp_w / disp_h; }
    PreviewConfig cfg;
    cfg.cam_w = cam_w; cfg.cam_h = cam_h; cfg.rotation = rotation;
    cfg.disp_x = disp_x; cfg.disp_y = disp_y;
    cfg.disp_w = disp_w; cfg.disp_h = disp_h;
    cfg.src_x = ((src_w - crop_w) / 2) << 16;
    cfg.src_y = ((src_h - crop_h) / 2) << 16;
    cfg.src_w = crop_w << 16;
    cfg.src_h = crop_h << 16;
    return cfg;
}

/**
 * 开启摄像头预览
 *  - 初始化 DRM、libcamera
 *  - 开始持续推帧到屏幕
 * @return  0  成功
 *         <0  失败（错误已打印到 stderr）
 */
int preview_open(const PreviewConfig &cfg = {});

/**
 * 关闭摄像头预览
 *  - 停止推帧
 *  - 释放所有 DRM / libcamera 资源
 */
void preview_close();
