#ifndef UTILS__H
#define UTILS__H

#include "freertos/FreeRTOS.h"

inline TickType_t ms_to_ticks(uint32_t ms){ return (ms/portTICK_PERIOD_MS); }

inline uint8_t i2c_read_address(uint8_t address){
	return (address << 1) | 0x01;
}

inline uint8_t i2c_write_address(uint8_t address){
	return (address << 1) & 0xfe;
}

static const float GRAVITY_CONST = 9.8f;

/*
inline float g_to_acceleration(float g){
	return g * GRAVITY_CONST;
}

void set_intr_type(gpio_config_t *conf, gpio_int_type_t val){
	conf->intr_type = val;
}

void set_mode(gpio_config_t *conf, gpio_mode_t val){
	conf->mode = val;
}

void set_pin_bit_mask(gpio_config_t *conf, uint64_t val){
	conf->pin_bit_mask = val;
}

void set_pull_down_en(gpio_config_t *conf, gpio_pulldown_t val){
	conf->pull_down_en = val;
}

void set_pull_up_en(gpio_config_t *conf, gpio_pulldown_t val){
	conf->pull_up_en = val;
}
*/

#endif