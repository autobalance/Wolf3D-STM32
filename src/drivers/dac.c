#include <dac.h>

// double buffered audio for the circular DMA
#define SNDBUFLEN 256
uint8_t sndbuf[2][SNDBUFLEN];

// use TIMER6 to trigger a DMA transfer to the DAC
void dac_timer6_setup(void)
{
    RCC->APB1LENR |= RCC_APB1LENR_TIM6EN;

    // Wolf3D sound sample rate is 7042Hz (see wolf3d src), so set prescaler accordingly.
    // Multiply sample rate by 10, since prescaler can only fit 16bits.
    // (won't be played at precisely 7042Hz due to uneven divisor)
    TIM6->PSC = SystemD2Clock / 70420 - 1;
    TIM6->ARR = 10 - 1;
    TIM6->CR2 = 0b010 << TIM_CR2_MMS_Pos;
    TIM6->CR1 = TIM_CR1_ARPE | TIM_CR1_URS;
    TIM6->CR1 |= TIM_CR1_CEN;
}

void dac_dma1s1_setup(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

    // request 68 corresponds to dac_ch2_dma
    DMAMUX1_Channel1->CCR |= 68 << DMAMUX_CxCR_DMAREQ_ID_Pos;

    // circular buffer for DMA, with 8 bit sampling
    DMA1_Stream1->M0AR = (unsigned int) sndbuf[0];
    DMA1_Stream1->M1AR = (unsigned int) sndbuf[1];
    DMA1_Stream1->PAR = (unsigned int) &(DAC1->DHR8R2);
    DMA1_Stream1->NDTR = SNDBUFLEN;
    DMA1_Stream1->CR = (0b11 << DMA_SxCR_PL_Pos) | DMA_SxCR_DBM;
    DMA1_Stream1->CR |= (0b00 << DMA_SxCR_MSIZE_Pos) | (0b00 << DMA_SxCR_PSIZE_Pos);
    DMA1_Stream1->CR |= DMA_SxCR_MINC | (0b01 << DMA_SxCR_DIR_Pos) | DMA_SxCR_CIRC | DMA_SxCR_TCIE;

    NVIC_EnableIRQ(DMA1_Stream1_IRQn);
    DMA1_Stream1->CR |= DMA_SxCR_EN;
}

void dac_audio_setup(void)
{
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
    RCC->APB1LENR |= RCC_APB1LENR_DAC12EN;

    GPIOA->MODER &= ~GPIO_MODER_MODE5;
    GPIOA->MODER |= 0b11 << GPIO_MODER_MODE5_Pos;

    // use dac_ch2_trg5 for channel 2 trigger, corresponding to tim6_trgo
    DAC1->MCR &= ~DAC_MCR_MODE2;
    DAC1->CR |= DAC_CR_TEN2 | (0b0101 << DAC_CR_TSEL2_Pos) | DAC_CR_DMAEN2;
    DAC1->CR |= DAC_CR_EN2;

    dac_dma1s1_setup();

    dac_timer6_setup();
}
