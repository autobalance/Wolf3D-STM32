/*
    SD card driver using the SDMMC peripheral. Derived from the state machine
    outlines provided in the STM32H7xx reference manual, along with help from
    the following:

        http://elm-chan.org/docs/mmc/mmc_e.html
        https://openlabpro.com/guide/interfacing-microcontrollers-with-sd-card/
        https://yannik520.github.io/sdio.html

    and the official SD spec at:

        https://www.sdcard.org/downloads/pls/

    Not a complete implementation, and assumes the use of high capacity SD cards;
    initialization won't work otherwise.
*/

#ifndef __SDCARD_H__
#define __SDCARD_H__

#include <stm32h7xx.h>
#include <systick.h>
#include <stdio.h>

#define SD_BLOCK_LEN 512

void sdmmc1_setup(void);

uint32_t sd_cmd(uint8_t index, uint32_t arg, int acmd);

void sd_init(void);

uint32_t sd_block_len(void);
uint64_t sd_num_blocks(void);

uint32_t sd_read_single_block(uint8_t buf[512], uint32_t sector);
uint32_t sd_read_multi_block(uint8_t *buf, uint32_t sector, uint32_t n_sectors);

uint32_t sd_write_single_block(const uint8_t buf[512], uint32_t sector);
uint32_t sd_write_multi_block(const uint8_t *buf, uint32_t sector, uint32_t n_sectors);

#endif
