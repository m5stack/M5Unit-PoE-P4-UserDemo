#include "user_key.h"

#define DEBOUNCE_MS      50
#define LONG_PRESS_LIMIT 2000

static const char *TAG = "key_count";

static volatile int counter  = 0;  // user key counter
static int64_t press_time_ms = 0;  // press time in ms
static int64_t last_isr_ms   = 0;  // last ISR time for debounce

/**
 * @brief Interrupt service routine for the user key press detection
 *
 * This function handles GPIO interrupt events triggered by changes in the user key state.
 * It implements debouncing logic to prevent multiple triggers from mechanical switch bounce.
 * The function detects short presses (less than LONG_PRESS_LIMIT) and increments a counter
 * only for valid short presses. The interrupt is configured to trigger on both rising and
 * falling edges of the GPIO signal.
 *
 * @param arg User-defined argument passed to the interrupt handler (not used in this implementation)
 *
 * @note This function is marked with IRAM_ATTR to ensure it remains in instruction RAM
 *       during light sleep modes, allowing it to respond to interrupts quickly.
 *
 * @details The function works as follows:
 * 1. Gets current time in milliseconds using esp_timer_get_time()
 * 2. Checks if enough time has passed since last interrupt to avoid bouncing
 * 3. Reads current GPIO level to determine if key is pressed (low) or released (high)
 * 4. For key press (level 0): records the time of press
 * 5. For key release (level 1): calculates duration and increments counter if it's a short press
 */
static void IRAM_ATTR key_isr_handler(void *arg)
{
    int64_t now_ms = esp_timer_get_time() / 1000;
    if (now_ms - last_isr_ms < DEBOUNCE_MS) {
        return;
    }
    last_isr_ms = now_ms;

    int level = gpio_get_level(USR_KEY_GPIO);

    if (level == 0) {
        press_time_ms = now_ms;
    } else {
        int64_t press_duration = now_ms - press_time_ms;
        if (press_duration > 0 && press_duration < LONG_PRESS_LIMIT) {
            counter++;
        }
    }
}

/**
 * @brief Retrieves the current value of the user key press counter
 * @return int The number of valid short presses detected
 */
int get_key_counter(void)
{
    return counter;
}

/**
 * @brief Resets the user key press counter to zero
 */
void reset_key_counter(void)
{
    counter = 0;
}

/**
 * @brief Initializes the user key GPIO and configures the interrupt service
 */
void user_key_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << USR_KEY_GPIO,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE  // 双边沿检测
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(USR_KEY_GPIO, key_isr_handler, NULL);
}
