#include <stdio.h>
#include "pico/stdlib.h"
#include <string.h>
#include "console.h"
#include "lekker_switch.h"
#include "settings.h"
#include "parameters.h"

bool debug_console_enable = false;

#define MAX_NUM_DATA_LOG_ENTRIES 10000 // Maximum number of log entries. 10 Secs at 1kHz

enum
{
    CONSOLE_STATE_DRAW_MENU,
    CONSOLE_STATE_WAIT_MENU,
    CONSOLE_STATE_1HZ_ADC,
    CONSOLE_STATE_CALIBRATION,
};

enum
{
    CALIBRATION_STATE_START,
    CALIBRATION_STATE_READ_REST,
    CALIBRATION_STATE_PRINT_PRESSED_INSTR,
    CALIBRATION_STATE_READ_PRESSED,
    CALIBRATION_STATE_WRITE_FLASH
};

//============== Static variables============
static int state;

// For setting switch
static int calibration_state;
static int calibration_index;
static lekker_calibration_t calibrated_values[NUM_LEKKER_SWITCH];

// For data log
static data_log_entry_t data_log_buffer[MAX_NUM_DATA_LOG_ENTRIES];
static int data_log_index;
static bool data_log_enabled;

void console_initialize()
{
    state = CONSOLE_STATE_WAIT_MENU;
}

void print_menu()
{
    printf(
        "Stepad Debug Console:\n"
        "================================\n"
        "A) Print 1Hz ADC counts\n"
        "B) Start Calibration procedure\n"
        "C) Print current calibration settings\n"
        "D) Start Switch distance data logging\n"
        "E) Stop and Print logged data\n"
        "F) Load default settings\n"
        "\n\n"
    );
}


void print_calibration_settings()
{
    int i;
    for(i=0; i<NUM_LEKKER_SWITCH; ++i)
    {
        printf("Switch %d: Top: %d\tBottom:%d\n", i, settings.cal[i].top_count, settings.cal[i].bottom_count);
    }
}


void print_logged_data()
{
    int i,j;
    // print header
    printf("index");
    for(i=0; i < NUM_LEKKER_SWITCH; ++i)
    {
        printf(",sw%d travel,sw%d pressed", i,i);
    }
    printf("\n");

    // print data
    for(i=0; i < data_log_index; ++i)
    {
        printf("%d", i);
        for(j=0; j < NUM_LEKKER_SWITCH; ++j)
        {
            printf(",%d,%d", (int)data_log_buffer[i].switch_travel[j], (int)data_log_buffer[i].switch_pressed[j]);
        }
        printf("\n");
    }
}


void process_menu_inputs(int c)
{
    switch (c)
    {
        case PICO_ERROR_TIMEOUT:
            break;
        
        // Start 1Hz ADC monitor
        case 'A':
        case 'a':
            state = CONSOLE_STATE_1HZ_ADC;
            break;

        // Start Switch calibration
        case 'B':
        case 'b':
            calibration_index = 0;
            calibration_state = CALIBRATION_STATE_START;
            state = CONSOLE_STATE_CALIBRATION;
            break;

        // Print calibration settings
        case 'C':
        case 'c':
            print_calibration_settings();
            state = CONSOLE_STATE_DRAW_MENU;
            break;

        // Start data logging
        case 'D':
        case 'd':
            data_log_index = 0;
            data_log_enabled = true;
            printf("Starting Data log\n");
            break;
        case 'E':
        case 'e':
            data_log_enabled = false;
            printf("Stopping data log\n");
            print_logged_data();
            data_log_index = 0;
            break;

        case 'F':
        case 'f':
            printf("Loading default settings");
            load_default_settings();
            break;
        default:
            print_menu();
            break;
    }
}

void console_1Hz_adc_process(int c)
{
    static absolute_time_t last_time = {0};
    absolute_time_t current_time = get_absolute_time();
    uint64_t time_diff_us = absolute_time_diff_us(last_time, current_time);
    if(time_diff_us > 1e6)
    {
        last_time = current_time;
        for(int i=0; i<NUM_LEKKER_SWITCH; ++i)
        {
            uint16_t sample = sample_buf[i];
            float result_f = sample*3.3/4096;
            printf("ADC%d: %d cnt %.3f\tmean count: %ld\ttravel: %d\n", i, sample, result_f, sample_mean[i], switch_travel[i]);
        }
        printf("\n");
        printf("Press any button to stop\n");
    }
    
    if(c != PICO_ERROR_TIMEOUT)
    {
        state = CONSOLE_STATE_DRAW_MENU;
    }
}


void calibration_process(int c)
{
    int i;
    if(c == 'q' || c == 'Q')
    {
        state = CONSOLE_STATE_WAIT_MENU;
    }

    switch(calibration_state)
    {
        case CALIBRATION_STATE_START:
            printf("Starting calibration procedure. Press Q to cancel anytime.\n");
            printf("Leave all keys in their unpressed position, wait 1 second, then press C\n");
            calibration_state = CALIBRATION_STATE_READ_REST;
            break;

        case CALIBRATION_STATE_READ_REST:
            // Read all the rest once C is pressed
            if(c == 'c' || c == 'C')
            {
                for(i=0; i<NUM_LEKKER_SWITCH; ++i)
                {
                    calibrated_values[i].top_count = sample_mean[i];
                    printf("Rest ADC%d: %ld\n", i, sample_mean[i]);
                }
                calibration_state = CALIBRATION_STATE_PRINT_PRESSED_INSTR;
            }
            break;

        case CALIBRATION_STATE_PRINT_PRESSED_INSTR:
            printf("Press analog switch %d as far as it can travel. Wait 1 sec then press C to continue\n", calibration_index+1);
            calibration_state = CALIBRATION_STATE_READ_PRESSED; 
            break;

        case CALIBRATION_STATE_READ_PRESSED:
            if(c == 'c' || c == 'C')
            {
                calibrated_values[calibration_index].bottom_count = sample_mean[calibration_index];
                printf("Pressed ADC%d: %d\n", calibration_index, sample_mean[calibration_index]);
                if(++calibration_index >= NUM_LEKKER_SWITCH)
                {
                    calibration_state = CALIBRATION_STATE_WRITE_FLASH;
                }
                else
                {
                    calibration_state = CALIBRATION_STATE_PRINT_PRESSED_INSTR;
                }
            }
            break;

        case CALIBRATION_STATE_WRITE_FLASH:
            printf("Finished Calibration procedure\n");
            // Copy values into local settings
            memcpy(&settings.cal[0], &calibrated_values[0], sizeof(calibrated_values));
            process_settings();

            for(i=0; i<NUM_LEKKER_SWITCH; ++i)
            {
                printf("Switch %d: Top: %d\tBottom:%d\n", i, settings.cal[i].top_count, settings.cal[i].bottom_count);
            }
            calibration_state = CALIBRATION_STATE_START;
            calibration_index = 0;
            state = CONSOLE_STATE_DRAW_MENU;
            break;
        
        default:
            calibration_state = CALIBRATION_STATE_START;
            calibration_index = 0;
            state = CONSOLE_STATE_DRAW_MENU;
            break;
    }
}


void write_data_log(data_log_entry_t* data)
{
    if(data_log_enabled)
    {
        if(data_log_index < MAX_NUM_DATA_LOG_ENTRIES)
        {
            memcpy(&data_log_buffer[data_log_index], data, sizeof(data_log_entry_t));
            ++data_log_index;
        }
        else
        {
            // Buffer is full, forcefully turn off logging
            data_log_enabled = false;
        }
        
    }
}


void console_process(void)
{
    int c = getchar_timeout_us(0);

    switch(state)
    {
        case CONSOLE_STATE_DRAW_MENU:
            print_menu();
            state = CONSOLE_STATE_WAIT_MENU;
            break;
        case CONSOLE_STATE_WAIT_MENU:
            process_menu_inputs(c);
            break;

        case CONSOLE_STATE_1HZ_ADC:
            console_1Hz_adc_process(c);
            break;

        case CONSOLE_STATE_CALIBRATION:
            calibration_process(c);
            break;
        default:
            state = CONSOLE_STATE_WAIT_MENU;
            break;
    }
}