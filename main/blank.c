#include <stdio.h>
#include "esp_log.h"
#include "esp_intr_alloc.h"

#include "freertos/FreeRTOS.h"

#include "driver/i2c.h"
#include "driver/gpio.h"

#include "headers/sensor_i2c_comm_conf.h"
#include "headers/utils.h"

static const char *TAG = "LABWORK_ESP";

void i2c_reader(void *params){
	const char *I2C_TAG = "I2C Loop";
	const TickType_t xDelay = ms_to_ticks(50);

	init_mpu6050();

	ESP_LOGI(I2C_TAG, "Running i2c reader loop...");
	while(true){
		// https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf
		
		float measurements_f[3] = {0};
		read_acc_registers_calibrated(measurements_f);

		// for(int i = 0; i < 3; i++){ measurements_f[i] = g_to_acceleration(measurements_f[i]); }
		ESP_LOGI(I2C_TAG, "Acc X: %f m/s^2", measurements_f[0]);
		ESP_LOGI(I2C_TAG, "Acc Y: %f m/s^2", measurements_f[1]);
		ESP_LOGI(I2C_TAG, "Acc Z: %f m/s^2", measurements_f[2]);
		ESP_LOGI(I2C_TAG, "------------------------------");

		vTaskDelay( xDelay );
	}
}

void app_main(void)
{
	est_err_t err = esp_intr_alloc()

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
