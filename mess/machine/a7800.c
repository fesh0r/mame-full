/***************************************************************************

	a7800.c

	Machine file to handle emulation of the Atari 7800.

	 5-Nov-2003 npwoods		Cleanups

	14-May-2002	kubecj		Fixed Fatal Run - adding simple riot timer helped.
							maybe someone with knowledge should add full fledged
							riot emulation?

	13-May-2002 kubecj		Fixed a7800_cart_type not to be too short ;-D
							fixed for loading of bank6 cart (uh, I hope)
							fixed for loading 64k supercarts
							fixed for using PAL bios
							cart not needed when in PAL mode
							added F18 Hornet bank select type
							added Activision bank select type

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "sound/tiasound.h"
#include "cpuintrf.h"
#include "zlib.h"
#include "image.h"

#include "includes/a7800.h"

int a7800_lines;
int a7800_ispal;

/* local */
static unsigned char *a7800_cart_bkup = NULL;
static unsigned char *a7800_bios_bkup = NULL;
static int a7800_ctrl_lock;
static int a7800_ctrl_reg;
static int maria_flag;

static int riot_pa_ddr;
static int riot_pa_out;
static int riot_pb_ddr;
static int riot_pb_out;
static int riot_timer;

static unsigned char *a7800_cartridge_rom;
static unsigned int a7800_cart_type;
static unsigned long a7800_cart_size;
static unsigned char a7800_stick_type;
static UINT8 *ROM;



/* -----------------------------------------------------------------------
 * Driver/Machine Init
 * ----------------------------------------------------------------------- */

static void a7800_driver_init(int ispal, int lines)
{
	ROM = memory_region(REGION_CPU1);
	a7800_ispal = ispal;
	a7800_lines = lines;

	/* standard banks */
	cpu_setbank(5, &ROM[0x2040]);		/* RAM0 */
	cpu_setbank(6, &ROM[0x2140]);		/* RAM1 */
	cpu_setbank(7, &ROM[0x2000]);		/* MAINRAM */
}



DRIVER_INIT( a7800_ntsc )
{
	a7800_driver_init(FALSE, 262);
}



DRIVER_INIT( a7800_pal )
{
	a7800_driver_init(TRUE, 312);
}



MACHINE_INIT( a7800 )
{
	UINT8 *memory;

	a7800_ctrl_lock = 0;
	a7800_ctrl_reg = 0;
	maria_flag = 0;
	
	/* set banks to default states */
	memory = memory_region(REGION_CPU1);
	cpu_setbank( 1, memory + 0x4000 );
	cpu_setbank( 2, memory + 0x8000 );
	cpu_setbank( 3, memory + 0xA000 );
	cpu_setbank( 4, memory + 0xC000 );

	/* pokey cartridge */
	if (a7800_cart_type & 0x01)
		install_mem_write_handler(0, 0x4000, 0x7FFF, pokey1_w);
}



/* ----------------------------------------------------------------------- */

#define MBANK_TYPE_ATARI 0x0000
#define MBANK_TYPE_ACTIVISION 0x0100
#define MBANK_TYPE_ABSOLUTE 0x0200

/*	Header format
0	  Header version	 - 1 byte
1..16  "ATARI7800	  "  - 16 bytes
17..48 Cart title		 - 32 bytes
49..52 data length		- 4 bytes
53..54 cart type		  - 2 bytes
	bit 0 0x01 - pokey cart
	bit 1 0x02 - supercart bank switched
	bit 2 0x04 - supercart RAM at $4000
	bit 3 0x08 - additional ROM at $4000

	bit 8-15 - Special
		0 = Normal cart
		1 = Absolute (F18 Hornet)
		2 = Activision

55	 controller 1 type  - 1 byte
56	 controller 2 type  - 1 byte
	0 = None
	1 = Joystick
	2 = Light Gun
57  0 = NTSC/1 = PAL

100..127 "ACTUAL CART DATA STARTS HERE" - 28 bytes

Versions:
	Version 0: Initial release
	Version 1: Added PAL/NTSC bit. Added Special cart byte.
			   Changed 53 bit 2, added bit 3

*/

UINT32 a7800_partialcrc(const unsigned char *buf, size_t size)
{
	UINT32 crc;
	if (size < 129)
		return 0;
	crc =(UINT32) crc32(0L, &buf[128], size-128);
	logerror("A7800 Partial CRC: %08lx %d [%s]\n", (long) crc, (int)size, &buf[1]);
	return crc;
}

static int a7800_verify_cart(char header[128])
{
	const char* tag = "ATARI7800";

	if( strncmp( tag, header + 1, 9 ) )
	{
		logerror("Not a valid A7800 image\n");
		return IMAGE_VERIFY_FAIL;
	}

	logerror("returning ID_OK\n");
	return IMAGE_VERIFY_PASS;
}

DEVICE_LOAD( a7800_cart )
{
	long len,start;
	unsigned char header[128];
	UINT8 *memory;

	memory = memory_region(REGION_CPU1);
	a7800_bios_bkup = NULL;
	a7800_cart_bkup = NULL;

	/* Allocate memory for BIOS bank switching */
	a7800_bios_bkup = (UINT8*) image_malloc(image, 0x4000);
	if (!a7800_bios_bkup)
	{
		logerror("Could not allocate ROM memory\n");
		return INIT_FAIL;
	}

	a7800_cart_bkup = (UINT8*) image_malloc(image,  0x4000);
	if (!a7800_cart_bkup)
	{
		logerror("Could not allocate ROM memory\n");
		return INIT_FAIL;
	}

	/* save the BIOS so we can switch it in and out */
	memcpy( a7800_bios_bkup, memory + 0xC000, 0x4000 );

	/* defaults for PAL bios without cart */
	a7800_cart_type = 0;
	a7800_stick_type = 1;

	/* Load and decode the header */
	mame_fread( file, header, 128 );

	/* Check the cart */
	if( a7800_verify_cart((char *)header) == IMAGE_VERIFY_FAIL)
		return INIT_FAIL;

	len =(header[49] << 24) |(header[50] << 16) |(header[51] << 8) | header[52];
	a7800_cart_size = len;

	a7800_cart_type =(header[53] << 8) | header[54];
	a7800_stick_type = header[55];
	logerror( "Cart type: %x\n", a7800_cart_type );

	/* For now, if game support stick and gun, set it to stick */
	if( a7800_stick_type == 3 )
		a7800_stick_type = 1;

	if( a7800_cart_type == 0 || a7800_cart_type == 1 )
	{
		/* Normal Cart */

		start = 0x10000 - len;
		a7800_cartridge_rom = memory + start;
		mame_fread(file, a7800_cartridge_rom, len);
	}
	else if( a7800_cart_type & 0x02 )
	{
		/* Super Cart */

		/* Extra ROM at $4000 */
		if( a7800_cart_type & 0x08 )
		{
			mame_fread(file, memory + 0x4000, 0x4000 );
			len -= 0x4000;
		}

		a7800_cartridge_rom = memory + 0x10000;
		mame_fread(file, a7800_cartridge_rom, len);

		/* bank 0 */
		memcpy( memory + 0x8000, memory + 0x10000, 0x4000);

		/* last bank */
		memcpy( memory + 0xC000, memory + 0x10000 + len - 0x4000, 0x4000);

		/* fixed 2002/05/13 kubecj
			there was 0x08, I added also two other cases.
			Now, it loads bank n-2 to $4000 if it's empty.
		*/

		/* bank n-2	*/
		if( ! ( a7800_cart_type & 0x0D ) )
		{
			memcpy( memory + 0x4000, memory + 0x10000 + len - 0x8000, 0x4000);
		}
	}
	else if( a7800_cart_type == MBANK_TYPE_ABSOLUTE )
	{
		/* F18 Hornet */

		logerror( "Cart type: %x Absolute\n",a7800_cart_type );

		a7800_cartridge_rom = memory + 0x10000;
		mame_fread(file, a7800_cartridge_rom, len );

		/* bank 0 */
		memcpy( memory + 0x4000, memory + 0x10000, 0x4000 );

		/* last bank */
		memcpy( memory + 0x8000, memory + 0x18000, 0x8000);
	}
	else if( a7800_cart_type == MBANK_TYPE_ACTIVISION )
	{
		/* Activision */

		logerror( "Cart type: %x Activision\n",a7800_cart_type );

		a7800_cartridge_rom = memory + 0x10000;
		mame_fread( file, a7800_cartridge_rom, len );

		/* bank 0 */
		memcpy( memory + 0xA000, memory + 0x10000, 0x4000 );

		/* bank6 hi */
		memcpy( memory + 0x4000, memory + 0x2A000, 0x2000 );

		/* bank6 lo */
		memcpy( memory + 0x6000, memory + 0x28000, 0x2000 );

		/* bank7 hi */
		memcpy( memory + 0x8000, memory + 0x2E000, 0x2000 );

		/* bank7 lo */
		memcpy( memory + 0xE000, memory + 0x2C000, 0x2000 );

	}

	memcpy( a7800_cart_bkup, memory + 0xC000, 0x4000 );
	memcpy( memory + 0xC000, a7800_bios_bkup, 0x4000 );
	return INIT_PASS;
}


/******  TIA  *****************************************/

READ_HANDLER( a7800_TIA_r )
{
	switch(offset)
	{
		case 0x08:
			  return((input_port_1_r(0) & 0x02) << 6);
		case 0x09:
			  return((input_port_1_r(0) & 0x08) << 4);
		case 0x0A:
			  return((input_port_1_r(0) & 0x01) << 7);
		case 0x0B:
			  return((input_port_1_r(0) & 0x04) << 5);
		case 0x0c:
			if((input_port_1_r(0) & 0x08) ||(input_port_1_r(0) & 0x02))
				return 0x00;
			else
				return 0x80;
		case 0x0d:
			if((input_port_1_r(0) & 0x01) ||(input_port_1_r(0) & 0x04))
				return 0x00;
			else
				return 0x80;
		default:
			logerror("undefined TIA read %x\n",offset);

	}
	return 0xFF;
}

WRITE_HANDLER( a7800_TIA_w )
{
	switch(offset) {
	case 0x01:
		if(data & 0x01)
		{
			maria_flag=1;
		}
		if(!a7800_ctrl_lock)
		{
			a7800_ctrl_lock = data & 0x01;
			a7800_ctrl_reg = data;

			if (data & 0x04)
				memcpy( ROM + 0xC000, a7800_cart_bkup, 0x4000 );
			else
				memcpy( ROM + 0xC000, a7800_bios_bkup, 0x4000 );
		}
		break;
	}
	tia_sound_w(offset,data);
	ROM[offset] = data;
}

/****** RIOT ****************************************/

READ_HANDLER( a7800_RIOT_r )
{
	unsigned char data;

	offset &= 0xF;

	switch( offset )
	{
		case 0:
			if( a7800_stick_type == 0x01 )
				data = input_port_0_r(0);
			else
				data = ((input_port_1_r(0) & 0x02) << 3);

			return ( data & ~riot_pa_ddr ) | ( riot_pa_out & riot_pa_ddr );

		case 2:
			return ( input_port_3_r(0) & ~riot_pb_ddr ) | ( riot_pb_out & riot_pb_ddr );

		case 4:
			return riot_timer--;

		default:
			logerror("undefined RIOT I/O read %x\n",offset);
	}

	return 0xFF;
}

WRITE_HANDLER( a7800_RIOT_w )
{
	switch( offset & 0x1F )
	{
		case 0:
			riot_pa_out = data;
			break;

		case 1:
			riot_pa_ddr = data;
			break;

		case 2:
			riot_pb_out = data;
			break;

		case 3:
			riot_pb_ddr = data;
			break;

		case 0x17:
			riot_timer = data;
			break;

		default:
			logerror("undefined RIOT write %x,%x\n", offset, data );
			break;
	}
}


/****** RAM Mirroring ******************************/

WRITE_HANDLER( a7800_RAM0_w )
{
	ROM[0x2040 + offset] = data;
	ROM[0x40 + offset] = data;
}

WRITE_HANDLER( a7800_cart_w )
{
	if(offset < 0x4000)
	{
		if(a7800_cart_type & 0x04)
		{
			ROM[0x4000 + offset] = data;
		}
		else if(a7800_cart_type & 0x01)
		{
			pokey1_w(offset,data);
		}
		else
		{
			logerror("Undefined write A: %x",offset + 0x4000);
		}
	}

	if(( a7800_cart_type & 0x02 ) &&( offset >= 0x4000 ) )
	{
		/* fix for 64kb supercart */
		if( a7800_cart_size == 0x10000 )
		{
			data &= 0x03;
		}
		else
		{
			data &= 0x07;
		}
		cpu_setbank(2,memory_region(REGION_CPU1) + 0x10000 + (data << 14));
		cpu_setbank(3,memory_region(REGION_CPU1) + 0x12000 + (data << 14));
	/*	logerror("BANK SEL: %d\n",data); */
	}
	else if(( a7800_cart_type == MBANK_TYPE_ABSOLUTE ) &&( offset == 0x4000 ) )
	{
		/* F18 Hornet */
		/*logerror( "F18 BANK SEL: %d\n", data );*/
		if( data & 1 )
		{
			cpu_setbank(1,memory_region(REGION_CPU1) + 0x10000 );
		}
		else if( data & 2 )
		{
			cpu_setbank(1,memory_region(REGION_CPU1) + 0x14000 );
		}
	}
	else if(( a7800_cart_type == MBANK_TYPE_ACTIVISION ) &&( offset >= 0xBF80 ) )
	{
		/* Activision */
		data = offset & 7;

		/*logerror( "Activision BANK SEL: %d\n", data );*/

		cpu_setbank( 3, memory_region( REGION_CPU1 ) + 0x10000 + ( data << 14 ) );
		cpu_setbank( 4, memory_region( REGION_CPU1 ) + 0x12000 + ( data << 14 ) );
	}
}


