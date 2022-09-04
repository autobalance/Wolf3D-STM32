#include <stm32h7xx.h>

#include <led.h>
#include <rcc.h>
#include <usart.h>
#include <dac.h>
#include <ps2_kb.h>
#include <ltdc.h>
#include <sdcard.h>
#include <ff.h>

#include <systick.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

extern void wl_main(void);

FATFS fs;

void error_handler(void)
{
    for (;;);
}

/*uint32_t sp_min = 0x20020000;

void __cyg_profile_func_enter (void *this_fn, void *call_site)
{
    uint32_t sp_top = (uint32_t) __get_MSP();
    sp_min = sp_top < sp_min ? sp_top : sp_min;
}

void __cyg_profile_func_exit  (void *this_fn, void *call_site)
{
}

void dwt_cyccnt_setup(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}*/

int main(void)
{
    rcc_pwr_setup();

    // enable cache for significant performance improvements when using QSPI for code
    // and SD card for game loading
    SCB_EnableICache();
    SCB_EnableDCache();

    //dwt_cyccnt_setup();
    led_setup();
    systick_setup();
    usart1_setup();
    ps2_kb_setup();
    ltdc_setup();
    dac_audio_setup();
    sdmmc1_setup();

    if (f_mount(&fs, "", 1) != FR_OK)
    {
        printf("could not mount disk, quitting...\r\n");
        error_handler();
    }

    ltdc_draw_intro();
    ps2_kb_set_baud_rate();

    for (;;)
    {
        wl_main();
    }
}
