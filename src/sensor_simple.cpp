#include "sensor_simple.hpp"
#include "driver/gpio.h"

#define SENSOR34 GPIO_NUM_34

void sensor_init()
{
    gpio_config_t io_conf = {};

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    io_conf.pin_bit_mask = (1ULL << SENSOR34);

    gpio_config(&io_conf);
}

bool sensor34_activo()
{
    return gpio_get_level(SENSOR34) == 0;
}