// preview.cpp — DRM + libcamera 摄像头预览实现
// 业务层只需 #include "preview.h" 并调用 preview_open / preview_close

#include "preview.h"

#include <libcamera/libcamera.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/stream.h>
#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/request.h>
#include <libcamera/controls.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <drm/drm_fourcc.h>

using namespace libcamera;

#ifndef DRM_MODE_CONNECTED
#define DRM_MODE_CONNECTED 1
#endif

// ===================== 模块内部状态 =====================
static PreviewConfig  g_cfg;
static bool           g_running    = false;
static bool           g_use_nv12   = false;
static uint32_t       g_stride     = 0;
static uint32_t       g_cur_fb     = 0;
static uint32_t       g_rot_prop   = 0;
static uint32_t       g_buf_w      = 0;   // libcamera 实际输出宽（已含旋转）
static uint32_t       g_buf_h      = 0;   // libcamera 实际输出高（已含旋转）

// DRM
static int      drm_fd       = -1;
static uint32_t drm_crtc     = 0;
static uint32_t drm_conn     = 0;
static uint32_t drm_yuv_plane = 0;

// libcamera
static CameraManager                    *g_cam_mgr     = nullptr;
static std::shared_ptr<Camera>           g_camera;
static std::unique_ptr<FrameBufferAllocator> g_alloc;
static Stream                           *g_view_stream  = nullptr;
static std::vector<std::unique_ptr<Request>> g_req_list;

// ===================== DRM 底层封装 =====================
static int drm_ioctl(int fd, unsigned long req, void *arg)
{
    return ioctl(fd, req, arg);
}

static drm_mode_card_res *drm_get_resources(int fd)
{
    drm_mode_card_res res{};
    if (drm_ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res) < 0) return nullptr;
    size_t sz = sizeof(drm_mode_card_res) +
        (res.count_connectors + res.count_encoders + res.count_crtcs) * sizeof(uint32_t);
    auto *ret = (drm_mode_card_res*)calloc(1, sz);
    ret->count_connectors = res.count_connectors;
    ret->count_encoders   = res.count_encoders;
    ret->count_crtcs      = res.count_crtcs;
    uint32_t *p = (uint32_t*)(ret + 1);
    ret->connector_id_ptr = (uint64_t)p; p += res.count_connectors;
    ret->encoder_id_ptr   = (uint64_t)p; p += res.count_encoders;
    ret->crtc_id_ptr      = (uint64_t)p;
    if (drm_ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, ret) < 0) { free(ret); return nullptr; }
    return ret;
}

static drm_mode_get_connector *drm_get_connector(int fd, uint32_t conn_id)
{
    drm_mode_get_connector conn{};
    conn.connector_id = conn_id;
    if (drm_ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn) < 0) return nullptr;
    size_t sz = sizeof(drm_mode_get_connector) +
        conn.count_modes    * sizeof(drm_mode_modeinfo) +
        conn.count_props    * (sizeof(uint32_t) + sizeof(uint64_t)) +
        conn.count_encoders * sizeof(uint32_t);
    auto *ret = (drm_mode_get_connector*)calloc(1, sz);
    *ret = conn;
    uint8_t *p = (uint8_t*)(ret + 1);
    ret->modes_ptr       = (uint64_t)p; p += conn.count_modes    * sizeof(drm_mode_modeinfo);
    ret->props_ptr       = (uint64_t)p; p += conn.count_props    * sizeof(uint32_t);
    ret->prop_values_ptr = (uint64_t)p; p += conn.count_props    * sizeof(uint64_t);
    ret->encoders_ptr    = (uint64_t)p;
    if (drm_ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, ret) < 0) { free(ret); return nullptr; }
    return ret;
}

static drm_mode_get_plane_res *drm_get_plane_res(int fd)
{
    drm_mode_get_plane_res res{};
    if (drm_ioctl(fd, DRM_IOCTL_MODE_GETPLANERESOURCES, &res) < 0) return nullptr;
    size_t sz = sizeof(drm_mode_get_plane_res) + res.count_planes * sizeof(uint32_t);
    auto *ret = (drm_mode_get_plane_res*)calloc(1, sz);
    ret->count_planes = res.count_planes;
    ret->plane_id_ptr = (uint64_t)(ret + 1);
    if (drm_ioctl(fd, DRM_IOCTL_MODE_GETPLANERESOURCES, ret) < 0) { free(ret); return nullptr; }
    return ret;
}

static drm_mode_get_plane *drm_get_plane(int fd, uint32_t plane_id)
{
    size_t sz = sizeof(drm_mode_get_plane) + 64 * sizeof(uint32_t);
    auto *plane = (drm_mode_get_plane*)calloc(1, sz);
    plane->plane_id           = plane_id;
    plane->count_format_types = 64;
    plane->format_type_ptr    = (uint64_t)(plane + 1);
    if (drm_ioctl(fd, DRM_IOCTL_MODE_GETPLANE, plane) < 0) { free(plane); return nullptr; }
    return plane;
}

static int drm_prime_fd_to_handle(int fd, int prime_fd, uint32_t *handle)
{
    drm_prime_handle arg{};
    arg.fd = prime_fd;
    if (drm_ioctl(fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &arg) < 0) return -1;
    *handle = arg.handle;
    return 0;
}

static int drm_add_fb2(int fd, drm_mode_fb_cmd2 *fb_cmd)
{
    return drm_ioctl(fd, DRM_IOCTL_MODE_ADDFB2, fb_cmd);
}

static int drm_set_plane_ex(int fd, uint32_t plane_id, uint32_t crtc_id,
    uint32_t fb_id, uint32_t flags,
    int32_t crtc_x, int32_t crtc_y, uint32_t crtc_w, uint32_t crtc_h,
    uint32_t src_x, uint32_t src_y, uint32_t src_w, uint32_t src_h)
{
    drm_mode_set_plane set{};
    set.plane_id = plane_id; set.crtc_id = crtc_id; set.fb_id = fb_id; set.flags = flags;
    set.crtc_x = crtc_x; set.crtc_y = crtc_y; set.crtc_w = crtc_w; set.crtc_h = crtc_h;
    set.src_x = src_x; set.src_y = src_y; set.src_w = src_w; set.src_h = src_h;
    return drm_ioctl(fd, DRM_IOCTL_MODE_SETPLANE, &set);
}

static int drm_rm_fb(int fd, uint32_t fb_id)
{
    return drm_ioctl(fd, DRM_IOCTL_MODE_RMFB, &fb_id);
}

// ===================== 旋转支持 =====================
// 角度 → DRM 旋转掩码（drm_mode.h 中的 DRM_MODE_ROTATE_x）
static uint32_t deg_to_drm_rot(uint32_t deg)
{
    switch (deg) {
    case 90:  return (1u << 1);   // DRM_MODE_ROTATE_90
    case 180: return (1u << 2);   // DRM_MODE_ROTATE_180
    case 270: return (1u << 3);   // DRM_MODE_ROTATE_270
    default:  return (1u << 0);   // DRM_MODE_ROTATE_0
    }
}

// 在 plane 的属性列表里按名称查找属性 ID
static uint32_t find_plane_prop(uint32_t plane_id, const char *name)
{
    // 第一次调用：获取属性数量
    drm_mode_obj_get_properties q{};
    q.obj_id   = plane_id;
    q.obj_type = DRM_MODE_OBJECT_PLANE;
    if (ioctl(drm_fd, DRM_IOCTL_MODE_OBJ_GETPROPERTIES, &q) < 0) return 0;

    uint32_t n = q.count_props;
    auto *ids  = (uint32_t*)calloc(n, sizeof(uint32_t));
    auto *vals = (uint64_t*)calloc(n, sizeof(uint64_t));
    q.props_ptr       = (uint64_t)ids;
    q.prop_values_ptr = (uint64_t)vals;
    if (ioctl(drm_fd, DRM_IOCTL_MODE_OBJ_GETPROPERTIES, &q) < 0) {
        free(ids); free(vals); return 0;
    }
    uint32_t found = 0;
    for (uint32_t i = 0; i < n; i++) {
        drm_mode_get_property p{};
        p.prop_id = ids[i];
        if (ioctl(drm_fd, DRM_IOCTL_MODE_GETPROPERTY, &p) < 0) continue;
        if (strcmp(p.name, name) == 0) { found = ids[i]; break; }
    }
    free(ids); free(vals);
    return found;
}

// 将旋转属性写入 plane
static void apply_rotation()
{
    if (!g_rot_prop) { fprintf(stderr, "[preview] plane has no rotation property\n"); return; }
    drm_mode_obj_set_property s{};
    s.obj_id   = drm_yuv_plane;
    s.obj_type = DRM_MODE_OBJECT_PLANE;
    s.prop_id  = g_rot_prop;
    s.value    = deg_to_drm_rot(g_cfg.rotation);
    if (ioctl(drm_fd, DRM_IOCTL_MODE_OBJ_SETPROPERTY, &s) < 0)
        perror("[preview] set rotation");
    else
        fprintf(stderr, "[preview] rotation=%u°\n", g_cfg.rotation);
}

// ===================== DRM 初始化 =====================
static int drm_init()
{
    drm_fd = open("/dev/dri/card0", O_RDWR);
    if (drm_fd < 0) { perror("open /dev/dri/card0"); return -1; }

    // 开启 universal planes，让内核返回完整格式列表
    drm_set_client_cap cap{};
    cap.capability = DRM_CLIENT_CAP_UNIVERSAL_PLANES;
    cap.value = 1;
    ioctl(drm_fd, DRM_IOCTL_SET_CLIENT_CAP, &cap);

    drm_mode_card_res *res = drm_get_resources(drm_fd);
    if (!res) return -1;
    auto *conn_ids = (uint32_t*)res->connector_id_ptr;
    auto *crtc_ids = (uint32_t*)res->crtc_id_ptr;

    // 找已连接的 connector
    for (int i = 0; i < (int)res->count_connectors; i++) {
        auto *conn = drm_get_connector(drm_fd, conn_ids[i]);
        if (!conn) continue;
        if (conn->connection == DRM_MODE_CONNECTED) {
            drm_conn = conn_ids[i];
            free(conn);
            break;
        }
        free(conn);
    }

    // 找活跃 CRTC（mode_valid=1）
    uint32_t crtc_mask = 0;
    for (int i = 0; i < (int)res->count_crtcs; i++) {
        drm_mode_crtc ci{};
        ci.crtc_id = crtc_ids[i];
        if (ioctl(drm_fd, DRM_IOCTL_MODE_GETCRTC, &ci) < 0) continue;
        if (ci.mode_valid) {
            drm_crtc  = crtc_ids[i];
            crtc_mask = (1u << i);
            break;
        }
    }

    // 找支持 YUV420 / NV12 且属于此 CRTC 的 plane
    auto *plane_res = drm_get_plane_res(drm_fd);
    if (!plane_res) { free(res); return -1; }
    auto *plane_ids = (uint32_t*)plane_res->plane_id_ptr;
    for (int i = 0; i < (int)plane_res->count_planes; i++) {
        auto *plane = drm_get_plane(drm_fd, plane_ids[i]);
        if (!plane) continue;
        if (crtc_mask && !(plane->possible_crtcs & crtc_mask)) { free(plane); continue; }
        auto *fmts = (uint32_t*)plane->format_type_ptr;
        for (int f = 0; f < (int)plane->count_format_types; f++) {
            if (fmts[f] == DRM_FORMAT_YUV420) {
                drm_yuv_plane = plane_ids[i]; g_use_nv12 = false;
                free(plane); goto plane_found;
            }
            if (fmts[f] == DRM_FORMAT_NV12 && !drm_yuv_plane) {
                drm_yuv_plane = plane_ids[i]; g_use_nv12 = true;
            }
        }
        free(plane);
    }
plane_found:
    free(plane_res);
    free(res);

    if (!drm_yuv_plane || !drm_crtc || !drm_conn) {
        fprintf(stderr, "[preview] DRM init failed: conn=%u crtc=%u plane=%u\n",
                drm_conn, drm_crtc, drm_yuv_plane);
        return -1;
    }
    fprintf(stderr, "[preview] DRM ok: conn=%u crtc=%u plane=%u %s\n",
            drm_conn, drm_crtc, drm_yuv_plane, g_use_nv12 ? "NV12" : "YUV420");

    // 查找 plane 的旋转属性 ID（部分驱动可能不支持，非致命）
    g_rot_prop = find_plane_prop(drm_yuv_plane, "rotation");
    return 0;
}

static void drm_cleanup()
{
    if (g_cur_fb) { drm_rm_fb(drm_fd, g_cur_fb); g_cur_fb = 0; }
    if (drm_fd >= 0) { close(drm_fd); drm_fd = -1; }
}

// ===================== 帧处理 =====================
static uint32_t import_fb(const FrameBuffer *frame_buf)
{
    drm_mode_fb_cmd2 fb{};
    fb.width  = g_buf_w;   // 旋转后的实际 buffer 宽
    fb.height = g_buf_h;   // 旋转后的实际 buffer 高

    int dma_fd = frame_buf->planes()[0].fd.get();
    uint32_t gem;
    if (drm_prime_fd_to_handle(drm_fd, dma_fd, &gem) < 0) {
        perror("drm_prime_fd_to_handle"); return 0;
    }
    if (g_use_nv12) {
        fb.pixel_format = DRM_FORMAT_NV12;
        fb.handles[0] = fb.handles[1] = gem;
        fb.pitches[0] = g_stride;
        fb.pitches[1] = g_stride;
        fb.offsets[0] = 0;
        fb.offsets[1] = g_stride * g_buf_h;
    } else {
        fb.pixel_format = DRM_FORMAT_YUV420;
        fb.handles[0] = fb.handles[1] = fb.handles[2] = gem;
        fb.pitches[0] = g_stride;
        fb.pitches[1] = g_stride / 2;
        fb.pitches[2] = g_stride / 2;
        fb.offsets[0] = 0;
        fb.offsets[1] = g_stride * g_buf_h;
        fb.offsets[2] = g_stride * g_buf_h + (g_stride / 2) * (g_buf_h / 2);
    }
    if (drm_add_fb2(drm_fd, &fb) < 0) { perror("drm_add_fb2"); return 0; }
    return fb.fb_id;
}

static void display_fb(uint32_t fb_id)
{
    // src 使用 config 中的裁剪参数（preview_fill 时有值），默认全图
    uint32_t sx = g_cfg.src_x;
    uint32_t sy = g_cfg.src_y;
    uint32_t sw = g_cfg.src_w ? g_cfg.src_w : (g_buf_w << 16);
    uint32_t sh = g_cfg.src_h ? g_cfg.src_h : (g_buf_h << 16);

    if (drm_set_plane_ex(drm_fd,
            drm_yuv_plane, drm_crtc, fb_id, 0,
            g_cfg.disp_x, g_cfg.disp_y, g_cfg.disp_w, g_cfg.disp_h,
            sx, sy, sw, sh) < 0)
        perror("drm_set_plane");
}

static void on_frame_complete(Request *req)
{
    if (!g_running) return;
    if (req->status() != Request::RequestComplete) {
        req->reuse(Request::ReuseBuffers);
        g_camera->queueRequest(req);
        return;
    }
    const FrameBuffer *buf = req->buffers().at(g_view_stream);
    uint32_t new_fb = import_fb(buf);
    if (new_fb) {
        display_fb(new_fb);
        if (g_cur_fb) drm_rm_fb(drm_fd, g_cur_fb);
        g_cur_fb = new_fb;
    }
    req->reuse(Request::ReuseBuffers);
    g_camera->queueRequest(req);
}

// ===================== 公开接口 =====================
int preview_open(const PreviewConfig &cfg)
{
    g_cfg = cfg;

    if (drm_init() < 0) return -1;

    g_cam_mgr = new CameraManager();
    if (g_cam_mgr->start() < 0) {
        fprintf(stderr, "[preview] CameraManager start failed\n");
        goto err_drm;
    }
    if (g_cam_mgr->cameras().empty()) {
        fprintf(stderr, "[preview] No camera found\n");
        goto err_mgr;
    }

    g_camera = g_cam_mgr->cameras()[0];
    if (g_camera->acquire() < 0) {
        fprintf(stderr, "[preview] Camera acquire failed\n");
        goto err_mgr;
    }

    {
        auto cam_cfg = g_camera->generateConfiguration({StreamRole::Viewfinder});
        StreamConfiguration &sc = cam_cfg->at(0);

        // 90°/270° 旋转后 buffer 宽高互换，请求对应的竖向分辨率
        bool swap = (cfg.rotation == 90 || cfg.rotation == 270);
        sc.size        = swap ? Size(cfg.cam_h, cfg.cam_w) : Size(cfg.cam_w, cfg.cam_h);
        sc.pixelFormat = g_use_nv12 ? formats::NV12 : formats::YUV420;
        sc.bufferCount = 4;

        // 用 libcamera ISP Transform 做旋转（VC4 HVS 不支持 90/270 硬件旋转）
       

        cam_cfg->validate();
        if (g_camera->configure(cam_cfg.get()) < 0) {
            fprintf(stderr, "[preview] Camera configure failed\n");
            goto err_cam;
        }
        g_view_stream = sc.stream();
        g_stride      = sc.stride;
        g_buf_w       = sc.size.width;   // 实际 buffer 宽（旋转后）
        g_buf_h       = sc.size.height;  // 实际 buffer 高（旋转后）
        fprintf(stderr, "[preview] Camera buf=%ux%u stride=%u rot=%u° %s\n",
                g_buf_w, g_buf_h, g_stride, cfg.rotation,
                g_use_nv12 ? "NV12" : "YUV420");
    }

    g_alloc = std::make_unique<FrameBufferAllocator>(g_camera);
    if (g_alloc->allocate(g_view_stream) < 0) {
        fprintf(stderr, "[preview] Buffer alloc failed\n");
        goto err_cam;
    }

    for (unsigned i = 0; i < 4; i++) {
        auto req = g_camera->createRequest();
        if (!req) break;
        req->addBuffer(g_view_stream, g_alloc->buffers(g_view_stream)[i].get());
        g_req_list.push_back(std::move(req));
    }

    g_camera->requestCompleted.connect(on_frame_complete);
    if (g_camera->start() < 0) {
        fprintf(stderr, "[preview] Camera start failed\n");
        goto err_alloc;
    }

    g_running = true;
    apply_rotation();   // 在推帧前写入旋转属性
    for (auto &r : g_req_list) g_camera->queueRequest(r.get());
    fprintf(stderr, "[preview] Preview started  display=%dx%d@(%d,%d)\n",
            cfg.disp_w, cfg.disp_h, cfg.disp_x, cfg.disp_y);
    return 0;

err_alloc:
    g_req_list.clear();
    g_alloc.reset();
err_cam:
    g_camera->release();
    g_camera.reset();
err_mgr:
    g_cam_mgr->stop();
    delete g_cam_mgr;
    g_cam_mgr = nullptr;
err_drm:
    drm_cleanup();
    return -1;
}

void preview_close()
{
    g_running = false;
    if (g_camera) {
        g_camera->stop();
        g_camera->requestCompleted.disconnect(on_frame_complete);
    }
    drm_cleanup();
    g_req_list.clear();
    g_alloc.reset();
    if (g_camera) { g_camera->release(); g_camera.reset(); }
    usleep(50000);
    if (g_cam_mgr) { g_cam_mgr->stop(); delete g_cam_mgr; g_cam_mgr = nullptr; }
    drm_crtc = drm_conn = drm_yuv_plane = 0;
    g_stride = g_cur_fb = 0;
    fprintf(stderr, "[preview] Preview closed\n");
}
