/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "led_ctrl.h"
#include "app_ethernet.h"
#include "user_key.h"
#include "mdns.h"
#include "app_webserver.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HOST_NAME "poe-p4"

static const char *TAG = "main";
uint32_t g_ip_addr     = 0;

/**
 * @brief Initialize and configure mDNS service
 *
 * This function initializes the mDNS service, sets the hostname, instance name,
 * and adds HTTP service (_http._tcp) on port 80. It also adds a subtype "_server"
 * for the HTTP service to allow easier discovery.
 *
 * @param hostname The hostname to be set for mDNS discovery
 */
static void initialise_mdns(const char *hostname)
{
    char *instance_name = "Unit PoE-P4 Device";
    // init service of mDNS
    esp_err_t err = mdns_init();
    if (err) {
        printf("MDNS Init failed: %d\n", err);
        return;
    }
    ESP_ERROR_CHECK(mdns_hostname_set(hostname));
    ESP_LOGI("MDNS", "mdns hostname set to: %s", hostname);
    ESP_ERROR_CHECK(mdns_instance_name_set(instance_name));
    ESP_ERROR_CHECK(mdns_service_add(instance_name, "_http", "_tcp", 80, NULL, 0));
    ESP_ERROR_CHECK(mdns_service_subtype_add_for_host(instance_name, "_http", "_tcp", NULL, "_server"));
}

/**
 * @brief Event handler for Ethernet events
 */
static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
        case ETHERNET_EVENT_CONNECTED:
            esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
            ESP_LOGI(TAG, "Ethernet Link Up");
            ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2],
                     mac_addr[3], mac_addr[4], mac_addr[5]);
            break;
        case ETHERNET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Ethernet Link Down");
            break;
        case ETHERNET_EVENT_START:
            ESP_LOGI(TAG, "Ethernet Started");
            break;
        case ETHERNET_EVENT_STOP:
            ESP_LOGI(TAG, "Ethernet Stopped");
            break;
        default:
            break;
    }
}

/**
 * @brief Event handler for IP_EVENT_ETH_GOT_IP
 */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event           = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    g_ip_addr = ip_info->ip.addr;
    // start mDNS service
    initialise_mdns(HOST_NAME);
}

void app_main(void)
{
    rgb_led_init();           // init RGB LED
    rgb_set_color(0x00ff00);  // set LED to green

    user_gpio_init();  // init all GPIO
    user_key_init();   // init user key

    spiffs_init();
    app_ethernet_init();  // init ethernet
    // Register user defined event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    app_ethernet_start();  // start ethernet

#if CONFIG_EXAMPLE_ETH_DEINIT_AFTER_S >= 0
    app_ethernet_deinit();
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_ETH_GOT_IP, got_ip_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(ETH_EVENT, ESP_EVENT_ANY_ID, eth_event_handler));
    ESP_ERROR_CHECK(esp_event_loop_delete_default());
#endif  // EXAMPLE_ETH_DEINIT_AFTER_S > 0

    start_webserver();  // start HTTP server and WebSocket
}

#ifdef __cplusplus
}
#endif