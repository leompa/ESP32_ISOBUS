#include "rpm_counter.hpp"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_attr.h"

#define NUM_SENSORS 5

static const gpio_num_t pins[NUM_SENSORS] = {
    GPIO_NUM_27,
    GPIO_NUM_26,
    GPIO_NUM_25,
    GPIO_NUM_33,
    GPIO_NUM_32
};

volatile uint64_t last_pulse_time[NUM_SENSORS] = {0};
volatile uint64_t prev_pulse_time[NUM_SENSORS] = {0};

static void IRAM_ATTR isr0(void* arg)
{
    uint64_t now = esp_timer_get_time();
    
    // Anti-ruido: ignorar pulsos más rápidos que ~6000-8000 RPM
    if (now - last_pulse_time[0] < 14000ULL) {  
        return;
    }
    
    prev_pulse_time[0] = last_pulse_time[0];
    last_pulse_time[0] = now;
}

static void IRAM_ATTR isr1(void* arg)
{
    uint64_t now = esp_timer_get_time();
    
     // Anti-ruido: ignorar pulsos más rápidos que ~6000-8000 RPM
    if (now - last_pulse_time[1] < 24000ULL) {  
        return;
    }

    prev_pulse_time[1] = last_pulse_time[1];
    last_pulse_time[1] = now;
}

static void IRAM_ATTR isr2(void* arg)
{
    uint64_t now = esp_timer_get_time();
     // Anti-ruido: ignorar pulsos más rápidos que ~6000-8000 RPM
    if (now - last_pulse_time[2] < 24000ULL) {  
        return;
    }
    prev_pulse_time[2] = last_pulse_time[2];
    last_pulse_time[2] = now;
}

static void IRAM_ATTR isr3(void* arg)
{
    uint64_t now = esp_timer_get_time();
    prev_pulse_time[3] = last_pulse_time[3];
    last_pulse_time[3] = now;
}

static void IRAM_ATTR isr4(void* arg)
{
    uint64_t now = esp_timer_get_time();
    prev_pulse_time[4] = last_pulse_time[4];
    last_pulse_time[4] = now;
}


void rpm_init()
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;

    for(int i=0;i<NUM_SENSORS;i++)
    {
        io_conf.pin_bit_mask = (1ULL << pins[i]);
        gpio_config(&io_conf);
    }

    gpio_install_isr_service(0);

    gpio_isr_handler_add(pins[0], isr0, NULL);
    gpio_isr_handler_add(pins[1], isr1, NULL);
    gpio_isr_handler_add(pins[2], isr2, NULL);
    gpio_isr_handler_add(pins[3], isr3, NULL);
    gpio_isr_handler_add(pins[4], isr4, NULL);
}

int rpm_get(int index)
{
    if(index < 0 || index >= NUM_SENSORS)
        return 0;

    uint64_t now = esp_timer_get_time();

    uint64_t t1 = prev_pulse_time[index];
    uint64_t t2 = last_pulse_time[index];

    // si hace más de 1 segundo que no hay pulsos
    if(now - t2 > 4000000)
        return 0;

    if(t1 > 0 && t2 > t1)
    {
        float delta = (t2 - t1) / 1000000.0f;
        float rpm = (1.0f / delta) * 60.0f;
        return (int)rpm;
    }

    return 0;
}

bool rpm_get_rotacion()
{
    int activos = 0;

    for(int i = 0; i < 5; i++)
    {
        if(rpm_get(i) > 0)
            activos++;
    }

    if(activos >= 2)
        return true;

    return false;
}

bool rpm_get_eje_cero ()  
{
    bool alguno_en_cero = false;

    for(int i = 0; i < 3; i++)
    {
        if(rpm_get(i) == 0)
            alguno_en_cero = true;
    }

    return alguno_en_cero;  
}