#include "pico/stdlib.h"
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <string.h>
#include "settings.h"

switch_settings_t settings;
bool changed;

static void write_settings_to_flash();
static bool read_settings_from_flash();

void initialize(void)
{
    // int i;

    // Read in the current values from flash
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
