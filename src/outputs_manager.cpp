#include "outputs_manager.hpp"

void OutputsManager::init()
{
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    for (int i = 0; i < NUM_OUT; i++)
    {
        io_conf.pin_bit_mask = (1ULL << pins[i]);
        gpio_config(&io_conf);

        gpio_set_level(pins[i], 0);

        states[i] = false;
        blinking[i] = false;
        blink_state[i] = false;
        last_toggle[i] = 0;
    }
}

bool OutputsManager::getState(int index)
{
        if (index >= NUM_OUT) return false;
        return states[index];
}

void OutputsManager::set(int index)
{
    if (index < 0 || index >= NUM_OUT || states[index])
        return;
    states[index] = true;
    blinking[index] = false;
    gpio_set_level(pins[index], 1);
}

void OutputsManager::off(int index)
{
    if (index < 0 || index >= NUM_OUT || !states[index]) 
        return;
    states[index] = false;
    blinking[index] = false;
    gpio_set_level(pins[index], 0);
}

void OutputsManager::off_all()
{
    for (int i = 0; i < NUM_OUT; i++)
    {
        off(i);
    }
}

void OutputsManager::blink(int index)
{
    if (index < 0 || index >= NUM_OUT)
        return;

    blinking[index] = true;
    states[index] = true;

    last_toggle[index] = esp_timer_get_time();
    blink_state[index] = true;

    gpio_set_level(pins[index], 1);
}

void OutputsManager::stop_blink(int index)
{
    if (index < 0 || index >= NUM_OUT)
        return;

    blinking[index] = false;
    blink_state[index] = false;

    gpio_set_level(pins[index], states[index] ? 1 : 0);
}

void OutputsManager::update()
{
    uint64_t now = esp_timer_get_time();

    for (int i = 0; i < NUM_OUT; i++)
    {
        if (!blinking[i])
            continue;

        if (now - last_toggle[i] >= BLINK_PERIOD_US)
        {
            blink_state[i] = !blink_state[i];
            gpio_set_level(pins[i], blink_state[i] ? 1 : 0);
            last_toggle[i] = now;
        }
    }
}