#ifndef _USER_KEY_H
#define _USER_KEY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

#define USR_KEY_GPIO GPIO_NUM_45

void user_key_init(void);
int get_key_counter(void);
void reset_key_counter(void);

#ifdef __cplusplus
}
#endif

#endif  // USER_KEY_H