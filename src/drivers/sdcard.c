#include <sdcard.h>
#include <led.h>

static uint32_t rca = 0;
static uint64_t num_blocks = 0;

void sdmmc1_setup(void)
{
    RCC->D1CCIPR |= 0b1 << RCC_D1CCIPR_SDMMCSEL_Pos;

    RCC->AHB3RSTR |= RCC_AHB3RSTR_SDMMC1RST;
    delay_ms(2);
    RCC->AHB3RSTR &= ~RCC_AHB3RSTR_SDMMC1RST;

    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN;

    GPIOC->MODER &= ~(GPIO_MODER_MODE8 | GPIO_MODER_MODE9 | GPIO_MODER_MODE10 | GPIO_MODER_MODE11);
    GPIOC->MODER &= ~GPIO_MODER_MODE12;
    GPIOD->MODER &= ~GPIO_MODER_MODE2;

    GPIOC->MODER |= 0b10 << GPIO_MODER_MODE8_Pos | 0b10 << GPIO_MODER_MODE9_Pos |
                    0b10 << GPIO_MODER_MODE10_Pos | 0b10 << GPIO_MODER_MODE11_Pos;
    GPIOC->MODER |= 0b10 << GPIO_MODER_MODE12_Pos;
    GPIOD->MODER |= 0b10 << GPIO_MODER_MODE2_Pos;

    GPIOC->OSPEEDR |= 0b11 << GPIO_OSPEEDR_OSPEED8_Pos | 0b11 << GPIO_OSPEEDR_OSPEED9_Pos |
                      0b11 << GPIO_OSPEEDR_OSPEED10_Pos | 0b11 << GPIO_OSPEEDR_OSPEED11_Pos;
    GPIOC->OSPEEDR |= 0b11 << GPIO_OSPEEDR_OSPEED12_Pos;
    GPIOD->OSPEEDR |= 0b11 << GPIO_OSPEEDR_OSPEED2_Pos;

    GPIOC->AFR[1] |= 0b1100 << GPIO_AFRH_AFSEL8_Pos | 0b1100 << GPIO_AFRH_AFSEL9_Pos |
                     0b1100 << GPIO_AFRH_AFSEL10_Pos | 0b1100 << GPIO_AFRH_AFSEL11_Pos;
    GPIOC->AFR[1] |= 0b1100 << GPIO_AFRH_AFSEL12_Pos;
    GPIOD->AFR[0] |= 0b1100 << GPIO_AFRL_AFSEL2_Pos;

    RCC->AHB3ENR |= RCC_AHB3ENR_SDMMC1EN;

    SDMMC1->POWER = 0b10 << SDMMC_POWER_PWRCTRL_Pos;
    delay_ms(10);
    SDMMC1->POWER = 0b00 << SDMMC_POWER_PWRCTRL_Pos;
    delay_ms(10);

    SDMMC1->CLKCR = ((512/2) << SDMMC_CLKCR_CLKDIV_Pos) | SDMMC_CLKCR_HWFC_EN | SDMMC_CLKCR_NEGEDGE;

    SDMMC1->POWER = 0b11 << SDMMC_POWER_PWRCTRL_Pos;
    delay_ms(10);
}

uint32_t sd_cmd6(uint32_t arg, uint32_t response_type, uint32_t ack_type)
{
    SDMMC1->DLEN = 64;
    SDMMC1->DTIMER = 0xffffff;
    SDMMC1->DCTRL = (0b0110 << SDMMC_DCTRL_DBLOCKSIZE_Pos) | SDMMC_DCTRL_DTDIR;

    SDMMC1->ARG = arg;
    SDMMC1->CMD = response_type | (6 << SDMMC_CMD_CMDINDEX_Pos) | SDMMC_CMD_CMDTRANS | SDMMC_CMD_CPSMEN;

    while (!(SDMMC1->STA & (ack_type | SDMMC_STA_CTIMEOUT)));

    uint32_t success = SDMMC1->STA & SDMMC_STA_CTIMEOUT ? SDMMC1->STA : 0;
    if (success != 0)
    {
        return success;
    }

    while (SDMMC1->DCOUNT > 0)
    {
        if (!(SDMMC1->STA & SDMMC_STA_RXFIFOE))
        {
            SDMMC1->FIFO;
        }
    }

    success = SDMMC1->STA & (SDMMC_STA_DTIMEOUT | SDMMC_STA_DCRCFAIL) ? SDMMC1->STA : 0;

    SDMMC1->ICR = 0xfff;

    return success;
}

uint32_t sd_cmd(uint8_t index, uint32_t arg, int acmd)
{
    volatile uint32_t response_type, ack_type;
    switch (index)
    {
        case 0:
            response_type = 0;
            ack_type = SDMMC_STA_CMDSENT;
            break;
        case 2:
        case 9:
            response_type = 0b11 << SDMMC_CMD_WAITRESP_Pos;
            ack_type = SDMMC_STA_CMDREND;
            break;
        default:
            response_type = 0b01 << SDMMC_CMD_WAITRESP_Pos;
            ack_type = SDMMC_STA_CMDREND;
            break;
    }

    if (index == 6 && acmd == 0)
    {
        return sd_cmd6(arg, response_type, ack_type);
    }

    if (acmd)
    {
        sd_cmd(55, rca, 0);
        if (index == 41)
            ack_type = SDMMC_STA_CCRCFAIL;
    }

    SDMMC1->ARG = arg;
    SDMMC1->CMD = response_type | (index << SDMMC_CMD_CMDINDEX_Pos) | SDMMC_CMD_CPSMEN;

    while (!(SDMMC1->STA & (ack_type | SDMMC_STA_CTIMEOUT)));

    uint32_t success = SDMMC1->STA & SDMMC_STA_CTIMEOUT ? SDMMC1->STA : 0;

    SDMMC1->ICR = 0xfff;

    return success;
}

void sd_init(void)
{
    sd_cmd(0, 0, 0);
    sd_cmd(8, 0x1aa, 0); // to support high capacity

    // TODO: This block is needed at start since the SD card sometimes presents as uninitialized,
    //       i.e. dirty power-on state? Look into properly resetting an SD card to start again after
    //       something like a soft reset.
    while (SDMMC1->STA)
    {
        sdmmc1_setup();

        sd_cmd(0, 0, 0);
        sd_cmd(8, 0x1aa, 0);
    }

    do
    {
        sd_cmd(41, 0x40100000, 1);
    }
    while ((SDMMC1->RESP1 >> 31) != 1);

    sd_cmd(2, 0, 0); // card send CID
    sd_cmd(3, 0, 0); // card send RCA

    rca = SDMMC1->RESP1 & 0xffff0000; // used to address the card from here on

    sd_cmd(9, rca, 0); // to determine the SD cards size
    uint64_t c_size = ((SDMMC1->RESP2 & 0x3f) << 16) | (SDMMC1->RESP3 >> 16);
    num_blocks = (((c_size + 1) * 512) * 1024) / SD_BLOCK_LEN;

    sd_cmd(7, rca, 0); // select the card
    sd_cmd(6, 0x02, 1); // ACMD6, tell the card to start using 4-bit SDIO mode

    // set the SDIO bus width, hardware flow control and change command/data on same falling edge
    SDMMC1->CLKCR = (0b01 << SDMMC_CLKCR_WIDBUS_Pos) | SDMMC_CLKCR_HWFC_EN | SDMMC_CLKCR_NEGEDGE;

    sd_cmd(6, 0x80fffff1, 0); // switch card to high speed/SDR25 rates, keeping all other settings

    // sdmmc_ker_ck / 4 = 200MHz/4 = 50MHz SD bus clock, and enable power saving when bus is unused
    SDMMC1->CLKCR |= ((4/2) << SDMMC_CLKCR_CLKDIV_Pos) | SDMMC_CLKCR_PWRSAV;
}

uint32_t sd_block_len(void)
{
    return SD_BLOCK_LEN;
}

uint64_t sd_num_blocks(void)
{
    return num_blocks;
}

uint32_t sd_read_single_block(uint8_t buf[512], uint32_t sector)
{
    if (sd_cmd(17, sector, 0) != 0) return (1 << 31);

    SDMMC1->DLEN = 512;
    SDMMC1->DTIMER = 0xffffffff;
    SDMMC1->DCTRL = (0b1001 << SDMMC_DCTRL_DBLOCKSIZE_Pos) | SDMMC_DCTRL_DTDIR | SDMMC_DCTRL_DTEN;

    uint32_t *buf_u32 = (uint32_t *) buf;
    uint32_t i = 0;
    while (SDMMC1->DCOUNT > 0)
    {
        if (!(SDMMC1->STA & SDMMC_STA_RXFIFOE) && i < 512 / 4)
        {
            buf_u32[i++] = SDMMC1->FIFO;
        }
    }

    if (SDMMC1->STA & (SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT)) return SDMMC1->STA;

    while (SDMMC1->STA & SDMMC_STA_DPSMACT);
    SDMMC1->ICR = SDMMC_ICR_DATAENDC;

    return 0;
}

uint32_t sd_read_multi_block(uint8_t *buf, uint32_t sector, uint32_t n_sectors)
{
    if (sd_cmd(23, n_sectors, 0) != 0) return (1 << 31);
    if (sd_cmd(18, sector, 0) != 0) return (1 << 31);

    SDMMC1->DLEN = 512*n_sectors;
    SDMMC1->DTIMER = 0xffffffff;
    SDMMC1->DCTRL = (0b1001 << SDMMC_DCTRL_DBLOCKSIZE_Pos) | SDMMC_DCTRL_DTDIR | SDMMC_DCTRL_DTEN;

    led_on(); // indicate read-activity on the SD card bus

    uint32_t *buf_u32 = (uint32_t *) buf;
    uint32_t i = 0;
    while (SDMMC1->DCOUNT > 0)
    {
        if (!(SDMMC1->STA & SDMMC_STA_RXFIFOE) && i < (512*n_sectors) / sizeof(uint32_t))
        {
            buf_u32[i++] = SDMMC1->FIFO;
        }
    }

    if (SDMMC1->STA & (SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT)) return SDMMC1->STA;

    while (SDMMC1->STA & SDMMC_STA_DPSMACT);
    SDMMC1->ICR = SDMMC_ICR_DATAENDC;

    led_off();

    return 0;
}

uint32_t sd_write_single_block(const uint8_t buf[512], uint32_t sector)
{
    if (sd_cmd(24, sector, 0) != 0) return (1 << 31);

    SDMMC1->DLEN = 512;
    SDMMC1->DTIMER = 0xffffffff;
    SDMMC1->DCTRL = (0b1001 << SDMMC_DCTRL_DBLOCKSIZE_Pos) | SDMMC_DCTRL_DTEN;

    uint32_t *buf_u32 = (uint32_t *) buf;
    uint32_t i = 0;
    while (SDMMC1->DCOUNT > 0)
    {
        if (!(SDMMC1->STA & SDMMC_STA_TXFIFOF) && i < 512 / 4)
        {
            SDMMC1->FIFO = buf_u32[i++];
        }
    }

    if (SDMMC1->STA & (SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT)) return SDMMC1->STA;

    while (SDMMC1->STA & SDMMC_STA_DPSMACT);
    SDMMC1->ICR = SDMMC_ICR_DATAENDC;

    return 0;
}

uint32_t sd_write_multi_block(const uint8_t *buf, uint32_t sector, uint32_t n_sectors)
{
    if (sd_cmd(23, n_sectors, 0) != 0) return (1 << 31);
    if (sd_cmd(25, sector, 0) != 0) return (1 << 31);

    SDMMC1->DLEN = 512*n_sectors;
    SDMMC1->DTIMER = 0xffffffff;
    SDMMC1->DCTRL = (0b1001 << SDMMC_DCTRL_DBLOCKSIZE_Pos) | SDMMC_DCTRL_DTEN;

    uint32_t *buf_u32 = (uint32_t *) buf;
    uint32_t i = 0;
    while (SDMMC1->DCOUNT > 0)
    {
        if (!(SDMMC1->STA & SDMMC_STA_TXFIFOF) && i < (512*n_sectors) / sizeof(uint32_t))
        {
            SDMMC1->FIFO = buf_u32[i++];
        }
    }

    if (SDMMC1->STA & (SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT)) return SDMMC1->STA;

    while (SDMMC1->STA & SDMMC_STA_DPSMACT);
    SDMMC1->ICR = SDMMC_ICR_DATAENDC;

    return 0;
}
