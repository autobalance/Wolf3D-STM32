#include <led.h>

void led_setup(void)
{
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;

    GPIOA->MODER &= ~GPIO_MODER_MODE1;
    GPIOA->MODER |= 0b01 << GPIO_MODER_MODE1_Pos;

    GPIOA->BSRR = GPIO_BSRR_BR1;
}

void led_on(void)
{
    GPIOA->BSRR = GPIO_BSRR_BR1;
}

void led_off(void)
{
    GPIOA->BSRR = GPIO_BSRR_BS1;
}

void led_toggle(void)
{
    GPIOA->ODR ^= GPIO_ODR_OD1;
}
