#ifndef IMU_IIO_H
#define IMU_IIO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double accel_x;
    double accel_y;
    double accel_z;
    double gyro_x;
    double gyro_y;
    double gyro_z;
    int has_accel;
    int has_gyro;
} imu_iio_sample_t;

/* Pass NULL to use /sys/bus/iio/devices/iio:device0. */
int imu_iio_init(const char *device_path);
int imu_iio_read(imu_iio_sample_t *sample);
void imu_iio_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
