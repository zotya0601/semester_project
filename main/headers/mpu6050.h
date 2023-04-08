#ifndef MPU6050__H
#define MPU6050__H

#define SAMPLE_RATE_DIVIDER_REG             (0x19)
#define SAMPLE_RATE_DIVIDER_NO_DIVIDER      (0x00)
#define SAMPLE_RATE_DIVIDER_1KHZ            (7)

#define FSYNC_DLPF_CONF_REG                 (0x1a)

#define FSYNC_DLPF_CONF_EXT_SYNC_DISABLED   (0b00000000)
#define FSYNC_DLPF_CONF_EXT_SYNC_TEMP_OUT   (0b00001000)
#define FSYNC_DLPF_CONF_EXT_SYNC_GYRO_X     (0b00010000)
#define FSYNC_DLPF_CONF_EXT_SYNC_GYRO_Y     (0b00011000)
#define FSYNC_DLPF_CONF_EXT_SYNC_GYRO_Z     (0b00100000)
#define FSYNC_DLPF_CONF_EXT_SYNC_ACC_X      (0b00101000)
#define FSYNC_DLPF_CONF_EXT_SYNC_ACC_Y      (0b00110000)
#define FSYNC_DLPF_CONF_EXT_SYNC_ACC_Z      (0b00111000)

#define FSYNC_DLPF_CONF_DLPF_OFF            (0b00000000)
#define FSYNC_DLPF_CONF_DLPF_1              (0b00000001)
#define FSYNC_DLPF_CONF_DLPF_2              (0b00000010)
#define FSYNC_DLPF_CONF_DLPF_3              (0b00000011)
#define FSYNC_DLPF_CONF_DLPF_4              (0b00000100)
#define FSYNC_DLPF_CONF_DLPF_5              (0b00000101)
#define FSYNC_DLPF_CONF_DLPF_MAX            (0b00000110)

#define ACCEL_CONFIG_REG                    (0x1c)

#define ACCEL_CONFIG_SELFTEST_X             (0b10000000)
#define ACCEL_CONFIG_SELFTEST_Y             (0b01000000)
#define ACCEL_CONFIG_SELFTEST_Z             (0b00100000)
#define ACCEL_CONFIG_RANGE_2G               (0b00000000)
#define ACCEL_CONFIG_RANGE_4G               (0b00001000)
#define ACCEL_CONFIG_RANGE_8G               (0b00010000)
#define ACCEL_CONFIG_RANGE_16G              (0b00011000)

#define PWR_MGMT_1_REG                      (0x6b)
#define PWR_MGMT_1_CLKSEL_INTERNAL          (0b00000000)
#define PWR_MGMT_1_CLKSEL_GYRO_X            (0b00000001)
#define PWR_MGMT_1_CLKSEL_GYRO_Y            (0b00000010)
#define PWR_MGMT_1_CLKSEL_GYRO_Z            (0b00000011)
#define PWR_MGMT_1_CLKSEL_EXT_32_768KHZ     (0b00000100)
#define PWR_MGMT_1_CLKSEL_EXT_19_2MHZ       (0b00000101)
#define PWR_MGMT_1_CLKSEL_STOP_RESET        (0b00000111)

#define INT_PIN_CFG_1_REG                   (0x37)
#define INT_PIN_CFG_2_REG                   (0x38)

#define INT_PIN_CFG_1_INT_LEVEL_ACT_L       (0b10000000)
#define INT_PIN_CFG_1_INT_OPEN_OD           (0b01000000)
#define INT_PIN_CFG_1_HOLD_TIL_CLEAR        (0b00100000)
#define INT_PIN_CFG_1_INT_RD_CLEAR_ANY_READ (0b00010000)


#include "driver/gpio.h"
#include "headers/utils.h"

void initialize_int_receiver(gpio_num_t gpio_pin);

typedef struct _Measurements{
    Complex x, y, z;
} Measurements;

#endif