/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for Petit FatFs (C)ChaN, 2009      */
/* 3DS Version by Kane49												 */
/*-----------------------------------------------------------------------*/

#include "diskio.h"
#include "../minlib.h"

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize (void)
{
	DSTATUS stat;
	sdmmc_controller_init();
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Read Partial Sector                                                   */
/*-----------------------------------------------------------------------*/

static unsigned char sect[512];

DRESULT disk_readp (
	BYTE* dest,			/* Pointer to the destination object */
	DWORD sector,		/* Sector number (LBA) */
	WORD sofs,			/* Offset in the sector */
	WORD count			/* Byte count (bit15:destination) */
)
{
	DRESULT res;

	int i,k;
	
	if (sdmmc_sdcard_readsectors(sector,1,(void*)&sect)) 
		return RES_ERROR;

    k = 0;
    for (i = sofs; i < sofs+count; i++){
    	dest[k] = sect[i];
    	k++;
    }

	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Partial Sector                                                  */
/*-----------------------------------------------------------------------*/

static DWORD current_write_sector;
static WORD bytes_written;
static BYTE write_buffer[512];

DRESULT disk_writep (
	const BYTE* buff,		/* Pointer to the data to be written, NULL:Initiate/Finalize write operation */
	DWORD sc		/* Sector number (LBA) or Number of bytes to send */
)
{
	DRESULT res;

	if (!buff) {
		if (sc) {
			p("Init Write");
			current_write_sector = sc;
			bytes_written = 0;
		} else {
			p("Finish Write");
			memset((void*)write_buffer+bytes_written,0x0,512-bytes_written);
			if (sdmmc_sdcard_writesectors(current_write_sector,1, write_buffer))
				return RES_ERROR;
			bytes_written = 512;
		}
	} else {
		//p("Adding");
		memcpy((void*)write_buffer+bytes_written, buff, sc);
		bytes_written+=sc;
	}

	return RES_OK;
}

