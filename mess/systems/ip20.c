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

static data32_t nMC_CPUControl0;
static data32_t nMC_CPUControl1;
static data32_t nMC_Watchdog;
static data32_t nMC_SysID;
static data32_t nMC_RPSSDiv;
static data32_t nMC_RefCntPreload;
static data32_t nMC_RefCnt;
static data32_t nMC_GIO64ArbParam;
static data32_t nMC_ArbCPUTime;
static data32_t nMC_ArbBurstTime;
static data32_t nMC_MemCfg0;
static data32_t nMC_MemCfg1;
static data32_t nMC_CPUMemAccCfg;
static data32_t nMC_GIOMemAccCfg;
static data32_t nMC_CPUErrorAddr;
static data32_t nMC_CPUErrorStatus;
static data32_t nMC_GIOErrorAddr;
static data32_t nMC_GIOErrorStatus;
static data32_t nMC_SysSemaphore;
static data32_t nMC_GIOLock;
static data32_t nMC_EISALock;
static data32_t nMC_GIO64TransMask;
static data32_t nMC_GIO64Subst;
static data32_t nMC_DMAIntrCause;
static data32_t nMC_DMAControl;
static data32_t nMC_DMATLBEntry0Hi;
static data32_t nMC_DMATLBEntry0Lo;
static data32_t nMC_DMATLBEntry1Hi;
static data32_t nMC_DMATLBEntry1Lo;
static data32_t nMC_DMATLBEntry2Hi;
static data32_t nMC_DMATLBEntry2Lo;
static data32_t nMC_DMATLBEntry3Hi;
static data32_t nMC_DMATLBEntry3Lo;
static data32_t nMC_RPSSCounter;

static READ32_HANDLER( mc_r )
{
	offset <<= 2;
	switch( offset )
	{
	case 0x0000:
	case 0x0004:
		verboselog( 2, "CPU Control 0 Read: %08x\n", nMC_CPUControl0 );
		return nMC_CPUControl0;
		break;
	case 0x0008:
	case 0x000c:
		verboselog( 2, "CPU Control 1 Read: %08x\n", nMC_CPUControl1 );
		return nMC_CPUControl1;
		break;
	case 0x0010:
	case 0x0014:
		verboselog( 2, "Watchdog Timer Read: %08x\n", nMC_Watchdog );
		return nMC_Watchdog;
		break;
	case 0x0018:
	case 0x001c:
		verboselog( 2, "System ID Read: %08x\n", nMC_SysID );
		return nMC_SysID;
		break;
	case 0x0028:
	case 0x002c:
		verboselog( 2, "RPSS Divider Read: %08x\n", nMC_RPSSDiv );
		return nMC_RPSSDiv;
		break;
	case 0x0030:
	case 0x0034:
		verboselog( 2, "R4000 EEPROM Read\n" );
		return 0;
		break;
	case 0x0040:
	case 0x0044:
		verboselog( 2, "Refresh Count Preload Read: %08x\n", nMC_RefCntPreload );
		return nMC_RefCntPreload;
		break;
	case 0x0048:
	case 0x004c:
		verboselog( 2, "Refresh Count Read: %08x\n", nMC_RefCnt );
		return nMC_RefCnt;
		break;
	case 0x0080:
	case 0x0084:
		verboselog( 2, "GIO64 Arbitration Param Read: %08x\n", nMC_GIO64ArbParam );
		return nMC_GIO64ArbParam;
		break;
	case 0x0088:
	case 0x008c:
		verboselog( 2, "Arbiter CPU Time Read: %08x\n", nMC_ArbCPUTime );
		return nMC_ArbCPUTime;
		break;
	case 0x0098:
	case 0x009c:
		verboselog( 2, "Arbiter Long Burst Time Read: %08x\n", nMC_ArbBurstTime );
		return nMC_ArbBurstTime;
		break;
	case 0x00c0:
	case 0x00c4:
		verboselog( 2, "Memory Configuration Register 0 Read: %08x\n", nMC_MemCfg0 );
		return nMC_MemCfg0;
		break;
	case 0x00c8:
	case 0x00cc:
		verboselog( 2, "Memory Configuration Register 1 Read: %08x\n", nMC_MemCfg1 );
		return nMC_MemCfg1;
		break;
	case 0x00d0:
	case 0x00d4:
		verboselog( 2, "CPU Memory Access Config Params Read: %08x\n", nMC_CPUMemAccCfg );
		return nMC_CPUMemAccCfg;
		break;
	case 0x00d8:
	case 0x00dc:
		verboselog( 2, "GIO Memory Access Config Params Read: %08x\n", nMC_GIOMemAccCfg );
		return nMC_GIOMemAccCfg;
		break;
	case 0x00e0:
	case 0x00e4:
		verboselog( 2, "CPU Error Address Read: %08x\n", nMC_CPUErrorAddr );
		return nMC_CPUErrorAddr;
		break;
	case 0x00e8:
	case 0x00ec:
		verboselog( 2, "CPU Error Status Read: %08x\n", nMC_CPUErrorStatus );
		return nMC_CPUErrorStatus;
		break;
	case 0x00f0:
	case 0x00f4:
		verboselog( 2, "GIO Error Address Read: %08x\n", nMC_GIOErrorAddr );
		return nMC_GIOErrorAddr;
		break;
	case 0x00f8:
	case 0x00fc:
		verboselog( 2, "GIO Error Status Read: %08x\n", nMC_GIOErrorStatus );
		return nMC_GIOErrorStatus;
		break;
	case 0x0100:
	case 0x0104:
		verboselog( 2, "System Semaphore Read: %08x\n", nMC_SysSemaphore );
		return nMC_SysSemaphore;
		break;
	case 0x0108:
	case 0x010c:
		verboselog( 2, "GIO Lock Read: %08x\n", nMC_GIOLock );
		return nMC_GIOLock;
		break;
	case 0x0110:
	case 0x0114:
		verboselog( 2, "EISA Lock Read: %08x\n", nMC_EISALock );
		return nMC_EISALock;
		break;
	case 0x0150:
	case 0x0154:
		verboselog( 2, "GIO64 Translation Address Mask Read: %08x\n", nMC_GIO64TransMask );
		return nMC_GIO64TransMask;
		break;
	case 0x0158:
	case 0x015c:
		verboselog( 2, "GIO64 Translation Address Substitution Bits Read: %08x\n", nMC_GIO64Subst );
		return nMC_GIO64Subst;
		break;
	case 0x0160:
	case 0x0164:
		verboselog( 2, "DMA Interrupt Cause: %08x\n", nMC_DMAIntrCause );
		return nMC_DMAIntrCause;
	case 0x0168:
	case 0x016c:
		verboselog( 2, "DMA Control Read: %08x\n", nMC_DMAControl );
		return nMC_DMAControl;
		break;
	case 0x0180:
	case 0x0184:
		verboselog( 2, "DMA TLB Entry 0 High Read: %08x\n", nMC_DMATLBEntry0Hi );
		return nMC_DMATLBEntry0Hi;
		break;
	case 0x0188:
	case 0x018c:
		verboselog( 2, "DMA TLB Entry 0 Low Read: %08x\n", nMC_DMATLBEntry0Lo );
		return nMC_DMATLBEntry0Lo;
		break;
	case 0x0190:
	case 0x0194:
		verboselog( 2, "DMA TLB Entry 1 High Read: %08x\n", nMC_DMATLBEntry1Hi );
		return nMC_DMATLBEntry1Hi;
		break;
	case 0x0198:
	case 0x019c:
		verboselog( 2, "DMA TLB Entry 1 Low Read: %08x\n", nMC_DMATLBEntry1Lo );
		return nMC_DMATLBEntry1Lo;
		break;
	case 0x01a0:
	case 0x01a4:
		verboselog( 2, "DMA TLB Entry 2 High Read: %08x\n", nMC_DMATLBEntry2Hi );
		return nMC_DMATLBEntry2Hi;
		break;
	case 0x01a8:
	case 0x01ac:
		verboselog( 2, "DMA TLB Entry 2 Low Read: %08x\n", nMC_DMATLBEntry2Lo );
		return nMC_DMATLBEntry2Lo;
		break;
	case 0x01b0:
	case 0x01b4:
		verboselog( 2, "DMA TLB Entry 3 High Read: %08x\n", nMC_DMATLBEntry3Hi );
		return nMC_DMATLBEntry3Hi;
		break;
	case 0x01b8:
	case 0x01bc:
		verboselog( 2, "DMA TLB Entry 3 Low Read: %08x\n", nMC_DMATLBEntry3Lo );
		return nMC_DMATLBEntry3Lo;
		break;
	case 0x1000:
	case 0x1004:
		verboselog( 2, "RPSS 100ns Counter Read: %08x\n", nMC_RPSSCounter );
		return nMC_RPSSCounter;
		break;
	}
	verboselog( 0, "Unmapped MC read: 0x%08x\n", 0x1fb80000 + offset );
	return 0;
}

static WRITE32_HANDLER( mc_w )
{
	offset <<= 2;
	switch( offset )
	{
	case 0x0000:
	case 0x0004:
		verboselog( 2, "CPU Control 0 Write: %08x\n", data );
		nMC_CPUControl0 = data;
		break;
	case 0x0008:
	case 0x000c:
		verboselog( 2, "CPU Control 1 Write: %08x\n", data );
		nMC_CPUControl1 = data;
		break;
	case 0x0010:
	case 0x0014:
		verboselog( 2, "Watchdog Timer Clear" );
		nMC_Watchdog = 0;
		break;
	case 0x0028:
	case 0x002c:
		verboselog( 2, "RPSS Divider Write: %08x\n", data );
		nMC_RPSSDiv = data;
		break;
	case 0x0030:
	case 0x0034:
		verboselog( 2, "R4000 EEPROM Write\n" );
		break;
	case 0x0040:
	case 0x0044:
		verboselog( 2, "Refresh Count Preload Write: %08x\n", data );
		nMC_RefCntPreload = data;
		break;
	case 0x0080:
	case 0x0084:
		verboselog( 2, "GIO64 Arbitration Param Write: %08x\n", data );
		nMC_GIO64ArbParam = data;
		break;
	case 0x0088:
	case 0x008c:
		verboselog( 2, "Arbiter CPU Time Write: %08x\n", data );
		nMC_ArbCPUTime = data;
		break;
	case 0x0098:
	case 0x009c:
		verboselog( 2, "Arbiter Long Burst Time Write: %08x\n", data );
		nMC_ArbBurstTime = data;
		break;
	case 0x00c0:
	case 0x00c4:
		verboselog( 2, "Memory Configuration Register 0 Write: %08x\n", data );
		nMC_MemCfg0 = data;
		break;
	case 0x00c8:
	case 0x00cc:
		verboselog( 2, "Memory Configuration Register 1 Write: %08x\n", data );
		nMC_MemCfg1 = data;
		break;
	case 0x00d0:
	case 0x00d4:
		verboselog( 2, "CPU Memory Access Config Params Write: %08x\n", data );
		nMC_CPUMemAccCfg = data;
		break;
	case 0x00d8:
	case 0x00dc:
		verboselog( 2, "GIO Memory Access Config Params Write: %08x\n", data );
		nMC_GIOMemAccCfg = data;
		break;
	case 0x00e8:
	case 0x00ec:
		verboselog( 2, "CPU Error Status Clear\n" );
		nMC_CPUErrorStatus = 0;
		break;
	case 0x00f8:
	case 0x00fc:
		verboselog( 2, "GIO Error Status Clear\n" );
		nMC_GIOErrorStatus = 0;
		break;
	case 0x0100:
	case 0x0104:
		verboselog( 2, "System Semaphore Write: %08x\n", data );
		nMC_SysSemaphore = data;
		break;
	case 0x0108:
	case 0x010c:
		verboselog( 2, "GIO Lock Write: %08x\n", data );
		nMC_GIOLock = data;
		break;
	case 0x0110:
	case 0x0114:
		verboselog( 2, "EISA Lock Write: %08x\n", data );
		nMC_EISALock = data;
		break;
	case 0x0150:
	case 0x0154:
		verboselog( 2, "GIO64 Translation Address Mask Write: %08x\n", data );
		nMC_GIO64TransMask = data;
		break;
	case 0x0158:
	case 0x015c:
		verboselog( 2, "GIO64 Translation Address Substitution Bits Write: %08x\n", data );
		nMC_GIO64Subst = data;
		break;
	case 0x0160:
	case 0x0164:
		verboselog( 2, "DMA Interrupt Cause Write: %08x\n", data );
		nMC_DMAIntrCause = data;
		break;
	case 0x0168:
	case 0x016c:
		verboselog( 2, "DMA Control Write: %08x\n", data );
		nMC_DMAControl = data;
		break;
	case 0x0180:
	case 0x0184:
		verboselog( 2, "DMA TLB Entry 0 High Write: %08x\n", data );
		nMC_DMATLBEntry0Hi = data;
		break;
	case 0x0188:
	case 0x018c:
		verboselog( 2, "DMA TLB Entry 0 Low Write: %08x\n", data );
		nMC_DMATLBEntry0Lo = data;
		break;
	case 0x0190:
	case 0x0194:
		verboselog( 2, "DMA TLB Entry 1 High Write: %08x\n", data );
		nMC_DMATLBEntry1Hi = data;
		break;
	case 0x0198:
	case 0x019c:
		verboselog( 2, "DMA TLB Entry 1 Low Write: %08x\n", data );
		nMC_DMATLBEntry1Lo = data;
		break;
	case 0x01a0:
	case 0x01a4:
		verboselog( 2, "DMA TLB Entry 2 High Write: %08x\n", data );
		nMC_DMATLBEntry2Hi = data;
		break;
	case 0x01a8:
	case 0x01ac:
		verboselog( 2, "DMA TLB Entry 2 Low Write: %08x\n", data );
		nMC_DMATLBEntry2Lo = data;
		break;
	case 0x01b0:
	case 0x01b4:
		verboselog( 2, "DMA TLB Entry 3 High Write: %08x\n", data );
		nMC_DMATLBEntry3Hi = data;
		break;
	case 0x01b8:
	case 0x01bc:
		verboselog( 2, "DMA TLB Entry 3 Low Write: %08x\n", data );
		nMC_DMATLBEntry3Lo = data;
		break;
	default:
		verboselog( 0, "Unmapped MC write: 0x%08x: %08x\n", 0x1fa00000 + offset, data );
		break;
	}
}

data8_t nHPC_MiscStatus;
data32_t nHPC_ParBufPtr;

static READ32_HANDLER( hpc_r )
{
	offset <<= 2;
	switch( offset )
	{
	case 0x00ac:
		verboselog( 2, "HPC Parallel Buffer Pointer Read: %08x\n", nHPC_ParBufPtr );
		return nHPC_ParBufPtr;
		break;
	case 0x00c0:
		verboselog( 2, "HPC Endianness Read: %08x\n", 0x0000001f );
		return 0x0000001f;
		break;
	case 0x01b0:
		verboselog( 2, "HPC Misc. Status Read: %08x\n", nHPC_MiscStatus );
		return nHPC_MiscStatus;
		break;
	case 0x01bc:
		verboselog( 2, "HPC CPU Serial EEPROM Read\n" );
		return 0;
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
	verboselog( 0, "Unmapped HPC read: 0x%08x\n", 0x1fb80000 + offset );
	return 0;
}

static WRITE32_HANDLER( hpc_w )
{
	offset <<= 2;
	switch( offset )
	{
	case 0x00ac:
		verboselog( 2, "HPC Parallel Buffer Pointer Write: %08x\n", data );
		nHPC_ParBufPtr = data;
		break;
	case 0x01b0:
		verboselog( 2, "HPC Misc. Status Write: %08x\n", data );
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
		verboselog( 2, "HPC CPU Serial EEPROM Write: %08x\n", data );
		if( data & 0x00000001 )
		{
			verboselog( 2, "    PRE pin high\n" );
			verboselog( 2, "    CPU board LED on\n" );
		}
		if( data & 0x00000002 )
		{
			verboselog( 2, "    CS high\n" );
		}
		if( data & 0x00000004 )
		{
			verboselog( 2, "    CLK high\n" );
		}
		if( data & 0x00000008 )
		{
			verboselog( 2, "    Data high\n" );
		}
		break;
	case 0x0d00:
		verboselog( 2, "HPC DUART0 Channel B Control Write: %08x\n", data );
		break;
	case 0x0d04:
		verboselog( 2, "HPC DUART0 Channel B Data Write: %08x\n", data );
		break;
	case 0x0d08:
		verboselog( 2, "HPC DUART0 Channel A Control Write: %08x\n", data );
		break;
	case 0x0d0c:
		verboselog( 2, "HPC DUART0 Channel A Data Write: %08x\n", data );
		break;
	case 0x0d10:
		if( ( data & 0x000000ff ) >= 0x00000020 )
		{
			verboselog( 2, "HPC DUART1 Channel B Control Write: %08x %c\n", data, data & 0x000000ff );
		}
		else
		{
			verboselog( 2, "HPC DUART1 Channel B Control Write: %08x\n", data );
		}
		printf( "%c", data & 0x000000ff );
		break;
	case 0x0d14:
		verboselog( 2, "HPC DUART1 Channel B Data Write: %08x\n", data );
		break;
	case 0x0d18:
		verboselog( 2, "HPC DUART1 Channel A Control Write: %08x\n", data );
		break;
	case 0x0d1c:
		verboselog( 2, "HPC DUART1 Channel A Data Write: %08x\n", data );
		break;
	case 0x0d20:
		verboselog( 2, "HPC DUART2 Channel B Control Write: %08x\n", data );
		break;
	case 0x0d24:
		verboselog( 2, "HPC DUART2 Channel B Data Write: %08x\n", data );
		break;
	case 0x0d28:
		verboselog( 2, "HPC DUART2 Channel A Control Write: %08x\n", data );
		break;
	case 0x0d2c:
		verboselog( 2, "HPC DUART2 Channel A Data Write: %08x\n", data );
		break;
	case 0x0d30:
		verboselog( 2, "HPC DUART3 Channel B Control Write: %08x\n", data );
		break;
	case 0x0d34:
		verboselog( 2, "HPC DUART3 Channel B Data Write: %08x\n", data );
		break;
	case 0x0d38:
		verboselog( 2, "HPC DUART3 Channel A Control Write: %08x\n", data );
		break;
	case 0x0d3c:
		verboselog( 2, "HPC DUART3 Channel A Data Write: %08x\n", data );
		break;
	default:
		verboselog( 0, "Unmapped HPC write: 0x%08x: %08x\n", 0x1fb80000 + offset, data );
		break;
	}
}

static INTERRUPT_GEN( rpss_count_update )
{
	nMC_RPSSCounter += 1000;
}

static ADDRESS_MAP_START( ip204415_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE( 0x00000000, 0x0000ffff ) AM_RAM AM_SHARE(1)
	AM_RANGE( 0x1fa00000, 0x1fa1ffff ) AM_READWRITE( mc_r, mc_w ) AM_SHARE(3)
	AM_RANGE( 0x1fb80000, 0x1fb80fff ) AM_READWRITE( hpc_r, hpc_w ) AM_SHARE(4)
	AM_RANGE( 0x1fc00000, 0x1fc7ffff ) AM_ROM AM_SHARE(2) AM_REGION( REGION_USER1, 0 )
	AM_RANGE( 0x80000000, 0x8000ffff ) AM_RAM AM_SHARE(1)
	AM_RANGE( 0xa0000000, 0xa000ffff ) AM_RAM AM_SHARE(1)
	AM_RANGE( 0xbfa00000, 0xbfa1ffff ) AM_READWRITE( mc_r, mc_w ) AM_SHARE(3)
	AM_RANGE( 0xbfb80000, 0xbfb80fff ) AM_READWRITE( hpc_r, hpc_w ) AM_SHARE(4)
	AM_RANGE( 0xbfc00000, 0xbfc7ffff ) AM_ROM AM_SHARE(2) /* BIOS Mirror */
ADDRESS_MAP_END

static MACHINE_INIT( ip204415 )
{
	nMC_CPUControl0 = 0;
	nMC_CPUControl1 = 0;
	nMC_Watchdog = 0;
	nMC_SysID = 0;
	nMC_RPSSDiv = 0;
	nMC_RefCntPreload = 0;
	nMC_RefCnt = 0;
	nMC_GIO64ArbParam = 0;
	nMC_ArbCPUTime = 0;
	nMC_ArbBurstTime = 0;
	nMC_MemCfg0 = 0;
	nMC_MemCfg1 = 0;
	nMC_CPUMemAccCfg = 0;
	nMC_GIOMemAccCfg = 0;
	nMC_CPUErrorAddr = 0;
	nMC_CPUErrorStatus = 0;
	nMC_GIOErrorAddr = 0;
	nMC_GIOErrorStatus = 0;
	nMC_SysSemaphore = 0;
	nMC_GIOLock = 0;
	nMC_EISALock = 0;
	nMC_GIO64TransMask = 0;
	nMC_GIO64Subst = 0;
	nMC_DMAIntrCause = 0;
	nMC_DMAControl = 0;
	nMC_DMATLBEntry0Hi = 0;
	nMC_DMATLBEntry0Lo = 0;
	nMC_DMATLBEntry1Hi = 0;
	nMC_DMATLBEntry1Lo = 0;
	nMC_DMATLBEntry2Hi = 0;
	nMC_DMATLBEntry2Lo = 0;
	nMC_DMATLBEntry3Hi = 0;
	nMC_DMATLBEntry3Lo = 0;
	nMC_RPSSCounter = 0;

	nHPC_ParBufPtr = 0;
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
	MDRV_CPU_VBLANK_INT( rpss_count_update, 10000 )

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
