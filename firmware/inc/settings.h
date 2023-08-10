#ifndef SETTINGS_H_
#define SETTINGS_H_
#include <stdint.h>
#include "lekker_switch.h"
#include "parameters.h"

//--------------------------------------------------
// Typedefs for structs to put into flash
//--------------------------------------------------
typedef struct lekker_calibration
{
    // Calibration factors (ADC count at the top and bottom of switch travel)
    uint16_t top_count;
    uint16_t bottom_count;
} lekker_calibration_t;

typedef struct lekker_config
{
    uint16_t threshold;
    bool rapid_trigger;
} lekker_config_t;

typedef struct settings
{
    // N-key Rollover enable
    bool nkey_rollover;

    // Calibration
    lekker_calibration_t cal[NUM_LEKKER_SWITCH];

    // Configurations
    lekker_config_t switch_config[NUM_LEKKER_SWITCH];

    // TODO keymap

    // CRC at the end to verify integrity
    uint32_t crc32;
} switch_settings_t;

void initialize_settings(void);
void process_settings(void);

// void update_settings(uint16_t switch_id);

// Cached version of settings
extern switch_settings_t settings;

#endif