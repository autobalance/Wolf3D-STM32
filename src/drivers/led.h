/*
    LED driver for the devebox STM32H743/750 boards, with LED in
    open-drain configuration on GPIOA, pin 1.
*/

#ifndef __LED_H__
#define __LED_H__

#include <stm32h7xx.h>

void led_setup(void);

void led_on(void);
void led_off(void);
void led_toggle(void);

#endif
