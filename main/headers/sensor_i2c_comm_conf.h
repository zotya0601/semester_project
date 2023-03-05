#ifndef SENSOR_I2C_COMM_CONF__H
#define SENSOR_I2C_COMM_CONF__H

#define I2C_MASTER_SCL_IO           26      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           25      /*!< GPIO number used for I2C master data  */

#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define MPU6050_ADDRESS 0b1101000

void init_mpu6050();

#endif