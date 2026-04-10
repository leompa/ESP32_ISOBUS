//================================================================================================
/// @file main.cpp
///
/// @brief Defines `main` for the seeder example
/// @details This example is meant to use all the major protocols in a more "complete" application.
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
 #include "freertos/FreeRTOS.h"
 #include "freertos/timers.h"
 //#include "objectPoolObjects.h"
 #include "seeder.hpp"
 
 #include <functional>
 #include <iostream>
 #include <memory>
 #include <driver/twai.h>
 #include "driver/gpio.h"
#include "driver/gpio.h"

 #define rele1     GPIO_NUM_14
 #define rele2     GPIO_NUM_12
 #define rele3     GPIO_NUM_13
 #define rele4     GPIO_NUM_15

 #define led_rojo   GPIO_NUM_22
 #define led_verde  GPIO_NUM_23
 #define led_azul   GPIO_NUM_21

 void init_hardware_outputs()
{
    // Configurar RELÉS como salida
    gpio_set_direction(rele1, GPIO_MODE_OUTPUT);
    gpio_set_direction(rele4, GPIO_MODE_OUTPUT);
	gpio_set_direction(rele2, GPIO_MODE_OUTPUT);
    gpio_set_direction(rele3, GPIO_MODE_OUTPUT);


    // Configurar LEDs como salida
    gpio_set_direction(led_rojo, GPIO_MODE_OUTPUT);
    gpio_set_direction(led_verde, GPIO_MODE_OUTPUT);
    gpio_set_direction(led_azul, GPIO_MODE_OUTPUT);

	vTaskDelay(150);
    // Apagar LEDs
    gpio_set_level(led_rojo, 0);
    gpio_set_level(led_verde, 0);
    gpio_set_level(led_azul, 0);

	vTaskDelay(150);
    // Apagar relés
    gpio_set_level(rele1, 0);
    gpio_set_level(rele2, 0);
	gpio_set_level(rele3, 0);
	gpio_set_level(rele4, 0);
}


 extern "C" void app_main()
 {	
	vTaskDelay(1500);
	init_hardware_outputs();
	gpio_set_level(led_rojo, 1);
	gpio_set_level(rele1, 0);
    gpio_set_level(rele2, 0);
	gpio_set_level(rele3, 0);
	gpio_set_level(rele4, 0);
	//PCA9685Handler::init();
	vTaskDelay(1500);
	Seeder seederExample;
 	seederExample.initialize();

 	while (true)
 	{	gpio_set_level(led_rojo, 0);
		gpio_set_level(led_verde, 1);
 		// CAN stack runs in other threads. Do nothing forever.
 		seederExample.update();

 		vTaskDelay(10);
 	}
 }
 
