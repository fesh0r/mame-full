/***************************************************************************

 SAM Coupe Driver - Written By Lee Hammerton


	Sam Coupe Memory Map - Based around the current spectrum.c (for obvious reasons!!)

	CPU:
		0000-7fff Banked rom/ram
		8000-ffff Banked rom/ram


Interrupts:

Changes:

 V0.2	- Added FDC support. - Based on 1771 document. Coupe had a 1772... (any difference?)
		  	floppy supports only read sector single mode at present will add write sector
			in next version.
		  Fixed up palette - had red & green wrong way round.

***************************************************************************/

//#define SAM_DISK_DEBUG

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "../machine/coupe.h"

void coupe_init_machine(void);
void coupe_shutdown_machine(void);
int coupe_vh_start(void);
void coupe_vh_stop(void);
void coupe_eof_callback(void);
void coupe_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

void drawMode1_line(struct osd_bitmap *,int);
void drawMode2_line(struct osd_bitmap *,int);
void drawMode3_line(struct osd_bitmap *,int);
void drawMode4_line(struct osd_bitmap *,int);

static struct MemoryReadAddress coupe_readmem[] = {
	{ 0x0000, 0x3FFF, MRA_BANK1 },
	{ 0x4000, 0x7FFF, MRA_BANK2 },
	{ 0x8000, 0xBFFF, MRA_BANK3 },
	{ 0xC000, 0xFFFF, MRA_BANK4 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress coupe_writemem[] = {
	{ 0x0000, 0x3FFF, MWA_BANK5 },
	{ 0x4000, 0x7FFF, MWA_BANK6 },
	{ 0x8000, 0xBFFF, MWA_BANK7 },
	{ 0xC000, 0xFFFF, MWA_BANK8 },
	{ -1 }  /* end of table */
};

int coupe_line_interrupt(void)
{
	struct osd_bitmap *bitmap = Machine->scrbitmap;
	int interrupted=0;				// This is used to allow me to clear the STAT flag (easiest way I can do it!)

	HPEN = CURLINE;

	if (LINE_INT<192)
	{
		if (CURLINE == LINE_INT)
		{
			STAT=0x1E;								// No other interrupts can occur - NOT CORRECT!!!
			cpu_cause_interrupt(0, Z80_IRQ_INT);
			interrupted=1;
		}
	}

	if (CURLINE && (CURLINE-1) < 192)               // scan line on screen so draw last scan line (may need to alter this slightly!!)
	{
		switch ((VMPR & 0x60)>>5)
		{
		case 0:			// mode 1
			drawMode1_line(bitmap,(CURLINE-1));
			break;
		case 1:			// mode 2
			drawMode2_line(bitmap,(CURLINE-1));
			break;
		case 2:			// mode 3
			drawMode3_line(bitmap,(CURLINE-1));
			break;
		case 3:			// mode 4
			drawMode4_line(bitmap,(CURLINE-1));
			break;
		}
	}

	CURLINE = (CURLINE + 1) % (192+10);

	if (CURLINE == 193)
	{
		if (interrupted)
			STAT&=~0x08;
		else
			STAT=0x17;

		cpu_cause_interrupt(0, Z80_IRQ_INT);
		interrupted=1;
	}

	if (!interrupted)
		STAT=0x1F;

	return ignore_interrupt();
}

unsigned char getSamKey1(unsigned char hi)
{
	unsigned char result;

	hi=~hi;
	result=0xFF;

	if (hi==0x00)
		result &=readinputport(8) & 0x1F;
	else
	{
		if (hi&0x80) result &= readinputport(7) & 0x1F;
		if (hi&0x40) result &= readinputport(6) & 0x1F;
		if (hi&0x20) result &= readinputport(5) & 0x1F;
		if (hi&0x10) result &= readinputport(4) & 0x1F;
		if (hi&0x08) result &= readinputport(3) & 0x1F;
		if (hi&0x04) result &= readinputport(2) & 0x1F;
		if (hi&0x02) result &= readinputport(1) & 0x1F;
		if (hi&0x01) result &= readinputport(0) & 0x1F;
	}

	return result;
}

unsigned char getSamKey2(unsigned char hi)
{
	unsigned char result;

	hi=~hi;
	result=0xFF;

	if (hi==0x00)
	{
		// does not map to any keys?
	}
	else
	{
		if (hi&0x80) result &= readinputport(7) & 0xE0;
		if (hi&0x40) result &= readinputport(6) & 0xE0;
		if (hi&0x20) result &= readinputport(5) & 0xE0;
		if (hi&0x10) result &= readinputport(4) & 0xE0;
		if (hi&0x08) result &= readinputport(3) & 0xE0;
		if (hi&0x04) result &= readinputport(2) & 0xE0;
		if (hi&0x02) result &= readinputport(1) & 0xE0;
		if (hi&0x01) result &= readinputport(0) & 0xE0;
	}

	return result;
}

#ifdef SAM_DISK_DEBUG
void printDskStatusInfo(unsigned char data,int id,int side)
{

	{
		switch (coupe_fdc1772[id].Dsk_Command&0xF0)		// Identify command
		{
			case 0x00:			// Restore
			case 0x50:			// Step inwards - update track register
			case 0x70:			// Step outwards - update track register
			case 0xD0:			// Force Interrupt
				logerror("Dsk %d Side %d Status General : Not Ready=%d,Write Prot=%d,Head Loaded=%d,Seek Error=%d,CRC Error=%d,Track 0=%d,Index Pulse=%d,Busy=%d\n"
					,id,side,(data&0x80)>>7,(data&0x40)>>6,(data&0x20)>>5,(data&0x10)>>4,(data&0x08)>>3,(data&0x04)>>2,(data&0x02)>>1,(data&0x01));
				break;
			case 0x80:
				logerror("Dsk %d Side %d Status Read Sector Single : Not Ready=%d,Mark Type=%d,Rec Not Found=%d,CRC Error=%d,Lost Data=%d,DRQ=%d,Busy=%d\n"
					,id,side,(data&0x80)>>7,(data&0x60)>>5,(data&0x10)>>4,(data&0x08)>>3,(data&0x04)>>2,(data&0x02)>>1,(data&0x01));
				break;
			default:
				logerror("Dsk %d Side %d Status <UNKNOWN> (%02x)\n",id,side,coupe_fdc1772[id].Dsk_Command);
		}
	}
}
#endif

READ_HANDLER ( coupe_port_r )
{
	if (offset==SSND_ADDR)						// Sound address request
		return SOUND_ADDR;

	if (offset==HPEN_PORT)
		return HPEN;

	switch (offset & 0xFF)
	{
	case DSK1_PORT+0:							// This covers the total range of ports for 1 floppy controller
	case DSK1_PORT+1:
	case DSK1_PORT+2:
	case DSK1_PORT+3:
	case DSK1_PORT+4:
	case DSK1_PORT+5:
	case DSK1_PORT+6:
	case DSK1_PORT+7:
		switch (offset & 0x03)
		{
			case 0x00:							// Status byte requested on address line
#ifdef SAM_DISK_DEBUG
				printDskStatusInfo(coupe_fdc1772[0].Dsk_Status,0,(offset&0x04) >> 2);
#endif
				if ((coupe_fdc1772[0].Dsk_Command&0xF0) == 0)
				{
					if (coupe_fdc1772[0].Dsk_Status&0x02)			// Coupe requires this bit is pulsed
						coupe_fdc1772[0].Dsk_Status&=~0x02;
					else
						coupe_fdc1772[0].Dsk_Status|=0x02;
				}
				if (coupe_fdc1772[0].f)
					return coupe_fdc1772[0].Dsk_Status;
				else
					return 0x80;
			case 0x01:							// Track byte requested on address line
#ifdef SAM_DISK_DEBUG
				logerror("Dsk 0 Side %d Track Var Read - Returned %d\n",(offset&0x04)>>2,coupe_fdc1772[0].Dsk_Track);
#endif
				return coupe_fdc1772[0].Dsk_Track;
			case 0x02:							// Sector byte requested on address line
#ifdef SAM_DISK_DEBUG
				logerror("Dsk 0 Side %d Sector Var Read - Returned %d\n",(offset&0x04)>>2,coupe_fdc1772[0].Dsk_Sector);
#endif
				return coupe_fdc1772[0].Dsk_Sector;
			case 0x03:							// Data byte requested on address line
				if ((coupe_fdc1772[0].Dsk_Command & 0xF0)==0x80)
				{
					osd_fread(coupe_fdc1772[0].f,&coupe_fdc1772[0].Dsk_Data,1);
					coupe_fdc1772[0].bytesLeft--;
					if (!coupe_fdc1772[0].bytesLeft)
						coupe_fdc1772[0].Dsk_Status&=~0x03;
				}
#ifdef SAM_DISK_DEBUG
				logerror("Dsk 0 Side %d Data Var Read - Returned %d\n",(offset&0x04)>>2,coupe_fdc1772[0].Dsk_Data);
#endif
				return coupe_fdc1772[0].Dsk_Data;
		}
	case LPEN_PORT:
		return LPEN;
	case STAT_PORT:
		return ((getSamKey2((offset >> 8)&0xFF))&0xE0) | STAT;
	case LMPR_PORT:
		return LMPR;
	case HMPR_PORT:
		return HMPR;
	case VMPR_PORT:
		return VMPR;
	case KEYB_PORT:
		return (getSamKey1((offset >> 8)&0xFF)&0x1F) | 0xE0;
	case SSND_DATA:
		return SOUND_REG[SOUND_ADDR];
	default:
		logerror("Read Unsupported Port: %04x\n", offset);
		break;
	}

	return 0x0ff;
}

#ifdef SAM_DISK_DEBUG
void printDskCommandInfo(unsigned char data,int id,int side)
{

	{
		if (side)
		{
		switch (data&0xF0)		// decode command part of sequence
		{
			case 0x00:			// Restore
				logerror("Dsk %d Side %d Command Restore : Head Load=%d,Verify=%d,Motor Rate=%d\n"
					,id,side,(data&0x08)>>3,(data&0x04)>>2,(data&0x03));
				break;
			case 0x50:			// Step Inwards (update track register)
				logerror("Dsk %d Side %d Command Step Inwards Update Track Register :  Head Load=%d,Verify=%d,Motor Rate=%d\n"
					,id,side,(data&0x08)>>3,(data&0x04)>>2,(data&0x03));
				break;
			case 0x70:			// Step Outwards (update track register)
				logerror("Dsk %d Side %d Command Step Outwards Update Track Register :  Head Load=%d,Verify=%d,Motor Rate=%d\n"
					,id,side,(data&0x08)>>3,(data&0x04)>>2,(data&0x03));
				break;
			case 0x80:
				logerror("Dsk %d Side %d Command Read Sector Single :  Check Side=%d,Delay=%d\n"
					,id,side,(data&0x08)>>3,(data&0x04)>>2);
				break;
			case 0xD0:			// Force Interrupt
				logerror("Dsk %d Side %d Command Force Interrupt : Now=%d,Next Index=%d,Ready->Not Ready=%d,Not Ready->Ready=%d,Abort=%d\n"
					,id,side,(data&0x08)>>3,(data&0x04)>>2,(data&0x02)>>1,(data&0x01),(data&0x0F)==0);
				break;
			default:
				logerror("Dsk %d Side %d Command <UNKNOWN> (%02x)\n",id,side,data);
		}
		}
	}
}
#endif

void handleDskCommandWrite(unsigned char data,int id,int side)
{
	coupe_fdc1772[id].Dsk_Command=data;			// Always pass command through regardless of whether disk image or not!

	if (coupe_fdc1772[id].f!=NULL)				// Valid disk image loaded so we can allow the commands to function
	{
		switch (data&0xF0)
		{
			case 0x00:								// Restore
				coupe_fdc1772[id].Dsk_Track=0;
				coupe_fdc1772[id].Dsk_Data=0;		// With this disk emulation everything is immediate...
				coupe_fdc1772[id].Dsk_Status=0x26;	// Set status to expected result. (head loaded, track 0)
				break;
			case 0x50:								// Step inwards update track register
				coupe_fdc1772[id].Dsk_Track++;
				coupe_fdc1772[id].Dsk_Status=0x20;
				break;
			case 0x70:								// Step outwards update track register
				coupe_fdc1772[id].Dsk_Track--;
				if (coupe_fdc1772[id].Dsk_Track==0)
					coupe_fdc1772[id].Dsk_Status=0x24;
				else
					coupe_fdc1772[id].Dsk_Status=0x20;
				break;
			case 0x80:								// Read sector single
				coupe_fdc1772[id].Dsk_Data=0;		// should be set to first byte of wanted sector.....!
				coupe_fdc1772[id].Dsk_Status=0x03;
				osd_fseek(coupe_fdc1772[id].f,(2*coupe_fdc1772[id].Dsk_Track * 10 * 512 + (side * 10 * 512)) + ((coupe_fdc1772[id].Dsk_Sector-1) * 512),SEEK_SET);
				coupe_fdc1772[id].bytesLeft=512;
				break;
			case 0xD0:								// Force interrupt
				coupe_fdc1772[id].Dsk_Status&=0xFE;	// Clear busy bit - should only be done on abort, but coupe does not support disk
													//  interrupts so, the only use for this function is to abort.
				coupe_fdc1772[id].Dsk_Command=0;	// This will allow the pulse bit to work properly again
				break;
			default:
													// Do nothing.. Command not emulated yet..
				break;
		}
	}
}

WRITE_HANDLER (  coupe_port_w )
{
	if (offset==SSND_ADDR)						// Set sound address
	{
		SOUND_ADDR=data&0x1F;					// 32 registers max
		return;
	}

	switch (offset & 0xFF)
	{
	case DSK1_PORT+0:							// This covers the total range of ports for 1 floppy controller
	case DSK1_PORT+1:
	case DSK1_PORT+2:
	case DSK1_PORT+3:
	case DSK1_PORT+4:
	case DSK1_PORT+5:
	case DSK1_PORT+6:
	case DSK1_PORT+7:
		switch (offset & 0x03)
		{
			case 0x00:							// Command byte requested on address line
#ifdef SAM_DISK_DEBUG
				printDskCommandInfo(data,0,(offset&0x04) >> 2);
#endif
				handleDskCommandWrite(data,0,(offset&0x04) >> 2);
				break;
			case 0x01:							// Track byte requested on address line
#ifdef SAM_DISK_DEBUG
				logerror("Dsk 0 Side %d Track %02x Set\n",(offset&0x04)>>2,data);
#endif
				coupe_fdc1772[0].Dsk_Track=data;
				break;
			case 0x02:							// Sector byte requested on address line
#ifdef SAM_DISK_DEBUG
				logerror("Dsk 0 Side %d Sector %02x Set\n",(offset&0x04)>>2,data);
#endif
				coupe_fdc1772[0].Dsk_Sector=data;
				break;
			case 0x03:							// Data byte requested on address line
#ifdef SAM_DISK_DEBUG
				logerror("Dsk 0 Side %d Data %02x Set\n",(offset&0x04)>>2,data);
#endif
				coupe_fdc1772[0].Dsk_Data=data;
				break;
		}
		return;
	case CLUT_PORT:
		CLUT[(offset >> 8)&0x0F]=data&0x7F;		// set CLUT data
		return;
	case LINE_PORT:
		LINE_INT=data;						// Line to generate interrupt on
		return;
	case LMPR_PORT:
		LMPR=data;
		coupe_update_memory();
		return;
	case HMPR_PORT:
		HMPR=data;
		coupe_update_memory();
		return;
	case VMPR_PORT:
		VMPR=data;
		coupe_update_memory();
		return;
	case BORD_PORT:
		/* DAC output state */
		speaker_level_w(0,(data>>4) & 0x01);
		return;
	case SSND_DATA:
		SOUND_REG[SOUND_ADDR]=data;
		return;
	default:
		logerror("Write Unsupported Port: %04x,%02x\n", offset,data);
		break;
	}
}

static struct IOReadPort coupe_readport[] = {
	{0, 0x0ffff, coupe_port_r},
	{ -1 }
};

static struct IOWritePort coupe_writeport[] = {
	{0x0000, 0x0ffff, coupe_port_w},
	{ -1 }
};

static struct GfxDecodeInfo coupe_gfxdecodeinfo[] = {
	{ -1 } /* end of array */
};

INPUT_PORTS_START( coupe )
	PORT_START // FE  0
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X,  IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V,  IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F1",KEYCODE_F1, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "F2",KEYCODE_F2, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "F3",KEYCODE_F3, IP_JOY_NONE )

	PORT_START // FD  1
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D,  IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G,  IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F4",KEYCODE_F4, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "F5",KEYCODE_F5, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "F6",KEYCODE_F6, IP_JOY_NONE )

	PORT_START // FB  2
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E,  IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T,  IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F7",KEYCODE_F7, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "F8",KEYCODE_F8, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "F9",KEYCODE_F9, IP_JOY_NONE )

	PORT_START // F7  3
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3,  IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5,  IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "ESC",KEYCODE_ESC, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "TAB",KEYCODE_TAB, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "CAPS LOCK",KEYCODE_CAPSLOCK, IP_JOY_NONE )

	PORT_START // EF  4
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8,  IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6,  IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "+", KEYCODE_EQUALS, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "BACKSPACE",KEYCODE_BACKSPACE, IP_JOY_NONE )

	PORT_START // DF  5
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I,  IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y,  IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "=", KEYCODE_OPENBRACE, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "\"", KEYCODE_CLOSEBRACE, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "F0", KEYCODE_F10, IP_JOY_NONE )

	PORT_START // BF  6
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "RETURN", KEYCODE_ENTER,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K,  IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H,  IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, ":", KEYCODE_QUOTE, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "EDIT", KEYCODE_RALT, IP_JOY_NONE )

	PORT_START // 7F  7
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "SYMBOL", KEYCODE_LCONTROL,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M,  IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B,  IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "INV", KEYCODE_SLASH, IP_JOY_NONE )

	PORT_START // FF  8
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LALT,  IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "UP", KEYCODE_UP,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "DOWN", KEYCODE_DOWN,  IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT", KEYCODE_LEFT,  IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHT", KEYCODE_RIGHT,  IP_JOY_NONE )

	PORT_START
	PORT_DIPNAME(0x80, 0x00, "Sam Ram")
	PORT_DIPSETTING(0x00, "256K" )						// Not implemented yet!!!
	PORT_DIPSETTING(0x80, "512K" )
INPUT_PORTS_END

/* Initialise the palette */
static void coupe_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	unsigned char red,green,blue;
	int a;
	unsigned char coupe_palette[128*3];
	unsigned short coupe_colortable[128];		// 1-1 relationship to palette!

	for (a=0;a<128;a++)
	{
		// decode colours for palette as follows :
		// bit number       7		6		5		4		3		2		1		0
		// 						|		|		|		|		|		|		|
		//				 nothing   G+4     R+4     B+4    ALL+1    G+2     R+2     B+2
		//
		// these values scaled up to 0-255 range would give modifiers of :  +4 = +(4*36), +2 = +(2*36), +1 = *(1*36)
		// not quite max of 255 but close enough for me!

		red=green=blue=0;
		if (a&0x01)
			blue+=2*36;
		if (a&0x02)
			red+=2*36;
		if (a&0x04)
			green+=2*36;
		if (a&0x08)
		{
			red+=1*36;
			green+=1*36;
			blue+=1*36;
		}
		if (a&0x10)
			blue+=4*36;
		if (a&0x20)
			red+=4*36;
		if (a&0x40)
			green+=4*36;

		coupe_palette[a*3+0]=red;
		coupe_palette[a*3+1]=green;
		coupe_palette[a*3+2]=blue;

		coupe_colortable[a]=a;
	}

	memcpy(sys_palette,coupe_palette,sizeof(coupe_palette));
	memcpy(sys_colortable,coupe_colortable,sizeof(coupe_colortable));
}

static struct Speaker_interface coupe_speaker_interface=
{
 1,
 {50},
};

static struct MachineDriver machine_driver_coupe =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
			6000000,        /* 6 Mhz */
		    coupe_readmem,coupe_writemem,
			coupe_readport,coupe_writeport,
			coupe_line_interrupt,192 + 10,			// 192 scanlines + 10 lines of vblank (approx)..

		},
	},
	50, /*2500*/0,       /* frames per second, vblank duration */
	1,
	coupe_init_machine,
	coupe_shutdown_machine,

	/* video hardware */
	64*8,                               /* screen width */
	24*8,                               /* screen height */
	{ 0, 64*8-1, 0, 24*8-1 },           /* visible_area */
	coupe_gfxdecodeinfo,				/* graphics decode info */
	128, 128,							/* colors used for the characters */
	coupe_init_palette,					/* initialise palette */

	VIDEO_TYPE_RASTER,
	0,
	coupe_vh_start,
	coupe_vh_stop,
	coupe_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		/* standard spectrum sound */
		{
			SOUND_SPEAKER,
			&coupe_speaker_interface
		},
	}

};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(coupe)
	ROM_REGION(0x18000,REGION_CPU1)
	ROM_LOAD("sam_rom0.rom", 0x10000, 0x4000, 0x9954CF1A)
	ROM_LOAD("sam_rom1.rom", 0x14000, 0x4000, 0xF031AED4)
ROM_END

static const struct IODevice io_coupe[] =
{
	{
		IO_FLOPPY,			/* type */
		2,					/* count */
		"dsk\0",            /* file extensions */		// Only .DSK (raw dump images) are supported at present
		NULL,				/* private */
		NULL,				/* id */
		coupe_fdc_init, 	/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* tell */
        NULL,               /* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
    },
	{ IO_END }
};

/*    YEAR  NAME      PARENT    MACHINE         INPUT     INIT          COMPANY                 		  FULLNAME */
COMP( 1989, coupe,	  0,        coupe,          coupe,    0,            "Miles Gordon Technology plc",    "Sam Coupé" )
