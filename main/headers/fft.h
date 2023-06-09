#ifndef FFT__H
#define FFT__H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/stream_buffer.h"
#include "freertos/queue.h"

#include "headers/utils.h"

#ifdef CONFIG_FFT_TASK_QUEUE_LENGTH
    #define TASK_QUEUE_LENGTH CONFIG_FFT_TASK_QUEUE_LENGTH
#else
    #define TASK_QUEUE_LENGTH 10
#endif

extern TaskHandle_t fft_handle;
extern QueueHandle_t FFT_TaskQueue;

bool fft_add_job_to_queue(FFTJob job);
void fft_task(void *params);

#endif