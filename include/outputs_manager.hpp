#pragma once

#include "driver/gpio.h"
#include "esp_timer.h"

class OutputsManager
{
public:
    void init();
    void set(int index);       // enciende una salida
    void off(int index);       // apaga una salida
    void off_all();            // apaga todas
    void blink(int index);     // activa parpadeo en esa salida
    void stop_blink(int index);// detiene parpadeo
    bool getState (int index);

    void update();             // llamar en loop

private:
    static const int NUM_OUT = 5;

    gpio_num_t pins[NUM_OUT] = {
        GPIO_NUM_2,
        GPIO_NUM_4,
        GPIO_NUM_5,
        GPIO_NUM_18,
        GPIO_NUM_19
    };

    bool states[NUM_OUT] = {false};      // estado ON/OFF
    bool blinking[NUM_OUT] = {false};    // si está parpadeando

    uint64_t last_toggle[NUM_OUT] = {0};
    bool blink_state[NUM_OUT] = {false};

    const uint64_t BLINK_PERIOD_US = 2000000; // 500 ms
};