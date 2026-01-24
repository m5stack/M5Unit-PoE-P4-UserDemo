#ifndef LED_CTRL_H
#define LED_CTRL_H

#include "driver/ledc.h"
#include "esp_err.h"

#define PIN_LED_R GPIO_NUM_17
#define PIN_LED_G GPIO_NUM_15
#define PIN_LED_B GPIO_NUM_16
// PWM 频率和分辨率
#define LEDC_TIMER     LEDC_TIMER_0
#define LEDC_MODE      LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES  LEDC_TIMER_8_BIT
#define LEDC_FREQUENCY 5000

void rgb_led_init(void);
void rgb_led_set_color(uint8_t r, uint8_t g, uint8_t b);
void rgb_set_color(uint32_t color);

#endif  // LED_CTRL_H