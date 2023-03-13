#include "headers/sensor_i2c_comm_conf.h"
#include "headers/utils.h"

#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

#include <string.h>
#include <math.h>

// static const uint8_t FIFO_ENABLE_REG[2] = {0x23, 0b00001000};
static const uint8_t SAMPLE_RATE_DIVIDER_REG[2] = {0x19, 0x00};
static const uint8_t FSYNC_DLPF_CONF_REF[2] = {0x1a, 0b00101110};
static const uint8_t ACCEL_CONFIG_REG[2] = {0x1c, 0b00010000};
static const uint8_t PWR_MGMT_1_REG[2] = {0x6b, 0b00000001};

/*
 * Struktúra mozgó átlag számolásához -> 10 mintás ablak
 * Circular buffer-féle megoldás
 * Elemei:
 * - 10 elemű "Measurement" típusú tömb az előző minták számára, (x,y,z)
 * - Index, hogy a tömbökben hová lehet írni (következő üres, vagy a legrégebbi elem)
 * - A 10 elem összegét tartalmazó "Measurement típus"
 * 
 * A pointer, mely a legrégebbi elemre mutat, használható arra is, hogy mely elem értékét kell kivonni az összegből (helyére megy az új érték)
 */
typedef struct _measurements{
	float x, y, z;
} Measurements;

#define MEASUREMENT_WINDOW_LEN 10

typedef struct _measurement_correction{
	Measurements window[MEASUREMENT_WINDOW_LEN];
	Measurements *oldest;
	Measurements sum, avg;
	int length;
} MeasurementCorrection;

static void inc_oldest_ptr(MeasurementCorrection *mc){
	Measurements *last = mc->window + MEASUREMENT_WINDOW_LEN - 1;
	if(mc->oldest + 1 > last){
		mc->oldest = mc->window;
	} else {
		mc->oldest++;
	}
}

static inline void subtract_measurements(Measurements *a, Measurements *b){
	a->x -= b->x;
	a->y -= b->y;
	a->z -= b->z;
}

static inline void add_measurements(Measurements *a, Measurements *b){
	a->x += b->x;
	a->y += b->y;
	a->z += b->z;
}

static void push_measurement(MeasurementCorrection *mc, float *buf){
	Measurements old = *(mc->oldest);
	Measurements new;

	new.x = buf[0];
	new.y = buf[1];
	new.z = buf[2];

	subtract_measurements(&(mc->sum), &old);
	add_measurements(&(mc->sum), &new);
	
	mc->avg.x = mc->sum.x / MEASUREMENT_WINDOW_LEN;
	mc->avg.y = mc->sum.y / MEASUREMENT_WINDOW_LEN;
	mc->avg.z = mc->sum.z / MEASUREMENT_WINDOW_LEN;

	inc_oldest_ptr(mc);
}

MeasurementCorrection running_mean;

static void calibrate_sensor(){
	for(int i = 0; i < MEASUREMENT_WINDOW_LEN; i++){
		float buf[3] = {0};
		read_acc_registers(buf);

		push_measurement(&running_mean, buf);
	}
}

void init_mpu6050() {
	// i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDRESS, FIFO_ENABLE_REG, 2, TIMEOUT);

	i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDRESS, SAMPLE_RATE_DIVIDER_REG, 2, TIMEOUT);
	i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDRESS, FSYNC_DLPF_CONF_REF, 2, TIMEOUT);
	i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDRESS, PWR_MGMT_1_REG, 2, TIMEOUT);

	i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDRESS, ACCEL_CONFIG_REG, 2, TIMEOUT);

	running_mean.oldest = running_mean.window;
	running_mean.sum.x = 0;
	running_mean.sum.y = 0;
	running_mean.sum.z = 0;

	running_mean.length = 0;

	calibrate_sensor();

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
	
	int16_t temp[3];
	memcpy(temp, rev_byte_order, 6);

	ESP_LOGI("AAAAA", "Elements in buf: (%02x, %02x, %02x, %02x, %02x, %02x)", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
	ESP_LOGI("AAAAA", "Elements in temp: (%04x, %04x, %04x)", temp[0], temp[1], temp[2]);
	ESP_LOGI("AAAAA", "Elements in temp: (%d, %d, %d)", temp[0], temp[1], temp[2]);
	
	for(int i = 0; i < 3; i++){
		res[i] = ((float)temp[i]) / 4096;	
	}

	ESP_LOGI("AAAAA", "(%f, %f, %f)", res[0], res[1], res[2]);

	return ESP_OK;

}

esp_err_t read_acc_registers_calibrated(float *res){
	esp_err_t err = read_acc_registers(res);
	if(err != ESP_OK) return err;

	push_measurement(&running_mean, res);

	res[0] -= running_mean.avg.x;
	res[1] -= running_mean.avg.y;
	res[2] -= running_mean.avg.z;

	return ESP_OK;
}