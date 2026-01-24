#ifndef _APP_WEBSERVER_H_
#define _APP_WEBSERVER_H_

#include "esp_log.h"
#include "cJSON.h"
#include "user_key.h"
#include "user_gpio.h"
#include "led_ctrl.h"
#include "esp_spiffs.h"
#include "esp_http_server.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void spiffs_init(void);
httpd_handle_t start_webserver(void);
esp_err_t websocket_handler(httpd_req_t *req);
void app_get_key_counter(void *pvParameters);
void gpio_status_update_task(void *pvParameters);

#endif  // _APP_WEBSERVER_H_