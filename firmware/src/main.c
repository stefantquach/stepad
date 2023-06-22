/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include "pico/stdlib.h"

int main() {
    const uint LED_COL0 = 5;
    const uint LED_PIN = 0;

    stdio_init_all();
    
    gpio_init(LED_COL0);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_COL0, GPIO_OUT);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_put(LED_COL0, 1);
    while (true) {
        printf("Hello, world!\n");
        gpio_put(LED_PIN, 1);
        sleep_ms(500);
        gpio_put(LED_PIN, 0);
        sleep_ms(500);
    }
}
