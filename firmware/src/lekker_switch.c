#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/interp.h"
#include "hardware/dma.h"
#include "lekker_switch.h"
#include "settings.h"
#include "console.h"
#include "switch.h"
#include "digital_switch.h"
#include "usb_hid.h"


#define ADC_SAMPLING_FREQUENCY_HZ USB_POLLING_RATE_HZ
#define ADC_CLK_FREQ_HZ           48000000

#define ADC_SAMPLE_CLK_FREQ_HZ (ADC_SAMPLING_FREQUENCY_HZ*NUM_LEKKER_SWITCH)
#define ADC_TOTAL_SAMPLE_FREQ_HZ (ADC_SAMPLING_FREQUENCY_HZ)

// Variables
uint8_t switch_travel[NUM_LEKKER_SWITCH];
uint16_t sample_buf[NUM_LEKKER_SWITCH];
uint32_t sample_mean[NUM_LEKKER_SWITCH];

// static
static uint16_t count;
static uint32_t sample_sum[NUM_LEKKER_SWITCH];
// pointer to the global address of bool array for switch status. Digital switches first, then Lekker switches
static bool *lekker_pressed = &switch_pressed[NUM_DIGITAL_SWITCH]; 


void adc_irq_handler();

/**
 * Initialize stuff
 * 
*/
int init_switch_adc()
{
    count = 0;

    // Initialize GPIO pins for use with ADC
    for(int i=0; i<NUM_LEKKER_SWITCH; ++i)
    {
        adc_gpio_init(26 + i);
    }
    
    // ------------------------ Init ADC ------------------------
    adc_init();
    adc_set_round_robin(0xf); // round robin between channels 0-3

    adc_fifo_setup(
        true,    // Write each completed conversion to the sample FIFO
        true,    // Enable DMA data request (DREQ)
        NUM_LEKKER_SWITCH,      // DREQ (and IRQ) asserted when at least 4 samples present
        false,   // We won't see the ERR bit because of 8 bit reads; disable.
        false    // Shift each sample to 8 bits when pushing to FIFO
    );

    // Set up for the given sample rate
    // This is all timed by the 48 MHz ADC clock.
    adc_set_clkdiv(ADC_CLK_FREQ_HZ/(ADC_SAMPLE_CLK_FREQ_HZ));

    adc_irq_set_enabled(true);

    irq_set_exclusive_handler(ADC_IRQ_FIFO, adc_irq_handler);
    irq_set_enabled(ADC_IRQ_FIFO, true);
    
    // ------------------------ Init Interpolator ------------------------
    // We use the interpolator to get the switch travel distance as an 8-bit
    // value between 0 and 255 (clamped)
    interp_config cfg = interp_default_config();
    interp_config_set_blend(&cfg, true); // blend mode to linear interpolate
    interp_config_set_clamp(&cfg, true); // Clamp output
    interp0->base[0] = 255; // always interpolate between 255 and 0
    interp0->base[1] = 0;

    interp_set_config(interp0, 0, &cfg);

    return 0;
}


void start_switch_adc()
{
    adc_run(true);
}


void stop_switch_adc()
{
    adc_run(false);
    adc_fifo_drain();
}

/**
 * Calculate the distance the switch has been pressed using calibration factors
 * The ADC counts go down as the switch is pressed.
*/
uint8_t calculate_travel(uint16_t adc_count, uint16_t rest_count, uint16_t pressed_count)
{
    int32_t value = 255 - ((int32_t)adc_count - (int32_t)pressed_count) * 255L / ((int32_t)rest_count - (int32_t)pressed_count); 
    value = value >= 0xFF ? 0xFF : value;
    value = value < 0 ? 0 : value;
    return (uint32_t) value;

    // Use hardware interpolator modulator
    // int alpha = (adc_count - pressed_count) * 255 / ((int)rest_count - (int)pressed_count);
     
}


/**
 * ADC IRQ handler. This is run a the USB polling rate and is used as the main interrupt
*/
void adc_irq_handler()
{
    int i;
    uint16_t sample;
    int write_mean = false;

    // Increment count everytime a new sample is generated
    if(++count >= ADC_TOTAL_SAMPLE_FREQ_HZ)
    {
        count = 0;
        write_mean = true;
    }

    data_log_entry_t data_log;

    // copy ADC data into sample buffer
    for(i=0; i<NUM_LEKKER_SWITCH; ++i)
    {
        sample = adc_hw->fifo;
        sample_buf[i] = sample & 0xFFF;
        // convert the ADC counts to travel distance
        switch_travel[i] = calculate_travel(sample & 0xFFF, settings.cal[i].top_count, settings.cal[i].bottom_count);

        // Compute if switch should be considered "pressed"
        if(settings.switch_config[i].rapid_trigger)
        {
            // If rapid trigger is on
        }
        else
        {
            // Standard mode (threshold with some hysteresis)
            if(lekker_pressed[i])
            {
                // If the switch is currently active, then need to press higher on counts (ie lower count) in order to release
                lekker_pressed[i] = switch_travel[i] > (settings.switch_config[i].threshold - SWITCH_HYSTERESIS_COUNTS);
            }
            else
            {
                // If the switch is currently not active, then need to press deeper on counts (ie higher count) in order to press
                lekker_pressed[i] = switch_travel[i] > (settings.switch_config[i].threshold + SWITCH_HYSTERESIS_COUNTS);
            }
        }

        // Log data
        data_log.switch_travel[i] = switch_travel[i];
        data_log.switch_pressed[i] = lekker_pressed[i];

        // Calculating stats
        sample_sum[i] += (uint32_t)sample;
        if(write_mean)
        {
            // calculate and store the mean
            sample_mean[i] = sample_sum[i]/ADC_TOTAL_SAMPLE_FREQ_HZ;
            sample_sum[i] = 0;
        }
    }

    // check digital switches
    check_digital_switch_status();

    // send USB report
    send_keyboard_report();

    // write the data in
    write_data_log(&data_log);
}
