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
#include "headers/fft.h"

static const char *TAG = "LABWORK_ESP";

gpio_config_t led = {
	.pin_bit_mask = (1ULL << GPIO_NUM_25),
	.mode = GPIO_MODE_OUTPUT,
	.pull_up_en = 0,
	.pull_down_en = 0,
	.intr_type = GPIO_INTR_DISABLE
};

#define READINGS 1024	// float -> 4096 byte

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

	ESP_ERROR_CHECK(init_wifi(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD));
	wifi_start();

	init_mqtt(CONFIG_MQTT_URI, CONFIG_MQTT_TOPIC);
	mqtt_register_event_handler(mqtt_event_handler);
	mqtt_start();

	BaseType_t taskc_res;
	taskc_res = xTaskCreatePinnedToCore(fft_task, "FFT loop", 80000, &ucParameterToPass, 2, &fft_handle, tskNO_AFFINITY);
	if(taskc_res != pdPASS){
		ESP_LOGE("main", "Could not start task FFT loop");
		return;
	}
	
	taskc_res = xTaskCreatePinnedToCore(i2c_reader_task, "Acc.meter loop", 10000, &ucParameterToPass, 20, &i2c_handle, 1);
	if(taskc_res != pdPASS){
		ESP_LOGE("main", "Could not start task Accelerometer loop");
		return;
	}

	// init_adc();
}
