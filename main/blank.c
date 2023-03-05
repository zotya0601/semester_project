#include <stdio.h>
#include "esp_log.h"

#include "freertos/FreeRTOS.h"

#include "driver/i2c.h"
#include "driver/gpio.h"

#include "headers/sensor_i2c_comm_conf.h"

static const char *TAG = "LABWORK_ESP";

static const uint8_t SAMPLE_RATE_DIVIDER_REG[2] = {0x25, 0};
static const uint8_t FSYNC_DLPF_CONF_REF[2] = {0x26, 0b00001101};
static const uint8_t GYRO_CONF_REG[2] = {0x27, 0b11101000};
static const uint8_t PWR_MGMT_1_REG[2] = {0x6b, 0b00000001};
static const uint8_t ACC_CONFIG_REG[2] = {0x1C, 0b11101000};

#define TIMEOUT (I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS)


static inline TickType_t ms_to_ticks(uint32_t ms){ return (ms/portTICK_PERIOD_MS); }

static uint16_t u8toi16(uint8_t msb, uint8_t lsb){
	int16_t res = (int16_t)msb;
	ESP_LOGW("Convert", "%d", res);
	res <<= 8;
	ESP_LOGW("Convert", "%d", res);
	res |= (int16_t)lsb;
	ESP_LOGW("Convert", "%d", res);
	return res;
}

void i2c_reader(void *params){
	const char *I2C_TAG = "I2C Loop";
	const TickType_t xDelay = ms_to_ticks(500);

	init_mpu6050();

	ESP_LOGI(I2C_TAG, "Running i2c reader loop...");
	while(true){
		// https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf
		
		uint8_t addr = 0x3b;
		uint8_t data[6] = {0};
		for(int i = 0; i < 6; i++){
			i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDRESS, &addr, 1, data + i, 1, TIMEOUT);	
			addr++;
		}
		
		int16_t measurement[3] = {u8toi16(data[0], data[1]), u8toi16(data[2], data[3]), u8toi16(data[4], data[5])};
		for(int i = 0; i < 3; i++){ measurement[i] = measurement[i] / (8192 / 4); }
		ESP_LOGI(I2C_TAG, "Acc X: %d", measurement[0] - 2);
		ESP_LOGI(I2C_TAG, "Acc Y: %d", measurement[1] - 1);
		ESP_LOGI(I2C_TAG, "Acc Z: %d", measurement[2] - 6);


		vTaskDelay( xDelay );
	}
}

void app_main(void)
{
	i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

	int i2c_master_port = I2C_MASTER_NUM;

    i2c_param_config(i2c_master_port, &conf);

    ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0));

    ESP_LOGI(TAG, "I2C initialized successfully");

	TaskHandle_t i2c_handle = 0;
	static uint8_t ucParameterToPass;

	xTaskCreate(i2c_reader, "Accelerometer loop", 10000, &ucParameterToPass, tskIDLE_PRIORITY, &i2c_handle);
}
