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
#include "led_matrix.h"

#if 0
void blinky()
{
    const uint LED_COL0 = 5;
    const uint LED_PIN = 0;

    stdio_init_all();

    // Initialize GPIO    
    gpio_init(LED_COL0);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_COL0, GPIO_OUT);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_put(LED_COL0, 1); // Need to enable the column bc its a matrix

    // Initialize ADC
    adc_init();

    while (true) {
        printf("Hello, world!\n");
        uint32_t result = adc_read();
        float result_f = result*3.3/4096;
        printf("ADC read: %ld cnt, %.3f\n", result, result_f);
        gpio_put(LED_PIN, 1);
        sleep_ms(500);
        gpio_put(LED_PIN, 0);
        sleep_ms(500);
    }
}
#endif

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

    // TODO test lines
    set_led(0,0, 100, 0, 0);
    set_led(0,1, 0, 100, 0);
    set_led(0,2, 0, 0, 100);
    set_led(0,3, 100, 0, 100);

    start_led_process();

    while(1)
    {
        tight_loop_contents();
    }
}

/**
 * CPU0 Entry
*/
int main() {
    // blinky();
    stdio_init_all();

    // const uint LED_COL1 = 3;
    // const uint LED_COL3 = 5;
    // const uint LED_ROW_R = 0;

    // Initialize GPIO    
    // gpio_init(LED_COL1);
    // gpio_init(LED_COL3);
    // gpio_init(LED_ROW_R);
    // gpio_set_dir(LED_COL1, GPIO_OUT);
    // gpio_set_dir(LED_COL3, GPIO_OUT);
    // gpio_set_dir(LED_ROW_R, GPIO_OUT);
    // // Turn on one LED so we know application is running
    // gpio_put(LED_COL3, 1); // Need to enable the column bc its a matrix
    // gpio_put(LED_COL1, 0);
    // gpio_put(LED_ROW_R, 0);

    // Check for boot mode
    // This check is only performed on start up.
    check_for_boot();

    // Initialize CPU2
    multicore_launch_core1(core1_main);

    // Initialize processes
    init_switch_adc();

    start_switch_adc();

    // absolute_time_t start_time = get_absolute_time();
    printf("Starting CPU0 loop\n");
    while(1)
    {
        // printf("Capture finished\n");
        // absolute_time_t finish_time = get_absolute_time();
        // uint64_t time_diff_us = absolute_time_diff_us(start_time, finish_time);
        // start_time = finish_time;
        // sum += time_diff_us;

        // for(int i=0; i<4; ++i)
        // {
        //     uint16_t sample = sample_buf[i];
        //     float result_f = sample*3.3/4096;
        //     printf("ADC%d: %d cnt, %.3f\n", i, sample, result_f);
        // }
        // printf("%lld\n", time_diff_us);
        // return 0;
        tight_loop_contents();
    }

}


