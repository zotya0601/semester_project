#include <stdio.h>
#include "esp_log.h"
#include "esp_intr_alloc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/i2c.h"
#include "driver/gpio.h"

#include "headers/mpu6050.h"
#include "headers/sensor_i2c_comm_conf.h"
#include "headers/utils.h"

static const char *TAG = "LABWORK_ESP";

#define GPIO_INT_IN (1ULL << GPIO_NUM_32)
#define GPIO_INT_OUT (1ULL << GPIO_NUM_33)

static SemaphoreHandle_t xSemaphore = NULL;

void i2c_reader(void *params){
	const char *I2C_TAG = "I2C Loop";
	const TickType_t xDelay = ms_to_ticks(50);

	init_mpu6050();

	ESP_LOGI(I2C_TAG, "Running i2c reader loop...");
	while(true){
		float measurements_f[3] = {0};
		read_acc_registers(measurements_f);

		// for(int i = 0; i < 3; i++){ measurements_f[i] = g_to_acceleration(measurements_f[i]); }
		ESP_LOGI(I2C_TAG, "Acc X: %f m/s^2", measurements_f[0]);
		ESP_LOGI(I2C_TAG, "Acc Y: %f m/s^2", measurements_f[1]);
		ESP_LOGI(I2C_TAG, "Acc Z: %f m/s^2", measurements_f[2]);
		ESP_LOGI(I2C_TAG, "------------------------------");

		vTaskDelay( xDelay );
	}
}

static void IRAM_ATTR gpio_isr_handler(void* arg) {
	uint32_t num = 0;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);
}

static void gpio_task_example(void *arg){
	int level = 0;
	for(;;){
		if(xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE){
			uint32_t num;
			ESP_LOGW("gpio_task_example", "RUNNING");
			if(level == 0)
			{ 
				level = 1; 
			} else { 
				level = 0; 
			}
			ESP_LOGW("gpio_task_example", "Level: %d", level);
			gpio_set_level(GPIO_NUM_33, level);
			// vTaskDelay(ms_to_ticks(200));
		}
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
	static uint8_t ucParameterToPass = 0;
	static uint8_t ucGPIOParameter = GPIO_NUM_32;

	//zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_INT_OUT;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings	
    gpio_config(&io_conf);

    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INT_IN;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

	initialize_int_receiver(GPIO_NUM_32);
	
	xSemaphore = xSemaphoreCreateBinary();

	ESP_LOGW("AAAAA", "3");
	gpio_install_isr_service(0);
	ESP_LOGW("AAAAA", "4");
	ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_NUM_32, gpio_isr_handler, (void*)(&ucGPIOParameter)));
	ESP_LOGW("AAAAA", "5");

	xTaskCreate(i2c_reader, "Accelerometer loop", 10000, &ucParameterToPass, 10, &i2c_handle);
	xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);
}
