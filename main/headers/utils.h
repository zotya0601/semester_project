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

#endif