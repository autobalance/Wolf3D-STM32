#include <usart.h>

#define UART_BAUD_RATE 921600U

// used by syscalls.c to enable printf (e.g. a debug aid)
int __io_putchar(int ch)
{
    while (!(USART1->ISR & USART_ISR_TXE_TXFNF));
    USART1->TDR = (char) ch;

    return 0;
}

void usart1_setup(void)
{
    // select clock for peripheral
    RCC->D2CCIP2R |= 0b000 << RCC_D2CCIP2R_USART16SEL_Pos;

    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;

    GPIOA->MODER &= ~GPIO_MODER_MODE9;
    GPIOA->MODER |= 0b10 << GPIO_MODER_MODE9_Pos;
    GPIOA->OSPEEDR |= 0b10 << GPIO_OSPEEDR_OSPEED9_Pos;
    GPIOA->AFR[1] |= 7 << GPIO_AFRH_AFSEL9_Pos; // devebox has USART TXD on GPIOA, pin 9

    // peripheral runs in domain 2, divide clock according to source selected above
    USART1->BRR = (SystemD2Clock / 2) / UART_BAUD_RATE;
    USART1->CR1 |= USART_CR1_UE | USART_CR1_FIFOEN;
    USART1->CR1 |= USART_CR1_TE;
}
