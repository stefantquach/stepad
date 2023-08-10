#include "keymap.h"
#include "keycodes.h"
#include <string.h>

uint16_t keymap[MAX_KEY_LAYERS][TOTAL_NUM_SWITCH];

static uint16_t default_keymap[MAX_KEY_LAYERS][TOTAL_NUM_SWITCH] = 
{
    {
              KC_R, KC_U, 
        KC_D, KC_F, KC_J, KC_K
    },
    {0}
};

static int layer_state;

void initialize_keymap()
{
    // Try getting it from flash TODO

    // Else if failed, load default
    memcpy(keymap, default_keymap, sizeof(keymap));

    // initialize normal variables
    layer_state = 0;
}


uint16_t get_keycode(uint16_t switch_id)
{
    // TODO implement layers
    return keymap[layer_state][switch_id];
}