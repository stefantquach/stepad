#ifndef KEYMAP_H_
#define KEYMAP_H_
#include <stdint.h>
#include "parameters.h"
#include "keycodes.h"

extern uint16_t keymap[MAX_KEY_LAYERS][TOTAL_NUM_SWITCH];

void initialize_keymap();

uint16_t get_keycode(uint16_t switch_id);


#endif