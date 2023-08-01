//
// Created by Hessian on 2023/7/30.
//
#include "freertos/FreeRTOS.h"

#include "esp_log.h"
#include "esp_err.h"

#include "custom_i2c.h"
#include "settings.h"


static const char *TAG = "APP_MENJIN";

#define MENJIN_I2C_SCL_PIN           5                /*!< gpio number for I2C master clock */
#define MENJIN_I2C_SDA_PIN           4                /*!< gpio number for I2C master data  */
#define MENJIN_I2C_NUM               I2C_NUM_0        /*!< I2C port number for master dev */

#define WRITE_BIT                    I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                     I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN                 0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                0x0              /*!< I2C master will not check ack from slave */
#define ACK_VAL                      0x0              /*!< I2C ack value */
#define NACK_VAL                     0x1              /*!< I2C nack value */
#define LAST_NACK_VAL                0x2              /*!< I2C last_nack value */


/**
 * @brief i2c master initialization
 */
esp_err_t menjin_init()
{
    ESP_LOGI(TAG, "Initializing Menjin...");

    sys_param_t *settings = settings_get_parameter();

    if (settings->i2c_clock > 0) {
        i2c_master_set_clock(settings->i2c_clock);
    }

    int i2c_master_port = MENJIN_I2C_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = MENJIN_I2C_SDA_PIN;
    conf.sda_pullup_en = 1;
    conf.scl_io_num = MENJIN_I2C_SCL_PIN;
    conf.scl_pullup_en = 1;
    conf.clk_stretch_tick = 300; // 300 ticks, Clock stretch is about 210us, you can make changes according to the actual situation.
    ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, conf.mode));
    ESP_ERROR_CHECK(i2c_param_config(i2c_master_port, &conf));
    return ESP_OK;
}

esp_err_t menjin_stop()
{
    return i2c_driver_delete(MENJIN_I2C_NUM);
}

/**
 * @brief code to write by i2c
 *
 * 1. send data
 * _______________________________________________________________________
 * |  start | slave_addr + wr_bit + ack | write data byte + ack  | stop |
 * |--------|---------------------------|------------------------|------|
 *
 * @param i2c_num I2C port number
 * @param data data to send
 * @param data_len data length
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Parameter error
 *     - ESP_FAIL Sending command error, slave doesn't ACK the transfer.
 *     - ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
 *     - ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
 */
esp_err_t menjin_cmd_write(uint8_t data)
{
    sys_param_t *settings = settings_get_parameter();

    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, settings->i2c_address << 1 | WRITE_BIT, ACK_CHECK_DIS);
    i2c_master_write_byte(cmd, data, ACK_CHECK_DIS);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(MENJIN_I2C_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

uint32_t menjin_get_clock()
{
    return i2c_master_get_clock();
}

void menjin_set_clock(uint32_t clock)
{
    i2c_master_set_clock(clock);
}
