#include <ltdc.h>

uint8_t framebuf[FRAMEBUF_HEIGHT*2][FRAMEBUF_WIDTH];

/*
    RGB565 output:

    VSYNC: PA4, AF14
    HSYNC: PC6, AF14

    R7: PE15, AF14
    R6: PA8,  AF14
    R5: PA12, AF14
    R4: PA11, AF14
    R3: PB0,  AF9

    G7: PD3,  AF14
    G6: PC7,  AF14
    G5: PB11, AF14
    G4: PB10, AF14
    G3: PE11, AF14
    G2: PA6,  AF14

    B7: PB9,  AF14
    B6: PB8,  AF14
    B5: PA3,  AF14
    B4: PE12, AF14
    B3: PD10, AF14

    AF9 : PB{0}
    AF14: PA{3,4,6,8,11,12}, PB{8,9,10,11}, PC{6,7}, PD{3,10}, PE{11,12,15}
*/
void setup_ltdc_gpio(void)
{
    RCC->AHB4ENR |= 0x1F; // GPIO{A,B,C,D,E} clock enable


    // GPIOA pin configurations
    GPIOA->MODER &= ~(GPIO_MODER_MODE3  | GPIO_MODER_MODE4 | GPIO_MODER_MODE6 | GPIO_MODER_MODE8 |
                      GPIO_MODER_MODE11 | GPIO_MODER_MODE12);
    GPIOA->MODER |= (0b10 << GPIO_MODER_MODE3_Pos)  | (0b10 << GPIO_MODER_MODE4_Pos) |
                    (0b10 << GPIO_MODER_MODE6_Pos)  | (0b10 << GPIO_MODER_MODE8_Pos) |
                    (0b10 << GPIO_MODER_MODE11_Pos) | (0b10 << GPIO_MODER_MODE12_Pos);

    GPIOA->OSPEEDR |= (0b11 << GPIO_OSPEEDR_OSPEED3_Pos)  | (0b11 << GPIO_OSPEEDR_OSPEED4_Pos) |
                      (0b11 << GPIO_OSPEEDR_OSPEED6_Pos)  | (0b11 << GPIO_OSPEEDR_OSPEED8_Pos) |
                      (0b11 << GPIO_OSPEEDR_OSPEED11_Pos) | (0b11 << GPIO_OSPEEDR_OSPEED12_Pos);

    GPIOA->AFR[0] |= (14 << GPIO_AFRL_AFSEL3_Pos) | (14 << GPIO_AFRL_AFSEL4_Pos) |
                     (14 << GPIO_AFRL_AFSEL6_Pos);
    GPIOA->AFR[1] |= (14 << GPIO_AFRH_AFSEL8_Pos) | (14 << GPIO_AFRH_AFSEL11_Pos) |
                     (14 << GPIO_AFRH_AFSEL12_Pos);


    // GPIOB pin configurations
    GPIOB->MODER &= ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE8 | GPIO_MODER_MODE9 | GPIO_MODER_MODE10 |
                      GPIO_MODER_MODE11);
    GPIOB->MODER |= (0b10 << GPIO_MODER_MODE0_Pos) | (0b10 << GPIO_MODER_MODE8_Pos) |
                    (0b10 << GPIO_MODER_MODE9_Pos) | (0b10 << GPIO_MODER_MODE10_Pos) |
                    (0b10 << GPIO_MODER_MODE11_Pos);

    GPIOB->OSPEEDR |= (0b11 << GPIO_OSPEEDR_OSPEED0_Pos) | (0b11 << GPIO_OSPEEDR_OSPEED8_Pos) |
                      (0b11 << GPIO_OSPEEDR_OSPEED9_Pos) | (0b11 << GPIO_OSPEEDR_OSPEED10_Pos) |
                      (0b11 << GPIO_OSPEEDR_OSPEED11_Pos);

    GPIOB->AFR[0] |= (9 << GPIO_AFRL_AFSEL0_Pos);
    GPIOB->AFR[1] |= (14 << GPIO_AFRH_AFSEL8_Pos)  | (14 << GPIO_AFRH_AFSEL9_Pos) |
                     (14 << GPIO_AFRH_AFSEL10_Pos) | (14 << GPIO_AFRH_AFSEL11_Pos);


    // GPIOC pin configurations
    GPIOC->MODER &= ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE7);
    GPIOC->MODER |= (0b10 << GPIO_MODER_MODE6_Pos) | (0b10 << GPIO_MODER_MODE7_Pos);

    GPIOC->OSPEEDR |= (0b11 << GPIO_OSPEEDR_OSPEED6_Pos) | (0b11 << GPIO_OSPEEDR_OSPEED7_Pos);

    GPIOC->AFR[0] |= (14 << GPIO_AFRL_AFSEL6_Pos) | (14 << GPIO_AFRL_AFSEL7_Pos);


    // GPIOD pin configurations
    GPIOD->MODER &= ~(GPIO_MODER_MODE3 | GPIO_MODER_MODE10);
    GPIOD->MODER |= (0b10 << GPIO_MODER_MODE3_Pos) | (0b10 << GPIO_MODER_MODE10_Pos);

    GPIOD->OSPEEDR |= (0b11 << GPIO_OSPEEDR_OSPEED3_Pos) | (0b11 << GPIO_OSPEEDR_OSPEED10_Pos);

    GPIOD->AFR[0] |= (14 << GPIO_AFRL_AFSEL3_Pos);
    GPIOD->AFR[1] |= (14 << GPIO_AFRH_AFSEL10_Pos);


    // GPIOE pin configurations
    GPIOE->MODER &= ~(GPIO_MODER_MODE11 | GPIO_MODER_MODE12 | GPIO_MODER_MODE15);
    GPIOE->MODER |= (0b10 << GPIO_MODER_MODE11_Pos) | (0b10 << GPIO_MODER_MODE12_Pos) |
                    (0b10 << GPIO_MODER_MODE15_Pos);

    GPIOE->OSPEEDR |= (0b11 << GPIO_OSPEEDR_OSPEED11_Pos) | (0b11 << GPIO_OSPEEDR_OSPEED12_Pos) |
                      (0b11 << GPIO_OSPEEDR_OSPEED15_Pos);

    GPIOE->AFR[1] |= (14 << GPIO_AFRH_AFSEL11_Pos) | (14 << GPIO_AFRH_AFSEL12_Pos) |
                     (14 << GPIO_AFRH_AFSEL15_Pos);
}

// follows the LTDC programming procedure (see section 32.6 in the reference manual, RM0433)
void ltdc_setup(void)
{
    RCC->APB3ENR |= RCC_APB3ENR_LTDCEN;

    setup_ltdc_gpio();

    LTDC->SSCR = ((SS_HORI - 1) << LTDC_SSCR_HSW_Pos)    | ((SS_VERT - 1) << LTDC_SSCR_VSH_Pos);
    LTDC->BPCR = ((BP_HORI - 1) << LTDC_BPCR_AHBP_Pos)   | ((BP_VERT - 1) << LTDC_BPCR_AVBP_Pos);
    LTDC->AWCR = ((AW_HORI - 1) << LTDC_AWCR_AAW_Pos)    | ((AW_VERT - 1) << LTDC_AWCR_AAH_Pos);
    LTDC->TWCR = ((TW_HORI - 1) << LTDC_TWCR_TOTALW_Pos) | ((TW_VERT - 1) << LTDC_TWCR_TOTALH_Pos);

    // active low horizontal sync and active high vertical sync
    LTDC->GCR = (0 << LTDC_GCR_HSPOL_Pos) | (1 << LTDC_GCR_VSPOL_Pos);

    LTDC_Layer1->WHPCR = ((WHP_START) << LTDC_LxWHPCR_WHSTPOS_Pos) | ((WHP_STOP - 1) << LTDC_LxWHPCR_WHSPPOS_Pos);
    LTDC_Layer1->WVPCR = ((WVP_START) << LTDC_LxWVPCR_WVSTPOS_Pos) | ((WVP_STOP - 1) << LTDC_LxWVPCR_WVSPPOS_Pos);

    // 8-bit luminance pixel format (to be used with a custom LUT corresponding to Wolf3D 256 colour palette)
    LTDC_Layer1->PFCR = 0b101 << LTDC_LxPFCR_PF_Pos;

    LTDC_Layer1->CFBAR = (unsigned int) framebuf;

    // each pixel in the screen buffer will be 8-bits corresponding to table lookup, so set pitch accordingly
    LTDC_Layer1->CFBLR = ((FRAMEBUF_WIDTH) << LTDC_LxCFBLR_CFBP_Pos) |
                         ((FRAMEBUF_WIDTH + 7) << LTDC_LxCFBLR_CFBLL_Pos);
    LTDC_Layer1->CFBLNR = (FRAMEBUF_HEIGHT*2) << LTDC_LxCFBLNR_CFBLNBR_Pos;

    LTDC_Layer1->CR = LTDC_LxCR_CLUTEN | LTDC_LxCR_LEN;

    LTDC->SRCR = LTDC_SRCR_IMR;
    while (LTDC->SRCR & LTDC_SRCR_IMR);

    LTDC->GCR |= LTDC_GCR_LTDCEN;
}

void ltdc_wait_for_vsync(void)
{
    while (!(LTDC->CDSR & LTDC_CDSR_VSYNCS));
}

void ltdc_set_clut(uint8_t *rgba_s)
{
    for (int col = 0; col < 256; col++)
    {
        uint32_t fullcol = (uint32_t) rgba_s[0] << 16 |
                           (uint32_t) rgba_s[1] << 8  |
                           (uint32_t) rgba_s[2] << 0;

        LTDC_Layer1->CLUTWR = (col << 24) | fullcol;

        rgba_s += 4;
    }
    LTDC->SRCR = LTDC_SRCR_IMR;
}

void ltdc_draw_intro(void)
{
    uint8_t down_arrow_ascii[64] = "   OO   "
                                   "   OO   "
                                   "   OO   "
                                   "   OO   "
                                   "OOOOOOOO"
                                   " OOOOOO "
                                   "  OOOO  "
                                   "   OO   ";

    for (int row = 96; row < 104; row++)
    for (int col = 156; col < 164; col++)
    {
        framebuf[row][col] = down_arrow_ascii[(row - 96) * 8 + (col - 156)] == 'O' ? 0xff : 0;
    }
    for (int col = 100; col < 220; col++)
    {
        framebuf[108][col] = 0xff;
        framebuf[109][col] = 0xff;
        framebuf[120][col] = 0xff;
        framebuf[121][col] = 0xff;
    }
    for (int row = 110; row < 120; row++)
    {
        framebuf[row][100] = 0xff;
        framebuf[row][101] = 0xff;
        framebuf[row][218] = 0xff;
        framebuf[row][219] = 0xff;
    }
    SCB_CleanDCache_by_Addr((uint32_t*)(((uint32_t)framebuf) & ~(uint32_t)0x1F), 320*200 + 32);
}
