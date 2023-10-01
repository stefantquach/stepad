#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <stdio.h>
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <hardware/dma.h>
#include "hardware/structs/ssi.h"
#include <string.h>
#include "settings.h"
#include "lekker_switch.h"
#include "keymap.h"
/**
 * Settings for the STEPAD are stored in flash in order to be persistent between reboots.
 * Note that the Flash has a given write endurance and cannot be erased and written
 * indefinitely.
 * 
 * The last sector of flash is chosen to store these settings in order to avoid
 * interfering with the program. This mostly works since the struct itself is smaller than one page.
 * If this gets bigger, a linker script might be needed to make sure this sector doesnt get 
 * program written to.
 * 
 * To try to mitigate wear on the flash by erasing, we will write up the entire sector before 
*/

// Defines
#define CRC32_INIT                  ((uint32_t)-1l)
#define FLASH_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

switch_settings_t settings;
bool changed;

lekker_calibration_t default_calibration[NUM_LEKKER_SWITCH] = 
{   // Top   Bottom
    {  2036, 1513  },
    {  2091, 1464  },
    {  2012, 1311  },
    {  2052, 1478  },
};

static void write_settings_to_flash(switch_settings_t* settings_);
static bool read_settings_from_flash();

static uint32_t copy_and_compute_crc32(void* src, void* dst, size_t len);
void __no_inline_not_in_flash_func(flash_bulk_read)(uint32_t *rxbuf, uint32_t flash_offs, size_t len,
                                                 uint dma_chan);


/**
 * 
*/
void initialize_settings(void)
{
    // Initialize the keymap interface (this also loads default keymap)
    initialize_keymap();
    // Read in the current values from flash
    if(read_settings_from_flash() == false)
    // if(true)
    {
        printf("Settings not found in flash, revert to default\n");
        // If there are no values in flash, use defaults
        load_default_settings();
    }
    printf("Finished initializing settings\n");
}


/**
 * Checks for changes in settings and writes to flash if there are changes.
 * 
*/
void process_settings(void)
{
    switch_settings_t settings_copy;
    uint32_t sniffed_crc = copy_and_compute_crc32(&settings, &settings_copy, sizeof(switch_settings_t) - 4);

    printf("processing settings\n");
    if (settings.crc32 != sniffed_crc)
    {
        settings_copy.crc32 = sniffed_crc;
        // write settings
        write_settings_to_flash(&settings_copy);
        settings.crc32 = sniffed_crc;
    }
}


/**
 * 
*/
void load_default_settings(void)
{
    int i;
    printf("Loading default settings\n");
    memcpy(settings.keymap, default_keymap, sizeof(default_keymap));
    memcpy(&settings.cal[0], &default_calibration[0], sizeof(default_calibration));
    for(i=0; i<NUM_LEKKER_SWITCH; ++i)
    {
        settings.switch_config[i].threshold = 127; // Default to the middle
        settings.switch_config[i].rapid_trigger_sensitivity = 20; // Default rapid trigger sensitivity is ~0.3mm
        settings.switch_config[i].rapid_trigger_mode = RAPID_TRIGGER_OFF;
    }
    process_settings();
}



/**
 *  Write settings into flash.
*/
void write_settings_to_flash(switch_settings_t* settings_)
{
    printf("Writing settings to flash\n");
    int page;
    // Stop ADCs to prevent desync in samples
    stop_switch_adc();
    // Need to make sure no interrupts happen when writing to flash
    uint32_t ints = save_and_disable_interrupts();
    // Halt execution on the other CPU
    if(multicore_lockout_victim_is_initialized(!*((uint32_t*)SIO_BASE)))
    {
        printf("CPU%d is initialized, locking", *((uint32_t*)SIO_BASE));
        multicore_lockout_start_blocking();
    }
    // Find the first empty page of Flash to write to. If no empty pages, erase sector.
    // NOTE: 0xFFFFFFFF represents an erased sector
    int* addr;
    bool found_addr = false;
    for (page=0; page<FLASH_SECTOR_SIZE/FLASH_PAGE_SIZE; ++page)
    {
        addr = (int*)(XIP_BASE + FLASH_OFFSET + (page * FLASH_PAGE_SIZE));
        if (*addr == -1)
        {
            found_addr = true;
            break;
        }
    }

    if (!found_addr)
    {
        printf("Erasing flash sector\n");
        // No page available, erase the whole sector
        flash_range_erase(FLASH_OFFSET, FLASH_SECTOR_SIZE);
        page = 0; // set page to zero
    }
    // else we already know what page to write to
    printf("Writing to page %d\n", page);

    // write settings to a given page
    flash_range_program(FLASH_OFFSET + (page * FLASH_PAGE_SIZE), (uint8_t*)settings_, FLASH_PAGE_SIZE);

    // Reenable execution on the other CPU
    if(multicore_lockout_victim_is_initialized(!*((uint32_t*)SIO_BASE)))
        multicore_lockout_end_blocking();
    // reenable interrupts
    restore_interrupts(ints);
    // restart ADCs
    start_switch_adc();
}


/**
 * Read a sector of flash and find the settings in it. This is called once on initialization.
 * Must be called before CPU1 initiates
 * 
 * This is meant to be called once on initialization
*/
bool read_settings_from_flash()
{
    int page;
    int* addr;
    int* addr_next;
    printf("Reading settings from flash\n");

    // Read in the whole sector of flash where the settings struct is located
    uint8_t buf[FLASH_SECTOR_SIZE];
    int chan = dma_claim_unused_channel(true);
    flash_bulk_read((uint32_t*)&buf, FLASH_OFFSET, FLASH_SECTOR_SIZE/4, chan);
    dma_channel_unclaim(chan);

    printf("Finished reading flash\n");

    // Find the page of flash where the settings struct is
    bool found_page = false;
    for(page = 0; page < FLASH_SECTOR_SIZE/FLASH_PAGE_SIZE - 1; ++page)
    {
        addr = (int*)&buf[page*FLASH_PAGE_SIZE];
        addr_next = (int*)&buf[(page+1)*FLASH_PAGE_SIZE];
        if(*addr != -1 && *addr_next == -1)
        {
            found_page = true;
            break;
        }
    }
    // check the last page
    addr = (int*)&buf[FLASH_SECTOR_SIZE - FLASH_PAGE_SIZE];
    if(!found_page && *addr != -1)
    {
        page = FLASH_SECTOR_SIZE/FLASH_PAGE_SIZE - 1;
        found_page = true;
    }
    // If we found some data in flash
    if(found_page)
    {
        printf("Found settings page at page %d\n", page);
        // Copy into settings global variable to check CRC
        uint32_t crc = copy_and_compute_crc32(&buf[page*FLASH_PAGE_SIZE], &settings, sizeof(switch_settings_t) - 4); // exclude the crc
        switch_settings_t* temp_settings = (switch_settings_t*)&buf[page*FLASH_PAGE_SIZE];
        if (crc == temp_settings->crc32)
        {
            settings.crc32 = crc;
            return true;
        }
        else
        {
            // zero out the settings if crc doesnt pass
            printf("Failed CRC check\n");
            memset(&settings, 0, sizeof(switch_settings_t));
            return false;
        }
    }

    printf("No settings found in flash\n");
    return false;
}


uint32_t copy_and_compute_crc32(void* src, void* dst, size_t len)
{
    // Use the DMA to copy over into a local buffer (and calculate the CRC)
    int chan = dma_claim_unused_channel(true);

    // 8 bit transfers. 
    dma_channel_config c = dma_channel_get_default_config(chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, true);

    // (bit-reverse) CRC32 specific sniff set-up
    channel_config_set_sniff_enable(&c, true);
    dma_sniffer_set_data_accumulator(CRC32_INIT);
    dma_sniffer_set_output_reverse_enabled(true);
    dma_sniffer_enable(chan, DMA_SNIFF_CTRL_CALC_VALUE_CRC32R, true);

    dma_channel_configure(
        chan,          // Channel to be configured
        &c,            // The configuration we just created
        dst,      // The write address
        src,      // The initial read address
        len,      // Total number of transfers inc. Exclude crc32
        true           // Start immediately.
    );

    // Wait for the DMA to finish
    dma_channel_wait_for_finish_blocking(chan);

    uint32_t sniffed_crc = dma_sniffer_get_data_accumulator();
    
    dma_channel_unclaim(chan);

    return sniffed_crc;
}


/**
 * Bulk flash read. Copied from the ssi_dma example from the pico-examples
*/
void __no_inline_not_in_flash_func(flash_bulk_read)(uint32_t *rxbuf, uint32_t flash_offs, size_t len,
                                                 uint dma_chan) 
{
    // SSI must be disabled to set transfer size. If software is executing
    // from flash right now then it's about to have a bad time
    ssi_hw->ssienr = 0;
    ssi_hw->ctrlr1 = len - 1; // NDF, number of data frames
    ssi_hw->dmacr = SSI_DMACR_TDMAE_BITS | SSI_DMACR_RDMAE_BITS;
    ssi_hw->ssienr = 1;
    // Other than NDF, the SSI configuration used for XIP is suitable for a bulk read too.

    // Configure and start the DMA. Note we are avoiding the dma_*() functions
    // as we can't guarantee they'll be inlined
    dma_hw->ch[dma_chan].read_addr = (uint32_t) &ssi_hw->dr0;
    dma_hw->ch[dma_chan].write_addr = (uint32_t) rxbuf;
    dma_hw->ch[dma_chan].transfer_count = len;
    // Must enable DMA byteswap because non-XIP 32-bit flash transfers are
    // big-endian on SSI (we added a hardware tweak to make XIP sensible)
    dma_hw->ch[dma_chan].ctrl_trig =
            DMA_CH0_CTRL_TRIG_BSWAP_BITS |
            DREQ_XIP_SSIRX << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB |
            dma_chan << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB |
            DMA_CH0_CTRL_TRIG_INCR_WRITE_BITS |
            DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_WORD << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB |
            DMA_CH0_CTRL_TRIG_EN_BITS;

    // Now DMA is waiting, kick off the SSI transfer (mode continuation bits in LSBs)
    ssi_hw->dr0 = (flash_offs << 8u) | 0xa0u;

    // Wait for DMA finish
    while (dma_hw->ch[dma_chan].ctrl_trig & DMA_CH0_CTRL_TRIG_BUSY_BITS);

    // Reconfigure SSI before we jump back into flash!
    ssi_hw->ssienr = 0;
    ssi_hw->ctrlr1 = 0; // Single 32-bit data frame per transfer
    ssi_hw->dmacr = 0;
    ssi_hw->ssienr = 1;
}
