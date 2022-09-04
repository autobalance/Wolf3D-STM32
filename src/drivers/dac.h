/*
    DAC driver (using DMA) to produce audio output. Used DAC channel 2 and DMA1_Stream1.
    See 'dac_audio.c' for other details.
*/

#ifndef __DAC_H__
#define __DAC_H__

#include <stm32h7xx.h>

void dac_audio_setup(void);

#endif
