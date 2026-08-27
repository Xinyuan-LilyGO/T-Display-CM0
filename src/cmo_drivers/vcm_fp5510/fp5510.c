// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2015--2017 Intel Corporation.
// Copyright (c) 2026 LILYGO

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>

#define FP5510_NAME			"fp5510"
#define FP5510_MAX_FOCUS_POS		1023
#define FP5510_FOCUS_STEPS		1
#define FP5510_CTRL_STEPS		16
#define FP5510_CTRL_DELAY_US		1000
#define FP5510_POWER_ON_DELAY_US	12000

/*
 * FP5510 position write format:
 *
 * Byte1: Busy D9 D8 D7 D6 D5 D4 PD
 * Byte2: D3   D2 D1 D0 MS1 MS0 ML1 ML0
 *
 * Busy must be 0 on write.
 * PD = 0 selects normal operation.
 * MS[1:0] = 00 selects one-step mode.
 * ML[1:0] = 01 keeps the datasheet default.
 */
#define FP5510_DEFAULT_MODE		0x01

struct fp5510_device {
	struct v4l2_ctrl_handler ctrls_vcm;
	struct v4l2_subdev sd;
	u16 current_val;
	struct regulator *vcc;
};

static inline struct fp5510_device *to_fp5510_vcm(struct v4l2_ctrl *ctrl)
{
	return container_of(ctrl->handler, struct fp5510_device, ctrls_vcm);
}

static inline struct fp5510_device *sd_to_fp5510_vcm(struct v4l2_subdev *subdev)
{
	return container_of(subdev, struct fp5510_device, sd);
}

static void fp5510_val(u16 data, u8 buf[2])
{
	data = clamp_val(data, 0, FP5510_MAX_FOCUS_POS);

	buf[0] = ((data >> 4) << 1) & 0x7e;
	buf[1] = ((data & 0x0f) << 4) | FP5510_DEFAULT_MODE;
}

static int fp5510_i2c_write(struct i2c_client *client, u16 data)
{
	u8 buf[2];
	int ret;

	fp5510_val(data, buf);

	ret = i2c_master_send(client, buf, sizeof(buf));
	if (ret != sizeof(buf)) {
		dev_err(&client->dev, "I2C write fail\n");
		return ret < 0 ? ret : -EIO;
	}

	return 0;
}

static int fp5510_t_focus_vcm(struct fp5510_device *fp5510_dev, u16 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&fp5510_dev->sd);

	fp5510_dev->current_val = val;

	return fp5510_i2c_write(client, val);
}

static int fp5510_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct fp5510_device *dev_vcm = to_fp5510_vcm(ctrl);

	if (ctrl->id == V4L2_CID_FOCUS_ABSOLUTE)
		return fp5510_t_focus_vcm(dev_vcm, ctrl->val);

	return -EINVAL;
}

static const struct v4l2_ctrl_ops fp5510_vcm_ctrl_ops = {
	.s_ctrl = fp5510_set_ctrl,
};

static int fp5510_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	return pm_runtime_resume_and_get(sd->dev);
}

static int fp5510_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	pm_runtime_put(sd->dev);

	return 0;
}

static const struct v4l2_subdev_internal_ops fp5510_int_ops = {
	.open = fp5510_open,
	.close = fp5510_close,
};

static const struct v4l2_subdev_core_ops fp5510_core_ops = {
	.log_status = v4l2_ctrl_subdev_log_status,
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_ops fp5510_ops = {
	.core = &fp5510_core_ops,
};

static void fp5510_subdev_cleanup(struct fp5510_device *fp5510_dev)
{
	v4l2_async_unregister_subdev(&fp5510_dev->sd);
	v4l2_ctrl_handler_free(&fp5510_dev->ctrls_vcm);
	media_entity_cleanup(&fp5510_dev->sd.entity);
}

static int fp5510_init_controls(struct fp5510_device *dev_vcm)
{
	struct v4l2_ctrl_handler *hdl = &dev_vcm->ctrls_vcm;
	const struct v4l2_ctrl_ops *ops = &fp5510_vcm_ctrl_ops;

	v4l2_ctrl_handler_init(hdl, 1);

	v4l2_ctrl_new_std(hdl, ops, V4L2_CID_FOCUS_ABSOLUTE,
			  0, FP5510_MAX_FOCUS_POS, FP5510_FOCUS_STEPS, 0);

	if (hdl->error)
		dev_err(dev_vcm->sd.dev, "%s fail error: 0x%x\n",
			__func__, hdl->error);

	dev_vcm->sd.ctrl_handler = hdl;

	return hdl->error;
}

static int fp5510_probe(struct i2c_client *client)
{
	struct fp5510_device *fp5510_dev;
	int rval;

	fp5510_dev = devm_kzalloc(&client->dev, sizeof(*fp5510_dev),
				  GFP_KERNEL);
	if (!fp5510_dev)
		return -ENOMEM;

	fp5510_dev->vcc = devm_regulator_get(&client->dev, "vcc");
	if (IS_ERR(fp5510_dev->vcc))
		return PTR_ERR(fp5510_dev->vcc);

	rval = regulator_enable(fp5510_dev->vcc);
	if (rval < 0) {
		dev_err(&client->dev, "failed to enable vcc: %d\n", rval);
		return rval;
	}

	usleep_range(FP5510_POWER_ON_DELAY_US, FP5510_POWER_ON_DELAY_US + 1000);

	v4l2_i2c_subdev_init(&fp5510_dev->sd, client, &fp5510_ops);
	fp5510_dev->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
				V4L2_SUBDEV_FL_HAS_EVENTS;
	fp5510_dev->sd.internal_ops = &fp5510_int_ops;

	rval = fp5510_init_controls(fp5510_dev);
	if (rval)
		goto err_cleanup;

	rval = media_entity_pads_init(&fp5510_dev->sd.entity, 0, NULL);
	if (rval < 0)
		goto err_cleanup;

	fp5510_dev->sd.entity.function = MEDIA_ENT_F_LENS;

	rval = v4l2_async_register_subdev(&fp5510_dev->sd);
	if (rval < 0)
		goto err_cleanup;

	pm_runtime_set_active(&client->dev);
	pm_runtime_enable(&client->dev);
	pm_runtime_idle(&client->dev);

	return 0;

err_cleanup:
	regulator_disable(fp5510_dev->vcc);
	v4l2_ctrl_handler_free(&fp5510_dev->ctrls_vcm);
	media_entity_cleanup(&fp5510_dev->sd.entity);

	return rval;
}

static void fp5510_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct fp5510_device *fp5510_dev = sd_to_fp5510_vcm(sd);
	int ret;

	pm_runtime_disable(&client->dev);
	if (!pm_runtime_status_suspended(&client->dev)) {
		ret = regulator_disable(fp5510_dev->vcc);
		if (ret)
			dev_err(&client->dev,
				"Failed to disable vcc: %d\n", ret);
	}
	pm_runtime_set_suspended(&client->dev);
	fp5510_subdev_cleanup(fp5510_dev);
}

static int __maybe_unused fp5510_vcm_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct fp5510_device *fp5510_dev = sd_to_fp5510_vcm(sd);
	int ret, val;

	if (pm_runtime_suspended(&client->dev))
		return 0;

	for (val = fp5510_dev->current_val & ~(FP5510_CTRL_STEPS - 1);
	     val >= 0; val -= FP5510_CTRL_STEPS) {
		ret = fp5510_i2c_write(client, val);
		if (ret)
			dev_err_once(dev, "%s I2C failure: %d", __func__, ret);
		usleep_range(FP5510_CTRL_DELAY_US, FP5510_CTRL_DELAY_US + 10);
	}

	ret = regulator_disable(fp5510_dev->vcc);
	if (ret)
		dev_err(dev, "Failed to disable vcc: %d\n", ret);

	return ret;
}

static int  __maybe_unused fp5510_vcm_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct fp5510_device *fp5510_dev = sd_to_fp5510_vcm(sd);
	int ret, val;

	if (pm_runtime_suspended(&client->dev))
		return 0;

	ret = regulator_enable(fp5510_dev->vcc);
	if (ret) {
		dev_err(dev, "Failed to enable vcc: %d\n", ret);
		return ret;
	}

	usleep_range(FP5510_POWER_ON_DELAY_US, FP5510_POWER_ON_DELAY_US + 1000);

	for (val = fp5510_dev->current_val % FP5510_CTRL_STEPS;
	     val < fp5510_dev->current_val + FP5510_CTRL_STEPS - 1;
	     val += FP5510_CTRL_STEPS) {
		ret = fp5510_i2c_write(client, val);
		if (ret)
			dev_err_ratelimited(dev, "%s I2C failure: %d",
					    __func__, ret);
		usleep_range(FP5510_CTRL_DELAY_US, FP5510_CTRL_DELAY_US + 10);
	}

	return 0;
}

static const struct i2c_device_id fp5510_id_table[] = {
	{ FP5510_NAME },
	{ }
};
MODULE_DEVICE_TABLE(i2c, fp5510_id_table);

static const struct of_device_id fp5510_of_table[] = {
	{ .compatible = "fitipower,fp5510" },
	{ }
};
MODULE_DEVICE_TABLE(of, fp5510_of_table);

static const struct dev_pm_ops fp5510_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(fp5510_vcm_suspend, fp5510_vcm_resume)
	SET_RUNTIME_PM_OPS(fp5510_vcm_suspend, fp5510_vcm_resume, NULL)
};

static struct i2c_driver fp5510_i2c_driver = {
	.driver = {
		.name = FP5510_NAME,
		.pm = &fp5510_pm_ops,
		.of_match_table = fp5510_of_table,
	},
	.probe = fp5510_probe,
	.remove = fp5510_remove,
	.id_table = fp5510_id_table,
};

module_i2c_driver(fp5510_i2c_driver);

MODULE_AUTHOR("LILYGO");
MODULE_DESCRIPTION("FP5510 VCM driver");
MODULE_LICENSE("GPL v2");

