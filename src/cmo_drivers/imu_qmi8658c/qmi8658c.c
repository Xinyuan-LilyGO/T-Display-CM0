// SPDX-License-Identifier: GPL-2.0
/*
 * QST QMI8658C 6-axis IMU I2C IIO driver
 *
 * Default configuration:
 *   I2C address: 0x6a
 *   accelerometer: +/-8g, 500Hz setting
 *   gyroscope: +/-512dps, 500Hz setting
 */

#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/iio/iio.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/regmap.h>

#define QMI8658C_REG_WHO_AM_I 0x00
#define QMI8658C_REG_CTRL1    0x02
#define QMI8658C_REG_CTRL2    0x03
#define QMI8658C_REG_CTRL3    0x04
#define QMI8658C_REG_CTRL5    0x06
#define QMI8658C_REG_CTRL7    0x08
#define QMI8658C_REG_STATUS0  0x2e
#define QMI8658C_REG_TEMP_L   0x33
#define QMI8658C_REG_AX_L     0x35
#define QMI8658C_REG_AY_L     0x37
#define QMI8658C_REG_AZ_L     0x39
#define QMI8658C_REG_GX_L     0x3b
#define QMI8658C_REG_GY_L     0x3d
#define QMI8658C_REG_GZ_L     0x3f
#define QMI8658C_REG_RESET    0x60

#define QMI8658C_WHO_AM_I_VAL       0x05
#define QMI8658C_RESET_CMD          0xb0
#define QMI8658C_CTRL1_ADDR_AI      BIT(6)
#define QMI8658C_CTRL7_ACC_ENABLE   BIT(0)
#define QMI8658C_CTRL7_GYRO_ENABLE  BIT(1)

#define QMI8658C_ACC_8G_CTRL2       0x20
#define QMI8658C_ACC_ODR_500_CTRL2  0x04
#define QMI8658C_GYRO_512_CTRL3     0x50
#define QMI8658C_GYRO_ODR_500_CTRL3 0x04

#define QMI8658C_ACC_LSB_PER_G      4096
#define QMI8658C_GYRO_LSB_PER_DPS   64
#define QMI8658C_ACC_SCALE_NANO     2394201
#define QMI8658C_GYRO_SCALE_NANO    272707

struct qmi8658c_data {
	struct regmap *regmap;
	struct mutex lock;
};

enum qmi8658c_scan_index {
	QMI8658C_SCAN_ACCEL_X,
	QMI8658C_SCAN_ACCEL_Y,
	QMI8658C_SCAN_ACCEL_Z,
	QMI8658C_SCAN_GYRO_X,
	QMI8658C_SCAN_GYRO_Y,
	QMI8658C_SCAN_GYRO_Z,
	QMI8658C_SCAN_TEMP,
};

static const struct regmap_config qmi8658c_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = QMI8658C_REG_RESET,
};

static int qmi8658c_read_s16(struct qmi8658c_data *data, u8 reg, int *val)
{
	u8 buf[2];
	int ret;

	ret = regmap_bulk_read(data->regmap, reg, buf, sizeof(buf));
	if (ret)
		return ret;

	*val = (s16)((buf[1] << 8) | buf[0]);
	return 0;
}

static int qmi8658c_read_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan,
			     int *val, int *val2, long mask)
{
	struct qmi8658c_data *data = iio_priv(indio_dev);
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		mutex_lock(&data->lock);
		ret = qmi8658c_read_s16(data, chan->address, val);
		mutex_unlock(&data->lock);
		if (ret)
			return ret;

		return IIO_VAL_INT;

	case IIO_CHAN_INFO_PROCESSED:
		if (chan->type != IIO_TEMP)
			return -EINVAL;

		mutex_lock(&data->lock);
		ret = qmi8658c_read_s16(data, chan->address, val);
		mutex_unlock(&data->lock);
		if (ret)
			return ret;

		/* Temperature register resolution is 256 LSB/degC. */
		*val = (*val * 1000) / 256;
		return IIO_VAL_INT;

	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_ACCEL:
			/*
			 * IIO accelerometer scale is m/s^2.
			 * 1g = 9.80665m/s^2, +/-8g is 4096 LSB/g.
			 */
			*val = 0;
			*val2 = QMI8658C_ACC_SCALE_NANO;
			return IIO_VAL_INT_PLUS_NANO;

		case IIO_ANGL_VEL:
			/*
			 * IIO angular velocity scale is rad/s.
			 * 1dps = pi/180 rad/s ~= 17453293 nrad/s.
			 */
			*val = 0;
			*val2 = QMI8658C_GYRO_SCALE_NANO;
			return IIO_VAL_INT_PLUS_NANO;

		case IIO_TEMP:
			/* IIO temperature scale is millidegC per LSB. */
			*val = 3;
			*val2 = 906250;
			return IIO_VAL_INT_PLUS_MICRO;

		default:
			return -EINVAL;
		}

	default:
		return -EINVAL;
	}
}

static const struct iio_info qmi8658c_info = {
	.read_raw = qmi8658c_read_raw,
};

#define QMI8658C_ACCEL_CHANNEL(_axis, _addr, _idx) { \
	.type = IIO_ACCEL, \
	.modified = 1, \
	.channel2 = IIO_MOD_##_axis, \
	.address = (_addr), \
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW), \
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE), \
	.scan_index = (_idx), \
}

#define QMI8658C_GYRO_CHANNEL(_axis, _addr, _idx) { \
	.type = IIO_ANGL_VEL, \
	.modified = 1, \
	.channel2 = IIO_MOD_##_axis, \
	.address = (_addr), \
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW), \
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE), \
	.scan_index = (_idx), \
}

static const struct iio_chan_spec qmi8658c_channels[] = {
	QMI8658C_ACCEL_CHANNEL(X, QMI8658C_REG_AX_L, QMI8658C_SCAN_ACCEL_X),
	QMI8658C_ACCEL_CHANNEL(Y, QMI8658C_REG_AY_L, QMI8658C_SCAN_ACCEL_Y),
	QMI8658C_ACCEL_CHANNEL(Z, QMI8658C_REG_AZ_L, QMI8658C_SCAN_ACCEL_Z),
	QMI8658C_GYRO_CHANNEL(X, QMI8658C_REG_GX_L, QMI8658C_SCAN_GYRO_X),
	QMI8658C_GYRO_CHANNEL(Y, QMI8658C_REG_GY_L, QMI8658C_SCAN_GYRO_Y),
	QMI8658C_GYRO_CHANNEL(Z, QMI8658C_REG_GZ_L, QMI8658C_SCAN_GYRO_Z),
	{
		.type = IIO_TEMP,
		.address = QMI8658C_REG_TEMP_L,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
				      BIT(IIO_CHAN_INFO_PROCESSED) |
				      BIT(IIO_CHAN_INFO_SCALE),
		.scan_index = QMI8658C_SCAN_TEMP,
	},
};

static int qmi8658c_chip_init(struct device *dev, struct qmi8658c_data *data)
{
	unsigned int val;
	int ret;

	ret = regmap_write(data->regmap, QMI8658C_REG_RESET, QMI8658C_RESET_CMD);
	if (ret)
		return ret;

	msleep(15);

	ret = regmap_read(data->regmap, QMI8658C_REG_WHO_AM_I, &val);
	if (ret)
		return ret;

	if (val != QMI8658C_WHO_AM_I_VAL)
		return dev_err_probe(dev, -ENODEV,
				     "unexpected WHO_AM_I 0x%02x\n", val);

	/*
	 * ADDR_AI enables register auto increment. BE is left cleared so output
	 * data is little-endian, matching the datasheet output registers.
	 */
	ret = regmap_write(data->regmap, QMI8658C_REG_CTRL1, QMI8658C_CTRL1_ADDR_AI);
	if (ret)
		return ret;

	ret = regmap_write(data->regmap, QMI8658C_REG_CTRL2,
			   QMI8658C_ACC_8G_CTRL2 | QMI8658C_ACC_ODR_500_CTRL2);
	if (ret)
		return ret;

	ret = regmap_write(data->regmap, QMI8658C_REG_CTRL3,
			   QMI8658C_GYRO_512_CTRL3 | QMI8658C_GYRO_ODR_500_CTRL3);
	if (ret)
		return ret;

	ret = regmap_write(data->regmap, QMI8658C_REG_CTRL5, 0x00);
	if (ret)
		return ret;

	ret = regmap_write(data->regmap, QMI8658C_REG_CTRL7,
			   QMI8658C_CTRL7_ACC_ENABLE | QMI8658C_CTRL7_GYRO_ENABLE);
	if (ret)
		return ret;

	msleep(150);
	return 0;
}

static void qmi8658c_power_down(void *priv)
{
	struct qmi8658c_data *data = priv;

	regmap_write(data->regmap, QMI8658C_REG_CTRL7, 0x00);
}

static int qmi8658c_probe(struct i2c_client *client)
{
	struct iio_dev *indio_dev;
	struct qmi8658c_data *data;
	int ret;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;

	data = iio_priv(indio_dev);
	i2c_set_clientdata(client, indio_dev);
	mutex_init(&data->lock);

	data->regmap = devm_regmap_init_i2c(client, &qmi8658c_regmap_config);
	if (IS_ERR(data->regmap))
		return dev_err_probe(&client->dev, PTR_ERR(data->regmap),
				     "failed to initialize regmap\n");

	ret = qmi8658c_chip_init(&client->dev, data);
	if (ret)
		return ret;

	ret = devm_add_action_or_reset(&client->dev, qmi8658c_power_down, data);
	if (ret)
		return ret;

	indio_dev->name = "qmi8658c";
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->info = &qmi8658c_info;
	indio_dev->channels = qmi8658c_channels;
	indio_dev->num_channels = ARRAY_SIZE(qmi8658c_channels);

	ret = devm_iio_device_register(&client->dev, indio_dev);
	if (ret)
		return dev_err_probe(&client->dev, ret,
				     "failed to register IIO device\n");

	dev_info(&client->dev, "QMI8658C IIO device registered at 0x%02x\n",
		 client->addr);

	return 0;
}

static const struct of_device_id qmi8658c_of_match[] = {
	{ .compatible = "qst,qmi8658c" },
	{ }
};
MODULE_DEVICE_TABLE(of, qmi8658c_of_match);

static const struct i2c_device_id qmi8658c_id[] = {
	{ "qmi8658c", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, qmi8658c_id);

static struct i2c_driver qmi8658c_driver = {
	.driver = {
		.name = "qmi8658c",
		.of_match_table = qmi8658c_of_match,
	},
	.probe = qmi8658c_probe,
	.id_table = qmi8658c_id,
};
module_i2c_driver(qmi8658c_driver);

MODULE_AUTHOR("Codex");
MODULE_DESCRIPTION("QMI8658C I2C IIO driver");
MODULE_LICENSE("GPL");

