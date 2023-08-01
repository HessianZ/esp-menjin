//
// Created by Hessian on 2023/7/30.
//

#ifndef ESP_MENJIN_APP_MENJIN_H
#define ESP_MENJIN_APP_MENJIN_H

#include <esp_err.h>

enum {
    MENJIN_CMD_KEY4_SPEAKER = 0x61,
    MENJIN_CMD_KEY2,
    MENJIN_CMD_KEY3_UNLOCK,
    MENJIN_CMD_KEY1,
} MENJIN_CMD;

esp_err_t menjin_init();
esp_err_t menjin_stop();
esp_err_t menjin_cmd_write(uint8_t data);
uint32_t menjin_get_clock();
void menjin_set_clock(uint32_t clock);

#endif //ESP_MENJIN_APP_MENJIN_H