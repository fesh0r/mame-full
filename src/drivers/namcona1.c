/***************************************************************************
Namco NA-1 / NA-2 System

NA-1 Games:
*	Bakuretsu Quiz Ma-Q Dai Bouken (uninitialized palette)
-	F/A
-	Super World Court (C354, C357)
-	Nettou! Gekitou! Quiztou!! (C354, C365 - both are 32pin)
-	Exbania (C350, C354)
-	Cosmo Gang the Puzzle (C356)
-	Tinkle Pit (C354, C367)
-	Emeraldia (C354, C358)

NA-2 Games:
*	Numan Athletics (NA-2) (hangs)
*	Knuckle Heads (NA-2) ("playable" but gfx are garbled)

*: non-working games

The board has a 28c16 EEPROM

No other CPUs on the board, but there are many custom chips.
Mother Board:
210 (28pin SOP)
C70 (80pin PQFP)
215 (176pin PQFP)
C218 (208pin PQFP)
219 (160pin PQFP)
Plus 1 or 2 custom chips on ROM board.

Notes:

-	NA-1 titles can run on NA-2.

-	Test mode for NA2 games includes an additional item: UART Test.

-	Quiz games use 1p button 1 to pick test, 2p button 1 to begin test,
	and 2p button 2 to exit. Because quiz games doesn't have joystick.

-	Almost all quiz games using JAMMA edge connector assigns
	button1 to up, button 2 to down, button 3 to left, button 4 to right.
	But Taito F2 quiz games assign button 3 to right and button 4 to left.

-	Sound isn't hooked up.  It appears to be the primary purpose of
	the MCU, in addition to input port services.

-	Video has ROZ feature; it is probably similar to Namco System 2.

-	Scroll registers and screen flipping aren't well understood.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"

extern FILE *namcona1_dump;

extern UINT8 *namcona1_workram;
extern UINT8 *namcona1_vreg;
extern UINT8 *namcona1_scroll;

extern WRITE_HANDLER( namcona1_videoram_w );
extern READ_HANDLER( namcona1_videoram_r );
extern READ_HANDLER( namcona1_gfxram_r );
extern WRITE_HANDLER( namcona1_gfxram_w );
extern READ_HANDLER( namcona1_paletteram_r );
extern WRITE_HANDLER( namcona1_paletteram_w );

extern void namcona1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern int namcona1_vh_start( void );
extern void namcona1_vh_stop( void );

/* for MCU simulation */
static UINT8 coin_count[4];
static UINT8 coin_state;

static int enable_interrupts;

/*************************************************************************/

#define NA1_NVRAM_SIZE (0x400)
static UINT8 *nvram;

static void namcosna1_nvram_handler(void *file,int read_or_write){
	if (read_or_write){
		osd_fwrite( file, nvram, NA1_NVRAM_SIZE );
	}
	else {
		if (file){
			osd_fread( file, nvram, NA1_NVRAM_SIZE );
		}
		else {
			/* this isn't perfect, but it prevents games from crashing when eprom is invalid */
			const UINT8 data[0x10] = {
				0x30,0x32,0x4f,0x63,0x74,0x39,0x32,0x52,0x45,0x56,0x49,0x53,0x49,0x4f,0x4e,0x35
			};
			memset( nvram, 0x00, NA1_NVRAM_SIZE );
			memcpy( nvram, data, 0x10 );
		}
	}
}

static READ_HANDLER( namcona1_nvram_r ){
	return nvram[offset/2];
}

static WRITE_HANDLER( namcona1_nvram_w ){
	int oldword = nvram[offset/2];
	int newword = COMBINE_WORD( oldword, data );
	nvram[offset/2] = newword&0xff;
}

/***************************************************************************/

#define NAMCONA1_COMMON_PORTS \
	PORT_START \
	PORT_DIPNAME( 0x01, 0x00, "DIP2 (Freeze)" ) \
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(	0x01, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x02, 0x00, "DIP1 (Test)" ) \
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(	0x02, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x20, 0x00, "Test" ) \
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(	0x20, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x40, 0x00, "SERVICE" ) \
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(	0x40, DEF_STR( On ) ) \
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE ) \
	\
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY ) \
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY ) \
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY ) \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY ) \
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) \
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 ) \
	\
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 ) \
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 ) \
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 ) \
	\
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 ) \
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER3 ) \
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 ) \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER3 ) \
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER3 ) \
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER3 ) \
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER3 ) \
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START3 ) \
	\
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 ) \
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER4 ) \
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER4 ) \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER4 ) \
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER4 ) \
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER4 ) \
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER4 ) \
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START4 ) \
	\
	PORT_START \
	PORT_BIT( 0xffff, IP_ACTIVE_HIGH, IPT_UNUSED /* analog0,1 */ ) \
	PORT_START \
	PORT_BIT( 0xffff, IP_ACTIVE_HIGH, IPT_UNUSED /* analog2,3 */ ) \
	PORT_START \
	PORT_BIT( 0xffff, IP_ACTIVE_HIGH, IPT_UNUSED /* analog4,5 */ ) \
	PORT_START \
	PORT_BIT( 0xffff, IP_ACTIVE_HIGH, IPT_UNUSED /* analog6,7 */ ) \
	PORT_START \
	PORT_BIT( 0xffff, IP_ACTIVE_HIGH, IPT_UNUSED /* encoder0,1 */ ) \
	\
	PORT_START \
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN ) \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 ) \
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 ) \
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN3 ) \
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN4 )

INPUT_PORTS_START( namcona1 ) /* generic */
	NAMCONA1_COMMON_PORTS

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* exit */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* select */
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( numanath )
	NAMCONA1_COMMON_PORTS

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* exit */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* select */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( cgangpzl )
	NAMCONA1_COMMON_PORTS

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* exit */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* select */
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/***************************************************************************/
/* MCU simulation */

static READ_HANDLER( namcona1_io1_r ){ /* 0xfc0..0xfd- */
/*
	int flag = 0;
	if( keyboard_pressed( KEYCODE_A ) ) flag |= 0x01;
	if( keyboard_pressed( KEYCODE_S ) ) flag |= 0x02;
	if( keyboard_pressed( KEYCODE_D ) ) flag |= 0x04;
	if( keyboard_pressed( KEYCODE_F ) ) flag |= 0x08;
	if( keyboard_pressed( KEYCODE_G ) ) flag |= 0x10;
	if( keyboard_pressed( KEYCODE_H ) ) flag |= 0x20;
	if( keyboard_pressed( KEYCODE_J ) ) flag |= 0x40;
	if( keyboard_pressed( KEYCODE_K ) ) flag |= 0x80;
*/
	switch( offset ){
	case 0x00: return readinputport(0x0); /* dipswitch */

	case 0x02:
	{
		UINT16 data = readinputport(0x1)<<8;
		if( readinputport(1)&0x80 ) data |= 0x80; /* TinklePit: P1 start */
		if( readinputport(2)&0x80 ) data |= 0x40; /* TinklePit: P2 start */
		if( readinputport(1)&0x20 ) data |= 0x20; /* TinklePit: throw */
		if( readinputport(1)&0x40 ) data |= 0x10; /* TinklePit: jump */
		return data;
	}
	case 0x04: return readinputport(0x2)<<8;	/* joy2 */
	case 0x06: return readinputport(0x3)<<8;	/* joy3 */
	case 0x08: return readinputport(0x4)<<8;	/* joy4 */

	/*	analog ports are polled during test mode, but I don't know of any
	**	games that actually use them */
	case 0x0a: return readinputport(0x5);		/* analog0,1 */
	case 0x0c: return readinputport(0x6);		/* analog2,3 */
	case 0x0e: return readinputport(0x7);		/* analog4,5 */
	case 0x10: return readinputport(0x8);		/* analog6,7 */

	/* what is "encoder?" is it spolled during test mode */
	case 0x12: return readinputport(0x9);		/* encoder0,1 */
	}

	{
		UINT8 new_state = readinputport(0xa);
		if( (new_state&0x8)&~(coin_state&0x8) ) coin_count[0]++;
		if( (new_state&0x4)&~(coin_state&0x4) ) coin_count[1]++;
		if( (new_state&0x2)&~(coin_state&0x2) ) coin_count[2]++;
		if( (new_state&0x1)&~(coin_state&0x1) ) coin_count[3]++;
		coin_state = new_state;
	}
	if( offset==0x14 ){
		return (coin_count[0]<<8)|coin_count[1];
	}
	else if( offset==0x16 ){
		return (coin_count[2]<<8)|coin_count[3];
	}

	return 0xffff;
}
static READ_HANDLER( namcona1_io2_r ){
	if( offset==2 ) return readinputport(0xb);
	return 0xffff;
}

static void write_version_info( void ){
	const UINT16 source[0x8] = { /* "NSA-BIOS ver1.13" */
		0x534e,0x2d41,0x4942,0x534f,0x7620,0x7265,0x2e31,0x3133
	};
	UINT16 *dest = (UINT16 *)namcona1_workram;
	int i;
	for( i=0; i<8; i++ ){
		dest[i] = source[i];
	}
}

static READ_HANDLER( mcu_busy_r ){
	return 0x0000;
}

static WRITE_HANDLER( mcu_interrupt_w ){
	UINT16 *source = (UINT16 *)memory_region( REGION_CPU1 );
	UINT16 cmd = source[0xf72/2]>>8;

	switch( cmd ){
	case 0x03:
		logerror( "MCU init\n" );
		/* save data from 0xf30..0xf6f */
		break;

	case 0x07:
		/* this command is used to detect Namco NA-2 hardware; without it,
		** NA-2 games (Knuckleheads, Numan Athletics) refuse to run. */
		logerror( "MCU get version\n" );
		write_version_info();
		break;
	}
}

/***************************************************************************/

enum {
	key_cgangpzl,
	key_emeralda,
	key_knckhead,
	key_bkrtmaq,
	key_exbania,
	key_quiztou,
	key_swcourt,
	key_tinklpit,
	key_numanath,
	key_fa
} custom_key;

static UINT16 key_data;

static READ_HANDLER( custom_key_r ){
	switch( custom_key ){
	case key_bkrtmaq:
		if( offset==0x4 ) return 0x015c;
		break;
	case key_fa:
		if( offset==0x4 ) return 0x015d;
		if( offset==0x8 ) return key_data++;
		break;
	case key_exbania:
		if( offset==0x4 ) return 0x015e;
		break;
	case key_cgangpzl:
		if( offset==0x2 ) return 0x0164;
		if( offset==0x4 ) return rand()&0xffff; /* ? */
		break;
	case key_swcourt:
		if( offset==0x2 ) return 0x0165;
		if( offset==0x4 ) return key_data++;
		break;
	case key_emeralda:
		if( offset==0x2 ) return 0x0166;
		if( offset==0x4 ) return key_data++;
		break;
	case key_numanath:
		if( offset==0x2 ) return 0x0167;
		if( offset==0x4 ) return key_data++;
		break;
	case key_knckhead:
		if( offset==0x2 ) return 0x0168;
		if( offset==0x4 ) return key_data++;
		break;
	case key_quiztou:
		if( offset==0x4 ) return 0x016d;
		break;
	case key_tinklpit:
		if( offset==0xe ) return 0x016f;
		if( offset==0x8 ) key_data = 0;
		if( offset==0x6 ){
			const UINT16 data[] = {
				0x0000,0x2000,0x2100,0x2104,0x0106,0x0007,0x4003,0x6021,
				0x61a0,0x31a4,0x9186,0x9047,0xc443,0x6471,0x6db0,0x39bc,
				0x9b8e,0x924f,0xc643,0x6471,0x6db0,0x19bc,0xba8e,0xb34b,
				0xe745,0x4576,0x0cb7,0x789b,0xdb29,0xc2ec,0x16e2,0xb491
			};
			return data[(key_data++)&0x1f];
		}
		break;
	default:
		return 0;
	}
//	logerror( "unmapped key read(0x%02x), pc=0x%08x)\n", offset, cpu_get_pc() );
	return rand()&0xffff;
}

static WRITE_HANDLER( custom_key_w ){
}

/***************************************************************/

static READ_HANDLER( namcona1_vreg_r ){
	return READ_WORD( &namcona1_vreg[offset] );
}
/*
	0001 0000 ???? 0001 0000 0000 GFXW ???? 	blit
	???? ???? ???? 0000 0000 eINT 001f 0001
	003f 003f IACK 0000 0000 0000 0000 0000
	---- ---- ---- ---- ---- ---- ---- ----
	---- ---- ---- ---- ---- ---- ---- ----
	---- ---- ---- ---- ---- ---- ---- ----
	---- ---- ---- ---- ---- ---- ---- ----
	---- ---- ---- ---- ---- ---- ---- ----
	0050 0170 0020 0100 0000 0000 0000 0001
	0000 0001 0002 0003 FLIP xxxx xxxx xxxx
	PRI  PRI  PRI  PRI	---- ---- 00c0 xxxx 	priority
	COLR COLR COLR COLR 0001 0004 0000 xxxx 	color
	???? ???? ???? ???? ???? ???? ???? xxxx 	ROZ
*/

static void namcona1_blit( void ){
	int bank = READ_WORD( &namcona1_vreg[0x0c] );
	int bytes_per_tile = (bank==2)?64:8;
	UINT32 pitch = 64*bytes_per_tile;

	UINT32 num_bytes = READ_WORD(&namcona1_vreg[0x16]);
	UINT32 source_baseaddr = 2*((READ_WORD(&namcona1_vreg[0x0e])<<16)+READ_WORD(&namcona1_vreg[0x10]));
	UINT32 dest_baseaddr = 2*((READ_WORD(&namcona1_vreg[0x12])<<16)+READ_WORD(&namcona1_vreg[0x14]));

//	UINT16 param0 = READ_WORD(&namcona1_vreg[0x00]); /* 0 = continuous; 1 = block */
	UINT16 param1 = READ_WORD(&namcona1_vreg[0x02]);
//	UINT16 param2 = READ_WORD(&namcona1_vreg[0x04]);

//	UINT16 param3 = READ_WORD(&namcona1_vreg[0x06]);
//	UINT16 param4 = READ_WORD(&namcona1_vreg[0x08]);
//	UINT16 param5 = READ_WORD(&namcona1_vreg[0x0a]);

	int type;
	int num_cols = 0;
	if( bank==2 ){
		type = param1>>4;
	}
	else {
		type = param1>>1;
	}

/*
	if( bank==2 )
		//fprintf( namcona1_dump,
		logerror(
			"%08x %08x %04x %04x %04x %04x %04x %04x %04x\n",
			source_baseaddr, dest_baseaddr,
			num_bytes,
			param0,param1,param2,
			param3,param4,param5
		);
*/

	if( dest_baseaddr<0xf00000 ) dest_baseaddr += 0xf40000;

	switch( type ){
	case 0x7e: // 1110
		num_cols = 1;
		break;
	case 0x7c: // 1100
		num_cols = 2;
		break;
	case 0x78: // 1000
		num_cols = 4;
		break;
	case 0x00:
		num_cols = 64;
		num_bytes -= 2;
		break;
	default:
		return;
	}

	if( num_bytes&1 ){
		logerror( "odd num_bytes!\n" );
		num_bytes++;
	}

	{
		UINT32 words_per_row = num_cols*bytes_per_tile/2;
		while( num_bytes>0 ){
			UINT32 dest = dest_baseaddr;
			UINT32 source = source_baseaddr;
			int i;
			for( i=0; i<words_per_row; i++ ){
				UINT16 data = cpu_readmem32_word( source );
				cpu_writemem32_word( dest, data );

				source += 2;
				dest += 2;
				num_bytes -= 2;
				if( num_bytes==0 ) return;
			}
			source_baseaddr += pitch;
			dest_baseaddr += pitch;
		}
	}
}

static WRITE_HANDLER( namcona1_vreg_w ){
	int oldword = READ_WORD( &namcona1_vreg[offset] );
	int newword = COMBINE_WORD(oldword,data);
	WRITE_WORD(&namcona1_vreg[offset],newword);

	switch( offset ){
	case 0x00: case 0x02: case 0x04:	/* blit parameters */
	case 0x06: case 0x08: case 0x0a:	/* blit parameters (mirror) */
	case 0x0c:							/* blit target (shape/cgram/pal) */
	case 0x0e: case 0x10:				/* blit source */
	case 0x12: case 0x14:				/* blit dest */
	case 0x16:							/* blit num_bytes */
		break;
	case 0x18: namcona1_blit(); /* see also 0x1e */
		break;

	case 0x1a: enable_interrupts = 1; /* interrupt enable mask; 0 enables INT level */
		break;
	case 0x1c: break; /* interrupt disable? */

	case 0x1e: break; /* gfxram related? */

	case 0x20: /* unknown */
	case 0x22: /* sprite page select */
	case 0x24: /* IRQ ack? */

	case 0x80: // 0x0050
	case 0x82: // 0x0170
	case 0x84: // 0x0020
	case 0x86: // 0x0100

	case 0x88:
	case 0x8a:
	case 0x8c:

	case 0x8e: // spriteram disable?

	case 0x90: /* ?0x00 */
	case 0x92: /* ?0x01 */
	case 0x94: /* ?0x02 */
	case 0x96: /* ?0x03 */
	case 0x98: /* flipscreen */
	case 0x9a:

	case 0xa0: /* priority0 */
	case 0xa2: /* priority1 */
	case 0xa4: /* priority2 */
	case 0xa6: /* priority3 */
	case 0xa8:
	case 0xaa:
	case 0xac:

	case 0xb0: /* color0 */
	case 0xb2: /* color1 */
	case 0xb4: /* color2 */
	case 0xb6: /* color3 */
	case 0xb8:
	case 0xba:
	case 0xbc:

	case 0xc0: /* ROZ? */
	case 0xc2:
	case 0xc4:
	case 0xc6:
	case 0xc8:
	case 0xca:
	case 0xcc:
		break;

	default:
		logerror( "unknown vreg[0x%02x]:=0x%02x; cpu=0x%04x\n",
			offset, newword, cpu_get_pc() );
		break;
	}
}

/***************************************************************/

static struct MemoryReadAddress namcona1_readmem[] = {
	{ 0x000000, 0x00053f, MRA_RAM },
	{ 0x000f60, 0x000f61, mcu_busy_r },
	{ 0x000f72, 0x000f73, MRA_NOP },
	{ 0x000fc0, 0x000fd7, namcona1_io1_r },
	{ 0x000ffc, 0x000fff, namcona1_io2_r },
	{ 0x001000, 0x07ffff, MRA_RAM },	/* work RAM */
	{ 0x400000, 0xbfffff, MRA_BANK1 },	/* data */
	{ 0xc00000, 0xdfffff, MRA_BANK2 },	/* code */
	{ 0xe00000, 0xe007ff, namcona1_nvram_r },
	{ 0xe40000, 0xe4000f, custom_key_r },
	{ 0xefff00, 0xefffff, namcona1_vreg_r },
	{ 0xf00000, 0xf01fff, namcona1_paletteram_r },
	{ 0xf40000, 0xf7ffff, namcona1_gfxram_r },
	{ 0xff0000, 0xff7fff, namcona1_videoram_r },
	{ 0xffd000, 0xffdfff, MRA_BANK3 },
	{ 0xffe000, 0xffefff, MRA_BANK4 },	/* scroll registers */
	{ 0xfff000, 0xffffff, MRA_BANK5 },	/* spriteram */
	{ -1 }
};

static struct MemoryWriteAddress namcona1_writemem[] = {
	{ 0x000000, 0x00053f, MWA_RAM },
	{ 0x000820, 0x0008ff, MWA_RAM }, /* sound? */
	{ 0x000f30, 0x000f71, MWA_RAM }, /* sound? */
	{ 0x000f72, 0x000f73, MWA_RAM }, /* MCU command */
	{ 0x000fc0, 0x000fc9, MWA_NOP }, /* knuckleheads */
	{ 0x000fd8, 0x000fd9, MWA_RAM },
	{ 0x000fbe, 0x000fbf, MWA_NOP }, /* watchdog */
	{ 0x001000, 0x07ffff, MWA_RAM, &namcona1_workram },
	{ 0x3f8008, 0x3f8009, mcu_interrupt_w },
	{ 0x400000, 0xdfffff, MWA_ROM }, /* data + code */
	{ 0xe00000, 0xe007ff, namcona1_nvram_w, &nvram },
	{ 0xe40000, 0xe4000f, custom_key_w },
	{ 0xefff00, 0xefffff, namcona1_vreg_w, &namcona1_vreg },
	{ 0xf00000, 0xf01fff, namcona1_paletteram_w, &paletteram },
	{ 0xf40000, 0xf7ffff, namcona1_gfxram_w },
	{ 0xff0000, 0xff7fff, namcona1_videoram_w, &videoram },
	{ 0xff8000, 0xffcfff, MWA_NOP }, /* ? */
	{ 0xffd000, 0xffdfff, MWA_BANK3 },
	{ 0xffe000, 0xffefff, MWA_BANK4, &namcona1_scroll },
	{ 0xfff000, 0xffffff, MWA_BANK5, &spriteram },
	{ -1 }
};

int namcona1_interrupt( void ){
	if( enable_interrupts ){
//		UINT16 disable = READ_WORD( &namcona1_vreg[0x24] );
		UINT16 enable = READ_WORD( &namcona1_vreg[0x1a] );//|disable;
		int level = cpu_getiloops(); /* 0,1,2,3,4 */
		if( (enable&(1<<level))==0 ) return level+1;
	}
	return ignore_interrupt();
}

static struct MachineDriver machine_driver_namcona1 = {
	{
		{
			CPU_M68000,
			3*8000000, /* 8MHz? */
			namcona1_readmem,namcona1_writemem,0,0,
			namcona1_interrupt,5
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,
	/* video hardware */
	38*8, 28*8, { 0*8, 38*8-1, 0*8, 28*8-1 },
//	64*8, 32*8, { 0*8, 64*8-1, 0*8, 32*8-1 },
	0,/* gfxdecodeinfo */
	0x1000, 0x1000,
	0,
	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	namcona1_vh_start,
	namcona1_vh_stop,
	namcona1_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{0,0}  /* similar to C140?	managed by MCU */
	},
	namcosna1_nvram_handler
};

void init_namcona1( void ){
	UINT16 *dest = (UINT16 *)memory_region( REGION_CPU1 );
	dest[0] = 0x0007; /* stack */
	dest[1] = 0xfffc;

	dest[2] = 0x00c0; /* reset vector */
	dest[3] = 0x0000;

	cpu_setbank( 1, memory_region( REGION_CPU3 ) ); /* data */
	cpu_setbank( 2, memory_region( REGION_CPU2 ) ); /* code */

	coin_count[0] = coin_count[1] = coin_count[2] = coin_count[3] = 0;
	coin_state = 0;
	enable_interrupts = 0;
}

int cgang_hack( void ){
	return (custom_key == key_cgangpzl);
}

void init_cgangpzl( void ){
	init_namcona1();
	custom_key = key_cgangpzl;
}
void init_knckhead( void ){
	init_namcona1();
	custom_key = key_knckhead;
}
void init_emeralda( void ){
	init_namcona1();
	custom_key = key_emeralda;
}
void init_bkrtmaq( void ){
	init_namcona1();
	custom_key = key_bkrtmaq;
}
void init_exbania( void ){
	init_namcona1();
	custom_key = key_exbania;
}
void init_quiztou( void ){
	init_namcona1();
	custom_key = key_quiztou;
}
void init_swcourt( void ){
	init_namcona1();
	custom_key = key_swcourt;
}
void init_tinklpit( void ){
	init_namcona1();
	custom_key = key_tinklpit;
}
void init_numanath( void ){
	init_namcona1();
	custom_key = key_numanath;
}
void init_fa( void ){
	init_namcona1();
	custom_key = key_fa;
}

ROM_START( bkrtmaq )
	ROM_REGION( 0x80000, REGION_CPU1 ) /* working RAM */

	ROM_REGION( 0x200000, REGION_CPU2 ) /* 68000 code for CPU1 */
	ROM_LOAD_ODD(  "mq1_ep0l.bin", 0x000000, 0x080000, 0xf029bc57 )
	ROM_LOAD_EVEN( "mq1_ep0u.bin", 0x000000, 0x080000, 0x4cff62b8 )
	ROM_LOAD_ODD(  "mq1_ep1l.bin", 0x100000, 0x080000, 0xe3be6f4b )
	ROM_LOAD_EVEN( "mq1_ep1u.bin", 0x100000, 0x080000, 0xb44e31b2 )

	ROM_REGION( 0x400000, REGION_CPU3 ) /* data for CPU1 */
	ROM_LOAD_ODD(  "mq1_ma0l.bin", 0x000000, 0x100000, 0x11fed35f )
	ROM_LOAD_EVEN( "mq1_ma0u.bin", 0x000000, 0x100000, 0x23442ac0 )
	ROM_LOAD_ODD(  "mq1_ma1l.bin", 0x100000, 0x100000, 0xfe82205f )
	ROM_LOAD_EVEN( "mq1_ma1u.bin", 0x100000, 0x100000, 0x0cdb6bd0 )
ROM_END

ROM_START( cgangpzl )
	ROM_REGION( 0x80000, REGION_CPU1 ) /* working RAM */

	ROM_REGION( 0x200000, REGION_CPU2 ) /* 68000 code for CPU1 */
	ROM_LOAD_ODD(  "cp2-ep0l.bin", 0x000000, 0x80000, 0x2825f7ba )
	ROM_LOAD_EVEN( "cp2-ep0u.bin", 0x000000, 0x80000, 0x94d7d6fc )

	ROM_REGION( 0x10000, REGION_CPU3 ) /* fake */
ROM_END

ROM_START( emeralda )
	ROM_REGION( 0x80000, REGION_CPU1 ) /* working RAM */

	ROM_REGION( 0x200000, REGION_CPU2 ) /* 68000 code for CPU1 */
	ROM_LOAD_ODD(  "ep0lb", 0x000000, 0x80000, 0xfcd55293 )
	ROM_LOAD_EVEN( "ep0ub", 0x000000, 0x80000, 0xa52f00d5 )
	ROM_LOAD_ODD(  "ep1l",  0x100000, 0x80000, 0x373c1c59 )
	ROM_LOAD_EVEN( "ep1u",  0x100000, 0x80000, 0x4e969152 )

	ROM_REGION( 0x10000, REGION_CPU3 ) /* fake */
ROM_END

ROM_START( exbania )
	ROM_REGION( 0x80000, REGION_CPU1 ) /* working RAM */

	ROM_REGION( 0x200000, REGION_CPU2 ) /* 68000 code for CPU1 */
	ROM_LOAD_ODD(  "ex1-ep0l.bin", 0x000000, 0x080000, 0x18c12015 )
	ROM_LOAD_EVEN( "ex1-ep0u.bin", 0x000000, 0x080000, 0x07d054d1 )

	ROM_REGION( 0x400000, REGION_CPU3 ) /* data for CPU1 */
	ROM_LOAD_ODD(  "ex1-ma0l.bin", 0x000000, 0x100000, 0x17922cd4 )
	ROM_LOAD_EVEN( "ex1-ma0u.bin", 0x000000, 0x100000, 0x93d66106 )
	ROM_LOAD_ODD(  "ex1-ma1l.bin", 0x200000, 0x100000, 0xe4bba6ed )
	ROM_LOAD_EVEN( "ex1-ma1u.bin", 0x200000, 0x100000, 0x04e7c4b0 )
ROM_END

ROM_START( knckhead )
	ROM_REGION( 0x80000, REGION_CPU1 ) /* working RAM */

	ROM_REGION( 0x200000, REGION_CPU2 ) /* 68000 code for CPU1 */
	ROM_LOAD_ODD(  "kh1-ep0l.bin", 0x000000, 0x80000, 0x94660bec )
	ROM_LOAD_EVEN( "kh1-ep0u.bin", 0x000000, 0x80000, 0xad640d69 )
	ROM_LOAD_ODD(  "kh1-ep1l.bin", 0x100000, 0x80000, 0x27e6ab4e )
	ROM_LOAD_EVEN( "kh1-ep1u.bin", 0x100000, 0x80000, 0x487b2434 )

	ROM_REGION( 0x800000, REGION_CPU3 ) /* data for CPU1 */
	ROM_LOAD_ODD( "kh1-ma0l.bin", 0x000000, 0x100000, 0x7b2db5df )
	ROM_LOAD_EVEN("kh1-ma0u.bin", 0x000000, 0x100000, 0x6983228b )
	ROM_LOAD_ODD( "kh1-ma1l.bin", 0x200000, 0x100000, 0xb24f93e6 )
	ROM_LOAD_EVEN("kh1-ma1u.bin", 0x200000, 0x100000, 0x18a60348 )
	ROM_LOAD_ODD( "kh1-ma2l.bin", 0x400000, 0x100000, 0x82064ee9 )
	ROM_LOAD_EVEN("kh1-ma2u.bin", 0x400000, 0x100000, 0x17fe8c3d )
	ROM_LOAD_ODD( "kh1-ma3l.bin", 0x600000, 0x100000, 0xad9a7807 )
	ROM_LOAD_EVEN("kh1-ma3u.bin", 0x600000, 0x100000, 0xefeb768d )
ROM_END

ROM_START( numanath )
	ROM_REGION( 0x80000, REGION_CPU1 ) /* working RAM */

	ROM_REGION( 0x200000, REGION_CPU2 ) /* 68000 code for CPU1 */
	ROM_LOAD_ODD(  "nm1_ep0l.bin", 0x000000, 0x080000, 0x4398b898 )
	ROM_LOAD_EVEN( "nm1_ep0u.bin", 0x000000, 0x080000, 0xbe90aa79 )
	ROM_LOAD_ODD(  "nm1_ep1l.bin", 0x100000, 0x080000, 0x4581dcb4 )
	ROM_LOAD_EVEN( "nm1_ep1u.bin", 0x100000, 0x080000, 0x30cd589a )

	ROM_REGION( 0x800000, REGION_CPU3 ) /* data for CPU1 */
	ROM_LOAD_ODD(  "nm1_ma0l.bin", 0x000000, 0x100000, 0x20faaa57 )
	ROM_LOAD_EVEN( "nm1_ma0u.bin", 0x000000, 0x100000, 0xed7c37f2 )
	ROM_LOAD_ODD(  "nm1_ma1l.bin", 0x200000, 0x100000, 0x2232e3b4 )
	ROM_LOAD_EVEN( "nm1_ma1u.bin", 0x200000, 0x100000, 0x6cc9675c )
	ROM_LOAD_ODD(  "nm1_ma2l.bin", 0x400000, 0x100000, 0x208abb39 )
	ROM_LOAD_EVEN( "nm1_ma2u.bin", 0x400000, 0x100000, 0x03a3f204 )
	ROM_LOAD_ODD(  "nm1_ma3l.bin", 0x600000, 0x100000, 0x42a539e9 )
	ROM_LOAD_EVEN( "nm1_ma3u.bin", 0x600000, 0x100000, 0xf79e2112 )
ROM_END

ROM_START( quiztou )
	ROM_REGION( 0x80000, REGION_CPU1 ) /* working RAM */

	ROM_REGION( 0x200000, REGION_CPU2 ) /* 68000 code for CPU1 */
	ROM_LOAD_ODD(  "qt1_ep0l.bin", 0x000000, 0x080000, 0xb680e543 )
	ROM_LOAD_EVEN( "qt1_ep0u.bin", 0x000000, 0x080000, 0x143c5e4d )
	ROM_LOAD_ODD(  "qt1_ep1l.bin", 0x100000, 0x080000, 0x33a72242 )
	ROM_LOAD_EVEN( "qt1_ep1u.bin", 0x100000, 0x080000, 0x69f876cb )

	ROM_REGION( 0x800000, REGION_CPU3 ) /* data for CPU1 */
	ROM_LOAD_ODD(  "qt1_ma0l.bin", 0x000000, 0x100000, 0x5597f2b9 )
	ROM_LOAD_EVEN( "qt1_ma0u.bin", 0x000000, 0x100000, 0xf0a4cb7d )
	ROM_LOAD_ODD(  "qt1_ma1l.bin", 0x200000, 0x100000, 0x1b9ce7a6 )
	ROM_LOAD_EVEN( "qt1_ma1u.bin", 0x200000, 0x100000, 0x58910872 )
	ROM_LOAD_ODD(  "qt1_ma2l.bin", 0x400000, 0x100000, 0x94739917 )
	ROM_LOAD_EVEN( "qt1_ma2u.bin", 0x400000, 0x100000, 0x6ba5b893 )
	ROM_LOAD_ODD(  "qt1_ma3l.bin", 0x600000, 0x100000, 0xaa9dc6ff )
	ROM_LOAD_EVEN( "qt1_ma3u.bin", 0x600000, 0x100000, 0x14a5a163 )
ROM_END

ROM_START( swcourt )
	ROM_REGION( 0x80000, REGION_CPU1 ) /* working RAM */

	ROM_REGION( 0x200000, REGION_CPU2 ) /* 68000 code for CPU1 */
	ROM_LOAD_ODD(  "sc1-ep0l.bin", 0x000000, 0x080000, 0x145111dd )
	ROM_LOAD_EVEN( "sc1-ep0u.bin", 0x000000, 0x080000, 0xc721c138 )
	ROM_LOAD_ODD(  "sc1-ep1l.bin", 0x100000, 0x080000, 0xfb45cf5f )
	ROM_LOAD_EVEN( "sc1-ep1u.bin", 0x100000, 0x080000, 0x1ce07b15 )

	ROM_REGION( 0x400000, REGION_CPU3 ) /* data for CPU1 */
	ROM_LOAD_ODD(  "sc1-ma0l.bin", 0x000000, 0x100000, 0x3e531f5e )
	ROM_LOAD_EVEN( "sc1-ma0u.bin", 0x000000, 0x100000, 0x31e76a45 )
	ROM_LOAD_ODD(  "sc1-ma1l.bin", 0x200000, 0x100000, 0x8ba3a4ec )
	ROM_LOAD_EVEN( "sc1-ma1u.bin", 0x200000, 0x100000, 0x252dc4b7 )
ROM_END

ROM_START( tinklpit )
	ROM_REGION( 0x80000, REGION_CPU1 ) /* working RAM */

	ROM_REGION( 0x200000, REGION_CPU2 ) /* 68000 code for CPU1 */
	ROM_LOAD_ODD(  "tk1-ep0l.bin", 0x000000, 0x080000, 0xfdccae42 )
	ROM_LOAD_EVEN( "tk1-ep0u.bin", 0x000000, 0x080000, 0x62cdb48c )
	ROM_LOAD_ODD(  "tk1-ep1l.bin", 0x100000, 0x080000, 0x7e90f104 )
	ROM_LOAD_EVEN( "tk1-ep1u.bin", 0x100000, 0x080000, 0x9c0b70d6 )

	ROM_REGION( 0x600000, REGION_CPU3 ) /* data for CPU1 */
	ROM_LOAD_ODD(  "tk1-ma0l.bin", 0x000000, 0x100000, 0xc6b4e15d )
	ROM_LOAD_EVEN( "tk1-ma0u.bin", 0x000000, 0x100000, 0xa3ad6f67 )
	ROM_LOAD_ODD(  "tk1-ma1l.bin", 0x200000, 0x100000, 0x61cfb92a )
	ROM_LOAD_EVEN( "tk1-ma1u.bin", 0x200000, 0x100000, 0x54b77816 )
	ROM_LOAD_ODD(  "tk1-ma2l.bin", 0x400000, 0x100000, 0x087311d2 )
	ROM_LOAD_EVEN( "tk1-ma2u.bin", 0x400000, 0x100000, 0x5ce20c2c )
ROM_END

ROM_START( fa )
	ROM_REGION( 0x80000, REGION_CPU1 ) /* working RAM */

	ROM_REGION( 0x200000, REGION_CPU2 ) /* 68000 code for CPU1 */
	ROM_LOAD_ODD(  "fa1_ep0l.bin", 0x000000, 0x080000, 0x182eee5c )
	ROM_LOAD_EVEN( "fa1_ep0u.bin", 0x000000, 0x080000, 0x7ea7830e )
	ROM_LOAD_ODD(  "fa1_ep1l.bin", 0x100000, 0x080000, 0xb23a5b01 )
	ROM_LOAD_EVEN( "fa1_ep1u.bin", 0x100000, 0x080000, 0xde2eb129 )

	ROM_REGION( 0x400000, REGION_CPU3 ) /* data for CPU1 */
	ROM_LOAD_ODD(  "fa1_ma0l.bin", 0x000000, 0x100000, 0xa0a95e54 )
	ROM_LOAD_EVEN( "fa1_ma0u.bin", 0x000000, 0x100000, 0x1d0135bd )
	ROM_LOAD_ODD(  "fa1_ma1l.bin", 0x200000, 0x100000, 0xc4adf0a2 )
	ROM_LOAD_EVEN( "fa1_ma1u.bin", 0x200000, 0x100000, 0x900297be )
ROM_END

/*			rom   parent machine   inp		 init */
GAMEX( 1992,bkrtmaq,  0, namcona1, namcona1, bkrtmaq,  ROT0_16BIT, "Namco", "Bakuretsu Quiz Ma-Q Dai Bouken", GAME_NOT_WORKING )
GAMEX( 1992,cgangpzl, 0, namcona1, cgangpzl, cgangpzl, ROT0_16BIT, "Namco", "Cosmo Gang the Puzzle", GAME_NO_SOUND )
GAMEX( 1900,emeralda, 0, namcona1, namcona1, emeralda, ROT0_16BIT, "Namco", "Emeraldia", GAME_NO_SOUND )
GAMEX( 1900,exbania,  0, namcona1, namcona1, exbania,  ROT0_16BIT, "Namco", "Exbania", GAME_NO_SOUND )
GAMEX( 1900,knckhead, 0, namcona1, namcona1, knckhead, ROT0_16BIT, "Namco", "Knuckle Heads", GAME_NOT_WORKING )
GAMEX( 1993,numanath, 0, namcona1, numanath, numanath, ROT0_16BIT, "Namco", "Numan Athletics", GAME_NOT_WORKING )
GAMEX( 1993,quiztou,  0, namcona1, namcona1, quiztou,  ROT0_16BIT, "Namco", "Nettou! Gekitou! Quiztou!!", GAME_NO_SOUND )
GAMEX( 1993,swcourt,  0, namcona1, namcona1, swcourt,  ROT0_16BIT, "Namco", "Super World Court", GAME_NO_SOUND )
GAMEX( 1993,tinklpit, 0, namcona1, namcona1, tinklpit, ROT0_16BIT, "Namco", "Tinkle Pit", GAME_NO_SOUND )
GAMEX( 1992,fa, 	  0, namcona1, namcona1, fa,	   ROT0_16BIT/*ROT90_16BIT*/,"Namco", "F/A", GAME_NO_SOUND )

