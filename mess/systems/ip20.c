/*********************************************************************\
*
*   SGI IP20 IRIS Indigo workstation
*
*  Skeleton Driver
*
*  Todo: Everything
*
*  Note: Machine uses R4400, not R4600
*
*  Memory map:
*
*  1fa00000 - 1fa02047      Memory Controller
*  1fb80000 - 1fb9a7ff      HPC3 CHIP0
*  1fc00000 - 1fc7ffff      BIOS
*
\*********************************************************************/

#include "driver.h"
#include "cpu/mips/mips3.h"
#include "machine/8530scc.h"
#include "machine/sgi.h"
#include "machine/eeprom.h"

#define VERBOSE_LEVEL ( 2 )

INLINE void verboselog( int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%08x: %s", activecpu_get_pc(), buf );
	}
}

static VIDEO_START( ip204415 )
{
	return 0;
}

static VIDEO_UPDATE( ip204415 )
{
}

static struct EEPROM_interface eeprom_interface_93C56 =
{
	8,					// address bits	8
	8,					// data bits	8
	"*110x",			// read			110x aaaaaaaa
	"*101x",			// write		101x aaaaaaaa dddddddd
	"*111x",			// erase		111x aaaaaaaa
	"*10000xxxxxxx",	// lock			100x 00xxxx
	"*10011xxxxxxx",	// unlock		100x 11xxxx
};

static NVRAM_HANDLER(93C56)
{
	if (read_or_write)
	{
		EEPROM_save(file);
	}
	else
	{
		EEPROM_init(&eeprom_interface_93C56);
		if (file)
		{
			EEPROM_load(file);
		}
		else
		{
			int length;
			UINT8 *dat;

			dat = EEPROM_get_data_pointer(&length);
			memset(dat, 0, length);
		}
	}
}

data8_t nHPC_MiscStatus;
data32_t nHPC_ParBufPtr;
data32_t nHPC_LocalIOReg0Mask;
data32_t nHPC_VMEIntMask0;

static READ32_HANDLER( hpc_r )
{
	offset <<= 2;
	switch( offset )
	{
	case 0x05c:
		verboselog( 2, "HPC Unknown Read: %08x (%08x) (returning 0x000000a5 as kludge)\n", 0x1fb80000 + offset, mem_mask );
		return 0x0000a500;
		break;
	case 0x00ac:
		verboselog( 2, "HPC Parallel Buffer Pointer Read: %08x (%08x)\n", nHPC_ParBufPtr, mem_mask );
		return nHPC_ParBufPtr;
		break;
	case 0x00c0:
		verboselog( 2, "HPC Endianness Read: %08x (%08x)\n", 0x0000001f, mem_mask );
		return 0x0000001f;
		break;
	case 0x0124:
		verboselog( 2, "HPC Unknown Read: %08x (%08x) (returning 0x00005a00 as kludge)\n", 0x1fb80000 + offset, mem_mask );
		return 0x0000a500;
		break;
	case 0x01b0:
		verboselog( 2, "HPC Misc. Status Read: %08x (%08x)\n", nHPC_MiscStatus, mem_mask );
		return nHPC_MiscStatus;
		break;
	case 0x01bc:
		verboselog( 2, "HPC CPU Serial EEPROM Read\n" );
		return ( ( EEPROM_read_bit() << 4 ) );
		break;
	case 0x01c4:
		verboselog( 2, "HPC Local IO Register 0 Mask Read: %08x (%08x)\n", nHPC_LocalIOReg0Mask, mem_mask );
		return nHPC_LocalIOReg0Mask;
		break;
	case 0x01d8:
		verboselog( 2, "HPC VME Interrupt Mask 0 Read: %08x (%08x)\n", nHPC_LocalIOReg0Mask, mem_mask );
		return nHPC_VMEIntMask0;
		break;
	case 0x0d00:
		verboselog( 2, "HPC DUART0 Channel B Control Read\n" );
		return 0x00000004;
		break;
	case 0x0d04:
		verboselog( 2, "HPC DUART0 Channel B Data Read\n" );
		return 0;
		break;
	case 0x0d08:
		verboselog( 2, "HPC DUART0 Channel A Control Read\n" );
		return 0;
		break;
	case 0x0d0c:
		verboselog( 2, "HPC DUART0 Channel A Data Read\n" );
		return 0;
		break;
	case 0x0d10:
		verboselog( 2, "HPC DUART1 Channel B Control Read\n" );
		return 0x00000004;
		break;
	case 0x0d14:
		verboselog( 2, "HPC DUART1 Channel B Data Read\n" );
		return 0;
		break;
	case 0x0d18:
		verboselog( 2, "HPC DUART1 Channel A Control Read\n" );
		return 0;
		break;
	case 0x0d1c:
		verboselog( 2, "HPC DUART1 Channel A Data Read\n" );
		return 0;
		break;
	case 0x0d20:
		verboselog( 2, "HPC DUART2 Channel B Control Read\n" );
		return 0x00000004;
		break;
	case 0x0d24:
		verboselog( 2, "HPC DUART2 Channel B Data Read\n" );
		return 0;
		break;
	case 0x0d28:
		verboselog( 2, "HPC DUART2 Channel A Control Read\n" );
		return 0;
		break;
	case 0x0d2c:
		verboselog( 2, "HPC DUART2 Channel A Data Read\n" );
		return 0;
		break;
	case 0x0d30:
		verboselog( 2, "HPC DUART3 Channel B Control Read\n" );
		return 0x00000004;
		break;
	case 0x0d34:
		verboselog( 2, "HPC DUART3 Channel B Data Read\n" );
		return 0;
		break;
	case 0x0d38:
		verboselog( 2, "HPC DUART3 Channel A Control Read\n" );
		return 0;
		break;
	case 0x0d3c:
		verboselog( 2, "HPC DUART3 Channel A Data Read\n" );
		return 0;
		break;
	}
	verboselog( 0, "Unmapped HPC read: 0x%08x (%08x)\n", 0x1fb80000 + offset, mem_mask );
	return 0;
}

static WRITE32_HANDLER( hpc_w )
{
	offset <<= 2;
	switch( offset )
	{
	case 0x00ac:
		verboselog( 2, "HPC Parallel Buffer Pointer Write: %08x (%08x)\n", data, mem_mask );
		nHPC_ParBufPtr = data;
		break;
	case 0x01b0:
		verboselog( 2, "HPC Misc. Status Write: %08x (%08x)\n", data, mem_mask );
		if( data & 0x00000001 )
		{
			verboselog( 2, "  Force DSP hard reset\n" );
		}
		if( data & 0x00000002 )
		{
			verboselog( 2, "  Force IRQA\n" );
		}
		if( data & 0x00000004 )
		{
			verboselog( 2, "  Set IRQA polarity high\n" );
		}
		else
		{
			verboselog( 2, "  Set IRQA polarity low\n" );
		}
		if( data & 0x00000008 )
		{
			verboselog( 2, "  SRAM size: 32K\n" );
		}
		else
		{
			verboselog( 2, "  SRAM size:  8K\n" );
		}
		nHPC_MiscStatus = data;
		break;
	case 0x01bc:
		verboselog( 2, "HPC CPU Serial EEPROM Write: %08x (%08x)\n", data, mem_mask );
		if( data & 0x00000001 )
		{
			verboselog( 2, "    PRE pin high\n" );
			verboselog( 2, "    CPU board LED on\n" );
		}
		if( data & 0x00000002 )
		{
			verboselog( 2, "    CS high\n" );
			EEPROM_set_cs_line( ( data & 0x00000002 ) ? CLEAR_LINE : ASSERT_LINE );
		}
		if( data & 0x00000004 )
		{
			verboselog( 2, "    CLK high\n" );
			EEPROM_set_clock_line( ( data & 0x00000004 ) ? CLEAR_LINE : ASSERT_LINE );
		}
		if( data & 0x00000008 )
		{
			verboselog( 2, "    Data high\n" );
			EEPROM_write_bit( ( data & 0x00000008 ) ? 1 : 0 );
		}
		break;
	case 0x01c4:
		verboselog( 2, "HPC Local IO Register 0 Mask Write: %08x (%08x)\n", data, mem_mask );
		nHPC_LocalIOReg0Mask = data;
		break;
	case 0x01d8:
		verboselog( 2, "HPC VME Interrupt Mask 0 Read: %08x (%08x)\n", data, mem_mask );
		nHPC_VMEIntMask0 = data;
		break;
	case 0x0d00:
		verboselog( 2, "HPC DUART0 Channel B Control Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d04:
		verboselog( 2, "HPC DUART0 Channel B Data Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d08:
		verboselog( 2, "HPC DUART0 Channel A Control Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d0c:
		verboselog( 2, "HPC DUART0 Channel A Data Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d10:
		if( ( data & 0x000000ff ) >= 0x00000020 )
		{
			verboselog( 2, "HPC DUART1 Channel B Control Write: %08x (%08x) %c\n", data, mem_mask, data & 0x000000ff );
		}
		else
		{
			verboselog( 2, "HPC DUART1 Channel B Control Write: %08x (%08x)\n", data, mem_mask );
		}
		printf( "%c", data & 0x000000ff );
		break;
	case 0x0d14:
		verboselog( 2, "HPC DUART1 Channel B Data Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d18:
		verboselog( 2, "HPC DUART1 Channel A Control Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d1c:
		verboselog( 2, "HPC DUART1 Channel A Data Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d20:
		verboselog( 2, "HPC DUART2 Channel B Control Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d24:
		verboselog( 2, "HPC DUART2 Channel B Data Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d28:
		verboselog( 2, "HPC DUART2 Channel A Control Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d2c:
		verboselog( 2, "HPC DUART2 Channel A Data Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d30:
		verboselog( 2, "HPC DUART3 Channel B Control Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d34:
		verboselog( 2, "HPC DUART3 Channel B Data Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d38:
		verboselog( 2, "HPC DUART3 Channel A Control Write: %08x (%08x)\n", data, mem_mask );
		break;
	case 0x0d3c:
		verboselog( 2, "HPC DUART3 Channel A Data Write: %08x (%08x)\n", data, mem_mask );
		break;
	default:
		verboselog( 0, "Unmapped HPC write: 0x%08x (%08x): %08x\n", 0x1fb80000 + offset, mem_mask, data);
		break;
	}
}

static INTERRUPT_GEN( ip20_update_chips )
{
	mc_update();
}

static ADDRESS_MAP_START( ip204415_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE( 0x08000000, 0x08ffffff ) AM_RAM AM_SHARE(5)
	AM_RANGE( 0x09000000, 0x097fffff ) AM_RAM AM_SHARE(6)
	AM_RANGE( 0x0a000000, 0x0a7fffff ) AM_RAM AM_SHARE(7)
	AM_RANGE( 0x0c000000, 0x0c7fffff ) AM_RAM AM_SHARE(8)
	AM_RANGE( 0x10000000, 0x107fffff ) AM_RAM AM_SHARE(9)
	AM_RANGE( 0x08000000, 0x087fffff ) AM_RAM AM_SHARE(1)
	AM_RANGE( 0x1fa00000, 0x1fa1ffff ) AM_READWRITE( mc_r, mc_w ) AM_SHARE(3)
	AM_RANGE( 0x1fb80000, 0x1fb80fff ) AM_READWRITE( hpc_r, hpc_w ) AM_SHARE(4)
	AM_RANGE( 0x1fc00000, 0x1fc7ffff ) AM_ROM AM_SHARE(2) AM_REGION( REGION_USER1, 0 )
	AM_RANGE( 0x80000000, 0x8000ffff ) AM_RAM AM_SHARE(1)
	AM_RANGE( 0xa8000000, 0xa8ffffff ) AM_RAM AM_SHARE(5)
	AM_RANGE( 0xa9000000, 0xa97fffff ) AM_RAM AM_SHARE(6)
	AM_RANGE( 0xaa000000, 0xaa7fffff ) AM_RAM AM_SHARE(7)
	AM_RANGE( 0xac000000, 0xac7fffff ) AM_RAM AM_SHARE(8)
	AM_RANGE( 0xb0000000, 0xb07fffff ) AM_RAM AM_SHARE(9)
	AM_RANGE( 0xb8000000, 0xb87fffff ) AM_RAM AM_SHARE(1)
	AM_RANGE( 0xbfa00000, 0xbfa1ffff ) AM_READWRITE( mc_r, mc_w ) AM_SHARE(3)
	AM_RANGE( 0xbfb80000, 0xbfb80fff ) AM_READWRITE( hpc_r, hpc_w ) AM_SHARE(4)
	AM_RANGE( 0xbfc00000, 0xbfc7ffff ) AM_ROM AM_SHARE(2) /* BIOS Mirror */
ADDRESS_MAP_END

static MACHINE_INIT( ip204415 )
{
	mc_init();

	nHPC_ParBufPtr = 0;
	nHPC_LocalIOReg0Mask = 0;
	nHPC_VMEIntMask0 = 0;
}

static DRIVER_INIT( ip204415 )
{
}

INPUT_PORTS_START( ip204415 )
	PORT_START
	PORT_BIT ( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

static struct mips3_config config =
{
	32768,	/* code cache size */
	32768	/* data cache size */
};

MACHINE_DRIVER_START( ip204415 )
	MDRV_CPU_ADD_TAG( "main", R4600BE, 50000000*3 )
	MDRV_CPU_CONFIG( config )
	MDRV_CPU_PROGRAM_MAP( ip204415_map, 0 )
	MDRV_CPU_VBLANK_INT( ip20_update_chips, 10000 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT( ip204415 )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(800, 600)
	MDRV_VISIBLE_AREA(0, 799, 0, 599)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START( ip204415 )
	MDRV_VIDEO_UPDATE( ip204415 )
MACHINE_DRIVER_END

ROM_START( ip204415 )
	ROM_REGION( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "ip204415.bin", 0x000000, 0x080000, CRC(940d960e) SHA1(596aba530b53a147985ff3f6f853471ce48c866c) )
ROM_END

SYSTEM_CONFIG_START( ip204415 )
SYSTEM_CONFIG_END

/*     YEAR  NAME      PARENT    COMPAT    MACHINE   INPUT     INIT      CONFIG    COMPANY   FULLNAME */
COMPX( 1993, ip204415, 0,        0,        ip204415, ip204415, ip204415, ip204415, "Silicon Graphics, Inc", "IRIS Indigo (R4400, 150MHz)", GAME_NOT_WORKING | GAME_NO_SOUND )
