/***************************************************************************
    vtech1.c

    Video Technology Models (series 1)
    Laser 110 monochrome
    Laser 210
        Laser 200 (same hardware?)
        aka VZ 200 (Australia)
        aka Salora Fellow (Finland)
        aka Texet8000 (UK)
    Laser 310
        aka VZ 300 (Australia)

    system driver
    Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

    Thanks go to:
    - Guy Thomason
    - Jason Oakley
    - Bushy Maunder
    - and anybody else on the vzemu list :)
    - Davide Moretti for the detailed description of the colors.

    Laser 110 memory map (preliminary)

        0000-1FFF ROM 0 8K (VZ-200)
        2000-3FFF ROM 1 8K (VZ-200)

        4000-5FFF optional DOS ROM 8K
        6000-67FF reserved for rom cartridges (2k)
        6800-6FFF memory mapped I/O 2k
                  R: keyboard
                  W: cassette I/O, speaker, VDP control
        7000-77FF video RAM 2K
        7800-7FFF internal user RAM 2K (?)
        8800-C7FF 16K expansion

    Laser 210 / VZ 200 memory map (preliminary)

        0000-1FFF ROM 0 8K (VZ-200)
        2000-3FFF ROM 1 8K (VZ-200)

        4000-5FFF optional DOS ROM 8K
        6000-67FF reserved for rom cartridges (2k)
        6800-6FFF memory mapped I/O 2k
                  R: keyboard
                  W: cassette I/O, speaker, VDP control
        7000-77FF video RAM 2K
        7800-8FFF internal user RAM 6K
        9000-CFFF 16K expansion

    Laser 310 / VZ 300 memory map (preliminary)

        0000-3FFF ROM
        4000-5FFF optional DOS ROM 8K
        6000-67FF reserved for rom cartridges
        6800-6FFF memory mapped I/O
                  R: keyboard
                  W: cassette I/O, speaker, VDP control
        7000-77FF video RAM 2K
        7800-B7FF internal user RAM 16K
        B800-F7FF 16K expansion

    TODO:
        Add support for the 64K banked RAM extension.
        Add ROMs and drivers for the Laser100, 110, 210 and 310
        machines and the Texet 8000.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/m6847.h"
#include "includes/vtech1.h"
#include "devices/snapquik.h"
#include "devices/cassette.h"
#include "formats/vt_cas.h"

#define VERBOSE 0

#if VERBOSE
#define LOG(x)  logerror x
#else
#define LOG(x)  /* x */
#endif


static ADDRESS_MAP_START(laser110_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x5fff) AM_ROM
	AM_RANGE(0x6000, 0x67ff) AM_ROM
	AM_RANGE(0x6800, 0x6fff) AM_READWRITE(vtech1_keyboard_r, vtech1_latch_w)
	AM_RANGE(0x7000, 0x77ff) AM_READWRITE(videoram_r, videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size) /* VDG 6847 */
	AM_RANGE(0x7800, 0x7fff) AM_RAM
//	AM_RANGE(0x8000, 0xbfff) AM_RAM    /* opt. installed */
	AM_RANGE(0xc000, 0xffff) AM_NOP
ADDRESS_MAP_END


static ADDRESS_MAP_START(laser210_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x5fff) AM_ROM
	AM_RANGE(0x6000, 0x67ff) AM_ROM
	AM_RANGE(0x6800, 0x6fff) AM_READWRITE(vtech1_keyboard_r, vtech1_latch_w)
	AM_RANGE(0x7000, 0x77ff) AM_READWRITE(videoram_r, videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size) /* VDG 6847 */
	AM_RANGE(0x7800, 0x8fff) AM_RAM
//	AM_RANGE(0x9000, 0xcfff) AM_RAM    /* opt. installed */
	AM_RANGE(0xd000, 0xffff) AM_NOP
ADDRESS_MAP_END


static ADDRESS_MAP_START(laser310_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x5fff) AM_ROM
	AM_RANGE(0x6000, 0x67ff) AM_ROM
	AM_RANGE(0x6800, 0x6fff) AM_READWRITE(vtech1_keyboard_r, vtech1_latch_w)
	AM_RANGE(0x7000, 0x77ff) AM_READWRITE(videoram_r, videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size) /* VDG 6847 */
	AM_RANGE(0x7800, 0xb7ff) AM_RAM
//	AM_RANGE(0xb800, 0xf7ff) AM_RAM    /* opt. installed */
	AM_RANGE(0xf800, 0xffff) AM_NOP
ADDRESS_MAP_END


static ADDRESS_MAP_START(vtech1_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x10, 0x1f) AM_READWRITE(vtech1_fdc_r, vtech1_fdc_w)
	AM_RANGE(0x20, 0x2f) AM_READ(vtech1_joystick_r)
ADDRESS_MAP_END


INPUT_PORTS_START( vtech1 )
    PORT_START /* IN0 */
    PORT_DIPNAME( 0x80, 0x80, "16K RAM module")
    PORT_DIPSETTING(    0x00, DEF_STR( No ))
    PORT_DIPSETTING(    0x80, DEF_STR( Yes ))
    PORT_DIPNAME( 0x40, 0x40, "DOS extension")
    PORT_DIPSETTING(    0x00, DEF_STR( No ))
    PORT_DIPSETTING(    0x40, DEF_STR( Yes ))
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD | IPF_RESETCPU, "Reset",         KEYCODE_F3, IP_JOY_NONE )
    PORT_BIT( 0x1f, 0x1f, IPT_UNUSED )

    PORT_START /* IN1 KEY ROW 0 */
    PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "R       RETURN  LEFT$",   KEYCODE_R,          IP_JOY_NONE )
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q       FOR     CHR$",    KEYCODE_Q,          IP_JOY_NONE )
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "E       NEXT    LEN(",    KEYCODE_E,          IP_JOY_NONE )
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "n/a",                     IP_KEY_NONE,        IP_JOY_NONE )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "W       TO      VAL(",    KEYCODE_W,          IP_JOY_NONE )
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "T       THEN    MID$",    KEYCODE_T,          IP_JOY_NONE )

    PORT_START /* IN2 KEY ROW 1 */
    PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F       GOSUB   RND(",    KEYCODE_F,          IP_JOY_NONE )
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "A       MODE(   ASC(",    KEYCODE_A,          IP_JOY_NONE )
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "D               RESTORE", KEYCODE_D,          IP_JOY_NONE )
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "L-CTRL",                  KEYCODE_LCONTROL,   IP_JOY_NONE )
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "R-CTRL",                  KEYCODE_RCONTROL,   IP_JOY_NONE )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "S       STEP    SINS(",   KEYCODE_S,          IP_JOY_NONE )
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "G       GOTO    STOP",    KEYCODE_G,          IP_JOY_NONE )

    PORT_START /* IN3 KEY ROW 2 */
    PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "V       LPRINT  USR",     KEYCODE_V,          IP_JOY_NONE )
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z       PEEK(   INP",     KEYCODE_Z,          IP_JOY_NONE )
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C       CONT    COPY",    KEYCODE_C,          IP_JOY_NONE )
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "L-SHIFT",                 KEYCODE_LSHIFT,     IP_JOY_NONE )
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "R-SHIFT",                 KEYCODE_RSHIFT,     IP_JOY_NONE )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "X       POKE    OUT",     KEYCODE_X,          IP_JOY_NONE )
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "B       LLIST   SOUND",   KEYCODE_B,          IP_JOY_NONE )

    PORT_START /* IN4 KEY ROW 3 */
    PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "4  $    VERFY   ATN(",    KEYCODE_4,          IP_JOY_NONE )
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "1  !    CSAVE   SIN(",    KEYCODE_1,          IP_JOY_NONE )
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "3  #    CALL??  TAN(",    KEYCODE_3,          IP_JOY_NONE )
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "n/a",                                         IP_KEY_NONE,        IP_JOY_NONE )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2  \"    CLOAD   COS(",   KEYCODE_2,          IP_JOY_NONE )
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "5  %    LIST    LOG(",    KEYCODE_5,          IP_JOY_NONE )

    PORT_START /* IN5 KEY ROW 4 */
    PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "M       [Left]",          KEYCODE_M,          IP_JOY_NONE )
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE   [Down]",          KEYCODE_SPACE,      IP_JOY_NONE )
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ",       [Right]",         KEYCODE_COMMA,      IP_JOY_NONE )
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "n/a",                     IP_KEY_NONE,        IP_JOY_NONE )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ".       [Up]",            KEYCODE_STOP,       IP_JOY_NONE )
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "N       COLOR   USING",   KEYCODE_N,          IP_JOY_NONE )

    PORT_START /* IN6 KEY ROW 5 */
    PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "7  '    END     ???",     KEYCODE_7,          IP_JOY_NONE )
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "0  @    ????    INT(",    KEYCODE_0,          IP_JOY_NONE )
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "8  (    NEW     SQR(",    KEYCODE_8,          IP_JOY_NONE )
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "-  =    [Break]",         KEYCODE_MINUS,      IP_JOY_NONE )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9  )    READ    ABS(",    KEYCODE_9,          IP_JOY_NONE )
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "6  &    RUN     EXP(",    KEYCODE_6,          IP_JOY_NONE )

    PORT_START /* IN7 KEY ROW 6 */
    PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "U       IF      INKEY$",  KEYCODE_U,          IP_JOY_NONE )
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "P       PRINT   NOT",     KEYCODE_P,          IP_JOY_NONE )
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "I       INPUT   AND",     KEYCODE_I,          IP_JOY_NONE )
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "RETURN  [Function]",      KEYCODE_ENTER,      IP_JOY_NONE )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O       LET     OR",      KEYCODE_O,          IP_JOY_NONE )
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y       ELSE    RIGHT$",  KEYCODE_Y,          IP_JOY_NONE )

    PORT_START /* IN8 KEY ROW 7 */
    PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "J       REM     RESET",   KEYCODE_J,          IP_JOY_NONE )
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ";       [Rubout]",        KEYCODE_QUOTE,      IP_JOY_NONE )
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "K       TAB(    PRINT",   KEYCODE_K,          IP_JOY_NONE )
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ":       [Inverse]",       KEYCODE_COLON,      IP_JOY_NONE )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L       [Insert]",        KEYCODE_L,          IP_JOY_NONE )
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "H       CLS     SET",     KEYCODE_H,          IP_JOY_NONE )

    PORT_START /* IN9 EXTRA KEYS */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "(Inverse)",               KEYCODE_LALT,       IP_JOY_NONE )
    PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "(Rubout)",                KEYCODE_DEL,        IP_JOY_NONE )
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "(Cursor left)",           KEYCODE_LEFT,       IP_JOY_NONE )
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "(Cursor down)",           KEYCODE_DOWN,       IP_JOY_NONE )
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "(Cursor right)",          KEYCODE_RIGHT,      IP_JOY_NONE )
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "(Backspace)",             KEYCODE_BACKSPACE,  IP_JOY_NONE )
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(Cursor up)",             KEYCODE_UP,         IP_JOY_NONE )
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "(Insert)",                KEYCODE_INSERT,     IP_JOY_NONE )

    PORT_START /* IN10 JOYSTICK #1 */
    PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPF_PLAYER1 | IPT_BUTTON1 )
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPF_PLAYER1 | IPT_JOYSTICK_RIGHT )
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPF_PLAYER1 | IPT_JOYSTICK_LEFT )
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPF_PLAYER1 | IPT_JOYSTICK_DOWN )
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPF_PLAYER1 | IPT_JOYSTICK_UP )

    PORT_START /* IN11 JOYSTICK #1 'Arm' */
    PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPF_PLAYER1 | IPT_BUTTON2 )
    PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED )

    PORT_START /* IN12 JOYSTICK #2 */
    PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_BUTTON1 )
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_RIGHT )
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_LEFT )
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_DOWN )
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_UP )

    PORT_START /* IN13 JOYSTICK #2 'Arm' */
    PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_BUTTON2 )
    PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/* note - Juergen's colors do not match the colors in the m6847 code */
static unsigned char vt_palette[] =
{
      0,  0,  0,    /* black (block graphics) */
      0,224,  0,    /* green */
    208,255,  0,    /* yellow (greenish) */
      0,  0,255,    /* blue */
    255,  0,  0,    /* red */
    224,224,144,    /* buff */
      0,255,160,    /* cyan (greenish) */
    255,  0,255,    /* magenta */
    240,112,  0,    /* orange */
      0, 64,  0,    /* dark green (alphanumeric characters) */
      0,224, 24,    /* bright green (alphanumeric characters) */
     64, 16,  0,    /* dark orange (alphanumeric characters) */
    255,196, 24,    /* bright orange (alphanumeric characters) */
};

/* Initialise the palette */
static PALETTE_INIT( monochrome )
{
    int i;
    for (i = 0; i < sizeof(vt_palette)/(sizeof(unsigned char)*3); i++)
    {
        int mono;
        mono = (int)(vt_palette[i*3+0] * 0.299 + vt_palette[i*3+1] * 0.587 + vt_palette[i*3+2] * 0.114);
		palette_set_color(i, mono, mono, mono);
    }
}

/* Initialise the palette */
static PALETTE_INIT( color )
{
	palette_set_colors(0, vt_palette, sizeof(vt_palette) / (sizeof(vt_palette[0]) * 3));
}

static INT16 speaker_levels[] = {-32768,0,32767,0};

static struct Speaker_interface speaker_interface = {
    1,
    { 75 },
    { 4 },
    { speaker_levels }
};

static struct Wave_interface wave_interface = {
    1,
    { 25 }
};

static MACHINE_DRIVER_START( laser110 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3579500)        /* 3.57950 Mhz */
	MDRV_CPU_PROGRAM_MAP(laser110_mem, 0)
	MDRV_CPU_IO_MAP(vtech1_io, 0)
	MDRV_CPU_VBLANK_INT(vtech1_interrupt,1)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( laser110 )

    /* video hardware */
	MDRV_M6847_PAL( vtech1 )
	MDRV_PALETTE_INIT(monochrome)

	/* sound hardware */
	MDRV_SOUND_ADD(SPEAKER, speaker_interface)
	MDRV_SOUND_ADD(WAVE, wave_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( laser210 )
	MDRV_IMPORT_FROM( laser110 )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(laser210_mem, 0)

	MDRV_MACHINE_INIT( laser210 )
	MDRV_PALETTE_INIT( color )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( laser310 )
	MDRV_IMPORT_FROM( laser110 )
	MDRV_CPU_REPLACE( "main", Z80, 17734000/5 )	/* 17.734MHz / 5 = 3.54690 Mhz */
	MDRV_CPU_PROGRAM_MAP(laser310_mem, 0)

	MDRV_MACHINE_INIT( laser310 )
	MDRV_PALETTE_INIT( color )
MACHINE_DRIVER_END


ROM_START( laser110 )
    ROM_REGION(0x10000,REGION_CPU1,0)
    ROM_LOAD("vtechv12.lo",  0x0000, 0x2000, CRC(99412d43))
    ROM_LOAD("vtechv12.hi",  0x2000, 0x2000, CRC(e4c24e8b))
#ifdef OLD_VIDEO
    ROM_REGION(0x0d00,REGION_GFX1,0)
    ROM_LOAD("vtech1.chr",   0x0000, 0x0c00, CRC(ead006a1))
#endif
ROM_END
#define rom_laser200    rom_laser110
#define rom_tx8000      rom_laser110

ROM_START( laser210 )
    ROM_REGION(0x10000,REGION_CPU1,0)
    ROM_LOAD("vtechv20.lo",  0x0000, 0x2000, CRC(cc854fe9))
    ROM_LOAD("vtechv20.hi",  0x2000, 0x2000, CRC(7060f91a))
#ifdef OLD_VIDEO
    ROM_REGION(0x0d00,REGION_GFX1,0)
    ROM_LOAD("vtech1.chr",   0x0000, 0x0c00, CRC(ead006a1))
#endif
ROM_END
#define rom_vz200       rom_laser210
#define rom_fellow      rom_laser210

ROM_START( laser310 )
    ROM_REGION(0x10000,REGION_CPU1,0)
    ROM_LOAD("vtechv20.rom", 0x0000, 0x4000, CRC(613de12c))
#ifdef OLD_VIDEO
    ROM_REGION(0x0d00,REGION_GFX1,0)
    ROM_LOAD("vtech1.chr",   0x0000, 0x0c00, CRC(ead006a1))
#endif
ROM_END
#define rom_vz300       rom_laser310

/***************************************************************************

  Game driver(s)

***************************************************************************/

SYSTEM_CONFIG_START(vtech1)
	CONFIG_DEVICE_CASSETTE(			1,	vtech1_cassette_formats )
	CONFIG_DEVICE_SNAPSHOT_DELAY(		"vz\0",  vtech1, 0.5)
	CONFIG_DEVICE_LEGACY(IO_FLOPPY, 2,	"dsk\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_RW_CREATE_OR_READ, NULL, NULL, device_load_vtech1_floppy, NULL, NULL)
SYSTEM_CONFIG_END

/*     YEAR  NAME      PARENT		COMPAT	MACHINE   INPUT     INIT    CONFIG,	COMPANY   FULLNAME */
COMP ( 1983, laser110, 0,			0,		laser110, vtech1,  NULL,   vtech1,	"Video Technology", "Laser 110" )
COMP ( 1983, laser210, 0,			0,		laser210, vtech1,  NULL,   vtech1,	"Video Technology", "Laser 210" )
COMP ( 1983, laser200, laser210,	0,		laser210, vtech1,  NULL,   vtech1,	"Video Technology", "Laser 200" )
COMP ( 1983, vz200,    laser210,	0,		laser210, vtech1,  NULL,   vtech1,	"Video Technology", "Sanyo / Dick Smith VZ200" )
COMP ( 1983, fellow,   laser210,	0,		laser210, vtech1,  NULL,   vtech1,	"Video Technology", "Salora Fellow" )
COMP ( 1983, tx8000,   laser210,	0,		laser210, vtech1,  NULL,   vtech1,	"Video Technology", "Texet TX8000" )
COMP ( 1983, laser310, 0,			0,		laser310, vtech1,  NULL,   vtech1,	"Video Technology", "Laser 310" )
COMP ( 1983, vz300,    laser310,	0,		laser310, vtech1,  NULL,   vtech1,	"Video Technology", "Sanyo / Dick Smith VZ300" )
