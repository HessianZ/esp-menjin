/* Esptouch example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_smartconfig.h"
#include "system/smartconfig.h"
#include "system/mqtt.h"
#include "system/settings.h"
#include "http_server.h"

static const char *TAG = "APP_MAIN";

extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");

static void init_wifi_task(void *param)
{
    ESP_LOGI(TAG, "Initializing WiFi...");
    smartconfig_init(NULL);
    vTaskDelete(NULL);
}

void app_main()
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    esp_log_level_set("MQTT", ESP_LOG_VERBOSE);
    esp_log_level_set("smartconfig", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(settings_read_parameter_from_nvs());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

//    ESP_ERROR_CHECK(esp_tls_init_global_ca_store());
//    esp_err_t err = esp_tls_set_global_ca_store(&server_root_cert_pem_start, server_root_cert_pem_end - server_root_cert_pem_start);
//    ESP_ERROR_CHECK(err);

    xTaskCreate(init_wifi_task, "init_wifi_task", 4096, NULL, 3, NULL);
    xTaskCreate(mqtt_task, "mqtt_task", 4096, NULL, 3, NULL);

    http_server_init();
}

