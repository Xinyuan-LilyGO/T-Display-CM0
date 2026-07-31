#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <drm/drm_panel.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_connector.h>
#include <drm/drm_modes.h>

#define RM69A10_ENABLE_BIST 0

struct amoled_panel {
    struct drm_panel panel;
    struct mipi_dsi_device *dsi;
    struct gpio_desc *reset_gpio;
    struct gpio_desc *enable_gpio;
};

static const struct drm_display_mode rm69a10_default_mode = {
    .clock       = 27331, /* kHz, 612 * 1276 * 60 */  //46857  23428
    .hdisplay    = 568,
    .hsync_start = 568 + 20,
    .hsync_end   = 568 + 20 + 4,
    .htotal      = 568 + 20 + 4 + 20,
    .vdisplay    = 1232,
    .vsync_start = 1232 + 20,
    .vsync_end   = 1232 + 20 + 4,
    .vtotal      = 1232 + 20 + 4 + 20,
    .width_mm    = 44,
    .height_mm   = 94,
};

static int amoled_panel_get_modes(struct drm_panel *panel,
                                  struct drm_connector *connector)
{
    struct amoled_panel *p = container_of(panel, struct amoled_panel, panel);
    struct drm_display_mode *mode;

    dev_info(&p->dsi->dev, "get_modes called\n");

    mode = drm_mode_duplicate(connector->dev, &rm69a10_default_mode);
    if (!mode) {
        dev_err(&p->dsi->dev, "failed to create mode\n");
        return -ENOMEM;
    }

    drm_mode_set_name(mode);
    mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
    drm_mode_probed_add(connector, mode);

    dev_info(&p->dsi->dev, "mode: %dx%d clock=%dkHz\n",
             mode->hdisplay, mode->vdisplay, mode->clock);
    return 1;
}

static int amoled_panel_prepare(struct drm_panel *panel)
{
    struct amoled_panel *p = container_of(panel, struct amoled_panel, panel);
    struct device *dev = &p->dsi->dev;

    dev_info(dev, "panel prepare start\n");

    gpiod_set_value_cansleep(p->enable_gpio, 1);
    msleep(50);
    gpiod_set_value_cansleep(p->reset_gpio, 1);
    msleep(20);
    gpiod_set_value_cansleep(p->reset_gpio, 0);
    msleep(150);

    dev_info(dev, "sending init sequence\n");

    mipi_dsi_dcs_write_seq(p->dsi, 0xFE, 0xFD);
    mipi_dsi_dcs_write_seq(p->dsi, 0x80, 0xFC);
    mipi_dsi_dcs_write_seq(p->dsi, 0xFE, 0x00);
    
    #if 0
    mipi_dsi_dcs_write_seq(p->dsi, 0xFE, 0x49);
    mipi_dsi_dcs_write_seq(p->dsi, 0x94, 0x02);
    mipi_dsi_dcs_write_seq(p->dsi, 0xFE, 0x70);
    mipi_dsi_dcs_write_seq(p->dsi, 0x0E, 0x02);
    mipi_dsi_dcs_write_seq(p->dsi, 0xFE, 0x00);
    mipi_dsi_dcs_write_seq(p->dsi, 0x53, 0x28);
    #endif
    
    

    mipi_dsi_dcs_write_seq(p->dsi, 0x2A, 0x00, 0x00, 0x02, 0x37);
    mipi_dsi_dcs_write_seq(p->dsi, 0x2B, 0x00, 0x00, 0x04, 0xCF);
    mipi_dsi_dcs_write_seq(p->dsi, 0x31, 0x00, 0x03, 0x02, 0x34);
    mipi_dsi_dcs_write_seq(p->dsi, 0x30, 0x00, 0x00, 0x04, 0xCF);

    mipi_dsi_dcs_write_seq(p->dsi, 0x12, 0x00);
    mipi_dsi_dcs_write_seq(p->dsi, 0x35, 0x00);
    mipi_dsi_dcs_write_seq(p->dsi, 0x51, 0xFE);

#if RM69A10_ENABLE_BIST
    mipi_dsi_dcs_write_seq(p->dsi, 0x11);
    msleep(120);

    mipi_dsi_dcs_write_seq(p->dsi, 0x29);

    mipi_dsi_dcs_write_seq(p->dsi, 0xFE, 0x97);
    mipi_dsi_dcs_write_seq(p->dsi, 0x39, 0x03);
    mipi_dsi_dcs_write_seq(p->dsi, 0x3D, 0xFF);
    mipi_dsi_dcs_write_seq(p->dsi, 0x3B, 0xFF);
    mipi_dsi_dcs_write_seq(p->dsi, 0x3A, 0x03);
#else
    mipi_dsi_dcs_write_seq(p->dsi, 0x3A, 0x77);
#endif

    mipi_dsi_dcs_write_seq(p->dsi, 0x11);
    msleep(120);

    mipi_dsi_dcs_write_seq(p->dsi, 0x29);
    
     #if 0
    msleep(60);
    mipi_dsi_dcs_write_seq(p->dsi, 0x51, 0xBB);
    #endif
    dev_info(dev, "panel prepare done\n");
    return 0;
}

static int amoled_panel_enable(struct drm_panel *panel)
{
    struct amoled_panel *p = container_of(panel, struct amoled_panel, panel);
    struct device *dev = &p->dsi->dev;

    dev_info(dev, "panel enable\n");
    return 0;
}

static int amoled_panel_disable(struct drm_panel *panel)
{
    struct amoled_panel *p = container_of(panel, struct amoled_panel, panel);

    dev_info(&p->dsi->dev, "panel disable\n");
    mipi_dsi_dcs_write_seq(p->dsi, 0x28);
    msleep(20);
    mipi_dsi_dcs_write_seq(p->dsi, 0x10);
    msleep(80);
    return 0;
}

static int amoled_panel_unprepare(struct drm_panel *panel)
{
    struct amoled_panel *p = container_of(panel, struct amoled_panel, panel);

    dev_info(&p->dsi->dev, "panel unprepare\n");
    return 0;
}

static const struct drm_panel_funcs amoled_panel_funcs = {
    .prepare   = amoled_panel_prepare,
    .enable    = amoled_panel_enable,
    .disable   = amoled_panel_disable,
    .unprepare = amoled_panel_unprepare,
    .get_modes = amoled_panel_get_modes,
};

static int amoled_panel_probe(struct mipi_dsi_device *dsi)
{
    struct device *dev = &dsi->dev;
    struct amoled_panel *p;
    int ret;

    dev_info(dev, "panel probe start\n");

    p = devm_kzalloc(dev, sizeof(*p), GFP_KERNEL);
    if (!p)
        return -ENOMEM;

    p->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
    if (IS_ERR(p->reset_gpio))
        return dev_err_probe(dev, PTR_ERR(p->reset_gpio),
                             "failed to get reset gpio\n");
    dev_info(dev, "lcd reset gpio OK\n");

    p->enable_gpio = devm_gpiod_get(dev, "enable", GPIOD_OUT_LOW);
    if (IS_ERR(p->enable_gpio))
        return dev_err_probe(dev, PTR_ERR(p->enable_gpio),
                             "failed to get enable gpio\n");
    dev_info(dev, "lcd enable gpio OK\n");

    mipi_dsi_set_drvdata(dsi, p);
    p->dsi = dsi;

    dsi->lanes = 2;
    dsi->hs_rate = 500000000;
    dsi->lp_rate = 10000000;
    dsi->format = MIPI_DSI_FMT_RGB888;
    dsi->mode_flags = MIPI_DSI_MODE_VIDEO |
                      MIPI_DSI_MODE_VIDEO_BURST |
                      MIPI_DSI_MODE_LPM;

    p->panel.prepare_prev_first = true;
    drm_panel_init(&p->panel, dev, &amoled_panel_funcs,
                   DRM_MODE_CONNECTOR_DSI);
    drm_panel_add(&p->panel);

    ret = mipi_dsi_attach(dsi);
    if (ret) {
        dev_err(dev, "mipi_dsi_attach failed: %d\n", ret);
        drm_panel_remove(&p->panel);
        return ret;
    }

    dev_info(dev, "panel probe done\n");
    return 0;
}

static void amoled_panel_remove(struct mipi_dsi_device *dsi)
{
    struct amoled_panel *p = mipi_dsi_get_drvdata(dsi);

    dev_info(&dsi->dev, "panel remove\n");
    mipi_dsi_detach(dsi);
    drm_panel_remove(&p->panel);
}

static const struct of_device_id amoled_of_match[] = {
    { .compatible = "raydium,rm69a10" },
    { }
};
MODULE_DEVICE_TABLE(of, amoled_of_match);

static struct mipi_dsi_driver amoled_panel_driver = {
    .probe  = amoled_panel_probe,
    .remove = amoled_panel_remove,
    .driver = {
        .name           = "panel-rm69a10",
        .of_match_table = amoled_of_match,
    },
};
module_mipi_dsi_driver(amoled_panel_driver);

MODULE_DESCRIPTION("RM69A10 AMOLED DSI Panel Driver");
MODULE_LICENSE("GPL");

