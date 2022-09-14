/*
    DAC driver (using DMA) to produce audio output. Used DAC channel 2 and DMA1_Stream1.
    See 'dac_audio.c' for other details.
*/

#ifndef __DAC_H__
#define __DAC_H__

#include <stm32h7xx.h>

#define DAC_BUF_LEN 256

void dac_start(void);
void dac_audio_setup(void);

#endif
