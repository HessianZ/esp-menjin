#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <esp_tls.h>
#include "esp_system.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "smartconfig.h"
#include "settings.h"

static const char *TAG = "MQTT";

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");
void mqtt_task(void *pvParameters)
{
    sys_param_t *settings = settings_get_parameter();

    if (strlen(settings->mqtt_url) == 0) {
        ESP_LOGE(TAG, "mqtt_url is empty, mqtt client will not start");
        vTaskDelete(NULL);
        return;
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = settings->mqtt_url,
        .cert_pem = &server_root_cert_pem_start,
        .cert_len = server_root_cert_pem_end - server_root_cert_pem_start,
//        .skip_cert_common_name_check = true,
//        .use_global_ca_store = true,
    };

    if (strlen(settings->mqtt_username) > 0) {
        mqtt_cfg.username = settings->mqtt_username;
    }

    if (strlen(settings->mqtt_password) > 0) {
        mqtt_cfg.password = settings->mqtt_password;
    }

    ESP_LOGI(TAG, "check wifi connected");
    while (smartconfig_status() != WIFI_STATUS_CONNECTED) {
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    ESP_LOGI(TAG, "wifi ok, start mqtt_client");

    ESP_LOGI(TAG, "mqtt_cfg.uri: %s", mqtt_cfg.uri);
    ESP_LOGI(TAG, "mqtt_cfg.username: %s", mqtt_cfg.username);
    ESP_LOGI(TAG, "mqtt_cfg.password: %s", mqtt_cfg.password);

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
    ESP_LOGI(TAG, "mqtt_client started");
    vTaskDelete(NULL);
}
