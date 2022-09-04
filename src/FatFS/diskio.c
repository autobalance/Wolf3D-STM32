/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */


static LBA_t n_sectors = 0;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
    if (pdrv != 0)
    {
        return STA_NOINIT;
    }

	return (sd_num_blocks() == 0) ? STA_NOINIT : 0;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
    if (pdrv != 0)
    {
        return STA_NOINIT;
    }

    if (n_sectors == 0)
    {
        sd_init();

        n_sectors = sd_num_blocks();

        if (n_sectors == 0)
        {
            return STA_NOINIT;
        }
    }

	return 0;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
    if (pdrv != 0 ||
        !buff ||
        sector >= n_sectors ||
        (sector + count) > n_sectors)
    {
        return RES_PARERR;
    }

	if (sd_num_blocks() == 0)
    {
        return RES_NOTRDY;
    }

    if (sd_read_multi_block(buff, sector, count) != 0)
    {
        return RES_ERROR;
    }

    return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
    if (pdrv != 0 ||
        !buff ||
        sector >= n_sectors ||
        (sector + count) > n_sectors)
    {
        return RES_PARERR;
    }

	if (sd_num_blocks() == 0)
    {
        return RES_NOTRDY;
    }

    if (sd_write_multi_block(buff, sector, count) != 0)
    {
        return RES_ERROR;
    }

    return RES_OK;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
    if (pdrv != 0)
    {
        return RES_PARERR;
    }

    DRESULT res;
    switch (cmd)
    {
        case CTRL_SYNC:
            res = RES_OK;
            break;
        case GET_SECTOR_COUNT:
            *((LBA_t *) buff) = n_sectors;
            res = RES_OK;
            break;
        case GET_BLOCK_SIZE:
            *((DWORD *) buff) = 1;
            res = RES_OK;
            break;
        default:
            res = RES_PARERR;
            break;
    }

    return res;
}

