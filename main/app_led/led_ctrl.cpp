#include "led_ctrl.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initializes the RGB LED by configuring the LEDC timer and channels
 */
void rgb_led_init(void)
{
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode          = LEDC_MODE;
    ledc_timer.duty_resolution     = LEDC_DUTY_RES;
    ledc_timer.timer_num           = LEDC_TIMER;
    ledc_timer.freq_hz             = LEDC_FREQUENCY;
    ledc_timer.clk_cfg             = LEDC_AUTO_CLK;
    ledc_timer_config(&ledc_timer);

    const int led_pins[]                = {PIN_LED_R, PIN_LED_G, PIN_LED_B};
    const ledc_channel_t led_channels[] = {LEDC_CHANNEL_0, LEDC_CHANNEL_1, LEDC_CHANNEL_2};

    for (int i = 0; i < 3; i++) {
        ledc_channel_config_t ledc_channel = {};
        ledc_channel.gpio_num              = led_pins[i];
        ledc_channel.speed_mode            = LEDC_MODE;
        ledc_channel.channel               = led_channels[i];
        ledc_channel.timer_sel             = LEDC_TIMER;
        ledc_channel.duty                  = 0;
        ledc_channel.hpoint                = 0;
        ledc_channel_config(&ledc_channel);
    }
}

/**
 * @brief Sets the RGB LED color using individual red, green, and blue values
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @note The RGB LED is assumed to be common anode, so the duty values are inverted
 */
void rgb_led_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    // need to invert duty for common anode
    uint32_t duty_r = 255 - r;
    uint32_t duty_g = 255 - g;
    uint32_t duty_b = 255 - b;

    if (duty_r == 255) {
        ledc_stop(LEDC_MODE, LEDC_CHANNEL_0, 1);  // close led on high level
    } else {
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, duty_r);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
    }

    if (duty_g == 255) {
        ledc_stop(LEDC_MODE, LEDC_CHANNEL_1, 1);  // close led on high level
    } else {
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_1, duty_g);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_1);
    }

    if (duty_b == 255) {
        ledc_stop(LEDC_MODE, LEDC_CHANNEL_2, 1);  // close led on high level
    } else {
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_2, duty_b);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_2);
    }
}

/**
 * @brief Sets the RGB LED color using a 0xRRGGBB formatted color value
 * @param color 32-bit color value in 0xRRGGBB format
 */
void rgb_set_color(uint32_t color)
{
    uint8_t r = (color >> 16) & 0xff;
    uint8_t g = (color >> 8) & 0xff;
    uint8_t b = color;
    rgb_led_set_color(r, g, b);
}
