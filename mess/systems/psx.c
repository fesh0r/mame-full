/***************************************************************************

  Sony PSX
  ========
  Preliminary driver by smf

  todo:
    add pad support
    tidy up cd controller
    work out why bios schph1000 doesn't get past the first screen
    fix garbage at the bottom of some of the bootup screens
    add memcard support
    add cd image support

***************************************************************************/

#include "driver.h"
#include "cpu/mips/psx.h"
#include "devices/snapquik.h"
#include "includes/psx.h"

/* this seems to have disappeared */
static void mips_stop( void ) {}

struct
{
	UINT8 id[ 8 ];
	UINT32 text;	/* SCE only */
	UINT32 data;	/* SCE only */
	UINT32 pc0;
	UINT32 gp0;		/* SCE only */
	UINT32 t_addr;
	UINT32 t_size;
	UINT32 d_addr;	/* SCE only */
	UINT32 d_size;	/* SCE only */
	UINT32 b_addr;	/* SCE only */
	UINT32 b_size;	/* SCE only */
	UINT32 s_addr;
	UINT32 s_size;
	UINT32 SavedSP;
	UINT32 SavedFP;
	UINT32 SavedGP;
	UINT32 SavedRA;
	UINT32 SavedS0;
	UINT8 dummy[ 0x800 - 76 ];
} m_psxexe_header;

static UINT8 *m_p_psxexe;

static OPBASE_HANDLER( psx_setopbase )
{
	if( address == 0x80030000 )
	{
		UINT8 *p_ram;
		UINT8 *p_psxexe;
		UINT32 n_stack;
		UINT32 n_ram;
		UINT32 n_left;
		UINT32 n_address;

		logerror( "psxexe_load: pc  %08x\n", m_psxexe_header.pc0 );
		logerror( "psxexe_load: org %08x\n", m_psxexe_header.t_addr );
		logerror( "psxexe_load: len %08x\n", m_psxexe_header.t_size );
		logerror( "psxexe_load: sp  %08x\n", m_psxexe_header.s_addr );
		logerror( "psxexe_load: len %08x\n", m_psxexe_header.s_size );

		p_ram = (UINT8 *)g_p_n_psxram;
		n_ram = g_n_psxramsize;

		p_psxexe = m_p_psxexe;

		n_address = m_psxexe_header.t_addr;
		n_left = m_psxexe_header.t_size;
		while( n_left != 0 )
		{
			p_ram[ BYTE4_XOR_LE( n_address ) % n_ram ] = *( p_psxexe );
			n_address++;
			p_psxexe++;
			n_left--;
		}

		free( m_p_psxexe );

		activecpu_set_reg( MIPS_PC, m_psxexe_header.pc0 );
		activecpu_set_reg( MIPS_R28, m_psxexe_header.gp0 );
		n_stack = m_psxexe_header.s_addr + m_psxexe_header.s_size;
		if( n_stack != 0 )
		{
			activecpu_set_reg( MIPS_R29, n_stack );
			activecpu_set_reg( MIPS_R30, n_stack );
		}

		memory_set_opbase_handler( 0, NULL );
		mips_stop();
		return ~0;
	}
	return address;
}

static void psxexe_conv32( UINT32 *p_uint32 )
{
	UINT8 *p_uint8;

	p_uint8 = (UINT8 *)p_uint32;

	*( p_uint32 ) = p_uint8[ 0 ] |
		( p_uint8[ 1 ] << 8 ) |
		( p_uint8[ 2 ] << 16 ) |
		( p_uint8[ 3 ] << 24 );
}

static QUICKLOAD_LOAD( psxexe_load )
{
	if( mame_fread( fp, &m_psxexe_header, sizeof( m_psxexe_header ) ) != sizeof( m_psxexe_header ) )
	{
		logerror( "psxexe_load: invalid exe\n" );
		return INIT_FAIL;
	}
	if( memcmp( m_psxexe_header.id, "PS-X EXE", 8 ) != 0 )
	{
		logerror( "psxexe_load: invalid header id\n" );
		return INIT_FAIL;
	}

	psxexe_conv32( &m_psxexe_header.text );
	psxexe_conv32( &m_psxexe_header.data );
	psxexe_conv32( &m_psxexe_header.pc0 );
	psxexe_conv32( &m_psxexe_header.gp0 );
	psxexe_conv32( &m_psxexe_header.t_addr );
	psxexe_conv32( &m_psxexe_header.t_size );
	psxexe_conv32( &m_psxexe_header.d_addr );
	psxexe_conv32( &m_psxexe_header.d_size );
	psxexe_conv32( &m_psxexe_header.b_addr );
	psxexe_conv32( &m_psxexe_header.b_size );
	psxexe_conv32( &m_psxexe_header.s_addr );
	psxexe_conv32( &m_psxexe_header.s_size );
	psxexe_conv32( &m_psxexe_header.SavedSP );
	psxexe_conv32( &m_psxexe_header.SavedFP );
	psxexe_conv32( &m_psxexe_header.SavedGP );
	psxexe_conv32( &m_psxexe_header.SavedRA );
	psxexe_conv32( &m_psxexe_header.SavedS0 );

	m_p_psxexe = malloc( m_psxexe_header.t_size );
	if( m_p_psxexe == NULL )
	{
		logerror( "psxexe_load: out of memory\n" );
		return INIT_FAIL;
	}
	mame_fread( fp, m_p_psxexe, m_psxexe_header.t_size );
	memory_set_opbase_handler( 0, psx_setopbase );
	return INIT_PASS;
}

/* -----------------------------------------------------------------------
 * cd_stat values
 *
 * NoIntr       0x00 No interrupt
 * DataReady    0x01 Data Ready
 * Acknowledge  0x02 Command Complete
 * Complete     0x03 Acknowledge
 * DataEnd      0x04 End of Data Detected
 * DiskError    0x05 Error Detected
 * ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */

static int cd_param_p;
static int cd_result_p;
static int cd_result_c;
static int cd_result_ready;
static int cd_readed = -1;
static int cd_reset;
static UINT8 cd_stat;
static UINT8 cd_io_status;
static UINT8 cd_param[8];
static UINT8 cd_result[8];

/* ----------------------------------------------------------------------- */

static void psx_cdcmd_sync(void)
{
	/* NYI */
}

static void psx_cdcmd_nop(void)
{
	/* NYI */
}

static void psx_cdcmd_setloc(void)
{
	/* NYI */
}

static void psx_cdcmd_play(void)
{
	/* NYI */
}

static void psx_cdcmd_forward(void)
{
	/* NYI */
}

static void psx_cdcmd_backward(void)
{
	/* NYI */
}

static void psx_cdcmd_readn(void)
{
	/* NYI */
}

static void psx_cdcmd_standby(void)
{
	/* NYI */
}

static void psx_cdcmd_stop(void)
{
	/* NYI */
}

static void psx_cdcmd_pause(void)
{
	/* NYI */
}

static void psx_cdcmd_init(void)
{
	cd_result_p = 0;
	cd_result_c = 1;
	cd_stat = 0x02;
	cd_result[0] = cd_stat;
	/* NYI */
}

static void psx_cdcmd_mute(void)
{
	/* NYI */
}

static void psx_cdcmd_demute(void)
{
	/* NYI */
}

static void psx_cdcmd_setfilter(void)
{
	/* NYI */
}

static void psx_cdcmd_setmode(void)
{
	/* NYI */
}

static void psx_cdcmd_getparam(void)
{
	/* NYI */
}

static void psx_cdcmd_getlocl(void)
{
	/* NYI */
}

static void psx_cdcmd_getlocp(void)
{
	/* NYI */
}

static void psx_cdcmd_gettn(void)
{
	/* NYI */
}

static void psx_cdcmd_gettd(void)
{
	/* NYI */
}

static void psx_cdcmd_seekl(void)
{
	/* NYI */
}

static void psx_cdcmd_seekp(void)
{
	/* NYI */
}

static void psx_cdcmd_test(void)
{
	static const UINT8 test20[] = { 0x98, 0x06, 0x10, 0xC3 };
	static const UINT8 test22[] = { 0x66, 0x6F, 0x72, 0x20, 0x45, 0x75, 0x72, 0x6F };
	static const UINT8 test23[] = { 0x43, 0x58, 0x44, 0x32, 0x39 ,0x34, 0x30, 0x51 };

	switch(cd_param[0]) {
	case 0x04:	/* read SCEx counter (returned in 1st byte?) */
		break;
	case 0x05:	/* reset SCEx counter. */
		break;
	case 0x20:
		memcpy(cd_result, test20, sizeof(test20));
		cd_result_p = 0;
		cd_result_c = sizeof(test20);
		break;
	case 0x22:
		memcpy(cd_result, test22, sizeof(test22));
		cd_result_p = 0;
		cd_result_c = sizeof(test22);
		break;
	case 0x23:
		memcpy(cd_result, test23, sizeof(test23));
		cd_result_p = 0;
		cd_result_c = sizeof(test23);
		break;
	}
	cd_stat = 3;
}

static void psx_cdcmd_id(void)
{
	/* NYI */
}

static void psx_cdcmd_reads(void)
{
	/* NYI */
}

static void psx_cdcmd_reset(void)
{
	/* NYI */
}

static void psx_cdcmd_readtoc(void)
{
	/* NYI */
}

static void (*psx_cdcmds[])(void) =
{
	psx_cdcmd_sync,
	psx_cdcmd_nop,
	psx_cdcmd_setloc,
	psx_cdcmd_play,
	psx_cdcmd_forward,
	psx_cdcmd_backward,
	psx_cdcmd_readn,
	psx_cdcmd_standby,
	psx_cdcmd_stop,
	psx_cdcmd_pause,
	psx_cdcmd_init,
	psx_cdcmd_mute,
	psx_cdcmd_demute,
	psx_cdcmd_setfilter,
	psx_cdcmd_setmode,
	psx_cdcmd_getparam,
	psx_cdcmd_getlocl,
	psx_cdcmd_getlocp,
	psx_cdcmd_gettn,
	psx_cdcmd_gettd,
	psx_cdcmd_seekl,
	psx_cdcmd_seekp,
	NULL,
	NULL,
	NULL,
	psx_cdcmd_test,
	psx_cdcmd_id,
	psx_cdcmd_reads,
	psx_cdcmd_reset,
	NULL,
	psx_cdcmd_readtoc
};

/* ----------------------------------------------------------------------- */

READ32_HANDLER( psx_cd_r )
{
	UINT32 result = 0;

	if( mem_mask == 0xffffff00 )
	{
		logerror( "%08x cd0 read\n", activecpu_get_pc() );

		if (cd_result_ready && cd_result_c)
			cd_io_status |= 0x20;
		else
			cd_io_status &= ~0x20;

		if (cd_readed == 0)
			result = cd_io_status | 0x40;
		else
			result = cd_io_status;

		/* NPW 21-May-2003 - Seems to expect this on boot */
		result |= 0x0f;
	}
	else if( mem_mask == 0xffff00ff )
	{
		logerror( "%08x cd1 read\n", activecpu_get_pc() );

		if (cd_result_ready && cd_result_c)
			result = ((UINT32) cd_result[cd_result_p]) << 8;

		if (cd_result_ready)
		{
			cd_result_p++;
			if (cd_result_p == cd_result_c)
				cd_result_ready = 0;
		}
	}
	else if( mem_mask == 0xff00ffff )
	{
		logerror( "%08x cd2 read\n", activecpu_get_pc() );
	}
	else if( mem_mask == 0x00ffffff )
	{
		logerror( "%08x cd3 read\n", activecpu_get_pc() );

		result = ((UINT32) cd_stat) << 24;
	}
	else
	{
		logerror( "%08x cd_r( %08x, %08x ) unsupported transfer\n", activecpu_get_pc(), offset, mem_mask );
	}
	return result;
}

WRITE32_HANDLER( psx_cd_w )
{
	void (*psx_cdcmd)(void);

	if( mem_mask == 0xffffff00 )
	{
		/* write to CD register 0 */
		data = (data >> 0) & 0xff;
		logerror( "%08x cd0 write %02x\n", activecpu_get_pc(), data & 0xff );

		cd_param_p = 0;
		if (data == 0x00)
		{
			cd_result_ready = 0;
		}
		else
		{
			if (data & 0x01)
				cd_result_ready = 1;
			if (data == 0x01)
				cd_reset = 1;
		}
	}
	else if( mem_mask == 0xffff00ff )
	{
		/* write to CD register 1 */
		data = (data >> 8) & 0xff;
		logerror( "%08x cd1 write %02x\n", activecpu_get_pc(), data );

		if (data <= sizeof(psx_cdcmds) / sizeof(psx_cdcmds[0]))
			psx_cdcmd = psx_cdcmds[data];
		else
			psx_cdcmd = NULL;

		if (psx_cdcmd)
			psx_cdcmd();

		psx_irq_set(0x0004);
	}
	else if( mem_mask == 0xff00ffff )
	{
		/* write to CD register 2 */
		data = (data >> 16) & 0xff;
		logerror( "%08x cd2 write %02x\n", activecpu_get_pc(), data & 0xff );

		if ((cd_reset == 2) && (data == 0x07))
		{
			/* copied from SOPE; this code is weird */
			cd_param_p = 0;
			cd_result_ready = 1;
			cd_reset = 0;
			cd_stat = 0;
			cd_io_status = 0x00;
			if (cd_result_ready && cd_result_c)
				cd_io_status |= 0x20;
			else
				cd_io_status &= ~0x20;
		}
		else
		{
			/* used for sending arguments */
			cd_reset = 0;
			cd_param[cd_param_p] = data;
			if (cd_param_p < 7)
				cd_param_p++;
		}
	}
	else if( mem_mask == 0x00ffffff )
	{
		/* write to CD register 3 */
		data = (data >> 24) & 0xff;
		logerror( "%08x cd3 write %02x\n", activecpu_get_pc(), data & 0xff );

		if ((data == 0x07) && (cd_reset == 1))
		{
			cd_reset = 2;
//			psxirq_clear(0x0004);
		}
		else
		{
			cd_reset = 0;
		}
	}
	else
	{
		logerror( "%08x cd_w( %08x, %08x, %08x ) unsupported transfer\n", activecpu_get_pc(), offset, data, mem_mask );
	}
}

static ADDRESS_MAP_START( psx_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM	AM_SHARE(1) AM_BASE(&g_p_n_psxram) AM_SIZE(&g_n_psxramsize) /* ram */
	AM_RANGE(0x1f800000, 0x1f8003ff) AM_RAM /* scratchpad */
	AM_RANGE(0x1f801000, 0x1f801007) AM_WRITENOP
	AM_RANGE(0x1f801008, 0x1f80100b) AM_RAM /* ?? */
	AM_RANGE(0x1f80100c, 0x1f80102f) AM_WRITENOP
	AM_RANGE(0x1f801010, 0x1f801013) AM_READNOP
	AM_RANGE(0x1f801014, 0x1f801017) AM_READ(psx_spu_delay_r)
	AM_RANGE(0x1f801040, 0x1f80105f) AM_READWRITE(psx_sio_r, psx_sio_w)
	AM_RANGE(0x1f801060, 0x1f80106f) AM_WRITENOP
	AM_RANGE(0x1f801070, 0x1f801077) AM_READWRITE(psx_irq_r, psx_irq_w)
	AM_RANGE(0x1f801080, 0x1f8010ff) AM_READWRITE(psx_dma_r, psx_dma_w)
	AM_RANGE(0x1f801100, 0x1f80113f) AM_READWRITE(psx_counter_r, psx_counter_w)
	AM_RANGE(0x1f801800, 0x1f801803) AM_READWRITE(psx_cd_r, psx_cd_w)
	AM_RANGE(0x1f801810, 0x1f801817) AM_READWRITE(psx_gpu_r, psx_gpu_w)
	AM_RANGE(0x1f801820, 0x1f801827) AM_READWRITE(psx_mdec_r, psx_mdec_w)
	AM_RANGE(0x1f801c00, 0x1f801dff) AM_READWRITE(psx_spu_r, psx_spu_w)
	AM_RANGE(0x1f802020, 0x1f802033) AM_RAM /* ?? */
	AM_RANGE(0x1f802040, 0x1f802043) AM_WRITENOP
	AM_RANGE(0x1fc00000, 0x1fc7ffff) AM_ROM AM_SHARE(2) AM_REGION(REGION_USER1, 0) /* bios */
	AM_RANGE(0x80000000, 0x801fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0x9fc00000, 0x9fc7ffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xa0000000, 0xa01fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0xbfc00000, 0xbfc7ffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xfffe0130, 0xfffe0133) AM_WRITENOP
ADDRESS_MAP_END

static MACHINE_INIT( psx )
{
	psx_machine_init();
}

static DRIVER_INIT( psx )
{
	psx_driver_init();
}

INPUT_PORTS_START( psx )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE )	/* pause */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE	)	/* pause */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START   | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START   | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )

	PORT_START		/* DSWA */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* DSWB */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* DSWC */
	PORT_DIPNAME( 0xff, 0xff, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0xff, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )

	PORT_START		/* Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
INPUT_PORTS_END

static struct PSXSPUinterface psxspu_interface =
{
	100,
};

static MACHINE_DRIVER_START( psx )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( psx_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( psx )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 480 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2_1024x512 )
	MDRV_VIDEO_UPDATE( psx )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
	MDRV_SOUND_ADD( PSXSPU, psxspu_interface )
MACHINE_DRIVER_END

ROM_START( psx )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph1000.bin",  0x0000000, 0x080000, CRC(3b601fc8) SHA1(343883a7b555646da8cee54aadd2795b6e7dd070) )
ROM_END

ROM_START( psxe20 )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph1002.bin",  0x0000000, 0x080000, CRC(9bb87c4b) SHA1(20b98f3d80f11cbf5a7bfd0779b0e63760ecc62c) )
ROM_END

ROM_START( psxj22 )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
//	ROM_LOAD( "scph5000.bin",  0x0000000, 0x080000, BAD_DUMP CRC(8c93a399) SHA1(e340db2696274dda5fdc25e434a914db71e8b02b) )
	ROM_LOAD( "scph5000.bin",  0x0000000, 0x080000, CRC(24fc7e17) SHA1(ffa7f9a7fb19d773a0c3985a541c8e5623d2c30d) )
ROM_END

ROM_START( psxa22 )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph1001.bin",  0x0000000, 0x080000, CRC(37157331) SHA1(10155d8d6e6e832d6ea66db9bc098321fb5e8ebf) )
ROM_END

ROM_START( psxe22 )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "dtlh3002.bin",  0x0000000, 0x080000, CRC(1e26792f) SHA1(b6a11579caef3875504fcf3831b8e3922746df2c) )
ROM_END

ROM_START( psxj30 )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph5500.bin",  0x0000000, 0x080000, CRC(ff3eeb8c) SHA1(b05def971d8ec59f346f2d9ac21fb742e3eb6917) )
ROM_END

ROM_START( psxa30 )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph7003.bin",  0x0000000, 0x080000, CRC(8d8cb7e4) SHA1(0555c6fae8906f3f09baf5988f00e55f88e9f30b) )
ROM_END

ROM_START( psxe30 )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
//	ROM_LOAD( "scph5502.bin",  0x0000000, 0x080000, BAD_DUMP CRC(4d9e7c86) SHA1(f8de9325fc36fcfa4b29124d291c9251094f2e54) )
	ROM_LOAD( "scph5552.bin",  0x0000000, 0x080000, CRC(d786f0b9) SHA1(f6bc2d1f5eb6593de7d089c425ac681d6fffd3f0) )
ROM_END

ROM_START( psxj40 )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph7000.bin",  0x0000000, 0x080000, CRC(ec541cd0) SHA1(77b10118d21ac7ffa9b35f9c4fd814da240eb3e9) )
ROM_END

ROM_START( psxa41 )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph7001.bin",  0x0000000, 0x080000, CRC(502224b6) SHA1(14df4f6c1e367ce097c11deae21566b4fe5647a9) )
ROM_END

ROM_START( psxe41 )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph7502.bin",  0x0000000, 0x080000, CRC(318178bf) SHA1(8d5de56a79954f29e9006929ba3fed9b6a418c1d) )
ROM_END

ROM_START( psxa45 )
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 )
	ROM_LOAD( "scph101.bin",  0x0000000, 0x080000, CRC(171bdcec) SHA1(dcffe16bd90a723499ad46c641424981338d8378) )
ROM_END

SYSTEM_CONFIG_START( psx )
	CONFIG_DEVICE_QUICKLOAD( "exe\0", psxexe_load )
SYSTEM_CONFIG_END

/*
The version number & release date is stored in ascii text at the end of every bios, except for scph1000.
There is also a BCD encoded date at offset 0x100, but this is set to 041211995 in every version apart
from scph1000 & scph7000 ( where it is 22091994 & 29051997 respectively ).

missing:
 scph5501
 scph7002
 scph7501
 scph9000
 scph9003
 scph100
 scph102

*/

/*     YEAR  NAME    PARENT  COMPAT  MACHINE  INPUT  INIT  CONFIG  COMPANY   FULLNAME */
/* PU-7/PU-8 */
CONSX( 1994, psx,    0,      0,      psx,     psx,   psx,  psx,    "Sony",   "Sony PSX (scph1000)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONSX( 1995, psxe20, psx,    0,      psx,     psx,   psx,  psx,    "Sony",   "Sony PSX (scph1002 E v2.0 05/10/95)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONSX( 1995, psxa22, psx,    0,      psx,     psx,   psx,  psx,    "Sony",   "Sony PSX (scph1001/dtlh3000 A v2.2 12/04/95)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONSX( 1995, psxe22, psx,    0,      psx,     psx,   psx,  psx,    "Sony",   "Sony PSX (scph1002/dtlh3002 E v2.2 12/04/95)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
/* PU-18 */
CONSX( 1995, psxj22, psx,    0,      psx,     psx,   psx,  psx,    "Sony",   "Sony PSX (scph5000 J v2.2 12/04/95)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONSX( 1996, psxj30, psx,    0,      psx,     psx,   psx,  psx,    "Sony",   "Sony PSX (scph5500 J v3.0 09/09/96)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONSX( 1997, psxe30, psx,    0,      psx,     psx,   psx,  psx,    "Sony",   "Sony PSX (scph5502/scph5552 E v3.0 01/06/97)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
/* PU-20 */
CONSX( 1996, psxa30, psx,    0,      psx,     psx,   psx,  psx,    "Sony",   "Sony PSX (scph7003 A v3.0 11/18/96)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONSX( 1997, psxj40, psx,    0,      psx,     psx,   psx,  psx,    "Sony",   "Sony PSX (scph7000 J v4.0 08/18/97)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
CONSX( 1997, psxa41, psx,    0,      psx,     psx,   psx,  psx,    "Sony",   "Sony PSX (scph7001 A v4.1 12/16/97)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
/* PU-22 */
CONSX( 1997, psxe41, psx,    0,      psx,     psx,   psx,  psx,    "Sony",   "Sony PSX (scph7502 E v4.1 12/16/97)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
/* PU-23 */
/* PM-41 */
CONSX( 2000, psxa45, psx,    0,      psx,     psx,   psx,  psx,    "Sony",   "Sony PS one (scph101 A v4.5 05/25/00)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
