# Stepad Firmware


### Notes:
* 1kHz interrupt for when ADC completes all samples and DMA transfer is complete. This needs to be larger than 4 samples for future use. Use DMA to trigger IRQ.
* During 1kHz interrupt, send keyboard report for the analog keys that were pressed during that poll.
* Maybe also include the digital keys in the 1kHz loop since they should be pretty fast.

* If DSP is needed, maybe have one CPU dedicated to sampling the ADC and then doing the DSP. Probably dont do this since this doesnt scale well with number of switches.


- USB HID report generator needs to be at user level. 
- CPU1 user level needs as light as possible.
- CPU1 User level runs at 1kHz delta timed loop.

- CPU1 ADC samples ADCS at n times the USB polling rate where n is the number of samples for debouncing

- CPU2 handles LED stuff