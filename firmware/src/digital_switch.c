#include <pico/stdlib.h>
#include "digital_switch.h"
#include "parameters.h"
#include "switch.h"

bool *digital_switch_pressed = &switch_pressed[0]; // pointer to global array of switch status

static int debounce_count[NUM_DIGITAL_SWITCH];

static int digital_switch_pins[NUM_DIGITAL_SWITCH] = {STEPAD_K_COL0, STEPAD_K_COL1};

void init_digital_switches()
{
    int i;

    // Initialize one of the keyswitches so we can tell if we need to go into the bootloader
    gpio_init(STEPAD_K_ROW);
    gpio_set_dir(STEPAD_K_ROW, GPIO_OUT);
    gpio_init(STEPAD_K_COL0);
    gpio_set_dir(STEPAD_K_COL0, GPIO_IN);
    gpio_pull_down(STEPAD_K_COL0); // Enable pulldown resistor.
    gpio_set_dir(STEPAD_K_COL1, GPIO_IN);
    gpio_pull_down(STEPAD_K_COL1); // Enable pulldown resistor.

    // Just set the row to high since its only one row. Normally you would scan across rows.
    gpio_put(STEPAD_K_ROW, 1);

    // Initialize debouncer
    for(i=0; i<NUM_DIGITAL_SWITCH; ++i)
    {
        digital_switch_pressed[i] = false;
        debounce_count[i] = 0;
    }
}

void check_digital_switch_status()
{
    int i;
    bool state;
    for(i=0; i<NUM_DIGITAL_SWITCH; ++i)
    {
        state = gpio_get(digital_switch_pins[i]);
        if(digital_switch_pressed[i] ^ state)
        {
            // if switch state changed
            ++debounce_count[i];
            if(debounce_count[i] > DIGITAL_SWICH_DEBOUNCE_COUNT)
            {
                digital_switch_pressed[i] = state;
            }
        }
        else
        {
            debounce_count[i] = 0;
        }
    }
}
