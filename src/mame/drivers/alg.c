/**************************************************************************************

    American Laser Game Hardware

    Amiga 500 + sony ldp1450 laserdisc palyer

    Games Supported:

        Mad Dog McCree [3 versions]
        Who Shot Johnny Rock? [2 versions]
        Mad Dog II: The Lost Gold [2 versions]
        Space Pirates
        Gallagher's Gallery
        Crime Patrol
        Crime Patrol 2: Drug Wars [2 versions]
        The Last Bounty Hunter
        Fast Draw Showdown
        Platoon
        Zorton Brothers (Los Justicieros)

**************************************************************************************/

#include "driver.h"
#include "render.h"
#include "includes/amiga.h"
#include "machine/laserdsc.h"


static laserdisc_info *discinfo;
static mame_timer *serial_timer;
static UINT8 serial_timer_active;
static UINT16 input_select;

static render_texture *video_texture;
static render_texture *overlay_texture;

static void video_cleanup(running_machine *machine);
static void response_timer(int param);



/*************************************
 *
 *  Lightgun reading
 *
 *************************************/

static int get_lightgun_pos(int player, int *x, int *y)
{
	int xpos = readinputportbytag_safe((player == 0) ? "GUN1X" : "GUN2X", -1);
	int ypos = readinputportbytag_safe((player == 0) ? "GUN1Y" : "GUN2Y", -1);

	if (xpos == -1 || ypos == -1)
		return FALSE;

	*x = Machine->screen[0].visarea.min_x + xpos * (Machine->screen[0].visarea.max_x - Machine->screen[0].visarea.min_x + 1) / 255;
	*y = Machine->screen[0].visarea.min_y + ypos * (Machine->screen[0].visarea.max_y - Machine->screen[0].visarea.min_y + 1) / 255;
	return TRUE;
}



/*************************************
 *
 *  Video startup
 *
 *************************************/

static VIDEO_START( alg )
{
	/* reset our globals */
	video_texture = NULL;
	overlay_texture = NULL;

	/* configure for cleanup */
	add_exit_callback(machine, video_cleanup);

	/* standard video start */
	if (video_start_amiga(machine))
		return 1;

	/* configure pen 4096 as transparent in the renderer and use it for the genlock color */
	render_container_set_palette_alpha(render_container_get_screen(0), 4096, 0x00);
	amiga_set_genlock_color(4096);
	return 0;
}


static void video_cleanup(running_machine *machine)
{
	if (video_texture != NULL)
		render_texture_free(video_texture);
	if (overlay_texture != NULL)
		render_texture_free(overlay_texture);
}



/*************************************
 *
 *  Video update
 *
 *************************************/

static VIDEO_UPDATE( alg )
{
	/* composite the video */
	if (!video_skip_this_frame())
	{
		mame_bitmap *vidbitmap;
		rectangle fixedvis = Machine->screen[screen].visarea;
		fixedvis.max_x++;
		fixedvis.max_y++;

		/* first lay down the video data */
		laserdisc_get_video(discinfo, &vidbitmap);
		if (video_texture == NULL)
			video_texture = render_texture_alloc(vidbitmap, NULL, 0, TEXFORMAT_YUY16, NULL, NULL);
		else
			render_texture_set_bitmap(video_texture, vidbitmap, NULL, 0, TEXFORMAT_YUY16);

		/* then overlay the Amiga video */
		if (overlay_texture == NULL)
			overlay_texture = render_texture_alloc(tmpbitmap, &fixedvis, 0, TEXFORMAT_PALETTEA16, NULL, NULL);
		else
			render_texture_set_bitmap(overlay_texture, tmpbitmap, &fixedvis, 0, TEXFORMAT_PALETTEA16);

		/* add both quads to the screen */
		render_screen_add_quad(0, 0.0f, 0.0f, 1.0f, 1.0f, MAKE_ARGB(0xff,0xff,0xff,0xff), video_texture, PRIMFLAG_BLENDMODE(BLENDMODE_NONE) | PRIMFLAG_SCREENTEX(1));
		render_screen_add_quad(0, 0.0f, 0.0f, 1.0f, 1.0f, MAKE_ARGB(0xff,0xff,0xff,0xff), overlay_texture, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_SCREENTEX(1));
	}

	/* display disc information */
	if (discinfo != NULL)
		popmessage("%s", laserdisc_describe_state(discinfo));

	return 0;
}



/*************************************
 *
 *  Machine start/reset
 *
 *************************************/

static MACHINE_START( alg )
{
	discinfo = laserdisc_init(LASERDISC_TYPE_LDP1450, get_disk_handle(0), 1);
	serial_timer = timer_alloc(response_timer);
	serial_timer_active = FALSE;

	return 0;
}


static MACHINE_RESET( alg )
{
	machine_reset_amiga(machine);
	laserdisc_reset(discinfo, 0);
}



/*************************************
 *
 *  Laserdisc communication
 *
 *************************************/

static void response_timer(int param)
{
	/* if we still have data to send, do it now */
	if (laserdisc_line_r(discinfo, LASERDISC_LINE_DATA_AVAIL) == ASSERT_LINE)
	{
		UINT8 data = laserdisc_data_r(discinfo);
		if (data != 0x0a)
			mame_printf_debug("Sending serial data = %02X\n", data);
		amiga_serial_in_w(data);
	}

	/* if there's more to come, set another timer */
	if (laserdisc_line_r(discinfo, LASERDISC_LINE_DATA_AVAIL) == ASSERT_LINE)
		timer_adjust(serial_timer, amiga_get_serial_char_period(), 0, 0);
	else
		serial_timer_active = FALSE;
}


static void vsync_callback(void)
{
	/* only clock the disc every other frame */
	laserdisc_vsync(discinfo);

	/* if we have data available, set a timer to read it */
	if (!serial_timer_active && laserdisc_line_r(discinfo, LASERDISC_LINE_DATA_AVAIL) == ASSERT_LINE)
	{
		timer_adjust(serial_timer, amiga_get_serial_char_period(), 0, 0);
		serial_timer_active = TRUE;
	}
}


static void serial_w(UINT16 data)
{
	/* write to the laserdisc player */
	laserdisc_data_w(discinfo, data & 0xff);

	/* if we have data available, set a timer to read it */
	if (!serial_timer_active && laserdisc_line_r(discinfo, LASERDISC_LINE_DATA_AVAIL) == ASSERT_LINE)
	{
		timer_adjust(serial_timer, amiga_get_serial_char_period(), 0, 0);
		serial_timer_active = TRUE;
	}
}



/*************************************
 *
 *  I/O ports
 *
 *************************************/

static void alg_potgo_w(UINT16 data)
{
	/* bit 15 controls whether pin 9 is input/output */
	/* bit 14 controls the value, which selects which player's controls to read */
	input_select = (data & 0x8000) ? ((data >> 14) & 1) : 0;
}


static UINT32 lightgun_pos_r(void *param)
{
	int x, y;

	/* get the position based on the input select */
	get_lightgun_pos(input_select, &x, &y);
	return (y << 8) | (x >> 2);
}


static UINT32 lightgun_trigger_r(void *param)
{
	/* read the trigger control based on the input select */
	return (readinputportbytag("TRIGGERS") >> input_select) & 1;
}


static UINT32 lightgun_holster_r(void *param)
{
	/* read the holster control based on the input select */
	return (readinputportbytag("TRIGGERS") >> (2 + input_select)) & 1;
}



/*************************************
 *
 *  CIA port accesses
 *
 *************************************/

static void alg_cia_0_porta_w(UINT8 data)
{
	/* switch banks as appropriate */
	memory_set_bank(1, data & 1);

	/* swap the write handlers between ROM and bank 1 based on the bit */
	if ((data & 1) == 0)
		/* overlay disabled, map RAM on 0x000000 */
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x000000, 0x07ffff, 0, 0, MWA16_BANK1);

	else
		/* overlay enabled, map Amiga system ROM on 0x000000 */
		memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x000000, 0x07ffff, 0, 0, MWA16_ROM);
}


static UINT8 alg_cia_0_porta_r(void)
{
	return readinputportbytag("FIRE") | 0x3f;
}


static UINT8 alg_cia_0_portb_r(void)
{
	logerror("%06x:alg_cia_0_portb_r\n", activecpu_get_pc());
	return 0xff;
}


static void alg_cia_0_portb_w(UINT8 data)
{
	/* parallel port */
	logerror("%06x:alg_cia_0_portb_w(%02x)\n", activecpu_get_pc(), data);
}


static UINT8 alg_cia_1_porta_r(void)
{
	logerror("%06x:alg_cia_1_porta_r\n", activecpu_get_pc());
	return 0xff;
}


static void alg_cia_1_porta_w(UINT8 data)
{
	logerror("%06x:alg_cia_1_porta_w(%02x)\n", activecpu_get_pc(), data);
}



/*************************************
 *
 *  Memory map
 *
 *************************************/

static ADDRESS_MAP_START( main_map_r1, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x000000, 0x07ffff) AM_RAMBANK(1) AM_BASE(&amiga_chip_ram)	AM_SIZE(&amiga_chip_ram_size)
	AM_RANGE(0xbfd000, 0xbfefff) AM_READWRITE(amiga_cia_r, amiga_cia_w)
	AM_RANGE(0xc00000, 0xdfffff) AM_READWRITE(amiga_custom_r, amiga_custom_w) AM_BASE(&amiga_custom_regs)
	AM_RANGE(0xe80000, 0xe8ffff) AM_READWRITE(amiga_autoconfig_r, amiga_autoconfig_w)
	AM_RANGE(0xfc0000, 0xffffff) AM_ROM AM_REGION(REGION_USER1, 0)			/* System ROM */

	AM_RANGE(0xf00000, 0xf1ffff) AM_ROM AM_REGION(REGION_USER2, 0)			/* Custom ROM */
	AM_RANGE(0xf54000, 0xf55fff) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
ADDRESS_MAP_END


static ADDRESS_MAP_START( main_map_r2, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x000000, 0x07ffff) AM_RAMBANK(1) AM_BASE(&amiga_chip_ram)	AM_SIZE(&amiga_chip_ram_size)
	AM_RANGE(0xbfd000, 0xbfefff) AM_READWRITE(amiga_cia_r, amiga_cia_w)
	AM_RANGE(0xc00000, 0xdfffff) AM_READWRITE(amiga_custom_r, amiga_custom_w) AM_BASE(&amiga_custom_regs)
	AM_RANGE(0xe80000, 0xe8ffff) AM_READWRITE(amiga_autoconfig_r, amiga_autoconfig_w)
	AM_RANGE(0xfc0000, 0xffffff) AM_ROM AM_REGION(REGION_USER1, 0)			/* System ROM */

	AM_RANGE(0xf00000, 0xf3ffff) AM_ROM AM_REGION(REGION_USER2, 0)			/* Custom ROM */
	AM_RANGE(0xf7c000, 0xf7dfff) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
ADDRESS_MAP_END


static ADDRESS_MAP_START( main_map_picmatic, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x000000, 0x07ffff) AM_RAMBANK(1) AM_BASE(&amiga_chip_ram)	AM_SIZE(&amiga_chip_ram_size)
	AM_RANGE(0xbfd000, 0xbfefff) AM_READWRITE(amiga_cia_r, amiga_cia_w)
	AM_RANGE(0xc00000, 0xdfffff) AM_READWRITE(amiga_custom_r, amiga_custom_w) AM_BASE(&amiga_custom_regs)
	AM_RANGE(0xe80000, 0xe8ffff) AM_READWRITE(amiga_autoconfig_r, amiga_autoconfig_w)
	AM_RANGE(0xfc0000, 0xffffff) AM_ROM AM_REGION(REGION_USER1, 0)			/* System ROM */

	AM_RANGE(0xf00000, 0xf1ffff) AM_ROM AM_REGION(REGION_USER2, 0)			/* Custom ROM */
	AM_RANGE(0xf40000, 0xf41fff) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
ADDRESS_MAP_END



/*************************************
 *
 *  Input ports
 *
 *************************************/

INPUT_PORTS_START( alg )
	PORT_START_TAG("JOY0DAT")	/* read by Amiga core */
	PORT_BIT( 0x0303, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(amiga_joystick_convert, "P1JOY")
	PORT_BIT( 0xfcfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("JOY1DAT")	/* read by Amiga core */
	PORT_BIT( 0x0303, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(amiga_joystick_convert, "P2JOY")
	PORT_BIT( 0xfcfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("POTGO")		/* read by Amiga core */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xaaff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("HVPOS")		/* read by Amiga core */
	PORT_BIT( 0x1ffff, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(lightgun_pos_r, NULL)

	PORT_START_TAG("FIRE")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("P1JOY")		/* referenced by JOY0DAT */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START_TAG("P2JOY")		/* referenced by JOY1DAT */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("GUN1X")		/* referenced by lightgun_pos_r */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START_TAG("GUN1Y")		/* referenced by lightgun_pos_r */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_PLAYER(1)
INPUT_PORTS_END


INPUT_PORTS_START( alg_2p )
	PORT_INCLUDE(alg)

	PORT_MODIFY("POTGO")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_CUSTOM(lightgun_trigger_r, NULL)

	PORT_MODIFY("FIRE")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_MODIFY("P2JOY")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(lightgun_holster_r, NULL)

	PORT_START_TAG("GUN2X")		/* referenced by lightgun_pos_r */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START_TAG("GUN2Y")		/* referenced by lightgun_pos_r */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START_TAG("TRIGGERS")	/* referenced by lightgun_trigger_r and lightgun_holster_r */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
INPUT_PORTS_END



/*************************************
 *
 *  Sound definitions
 *
 *************************************/

static struct CustomSound_interface amiga_custom_interface =
{
	amiga_sh_start
};



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( alg_r1 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 7159090)        /* 7.15909 Mhz (NTSC) */
	MDRV_CPU_PROGRAM_MAP(main_map_r1,0)
	MDRV_CPU_VBLANK_INT(amiga_scanline_callback, 262)

	MDRV_SCREEN_REFRESH_RATE(59.97)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(0))

	MDRV_MACHINE_START(alg)
	MDRV_MACHINE_RESET(alg)
	MDRV_NVRAM_HANDLER(generic_0fill)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512*2, 262)
	MDRV_SCREEN_VISIBLE_AREA((129-8)*2, (449+8-1)*2, 44-8, 244+8-1)
	MDRV_PALETTE_LENGTH(4097)
	MDRV_PALETTE_INIT(amiga)

	MDRV_VIDEO_START(alg)
	MDRV_VIDEO_UPDATE(alg)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(CUSTOM, 3579545)
	MDRV_SOUND_CONFIG(amiga_custom_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.25)
	MDRV_SOUND_ROUTE(1, "right", 0.25)
	MDRV_SOUND_ROUTE(2, "right", 0.25)
	MDRV_SOUND_ROUTE(3, "left", 0.25)

	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(laserdisc_custom_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( alg_r2 )
	MDRV_IMPORT_FROM(alg_r1)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(main_map_r2,0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( picmatic )
	MDRV_IMPORT_FROM(alg_r1)

	/* adjust for PAL specs */
	MDRV_CPU_REPLACE("main", M68000, 7090000)
	MDRV_CPU_PROGRAM_MAP(main_map_picmatic,0)
	MDRV_CPU_VBLANK_INT(amiga_scanline_callback, 312)

	MDRV_SCREEN_REFRESH_RATE(50)

	MDRV_SCREEN_SIZE(512*2, 312)
	MDRV_SCREEN_VISIBLE_AREA((129-8)*2, (449+8-1)*2, 44-8, 300+8-1)
MACHINE_DRIVER_END



/*************************************
 *
 *  BIOS definitions
 *
 *************************************/

#define ALG_BIOS \
	ROM_REGION16_BE( 0x80000, REGION_USER1, 0 ) \
	ROM_LOAD16_WORD( "kick13.rom", 0x000000, 0x40000, CRC(c4f0f55f) SHA1(891e9a547772fe0c6c19b610baf8bc4ea7fcb785)) \
	ROM_COPY( REGION_USER1, 0x000000, 0x040000, 0x040000 )


SYSTEM_BIOS_START( alg_bios )
	SYSTEM_BIOS_ADD(0, "Kick1.3", "Kickstart 1.3" )
SYSTEM_BIOS_END



/*************************************
*
*  ROM definitions
*
*************************************/

/* BIOS */
ROM_START( alg_bios )
	ALG_BIOS
ROM_END


/* Rev. A board */
/* PAL R1 */
ROM_START( maddoga )
	ALG_BIOS

	ROM_REGION( 0x20000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "maddog_01.dat", 0x000000, 0x10000, CRC(04572557) SHA1(3dfe2ce94ced8701a3e73ed5869b6fbe1c8b3286) )
	ROM_LOAD16_BYTE( "maddog_02.dat", 0x000001, 0x10000, CRC(f64014ec) SHA1(d343a2cb5d8992153b8c916f39b11d3db736543d))

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "maddog", 0, NO_DUMP )
ROM_END


/* PAL R3 */
ROM_START( wsjr )  /* 1.6 */
	ALG_BIOS

	ROM_REGION( 0x20000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "johnny_01.bin", 0x000000, 0x10000, CRC(edde1745) SHA1(573b79f8808fedaabf3b762350a915792d26c1bc) )
	ROM_LOAD16_BYTE( "johnny_02.bin", 0x000001, 0x10000, CRC(046569b3) SHA1(efe5a8b2be1c555695f2a91c88951d3545f1b915) )
ROM_END

ROM_START( wsjr15 )  /* 1.5 */
	ALG_BIOS

	ROM_REGION( 0x20000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "wsjr151.bin", 0x000000, 0x10000, CRC(9beeb1d7) SHA1(3fe0265e5d36103d3d9557d75e5e3728e0b30da7) )
	ROM_LOAD16_BYTE( "wsjr152.bin", 0x000001, 0x10000, CRC(8ab626dd) SHA1(e45561f77fc279b71dc1dd2e15a0870cb5c1cd89) )
ROM_END


//REV.B
ROM_START( maddog )
	ALG_BIOS

	ROM_REGION( 0x40000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "md_2.03_1.bin", 0x000000, 0x20000, CRC(6f5b8f2d) SHA1(bbf32bb27a998d53744411d75efdbdb730855809) )
	ROM_LOAD16_BYTE( "md_2.03_2.bin", 0x000001, 0x20000, CRC(a50d3c04) SHA1(4cf100fdb5b2f2236539fd0ec33b3db19c64a6b8) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "maddog", 0, NO_DUMP )
ROM_END


ROM_START( maddog2 )
	ALG_BIOS

	ROM_REGION( 0x40000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "md2_01_v.2.04.bin", 0x000000, 0x20000, CRC(0e1227f4) SHA1(bfd9081bb7d2bcbb77357839f292ce6136e9b228) )
	ROM_LOAD16_BYTE( "md2_02_v.2.04.bin", 0x000001, 0x20000, CRC(361bd99c) SHA1(5de6ef38e334e19f509227de7880306ac984ec23) )
ROM_END

ROM_START( maddog22 )
	ALG_BIOS

	ROM_REGION( 0x40000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "md2_01.bin", 0x000000, 0x20000, CRC(4092227f) SHA1(6e5393aa5e64b59887260f483c50960084de7bd1) )
	ROM_LOAD16_BYTE( "md2_02.bin", 0x000001, 0x20000, CRC(addffa51) SHA1(665e9d93ddfa6b2ea5d006b41bf7eac3294244cc) )
ROM_END

ROM_START( maddog21 )
	ALG_BIOS

	ROM_REGION( 0x40000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "md2_1.0_1.bin", 0x000000, 0x20000, CRC(97272a1d) SHA1(109014647c491f019ffb21091c7d0b89e1755b75) )
	ROM_LOAD16_BYTE( "md2_1.0_2.bin", 0x000001, 0x20000, CRC(0ce8db97) SHA1(dd4c09db59bb8c6caba935b1b28babe28ba8516b) )
ROM_END


ROM_START( spacepir )
	ALG_BIOS

	ROM_REGION( 0x40000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "sp_02.dat", 0x000000, 0x20000, CRC(10d162a2) SHA1(26833d5be1057be8639c00a7be18be33404ea751) )
	ROM_LOAD16_BYTE( "sp_01.dat", 0x000001, 0x20000, CRC(c0975188) SHA1(fd7643dc972e7861249ab7e76199975984888ae4) )
ROM_END


ROM_START( gallgall )
	ALG_BIOS

	ROM_REGION( 0x40000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "gg_1.dat", 0x000000, 0x20000, CRC(3793b211) SHA1(dccb1d9c5e2d6a4d249426ae6348e9fc9b72e665)  )
	ROM_LOAD16_BYTE( "gg_2.dat", 0x000001, 0x20000,  CRC(855c9d82) SHA1(96711aaa02f309cacd3e8d8efbe95cfc811aba96) )
ROM_END


ROM_START( crimepat )
	ALG_BIOS

	ROM_REGION( 0x40000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "cp02.dat", 0x000000, 0x20000, CRC(a39a8b50) SHA1(55ca317ef13c3a42f12d68c480e6cc2d4459f6a4) )
	ROM_LOAD16_BYTE( "cp01.dat", 0x000001, 0x20000, CRC(e41fd2e8) SHA1(1cd9875fb4133ba4e3616271975dc736b343f156) )
ROM_END


ROM_START( crimep2 )
	ALG_BIOS

	ROM_REGION( 0x40000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "cp2_1.3_1.bin", 0x000000, 0x20000, CRC(e653395d) SHA1(8f6c86d98a52b7d85ae285fd841167cd07979318) )
	ROM_LOAD16_BYTE( "cp2_1.3_2.bin", 0x000001, 0x20000, CRC(dbdaa79a) SHA1(998044909d5c93e3bd1baafefab818fdb7b3f55e) )
ROM_END

ROM_START( crime211 )
	ALG_BIOS

	ROM_REGION( 0x40000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "cp2_1.dat", 0x000000, 0x20000, CRC(47879042) SHA1(8bb6c541e4e8e4508da8d4b93600176a2e7a1f41) )
	ROM_LOAD16_BYTE( "cp2_2.dat", 0x000001, 0x20000, CRC(f4e5251e) SHA1(e0c91343a98193d487c40e7a85f542b2a7a88f03) )
ROM_END


ROM_START( lastbh )
	ALG_BIOS

	ROM_REGION( 0x40000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "bounty_01.bin", 0x000000, 0x20000, CRC(977566b2) SHA1(937e079e992ecb5930b17c1024c326e10962642b) )
	ROM_LOAD16_BYTE( "bounty_02.bin", 0x000001, 0x20000, CRC(2727ef1d) SHA1(f53421390b65c21a7666ff9d0f53ebf2a463d836) )
ROM_END


ROM_START( fastdraw )
	ALG_BIOS

	ROM_REGION( 0x40000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "fast_01.bin", 0x000000, 0x20000, CRC(4c4eb71e) SHA1(3bd487c546b6c80770a5fc880dcb10395ca431a2) )
	ROM_LOAD16_BYTE( "fast_02.bin", 0x000001, 0x20000, CRC(0d76a2da) SHA1(d396371ae1b9b0b6e6bc6f1f85c4b97bfc5dc34d) )
ROM_END


ROM_START( aplatoon )
	ALG_BIOS

	ROM_REGION( 0x40000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "platoonv4u1.bin", 0x000000, 0x20000, CRC(8b33263e) SHA1(a1df38236321af90b522e2a783984fdf02e4c597) )
	ROM_LOAD16_BYTE( "platoonv4u2.bin", 0x000001, 0x20000, CRC(09a133cf) SHA1(9b3ff63035be8576c88fb284a25c2da5db0d5160) )
ROM_END


ROM_START( zortonbr )
	ALG_BIOS

	ROM_REGION( 0x40000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "zb_u2.bin", 0x000000, 0x10000, CRC(938b25cb) SHA1(d0114bbc588dcfce6a469013d0e35afb93e38af5) )
	ROM_LOAD16_BYTE( "zb_u3.bin", 0x000001, 0x10000, CRC(f59cfc4a) SHA1(9fadf7f1e23d6b4e828bf2b3de919d087c690a3f) )
ROM_END



/*************************************
 *
 *  Generic driver init
 *
 *************************************/

static void alg_init(void)
{
	static const amiga_machine_interface alg_intf =
	{
		ANGUS_CHIP_RAM_MASK,
		alg_cia_0_porta_r, alg_cia_0_portb_r,
		alg_cia_0_porta_w, alg_cia_0_portb_w,
		alg_cia_1_porta_r, NULL,
		alg_cia_1_porta_w, NULL,
		NULL, NULL, alg_potgo_w,
		NULL, NULL, serial_w,

		vsync_callback,
		NULL
	};
	amiga_machine_config(&alg_intf);

	/* set up memory */
	memory_configure_bank(1, 0, 1, amiga_chip_ram, 0);
	memory_configure_bank(1, 1, 1, memory_region(REGION_USER1), 0);
}



/*************************************
 *
 *  Per-game decryption
 *
 *************************************/

static DRIVER_INIT( palr1 )
{
	UINT32 length = memory_region_length(REGION_USER2);
	UINT8 *rom = memory_region(REGION_USER2);
	UINT8 *original = malloc_or_die(length);
	UINT32 srcaddr;

	memcpy(original, rom, length);
	for (srcaddr = 0; srcaddr < length; srcaddr++)
	{
		UINT32 dstaddr = srcaddr;
		if (srcaddr & 0x2000) dstaddr ^= 0x1000;
		if (srcaddr & 0x8000) dstaddr ^= 0x4000;
		rom[dstaddr] = original[srcaddr];
	}
	free(original);

	alg_init();
}

static DRIVER_INIT( palr3 )
{
	UINT32 length = memory_region_length(REGION_USER2);
	UINT8 *rom = memory_region(REGION_USER2);
	UINT8 *original = malloc_or_die(length);
	UINT32 srcaddr;

	memcpy(original, rom, length);
	for (srcaddr = 0; srcaddr < length; srcaddr++)
	{
		UINT32 dstaddr = srcaddr;
		if (srcaddr & 0x2000) dstaddr ^= 0x1000;
		rom[dstaddr] = original[srcaddr];
	}
	free(original);

	alg_init();
}

static DRIVER_INIT( palr6 )
{
	UINT32 length = memory_region_length(REGION_USER2);
	UINT8 *rom = memory_region(REGION_USER2);
	UINT8 *original = malloc_or_die(length);
	UINT32 srcaddr;

	memcpy(original, rom, length);
	for (srcaddr = 0; srcaddr < length; srcaddr++)
	{
		UINT32 dstaddr = srcaddr;
		if (~srcaddr & 0x2000) dstaddr ^= 0x1000;
		if ( srcaddr & 0x8000) dstaddr ^= 0x4000;
		dstaddr ^= 0x20000;
		rom[dstaddr] = original[srcaddr];
	}
	free(original);

	alg_init();
}

static DRIVER_INIT( aplatoon )
{
	/* NOT DONE TODO FIGURE OUT THE RIGHT ORDER!!!! */
	char *rom = memory_region(REGION_USER2);
	char *decrypted = auto_malloc(0x40000);
	int i;

	static const int shuffle[] =
	{
		0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
		32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63
	};

	for (i = 0; i < 64; i++)
		memcpy(decrypted + i * 0x1000, rom + shuffle[i] * 0x1000, 0x1000);
	memcpy(rom, decrypted, 0x40000);
	logerror("decrypt done\n ");
	alg_init();
}

static DRIVER_INIT( none )
{
	alg_init();
}



/*************************************
 *
 *  Game drivers
 *
 *************************************/

/* BIOS */
GAMEB( 199?, alg_bios, 0,        alg_bios, alg_r1,   alg,    0,        ROT0,  "American Laser Games", "American Laser Games BIOS", NOT_A_DRIVER )

/* Rev. A board */
/* PAL R1 */
GAMEB( 1990, maddoga,  maddog,   alg_bios, alg_r1,   alg,    palr1,    ROT0,  "American Laser Games", "Mad Dog McCree v1C board rev.A", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )

/* PAL R3 */
GAMEB( 1991, wsjr,     alg_bios, alg_bios, alg_r1,   alg,    palr3,    ROT0,  "American Laser Games", "Who Shot Johnny Rock? v1.6", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1991, wsjr15,   wsjr,     alg_bios, alg_r1,   alg,    palr3,    ROT0,  "American Laser Games", "Who Shot Johnny Rock? v1.5", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )

/* Rev. B board */
/* PAL R6 */
GAMEB( 1990, maddog,   alg_bios, alg_bios, alg_r2,   alg_2p, palr6,    ROT0,  "American Laser Games", "Mad Dog McCree v2.03 board rev.B", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
  /* works ok but uses right player (2) controls only for trigger and holster */
GAMEB( 1992, maddog2,  alg_bios, alg_bios, alg_r2,   alg_2p, palr6,    ROT0,  "American Laser Games", "Mad Dog II: The Lost Gold v2.04", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1992, maddog22, alg_bios, alg_bios, alg_r2,   alg_2p, palr6,    ROT0,  "American Laser Games", "Mad Dog II: The Lost Gold v2.02", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1992, maddog21, maddog2,  alg_bios, alg_r2,   alg_2p, palr6,    ROT0,  "American Laser Games", "Mad Dog II: The Lost Gold v1.0", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
  /* works ok but uses right player (2) controls only for trigger and holster */
GAMEB( 1992, spacepir, alg_bios, alg_bios, alg_r2,   alg_2p, palr6,    ROT0,  "American Laser Games", "Space Pirates v2.2", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1992, gallgall, alg_bios, alg_bios, alg_r2,   alg_2p, palr6,    ROT0,  "American Laser Games", "Gallagher's Gallery v2.2", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
  /* all good, but no holster */
GAMEB( 1993, crimepat, alg_bios, alg_bios, alg_r2,   alg_2p, palr6,    ROT0,  "American Laser Games", "Crime Patrol v1.4", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1993, crimep2,  alg_bios, alg_bios, alg_r2,   alg_2p, palr6,    ROT0,  "American Laser Games", "Crime Patrol 2: Drug Wars v1.3", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1993, crime211, crimep2,  alg_bios, alg_r2,   alg_2p, palr6,    ROT0,  "American Laser Games", "Crime Patrol 2: Drug Wars v1.1", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1994, lastbh,   alg_bios, alg_bios, alg_r2,   alg_2p, palr6,    ROT0,  "American Laser Games", "The Last Bounty Hunter v0.06", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEB( 1995, fastdraw, alg_bios, alg_bios, alg_r2,   alg_2p, palr6,    ROT90, "American Laser Games", "Fast Draw Showdown v1.3", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
  /* works ok but uses right player (2) controls only for trigger and holster */

/* NOVA games on ALG hardware with own address scramble */
GAMEB( 199?, aplatoon, alg_bios, alg_bios, alg_r2,   alg,    aplatoon, ROT0,  "Nova?", "Platoon V.?.? US", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )

/* Web Picmatic games PAL tv standard, own rom board */
GAMEB( 1993, zortonbr, alg_bios, alg_bios, picmatic, alg,    none,     ROT0,  "Web Picmatic", "Zorton Brothers (Los Justicieros)", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
