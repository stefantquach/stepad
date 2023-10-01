#include "keymap.h"
#include "keycodes.h"
#include "settings.h"
#include <string.h>

uint16_t default_keymap[MAX_KEY_LAYERS][TOTAL_NUM_SWITCH] = 
{
    {
              KC_R, KC_U, 
        KC_D, KC_F, KC_J, KC_K
    },
    {0}
};

static int layer_state;

typedef union
{
    uint16_t code;
} action_t;


void initialize_keymap()
{
    // initialize normal variables
    layer_state = 0;
}



uint16_t get_keycode(uint16_t switch_id)
{
    // TODO implement layers
    return settings.keymap[layer_state][switch_id];
}