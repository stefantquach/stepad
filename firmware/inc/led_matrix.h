#ifndef LED_MATRIX_H_
#define LED_MATRIX_H_
#include <stdint.h>

#define NUM_ROWS 1
#define NUM_COLS 4

void initialize_led_pwms();
void start_led_process();

void set_led(int row, int col, uint8_t r, uint8_t g, uint8_t b);



#endif