#include <stdlib.h>
#include <stdbool.h>

#include <esp_log.h>
#include <esp_system.h>
#include <esp_http_server.h>
#include "esp_event.h"
#include <cJSON.h>
#include <esp_event_base.h>

#include "http_server.h"
#include "json_parser.h"
#include "settings.h"

static const char *TAG="HTTP_SERVER";

int pre_start_mem, post_stop_mem, post_stop_min_mem;
bool basic_sanity = true;

struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
};

/********************* Basic Handlers Start *******************/

esp_err_t api_handler_get_settings(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET /api/settings handler read content length %d", req->content_len);

    char*  buf = malloc(512);

    sys_param_t *settings = settings_get_parameter();

    int len = sprintf(buf, "{"
                 "\"wifi_ssid:\": \"%s\","
                 "\"wifi_password:\": \"%s\","
                 "\"mqtt_url:\": \"%s\","
                 "\"mqtt_username:\": \"%s\","
                 "\"mqtt_password:\": \"%s\""
                 "}",
                settings->wifi_ssid,
                settings->wifi_password,
                settings->mqtt_url,
                settings->mqtt_username,
                settings->mqtt_password
     );

    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_send(req, buf, len);
    free (buf);
    return ESP_OK;
}
esp_err_t api_handler_post_settings(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST /api/settings handler read content length %d", req->content_len);

    char*  buf = malloc(req->content_len + 1);
    size_t off = 0;
    int    ret;

    if (!buf) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    while (off < req->content_len) {
        /* Read data received in the request */
        ret = httpd_req_recv(req, buf + off, req->content_len - off);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            free (buf);
            return ESP_FAIL;
        }
        off += ret;
        ESP_LOGI(TAG, "/api/settings handler recv length %d", ret);
    }
    buf[off] = '\0';

    ESP_LOGI(TAG, "/api/settings handler read %s", buf);

    jparse_ctx_t *jctx = NULL;
    jctx = (jparse_ctx_t *)malloc(sizeof(jparse_ctx_t));
    ret = json_parse_start(jctx, buf, req->content_len);
    if (ret != OS_SUCCESS) {
        ESP_LOGE(TAG, "json_parse_start failed\n");
        httpd_resp_set_status(req, HTTPD_500);
        httpd_resp_send(req, "json parse failed", 18);
        return ESP_OK;
    }

    sys_param_t *settings = settings_get_parameter();
    char value[128];
    if (json_obj_get_string(jctx, "wifi_ssid", &value, sizeof(settings->wifi_ssid)) == OS_SUCCESS) {
        strcpy(settings->wifi_ssid, value);
    }
    if (json_obj_get_string(jctx, "wifi_password", &value, sizeof(settings->wifi_password)) == OS_SUCCESS) {
        strcpy(settings->wifi_password, value);
    }
    if (json_obj_get_string(jctx, "mqtt_url", &value, sizeof(settings->mqtt_url)) == OS_SUCCESS) {
        strcpy(settings->mqtt_url, value);
    }
    if (json_obj_get_string(jctx, "mqtt_url", &value, sizeof(settings->mqtt_url)) == OS_SUCCESS) {
        strcpy(settings->mqtt_url, value);
    }
    if (json_obj_get_string(jctx, "mqtt_username", &value, sizeof(settings->mqtt_username)) == OS_SUCCESS) {
        strcpy(settings->mqtt_username, value);
    }
    if (json_obj_get_string(jctx, "mqtt_password", &value, sizeof(settings->mqtt_password)) == OS_SUCCESS) {
        strcpy(settings->mqtt_password, value);
    }

    ESP_ERROR_CHECK(settings_write_parameter_to_nvs());

    settings_dump();

    httpd_resp_send(req, buf, req->content_len);
    json_parse_end(jctx);
    free (buf);
    return ESP_OK;
}

static void restart_task(void *arg)
{
    vTaskDelay(200 / portTICK_PERIOD_MS);
    esp_restart();
}

esp_err_t api_handler_restart(httpd_req_t *req)
{
    ESP_LOGW(TAG, "Restarting the device");

    httpd_resp_send(req, "ok", 2);
    xTaskCreate(restart_task, "restart_task", 2048, NULL, 5, NULL);
    return ESP_OK;
}

esp_err_t api_handler_info(httpd_req_t *req)
{
#define STR "ESP32 MQTT Relay v1.0"
    ESP_LOGI(TAG, "/api/info handler read content length %d", req->content_len);

    httpd_resp_send(req, STR, strlen(STR));
    return ESP_OK;
#undef STR
}


httpd_uri_t basic_handlers[] = {
    { .uri      = "/api/settings",
            .method   = HTTP_GET,
            .handler  = api_handler_get_settings,
            .user_ctx = NULL,
    },
    { .uri      = "/api/settings",
            .method   = HTTP_POST,
            .handler  = api_handler_post_settings,
            .user_ctx = NULL,
    },
    { .uri      = "/api/restart",
      .method   = HTTP_GET,
      .handler  = api_handler_restart,
      .user_ctx = NULL,
    },
    { .uri      = "/api/info",
      .method   = HTTP_GET,
      .handler  = api_handler_info,
      .user_ctx = NULL,
    }
};

int basic_handlers_no = sizeof(basic_handlers)/sizeof(httpd_uri_t);
void register_basic_handlers(httpd_handle_t hd)
{
    int i;
    ESP_LOGI(TAG, "Registering basic handlers");
    ESP_LOGI(TAG, "No of handlers = %d", basic_handlers_no);
    for (i = 0; i < basic_handlers_no; i++) {
        if (httpd_register_uri_handler(hd, &basic_handlers[i]) != ESP_OK) {
            ESP_LOGW(TAG, "register uri failed for %d", i);
            return;
        }
    }
    ESP_LOGI(TAG, "Success");
}

httpd_handle_t http_server_start()
{
    pre_start_mem = esp_get_free_heap_size();
    httpd_handle_t hd;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    /* This check should be a part of http_server */
    config.max_open_sockets = (CONFIG_LWIP_MAX_SOCKETS - 3);

    if (httpd_start(&hd, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Started HTTP server on port: '%d'", config.server_port);
        ESP_LOGI(TAG, "Max URI handlers: '%d'", config.max_uri_handlers);
        ESP_LOGI(TAG, "Max Open Sessions: '%d'", config.max_open_sockets);
        ESP_LOGI(TAG, "Max Header Length: '%d'", HTTPD_MAX_REQ_HDR_LEN);
        ESP_LOGI(TAG, "Max URI Length: '%d'", HTTPD_MAX_URI_LEN);
        ESP_LOGI(TAG, "Max Stack Size: '%d'", config.stack_size);
        return hd;
    }
    return NULL;
}

static httpd_handle_t server = NULL;
static bool http_network_event_handled = false;

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        http_server_stop(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = http_server_init();
    }
}

httpd_handle_t http_server_init()
{
    if (!http_network_event_handled) {
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
        http_network_event_handled = true;
    }

    server = http_server_start();
    if (server) {
        register_basic_handlers(server);
    } else {
        ESP_LOGE(TAG, "Failed to start httpd server");
    }

    return server;
}

void http_server_stop(httpd_handle_t hd)
{
    ESP_LOGI(TAG, "Stopping httpd");
    httpd_stop(hd);
    post_stop_mem = esp_get_free_heap_size();
    ESP_LOGI(TAG, "HTTPD Stop: Current free memory: %d", post_stop_mem);
}

