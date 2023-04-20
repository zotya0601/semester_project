#include "headers/microphone.h"
#include "headers/utils.h"
#include "headers/fft.h"
#include "headers/mqtt.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_adc/adc_continuous.h"

#include "esp_dsp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include <string.h>

#define _EXAMPLE_ADC_UNIT_STR(unit)         #unit
#define EXAMPLE_ADC_UNIT_STR(unit)          _EXAMPLE_ADC_UNIT_STR(unit)

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
    #define EXAMPLE_ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE1
    #define EXAMPLE_ADC_GET_CHANNEL(p_data)     ((p_data)->type1.channel)
    #define EXAMPLE_ADC_GET_DATA(p_data)        ((p_data)->type1.data)
#endif

#define READINGS (ADC_READ_LEN / 2)

static TaskHandle_t task_handle = NULL;
static adc_continuous_handle_t adc_handle = NULL;

static void fft_mqtt_callback(void *data, int len){
    mqtt_publish(data, len * sizeof(float));
    free(data);
}

static StreamBufferHandle_t data_sbuf;

static void IRAM_ATTR handler(void *params){
    static const char *TAG = "ADC Handler method";
    static uint32_t ret_num = 0;
    
    static uint8_t result[ADC_READ_LEN] = {0};

    while(1){
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
        char unit[] = EXAMPLE_ADC_UNIT_STR(EXAMPLE_ADC_UNIT);

        ESP_LOGI(TAG, "Hanling ADC data");  
            esp_err_t ret = adc_continuous_read(adc_handle, result, ADC_READ_LEN, &ret_num, 0);
            if(ret == ESP_OK){
                ESP_LOGI("TASK", "ret is %x, ret_num is %"PRIu32, ret, ret_num);
                
                for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
                    adc_digi_output_data_t *p = (void*)&result[i];
                    uint32_t chan_num = EXAMPLE_ADC_GET_CHANNEL(p);
                    uint32_t data = EXAMPLE_ADC_GET_DATA(p);

                    data = (data & 0x00000fff);
                    float f_data = (float)data;
                    xStreamBufferSend(data_sbuf, &f_data, sizeof(float), portMAX_DELAY);
                    
                    continue;

                    /* Check the channel number validation, the data is invalid if the channel num exceed the maximum channel */
                    if (chan_num < SOC_ADC_CHANNEL_NUM(ADC_UNIT)) {
                        ESP_LOGI(TAG, "Unit: %s, Channel: %"PRIu32", Value: %"PRIx32, unit, chan_num, (data & 0x0fff));
                    } else {
                        ESP_LOGW(TAG, "Invalid data [%s_%"PRIu32"_%"PRIx32"]", unit, chan_num, data);
                    }
                }
                ESP_LOGI(TAG, "Measurements done");

                // xStreamBufferSend(data_sbuf, data_arr, ADC_READ_LEN * sizeof(float), portMAX_DELAY);
                StreamBufferHandle_t *handle = (StreamBufferHandle_t*)malloc(sizeof(StreamBufferHandle_t));
                FFTJob job = {
                    .nBuffers = 1,
                    .buffer_len = ADC_READ_LEN,
                    .buffers = handle,
                    .callback = fft_mqtt_callback
                };
                fft_add_job_to_queue(job);
                
            } else {
                ESP_LOGE(TAG, "Still bad...");
            }
    }
}

static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(task_handle, &mustYield);

    return (mustYield == pdTRUE);
}

esp_err_t init_adc(){
    esp_err_t err = ESP_OK;
    adc_continuous_handle_t handle = NULL;
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = ADC_BUFFER_SIZE,
        .conv_frame_size = ADC_READ_LEN,
    };

    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = SAMPLE_RATE,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
        .pattern_num = 1,
    };

    adc_unit_t unit;
    adc_channel_t channel;
    adc_continuous_io_to_channel(35, &unit, &channel);

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    adc_pattern[0].atten = ADC_ATTEN_DB_11;
    adc_pattern[0].channel = channel;
    adc_pattern[0].unit = unit;
    adc_pattern[0].bit_width = ADC_BITWIDTH_12;

    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

    adc_handle = handle;
    data_sbuf = xStreamBufferCreate(ADC_BUFFER_SIZE * sizeof(float), ADC_READ_LEN * sizeof(float));

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(adc_handle, &cbs, NULL));

    xTaskCreatePinnedToCore(handler, "ADC loop", 10000, NULL, 20, &task_handle, tskNO_AFFINITY);
    
    ESP_ERROR_CHECK(adc_continuous_start(adc_handle));

    return err;
}