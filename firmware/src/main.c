/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "lekker_switch.h"
#include "digital_switch.h"
#include "led_matrix.h"
#include "led_effects.h"
#include "console.h"
#include "settings.h"
#include "parameters.h"
#include "keymap.h"

#include "bsp/board.h"
#include "tusb.h"

/**
 * Helper function to check if bootloader needs to be entered.
 * The desired behavior is to check this once after a reset.
*/
void check_for_boot()
{
    // Initialize one of the keyswitches so we can tell if we need to go into the bootloader
    gpio_init(STEPAD_K_ROW);
    gpio_set_dir(STEPAD_K_ROW, GPIO_OUT);
    gpio_init(STEPAD_K_COL0);
    gpio_set_dir(STEPAD_K_COL0, GPIO_IN);
    gpio_pull_down(STEPAD_K_COL0); // Enable pulldown resistor.

    // Check if Switch is pressed (COL0 is high). If so enter bootloader
    gpio_put(STEPAD_K_ROW, 1);
    for(volatile int i=0; i<2000; ++i)
        ; // wait a bit for the pin level to rise
    if(gpio_get(STEPAD_K_COL0))
    {
        reset_usb_boot(0, 0);
    }
}

/**
 * CPU1 Entry
*/
void core1_main() {
    printf("Starting CPU1 loop\n");
    // initialize LED matrix
    initialize_led_pwms();

    start_led_process();

    absolute_time_t start_time = get_absolute_time();
    while(1)
    {
        // tight_loop_contents();
        absolute_time_t finish_time = get_absolute_time();
        uint64_t time_diff_us = absolute_time_diff_us(start_time, finish_time);
        if(time_diff_us > 100000ULL)
        {
            start_time = finish_time;
            
            hue_travel();
        }
    }
}

/**
 * CPU0 Entry
*/
int main() {
    // blinky();
    stdio_init_all();

    // Check for boot mode
    // This check is only performed on start up.
    check_for_boot();

    // Initialize TinyUSB
    board_init();
    tusb_init();

    // Initialize CPU2
    multicore_launch_core1(core1_main);

    // Initialize processes
    init_switch_adc();
    init_digital_switches();
    initialize_settings();
    console_initialize();

    // Start ADCs
    start_switch_adc();

    printf("Starting CPU0 loop\n");
    while(1)
    {
        console_process();

        tud_task();
    }

}
