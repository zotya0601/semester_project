#ifndef SENSOR_I2C_COMM_CONF__H
#define SENSOR_I2C_COMM_CONF__H

#include <stdint.h>
#include "esp_err.h"
#include "mpu6050.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_MASTER_SCL_IO           CONFIG_I2C_MASTER_SCL_IO      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           CONFIG_I2C_MASTER_SDA_IO      /*!< GPIO number used for I2C master data  */
#define I2C_INT_PIN                 CONFIG_SENSOR_INT_INPUT

#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define MPU6050_ADDRESS 0b1101000

#define TIMEOUT (I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS)

TaskHandle_t i2c_handle;
StreamBufferHandle_t MeasurementQueue_AxisX = NULL;
StreamBufferHandle_t MeasurementQueue_AxisY = NULL;
StreamBufferHandle_t MeasurementQueue_AxisZ = NULL;

void i2c_reader_task(void *params);
/*
void init_mpu6050();
esp_err_t read_acc_registers(float *res);
esp_err_t read_acc_registers_structured(Measurements *m);
*/

#endif