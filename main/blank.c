#include <stdio.h>
#include <math.h>

#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "esp_dsp.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"

#include "driver/i2c.h"
#include "driver/gpio.h"

#include "headers/mpu6050.h"
#include "headers/sensor_i2c_comm_conf.h"
#include "headers/utils.h"
#include "headers/mqtt.h"
#include "headers/wifi.h"
#include "headers/microphone.h"

static const char *TAG = "LABWORK_ESP";

static IRAM_ATTR SemaphoreHandle_t xInterruptSemaphore = NULL;
static IRAM_ATTR SemaphoreHandle_t xQueueDataReadySemaphore = NULL;
static StreamBufferHandle_t MeasurementsQueue = NULL;

gpio_config_t led = {
	.pin_bit_mask = (1ULL << GPIO_NUM_25),
	.mode = GPIO_MODE_OUTPUT,
	.pull_up_en = 0,
	.pull_down_en = 0,
	.intr_type = GPIO_INTR_DISABLE
};

#define READINGS 1024	// float -> 4096 byte

static volatile uint64_t start_time = 0;

static TaskHandle_t i2c_handle = 0;
static TaskHandle_t fft_handle = 0;


// 80kByte Stack (elféraz, jolesz)
// Tudom takarékosabbá tenni
void measurement_handler(void *params){
	while(true){
		ESP_ERROR_CHECK(dsps_fft2r_init_fc32(NULL, READINGS));

		__attribute__((aligned(16)))
		float x[READINGS * 2] = {0};	// 8192 byte
		__attribute__((aligned(16)))
		float y[READINGS * 2] = {0};	// 8192 byte
		__attribute__((aligned(16)))
		float z[READINGS * 2] = {0};	// 8192 byte

		__attribute__((aligned(16)))
		float wind[READINGS * 2] = {0};	// 8192 byte
		dsps_wind_hann_f32(wind, READINGS);
		
		// if( xSemaphoreTake( xQueueDataReadySemaphore, portMAX_DELAY ) == pdTRUE ){
		if( ulTaskNotifyTake( pdTRUE, portMAX_DELAY ) ){
			Measurements buffer[1024];	// 4096 byte
			xStreamBufferReceive(MeasurementsQueue, buffer, 1024 * sizeof(Measurements), portMAX_DELAY);

			for(int i = 0; i < READINGS; i++){
				x[i*2] = buffer[i].x;
				y[i*2] = buffer[i].y;
				z[i*2] = buffer[i].z;
			}
			dsps_fft2r_fc32(x, READINGS);
			dsps_fft2r_fc32(y, READINGS);
			dsps_fft2r_fc32(z, READINGS);

			dsps_bit_rev_fc32(x, READINGS);
			dsps_bit_rev_fc32(y, READINGS);
			dsps_bit_rev_fc32(z, READINGS);

			dsps_cplx2reC_fc32(x, READINGS);
			dsps_cplx2reC_fc32(y, READINGS);
			dsps_cplx2reC_fc32(z, READINGS);

			float res[(READINGS / 2) * 3] = {0};	// 6144 byte
			for(int i = 0; i < READINGS / 2; i+=3){
				res[i + 0] = 10 * log10f((x[i * 2 + 0] * x[i * 2 + 0] + x[i * 2 + 1] * x[i * 2 + 1])/READINGS);
				res[i + 1] = 10 * log10f((y[i * 2 + 0] * y[i * 2 + 0] + y[i * 2 + 1] * y[i * 2 + 1])/READINGS);
				res[i + 2] = 10 * log10f((z[i * 2 + 0] * z[i * 2 + 0] + z[i * 2 + 1] * z[i * 2 + 1])/READINGS);

				// y[i] = 10 * log10f((y[i * 2 + 0] * y[i * 2 + 0] + y[i * 2 + 1] * y[i * 2 + 1])/READINGS);
			}

			// dsps_view_spectrum(y, READINGS / 2, -1000, 1000);
			mqtt_publish((uint8_t*)res, (READINGS / 2) * 3);
		}
		dsps_fft2r_deinit_fc32();
	}
}

static void IRAM_ATTR gpio_isr_handler(void* arg) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	vTaskNotifyGiveFromISR(i2c_handle, &xHigherPriorityTaskWoken);
	// xSemaphoreGiveFromISR(xInterruptSemaphore, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );	
}

void i2c_reader(void *params){
	const char *I2C_TAG = DRAM_STR("I2C Loop");

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

	static DRAM_ATTR int16_t queueLength = 0;
	int LED_state = 0;

	init_mpu6050();
	initialize_int_receiver(I2C_INT_PIN);

	static uint8_t ucGPIOParameter = I2C_INT_PIN;
	gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
	ESP_ERROR_CHECK(gpio_isr_handler_add(I2C_INT_PIN, gpio_isr_handler, (void*)(&ucGPIOParameter)));

	ESP_LOGI(I2C_TAG, "Running i2c reader loop...");
	
	start_time = esp_timer_get_time();
	while(true){
		// if( xSemaphoreTake( xInterruptSemaphore, portMAX_DELAY ) == pdTRUE) {
		if( ulTaskNotifyTake( pdTRUE, portMAX_DELAY ) ) {
			Measurements m;
			read_acc_registers_structured(&m);
			size_t wbytes = xStreamBufferSend(MeasurementsQueue, &m, sizeof(Measurements), portMAX_DELAY);
			
			if(wbytes != 0){
				queueLength++;
				if(queueLength == 1024){
					uint64_t stop_time = esp_timer_get_time();
					xTaskNotifyGive( fft_handle );
					// xSemaphoreGive(xQueueDataReadySemaphore);
					float period_time = ((float)(stop_time - start_time)) * 1000000.0f;	// Time it takes to read 1024 samples
					float sample_rate = ((float)queueLength) / 1.0f / period_time;
					queueLength = 0;
					ESP_LOGI(I2C_TAG, "Sample time was: %f Hz", sample_rate);
					gpio_set_level(GPIO_NUM_25, LED_state);
					LED_state = !LED_state;

					start_time = stop_time;
				}
			}
			else {
				ESP_LOGW(I2C_TAG, "StreamBuffer length exceeded!");
			}
		}
	}
}

void app_main(void)
{
	esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	static uint8_t ucParameterToPass = 0;

	gpio_config(&led);
	
	MeasurementsQueue = xStreamBufferCreate(2048 * sizeof(Measurements), 1024 * sizeof(Measurements));

	ESP_ERROR_CHECK(init_wifi(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD));
	wifi_start();

	init_mqtt(CONFIG_MQTT_URI, CONFIG_MQTT_TOPIC);
	mqtt_register_event_handler(mqtt_event_handler);
	mqtt_start();

	init_adc();

	BaseType_t taskc_res;
	taskc_res = xTaskCreatePinnedToCore(i2c_reader, "Acc.meter loop", 10000, &ucParameterToPass, 20, &i2c_handle, 1);
	if(taskc_res != pdPASS){
		ESP_LOGE("main", "Could not start task Accelerometer loop");
		return;
	}

	taskc_res = xTaskCreatePinnedToCore(measurement_handler, "FFT loop", 80000, &ucParameterToPass, 2, &fft_handle, tskNO_AFFINITY);
	if(taskc_res != pdPASS){
		ESP_LOGE("main", "Could not start task FFT loop");
		return;
	}
}
