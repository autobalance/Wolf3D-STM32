#include <systick.h>

static volatile uint32_t ticks_ms = 0;

void SysTick_Handler(void)
{
    ticks_ms++;
}

void systick_setup(void)
{
    NVIC_EnableIRQ(SysTick_IRQn);

    // ticks at 1000Hz (1ms/tick), see '19.1.1 SysTick calibration value register'
    SysTick_Config((SystemCoreClock / 1000000) * SysTick->CALIB);
}

// crude delay function, allows interrupts to be serviced while waiting
void delay_ms(uint32_t ms)
{
    if (ms == 0) return;

    volatile uint32_t start = ticks_ms;

    while ((ticks_ms - start) < ms) __WFI();
}

uint32_t get_ticks_ms(void)
{
    return ticks_ms;
}

