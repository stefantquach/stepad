#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "lekker_switch.h"

#define ADC_SAMPLING_FREQUENCY_HZ 1000
#define ADC_CLK_FREQ_HZ           48000000

// Variables
bool switch_pressed[NUM_RPI_ADC];
uint16_t sample_buf[NUM_RPI_ADC];
uint dma_chan;

// static
static uint16_t count;

// TODO remove later. Just for time measurements for now
absolute_time_t start_time;
uint64_t sum = 0;

void adc_irq_handler();

static void arm_dma()
{
    // Initialize DMA
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);

    // Reading from constant address, writing to incrementing byte addresses
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);

    // Pace transfers based on availability of ADC samples
    channel_config_set_dreq(&cfg, DREQ_ADC);

    dma_channel_configure(dma_chan, &cfg,
        sample_buf,     // dst
        &adc_hw->fifo,  // src
        NUM_RPI_ADC,    // transfer count
        true            // start immediately
    );
}

int init_switch_adc()
{
    count = 0;

    // Initialize GPIO pins for use with ADC
    for(int i=0; i<NUM_RPI_ADC; ++i)
    {
        adc_gpio_init(26 + i);
    }
    
    // Init ADC
    adc_init();
    adc_set_round_robin(0xf); // round robin between channels 0-3

    adc_fifo_setup(
        true,    // Write each completed conversion to the sample FIFO
        true,    // Enable DMA data request (DREQ)
        NUM_RPI_ADC,      // DREQ (and IRQ) asserted when at least 4 samples present
        false,   // We won't see the ERR bit because of 8 bit reads; disable.
        false    // Shift each sample to 8 bits when pushing to FIFO
    );

    // Set up for the given sample rate
    // This is all timed by the 48 MHz ADC clock.
    adc_set_clkdiv(ADC_CLK_FREQ_HZ/(ADC_SAMPLING_FREQUENCY_HZ*NUM_RPI_ADC));

    adc_irq_set_enabled(true);

    irq_set_exclusive_handler(ADC_IRQ_FIFO, adc_irq_handler);
    irq_set_enabled(ADC_IRQ_FIFO, true);

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


void adc_irq_handler()
{
    // const uint16_t threshold = 1500; // TODO change this to settings.c
    int i;
    uint16_t sample;
    float result_f;

    // copy ADC data into sample buffer
    for(i=0; i<NUM_RPI_ADC; ++i)
    {
        sample = adc_hw->fifo;
        sample_buf[i] = sample & 0xFFF;
        // convert the ADC counts to travel distance
        
        // TODO Check out hardware interpolation module on RP2040

        // TODO debounce
        // switch_pressed[i] = true;
    }

    // Everything below this is for test reasons
    absolute_time_t finish_time = get_absolute_time();
    uint64_t time_diff_us = absolute_time_diff_us(start_time, finish_time);
    start_time = finish_time;
    sum += time_diff_us;

    if(++count >= ADC_SAMPLING_FREQUENCY_HZ)
    {
        // reset
        count = 0;
        sum = 0;

        // print adc counts
        for(int i=0; i<4; ++i)
        {
            sample = sample_buf[i];
            result_f = sample*3.3/4096;
            printf("ADC%d: %d cnt, %.3f\n", i, sample, result_f);
        }
        printf("Average diff: %lld\n", time_diff_us/count);
    }
}

// Change ADC count to 8 bit value representing 0-4mm
// (sample - bottom_count) * (256)/(max_count - bottom_count)