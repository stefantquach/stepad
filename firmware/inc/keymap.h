#ifndef KEYMAP_H_
#define KEYMAP_H_
#include <stdint.h>
#include "parameters.h"
#include "keycodes.h"

void initialize_keymap();

uint16_t get_keycode(uint16_t switch_id);


#endif