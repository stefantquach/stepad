#include "led_effects.h"
#include "led_matrix.h"
#include "lekker_switch.h"
#include "utility_colors.h"
#include "parameters.h"

// HSL To RGB converter
// rgb_t hsl2rgb(hsl_t rgb);

void hue_travel(void)
{
    int i;
    // hsl_t hsl;
    for(i=0; i<NUM_LEKKER_SWITCH; ++i)
    {
        uint8_t travel = switch_travel[i];
        set_led(0, i, travel, 0, travel);
    }
}


