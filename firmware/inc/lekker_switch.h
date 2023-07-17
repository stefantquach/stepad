#ifndef LEKKER_SWITCH_H_
#define LEKKER_SWITCH_H_

#define NUM_RPI_ADC 4

int init_switch_adc();

void start_switch_adc();
void stop_switch_adc();

extern uint16_t sample_buf[NUM_RPI_ADC];
extern bool switch_pressed[NUM_RPI_ADC];
extern uint dma_chan;
#endif
