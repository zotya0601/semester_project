#include "sensor_i2c_comm_conf.h"

#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

void init_mpu6050() {
		uint8_t buf[1] = {0};
		i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDRESS, SAMPLE_RATE_DIVIDER_REG, 2, buf, 1, TIMEOUT);
		i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDRESS, FSYNC_DLPF_CONF_REF, 2, buf, 1, TIMEOUT);
		i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDRESS, GYRO_CONF_REG, 2, buf, 1, TIMEOUT);
		i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDRESS, PWR_MGMT_1_REG, 2, buf, 1, TIMEOUT);
		i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDRESS, ACC_CONFIG_REG, 2, buf, 1, TIMEOUT);
	}