#include "imu_iio.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_IIO_DEVICE "/sys/bus/iio/devices/iio:device0"
#define PATH_BUFFER_SIZE 256

typedef struct {
    char raw_path[PATH_BUFFER_SIZE];
    double scale;
    double offset;
    int available;
} iio_axis_t;

static iio_axis_t accel_axes[3];
static iio_axis_t gyro_axes[3];
static int initialized;

static int read_number(const char *path, double *value)
{
    FILE *fp;
    int result;

    fp = fopen(path, "r");
    if (fp == NULL)
        return -1;

    result = fscanf(fp, "%lf", value) == 1 ? 0 : -1;
    fclose(fp);
    return result;
}

static int read_attribute(const char *device, const char *attribute,
                          double *value)
{
    char path[PATH_BUFFER_SIZE];
    int length;

    length = snprintf(path, sizeof(path), "%s/%s", device, attribute);
    if (length < 0 || (size_t)length >= sizeof(path))
        return -1;

    return read_number(path, value);
}

static void configure_axis(iio_axis_t *axis, const char *device,
                           const char *prefix, char axis_name)
{
    char attribute[64];
    int length;

    memset(axis, 0, sizeof(*axis));
    axis->scale = 1.0;

    snprintf(attribute, sizeof(attribute), "%s_%c_raw", prefix, axis_name);
    length = snprintf(axis->raw_path, sizeof(axis->raw_path), "%s/%s",
                      device, attribute);
    if (length < 0 || (size_t)length >= sizeof(axis->raw_path) ||
        access(axis->raw_path, R_OK) != 0)
        return;

    axis->available = 1;

    snprintf(attribute, sizeof(attribute), "%s_%c_scale", prefix, axis_name);
    if (read_attribute(device, attribute, &axis->scale) != 0) {
        snprintf(attribute, sizeof(attribute), "%s_scale", prefix);
        (void)read_attribute(device, attribute, &axis->scale);
    }

    snprintf(attribute, sizeof(attribute), "%s_%c_offset", prefix, axis_name);
    if (read_attribute(device, attribute, &axis->offset) != 0) {
        snprintf(attribute, sizeof(attribute), "%s_offset", prefix);
        (void)read_attribute(device, attribute, &axis->offset);
    }
}

static int read_xyz(const iio_axis_t axes[3], double values[3])
{
    int i;
    double raw;

    for (i = 0; i < 3; ++i) {
        if (!axes[i].available || read_number(axes[i].raw_path, &raw) != 0)
            return -1;
        values[i] = (raw + axes[i].offset) * axes[i].scale;
    }
    return 0;
}

int imu_iio_init(const char *device_path)
{
    static const char axis_names[3] = {'x', 'y', 'z'};
    const char *device = device_path != NULL ? device_path : DEFAULT_IIO_DEVICE;
    int i;

    if (access(device, R_OK) != 0)
        return -1;

    for (i = 0; i < 3; ++i) {
        configure_axis(&accel_axes[i], device, "in_accel", axis_names[i]);
        configure_axis(&gyro_axes[i], device, "in_anglvel", axis_names[i]);
    }

    if (!accel_axes[0].available && !gyro_axes[0].available)
        return -1;

    initialized = 1;
    return 0;
}

int imu_iio_read(imu_iio_sample_t *sample)
{
    double values[3];

    if (!initialized || sample == NULL)
        return -1;

    memset(sample, 0, sizeof(*sample));

    if (read_xyz(accel_axes, values) == 0) {
        sample->accel_x = values[0];
        sample->accel_y = values[1];
        sample->accel_z = values[2];
        sample->has_accel = 1;
    }

    if (read_xyz(gyro_axes, values) == 0) {
        sample->gyro_x = values[0];
        sample->gyro_y = values[1];
        sample->gyro_z = values[2];
        sample->has_gyro = 1;
    }

    return sample->has_accel || sample->has_gyro ? 0 : -1;
}

void imu_iio_deinit(void)
{
    memset(accel_axes, 0, sizeof(accel_axes));
    memset(gyro_axes, 0, sizeof(gyro_axes));
    initialized = 0;
}
