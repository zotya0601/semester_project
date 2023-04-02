#ifndef MICROPHONE__H
#define MICROPHONE__H

#include "esp_adc/adc_continuous.h"
#include "esp_err.h"

#define ADC_UNIT            ADC_UNIT_1
#define ADC_READ_LEN        CONFIG_READ_LEN
#define ADC_BUFFER_SIZE     CONFIG_ADC_BUFFER_SIZE
#define SAMPLE_RATE         CONFIG_SAMPLE_RATE


#if CONFIG_ADC1_GPIO_32
    #define MICROPHONE_GPIO ADC_CHANNEL_4
#elif CONFIG_ADC1_GPIO_33
    #define MICROPHONE_GPIO ADC_CHANNEL_5
#elif CONFIG_ADC1_GPIO_34
    #define MICROPHONE_GPIO ADC_CHANNEL_6
#elif CONFIG_ADC1_GPIO_35
    #define MICROPHONE_GPIO ADC_CHANNEL_7
#elif CONFIG_ADC1_GPIO_36
    #define MICROPHONE_GPIO ADC_CHANNEL_0
#elif CONFIG_ADC1_GPIO_39
    #define MICROPHONE_GPIO ADC_CHANNEL_3
#endif

esp_err_t init_adc();

#endif