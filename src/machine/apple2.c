/***************************************************************************

  apple2.c

  Machine file to handle emulation of the Apple II series.

  TODO:  Make a standard set of peripherals work.
  TODO:  Allow swappable peripherals in each slot.
  TODO:  Verify correctness of C08X switches.
			- are they read-only?
			- need to do double-read before write-enable RAM

***************************************************************************/

/* common.h included for the RomModule definition */
#include "common.h"
#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6502/M6502.h"
#include "drivers/apple2.h"

/* machine/ay3600.c */
extern void AY3600_init(void);
extern void AY3600_interrupt(void);
extern int  AY3600_anykey_clearstrobe_r(void);
extern int  AY3600_keydata_strobe_r(void);

/* machine/ap_disk2.c */
extern void apple2_slot6_init(void);

/* local */
unsigned char *apple2_slot1;
unsigned char *apple2_slot2;
unsigned char *apple2_slot3;
unsigned char *apple2_slot4;
unsigned char *apple2_slot5;
unsigned char *apple2_slot6;
unsigned char *apple2_slot7;

unsigned char *apple2_rom;

APPLE2_STRUCT a2;

static int a2_RDLCBNK2 = 0;
static int a2_RDLCRAM = 0;
static int a2_RDRAMRD = 0;
static int a2_RDRAMWR = 0;
static int a2_RDCXROM = 0;
static int a2_RDAUXZP = 0;
static int a2_RDC3ROM = 0;
static int a2_RD80COL = 0;
static int a2_RDALTCH = 0;
static int a2_RD80VID = 0;

static int a2_speaker_state = 0;

/***************************************************************************
  apple2_init_machine
***************************************************************************/
void apple2_init_machine(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* Init our language card banks to initially point to ROM */
	cpu_setbankhandler_w (1, MWA_ROM);
	cpu_setbankhandler_w (2, MWA_ROM);
	cpu_setbank (1, &RAM[0xD000]);
	cpu_setbank (2, &RAM[0xE000]);

	AY3600_init();

	/* TODO: add more initializers as we add more slots */
	apple2_slot6_init();
}

/***************************************************************************
  apple2_id_rom
***************************************************************************/
int apple2_id_rom (const char *name, const char *gamename)
{
	FILE *romfile;
	unsigned char magic[4];
	int retval;
		
	if (!(romfile = osd_fopen (name, gamename, OSD_FILETYPE_ROM_CART, 0))) return 0;

	retval = 0;
	/* Verify the file is in Apple II format */
	osd_fread (romfile, magic, 4);
	if (memcmp(magic,"2IMG",4)==0)
		retval = 1;

	osd_fclose (romfile);
	return retval;
}

/***************************************************************************
  apple2e_load_rom
***************************************************************************/
int apple2e_load_rom (void)
{
	/* Initialize second half of graphics memory to 0xFF for sneaky decoding purposes */
	memset(Machine->memory_region[1] + 0x1000,0xFF,0x1000);

	M6502_Type = M6502_PLAIN;

	return 0;
}

/***************************************************************************
  apple2ee_load_rom
***************************************************************************/
int apple2ee_load_rom (void)
{
	/* Initialize second half of graphics memory to 0xFF for sneaky decoding purposes */
	memset(Machine->memory_region[1] + 0x1000,0xFF,0x1000);

	M6502_Type = M6502_65C02;

	return 0;
}

/***************************************************************************
  apple2_interrupt
***************************************************************************/
int apple2_interrupt(void)
{
	/* We poll the keyboard periodically to scan the keys.  This is 
	   actually consistent with how the AY-3600 keyboard controller works. */
	AY3600_interrupt();

	return 0;
}

/***************************************************************************
  apple2_LC_ram1_w
***************************************************************************/
void apple2_LC_ram1_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	RAM[0x10000 + offset] = data;
}

/***************************************************************************
  apple2_LC_ram2_w
***************************************************************************/
void apple2_LC_ram2_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	RAM[0x11000 + offset] = data;
}

/***************************************************************************
  apple2_LC_ram_w
***************************************************************************/
void apple2_LC_ram_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	RAM[0x12000 + offset] = data;
}

/***************************************************************************
  apple2_c00x_r
***************************************************************************/
int apple2_c00x_r(int offset)
{
	/* Read the keyboard data and strobe */
	return AY3600_keydata_strobe_r();
}

/***************************************************************************
  apple2_c00x_w
***************************************************************************/
void apple2_c00x_w(int offset, int data)
{
	switch (offset)
	{
		/* 80STOREOFF */
		case 0x00:		a2.STORE80 = 0x00;		break;
		/* 80STOREON */
		case 0x01:		a2.STORE80 = 0x01;		break;
		/* RAMRDOFF */
		case 0x02:		a2.RAMRD = 0x00;		break;
		/* RAMRDON */
		case 0x03:		a2.RAMRD = 0x00;		break;
		/* RAMWRTOFF */
		case 0x04:		a2.RAMWRT = 0x00;		break;
		/* RAMWRTON */
		case 0x05:		a2.RAMWRT = 0x01;		break;
		/* INTCXROMOFF */
		case 0x06:		a2.INTCXROM = 0x00;		break;
		/* INTCXROMON */
		case 0x07:		a2.INTCXROM = 0x01;		break;
		/* ALZTPOFF */
		case 0x08:		a2.ALZTP = 0x00;		break;
		/* ALZTPON */
		case 0x09:		a2.ALZTP = 0x01;		break;
		/* SLOTC3ROMOFF */
		case 0x0A:		a2.SLOTC3ROM = 0x00;	break;
		/* SLOTC3ROMON */
		case 0x0B:		a2.SLOTC3ROM = 0x01;	break;
		/* 80COLOFF */
		case 0x0C:		a2.COL80 = 0x00;		break;
		/* 80COLON */
		case 0x0D:		a2.COL80 = 0x01;		break;
		/* ALTCHARSETOFF */
		case 0x0E:		a2.ALTCHARSET = 0x00;	break;
		/* ALTCHARSETON */
		case 0x0F:		a2.ALTCHARSET = 0x01;	break;
	}
}

/***************************************************************************
  apple2_c01x_r
***************************************************************************/
int apple2_c01x_r(int offset)
{
	switch (offset)
	{
		case 0x00:			return AY3600_anykey_clearstrobe_r();
		case 0x01:			return a2_RDLCBNK2;
		case 0x02:			return a2_RDLCRAM;
		case 0x03:			return a2_RDRAMRD;
		case 0x04:			return a2_RDRAMWR;
		case 0x05:			return a2_RDCXROM;
		case 0x06:			return a2_RDAUXZP;
		case 0x07:			return a2_RDC3ROM;
		case 0x08:			return a2_RD80COL;
		case 0x09:			return input_port_0_r(0);	/* RDVBLBAR */
		case 0x0A:			return a2.TEXT;
		case 0x0B:			return a2.MIXED;
		case 0x0C:			return a2.PAGE2;
		case 0x0D:			return a2.HIRES;
		case 0x0E:			return a2_RDALTCH;
		case 0x0F:			return a2_RD80VID;
	}

	return 0;
}

/***************************************************************************
  apple2_c01x_w
***************************************************************************/
void apple2_c01x_w(int offset, int data)
{
	/* Clear the keyboard strobe - ignore the returned results */
	AY3600_anykey_clearstrobe_r();
}

/***************************************************************************
  apple2_c02x_r
***************************************************************************/
int apple2_c02x_r(int offset)
{
	switch (offset)
	{
		case 0x00:
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}

	return 0;
}

/***************************************************************************
  apple2_c02x_w
***************************************************************************/
void apple2_c02x_w(int offset, int data)
{
	switch (offset)
	{
		case 0x00:
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}
}

/***************************************************************************
  apple2_c03x_r
***************************************************************************/
int apple2_c03x_r(int offset)
{
	switch (offset)
	{
		case 0x00:
			if (a2_speaker_state==0xFF)
				a2_speaker_state=0;
			else
				a2_speaker_state=0xFF;
			DAC_data_w(0,a2_speaker_state);
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}

	return 0;
}

/***************************************************************************
  apple2_c03x_w
***************************************************************************/
void apple2_c03x_w(int offset, int data)
{
	switch (offset)
	{
		case 0x00:
			if (a2_speaker_state==0xFF)
				a2_speaker_state=0;
			else
				a2_speaker_state=0xFF;
			DAC_data_w(0,a2_speaker_state);
			if (a2_speaker_state==0xFF)
				a2_speaker_state=0;
			else
				a2_speaker_state=0xFF;
			DAC_data_w(0,a2_speaker_state);
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}
}

/***************************************************************************
  apple2_c04x_r
***************************************************************************/
int apple2_c04x_r(int offset)
{
	switch (offset)
	{
		case 0x00:
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}

	return 0;
}

/***************************************************************************
  apple2_c04x_w
***************************************************************************/
void apple2_c04x_w(int offset, int data)
{
	switch (offset)
	{
		case 0x00:
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}
}

/***************************************************************************
  apple2_c05x_r
***************************************************************************/
int apple2_c05x_r(int offset)
{
	switch (offset)
	{
		case 0x00:		a2.TEXT  = 0x00; break;
		case 0x01:		a2.TEXT  = 0x80; break;
		case 0x02:		a2.MIXED = 0x00; break;
		case 0x03:		a2.MIXED = 0x80; break;
		case 0x04:		a2.PAGE2 = 0x00; break;
		case 0x05:		a2.PAGE2 = 0x80; break;
		case 0x06:		a2.HIRES = 0x00; break;
		case 0x07:		a2.HIRES = 0x80; break;
		case 0x08:		a2.AN0   = 0x80; break;	/* AN0 has reverse SET logic */
		case 0x09:		a2.AN0   = 0x00; break;
		case 0x0A:		a2.AN1   = 0x80; break; /* AN1 has reverse SET logic */
		case 0x0B:		a2.AN1   = 0x00; break;
		case 0x0C:		a2.AN2   = 0x80; break; /* AN2 has reverse SET logic */
		case 0x0D:		a2.AN2   = 0x00; break;
		case 0x0E:		a2.AN3   = 0x80; break; /* AN3 has reverse SET logic */
		case 0x0F:		a2.AN3   = 0x00; break;
	}

	return 0;
}

/***************************************************************************
  apple2_c05x_w
***************************************************************************/
void apple2_c05x_w(int offset, int data)
{
	switch (offset)
	{
		case 0x00:		a2.TEXT  = 0x00; break;
		case 0x01:		a2.TEXT  = 0x80; break;
		case 0x02:		a2.MIXED = 0x00; break;
		case 0x03:		a2.MIXED = 0x80; break;
		case 0x04:		a2.PAGE2 = 0x00; break;
		case 0x05:		a2.PAGE2 = 0x80; break;
		case 0x06:		a2.HIRES = 0x00; break;
		case 0x07:		a2.HIRES = 0x80; break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}
}

/***************************************************************************
  apple2_c06x_r
***************************************************************************/
int apple2_c06x_r(int offset)
{
	switch (offset)
	{
		case 0x00:
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}

	return 0;
}

/***************************************************************************
  apple2_c06x_w
***************************************************************************/
void apple2_c06x_w(int offset, int data)
{
	switch (offset)
	{
		case 0x00:
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}
}

/***************************************************************************
  apple2_c07x_r
***************************************************************************/
int apple2_c07x_r(int offset)
{
	switch (offset)
	{
		case 0x00:
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}

	return 0;
}

/***************************************************************************
  apple2_c07x_w
***************************************************************************/
void apple2_c07x_w(int offset, int data)
{
	switch (offset)
	{
		case 0x00:
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}
}

/***************************************************************************
  apple2_c08x_r
***************************************************************************/
int apple2_c08x_r(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if ((offset & 0x01)==0x00)
	{
		cpu_setbankhandler_w (1, MWA_ROM);
		cpu_setbankhandler_w (2, MWA_ROM);
	}
	else
	{
		cpu_setbankhandler_w (2, apple2_LC_ram_w);

		if ((offset & 0x08)==0x00)
			cpu_setbankhandler_w (1, apple2_LC_ram2_w);
		else
			cpu_setbankhandler_w (1, apple2_LC_ram1_w);
	}

	switch (offset & 0x03)
	{
		case 0x00:
		case 0x03:
			cpu_setbank (2, &RAM[0x12000]);
			if ((offset & 0x08)==0x00)
			{
				cpu_setbank (1, &RAM[0x11000]);
			}
			else
			{
				cpu_setbank (1, &RAM[0x10000]);
			}
			break;
		case 0x01:
		case 0x02:
			cpu_setbank (1, &RAM[0xD000]);
			cpu_setbank (2, &RAM[0xE000]);
			break;
	}

	return 0;
}

/***************************************************************************
  apple2_c08x_w
***************************************************************************/
void apple2_c08x_w(int offset, int data)
{
	switch (offset)
	{
		case 0x00:
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}
}

/***************************************************************************
  apple2_c0xx_slot1_r
***************************************************************************/
int apple2_c0xx_slot1_r(int offset)
{
	return 0;
}

/***************************************************************************
  apple2_c0xx_slot2_r
***************************************************************************/
int apple2_c0xx_slot2_r(int offset)
{
	return 0;
}

/***************************************************************************
  apple2_c0xx_slot3_r
***************************************************************************/
int apple2_c0xx_slot3_r(int offset)
{
	return 0;
}

/***************************************************************************
  apple2_c0xx_slot4_r
***************************************************************************/
int apple2_c0xx_slot4_r(int offset)
{
	return 0;
}

/***************************************************************************
  apple2_c0xx_slot5_r
***************************************************************************/
int apple2_c0xx_slot5_r(int offset)
{
	return 0;
}

/***************************************************************************
  apple2_c0xx_slot7_r
***************************************************************************/
int apple2_c0xx_slot7_r(int offset)
{
	return 0;
}

/***************************************************************************
  apple2_c0xx_slot1_w
***************************************************************************/
void apple2_c0xx_slot1_w(int offset, int data)
{
	return;
}

/***************************************************************************
  apple2_c0xx_slot2_w
***************************************************************************/
void apple2_c0xx_slot2_w(int offset, int data)
{
	return;
}

/***************************************************************************
  apple2_c0xx_slot3_w
***************************************************************************/
void apple2_c0xx_slot3_w(int offset, int data)
{
	return;
}

/***************************************************************************
  apple2_c0xx_slot4_w
***************************************************************************/
void apple2_c0xx_slot4_w(int offset, int data)
{
	return;
}

/***************************************************************************
  apple2_c0xx_slot5_w
***************************************************************************/
void apple2_c0xx_slot5_w(int offset, int data)
{
	return;
}

/***************************************************************************
  apple2_c0xx_slot7_w
***************************************************************************/
void apple2_c0xx_slot7_w(int offset, int data)
{
	return;
}

/***************************************************************************
  apple2_slot1_r
***************************************************************************/
int apple2_slot1_r(int offset)
{
	return 0;
}

/***************************************************************************
  apple2_slot2_r
***************************************************************************/
int apple2_slot2_r(int offset)
{
	return 0;
}

/***************************************************************************
  apple2_slot3_r
***************************************************************************/
int apple2_slot3_r(int offset)
{
	return 0;
}

/***************************************************************************
  apple2_slot4_r
***************************************************************************/
int apple2_slot4_r(int offset)
{
	return 0;
}

/***************************************************************************
  apple2_slot5_r
***************************************************************************/
int apple2_slot5_r(int offset)
{
	return 0;
}

/***************************************************************************
  apple2_slot7_r
***************************************************************************/
int apple2_slot7_r(int offset)
{
	return 0;
}

/***************************************************************************
  apple2_slot1_w
***************************************************************************/
void apple2_slot1_w(int offset, int data)
{
	return;
}

/***************************************************************************
  apple2_slot2_w
***************************************************************************/
void apple2_slot2_w(int offset, int data)
{
	return;
}

/***************************************************************************
  apple2_slot3_w
***************************************************************************/
void apple2_slot3_w(int offset, int data)
{
	return;
}

/***************************************************************************
  apple2_slot4_w
***************************************************************************/
void apple2_slot4_w(int offset, int data)
{
	return;
}

/***************************************************************************
  apple2_slot5_w
***************************************************************************/
void apple2_slot5_w(int offset, int data)
{
	return;
}

/***************************************************************************
  apple2_slot7_w
***************************************************************************/
void apple2_slot7_w(int offset, int data)
{
	return;
}

