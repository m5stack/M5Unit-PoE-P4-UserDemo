#include "app_webserver.h"

static int counter_value = 0;
static int last_counter  = 0;
static char led_color[8] = "00ff00";  // default green

#define TAG "app_webserver"

#define MAX_WS_CLIENTS 8

static httpd_handle_t server      = NULL;
static int ws_fds[MAX_WS_CLIENTS] = {-1, -1, -1, -1, -1, -1, -1, -1};  // fd list for WebSocket clients

static void ws_broadcast_state(void);

/**
 * @brief Converts a hex color string to RGB values
 * @param hex Hex color string (e.g., "#RRGGBB" or "RRGGBB")
 * @param r Pointer to store the red component
 * @param g Pointer to store the green component
 * @param b Pointer to store the blue component
 */
static void hex_to_rgb(const char *hex, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (hex[0] == '#') hex++;
    char rs[3] = {hex[0], hex[1], 0};
    char gs[3] = {hex[2], hex[3], 0};
    char bs[3] = {hex[4], hex[5], 0};
    *r         = (uint8_t)strtol(rs, NULL, 16);
    *g         = (uint8_t)strtol(gs, NULL, 16);
    *b         = (uint8_t)strtol(bs, NULL, 16);
}

/**
 * @brief Sends a file from SPIFFS to the HTTP client
 * This function opens the specified file from SPIFFS, reads its contents,
 * and sends it to the client in chunks. It also sets the appropriate content type.
 * @param req Pointer to the HTTP request
 * @param path Path to the file in SPIFFS
 * @param type MIME type of the file
 * @return ESP_OK on success, ESP_FAIL on failure
 */
static esp_err_t send_file(httpd_req_t *req, const char *path, const char *type)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    httpd_resp_set_type(req, type);
    char buf[512];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        httpd_resp_send_chunk(req, buf, n);
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// HTTP GET handler - Index, CSS, JS, M5Image
static esp_err_t index_handler(httpd_req_t *req)
{
    return send_file(req, "/spiffs/index.html", "text/html");
}
static esp_err_t css_handler(httpd_req_t *req)
{
    return send_file(req, "/spiffs/style.css", "text/css");
}
static esp_err_t js_handler(httpd_req_t *req)
{
    return send_file(req, "/spiffs/script.js", "application/javascript");
}
static esp_err_t m5image_handler(httpd_req_t *req)
{
    return send_file(req, "/spiffs/images/M5Stack_logo_name.png", "image/png");
}
static esp_err_t sdioimage_handler(httpd_req_t *req)
{
    return send_file(req, "/spiffs/images/SDIO_Area.png", "image/png");
}
static esp_err_t groveimage_handler(httpd_req_t *req)
{
    return send_file(req, "/spiffs/images/Grove_Area.png", "image/png");
}
static esp_err_t pinimage_handler(httpd_req_t *req)
{
    return send_file(req, "/spiffs/images/PIN_Area.png", "image/png");
}

// HTTP GET handler - Favicon
static esp_err_t favicon_get_handler(httpd_req_t *req)
{
    return send_file(req, "/spiffs/images/favicon.ico", "image/x-icon");
}

/**
 * @brief Handles the STATUS request
 * This function retrieves the current LED color and counter value,
 * formats them as a JSON string, and sends it back in the HTTP response.
 * @param req Pointer to the HTTP request
 */
static esp_err_t status_handler(httpd_req_t *req)
{
    char json[64];
    snprintf(json, sizeof(json), "{\"color\":\"#%s\",\"counter\":%d}", led_color, counter_value);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
    return ESP_OK;
}

/**
 * @brief Handles the SET color request
 * This function retrieves the "color" query parameter from the URL,
 * updates the global led_color variable, sets the RGB LED color,
 * and broadcasts the new state to all connected WebSocket clients.
 * @param req Pointer to the HTTP request
 * @return ESP_OK on success
 */
static esp_err_t set_color_handler(httpd_req_t *req)
{
    char buf[128], param[16];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        if (httpd_query_key_value(buf, "color", param, sizeof(param)) == ESP_OK) {
            ESP_LOGI(TAG, "Get color: %s", param);
            snprintf(led_color, sizeof(led_color), "%.6s", param + 3);
            uint8_t r, g, b;
            hex_to_rgb(led_color, &r, &g, &b);
            rgb_led_set_color(r, g, b);
            ESP_LOGI(TAG, "Set color: %s => RGB(%d, %d, %d)", led_color, r, g, b);
            ws_broadcast_state();
            memset(buf, 0, sizeof(buf));
            memset(param, 0, sizeof(param));
        }
    }
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

/**
 * @brief Handles the GET request for the counter value
 * This function retrieves the current counter value, formats it as a string,
 * and sends it back in the HTTP response with content type "text/plain".
 * @param req Pointer to the HTTP request
 * @return ESP_OK on success
 */
static esp_err_t get_counter_handler(httpd_req_t *req)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", counter_value);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/**
 * @brief Broadcasts the current counter value and LED color to all connected WebSocket clients
 *
 * This function creates a JSON message containing the current counter value and LED color,
 * then sends this information asynchronously to all currently connected WebSocket clients.
 * The message format is {"counter":<value>,"color":"#<hex_color>"}.
 *
 * @note This function uses ws_fds array to track active WebSocket connections and
 *       sends the same message to each connected client.
 */
static void ws_broadcast_state(void)
{
    char msg[80];
    snprintf(msg, sizeof(msg), "{\"type\":\"status\",\"counter\":%d,\"color\":\"#%s\"}", counter_value, led_color);
    ESP_LOGI(TAG, "Broadcast: %s", msg);
    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
        if (ws_fds[i] != -1) {
            httpd_ws_frame_t frame = {.final      = true,
                                      .fragmented = false,
                                      .type       = HTTPD_WS_TYPE_TEXT,
                                      .payload    = (uint8_t *)msg,
                                      .len        = strlen(msg)};
            esp_err_t err          = httpd_ws_send_frame_async(server, ws_fds[i], &frame);
            if (err != ESP_OK) ESP_LOGE(TAG, "fd[%d] ws_fds[%d] WebSocket send failed: %d", i, ws_fds[i], err);
        }
    }
}

/**
 * @brief Broadcasts a single GPIO state update to all connected WebSocket clients
 *
 * This function creates a JSON message containing the updated GPIO information
 * (ID, mode, and level) and sends it asynchronously to all currently connected
 * WebSocket clients. It constructs a JSON object with type "gpio_update" and
 * includes the GPIO's ID, mode (input/output), and current level (high/low).
 *
 * @param id The GPIO identifier being updated
 * @param mode A string representing the GPIO mode ("input" or "output")
 * @param level The current level of the GPIO (typically 0 or 1)
 */
static void broadcast_gpio_update(int id, const char *mode, int level)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "gpio_update");
    cJSON_AddNumberToObject(root, "id", id);  // use id
    cJSON_AddStringToObject(root, "mode", mode);
    cJSON_AddNumberToObject(root, "level", level);

    char *json_str = cJSON_PrintUnformatted(root);

    // broadcast to all connected clients
    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
        if (ws_fds[i] >= 0) {
            httpd_ws_frame_t ws_pkt = {
                .type = HTTPD_WS_TYPE_TEXT, .payload = (uint8_t *)json_str, .len = strlen(json_str)};
            httpd_ws_send_frame_async(server, ws_fds[i], &ws_pkt);
        }
    }

    free(json_str);
    cJSON_Delete(root);

    int pin = get_pin_by_id(id);
    ESP_LOGI(TAG, "Broadcast GPIO update: id=%d, pin=%d, mode=%s, level=%d", id, pin, mode, level);
}

/**
 * @brief Sends all GPIO statuses to a specific WebSocket client
 *
 * This function creates a JSON object containing the status of all GPIO pins,
 * including their IDs, modes (input/output), and current levels. It then sends
 * this information as a WebSocket text frame to the specified client.
 *
 * @param fd The file descriptor of the target WebSocket client to send the GPIO status to
 */
static void send_all_gpio_status(int fd)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "gpio_status");
    cJSON *gpios = cJSON_CreateArray();

    for (int i = 0; i < GPIO_COUNT; i++) {
        cJSON *gpio = cJSON_CreateObject();
        cJSON_AddNumberToObject(gpio, "id", i);  // 使用id
        cJSON_AddStringToObject(gpio, "mode", ((gpio_states[i].mode == GPIO_MODE_INPUT) ? "input" : "output"));
        cJSON_AddNumberToObject(gpio, "level", (gpio_states[i].level));
        cJSON_AddItemToArray(gpios, gpio);
    }

    cJSON_AddItemToObject(root, "gpios", gpios);
    char *json_str = cJSON_PrintUnformatted(root);

    httpd_ws_frame_t ws_pkt = {.type = HTTPD_WS_TYPE_TEXT, .payload = (uint8_t *)json_str, .len = strlen(json_str)};
    httpd_ws_send_frame_async(server, fd, &ws_pkt);

    free(json_str);
    cJSON_Delete(root);
    ESP_LOGI(TAG, "Sent all GPIO status to fd=%d", fd);
}

/**
 * @brief WebSocket handler function for opening new WebSocket connections
 * @param hd HTTP server handle
 * @param sockfd Socket file descriptor of the new WebSocket connection
 * @return esp_err_t ESP_OK on success
 */
static esp_err_t ws_open_handler(httpd_handle_t hd, int sockfd)
{
    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
        if (ws_fds[i] == -1) {
            ws_fds[i] = sockfd;
            ESP_LOGI("WS", "WebSocket client connected, ws_fds[%d]: fd=%d", i, sockfd);
            break;
        }
    }
    return ESP_OK;
}

/**
 * @brief WebSocket handler function for closing WebSocket connections
 * @param hd HTTP server handle
 * @param sockfd Socket file descriptor of the WebSocket connection to be closed
 */
static void ws_close_handler(httpd_handle_t hd, int sockfd)
{
    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
        if (ws_fds[i] == sockfd) {
            ws_fds[i] = -1;
            ESP_LOGI("WS", "WebSocket client disconnected, ws_fds[%d]: fd=%d", i, sockfd);
            break;
        }
    }
}

/**
 * @brief WebSocket handler function for processing incoming WebSocket messages
 *
 * This function handles different types of WebSocket messages including:
 * - Setting LED color via 'set_color' command
 * - Getting counter value via 'get_counter' command
 * - Getting status (color and counter) via 'get_status' command
 * - Controlling GPIO pins via 'gpio_control' command
 * - Requesting GPIO status via 'get_gpio_status' command
 *
 * The function also handles connection management, including detecting client disconnections
 * and processing WebSocket close frames.
 *
 * @param req HTTP request structure containing WebSocket connection information and message payload
 * @return esp_err_t ESP_OK on success, ESP_FAIL on failure, or other ESP error codes
 *         - ESP_OK: Success, message processed or connection established
 *         - ESP_FAIL: General failure in message processing
 *         - ESP_ERR_NO_MEM: Memory allocation failure for payload buffer
 */
esp_err_t websocket_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt = {0};

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        int fd = httpd_req_to_sockfd(req);
        httpd_sess_trigger_close(req->handle, fd);
        ws_close_handler(req->handle, fd);
        ESP_LOGW("WS", "recv_frame failed, assume client dropped");
        return ESP_FAIL;
    }

    if (ws_pkt.len) {
        ws_pkt.payload = (uint8_t *)malloc(ws_pkt.len + 1);
        if (!ws_pkt.payload) return ESP_ERR_NO_MEM;
        if (httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len) != ESP_OK) {
            free(ws_pkt.payload);
            int fd = httpd_req_to_sockfd(req);
            ws_close_handler(req->handle, fd);
            httpd_sess_trigger_close(req->handle, fd);
            return ESP_FAIL;
        }
        ((char *)ws_pkt.payload)[ws_pkt.len] = '\0';
    }

    if (ws_pkt.type == HTTPD_WS_TYPE_CLOSE) {
        free(ws_pkt.payload);
        int fd = httpd_req_to_sockfd(req);
        httpd_sess_trigger_close(req->handle, fd);
        ws_close_handler(req->handle, fd);
        ESP_LOGI("WS", "Client sent CLOSE frame");
        return ESP_OK;
    }

    // parse JSON and handle commands
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT && ws_pkt.payload) {
        cJSON *root = cJSON_Parse((char *)ws_pkt.payload);
        if (root) {
            const cJSON *type = cJSON_GetObjectItem(root, "type");
            if (cJSON_IsString(type) && type->valuestring) {
                /* SET_COLOR */
                if (strcmp(type->valuestring, "set_color") == 0) {
                    const cJSON *color = cJSON_GetObjectItem(root, "color");
                    if (cJSON_IsString(color) && color->valuestring) {
                        ESP_LOGI(TAG, "WS set_color: %s", color->valuestring);
                        snprintf(led_color, sizeof(led_color), "%.6s", color->valuestring + 1);
                        uint8_t r, g, b;
                        hex_to_rgb(led_color, &r, &g, &b);
                        rgb_led_set_color(r, g, b);
                        ESP_LOGI(TAG, "Set color: %s => RGB(%d, %d, %d)", led_color, r, g, b);
                        ws_broadcast_state();
                    }

                    /* GET_COUNTER */
                } else if (strcmp(type->valuestring, "get_counter") == 0) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "{\"type\":\"counter_update\",\"counter\":%d}", counter_value);
                    httpd_ws_frame_t resp = {.type = HTTPD_WS_TYPE_TEXT, .payload = (uint8_t *)buf, .len = strlen(buf)};
                    ESP_LOGI(TAG, "WS get_counter: %s", buf);
                    httpd_ws_send_frame(req, &resp);

                    /* RESET_COUNTER */
                } else if (strcmp(type->valuestring, "reset_counter") == 0) {
                    reset_key_counter();
                    ESP_LOGI(TAG, "WS reset_counter: counter reset to 0");
                    ws_broadcast_state();

                    /* GET_STATUS (color and user key counter) */
                } else if (strcmp(type->valuestring, "get_status") == 0) {
                    char json[80];
                    snprintf(json, sizeof(json), "{\"type\":\"status\",\"color\":\"#%s\",\"counter\":%d}", led_color,
                             counter_value);
                    httpd_ws_frame_t resp = {
                        .type = HTTPD_WS_TYPE_TEXT, .payload = (uint8_t *)json, .len = strlen(json)};
                    httpd_ws_send_frame(req, &resp);
                }
                /* GPIO_CONTROL */
                else if (strcmp(type->valuestring, "gpio_control") == 0) {
                    const cJSON *id_obj     = cJSON_GetObjectItem(root, "id");
                    const cJSON *action_obj = cJSON_GetObjectItem(root, "action");
                    const cJSON *value_obj  = cJSON_GetObjectItem(root, "value");

                    if (cJSON_IsNumber(id_obj) && cJSON_IsString(action_obj) && cJSON_IsString(value_obj)) {
                        int id             = id_obj->valueint;
                        const char *action = action_obj->valuestring;
                        const char *value  = value_obj->valuestring;

                        // verfiy id range
                        if (id < 0 || id >= GPIO_COUNT) {
                            ESP_LOGW(TAG, "Invalid GPIO id: %d", id);
                        } else {
                            gpio_num_t gpio_num = gpio_states[id].gpio_num;
                            int pin             = get_pin_by_id(id);
                            ESP_LOGI(TAG, "GPIO control: id=%d, pin=%d, action=%s, value=%s", id, pin, action, value);

                            /* SET_MODE  */
                            if (strcmp(action, "set_mode") == 0) {
                                if (strcmp(value, "input") == 0) {
                                    user_single_gpio_reinit(gpio_num, GPIO_MODE_INPUT);

                                } else if (strcmp(value, "output") == 0) {
                                    user_single_gpio_reinit(gpio_num, GPIO_MODE_OUTPUT);

                                    user_single_gpio_set_level(gpio_num, gpio_states[id].level);
                                }
                            }
                            /* SET_LEVEL */
                            else if (strcmp(action, "set_level") == 0) {
                                if (gpio_states[id].mode == GPIO_MODE_OUTPUT) {
                                    if (strcmp(value, "high") == 0) {
                                        user_single_gpio_set_level(gpio_num, 1);

                                    } else if (strcmp(value, "low") == 0) {
                                        user_single_gpio_set_level(gpio_num, 0);
                                    }
                                } else {
                                    // cannot set level on input mode
                                    ESP_LOGW(TAG, "Cannot set level on input GPIO id=%d", id);

                                    // Send ERROR response
                                    cJSON *err_root = cJSON_CreateObject();
                                    cJSON_AddStringToObject(err_root, "type", "gpio_confirm");
                                    cJSON_AddNumberToObject(err_root, "id", id);
                                    cJSON_AddBoolToObject(err_root, "success", false);
                                    cJSON_AddStringToObject(err_root, "message", "Cannot set level in input mode");

                                    char *err_json            = cJSON_PrintUnformatted(err_root);
                                    httpd_ws_frame_t err_resp = {.type    = HTTPD_WS_TYPE_TEXT,
                                                                 .payload = (uint8_t *)err_json,
                                                                 .len     = strlen(err_json)};
                                    httpd_ws_send_frame(req, &err_resp);

                                    free(err_json);
                                    cJSON_Delete(err_root);
                                }
                            }
                        }
                    }

                    /* GET_GPIO_STATUS */
                } else if (strcmp(type->valuestring, "get_gpio_status") == 0) {
                    ESP_LOGI(TAG, "WS get_gpio_status");
                    int fd = httpd_req_to_sockfd(req);
                    send_all_gpio_status(fd);
                }
            }
            cJSON_Delete(root);
        }
    }

    free(ws_pkt.payload);
    return ESP_OK;
}

/**
 * @brief Task to get key counter value and broadcast via WebSocket
 * @param pvParameters Task parameters (not used)
 */
void app_get_key_counter(void *pvParameters)
{
    ESP_UNUSED(pvParameters);

    while (1) {
        counter_value = get_key_counter();
        if (counter_value != last_counter) {
            ESP_LOGI(TAG, "Key counter: %d", counter_value);
            last_counter = counter_value;
            ws_broadcast_state();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief Task to monitor GPIO state changes and broadcast updates via WebSocket
 *
 * This FreeRTOS task continuously monitors the GPIO states array for any changes.
 * When an update is detected (indicated by the update flag in gpio_states),
 * it broadcasts the new GPIO state to all connected WebSocket clients using
 * the broadcast_gpio_update function. After broadcasting, it resets the
 * update flag for that GPIO. The task runs in an infinite loop, checking
 * for updates every 100 milliseconds.
 *
 * @param pvParameters Task parameters (unused, marked with ESP_UNUSED)
 */
void gpio_status_update_task(void *pvParameters)
{
    ESP_UNUSED(pvParameters);

    while (1) {
        for (int i = 0; i < GPIO_COUNT; i++) {
            if (gpio_states[i].update) {
                const char *mode_str = (gpio_states[i].mode == GPIO_MODE_INPUT) ? "input" : "output";
                broadcast_gpio_update(i, mode_str, gpio_states[i].level);
                gpio_states[i].update = false;  // reset update flag
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ===== start HTTP + WS =====
/**
 * @brief Interface of start HTTP and WS server
 * @return httpd_handle_t HTTP server handle
 */
httpd_handle_t start_webserver(void)
{
    ESP_LOGI(TAG, "Starting webserver...");
    httpd_config_t config    = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets  = MAX_WS_CLIENTS + 4;
    config.open_fn           = ws_open_handler;
    config.close_fn          = ws_close_handler;
    config.lru_purge_enable  = true;
    config.recv_wait_timeout = 5;
    config.send_wait_timeout = 5;

    if (httpd_start(&server, &config) == ESP_OK) {
        // WebSocket
        httpd_uri_t uri_ws = {
            .uri = "/ws", .method = HTTP_GET, .handler = websocket_handler, .user_ctx = NULL, .is_websocket = true};
        httpd_register_uri_handler(server, &uri_ws);

        httpd_uri_t uri_root = {.uri = "/", .method = HTTP_GET, .handler = index_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &uri_root);

        httpd_uri_t uri_css = {.uri = "/style.css", .method = HTTP_GET, .handler = css_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &uri_css);

        httpd_uri_t uri_js = {.uri = "/script.js", .method = HTTP_GET, .handler = js_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &uri_js);

        httpd_uri_t uri_m5image = {
            .uri = "/images/M5Stack_logo_name.png", .method = HTTP_GET, .handler = m5image_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &uri_m5image);

        httpd_uri_t uri_sdioimage = {
            .uri = "/images/SDIO_Area.png", .method = HTTP_GET, .handler = sdioimage_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &uri_sdioimage);

        httpd_uri_t uri_groveimage = {
            .uri = "/images/Grove_Area.png", .method = HTTP_GET, .handler = groveimage_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &uri_groveimage);

        httpd_uri_t uri_pinimage = {
            .uri = "/images/PIN_Area.png", .method = HTTP_GET, .handler = pinimage_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &uri_pinimage);

        httpd_uri_t uri_favicon = {
            .uri = "/images/favicon.ico", .method = HTTP_GET, .handler = favicon_get_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &uri_favicon);

        xTaskCreate(gpio_status_update_task, "gpio_status_update_task", 1024 * 4, NULL, 5, NULL);
        xTaskCreate(app_get_key_counter, "app_get_key_counter", 1024 * 4, NULL, 5, NULL);
    }
    return server;
}

/**
 * @brief Initialize SPIFFS file system
 */
void spiffs_init(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs", .partition_label = NULL, .max_files = 5, .format_if_mount_failed = true};
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
}
