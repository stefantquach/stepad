#ifndef CONSOLE_H_
#define CONSOLE_H_
#include "parameters.h"
#include <stdbool.h>

typedef struct data_log_entry
{
    uint8_t switch_travel[NUM_LEKKER_SWITCH];
    bool switch_pressed[NUM_LEKKER_SWITCH];
} data_log_entry_t;

void console_initialize();
void console_process(void);

void write_data_log(data_log_entry_t* data);

extern bool debug_console_enable;
extern bool calibration_in_progress;

#endif