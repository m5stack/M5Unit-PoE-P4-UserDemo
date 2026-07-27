#ifndef USER_GPIO_H_
#define USER_GPIO_H_

#include "driver/gpio.h"

#define GPIO_COUNT 19  // there is total 19 GPIOs used

#define GPIO0  GPIO_NUM_8
#define GPIO1  GPIO_NUM_9
#define GPIO2  GPIO_NUM_10
#define GPIO3  GPIO_NUM_11
#define GPIO4  GPIO_NUM_12
#define GPIO5  GPIO_NUM_13
#define GPIO6  GPIO_NUM_19
#define GPIO7  GPIO_NUM_20
#define GPIO8  GPIO_NUM_21
#define GPIO9  GPIO_NUM_22
#define GPIO10 GPIO_NUM_23
#define GPIO11 GPIO_NUM_39
#define GPIO12 GPIO_NUM_40
#define GPIO13 GPIO_NUM_41
#define GPIO14 GPIO_NUM_42
#define GPIO15 GPIO_NUM_43
#define GPIO16 GPIO_NUM_44
#define GPIO17 GPIO_NUM_53
#define GPIO18 GPIO_NUM_54

extern uint8_t gpio_input_status[GPIO_COUNT];

#define ALL_GPIO_PINS_BIT_MASK                                                                                         \
    ((1ULL << GPIO0) | (1ULL << GPIO1) | (1ULL << GPIO2) | (1ULL << GPIO3) | (1ULL << GPIO4) | (1ULL << GPIO5) |       \
     (1ULL << GPIO6) | (1ULL << GPIO7) | (1ULL << GPIO8) | (1ULL << GPIO9) | (1ULL << GPIO10) | (1ULL << GPIO11) |     \
     (1ULL << GPIO12) | (1ULL << GPIO13) | (1ULL << GPIO14) | (1ULL << GPIO15) | (1ULL << GPIO16) | (1ULL << GPIO17) | \
     (1ULL << GPIO18))

typedef struct {
    gpio_num_t gpio_num;
    int array_index;
} gpio_isr_arg_t;

typedef struct {
    gpio_num_t gpio_num;
    gpio_mode_t mode;
    int level;
    bool update;  // if paramter in gpio_state_t has any update
} gpio_state_t;

extern gpio_state_t gpio_states[GPIO_COUNT];

void user_gpio_init(void);
void user_single_gpio_reinit(gpio_num_t gpio_num, gpio_mode_t mode);
bool user_single_gpio_set_level(gpio_num_t gpio_num, uint32_t level);
int get_pin_by_id(int id);
int get_id_by_pin(int pin);

#endif  // _USER_GPIO_H_
