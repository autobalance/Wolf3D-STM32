/*
    System clock configuration functions. Default is to enable all SRAM
    and put the device into VOS0 state to achieve a system clock of 480MHz.
    Note that the bottleneck in Wolf3D is not the CPU speed; same frame-rate
    can be achieved most of the time at as low as 120MHz. The SD card interface
    is the biggest bottlneck, so configure 'id_pm.h' to store more pages in memory
    for better performance.
*/

#ifndef __RCC_H__
#define __RCC_H__

#include <stm32h7xx.h>

static inline void cfg_pwr(void)
{
    PWR->CR3 = (PWR_CR3_SCUEN | PWR_CR3_LDOEN) & ~PWR_CR3_BYPASS;
    while (!(PWR->CSR1 & PWR_CSR1_ACTVOSRDY));

    PWR->D3CR = 0b11 << PWR_D3CR_VOS_Pos;
    while (!(PWR->D3CR & PWR_D3CR_VOSRDY));

    RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;
    SYSCFG->PWRCR |= SYSCFG_PWRCR_ODEN;
    while (!(PWR->D3CR & PWR_D3CR_VOSRDY));
}

static inline void cfg_flash_acr(void)
{
    FLASH->ACR = (0b10 << FLASH_ACR_WRHIGHFREQ_Pos) |
                 (0b0100 << FLASH_ACR_LATENCY_Pos);

    while (!(FLASH->ACR & 0b00100100));
}

static inline void cfg_osc(void)
{
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));
}

// setup main system clock via PLL1_P1 at ((25MHz / DIVM) * DIVN) / DIVR = 480MHz
static inline void cfg_pll1(void)
{
    RCC->PLL1DIVR = 0;

    RCC->PLLCKSELR |= 5 << RCC_PLLCKSELR_DIVM1_Pos;

    RCC->PLLCFGR |= RCC_PLLCFGR_DIVP1EN;
    RCC->PLLCFGR |= 0b10 << RCC_PLLCFGR_PLL1RGE_Pos;

    RCC->PLL1DIVR |= (96 - 1) << RCC_PLL1DIVR_N1_Pos;
    RCC->PLL1DIVR |= (1 - 1) << RCC_PLL1DIVR_P1_Pos;

    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY));
}

// setup SDMMC kernel clock as PLL2_R2 at ((25MHz / DIVM) * DIVN) / DIVR = 200MHz
static inline void cfg_pll2(void)
{
    RCC->PLL2DIVR = 0;

    RCC->PLLCKSELR |= 5 << RCC_PLLCKSELR_DIVM2_Pos;

    RCC->PLLCFGR |= RCC_PLLCFGR_DIVR2EN;
    RCC->PLLCFGR |= 0b10 << RCC_PLLCFGR_PLL2RGE_Pos;

    RCC->PLL2DIVR |= (80 - 1) << RCC_PLL2DIVR_N2_Pos;
    RCC->PLL2DIVR |= (2 - 1) << RCC_PLL2DIVR_R2_Pos;

    RCC->CR |= RCC_CR_PLL2ON;
    while (!(RCC->CR & RCC_CR_PLL2RDY));
}

// setup pixel clock (PLL3_R3 should output e.g. 25.175MHz for 720x400@70Hz VGA)
// NOTE: current DIVN and FRACN clocks are for non-VGA 320x240@60Hz (25.175MHz / 4)
static inline void cfg_pll3(void)
{
    RCC->PLL3DIVR = 0;

    // PLL clock source divided by 5, e.g. 25MHz/5 = 5MHz ref. clock
    RCC->PLLCKSELR |= 5 << RCC_PLLCKSELR_DIVM3_Pos;

    // PLL3_R3 fractional part -> 2867 / (2^13) ~= 0.349975586
    RCC->PLL3FRACR = 4813 << RCC_PLL3FRACR_FRACN3_Pos;

    RCC->PLLCFGR |= RCC_PLLCFGR_PLL3FRACEN;
    RCC->PLLCFGR |= RCC_PLLCFGR_DIVR3EN;
    RCC->PLLCFGR |= 0b10 << RCC_PLLCFGR_PLL3RGE_Pos;

    // PLL3_R3 DIVN3 -> F_VCO = F_ref * (DIVN3 + FRACN3/(2^13)) ~= 5MHz * (50 + 0.349975586) ~= 251.74987793MHz
    RCC->PLL3DIVR |= (12 - 1) << RCC_PLL3DIVR_N3_Pos;

    // PLL3_R3 output clock -> F_PLLR3 = F_VCO / DIVR3 ~= 251.74987793MHz / 10 ~= 25.174987793MHz
    RCC->PLL3DIVR |= (10 - 1) << RCC_PLL3DIVR_R3_Pos;

    RCC->CR |= RCC_CR_PLL3ON;
    while (!(RCC->CR & RCC_CR_PLL3RDY));
}

static inline void cfg_plls(void)
{
    RCC->PLLCKSELR = 0;
    RCC->PLLCFGR = 0;

    RCC->PLLCKSELR |= 0b10 << RCC_PLLCKSELR_PLLSRC_Pos;

    cfg_pll1();
    cfg_pll2();
    cfg_pll3();
}

static inline void cfg_bus_clks(void)
{
    RCC->D1CFGR = (0b1000 << RCC_D1CFGR_HPRE_Pos) |
                  (0b100 << RCC_D1CFGR_D1PPRE_Pos);
    RCC->D2CFGR = (0b100 << RCC_D2CFGR_D2PPRE1_Pos) |
                  (0b100 << RCC_D2CFGR_D2PPRE2_Pos);
    RCC->D3CFGR =  0b100 << RCC_D3CFGR_D3PPRE_Pos;

    RCC->CFGR |= RCC_CFGR_SW_PLL1;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL1);
}

static inline void cfg_d2sram(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_SRAM1EN;
    RCC->AHB2ENR |= RCC_AHB2ENR_SRAM2EN;
    RCC->AHB2ENR |= RCC_AHB2ENR_SRAM3EN;

    while (!(RCC->AHB2ENR & (RCC_AHB2ENR_SRAM1EN | RCC_AHB2ENR_SRAM2EN | RCC_AHB2ENR_SRAM3EN)));

    while (!(RCC->CR & RCC_CR_D2CKRDY));

    do
    {
        PWR->CPUCR &= ~PWR_CPUCR_SBF_D2;
    }
    while (PWR->CPUCR & PWR_CPUCR_SBF_D2);

    // access to domain 2 requires initial access to the SRAM
    *((volatile uint32_t *) D2_AHBSRAM_BASE);
}

static inline void rcc_pwr_setup(void)
{
    cfg_pwr();
    cfg_flash_acr();
    cfg_osc();
    cfg_plls();
    cfg_bus_clks();
    cfg_d2sram();

    SystemCoreClockUpdate();
}

#endif
