#include "headers/sensor_i2c_comm_conf.h"

#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

const uint8_t SAMPLE_RATE_DIVIDER_REG[2] = {0x25, 0};
const uint8_t FSYNC_DLPF_CONF_REF[2] = {0x26, 0b00001101};
const uint8_t GYRO_CONF_REG[2] = {0x27, 0b11101000};
const uint8_t PWR_MGMT_1_REG[2] = {0x6b, 0b00000001};
const uint8_t ACC_CONFIG_REG[2] = {0x1C, 0b11101000};

void init_mpu6050() {
	uint8_t buf[1] = {0};
	i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDRESS, SAMPLE_RATE_DIVIDER_REG, 2, buf, 1, TIMEOUT);
	i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDRESS, FSYNC_DLPF_CONF_REF, 2, buf, 1, TIMEOUT);
	i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDRESS, GYRO_CONF_REG, 2, buf, 1, TIMEOUT);
	i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDRESS, PWR_MGMT_1_REG, 2, buf, 1, TIMEOUT);
	i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDRESS, ACC_CONFIG_REG, 2, buf, 1, TIMEOUT);
}