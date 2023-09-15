#include "pico/stdlib.h"
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <string.h>
#include "settings.h"

switch_settings_t settings;
bool changed;

lekker_calibration_t default_calibration[NUM_LEKKER_SWITCH] = 
{   // Top   Bottom
    {  2036, 1513  },
    {  2091, 1464  },
    {  2012, 1311  },
    {  2052, 1478  },
};

static void write_settings_to_flash();
static bool read_settings_from_flash();

void initialize_settings(void)
{
    int i;
    // Initialize the keymap interface (this also loads default keymap)
    initialize_keymap();
    // Read in the current values from flash
    if(!read_settings_from_flash())
    {
        // If there are no values in flash, use defaults
        memcpy(&settings.cal[0], &default_calibration[0], sizeof(default_calibration));
        for(i=0; i<NUM_LEKKER_SWITCH; ++i)
        {
            settings.switch_config[i].threshold = 127; // Default to the middle
            settings.switch_config[i].rapid_trigger_mode = RAPID_TRIGGER_OFF;
        }
    }
}


void process_settings(void)
{
    // Check for any changes in settings

}


void write_settings_to_flash()
{
    // Need to make sure no interrupts happen when writing to flash
    uint32_t ints = save_and_disable_interrupts();

    restore_interrupts(ints);
}


bool read_settings_from_flash()
{
    // int a = PICO_FLASH_SIZE_BYTES;
    // int b = FLASH_SECTOR_SIZE;
    return false;
}
