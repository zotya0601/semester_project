#ifndef UTILS__H
#define UTILS__H

#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"

inline TickType_t ms_to_ticks(uint32_t ms){ return (ms/portTICK_PERIOD_MS); }

inline uint8_t i2c_read_address(uint8_t address){
	return (address << 1) | 0x01;
}

inline uint8_t i2c_write_address(uint8_t address){
	return (address << 1) & 0xfe;
}

static const float GRAVITY_CONST = 9.8f;

// typedef struct {
// 	float real, cplx;
// } Complex;

typedef void (*job_callback)(void*, int);

typedef struct _FFTJob{
	int nBuffers;					// Number of buffers
	StreamBufferHandle_t *buffers;	// The buffers
	int buffer_len;					// One buffer's length - Number of *Complex numbers* inside it
	job_callback callback;			// Callback
} FFTJob;

#endif