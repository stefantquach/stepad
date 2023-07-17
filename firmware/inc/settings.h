#ifndef SETTINGS_H_
#define SETTINGS_H_
#include <stdint.h>
#include "lekker_switch.h"

typedef struct settings
{
    // Calibration factors (ADC count at the top and bottom of switch travel)
    uint16_t top_count;
    uint16_t bottom_count;

    // Settings
    uint16_t threshold;
    bool rapid_trigger;

} lekker_switch_settings_t;

#endif