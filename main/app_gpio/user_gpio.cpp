#include "user_gpio.h"

gpio_state_t gpio_states[GPIO_COUNT];

// create gpio_isr_arg_t array for ISR arguments
static gpio_isr_arg_t gpio_isr_args[] = {
    {.gpio_num = GPIO0, .array_index = 0},   {.gpio_num = GPIO1, .array_index = 1},
    {.gpio_num = GPIO2, .array_index = 2},   {.gpio_num = GPIO3, .array_index = 3},
    {.gpio_num = GPIO4, .array_index = 4},   {.gpio_num = GPIO5, .array_index = 5},
    {.gpio_num = GPIO6, .array_index = 6},   {.gpio_num = GPIO7, .array_index = 7},
    {.gpio_num = GPIO8, .array_index = 8},   {.gpio_num = GPIO9, .array_index = 9},
    {.gpio_num = GPIO10, .array_index = 10}, {.gpio_num = GPIO11, .array_index = 11},
    {.gpio_num = GPIO12, .array_index = 12}, {.gpio_num = GPIO13, .array_index = 13},
    {.gpio_num = GPIO14, .array_index = 14}, {.gpio_num = GPIO15, .array_index = 15},
    {.gpio_num = GPIO16, .array_index = 16}, {.gpio_num = GPIO17, .array_index = 17},
    {.gpio_num = GPIO18, .array_index = 18},
};

/**
 * @brief GPIO interrupt service routine to detect status changes
 * @param arg Pointer to gpio_isr_arg_t structure containing GPIO number and array index
 */
static void gpio_status_detect_handler(void *arg)
{
    gpio_isr_arg_t *param = (gpio_isr_arg_t *)arg;
    gpio_num_t gpio_num   = param->gpio_num;
    int index             = param->array_index;

    uint32_t level = gpio_get_level(gpio_num);

    gpio_states[index].mode   = GPIO_MODE_INPUT;
    gpio_states[index].level  = level;
    gpio_states[index].update = true;
}

/**
 * @brief Initializes user GPIOs as inputs with interrupts
 */
void user_gpio_init(void)
{
    gpio_config_t io_conf = {.pin_bit_mask = ALL_GPIO_PINS_BIT_MASK,
                             .mode         = GPIO_MODE_INPUT,
                             .pull_up_en   = GPIO_PULLUP_ENABLE,
                             .pull_down_en = GPIO_PULLDOWN_DISABLE,
                             .intr_type    = GPIO_INTR_ANYEDGE};
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    // add ISR handler for each GPIO
    for (int i = 0; i < GPIO_COUNT; i++) {
        gpio_isr_handler_add(gpio_isr_args[i].gpio_num, gpio_status_detect_handler, &gpio_isr_args[i]);
    }
    for (int i = 0; i < GPIO_COUNT; i++) {
        gpio_states[i].gpio_num = gpio_isr_args[i].gpio_num;
        gpio_states[i].mode     = GPIO_MODE_INPUT;
        gpio_states[i].level    = gpio_get_level(gpio_states[i].gpio_num);
        gpio_states[i].update   = false;
    }
}

/**
 * @brief Re-initializes a single GPIO with the specified mode
 * @param gpio_num The GPIO number to re-initialize
 * @param mode The GPIO mode (GPIO_MODE_INPUT or GPIO_MODE_OUTPUT)
 * @note If set to input mode, the GPIO will have interrupts enabled, otherwise interrupts will be disabled,
 * and the GPIO will be set to the low level. 'gpio_states' array will be updated accordingly.
 */
void user_single_gpio_reinit(gpio_num_t gpio_num, gpio_mode_t mode)
{
    if (mode == GPIO_MODE_INPUT) {
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << gpio_num,
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_ANYEDGE  // dual edge detection
        };
        gpio_config(&io_conf);

        // find the corresponding gpio_isr_arg_t
        gpio_isr_arg_t *target_arg = NULL;
        for (int i = 0; i < GPIO_COUNT; i++) {
            if (gpio_isr_args[i].gpio_num == gpio_num) {
                target_arg = &gpio_isr_args[i];
                break;
            }
        }

        if (target_arg != NULL) {
            gpio_isr_handler_add(gpio_num, gpio_status_detect_handler, target_arg);
        }
        // find and update gpio_states array
        for (int i = 0; i < GPIO_COUNT; i++) {
            if (gpio_states[i].gpio_num == gpio_num) {
                gpio_states[i].mode   = GPIO_MODE_INPUT;
                gpio_states[i].level  = gpio_get_level(gpio_states[i].gpio_num);
                gpio_states[i].update = true;
                break;
            }
        }
    } else {
        gpio_config_t io_conf = {.pin_bit_mask = 1ULL << gpio_num,
                                 .mode         = GPIO_MODE_OUTPUT,
                                 .pull_up_en   = GPIO_PULLUP_ENABLE,
                                 .pull_down_en = GPIO_PULLDOWN_ENABLE,
                                 .intr_type    = GPIO_INTR_DISABLE};
        gpio_config(&io_conf);

        // remove ISR handler for the gpio_num
        gpio_isr_handler_remove(gpio_num);

        // find and update gpio_states array
        for (int i = 0; i < GPIO_COUNT; i++) {
            if (gpio_states[i].gpio_num == gpio_num) {
                gpio_states[i].mode   = GPIO_MODE_OUTPUT;
                gpio_states[i].level  = gpio_get_level(gpio_states[i].gpio_num);
                gpio_states[i].update = true;
                break;
            }
        }
    }
}

/**
 * @brief Sets the level of a single GPIO if it is in output mode
 * @param gpio_num The GPIO number to set the level
 * @param level The level to set (0 or 1)
 * @return true if the level was set successfully, false if the GPIO is in input mode
 */
bool user_single_gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
    for (int i = 0; i < GPIO_COUNT; i++) {
        if (gpio_states[i].gpio_num == gpio_num) {
            if (gpio_states[i].mode == GPIO_MODE_INPUT) {
                return false;  // 输入模式不能设置电平
            }
            gpio_states[i].level  = level;
            gpio_states[i].update = true;
            break;
        }
    }
    gpio_set_level(gpio_num, level);
    return true;
}

/**
 * @brief Retrieves the GPIO pin number corresponding to a given ID
 * @param id The ID of the GPIO (0 to GPIO_COUNT-1)
 * @return int The GPIO pin number, or -1 if the ID is invalid
 */
int get_pin_by_id(int id)
{
    const int gpio_pins[] = {8, 9, 10, 11, 12, 13, 19, 20, 21, 22, 23, 39, 40, 41, 42, 43, 44, 53, 54};
    if (id >= 0 && id < GPIO_COUNT) {
        return gpio_pins[id];
    }
    return -1;
}

/**
 * @brief Retrieves the ID corresponding to a given GPIO pin number
 * @param pin The GPIO pin number
 * @return int The ID, or -1 if the pin number is invalid
 */
int get_id_by_pin(int pin)
{
    const int gpio_pins[] = {8, 9, 10, 11, 12, 13, 19, 20, 21, 22, 23, 39, 40, 41, 42, 43, 44, 53, 54};
    for (int i = 0; i < GPIO_COUNT; i++) {
        if (gpio_pins[i] == pin) {
            return i;
        }
    }
    return -1;
}