/*
    SysTick configuration and interrupt handler, plus delay function.
    Used primarily to generate delays for startup code and in the Wolf3D
    tick delay routines.
*/

#ifndef __SYSTICK_H__
#define __SYSTICK_H__

#include <stm32h7xx.h>

void systick_setup(void);

void delay_ms(uint32_t ms);
uint32_t get_ticks_ms(void);

#endif
