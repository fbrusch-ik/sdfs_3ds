#include "sdmmc.h"
/*
*	SDIO Handling code by Kane49
*	Adapted from the sdmmc module made for libnds
*/

//Globals and Register Access
int sdmmc_curdevice = -1;
u32 sdmmc_cid[4];
vu32 sdmmc_cardready = 0;

static int sdmmc_timeout = 0;
static int sdmmc_gotcmd8reply = 0;
static int sdmmc_sdhc = 0;

//Severely device dependent
void swiDelay(uint32 loops){
  int i;
  for (i = 0; i < loops*SLEEP_MULTIPLIER_3DS;i++);
}

//Sending Commands to the Controller
//---------------------------------------------------------------------------------
int sdmmc_send_command(u16 cmd, u16 arg0, u16 arg1) {
//---------------------------------------------------------------------------------
	//For status deltas
	u16 is_stat0, was_stat0;
	u16 is_stat1, was_stat1;
	
	//Copy our arguments to the appropriate registers
	sdmmc_write16(REG_SDCMDARG0, arg0);
	sdmmc_write16(REG_SDCMDARG1, arg1);

	was_stat1 = sdmmc_read16(REG_SDSTATUS1);

	while(sdmmc_read16(REG_SDSTATUS1) & 0x4000);

	is_stat1 = sdmmc_read16(REG_SDSTATUS1);

	sdmmc_mask16(0x1E, 0x807F, 0);
	sdmmc_mask16(0x1C, 0x0001, 0);
	sdmmc_mask16(0x1C, 0x0004, 0);
	sdmmc_mask16(0x22, 0x807F, 0);
	sdmmc_timeout = 0;

	sdmmc_write16(REG_SDCMD, cmd);

	if (cmd != 0) {
		was_stat0 = sdmmc_read16(REG_SDSTATUS0);
		//Check for timeout
		if (cmd != 0x5016) {
			while((sdmmc_read16(REG_SDSTATUS0) & 5) == 0) {
				if(sdmmc_read16(REG_SDSTATUS1) & 0x40) {
					sdmmc_mask16(REG_SDSTATUS1, 0x40, 0);
					sdmmc_timeout = 1;
					break;
				}
			}
		}

		is_stat0 = sdmmc_read16(REG_SDSTATUS0);
	}

	sdmmc_mask16(0x22, 0, 0x807F);

	return 0;
}

//---------------------------------------------------------------------------------
int sdmmc_cardinserted() {
//---------------------------------------------------------------------------------
	return sdmmc_cardready;
}


//Pretty surprisingly but this just works !
//---------------------------------------------------------------------------------
void sdmmc_controller_init() {
//---------------------------------------------------------------------------------
	// Reset
	sdmmc_write16(0x100, 0x0402);
	sdmmc_write16(0x100, 0x0000);
	sdmmc_write16(0x104, 0x0000);
	sdmmc_write16(0x108, 0x0001);

	// InitIP
	sdmmc_mask16(REG_SDRESET, 0x0001, 0x0000);
	sdmmc_mask16(REG_SDRESET, 0x0000, 0x0001);
	sdmmc_mask16(REG_SDSTOP, 0x0001, 0x0000);

	// Reset
	sdmmc_mask16(REG_SDOPT, 0x0005, 0x0000);

	// EnableInfo
	sdmmc_mask16(REG_SDSTATUS0, 0x0018, 0x0000);
	sdmmc_mask16(0x20, 0x0018, 0x0000); // ??
	sdmmc_mask16(0x20, 0x0000, 0x0018);
}

//---------------------------------------------------------------------------------
void sdmmc_send_acmd41(u32 arg, u32 *resp) {
//---------------------------------------------------------------------------------
	sdmmc_send_command(55, 0x0000, 0x000);
	*resp = sdmmc_read16(REG_SDRESP0) | (sdmmc_read16(REG_SDRESP1)<<16);

	sdmmc_send_command(41, (u16)arg, (arg>>16));
	
	*resp = sdmmc_read16(REG_SDRESP0) | (sdmmc_read16(REG_SDRESP1)<<16);
}

//Just works, thank god !
//---------------------------------------------------------------------------------
int sdmmc_sdcard_init() {
//---------------------------------------------------------------------------------
	u16 sdaddr;
	u32 resp0;
	u32 resp1;
	u32 resp2;
	u32 resp3;
	u32 resp4;
	u32 resp5;
	u32 resp6;
	u32 resp7;

	u32 ocr, ccs, hcs, response;

	sdmmc_cardready = 0;

	sdmmc_write16(REG_SDCLKCTL, 0x20);
	sdmmc_write16(REG_SDOPT, 0x40EA); // XXX: document me!
	sdmmc_write16(0x02, 0x400);
	sdmmc_curdevice = 0;

	sdmmc_mask16(REG_SDCLKCTL, 0, 0x100);
	sdmmc_write16(REG_SDCLKCTL, sdmmc_read16(REG_SDCLKCTL));
	sdmmc_mask16(REG_SDCLKCTL, 0x100, 0);
	sdmmc_mask16(REG_SDCLKCTL, 0, 0x200);
	sdmmc_mask16(REG_SDCLKCTL, 0, 0x100);

	// CMD0
	sdmmc_send_command(0x0000, 0x0000, 0x0000);
	sdmmc_send_command(0x0408, 0x01aa, 0x0000);
	sdmmc_gotcmd8reply = 1;

	if(sdmmc_timeout)
		sdmmc_gotcmd8reply = 0;

	if(sdmmc_gotcmd8reply)
		hcs = (1<<30);
	else
		hcs = 0;

	ocr = 0x00ff8000;

	while (1) {
		sdmmc_send_acmd41(ocr | hcs, &response);

		if (response & 0x80000000)
			break;
	}

	ccs = response & (1<<30);

	if(ccs && hcs) 
		sdmmc_sdhc = 1;
	else
		sdmmc_sdhc = 0;

	// CMD2 - get CID
	sdmmc_send_command(2, 0, 0);
	
	resp0 = sdmmc_read16(REG_SDRESP0);
	resp1 = sdmmc_read16(REG_SDRESP1);
	resp2 = sdmmc_read16(REG_SDRESP2);
	resp3 = sdmmc_read16(REG_SDRESP3);
	resp4 = sdmmc_read16(REG_SDRESP4);
	resp5 = sdmmc_read16(REG_SDRESP5);
	resp6 = sdmmc_read16(REG_SDRESP6);
	resp7 = sdmmc_read16(REG_SDRESP7);

	sdmmc_cid[0] = resp0 | (u32)(resp1<<16);
	sdmmc_cid[1] = resp2 | (u32)(resp3<<16);
	sdmmc_cid[2] = resp4 | (u32)(resp5<<16);
	sdmmc_cid[3] = resp6 | (u32)(resp7<<16);

	// CMD3
	sdmmc_send_command(3, 0, 0);
	
	resp0 = sdmmc_read16(REG_SDRESP0);
	resp1 = sdmmc_read16(REG_SDRESP1);
	
	sdaddr = resp1;
	
	// CMD9 - get CSD
	sdmmc_send_command(9, 0, sdaddr);
	
	resp0 = sdmmc_read16(REG_SDRESP0);
	resp1 = sdmmc_read16(REG_SDRESP1);
	resp2 = sdmmc_read16(REG_SDRESP2);
	resp3 = sdmmc_read16(REG_SDRESP3);
	resp4 = sdmmc_read16(REG_SDRESP4);
	resp5 = sdmmc_read16(REG_SDRESP5);
	resp6 = sdmmc_read16(REG_SDRESP6);
	resp7 = sdmmc_read16(REG_SDRESP7);
	
	sdmmc_write16(REG_SDCLKCTL, 0x100);	
	
	// CMD7
	sdmmc_send_command(7, 0, sdaddr);
	
	resp0 = sdmmc_read16(REG_SDRESP0);
	resp1 = sdmmc_read16(REG_SDRESP1);
	
	// CMD55
	sdmmc_send_command(55, 0, sdaddr);
	
	// ACMD6
	sdmmc_send_command(6, 2, 0);
	
	resp0 = sdmmc_read16(REG_SDRESP0);
	resp1 = sdmmc_read16(REG_SDRESP1);

	sdmmc_send_command(13, 0, sdaddr);
	
	resp0 = sdmmc_read16(REG_SDRESP0);
	resp1 = sdmmc_read16(REG_SDRESP1);

	sdmmc_send_command(16, 0x200, 0x0);
	
	resp0 = sdmmc_read16(REG_SDRESP0);
	resp1 = sdmmc_read16(REG_SDRESP1);	
	
	sdmmc_write16(REG_SDCLKCTL, sdmmc_read16(REG_SDCLKCTL));
	sdmmc_write16(REG_SDBLKLEN, 0x200);

	sdmmc_mask16(REG_SDCLKCTL, 0x100, 0);

	sdmmc_cardready = 1;

	return 0;
}


//The Relations between these is fine, the multiplier may change per device
//---------------------------------------------------------------------------------
void sdmmc_clkdelay0() {
//---------------------------------------------------------------------------------
	swiDelay(1330);
}

//---------------------------------------------------------------------------------
void sdmmc_clkdelay1() {
//---------------------------------------------------------------------------------
	swiDelay(32000);
}

//---------------------------------------------------------------------------------
void sdmmc_clkdelay() {
//---------------------------------------------------------------------------------
	swiDelay(1330);
}

//---------------------------------------------------------------------------------
int sdmmc_sdcard_readsector(DWORD sector_no, void *out) {
        u8 *out8 = (u8*)out;
        u16 resp0, resp1, tmp;
        int i;

        //SDHC Cards use block addressing instead of by byte
        if(!sdmmc_sdhc)
                sector_no *= 512;

        //Turn on the clock and set the sector size
        sdmmc_mask16(REG_SDCLKCTL, 0, CLOCK_MASK);
        sdmmc_clkdelay();
        sdmmc_write16(REG_SDBLKLEN, SECTOR_SIZE_3DS);
        
        //Splitting the 32bit block address into 2 16 bit values
        // CMD17 - read single block
        sdmmc_send_command(17, sector_no & 0xffff, (sector_no >> 16));
        //We dont have interrupts so there are a bunch of waits from this part on out

	        //Should we timeout, better return an error
	        if(sdmmc_timeout) {
	                sdmmc_clkdelay1();
	                sdmmc_clkdelay0();
	                sdmmc_mask16(REG_SDCLKCTL, CLOCK_MASK, 0);
	                return 1;
	        }

	        resp0 = sdmmc_read16(REG_SDRESP0);
	        resp1 = sdmmc_read16(REG_SDRESP1);        

	        resp0 = sdmmc_read16(REG_SDSTATUS1);

	        while (!(sdmmc_read16(REG_SDSTATUS1) & 0x100));

	        resp0 = sdmmc_read16(REG_SDSTATUS1);
	        
	        sdmmc_clkdelay1();

	        sdmmc_mask16(REG_SDSTATUS1, 1, 0);
	        
	    //Filling out 512 byte work buffer with data from the pipeline register
        for(i = 0; i < 0x100; i++) {
	        tmp = sdmmc_read16(REG_SDFIFO);
	        //Yeah, bitwise rummaging so we get 2 bytes our of our 16 bits
	        out8[i*2+1] = (tmp & 0xFF00) >> 8;
	        out8[i*2] = (tmp & 0x00FF);
        }
        
        sdmmc_clkdelay0();
        //Turn off the clock
        sdmmc_mask16(REG_SDCLKCTL, 0x100, 0);

        return 0;

}

//Read the comments from ReadSector for explanations :)
//---------------------------------------------------------------------------------
int sdmmc_sdcard_readsectors(u32 sector_no, int numsectors, void *out) {
//---------------------------------------------------------------------------------
        u8 *out8 = (u8*)out;
        u16 resp0, resp1, tmp;
        int i;

        if(!sdmmc_sdhc)
                sector_no *= 512;

        sdmmc_write16(REG_SDSTOP, 0x100);
        sdmmc_write16(REG_SDBLKCOUNT, numsectors);
        sdmmc_mask16(REG_SDCLKCTL, 0, 0x100);
        sdmmc_clkdelay();
        sdmmc_write16(REG_SDBLKLEN, 0x200);        
        
        // CMD18 - read multiple blocks
        sdmmc_send_command(18, sector_no & 0xffff, (sector_no >> 16));

        if(sdmmc_timeout) {
                sdmmc_clkdelay1();
                sdmmc_clkdelay0();
                sdmmc_mask16(REG_SDCLKCTL, 0x100, 0);
                return 1;
        }

        resp0 = sdmmc_read16(REG_SDRESP0);
        resp1 = sdmmc_read16(REG_SDRESP1);        
        resp0 = sdmmc_read16(REG_SDSTATUS1);


        while (!(sdmmc_read16(REG_SDSTATUS1) & 0x100));

        resp0 = sdmmc_read16(REG_SDSTATUS1);
        
        sdmmc_clkdelay1();

        sdmmc_mask16(REG_SDSTATUS1, 1, 0);
        
        for(i = 0; i < 0x100; i++) {
	        tmp = sdmmc_read16(REG_SDFIFO);
	        out8[i*2+1] = (tmp & 0xFF00) >> 8;
	        out8[i*2] = (tmp & 0x00FF);
        }

        sdmmc_clkdelay0();
        sdmmc_mask16(REG_SDCLKCTL, 0x100, 0);

        return 0;
}

//---------------------------------------------------------------------------------
int sdmmc_sdcard_writesector(u32 sector_no, void *in) {
//---------------------------------------------------------------------------------
	u8 *in8 = (u8*)in;
	u16 resp0, resp1, tmp;
	int i;

	if(!sdmmc_sdhc)sector_no *= 512;

	sdmmc_clkdelay0();
	sdmmc_mask16(REG_SDCLKCTL, 0, 0x100);
	sdmmc_clkdelay();
	sdmmc_write16(REG_SDBLKLEN, 0x200);	
	
	// CMD24 - write single block
	sdmmc_send_command(24, sector_no & 0xffff, (sector_no >> 16));

    if(sdmmc_timeout) {
                sdmmc_clkdelay1();
                sdmmc_clkdelay0();
                sdmmc_mask16(REG_SDCLKCTL, 0x100, 0);
                return 1;
    }

	resp0 = sdmmc_read16(REG_SDRESP0);
	resp1 = sdmmc_read16(REG_SDRESP1);	

	resp0 = sdmmc_read16(REG_SDSTATUS1);

	while (!(sdmmc_read16(REG_SDSTATUS1) & 0x200));

	resp0 = sdmmc_read16(REG_SDSTATUS1);

	sdmmc_clkdelay1();

	sdmmc_mask16(REG_SDSTATUS1, 1, 0);
	
	for(i = 0; i < 0x100; i++) {
        tmp = 0;
        tmp = tmp | in8[i*2+1];
        tmp = tmp << 8;
        tmp = tmp | in8[i*2+0];
        if (tmp !=0) ps(tmp);
        sdmmc_write16(REG_SDFIFO, tmp);
	}

	sdmmc_clkdelay0();
	sdmmc_mask16(REG_SDCLKCTL, 0x100, 0);
	sdmmc_clkdelay();
	return 0;
}

//---------------------------------------------------------------------------------
int sdmmc_sdcard_writesectors(u32 sector_no, int numsectors, void *in) {
//---------------------------------------------------------------------------------
	u32 sect_count = 0;
	for (sect_count = 0; sect_count < numsectors; sect_count++){
		sdmmc_sdcard_writesector(sector_no+sect_count, in+512*sect_count);
	}
	return 0;
}