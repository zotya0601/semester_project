#include "headers/mpu6050.h"

void initialize_int_receiver(gpio_num_t gpio_pin){
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << gpio_pin),
        .pull_down_en = 0,
        .pull_up_en = 1
    };
    gpio_config(&io_conf);
}