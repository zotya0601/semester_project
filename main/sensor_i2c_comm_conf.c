#include "headers/sensor_i2c_comm_conf.h"
#include "headers/utils.h"
#include "headers/mpu6050.h"
#include "headers/fft.h"
#include "headers/mqtt.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_dsp.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

#include <string.h>
#include <math.h>

static const uint8_t SAMPLE_RATE_DIVIDER[2] = 
	{SAMPLE_RATE_DIVIDER_REG, SAMPLE_RATE_DIVIDER_1KHZ};	// Default: 0x00
static const uint8_t FSYNC_DLPF_CONF[2] = 
	{FSYNC_DLPF_CONF_REG, (FSYNC_DLPF_CONF_EXT_SYNC_GYRO_X | FSYNC_DLPF_CONF_DLPF_OFF)}; // Strong DLPF: 0b00101110
static const uint8_t ACCEL_CONFIG[2] = 
	{ACCEL_CONFIG_REG, ACCEL_CONFIG_RANGE_8G};
static const uint8_t PWR_MGMT_1[2] = 
	{PWR_MGMT_1_REG, PWR_MGMT_1_CLKSEL_GYRO_X};

static const uint8_t INT_PIN_CFG_1[2] = 
	{INT_PIN_CFG_1_REG, INT_PIN_CFG_1_INT_RD_CLEAR_ANY_READ | INT_PIN_CFG_1_INT_OPEN_OD};
static const uint8_t INT_PIN_CFG_2[2] = {0x38, 0b00000001};

static void init_mpu6050() {
	i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDRESS, SAMPLE_RATE_DIVIDER, 2, TIMEOUT);
	i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDRESS, FSYNC_DLPF_CONF, 2, TIMEOUT);
	i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDRESS, PWR_MGMT_1, 2, TIMEOUT);

	i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDRESS, ACCEL_CONFIG, 2, TIMEOUT);

	i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDRESS, INT_PIN_CFG_1, 2, TIMEOUT);
	i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_ADDRESS, INT_PIN_CFG_2, 2, TIMEOUT);

}

static esp_err_t read_acc_registers(float *res){
	uint8_t accel_reg_1 = 0x3b;
	uint8_t buf[6] = {0};
	i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDRESS, &accel_reg_1, 1, buf, 6, TIMEOUT);

	uint8_t rev_byte_order[6];
	rev_byte_order[0] = buf[1];
	rev_byte_order[1] = buf[0];
	rev_byte_order[2] = buf[3];
	rev_byte_order[3] = buf[2];
	rev_byte_order[4] = buf[5];
	rev_byte_order[5] = buf[4];
	
	int16_t *temp = (void*)(rev_byte_order);
	/*
	int16_t temp[3];
	memcpy(temp, rev_byte_order, 6);
	*/
	
	res[0] = ((float)temp[0]) / 4096;
	res[1] = ((float)temp[1]) / 4096;
	res[2] = ((float)temp[2]) / 4096;
	
	return ESP_OK;

}

static esp_err_t read_acc_registers_structured(Measurements *m){
	uint8_t accel_reg_1 = 0x3b;
	uint8_t buf[6] = {0};
	i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDRESS, &accel_reg_1, 1, buf, 6, TIMEOUT);

	uint8_t rev_byte_order[6];
	rev_byte_order[0] = buf[1];
	rev_byte_order[1] = buf[0];
	rev_byte_order[2] = buf[3];
	rev_byte_order[3] = buf[2];
	rev_byte_order[4] = buf[5];
	rev_byte_order[5] = buf[4];

	int16_t *temp = (void*)(rev_byte_order);

	m->x.real = ((float)temp[0]) / 4096;
	m->y.real = ((float)temp[1]) / 4096;
	m->z.real = ((float)temp[2]) / 4096;
	m->x.cplx = m->y.cplx = m->z.cplx = 0;

	return ESP_OK;
}

static void IRAM_ATTR gpio_isr_handler(void* arg) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	vTaskNotifyGiveFromISR(i2c_handle, &xHigherPriorityTaskWoken);
	
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );	
}

static void fft_callback(void *res, int len){
	Complex *fft_res = (Complex*)res;
	int buf_len = len / 3;
	Complex *x = fft_res + 0;
	Complex *y = fft_res + buf_len;
	Complex *z = fft_res = (buf_len * 2);

	float *data = (float*)malloc(len * sizeof(float));
	for(int i = 0; i < buf_len; i+=3){
		data[i + 0] = x[i].real;
		data[i + 1] = y[i].real;
		data[i + 2] = z[i].real;
	}

	mqtt_publish(data, len * sizeof(float));
	free(res);
	free(data);
}

void i2c_reader_task(void *params){
	const char *I2C_TAG = DRAM_STR("I2C Loop");

	i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

	int i2c_master_port = I2C_MASTER_NUM;

    i2c_param_config(i2c_master_port, &conf);

    ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0));

    ESP_LOGI(I2C_TAG, "I2C initialized successfully");

	static DRAM_ATTR int16_t queueLength = 0;
	
	MeasurementQueue_AxisX = xStreamBufferCreate(2048 * sizeof(Complex), 1024 * sizeof(Complex));
	MeasurementQueue_AxisY = xStreamBufferCreate(2048 * sizeof(Complex), 1024 * sizeof(Complex));
	MeasurementQueue_AxisZ = xStreamBufferCreate(2048 * sizeof(Complex), 1024 * sizeof(Complex));

	int LED_state = 0;

	init_mpu6050();
	initialize_int_receiver(I2C_INT_PIN);

	static uint8_t ucGPIOParameter = I2C_INT_PIN;
	gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
	ESP_ERROR_CHECK(gpio_isr_handler_add(I2C_INT_PIN, gpio_isr_handler, (void*)(&ucGPIOParameter)));

	ESP_LOGI(I2C_TAG, "Running i2c reader loop...");
	
	while(true){
		if( ulTaskNotifyTake( pdTRUE, portMAX_DELAY ) ) {
			Measurements m;
			read_acc_registers_structured(&m);
			
			bool error_happened = false;
			if( xStreamBufferSend(MeasurementQueue_AxisX, &(m.x), sizeof(Complex), portMAX_DELAY) == 0 ){
				ESP_LOGE(I2C_TAG, "Buffer X is full");
				error_happened = true;
			}
			if( xStreamBufferSend(MeasurementQueue_AxisY, &(m.y), sizeof(Complex), portMAX_DELAY) == 0 ){
				ESP_LOGE(I2C_TAG, "Buffer Y is full");
				error_happened = true;
			}
			if( xStreamBufferSend(MeasurementQueue_AxisZ, &(m.z), sizeof(Complex), portMAX_DELAY) == 0 ){
				ESP_LOGE(I2C_TAG, "Buffer Z is full");
				error_happened = true;
			}

			if(!error_happened){
				queueLength++;
				if(queueLength == 1024){
					StreamBufferHandle_t *buffers = (StreamBufferHandle_t*)malloc(3 * sizeof(StreamBufferHandle_t));
					buffers[0] = MeasurementQueue_AxisX;
					buffers[1] = MeasurementQueue_AxisY;
					buffers[2] = MeasurementQueue_AxisZ;

					FFTJob job = {
						.buffer_len = 1024
						.nBuffers = 3,
						.buffers = buffers,
						.callback = fft_callback
					};
					bool success = fft_add_job_to_queue(job);
					if(!success){
						ESP_LOGE(I2C_TAG, "Could not add job to FFT Jobqueue");
					}

					queueLength = 0;
					
					gpio_set_level(GPIO_NUM_25, LED_state);
					LED_state = !LED_state;
				}
			}
		}
	}
}