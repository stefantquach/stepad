#ifndef LEKKER_SWITCH_H_
#define LEKKER_SWITCH_H_
#include <stdbool.h>
#include <stdint.h>
#include "parameters.h"

#define NUM_LEKKER_SWITCH 4

int init_switch_adc();

void start_switch_adc();
void stop_switch_adc();

extern uint16_t sample_buf[NUM_LEKKER_SWITCH];
extern uint32_t sample_mean[NUM_LEKKER_SWITCH];

extern uint8_t switch_travel[NUM_LEKKER_SWITCH];
extern bool switch_pressed[NUM_LEKKER_SWITCH];

#endif
