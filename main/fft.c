#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_dsp.h"

#include "headers/fft.h"

#define READINGS 4096

TaskHandle_t fft_handle;
QueueHandle_t FFT_TaskQueue;

bool fft_add_job_to_queue(FFTJob job){
	BaseType_t res = xQueueSendToBack(FFT_TaskQueue, &job, portMAX_DELAY);
	return (res == pdTRUE);
}

void fft_task(void *params){
	FFT_TaskQueue = xQueueCreate(TASK_QUEUE_LENGTH, sizeof(FFTJob));

	ESP_ERROR_CHECK(dsps_fft2r_init_fc32(NULL, READINGS));

	__attribute__((aligned(16)))
	float buffer[READINGS * 2] = {0};	// 8192 byte
	__attribute__((aligned(16)))
	float wind[READINGS * 2] = {0};	// 8192 byte
	dsps_wind_hann_f32(wind, READINGS);

	while(true){
		FFTJob job;
		if( xQueueReceive(FFT_TaskQueue, &job, portMAX_DELAY) == pdTRUE ){
			if(job.buffer_len > READINGS){
				continue;
			}

			int nComplex = job.nBuffers * job.buffer_len;
			float *res = (float*)malloc(nComplex * sizeof(Complex));
			memset(res, 0, nComplex * sizeof(Complex));

			float *ptr = NULL;

			for(int i = 0; i < job.nBuffers; i++){
				xStreamBufferReceive(job.buffers[i], buffer, job.buffer_len * sizeof(Complex), portMAX_DELAY);
				
				ptr = res + (i * job.buffer_len);

				dsps_fft2r_fc32(buffer, job.buffer_len);
				dsps_bit_rev_fc32(buffer, job.buffer_len);	
				dsps_cplx2reC_fc32(buffer, job.buffer_len);
				
				memcpy(ptr, buffer, job.buffer_len * sizeof(float));
			}

			// Kapott komplex számok mennyiségének a fele, szorozva a bufferek számával -> Eredmény komplex számok
			int final_len = (nComplex / 2);
			for(int i = 0; i < final_len; i++){
				res[i] = 10 * log10f((res[i * 2 + 0] * res[i * 2 + 0] + res[i * 2 + 1] * res[i * 2 + 1])/job.buffer_len);
			}

			free(job.buffers);

			if(job.callback != NULL){
				job.callback(res, final_len);
			}
		}
	}
}