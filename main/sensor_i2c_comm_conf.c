#include "headers/sensor_i2c_comm_conf.h"
#include "headers/utils.h"
#include "headers/mpu6050.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_dsp.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

#include <string.h>
#include <math.h>

static const uint8_t SAMPLE_RATE_DIVIDER[2] = 
	{SAMPLE_RATE_DIVIDER_REG, SAMPLE_RATE_DIVIDER_1KHZ};	// Default: 0x00
static const uint8_t FSYNC_DLPF_CONF[2] = 
	{FSYNC_DLPF_CONF_REG, (FSYNC_DLPF_CONF_EXT_SYNC_GYRO_X | FSYNC_DLPF_CONF_DLPF_OFF)}; // Strong DLPF: 0b00101110
static const uint8_t ACCEL_CONFIG[2] = 
	{ACCEL_CONFIG_REG, ACCEL_CONFIG_RANGE_8G};
static const uint8_t PWR_MGMT_1[2] = 
	{PWR_MGMT_1_REG, PWR_MGMT_1_CLKSEL_GYRO_X};

static const uint8_t INT_PIN_CFG_1[2] = 
	{INT_PIN_CFG_1_REG, INT_PIN_CFG_1_INT_RD_CLEAR_ANY_READ | INT_PIN_CFG_1_INT_OPEN_OD};
static const uint8_t INT_PIN_CFG_2[2] = {0x38, 0b00000001};

void init_mpu6050() {
	i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDRESS, SAMPLE_RATE_DIVIDER, 2, TIMEOUT);
	i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDRESS, FSYNC_DLPF_CONF, 2, TIMEOUT);
	i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDRESS, PWR_MGMT_1, 2, TIMEOUT);

	i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDRESS, ACCEL_CONFIG, 2, TIMEOUT);

	i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDRESS, INT_PIN_CFG_1, 2, TIMEOUT);
	i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDRESS, INT_PIN_CFG_2, 2, TIMEOUT);

}

esp_err_t read_acc_registers(float *res){
	uint8_t accel_reg_1 = 0x3b;
	uint8_t buf[6] = {0};
	i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDRESS, &accel_reg_1, 1, buf, 6, TIMEOUT);

	uint8_t rev_byte_order[6];
	rev_byte_order[0] = buf[1];
	rev_byte_order[1] = buf[0];
	rev_byte_order[2] = buf[3];
	rev_byte_order[3] = buf[2];
	rev_byte_order[4] = buf[5];
	rev_byte_order[5] = buf[4];
	
	int16_t *temp = (void*)(rev_byte_order);
	/*
	int16_t temp[3];
	memcpy(temp, rev_byte_order, 6);
	*/
	
	res[0] = ((float)temp[0]) / 4096;
	res[1] = ((float)temp[1]) / 4096;
	res[2] = ((float)temp[2]) / 4096;
	
	return ESP_OK;

}

esp_err_t read_acc_registers_structured(Measurements *m){
	esp_err_t err = read_acc_registers((float*)m);
	return err;
	/*
	float buf[3] = {0};
	esp_err_t err = read_acc_registers(buf);
	if(err != ESP_OK) return err;
	m->x = buf[0];
	m->y = buf[1];
	m->z = buf[2];
	return ESP_OK;
	*/
}