/***************************************************************************

  snes.c

  Machine file to handle emulation of the Nintendo Super NES

  Anthony Kruize
  Based on the original code by Lee Hammerton (aka Savoury Snax)

***************************************************************************/
#define __MACHINE_SNES_C

#include "driver.h"
#include "includes/snes.h"
#include "cpu/g65816/g65816.h"
#include "image.h"

/* -- Macros -- */
/* Handles double-write(16bit) registers */
#define DOUBLE_WRITE( addr, data )				\
	addr = ((addr >> 8) & 0xff) + (data << 8);

/* -- Useful Defines -- */
#define DMA_BASE 0x4300
#define MODE_20 1
#define MODE_21 2
#ifdef MAME_DEBUG
/* #define V_GENERAL*/		/* Display general debug information */
/* #define V_GDMA*/			/* Display GDMA debug information */
/* #define V_HDMA*/			/* Display HDMA debug information */
/* #define V_REGISTERS*/	/* Display register debug information */
#endif
#define USE_SPCSKIPPER		/* Use the SPCSkipper instead of the SPC700 */

/* -- Globals -- */
UINT8  *snes_ram;		/* 65816 ram */
UINT8  *spc_ram;		/* spc700 ram */
UINT16 sram_size;		/* Size of the save ram in the cart */
UINT8  *snes_vram;		/* Video RAM (Should be 16-bit, but it's easier this way) */
UINT16 *snes_cgram;		/* Colour RAM */
UINT16 *snes_oam;		/* Object Attribute Memory */
UINT16 bg_hoffset[4];	/* Background horizontal scroll offsets */
UINT16 bg_voffset[4];	/* Background vertical scroll offsets */
UINT16 mode7_data[6];	/* Data for mode7 matrix calculation */
UINT8  spc_port_in[4];	/* Port for sending data to the SPC700 */
UINT8  spc_port_out[4];	/* Port for receiving data from the SPC700 */
static UINT8 oam_address_l, oam_address_h;	/* Address in oam */
static UINT16 cur_vline = 0, cur_hline = 0;	/* Current h/v line */
static UINT16 joypad_oldrol;				/* For old joystick stuff */
static UINT16 cgram_address = 0;	/* Current r/w address into cgram */
static UINT16 ppu_vlatch = 0;		/* Latch for vertical beam pos. */
static UINT16 ppu_hlatch = 0;		/* Latch for horizontal beam pos. */
static UINT8 rom_mode = MODE_20;	/* Rom Mode */

MACHINE_INIT( snes )
{
	/* Init VRAM */
	snes_vram = (UINT8 *)memory_region( REGION_GFX1 );
	memset( snes_vram, 0, 0x20000 );

	/* Init Colour RAM */
	snes_cgram = (UINT16 *)memory_region( REGION_USER1 );
	memset( snes_cgram, 0, 0x202 );

	/* Init oam RAM */
	snes_oam = (UINT16 *)memory_region( REGION_USER2 );
	memset( snes_oam, 0xff, 0x440 );

	/* Get SPC700 ram */
	spc_ram = (UINT8 *)memory_region( REGION_CPU2 );
}

MACHINE_STOP( snes )
{
	/* Save RAM */
	battery_save( image_filename(IO_CARTSLOT,0), &snes_ram[0x700000], sram_size );

#ifdef MAME_DEBUG
	battery_save( "snesvram", snes_vram, 0x20000 );
	battery_save( "snescgram", snes_cgram, 0x202 );
	battery_save( "snesoam", snes_oam, 0x440 );
#endif
}

/* 0x000000 - 0x2fffff */
READ_HANDLER( snes_r_bank1 )
{
	UINT16 address = offset & 0xffff;

	if( address <= 0x1fff )								/* Mirror of Low RAM */
		return cpu_readmem24( 0x7e0000 + address );
	else if( address >= 0x2000 && address <= 0x5fff )	/* I/O */
		return snes_r_io( address );
	else if( address >= 0x6000 && address <= 0x7fff )	/* Reserved */
		return 0xff;
	else
	{
		if( rom_mode == MODE_20 )
			return snes_ram[offset];
		else	/* MODE_21 */
			return snes_ram[0xc00000 + offset];
	}

	return 0xff;
}

/* 0x300000 - 0x3fffff */
READ_HANDLER( snes_r_bank2 )
{
	UINT16 address = offset & 0xffff;

	if( address <= 0x1fff )								/* Mirror of Low RAM */
		return cpu_readmem24( 0x7e0000 + address );
	else if( address >= 0x2000 && address <= 0x5fff )	/* I/O */
		return snes_r_io( address );
	else if( address >= 0x6000 && address <= 0x7fff )	/* Reserved */
	{
		if( rom_mode == MODE_20 )
			return 0xff;
		else	/* MODE_21 */
			return snes_ram[0x300000 + offset];		/* FIXME: this should be sram */
	}
	else
	{
		if( rom_mode == MODE_20 )
			return snes_ram[0x300000 + offset];
		else	/* MODE_21 */
			return snes_ram[0xf00000 + offset];
	}

	return 0xff;
}

/* 0x400000 - 0x5fffff */
READ_HANDLER( snes_r_bank3 )
{
	UINT16 address = offset & 0xffff;

	if( rom_mode == MODE_20 )
	{
		if( address <= 0x7fff )
			return 0xff;
		else
			return snes_ram[0x400000 + offset];
	}
	else	/* MODE_21 */
	{
		return snes_ram[0x400000 + offset];
	}

	return 0xff;
}

/* 0x800000 - 0xffffff */
READ_HANDLER( snes_r_bank4 )
{
	if( rom_mode == MODE_20 )
	{
		if( offset <= 0x5fffff )
			return cpu_readmem24( offset );
		else
			return 0xff;
	}
	else	/* MODE_21 */
	{
		if( offset <= 0x3fffff )
			return cpu_readmem24( offset );
		else
			return snes_ram[offset + 0x800000];
	}

	return 0xff;
}

/* 0x000000 - 0x2fffff */
WRITE_HANDLER( snes_w_bank1 )
{
	UINT16 address = offset & 0xffff;

	if( address <= 0x1fff )								/* Mirror of Low RAM */
		cpu_writemem24( 0x7e0000 + address, data );
	else if( address >= 0x2000 && address <= 0x5fff )	/* I/O */
		snes_w_io( address, data );
	else if( address >= 0x6000 && address <= 0x7fff )	/* Reserved */
		logerror( "Attempt to write to reserved address: %X\n", offset );
	else
		logerror( "Attempt to write to ROM address: %X\n", offset );
}

/* 0x300000 - 0x3fffff */
WRITE_HANDLER( snes_w_bank2 )
{
	UINT16 address = offset & 0xffff;

	if( address <= 0x1fff )								/* Mirror of Low RAM */
		cpu_writemem24( 0x7e0000 + address, data );
	else if( address >= 0x2000 && address <= 0x5fff )	/* I/O */
		snes_w_io( address, data );
	else if( address >= 0x6000 && address <= 0x7fff )	/* Reserved */
	{
		if( rom_mode == MODE_20 )
			logerror( "Attempt to write to reserved address: %X\n", offset );
		else
			snes_ram[0x300000 + offset] = data;  /* FIXME: this should be sram */
	}
	else
		logerror( "Attempt to write to ROM address: %X\n", offset );
}

/* 0x800000 - 0xffffff */
WRITE_HANDLER( snes_w_bank4 )
{
	if( rom_mode == MODE_20 )
	{
		if( offset <= 0x2fffff )
			snes_w_bank1( offset, data );
		else if( offset >= 0x300000 && offset <= 0x3fffff )
			snes_w_bank2( offset - 0x300000, data );
	}
	else /* MODE_21 */
	{
		if( offset <= 0x2fffff )
			snes_w_bank1( offset, data );
		else if( offset >= 0x300000 && offset <= 0x3fffff )
			snes_w_bank2( offset - 0x300000, data );
		else
			logerror( "Attempt to write to ROM address: %X\n", offset );
	}
}

READ_HANDLER( snes_r_io )
{
	UINT8 value;
	static UINT8 joypad1_l, joypad2_l, joypad3_l, joypad4_l;
	static UINT8 joypad1_h, joypad2_h, joypad3_h, joypad4_h;

	/* offset is from 0x000000 */
	switch( offset )
	{
		case MPYL:		/* Multiplication result (low) */
		case MPYM:		/* Multiplication result (mid) */
		case MPYH:		/* Multiplication result (high) */
			{
				/* Perform 16bit * 8bit multiply */
				UINT32 c = mode7_data[0] * (mode7_data[1] >> 8);
				snes_ram[MPYL] = c & 0xff;
				snes_ram[MPYM] = (c >> 8) & 0xff;
				snes_ram[MPYH] = (c >> 16) & 0xff;
				return snes_ram[offset];
			}
		case SLHV:		/* Software latch for H/V counter */
			/* FIXME: horizontal latch is a major fudge!!! */
			ppu_vlatch = cur_vline;
			ppu_hlatch = cur_hline++;
			if( cur_hline > 339 )
				cur_hline = 0;
			break;
		case ROAMDATA:	/* Read data from OAM (DR) */
			{
				UINT16 oam_address = ((snes_ram[OAMADDH] & 0x1) << 8) + snes_ram[OAMADDL];
				value = snes_oam[oam_address] & (0xff << (snes_ram[OAMDATA] << 3));
				snes_ram[OAMDATA]++;
				if( snes_ram[OAMDATA] == 2 )
				{
					snes_ram[OAMDATA] = 0;
					oam_address++;
					snes_ram[OAMADDL] = oam_address & 0xff;
					snes_ram[OAMADDH] = (oam_address >> 8) & 0x1;	/* FIXME: we are splatting the priority bit here */
				}
			}
			return value;
		case RVMDATAL:	/* Read data from VRAM (low) */
			{
				UINT16 addr = (snes_ram[VMADDH] << 8) | snes_ram[VMADDL];

				value = snes_vram[addr << 1];
				if( !(snes_ram[VMAIN] & 0x80) )
				{
					switch( snes_ram[VMAIN] & 0x03 )
					{
						case 0: addr++;      break;
						case 1: addr += 32;  break;
						case 2: addr += 128; break; /* Should be 64, but a bug in the snes means it's 128 */
						case 3: addr += 128; break;
					}
					snes_ram[VMADDL] = addr & 0xff;
					snes_ram[VMADDH] = (addr >> 8) & 0xff;
				}
				return value;
			}
		case RVMDATAH:	/* Read data from VRAM (high) */
			{
				UINT16 addr = (snes_ram[VMADDH] << 8) | snes_ram[VMADDL];

				value = snes_vram[(addr << 1) + 1];
				if( snes_ram[VMAIN] & 0x80 )
				{
					/* Increase the address */
					switch( snes_ram[VMAIN] & 0x03 )
					{
						case 0: addr++;      break;
						case 1: addr += 32;  break;
						case 2: addr += 128; break; /* Should be 64, but a bug in the snes means it's 128 */
						case 3: addr += 128; break;
					}
					snes_ram[VMADDL] = addr & 0xff;
					snes_ram[VMADDH] = (addr >> 8) & 0xff;
				}
				return value;
			}
		case RCGDATA:	/* Read data from CGRAM */
				value = ((UINT8 *)snes_cgram)[cgram_address++];
				if( cgram_address >= 0x202 )
					cgram_address = 0;
				return value;
		case OPHCT:		/* Horizontal counter data by ext/soft latch */
			{
				/* FIXME: need to handle STAT78 reset */
				static UINT8 read_ophct = 0;
				if( read_ophct )
				{
					value = (ppu_hlatch >> 8) & 0x1;
					read_ophct = 0;
				}
				else
				{
					value = ppu_hlatch & 0xff;
					read_ophct = 1;
				}
				return value;
			}
		case OPVCT:		/* Vertical counter data by ext/soft latch */
			{
				/* FIXME: need to handle STAT78 reset */
				static UINT8 read_opvct = 0;
				if( read_opvct )
				{
					value = (ppu_vlatch >> 8) & 0x1;
					read_opvct = 0;
				}
				else
				{
					value = ppu_vlatch & 0xff;
					read_opvct = 1;
				}
				return value;
			}
		case STAT77:	/* PPU status flag and version number */
		case STAT78:	/* PPU status flag and version number */
			/* FIXME: need to reset OPHCT and OPVCT */
			return snes_ram[offset];
		case APU00:		/* Audio port register */
		case APU01:		/* Audio port register */
		case APU02:		/* Audio port register */
		case APU03:		/* Audio port register */
#ifndef USE_SPCSKIPPER
			return spc_port_out[offset & 0x3];
#else
			return snes_fakeapu_r_port( offset & 0x3 );
#endif /* USE_SPCSKIPPER */
		case WMDATA:	/* Data to read from WRAM */
			{
				UINT32 addr = ((snes_ram[WMADDH] & 0x1) << 16) | (snes_ram[WMADDM] << 8) | snes_ram[WMADDL];

				value = cpu_readmem24(0x7e0000 + addr++);
				snes_ram[WMADDH] = (addr >> 16) & 0x1;
				snes_ram[WMADDM] = (addr >> 8) & 0xff;
				snes_ram[WMADDL] = addr & 0xff;
				return value;
			}
		case WMADDL:	/* Address to read/write to wram (low) */
		case WMADDM:	/* Address to read/write to wram (mid) */
		case WMADDH:	/* Address to read/write to wram (high) */
			return snes_ram[offset];
		case OLDJOY1:	/* Data for old NES controllers */
		case OLDJOY2:	/* Data for old NES controllers */
			/* FIXME: Is this even correct?  what are we doing? */
			value = (joypad1_h << 8) | joypad1_l;
			value <<= joypad_oldrol;
			joypad_oldrol++;
			joypad_oldrol &= 0x15;
/*			return (value & 0x1);*/
			return 0xff;	/* FIXME: Returning 0xff for now, this is not correct though */
		case HTIMEL:
		case HTIMEH:
		case VTIMEL:
		case VTIMEH:
			return snes_ram[offset];
		case RDNMI:			/* NMI flag by v-blank and version number */
			value = snes_ram[offset];
			snes_ram[offset] &= 0xf;	/* NMI flag is reset on read */
			return value;
		case TIMEUP:		/* IRQ flag by H/V count timer */
			value = snes_ram[offset];
			snes_ram[offset] = 0;	/* Register is reset on read */
			return value;
		case HVBJOY:		/* H/V blank and joypad controller enable */
			/* FIXME: JOYCONT and HBLANK are emulated wrong at present */
			value = snes_ram[offset] & 0xbe;
			snes_ram[offset] = ((snes_ram[offset]^0x41)&0x41)|value;
			return snes_ram[offset];
		case RDIO:			/* Programmable I/O port (in port ) */
			/* FIXME: do something here */
			break;
		case RDDIVL:		/* Quotient of divide result (low) */
		case RDDIVH:		/* Quotient of divide result (high) */
		case RDMPYL:		/* Product/Remainder of mult/div result (low) */
		case RDMPYH:		/* Product/Remainder of mult/div result (high) */
			return snes_ram[offset];
		case JOY1L:			/* Joypad 1 status register (low) */
			joypad1_l = readinputport( 0 );
			return joypad1_l;
		case JOY1H:			/* Joypad 1 status register (high) */
			joypad1_h = readinputport( 1 );
			return joypad1_h;
		case JOY2L:			/* Joypad 2 status register (low) */
			joypad2_l = readinputport( 2 );
			return joypad2_l;
		case JOY2H:			/* Joypad 2 status register (high) */
			joypad2_h = readinputport( 3 );
			return joypad2_h;
		case JOY3L:			/* Joypad 3 status register (low) */
			joypad3_l = readinputport( 4 );
			return joypad3_l;
		case JOY3H:			/* Joypad 3 status register (high) */
			joypad3_h = readinputport( 5 );
			return joypad3_h;
		case JOY4L:			/* Joypad 4 status register (low) */
			joypad4_l = readinputport( 6 );
			return joypad4_l;
		case JOY4H:			/* Joypad 4 status register (high) */
			joypad4_h = readinputport( 7 );
			return joypad4_h;
	}

	/* Unsupported reads return 0xff */
	return 0xff;
}

WRITE_HANDLER( snes_w_io )
{
	/* offset is from 0x000000 */
	switch( offset )
	{
		case INIDISP:	/* Initial settings for screen */
			{
				UINT8 r, g, b, fade;
				UINT16 ii;
				UINT32 col;

				/* Modify the palette to fade out the colours */
				fade = (data & 0xf) + 1;
				for( ii = 0; ii <= 256; ii++ )
				{
					col = Machine->pens[snes_cgram[ii] & 0x7fff];
					r = ((col & 0x1f) * fade) >> 4;
					g = (((col & 0x3e0) >> 5) * fade) >> 4;
					b = (((col & 0x7c00) >> 10) * fade) >> 4;
					Machine->remapped_colortable[ii] = ((r & 0x1f) | ((g & 0x1f) << 5) | ((b & 0x1f) << 10));
				}
			} break;
		case OBSEL:		/* Object size and data area designation */
			/* Determine sprite size */
			switch( (data & 0xe0) >> 5 )
			{
				case 0:			/* 8 & 16 */
					ppu_obj_size[0] = 1;
					ppu_obj_size[1] = 2;
					break;
				case 1:			/* 8 & 32 */
					ppu_obj_size[0] = 1;
					ppu_obj_size[1] = 4;
					break;
				case 2:			/* 8 & 64 */
					ppu_obj_size[0] = 1;
					ppu_obj_size[1] = 8;
					break;
				case 3:			/* 16 & 32 */
					ppu_obj_size[0] = 2;
					ppu_obj_size[1] = 4;
					break;
				case 4:			/* 16 & 64 */
					ppu_obj_size[0] = 2;
					ppu_obj_size[1] = 8;
					break;
				case 5:			/* 32 & 64 */
					ppu_obj_size[0] = 4;
					ppu_obj_size[1] = 8;
					break;
			}
#ifdef V_REGISTERS
			if( ((data >> 5) != (snes_ram[OBSEL] >> 5)) )
				printf( "Object: size: %d/%d\n", ppu_obj_size[0] * 8, ppu_obj_size[1] * 8 );
#endif
			break;
		case OAMADDL:	/* Address for accessing OAM (low) */
			oam_address_l = data;
			snes_ram[OAMDATA] = 0;
			break;
		case OAMADDH:	/* Address for accessing OAM (high) */
			/* FIXME: We don't yet support high priority( bit 7) */
			oam_address_h = data;
			snes_ram[OAMDATA] = 0;
			break;
		case OAMDATA:	/* Data for OAM write (DW) */
			{
				UINT16 oam_address = ((snes_ram[OAMADDH] & 0x1) << 8) + snes_ram[OAMADDL];
				DOUBLE_WRITE(snes_oam[oam_address], data );
				snes_ram[OAMDATA]++;
				if( snes_ram[OAMDATA] == 2 )
				{
					snes_ram[OAMDATA] = 0;
					oam_address++;
					snes_ram[OAMADDL] = oam_address & 0xff;
					snes_ram[OAMADDH] = (oam_address >> 8) & 0x1;	/* FIXME: we are splatting the priority bit here */
				}
				return;
			}
		case BGMODE:	/* BG mode and character size settings */
#ifdef V_REGISTERS
			if( ((data & 0xf0) != (snes_ram[BGMODE] & 0xf0)) )
				printf( "BG tile sizes: 4:%d 3:%d 2:%d 1:%d\n", data & 0x80, data & 0x40, data & 0x20, data & 0x10 );
			if( ((data & 0x7) != (snes_ram[BGMODE] & 0x7)) )
				printf( "BG mode: %d\n", data & 0x7  );
#endif
		case MOSAIC:	/* Size and screen designation for mosaic */
			/* FIXME: We don't support horizontal mosaic yet */
		case BG1SC:		/* Address for storing SC data BG1 SC size designation */
		case BG2SC:		/* Address for storing SC data BG2 SC size designation  */
		case BG3SC:		/* Address for storing SC data BG3 SC size designation  */
		case BG4SC:		/* Address for storing SC data BG4 SC size designation  */
		case BG12NBA:	/* Address for BG 1 and 2 character data */
		case BG34NBA:	/* Address for BG 3 and 4 character data */
			break;
		case BG1HOFS:	/* BG1 - horizontal scroll (DW) */
			DOUBLE_WRITE( bg_hoffset[0], data )
			return;
		case BG1VOFS:	/* BG1 - vertical scroll (DW) */
			DOUBLE_WRITE( bg_voffset[0], data )
			return;
		case BG2HOFS:	/* BG2 - horizontal scroll (DW) */
			DOUBLE_WRITE( bg_hoffset[1], data )
			return;
		case BG2VOFS:	/* BG2 - vertical scroll (DW) */
			DOUBLE_WRITE( bg_voffset[1], data )
			return;
		case BG3HOFS:	/* BG3 - horizontal scroll (DW) */
			DOUBLE_WRITE( bg_hoffset[2], data )
			return;
		case BG3VOFS:	/* BG3 - vertical scroll (DW) */
			DOUBLE_WRITE( bg_voffset[2], data )
			return;
		case BG4HOFS:	/* BG4 - horizontal scroll (DW) */
			DOUBLE_WRITE( bg_hoffset[3], data )
			return;
		case BG4VOFS:	/* BG4 - vertical scroll (DW) */
			DOUBLE_WRITE( bg_voffset[3], data )
			return;
		case VMAIN:		/* VRAM address increment value designation */
			/* FIXME: We don't support gull graphic properly yet */
#ifdef V_REGISTERS
			if( (data & 0xc) && (data & 0xc) != (snes_ram[VMAIN] & 0xc) )
				printf( "Full graphic specified: %d\n", (data & 0xc) >> 2 );
#endif
			break;
		case VMADDL:	/* Address for VRAM read/write (low) */
		case VMADDH:	/* Address for VRAM read/write (high) */
			break;
		case VMDATAL:	/* Data for VRAM write (low) */
			{
				UINT16 addr = (snes_ram[VMADDH] << 8) | snes_ram[VMADDL];

				snes_vram[addr << 1] = data;
				if( !(snes_ram[VMAIN] & 0x80) )
				{
					switch( snes_ram[VMAIN] & 0x03 )
					{
						case 0: addr++;      break;
						case 1: addr += 32;  break;
						case 2: addr += 128; break; /* Should be 64, but a bug in the snes means it's 128 */
						case 3: addr += 128; break;
					}
					snes_ram[VMADDL] = addr & 0xff;
					snes_ram[VMADDH] = (addr >> 8) & 0xff;
				}
			} return;
		case VMDATAH:	/* Data for VRAM write (high) */
			{
				UINT16 addr = (snes_ram[VMADDH] << 8) | snes_ram[VMADDL];

				snes_vram[(addr << 1) + 1] = data;
				if( (snes_ram[VMAIN] & 0x80) )
				{
					switch( snes_ram[VMAIN] & 0x03 )
					{
						case 0: addr++;      break;
						case 1: addr += 32;  break;
						case 2: addr += 128; break; /* Should be 64, but a bug in the snes means it's 128 */
						case 3: addr += 128; break;
					}
					snes_ram[VMADDL] = addr & 0xff;
					snes_ram[VMADDH] = (addr >> 8) & 0xff;
				}
			} return;
		case M7SEL:		/* Mode 7 initial settings */
			/* FIXME: We don't support flipping yet */
			break;
		case M7A:		/* Mode 7 COS angle/x expansion (DW) */
		case M7B:		/* Mode 7 SIN angle/ x expansion (DW) */
		case M7C:		/* Mode 7 SIN angle/y expansion (DW) */
		case M7D:		/* Mode 7 COS angle/y expansion (DW) */
		case M7X:		/* Mode 7 x center position (DW) */
		case M7Y:		/* Mode 7 y center position (DW) */
			DOUBLE_WRITE( mode7_data[offset - M7A], data )
			break;
		case CGADD:		/* Initial address for colour RAM writing */
			/* CGRAM is 16-bit, but when reading/writing we treat it as
			 * 8-bit, so we need to double the address */
			cgram_address = data << 1;
			break;
		case CGDATA:	/* Data for colour RAM */
			((UINT8 *)snes_cgram)[cgram_address++] = data;
			if( cgram_address >= 0x202 )
				cgram_address = 0;
			break;
		case W12SEL:	/* Window mask settings for BG1-2 */
		case W34SEL:	/* Window mask settings for BG3-4 */
		case WOBJSEL:	/* Window mask settings for objects */
		case WH0:		/* Window 1 left position */
		case WH1:		/* Window 1 right position */
		case WH2:		/* Window 2 left position */
		case WH3:		/* Window 2 right position */
		case WBGLOG:	/* Window mask logic for BG's */
		case WOBJLOG:	/* Window mask logic for objects */
		case TM:		/* Main screen designation */
		case TS:		/* Subscreen designation */
		case TMW:		/* Window mask for main screen designation */
		case TSW:		/* Window mask for subscreen designation */
			break;
		case CGWSEL:	/* Initial settings for Fixed colour addition or screen addition */
			/* FIXME: We don't support direct select(bit 0), main/sub stuff, or
			 * whether add/sub is to fixed colour or sub screen */
#ifdef V_REGISTERS
			if( (data & 0x2) != (snes_ram[CGWSEL] & 0x2) )
				printf( "Add/Sub Layer: %s\n", ((data & 0x2) >> 1) ? "Subscreen" : "Fixed colour" );
#endif
		case CGADSUB:	/* Addition/Subtraction designation for each screen */
			break;
		case COLDATA:	/* Fixed colour data for fixed colour addition/subtraction */
			{
				/* Store it in the extra space we made in the CGRAM
				 * It doesn't really go there, but it's as good a place as any. */
				UINT8 r,g,b;

				/* Get existing value.  Stored as BGR, why do Nintendo do this? */
				b = snes_cgram[FIXED_COLOUR] & 0x1f;
				g = (snes_cgram[FIXED_COLOUR] & 0x3e0) >> 5;
				r = (snes_cgram[FIXED_COLOUR] & 0x7c00) >> 10;
				/* Set new value */
				if( data & 0x20 )
					r = data & 0x1f;
				if( data & 0x40 )
					g = data & 0x1f;
				if( data & 0x80 )
					b = data & 0x1f;
				snes_cgram[FIXED_COLOUR] = (r | (g << 5) | (b << 10));
				Machine->remapped_colortable[FIXED_COLOUR] = snes_cgram[FIXED_COLOUR];
			} break;
		case SETINI:	/* Screen mode/video select */
			/* FIXME: Don't support anything here */
#ifdef V_REGISTERS
			if( (data & 0x8) != (snes_ram[SETINI] & 0x8) )
				printf( "Pseudo 512 mode: %s\n", (data & 0x8) ? "on" : "off" );
#endif
			break;
		case APU00:
		case APU01:
		case APU02:
		case APU03:
#ifndef USE_SPCSKIPPER
			spc_port_in[offset & 0x3] = data;
#else
			snes_fakeapu_w_port( offset & 0x3, data );
#endif /* USE_SPCSKIPPER */
			return;
		case WMDATA:	/* Data to write to WRAM */
			{
				UINT32 addr = ((snes_ram[WMADDH] & 0x1) << 16) | (snes_ram[WMADDM] << 8) | snes_ram[WMADDL];

				cpu_writemem24( 0x7e0000 + addr++, data );
				snes_ram[WMADDH] = (addr >> 16) & 0x1;
				snes_ram[WMADDM] = (addr >> 8) & 0xff;
				snes_ram[WMADDL] = addr & 0xff;
				return;
			} break;
		case WMADDL:	/* Address to read/write to wram (low) */
		case WMADDM:	/* Address to read/write to wram (mid) */
		case WMADDH:	/* Address to read/write to wram (high) */
			break;
		case OLDJOY1:	/* Old NES joystick support */
			joypad_oldrol = 0;
			return;
		case NMITIMEN:	/* Flag for v-blank, timer int. and joy read */
		case WRIO:		/* Programmable I/O port */
		case WRMPYA:	/* Multiplier A */
			break;
		case WRMPYB:	/* Multiplier B */
			{
				UINT16 c = snes_ram[WRMPYA] * data;
				snes_ram[RDMPYL] = c & 0xff;
				snes_ram[RDMPYH] = (c >> 8) & 0xff;
			} break;
		case WRDIVL:	/* Dividend (low) */
		case WRDIVH:	/* Dividend (high) */
			break;
		case WRDVDD:	/* Divisor */
			{
				UINT16 value, dividend, remainder;
				dividend = remainder = 0;
				if( data > 0 )
				{
					value = (snes_ram[WRDIVH] << 8) + snes_ram[WRDIVL];
					dividend = value / data;
					remainder = value % data;
				}
				snes_ram[RDDIVL] = dividend & 0xff;
				snes_ram[RDDIVH] = (dividend >> 8) & 0xff;
				snes_ram[RDMPYL] = remainder & 0xff;
				snes_ram[RDMPYH] = (remainder >> 8) & 0xff;
			} break;
		case HTIMEL:	/* H-Count timer settings (low)  */
		case HTIMEH:	/* H-Count timer settings (high) */
		case VTIMEL:	/* V-Count timer settings (low)  */
		case VTIMEH:	/* V-Count timer settings (high) */
			break;
		case MDMAEN:	/* GDMA channel designation and trigger */
			snes_gdma( data );
			data = 0;	/* Once DMA is done we need to reset all bits to 0 */
			break;
		case HDMAEN:	/* HDMA channel designation */
			break;
		case MEMSEL:	/* Access cycle designation in memory (2) area */
			/* FIXME: Need to adjust the speed only during access of banks 0x80+
			 * Currently we are just increasing it no matter what */
			timer_set_overclock( 0, (data & 0x1) ? 1.335820896 : 1.0 );
#ifdef V_REGISTERS
			if( (data & 0x1) != (snes_ram[MEMSEL] & 0x1) )
				printf( "CPU speed: %f Mhz\n", (data & 0x1) ? 3.58 : 2.68 );
#endif
			break;
	/* Following are read-only */
		case HVBJOY:	/* H/V blank and joypad enable */
		case MPYL:		/* Multiplication result (low) */
		case MPYM:		/* Multiplication result (mid) */
		case MPYH:		/* Multiplication result (high) */
		case RDIO:
		case RDDIVL:
		case RDDIVH:
		case RDMPYL:
		case RDMPYH:
		case JOY1L:
		case JOY1H:
		case JOY2L:
		case JOY2H:
		case JOY3L:
		case JOY3H:
		case JOY4L:
		case JOY4H:
#ifdef MAME_DEBUG
			logerror( "Write to read-only register: %X value: %X", offset, data );
#endif /* MAME_DEBUG */
			return;
	/* Below is all DMA related */
		case 0x4300:
		case 0x4301:
		case 0x4302:
		case 0x4303:
		case 0x4304:
		case 0x4305:
		case 0x4306:
		case 0x4307:
		case 0x4308:
		case 0x4309:
		case 0x430A:
		case 0x4310:
		case 0x4311:
		case 0x4312:
		case 0x4313:
		case 0x4314:
		case 0x4315:
		case 0x4316:
		case 0x4317:
		case 0x4318:
		case 0x4319:
		case 0x431A:
		case 0x4320:
		case 0x4321:
		case 0x4322:
		case 0x4323:
		case 0x4324:
		case 0x4325:
		case 0x4326:
		case 0x4327:
		case 0x4328:
		case 0x4329:
		case 0x432A:
		case 0x4330:
		case 0x4331:
		case 0x4332:
		case 0x4333:
		case 0x4334:
		case 0x4335:
		case 0x4336:
		case 0x4337:
		case 0x4338:
		case 0x4339:
		case 0x433A:
		case 0x4340:
		case 0x4341:
		case 0x4342:
		case 0x4343:
		case 0x4344:
		case 0x4345:
		case 0x4346:
		case 0x4347:
		case 0x4348:
		case 0x4349:
		case 0x434A:
		case 0x4350:
		case 0x4351:
		case 0x4352:
		case 0x4353:
		case 0x4354:
		case 0x4355:
		case 0x4356:
		case 0x4357:
		case 0x4358:
		case 0x4359:
		case 0x435A:
		case 0x4360:
		case 0x4361:
		case 0x4362:
		case 0x4363:
		case 0x4364:
		case 0x4365:
		case 0x4366:
		case 0x4367:
		case 0x4368:
		case 0x4369:
		case 0x436A:
		case 0x4370:
		case 0x4371:
		case 0x4372:
		case 0x4373:
		case 0x4374:
		case 0x4375:
		case 0x4376:
		case 0x4377:
		case 0x4378:
		case 0x4379:
		case 0x437A:
			break;
		default:
#ifdef V_REGISTERS
/*			printf( "IO write to %X: %X\n", offset, data ); */
#endif
			break;
	}

	snes_ram[offset] = data;
}

/* In this function, we check everything is in a valid range and return how
 * 'valid' this section is as an information block.
 * If it fails at least 3 checks then we'll assume it's not valid */
int snes_validate_infoblock( UINT8 *infoblock, UINT16 offset )
{
	INT8 valid = 3;

	/* Check the CRC and inverse CRC */
	if( ((infoblock[offset + 0x1c] + (infoblock[offset + 0x1d] << 8)) |
		(infoblock[offset + 0x1e] + (infoblock[offset + 0x1f] << 8))) != 0xffff )
	{
		valid--;
	}
	/* Check the ROM Size */
	if( infoblock[offset + 0x17] > 13 )
	{
		valid--;
	}
	/* Check the SRAM size */
	if( infoblock[offset + 0x18] > 8 )
	{
		valid--;
	}
	/* Check the Country */
	if( infoblock[offset + 0x19] > 13 )
	{
		valid--;
	}

	if( valid < 0 )
	{
		valid = 0;
	}

	return valid;
}

int snes_load_rom(int id, void *file, int open_mode)
{
	int i;
	UINT16 totalblocks, readblocks;
	UINT32 offset;
	UINT8 header[512], sample[0xffff];
	UINT8 valid_mode20, valid_mode21;

#ifdef V_GENERAL
	/* Cart types */
	static struct
	{
		UINT8 Code;
		char *Name;
	}
	CartTypes[] =
	{
		{  0, "ROM ONLY"       },
		{  1, "ROM+RAM"        },
		{  2, "ROM+SRAM"       },
		{  3, "ROM+DSP1"       },
		{  4, "ROM+RAM+DSP1"   },
		{  5, "ROM+SRAM+DSP1"  },
		{ 19, "ROM+SuperFX"    },
		{ 21, "ROM+RAM+SuperFX"},
		{227, "ROM+RAM+GBData" },
		{246, "ROM+DSP2"       }
	};

	/* Some known countries */
	const char *countries[] =
	{
		"Japan (NTSC)",
		"USA (NTSC)",
		"Australia, Europe, Oceania & Asia (PAL)",
		"Sweden (PAL)",
		"Finland (PAL)",
		"Denmark (PAL)",
		"France (PAL)",
		"Holland (PAL)",
		"Spain (PAL)",
		"Germany, Austria & Switzerland (PAL)",
		"Italy (PAL)",
		"Hong Kong & China (PAL)",
		"Indonesia (PAL)",
		"Korea (PAL)"
	};
#endif	/* V_GENERAL */

	if (file == NULL)
	{
		printf("Cartridge name required!\n");
		return INIT_FAIL;
	}

	if( new_memory_region(REGION_CPU1, 0x1000000,0) )
	{
		logerror("Memory allocation failed reading rom!\n");
		return INIT_FAIL;
	}

	snes_ram = memory_region( REGION_CPU1 );
	memset( snes_ram, 0, 0x1000000 );


	/* Check for a header (512 bytes) */
	offset = 512;
	osd_fread( file, header, 512 );
	if( (header[8] == 0xaa) && (header[9] == 0xbb) && (header[10] == 0x04) )
	{
		/* Found an SWC identifier */
		logerror( "Found header(SWC) - Skipped\n" );
	}
	else if( (header[0] | (header[1] << 8)) == (((osd_fsize(file) - 512) / 1024) / 8) )
	{
		/* Some headers have the rom size at the start, if this matches with the
		 * actual rom size, we probably have a header */
		logerror( "Found header(size) - Skipped\n" );
	}
	else if( (image_length( IO_CARTSLOT, id ) % 0x8000) == 512 )
	{
		/* As a last check we'll see if there's exactly 512 bytes extra to this
		 * image. */
		logerror( "Found header(extra) - Skipped\n" );
	}
	else
	{
		/* No header found so go back to the start of the file */
		logerror( "No header found.\n" );
		offset = 0;
		osd_fseek( file, offset, SEEK_SET );
	}

	/* We need to take a sample of 128kb to test what mode we need to be in */
	osd_fread( file, sample, 0xffff );
	osd_fseek( file, offset, SEEK_SET );	/* Rewind */
	/* Now to determine if this is a lo-ROM or a hi-ROM */
	valid_mode20 = snes_validate_infoblock( sample, 0x7fc0 );
	valid_mode21 = snes_validate_infoblock( sample, 0xffc0 );
	if( valid_mode20 > valid_mode21 )
	{
		logerror( "This appears to be a LO-ROM\n" );
		rom_mode = MODE_20;
	}
	else if( valid_mode21 > valid_mode20 )
	{
		logerror( "This appears to be a HI-ROM\n" );
		rom_mode = MODE_21;
	}
	else
	{
		logerror( "I don't know what type of ROM this is - assuming LO-ROM!\n" );
		rom_mode = MODE_20;
	}

	/* Find the amount of sram */
	sram_size = ((1 << (snes_ram[0xffd8] + 3)) / 8) * 1024;

	/* Find the number of blocks in this ROM */
	totalblocks = ((osd_fsize(file) - offset) >> (rom_mode == MODE_20 ? 15 : 16));

	/* FIXME: Insert crc check here */

	readblocks = 0;
	if( rom_mode == MODE_20 )
	{
		/* In mode 20, all blocks are 32kb. There are upto 96 blocks, giving a
		 * total of 24mbit(3mb) of ROM.
		 * The first 48 blocks are located in banks 0x00 to 0x2f at the address
		 * 0x8000.  They are mirrored in banks 0x80 to 0xaf.
		 * The next 16 blocks are located in banks 0x30 to 0x3f at address
		 * 0x8000.  They are mirrored in banks 0xb0 to 0xbf.
		 * The final 32 blocks are located in banks 0x40 - 0x5f at address
		 * 0x8000.  They are mirrord in banks 0xc0 to 0xdf.
		 */

		/* can probably do the following all in one shot */
		/* read first 48 blocks */
		i = 0;
		while( i < 96 && readblocks <= totalblocks )
		{
			osd_fread( file, &snes_ram[(i++ * 0x10000) + 0x8000], 0x8000);
			readblocks++;
		}
	}
	else	/* Mode 21 */
	{
		/* In mode 21, all blocks are 64kb. There are upto 96 blocks, giving a
		 * total of 48mbit(6mb) of ROM.
		 * The first 64 blocks are located in banks 0xc0 to 0xff. The MSB of
		 * each bank is mirrored in banks 0x00 to 0x3f.
		 * The final 48 blocks are located in banks 0x40 to 0x5f.
		 */

		/* read first 64 blocks */
		i = 0;
		while( i < 64 && readblocks <= totalblocks )
		{
			osd_fread( file, &snes_ram[0xc00000 + (i++ * 0x10000)], 0x10000);
			readblocks++;
		}
		/* read the next 48 blocks */
		i = 0;
		while( i < 48 && readblocks <= totalblocks )
		{
			osd_fread( file, &snes_ram[0x400000 + (i++ * 0x10000)], 0x10000);
			readblocks++;
		}
	}

#ifdef V_GENERAL
	{
		char title[21];
		printf( "ROM DETAILS\n" );
		printf( "\tHeader found:  %s\n", offset ? "Yes" : "No" );
		printf( "\tTotal blocks:  %d(%dmb)\n", totalblocks, totalblocks / (rom_mode == MODE_20 ? 32 : 16) );
		printf( "\tROM bank size: %s\n", (rom_mode == MODE_20) ? "LoROM" : "HiROM" );
		printf( "CART DETAILS\n" );
		memcpy(title, &snes_ram[0xffc0], 21);
		printf( "\tName:          %s\n", title );
		printf( "\tSpeed:         %s[%d]\n", ((snes_ram[0xffd5] & 0xf0)) ? "FastROM" : "SlowROM", (snes_ram[0xffd5] & 0xf0) >> 4 );
		printf( "\tBank size:     %s[%d]\n", (snes_ram[0xffd5] & 0xf) ? "HiROM" : "LoROM", snes_ram[0xffd5] & 0xf );
		printf( "\tType:          %s[%d]\n", "", snes_ram[0xffd6] );
		printf( "\tSize:          %d megabits\n", 1 << (snes_ram[0xffd7] - 7) );
		printf( "\tSRAM:          %d kilobytes\n", sram_size );
		if( snes_ram[0xffd9] <= 13 )
			printf( "\tCountry:       %s\n", countries[snes_ram[0xffd9]] );
		else
			printf( "\tCountry:       %d (Unknown)\n", snes_ram[0xffd9] );
		printf( "\tLicense:       %s[%d]\n", "", snes_ram[0xffda] );
		printf( "\tVersion:       1.%d\n", snes_ram[0xffdb] );
		printf( "\tInv Checksum:  %X %X\n", snes_ram[0xffdd], snes_ram[0xffdc] );
		printf( "\tChecksum:      %X %X\n", snes_ram[0xffdf], snes_ram[0xffde] );
		printf( "\tNMI Address:   %2X%2Xh\n", snes_ram[0xfffb], snes_ram[0xfffa] );
		printf( "\tStart Address: %2X%2Xh\n", snes_ram[0xfffd], snes_ram[0xfffc] );
	}
#endif

	/* Load the save RAM */
	battery_load( image_filename(IO_CARTSLOT,id), &snes_ram[0x700000], sram_size );

	/* All done */
	return INIT_PASS;
}

void snes_scanline_interrupt(void)
{
	UINT8 max_lines = (snes_ram[SETINI] & 0x4) ? 240 : 225;

	/* Start of VBlank */
	if( cur_vline == max_lines )
	{
		if( snes_ram[NMITIMEN] & 0x80 )	/* NMI only signaled if this bit set */
		{
			cpu_set_irq_line( 0, G65816_LINE_NMI, HOLD_LINE );
		}
		snes_ram[RDNMI]  |= 0x80;		/* Set NMI occured bit */
		snes_ram[HVBJOY] |= 0x80;		/* Set vblank bit to on */
		snes_ram[STAT77] &= 0x3f;		/* Clear Time Over and Range Over bits - done every nmi (presumably because no sprites drawn here) */
		cpu_writemem24( OAMADDL, oam_address_l ); /* Reset oam address at vblank */
		cpu_writemem24( OAMADDH, oam_address_h );
	}

	/* Setup HDMA on start of new frame */
	if( cur_vline == 0 )
		snes_hdma_init();

	/* Let's draw the current line */
	if( cur_vline < max_lines )
	{
		snes_refresh_scanline( cur_vline );

		/* If HDMA is enabled, then do that first */
		if( snes_ram[HDMAEN] )
			snes_hdma();
	}

	/* Vertical IRQ timer */
	if( snes_ram[NMITIMEN] & 0x20 )
	{
		if( cur_vline == (((snes_ram[VTIMEH] << 8) | snes_ram[VTIMEL]) & 0x1ff) )
		{
			cpu_set_irq_line( 0, G65816_LINE_IRQ, HOLD_LINE );
			snes_ram[TIMEUP] = 0x80;	/* Indicate that irq occured */
		}
	}
	/* Horizontal IRQ timer */
	if( snes_ram[NMITIMEN] & 0x10 )
	{
		if( (((snes_ram[HTIMEH] << 8) | snes_ram[HTIMEL]) & 0x1ff) )
		{
			cpu_set_irq_line( 0, G65816_LINE_IRQ, HOLD_LINE );
			snes_ram[TIMEUP] = 0x80;	/* Indicate that irq occured */
		}
	}

	/* Increase current line */
	cur_vline = (cur_vline + 1) % 262;
	if( cur_vline == 0 )
	{
		snes_ram[HVBJOY] &= 0x7f;		/* Clear blanking bit */
		snes_ram[RDNMI]  &= 0x7f;		/* Clear nmi occured bit */
		cpu_set_irq_line( 0, G65816_LINE_NMI, CLEAR_LINE );
	}
}

void snes_hdma_init()
{
	UINT8 mask = 1, dma = 0, i;

	for( i = 0; i < 8; i++ )
	{
		if( snes_ram[HDMAEN] & mask )
		{
			snes_ram[DMA_BASE + dma + 8] = snes_ram[DMA_BASE + dma + 2];
			snes_ram[DMA_BASE + dma + 9] = snes_ram[DMA_BASE + dma + 3];
			snes_ram[DMA_BASE + dma + 0xa] = 0;
		}
		dma += 0x10;
		mask <<= 1;
	}
}

void snes_hdma()
{
	UINT8 mask = 1, dma = 0, i, contmode;
	UINT16 bbus;
	UINT32 abus;

	/* Assume priority of the 8 DMA channels is 0-7 */
	for( i = 0; i < 8; i++ )
	{
		if( snes_ram[HDMAEN] & mask )
		{
			/* Check if we need to read a new line from the table */
			if( !(snes_ram[DMA_BASE + dma + 0xa] & 0x7f ) )
			{
				abus = (snes_ram[DMA_BASE + dma + 4] << 16) + (snes_ram[DMA_BASE + dma + 9] << 8) + snes_ram[DMA_BASE + dma + 8];

				/* Get the number of lines */
				snes_ram[DMA_BASE + dma + 0xa] = cpu_readmem24(abus);
				if( !snes_ram[DMA_BASE + dma + 0xa] )
				{
					/* No more lines so clear HDMA */
					snes_ram[HDMAEN] &= ~mask;
					continue;
				}
				abus++;
				snes_ram[DMA_BASE + dma + 8] = abus & 0xff;
				snes_ram[DMA_BASE + dma + 9] = (abus >> 8) & 0xff;
				if( snes_ram[DMA_BASE + dma] & 0x40 )
				{
					snes_ram[DMA_BASE + dma + 5] = cpu_readmem24(abus++);
					snes_ram[DMA_BASE + dma + 6] = cpu_readmem24(abus++);
					snes_ram[DMA_BASE + dma + 8] = abus & 0xff;
					snes_ram[DMA_BASE + dma + 9] = (abus >> 8) & 0xff;
				}
			}

			contmode = (--snes_ram[DMA_BASE + dma + 0xa]) & 0x80;

			/* Transfer addresses */
			if( snes_ram[DMA_BASE + dma] & 0x40 )	/* Indirect */
				abus = (snes_ram[DMA_BASE + dma + 7] << 16) + (snes_ram[DMA_BASE + dma + 6] << 8) + snes_ram[DMA_BASE + dma + 5];
			else									/* Absolute */
				abus = (snes_ram[DMA_BASE + dma + 4] << 16) + (snes_ram[DMA_BASE + dma + 9] << 8) + snes_ram[DMA_BASE + dma + 8];
			bbus = 0x2100 + snes_ram[DMA_BASE + dma + 1];

#ifdef V_HDMA
			printf( "HDMA-Ch: %d(%s) abus: %X bbus: %X type: %d(%X %X)\n", i, snes_ram[DMA_BASE + dma] & 0x40 ? "Indirect" : "Absolute", abus, bbus, snes_ram[DMA_BASE + dma] & 0x7, snes_ram[DMA_BASE + dma + 8],snes_ram[DMA_BASE + dma + 9] );
#endif

			switch( snes_ram[DMA_BASE + dma] & 0x7 )
			{
				case 0:		/* 1 address */
				{
					cpu_writemem24( bbus, cpu_readmem24(abus++));
				} break;
				case 1:		/* 2 addresses (l,h) */
				{
					cpu_writemem24( bbus, cpu_readmem24(abus++));
					cpu_writemem24( bbus + 1, cpu_readmem24(abus++));
				} break;
				case 2:		/* Write twice (l,l) */
				{
					cpu_writemem24( bbus, cpu_readmem24(abus++));
					cpu_writemem24( bbus, cpu_readmem24(abus++));
				} break;
				case 3:		/* 2 addresses/Write twice (l,l,h,h) */
				{
					cpu_writemem24( bbus, cpu_readmem24(abus++));
					cpu_writemem24( bbus, cpu_readmem24(abus++));
					cpu_writemem24( bbus + 1, cpu_readmem24(abus++));
					cpu_writemem24( bbus + 1, cpu_readmem24(abus++));
				} break;
				case 4:		/* 4 addresses (l,h,l,h) */
				{
					cpu_writemem24( bbus, cpu_readmem24(abus++));
					cpu_writemem24( bbus + 1, cpu_readmem24(abus++));
					cpu_writemem24( bbus + 2, cpu_readmem24(abus++));
					cpu_writemem24( bbus + 3, cpu_readmem24(abus++));
				} break;
				default:
#ifdef V_HDMA
					printf( "  HDMA of unsupported type: %d\n", snes_ram[DMA_BASE + dma] & 0x7 );
#endif
					break;
			}

			/* Update address */
			if( contmode )
			{
				if( snes_ram[DMA_BASE + dma] & 0x40 )	/* Indirect */
				{
					snes_ram[DMA_BASE + dma + 5] = abus & 0xff;
					snes_ram[DMA_BASE + dma + 6] = (abus >> 8) & 0xff;
				}
				else									/* Absolute */
				{
					snes_ram[DMA_BASE + dma + 8] = abus & 0xff;
					snes_ram[DMA_BASE + dma + 9] = (abus >> 8) & 0xff;
				}
			}

			if( !(snes_ram[DMA_BASE + dma + 0xa] & 0x7f) )
			{
				if( !(snes_ram[DMA_BASE + dma] & 0x40) )	/* Absolute */
				{
					if( !contmode )
					{
						snes_ram[DMA_BASE + dma + 8] = abus & 0xff;
						snes_ram[DMA_BASE + dma + 9] = (abus >> 8) & 0xff;
					}
				}
			}
		}
		dma += 0x10;
		mask <<= 1;
	}
}

void snes_gdma( UINT8 channels )
{
	UINT8 mask = 1, dma = 0, i;
	UINT8 increment;
	UINT16 bbus;
	UINT32 abus, length;

	/* Assume priority of the 8 DMA channels is 0-7 */
	for( i = 0; i < 8; i++ )
	{
		if( channels & mask )
		{
			/* Find transfer addresses */
			abus = (snes_ram[DMA_BASE + dma + 4] << 16) + (snes_ram[DMA_BASE + dma + 3] << 8) + snes_ram[DMA_BASE + dma + 2];
			bbus = 0x2100 + snes_ram[DMA_BASE + dma + 1];

			/* Auto increment */
			if( snes_ram[DMA_BASE + dma] & 0x8 )
			{
				increment = 0;
			}
			else
			{
				if( snes_ram[DMA_BASE + dma] & 0x10 )
					increment = -1;
				else
					increment = 1;
			}

			/* Number of bytes to transfer */
			length = (snes_ram[DMA_BASE + dma + 6] << 8) + snes_ram[DMA_BASE + dma + 5];
			if( !length )
				length = 0x10000;	/* 0x0000 really means 0x10000 */

#ifdef V_GDMA
			printf( "GDMA-Ch %d: len: %X, abus: %X, bbus: %X, incr: %d, dir: %s, type: %d\n", i, length, abus, bbus, increment, snes_ram[DMA_BASE + dma] & 0x80 ? "PPU->CPU" : "CPU->PPU", snes_ram[DMA_BASE + dma] & 0x7 );
#endif

			switch( snes_ram[DMA_BASE + dma] & 0x7 )
			{
				case 0:		/* 1 address */
				case 2:		/* 1 address ?? */
				{
					while( length-- )
					{
						if( snes_ram[DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus, cpu_readmem24(abus) );
						abus += increment;
					}
				} break;
				case 1:		/* 2 addresses (l,h) */
				{
					while( length-- )
					{
						if( snes_ram[DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus, cpu_readmem24(abus) );
						abus += increment;
						if( !(length--) )
							break;
						if( snes_ram[DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus + 1) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus + 1, cpu_readmem24(abus) );
						abus += increment;
					}
				} break;
				case 3:		/* 2 addresses/write twice (l,l,h,h) */
				{
					while( length-- )
					{
						if( snes_ram[DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus, cpu_readmem24(abus) );
						abus += increment;
						if( !(length--) )
							break;
						if( snes_ram[DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus, cpu_readmem24(abus) );
						abus += increment;
						if( !(length--) )
							break;
						if( snes_ram[DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus + 1) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus + 1, cpu_readmem24(abus) );
						abus += increment;
						if( !(length--) )
							break;
						if( snes_ram[DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus + 1) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus + 1, cpu_readmem24(abus) );
						abus += increment;
					}
				} break;
				case 4:		/* 4 addresses (l,h,l,h) */
				{
					while( length-- )
					{
						if( snes_ram[DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus, cpu_readmem24(abus) );
						abus += increment;
						if( !(length--) )
							break;
						if( snes_ram[DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus + 1) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus + 1, cpu_readmem24(abus) );
						abus += increment;
						if( !(length--) )
							break;
						if( snes_ram[DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus + 2) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus + 2, cpu_readmem24(abus) );
						abus += increment;
						if( !(length--) )
							break;
						if( snes_ram[DMA_BASE + dma] & 0x80 )	/* PPU->CPU */
							cpu_writemem24( abus, cpu_readmem24(bbus + 3) );
						else									/* CPU->PPU */
							cpu_writemem24( bbus + 3, cpu_readmem24(abus) );
						abus += increment;
					}
				} break;
				default:
#ifdef V_GDMA
					printf( "  GDMA of unsupported type: %d\n", snes_ram[DMA_BASE + dma] & 0x7 );
#endif
					break;
			}
			/* We're done so write the new abus back to the registers */
			snes_ram[DMA_BASE + dma + 2] = abus & 0xff;
			snes_ram[DMA_BASE + dma + 3] = (abus >> 8) & 0xff;
			snes_ram[DMA_BASE + dma + 4] = (abus >> 16) & 0xff;
		}
		dma += 0x10;
		mask <<= 1;
	}
}
