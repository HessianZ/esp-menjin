//
// Created by Hessian on 2023/7/29.
//

#pragma once

#include "esp_err.h"

typedef struct {
    char wifi_ssid[32];
    char wifi_password[64];
    char mqtt_url[64];
    char mqtt_username[32];
    char mqtt_password[64];
} sys_param_t;

esp_err_t settings_read_parameter_from_nvs(void);
esp_err_t settings_write_parameter_to_nvs(void);
sys_param_t *settings_get_parameter(void);
void settings_dump(void);
