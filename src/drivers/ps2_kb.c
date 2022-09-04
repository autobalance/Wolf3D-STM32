#include <ps2_kb.h>

#include <systick.h>
#include <stdio.h>

// Handle Wolf3D key service in the interrupt here.
// See 'id_in.c' for details (adapted from ID's original source for handling keyboard presses).
extern void INL_KeyService(int data);

void USART2_IRQHandler(void)
{
    INL_KeyService(USART2->RDR & 0xff);
}

// my PS2-compatible keyboard has a fairly stable clock of about 11900Hz,
// so use that as an initial guess for the actual baud rate
#define PS2_KB_BAUD_GUESS 11900U

void usart2_setup(void)
{
    RCC->D2CCIP2R |= 0b000 << RCC_D2CCIP2R_USART28SEL_Pos;
    RCC->APB1LENR |= RCC_APB1LENR_USART2EN;
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIODEN;

    GPIOD->MODER &= ~GPIO_MODER_MODE6;
    GPIOD->MODER |= 0b10 << GPIO_MODER_MODE6_Pos;
    GPIOD->OSPEEDR |= 0b10 << GPIO_OSPEEDR_OSPEED6_Pos;
    GPIOD->AFR[0] |= 7 << GPIO_AFRL_AFSEL6_Pos;

    NVIC_EnableIRQ(USART2_IRQn);

    USART2->BRR = (SystemD2Clock / 2) / PS2_KB_BAUD_GUESS;
    USART2->CR2 = 0b01 << USART_CR2_ABRMODE_Pos;
    USART2->CR1 = USART_CR1_M0 | USART_CR1_PCE | USART_CR1_PS;
    USART2->CR1 |= USART_CR1_UE | USART_CR1_FIFOEN | USART_CR1_RXNEIE_RXFNEIE;
    USART2->CR1 |= USART_CR1_RE;
}

void ps2_kb_setup(void)
{
    usart2_setup();
}

void ps2_kb_set_baud_rate(void)
{
    int abr_success = 0;
    do
    {
        delay_ms(100);

        USART2->RQR = USART_RQR_ABRRQ | USART_RQR_RXFRQ;
        USART2->CR2 &= ~USART_CR2_ABREN;

        USART2->CR2 |= USART_CR2_ABREN;
        printf("Press space key\r\n");

        while (!(USART2->ISR & USART_ISR_ABRF));

        if (USART2->ISR & USART_ISR_ABRE)
        {
            continue;
        }

        abr_success = (USART2->RDR & 0xff) == 0x29;
    }
    while (!abr_success);

    printf("PS2 KB baud guess: %lu\r\n", (SystemD2Clock / 2) / USART2->BRR);
}

