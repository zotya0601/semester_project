#include <stdio.h>
#include "esp_log.h"
#include "esp_intr_alloc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"

#include "driver/i2c.h"
#include "driver/gpio.h"

#include "headers/mpu6050.h"
#include "headers/sensor_i2c_comm_conf.h"
#include "headers/utils.h"

static const char *TAG = "LABWORK_ESP";

#define GPIO_INT_IN (1ULL << GPIO_NUM_32)
#define GPIO_INT_OUT (1ULL << GPIO_NUM_33)

static IRAM_ATTR SemaphoreHandle_t xInterruptSemaphore = NULL;
static IRAM_ATTR SemaphoreHandle_t xQueueDataReadySemaphore = NULL;
static StreamBufferHandle_t MeasurementsQueue = NULL;

gpio_config_t led = {
	.pin_bit_mask = (1ULL << GPIO_NUM_33),
	.mode = GPIO_MODE_OUTPUT,
	.pull_up_en = 0,
	.pull_down_en = 0,
	.intr_type = GPIO_INTR_DISABLE
};

void measurement_handler(void *params){
	while(true){
		if( xSemaphoreTake( xQueueDataReadySemaphore, portMAX_DELAY ) == pdTRUE ){
			Measurements buffer[1024];
			size_t rbytes = xStreamBufferReceive(MeasurementsQueue, buffer, 1024 * sizeof(Measurements), portMAX_DELAY);

			// ESP_LOGI("measurement_handler", "FFT and stuff... \n\tBytes read: %d", rbytes);
		}
	}
}

static portMUX_TYPE i2c_spinlock = portMUX_INITIALIZER_UNLOCKED;
void i2c_reader(void *params){
	const char *I2C_TAG = DRAM_STR("I2C Loop");
	
	static DRAM_ATTR int16_t queueLength = 0;
	int LED_state = 0;

	init_mpu6050();

	ESP_LOGI(I2C_TAG, "Running i2c reader loop...");
	
	while(true){
		if( xSemaphoreTake( xInterruptSemaphore, portMAX_DELAY ) == pdTRUE) {
			Measurements m;
			read_acc_registers_structured(&m);
			size_t wbytes = xStreamBufferSend(MeasurementsQueue, &m, sizeof(Measurements), portMAX_DELAY);
			
			if(wbytes != 0){
				queueLength++;
				if(queueLength == 1024){
					xSemaphoreGive(xQueueDataReadySemaphore);
					queueLength = 0;

					gpio_set_level(GPIO_NUM_33, LED_state);
					LED_state = !LED_state;
				}
			}
		}
	}
}

static void IRAM_ATTR gpio_isr_handler(void* arg) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(xInterruptSemaphore, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
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
	TaskHandle_t fft_handle = 0;
	static uint8_t ucParameterToPass = 0;
	static uint8_t ucGPIOParameter = GPIO_NUM_32;

	initialize_int_receiver(GPIO_NUM_32);
	gpio_config(&led);
	
	xInterruptSemaphore = xSemaphoreCreateBinary();
	xQueueDataReadySemaphore = xSemaphoreCreateBinary();
	// MeasurementsQueue = xQueueCreate((UBaseType_t) 1024, (UBaseType_t)sizeof(Measurements));
	MeasurementsQueue = xStreamBufferCreate(2048 * sizeof(Measurements), 1024 * sizeof(Measurements));

	gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
	ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_NUM_32, gpio_isr_handler, (void*)(&ucGPIOParameter)));

	BaseType_t taskc_res;
	taskc_res = xTaskCreatePinnedToCore(i2c_reader, "Acc.meter loop", 10000, &ucParameterToPass, 20, &i2c_handle, 0);
	if(taskc_res != pdPASS){
		ESP_LOGE("main", "Could not start task Accelerometer loop");
		return;
	}
	taskc_res = xTaskCreatePinnedToCore(measurement_handler, "FFT loop", 15000, &ucParameterToPass, 2, &fft_handle, 1);
	if(taskc_res != pdPASS){
		ESP_LOGE("main", "Could not start task FFT loop");
		return;
	}
}
