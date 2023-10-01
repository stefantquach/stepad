#ifndef SETTINGS_H_
#define SETTINGS_H_
#include <stdint.h>
#include <assert.h>
#include <hardware/flash.h>
#include "lekker_switch.h"
#include "parameters.h"
#include "keymap.h"

//--------------------------------------------------
// Typedefs for structs to put into flash
//--------------------------------------------------
typedef struct lekker_calibration
{
    // Calibration factors (ADC count at the top and bottom of switch travel)
    uint16_t top_count;
    uint16_t bottom_count;
} lekker_calibration_t;

enum rapid_trigger_enum
{
    RAPID_TRIGGER_OFF = 0,
    RAPID_TRIGGER_MODE_NORMAL,
    RAPID_TRIGGER_MODE_CONTINUOUS,
};

typedef struct lekker_config
{
    uint16_t threshold;
    int rapid_trigger_mode;
} lekker_config_t;

typedef struct settings
{
    // Calibration
    lekker_calibration_t cal[NUM_LEKKER_SWITCH];

    // Configurations
    lekker_config_t switch_config[NUM_LEKKER_SWITCH];

    // Keymap
    uint16_t keymap[MAX_KEY_LAYERS][TOTAL_NUM_SWITCH];

    // N-key Rollover enable
    bool nkey_rollover;
    
    // CRC at the end to verify integrity
    uint32_t crc32;
} switch_settings_t;

static_assert(sizeof(switch_settings_t) <= FLASH_PAGE_SIZE, "Switch setting struct is larger than 1 page");

void initialize_settings(void);
void process_settings(void);

void load_default_settings(void);


// void update_settings(uint16_t switch_id);

// Cached version of settings
extern switch_settings_t settings;

#endif