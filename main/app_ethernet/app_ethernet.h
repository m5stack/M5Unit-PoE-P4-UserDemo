#ifndef _APP_ETHERNET_H_
#define _APP_ETHERNET_H_

#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "ethernet_init.h"

void app_ethernet_init(void);
void app_ethernet_deinit(void);
void app_ethernet_start(void);

#endif  // _APP_ETHERNET_H_
