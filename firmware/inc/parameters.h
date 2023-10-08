#ifndef PARAMETERS_H_
#define PARAMETERS_H_

#define USB_POLLING_RATE_HZ 1000

#define NUM_LEKKER_SWITCH 4
#define NUM_DIGITAL_SWITCH 2
#define TOTAL_NUM_SWITCH (NUM_LEKKER_SWITCH + NUM_DIGITAL_SWITCH)

// Number of counts around the threshold that the hysteresis applies. This is in Travel units which is 4mm/256
// E.g threshold of 50 w hysteresis 6 would yield thresholds of 56 and 44. 
#define SWITCH_HYSTERESIS_COUNTS 3

// Switch debounce counts in 1kHz ticks
#define DIGITAL_SWICH_DEBOUNCE_COUNT 5

// Minimum threshold to be set for Lekker switches
#define MINIMUM_THRESHOLD 6


#endif