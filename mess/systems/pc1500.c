/******************************************************************************
 peter.trauner@jk.uni-linz.ac.at december 2000

 sharp pc1500, pc1500a, pc1600?
 tandy trs80 pc-2
																			   
******************************************************************************/
/*
// pc1500
// pc1500a
// tandy trs80 pc-2 (4kb ram)



4x 6116 (2kx8 ram)
2xsc43125a 4a 23 (lcd controller)
2xsc43125a 4a 13 (lcd controller)
3ma sc613128a04 (16kx8 rom)
lh5801 241d sharp
lh5811 53zd sharp

2x30 expansion port
2x20 cart port (for 16kb ram cartridge)

*/
/* with this one you include nearly all necessary structure definitions */
#include "driver.h"
#include "cpu/lh5801/lh5801.h"

#include "includes/pc1500.h"

// trs80pc2 configuration 
static MEMORY_READ_START( pc1500_readmem )
{ 0x00000, 0x03fff, MRA_NOP },
{ 0x04000, 0x047ff, MRA_RAM },
{ 0x07000, 0x071ff, MRA_BANK1 },
{ 0x07200, 0x073ff, MRA_BANK2 },
{ 0x07400, 0x075ff, MRA_BANK3 },
{ 0x07600, 0x077ff, MRA_RAM }, 
{ 0x07800, 0x07bff, MRA_RAM }, // maybe mirrored to 7c00
{ 0x07c00, 0x07fff, MRA_BANK4 },
{ 0x0c000, 0x0ffff, MRA_ROM },
{ 0x1f000, 0x1f00f, lh5811_r }, // 0x00YY0 ignored
MEMORY_END

static MEMORY_WRITE_START( pc1500_writemem )
{ 0x00000, 0x03fff, MWA_NOP },
{ 0x04000, 0x047ff, MWA_RAM }, 
{ 0x04800, 0x06fff, MWA_NOP },
{ 0x07000, 0x071ff, MWA_BANK1 },
{ 0x07200, 0x073ff, MWA_BANK2 },
{ 0x07400, 0x075ff, MWA_BANK3 },
{ 0x07600, 0x077ff, MWA_RAM }, //display chips with ram
{ 0x07800, 0x07bff, MWA_RAM },
{ 0x07c00, 0x07fff, MWA_BANK4 },
{ 0x08000, 0x0bfff, MWA_NOP },
{ 0x0c000, 0x0ffff, MWA_ROM },
{ 0x1f000, 0x1f00f, lh5811_w }, // 0x00YY0 ignored
MEMORY_END

// pc1500a configuration
static MEMORY_READ_START( pc1500a_readmem )
{ 0x00000, 0x03fff, MRA_NOP },
{ 0x04000, 0x057ff, MRA_RAM },
{ 0x07000, 0x071ff, MRA_BANK1 },
{ 0x07200, 0x073ff, MRA_BANK2 },
{ 0x07400, 0x075ff, MRA_BANK3 },
{ 0x07600, 0x077ff, MRA_RAM }, 
{ 0x07800, 0x07fff, MRA_RAM },
{ 0x0c000, 0x0ffff, MRA_ROM },
{ 0x1f000, 0x1f00f, lh5811_r }, // 0x00YY0 not ignored
MEMORY_END

static MEMORY_WRITE_START( pc1500a_writemem )
{ 0x00000, 0x03fff, MWA_NOP },
{ 0x04000, 0x057ff, MWA_RAM }, 
{ 0x05800, 0x06fff, MWA_NOP },
{ 0x07000, 0x071ff, MWA_BANK1 },
{ 0x07200, 0x073ff, MWA_BANK2 },
{ 0x07400, 0x075ff, MWA_BANK3 },
{ 0x07600, 0x077ff, MWA_RAM }, //display chips with ram
{ 0x07800, 0x07fff, MWA_RAM },
{ 0x08000, 0x0bfff, MWA_NOP },
{ 0x0c000, 0x0ffff, MWA_ROM },
{ 0x1f000, 0x1f00f, lh5811_w }, // 0x00YY0 not ignored
MEMORY_END

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, r)

INPUT_PORTS_START( pc1500 )
	PORT_START
	DIPS_HELPER( 0x0001, "DEF",			KEYCODE_LALT, KEYCODE_RALT)
	DIPS_HELPER( 0x0002, "      !",		KEYCODE_1, CODE_NONE)
	DIPS_HELPER( 0x0004, "      \"",	KEYCODE_2, CODE_NONE)
	DIPS_HELPER( 0x0008, "      #",		KEYCODE_3, CODE_NONE)
	DIPS_HELPER( 0x0010, "      $",		KEYCODE_4, CODE_NONE)
	DIPS_HELPER( 0x0020, "      %%",	KEYCODE_5, CODE_NONE)
	DIPS_HELPER( 0x0040, "      &",		KEYCODE_6, CODE_NONE)
	DIPS_HELPER( 0x0080, "SHIFT",		KEYCODE_LSHIFT, KEYCODE_RSHIFT)
// stupid multilanguage support, validation check
	DIPS_HELPER( 0x0100, "OFF ",		KEYCODE_F1, CODE_NONE) 
	DIPS_HELPER( 0x0200, "ON ",			KEYCODE_F2, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x0001, "Q",			KEYCODE_Q, CODE_NONE)
	DIPS_HELPER( 0x0002, "W",			KEYCODE_W, CODE_NONE)
	DIPS_HELPER( 0x0004, "E",			KEYCODE_E, CODE_NONE)
	DIPS_HELPER( 0x0008, "R",			KEYCODE_R, CODE_NONE)
	DIPS_HELPER( 0x0010, "T",			KEYCODE_T, CODE_NONE)
	DIPS_HELPER( 0x0020, "Y",			KEYCODE_Y, CODE_NONE)
	DIPS_HELPER( 0x0040, "U",			KEYCODE_U, CODE_NONE)
	DIPS_HELPER( 0x0080, "I",			KEYCODE_I, CODE_NONE)
	DIPS_HELPER( 0x0100, "O",			KEYCODE_O, CODE_NONE)
	DIPS_HELPER( 0x0200, "P",			KEYCODE_P, CODE_NONE)
	DIPS_HELPER( 0x0400, "7",			KEYCODE_7_PAD, CODE_NONE)
	DIPS_HELPER( 0x0800, "8",			KEYCODE_8_PAD, CODE_NONE)
	DIPS_HELPER( 0x1000, "9",			KEYCODE_9_PAD, CODE_NONE)
	DIPS_HELPER( 0x2000, "/     ?",		KEYCODE_SLASH_PAD, CODE_NONE)
	DIPS_HELPER( 0x4000, "CL    CA",	KEYCODE_ESC, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x0001, "A",			KEYCODE_A, CODE_NONE)
	DIPS_HELPER( 0x0002, "S",			KEYCODE_S, CODE_NONE)
	DIPS_HELPER( 0x0004, "D",			KEYCODE_D, CODE_NONE)
	DIPS_HELPER( 0x0008, "F",			KEYCODE_F, CODE_NONE)
	DIPS_HELPER( 0x0010, "G",			KEYCODE_G, CODE_NONE)
	DIPS_HELPER( 0x0020, "H",			KEYCODE_H, CODE_NONE)
	DIPS_HELPER( 0x0040, "J",			KEYCODE_J, CODE_NONE)
	DIPS_HELPER( 0x0080, "K",			KEYCODE_K, CODE_NONE)
	DIPS_HELPER( 0x0100, "L",			KEYCODE_L, CODE_NONE)
	DIPS_HELPER( 0x0200, "4",			KEYCODE_4_PAD, CODE_NONE)
	DIPS_HELPER( 0x0400, "5",			KEYCODE_5_PAD, CODE_NONE)
	DIPS_HELPER( 0x0800, "6",			KEYCODE_6_PAD, CODE_NONE)
	DIPS_HELPER( 0x1000, "*     :",		KEYCODE_ASTERISK, CODE_NONE)
	DIPS_HELPER( 0x2000, "MODE",		KEYCODE_F4, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x0001, "Z",			KEYCODE_Z, CODE_NONE)
	DIPS_HELPER( 0x0002, "X",			KEYCODE_X, CODE_NONE)
	DIPS_HELPER( 0x0004, "C",			KEYCODE_C, CODE_NONE)
	DIPS_HELPER( 0x0008, "V",			KEYCODE_V, CODE_NONE)
	DIPS_HELPER( 0x0010, "B",			KEYCODE_B, CODE_NONE)
	DIPS_HELPER( 0x0020, "N",			KEYCODE_N, CODE_NONE)
	DIPS_HELPER( 0x0040, "M",			KEYCODE_M, CODE_NONE)
	DIPS_HELPER( 0x0080, "(     <",		KEYCODE_OPENBRACE, CODE_NONE)
	DIPS_HELPER( 0x0100, ")     >",		KEYCODE_CLOSEBRACE, CODE_NONE)
	DIPS_HELPER( 0x0200, "1",			KEYCODE_1_PAD, CODE_NONE)
	DIPS_HELPER( 0x0400, "2",			KEYCODE_2_PAD, CODE_NONE)
	DIPS_HELPER( 0x0800, "3",			KEYCODE_3_PAD, CODE_NONE)
	DIPS_HELPER( 0x1000, "-     ,",		KEYCODE_MINUS_PAD, CODE_NONE)
	DIPS_HELPER( 0x2000, "left  DEL",	KEYCODE_LEFT, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x0001, "SML",			KEYCODE_LCONTROL, KEYCODE_RCONTROL)
	DIPS_HELPER( 0x0002, "RESERVE",		KEYCODE_F5, CODE_NONE)
	DIPS_HELPER( 0x0004, "RCL",			KEYCODE_F6, CODE_NONE)
	DIPS_HELPER( 0x0008, "SPACE and",	KEYCODE_SPACE, CODE_NONE)
	DIPS_HELPER( 0x0010, "down  pi",	KEYCODE_DOWN, CODE_NONE)
	DIPS_HELPER( 0x0020, "up    root",	KEYCODE_UP, CODE_NONE)
	DIPS_HELPER( 0x0040, "ENTER",		KEYCODE_ENTER, CODE_NONE)
	DIPS_HELPER( 0x0080, "0",			KEYCODE_0_PAD, CODE_NONE)
	DIPS_HELPER( 0x0100, ".",			KEYCODE_DEL_PAD, CODE_NONE)
	DIPS_HELPER( 0x0200, "=     @",		KEYCODE_ENTER_PAD, CODE_NONE)
	DIPS_HELPER( 0x0400, "+     ;",		KEYCODE_PLUS_PAD, CODE_NONE)
	DIPS_HELPER( 0x0800, "right INS",	KEYCODE_RIGHT, CODE_NONE)
INPUT_PORTS_END

static struct GfxLayout pc1500_charlayout =
{
        2,8,
        16,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0,0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        { 0,0 },
        /* y offsets */
        {
			7, 7,
			6, 6,
			5, 5,
			4, 4,
			3, 3,
			2, 2,
			1, 1
        },
        1*8
};

static struct GfxLayout pc1500_charlayout2 =
{
        2,6,
        16,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0,0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        { 0,0 },
        /* y offsets */
        {
			7, 7,
			6, 6,
			5, 5,
			4, 4,
			3, 3,
			2, 2,
			1, 1
        },
        1*8
};

static struct GfxDecodeInfo pc1500_gfxdecodeinfo[] = {
	{ 
		REGION_GFX1, /* memory region */
		0x0000, /* offset in memory region */
		&pc1500_charlayout,                     
		0, /* index in the color lookup table where color codes start */
		1  /* total number of color codes */
	},
	{ 
		REGION_GFX1, /* memory region */
		0x0000, /* offset in memory region */
		&pc1500_charlayout2,                     
		0, /* index in the color lookup table where color codes start */
		1  /* total number of color codes */
	},
    { -1 } /* end of array */
};

/* called about 60 times a second,
   maybe you should send a cpu core a vertical retrace interrupt signal */
static int pc1500_frame_int(void)
{
	return 0;
}

/* called when machine is reseted */
static void pc1500_machine_init(void)
{
}

static LH5801_CONFIG config={
	pc1500_in
};

#if 0
static struct beep_interface pc1500_sound=
{
	1,
	{100}
};
#endif

static struct MachineDriver machine_driver_pc1500 =
{
	/* basic machine hardware */
	{
		{
			CPU_LH5801,
			1300000,
			pc1500_readmem,pc1500_writemem, /* memory handler */
			0,0, /* port handler */
			pc1500_frame_int, /* vertical retrace interrupt */
			1, /* 1 vertical interrupt in 1/60 second */
			0, /* second interrupt, normally for raster line line interrupts */
			0,
			&config, /* cpu reset parameter, used for configuring of some cpus */
        }
	},
	/* frames per second, VBL duration */
	20, DEFAULT_60HZ_VBLANK_DURATION,
	1, /* single CPU */
	pc1500_machine_init,
	0,//stub_machine_stop,
//	640, 286, /* width and height of screen and allocated sizes */
//	{ 0, 640 - 1, 0, 286 - 1}, /* left, right, top, bottom of visible area */
	584, 248, /* width and height of screen and allocated sizes */
	{ 0, 584 - 1, 0, 248 - 1}, /* left, right, top, bottom of visible area */
	pc1500_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (pc1500_palette) / sizeof (pc1500_palette[0]) ,
	sizeof (pc1500_colortable) / sizeof(pc1500_colortable[0][0]),
	pc1500_init_colors,		/* convert color prom */

//	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,	/* video flags */
	VIDEO_TYPE_RASTER,	/* video flags */
	0,						/* obsolete */
    pc1500_vh_start,
	pc1500_vh_stop,
	pc1500_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
//        { SOUND_BEEP, &pc1500_sound }
		{ 0 }
    }
};

static struct MachineDriver machine_driver_trs80pc2 =
{
	/* basic machine hardware */
	{
		{
			CPU_LH5801,
			1300000,
			pc1500_readmem,pc1500_writemem, /* memory handler */
			0,0, /* port handler */
			pc1500_frame_int, /* vertical retrace interrupt */
			1, /* 1 vertical interrupt in 1/60 second */
			0, /* second interrupt, normally for raster line line interrupts */
			0,
			&config, /* cpu reset parameter, used for configuring of some cpus */
        }
	},
	/* frames per second, VBL duration */
	20, DEFAULT_60HZ_VBLANK_DURATION,
	1, /* single CPU */
	pc1500_machine_init,
	0,//stub_machine_stop,
//	640, 286, /* width and height of screen and allocated sizes */
//	{ 0, 640 - 1, 0, 286 - 1}, /* left, right, top, bottom of visible area */
	584, 248, /* width and height of screen and allocated sizes */
	{ 0, 584 - 1, 0, 248 - 1}, /* left, right, top, bottom of visible area */
	pc1500_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (pc1500_palette) / sizeof (pc1500_palette[0]) ,
	sizeof (pc1500_colortable) / sizeof(pc1500_colortable[0][0]),
	pc1500_init_colors,		/* convert color prom */

//	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,	/* video flags */
	VIDEO_TYPE_RASTER,	/* video flags */
	0,						/* obsolete */
    pc1500_vh_start,
	pc1500_vh_stop,
	trs80pc2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
//        { SOUND_BEEP, &pc1500_sound }
		{ 0 }
    }
};

static struct MachineDriver machine_driver_pc1500a =
{
	/* basic machine hardware */
	{
		{
			CPU_LH5801,
			1300000,
			pc1500a_readmem,pc1500a_writemem, /* memory handler */
			0,0, /* port handler */
			pc1500_frame_int, /* vertical retrace interrupt */
			1, /* 1 vertical interrupt in 1/60 second */
			0, /* second interrupt, normally for raster line line interrupts */
			0,
			&config, /* cpu reset parameter, used for configuring of some cpus */
        }
	},
	/* frames per second, VBL duration */
	20, DEFAULT_60HZ_VBLANK_DURATION,
	1, /* single CPU */
	pc1500_machine_init,
	0,//stub_machine_stop,
//	640, 286, /* width and height of screen and allocated sizes */
//	{ 0, 640 - 1, 0, 286 - 1}, /* left, right, top, bottom of visible area */
	584, 248, /* width and height of screen and allocated sizes */
	{ 0, 584 - 1, 0, 248 - 1}, /* left, right, top, bottom of visible area */
	pc1500_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (pc1500_palette) / sizeof (pc1500_palette[0]) ,
	sizeof (pc1500_colortable) / sizeof(pc1500_colortable[0][0]),
	pc1500_init_colors,		/* convert color prom */

//	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,	/* video flags */
	VIDEO_TYPE_RASTER,	/* video flags */
	0,						/* obsolete */
    pc1500_vh_start,
	pc1500_vh_stop,
	pc1500a_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
//        { SOUND_BEEP, &pc1500_sound }
		{ 0 }
    }
};

ROM_START(pc1500a)
	ROM_REGION(0x20000,REGION_CPU1, 0)
	ROM_LOAD("pc1500a.rom", 0xc000, 0x4000, 0xdca8f879)
	ROM_REGION(0x100,REGION_GFX1, 0)
ROM_END
/*
  0xe000
  0xe04d
   memory test
  0xe0b8
  0xe0ff
  0xc9e4
  0xca44 bhs

*/


#define rom_pc1500 rom_pc1500a
#define rom_trs80pc2 rom_pc1500a


static int pc1500_id_rom(int id)
{
	return ID_OK;	/* no id possible */

}

static int pc1500_load_rom(int id)
{
	FILE *cartfile;
	UINT8 *rom = memory_region(REGION_CPU1);

	if (device_filename(IO_CARTSLOT, id) == NULL)
	{
/* A cartridge isn't strictly mandatory, but it's recommended */
		return 0;
	}
	
	if (!(cartfile = (FILE*)image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0)))
	{
		return 1;
	}	

	osd_fread(cartfile, rom+0x400, 0x400);
	osd_fclose(cartfile);
	return 0;
}

static const struct IODevice io_pc1500[] = {
	{
		IO_CARTSLOT,					/* type */
		1,								/* count */
		"bin\0",                        /* file extensions */
		IO_RESET_ALL,					/* reset if file changed */
		pc1500_id_rom,					/* id */
		pc1500_load_rom, 				/* init */
		NULL,							/* exit */
		NULL,							/* info */
		NULL,							/* open */
		NULL,							/* close */
		NULL,							/* status */
		NULL,							/* seek */
		NULL,							/* tell */
		NULL,							/* input */
		NULL,							/* output */
		NULL,							/* input_chunk */
		NULL							/* output_chunk */
	},
    { IO_END }
};

#define io_pc1500a io_pc1500
#define io_trs80pc2 io_pc1500

/* only called once */
void init_pc1500(void)
{
	int i;
	UINT8 *gfx=memory_region(REGION_GFX1);
	for (i=0; i<256; i++) gfx[i]=i;
	
	cpu_setbank(1,memory_region(REGION_CPU1)+0x7600);
	cpu_setbank(2,memory_region(REGION_CPU1)+0x7600);
	cpu_setbank(3,memory_region(REGION_CPU1)+0x7600);
	cpu_setbank(4,memory_region(REGION_CPU1)+0x7800);

//	beep_set_frequency(0, 300);
}

void init_pc1500a(void)
{
	int i;
	UINT8 *gfx=memory_region(REGION_GFX1);
	for (i=0; i<256; i++) gfx[i]=i;
	
	cpu_setbank(1,memory_region(REGION_CPU1)+0x7600);
	cpu_setbank(2,memory_region(REGION_CPU1)+0x7600);
	cpu_setbank(3,memory_region(REGION_CPU1)+0x7600);

//	beep_set_frequency(0, 300);
}

/*    YEAR	NAME		PARENT  MACHINE   INPUT     INIT		COMPANY			FULLNAME */
COMPX( 1982, pc1500,		0,		pc1500,	   pc1500,	pc1500,	    "Sharp",			"Pocket Computer 1500", GAME_NOT_WORKING)
COMPX( 1982, trs80pc2,	pc1500, trs80pc2,  pc1500,	pc1500,	    "Tandy",			"TRS80 PC-2", GAME_NOT_WORKING|GAME_ALIAS)
COMPX( 1984, pc1500a,	pc1500, pc1500a,   pc1500,	pc1500a,    "Sharp",			"Pocket Computer 1500A", GAME_NOT_WORKING|GAME_ALIAS)
// 1986 pc1600 bigger display, ... (lh5803 cpu, seams to be also used in some pc1500a)


#ifdef RUNTIME_LOADER
extern void pc1500_runtime_loader_init(void)
{
	int i;
	for (i=0; drivers[i]; i++) {
		if ( strcmp(drivers[i]->name,"pc1500")==0) drivers[i]=&driver_pc1500;
		if ( strcmp(drivers[i]->name,"trs80pc2")==0) drivers[i]=&driver_trs80pc2;
		if ( strcmp(drivers[i]->name,"pc1500a")==0) drivers[i]=&driver_pc1500a;
	}
}
#endif
