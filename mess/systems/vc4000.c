/******************************************************************************
 Peter.Trauner@jk.uni-linz.ac.at May 2001

 Paul Robson's Emulator at www.classicgaming.com/studio2 made it possible
******************************************************************************/

#include "driver.h"
#include "cpu/s2650/s2650.h"

#include "includes/vc4000.h"

READ_HANDLER(vc4000_key_r)
{
    UINT8 data=0;
    switch(offset) {
    case 0:
	data = readinputport(1);
	break;
    case 1:
	data = readinputport(2);
	break;
    case 2:
	data = readinputport(3);
	break;
    case 3:
	data = readinputport(0);
	break;
    case 4:
	data = readinputport(4);
	break;
    case 5:
	data = readinputport(5);
	break;
    case 6:
	data = readinputport(6);
	break;
    }
    return data;
}


static MEMORY_READ_START( vc4000_readmem )
	{ 0x0000, 0x07ff, MRA_ROM },
{ 0x1e88, 0x1e8e, vc4000_key_r },
	{ 0x1f00, 0x1fff, vc4000_video_r },
MEMORY_END

static MEMORY_WRITE_START( vc4000_writemem )
	{ 0x0000, 0x07ff, MWA_ROM },
	{ 0x1f00, 0x1fff, vc4000_video_w },
MEMORY_END

static PORT_READ_START( vc4000_readport )
//{ S2650_CTRL_PORT,S2650_CTRL_PORT, },
//{ S2650_DATA_PORT,S2650_DATA_PORT, },
{ S2650_SENSE_PORT,S2650_SENSE_PORT, vc4000_vsync_r},
PORT_END

static PORT_WRITE_START( vc4000_writeport )
PORT_END

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, r)

INPUT_PORTS_START( vc4000 )
	PORT_START
	DIPS_HELPER( 0x40, "Start", KEYCODE_F1, CODE_NONE)
	DIPS_HELPER( 0x80, "Game Select", KEYCODE_F2, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "Player 1/Left 1", KEYCODE_1, CODE_NONE)
	DIPS_HELPER( 0x40, "Player 1/Left 4", KEYCODE_4, KEYCODE_Q)
	DIPS_HELPER( 0x20, "Player 1/Left 7", KEYCODE_7, KEYCODE_A)
	DIPS_HELPER( 0x10, "Player 1/Left Enter", KEYCODE_ENTER, KEYCODE_Z)
	PORT_START
	DIPS_HELPER( 0x80, "Player 1/Left 2/Button", KEYCODE_2, KEYCODE_LCONTROL)
	DIPS_HELPER( 0x40, "Player 1/Left 5", KEYCODE_5, KEYCODE_W)
	DIPS_HELPER( 0x20, "Player 1/Left 8", KEYCODE_8, KEYCODE_S)
	DIPS_HELPER( 0x10, "Player 1/Left 0", KEYCODE_0, KEYCODE_X)
	PORT_START
	DIPS_HELPER( 0x80, "Player 1/Left 3", KEYCODE_3, CODE_NONE)
	DIPS_HELPER( 0x40, "Player 1/Left 6", KEYCODE_6, KEYCODE_E)
	DIPS_HELPER( 0x20, "Player 1/Left 9", KEYCODE_9, KEYCODE_D)
	DIPS_HELPER( 0x10, "Player 1/Left Clear", KEYCODE_C, KEYCODE_ESC)
	PORT_START
	DIPS_HELPER( 0x80, "Player 2/Right 1", KEYCODE_1_PAD, CODE_NONE)
	DIPS_HELPER( 0x40, "Player 2/Right 4", KEYCODE_4_PAD, CODE_NONE)
	DIPS_HELPER( 0x20, "Player 2/Right 7", KEYCODE_7_PAD, CODE_NONE)
	DIPS_HELPER( 0x10, "Player 2/Right ENTER", KEYCODE_ENTER_PAD, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "Player 2/Right 2/Button", KEYCODE_2_PAD, KEYCODE_LALT)
	DIPS_HELPER( 0x40, "Player 2/Right 5", KEYCODE_5_PAD, CODE_NONE)
	DIPS_HELPER( 0x20, "Player 2/Right 8", KEYCODE_8_PAD, CODE_NONE)
	DIPS_HELPER( 0x10, "Player 2/Right 0", KEYCODE_0_PAD, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x80, "Player 2/Right 3", KEYCODE_3_PAD, CODE_NONE)
	DIPS_HELPER( 0x40, "Player 2/Right 6", KEYCODE_6_PAD, CODE_NONE)
	DIPS_HELPER( 0x20, "Player 2/Right 9", KEYCODE_9_PAD, CODE_NONE)
	DIPS_HELPER( 0x10, "Player 2/Right Clear", KEYCODE_DEL_PAD, CODE_NONE)
#if 0
    // shit, auto centering too slow, so only using 5 bits, and scaling at videoside
    PORT_START
PORT_ANALOGX(0x1ff,0x10,IPT_AD_STICK_X|IPF_CENTER,1,2000,0,0x1f,KEYCODE_LEFT,KEYCODE_RIGHT,JOYCODE_1_LEFT,JOYCODE_1_RIGHT)
    PORT_START
PORT_ANALOGX(0x1ff,0x10,IPT_AD_STICK_Y|IPF_CENTER,1,2000,0,0x1f,KEYCODE_UP,KEYCODE_DOWN,JOYCODE_1_UP,JOYCODE_1_DOWN)
    PORT_START
PORT_ANALOGX(0x1ff,0x10,IPT_AD_STICK_X|IPF_CENTER|IPF_PLAYER2,100,10,0,0x1f,KEYCODE_DEL,KEYCODE_PGDN,JOYCODE_2_LEFT,JOYCODE_2_RIGHT)
    PORT_START
PORT_ANALOGX(0x1ff,0x10,IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER2,100,10,0,0x1f,KEYCODE_HOME,KEYCODE_END,JOYCODE_2_UP,JOYCODE_2_DOWN)
#else
	PORT_START
	DIPS_HELPER( 0x01, "Player 1/left", KEYCODE_LEFT, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 1/right", KEYCODE_RIGHT, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 1/down", KEYCODE_DOWN, CODE_NONE)
	DIPS_HELPER( 0x08, "Player 1/up", KEYCODE_UP, CODE_NONE)
	DIPS_HELPER( 0x10, "Player 2/left", KEYCODE_DEL, CODE_NONE)
	DIPS_HELPER( 0x20, "Player 2/right", KEYCODE_PGDN, CODE_NONE)
	DIPS_HELPER( 0x40, "Player 2/down", KEYCODE_END, CODE_NONE)
	DIPS_HELPER( 0x80, "Player 2/up", KEYCODE_HOME, CODE_NONE)
#endif
INPUT_PORTS_END

static struct GfxLayout vc4000_charlayout =
{
        8,1,
        256,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        { 
	    0,
	    1,
	    2,
	    3,
	    4,
	    5,
	    6,
	    7,
        },
        /* y offsets */
        { 0 },
        1*8
};

static struct GfxDecodeInfo vc4000_gfxdecodeinfo[] = {
    { REGION_GFX1, 0x0000, &vc4000_charlayout,                     0, 2 },
    { -1 } /* end of array */
};

static int vc4000_frame_int(void)
{
	return 0;
}

static unsigned char vc4000_palette[16][3] =
{
    // background colors
    { 0, 0, 0 }, // black
    { 0, 0, 255 }, // blue
    { 0, 255, 0 }, // green
    { 0, 255, 255 }, // cyan
    { 255, 0, 0 }, // red
    { 255, 0, 255 }, // magenta
    { 255, 255, 0 }, // yellow
    { 255, 255, 255 }, // white
    // sprite colors
    // simplier to add another 8 colors else using colormapping
    // xor 7, bit 2 not green, bit 1 not blue, bit 0 not red
    { 255, 255, 255 }, // white
    { 0, 255, 255 }, // cyan
    { 255, 255, 0 }, // yellow
    { 0, 255, 0 }, // green
    { 255, 0, 255 }, // magenta
    { 0, 0, 255 }, // blue
    { 255, 0, 0 }, // red
    { 0, 0, 0 } // black
};

static unsigned short vc4000_colortable[1][2] = {
	{ 0, 1 },
};

static void vc4000_init_colors (unsigned char *sys_palette,
				 unsigned short *sys_colortable,
				 const unsigned char *color_prom)
{
    memcpy (sys_palette, vc4000_palette, sizeof (vc4000_palette));
    memcpy(sys_colortable, vc4000_colortable,sizeof(vc4000_colortable));
}

static void vc4000_machine_init(void)
{
}

static struct MachineDriver machine_driver_vc4000 =
{
	/* basic machine hardware */
	{
		{
			CPU_S2650,
			3580000/3,
			vc4000_readmem,vc4000_writemem,
			vc4000_readport,vc4000_writeport,
			vc4000_frame_int, 1,
			vc4000_video_line,312*50,
			NULL
		}
	},
	/* frames per second, VBL duration */
	50, DEFAULT_60HZ_VBLANK_DURATION,
	1,				/* single CPU */
	vc4000_machine_init,
	0,//pc1401_machine_stop,

//	128+2*XPOS, 208+YPOS+YBOTTOM_SIZE, { 0, 2*XPOS+128-1, 0, YPOS+208+YBOTTOM_SIZE-1},
	256+2*XPOS, 260+YPOS+YBOTTOM_SIZE, { 0, 2*XPOS+256-1, 0, YPOS+260+YBOTTOM_SIZE-1},
	vc4000_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (vc4000_palette) / sizeof (vc4000_palette[0]) ,
	sizeof (vc4000_colortable) / sizeof(vc4000_colortable[0][0]),
	vc4000_init_colors,		/* convert color prom */

	VIDEO_TYPE_RASTER,	/* video flags */
	0,						/* obsolete */
	vc4000_vh_start,
	vc4000_vh_stop,
	vc4000_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
	    {SOUND_CUSTOM, &vc4000_sound_interface},
	    { 0 }
	}
};

ROM_START(vc4000)
	ROM_REGION(0x8000,REGION_CPU1, 0)
	ROM_REGION(0x100,REGION_GFX1, 0)
ROM_END

static int vc4000_id_rom(int id)
{
	return ID_OK;	/* no id possible */

}

static int vc4000_load_rom(int id)
{
	FILE *cartfile;
	UINT8 *rom = memory_region(REGION_CPU1);
	int size;

	if (device_filename(IO_CARTSLOT, id) == NULL)
	{
		printf("%s requires Cartridge!\n", Machine->gamedrv->name);
		return 0;
	}
	
	if (!(cartfile = (FILE*)image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0)))
	{
		logerror("%s not found\n",device_filename(IO_CARTSLOT,id));
		return 1;
	}	
	size=osd_fsize(cartfile);

	if (osd_fread(cartfile, rom, size)!=size) {
		logerror("%s load error\n",device_filename(IO_CARTSLOT,id));
		osd_fclose(cartfile);
		return 1;
	}
	osd_fclose(cartfile);
	return 0;
}

static const struct IODevice io_vc4000[] = {
	{
		IO_CARTSLOT,					/* type */
		1,								/* count */
		"bin\0",                        /* file extensions */
		IO_RESET_ALL,					/* reset if file changed */
		vc4000_id_rom,					/* id */
		vc4000_load_rom, 				/* init */
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


void init_vc4000(void)
{
	int i;
	UINT8 *gfx=memory_region(REGION_GFX1);
	for (i=0; i<256; i++) gfx[i]=i;
}

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
CONSX( 1978, vc4000,	0,	vc4000,  vc4000,  vc4000,		"Interton",		"VC4000", GAME_NOT_WORKING|GAME_IMPERFECT_SOUND )


#ifdef RUNTIME_LOADER
extern void vc4000_runtime_loader_init(void)
{
	int i;
	for (i=0; drivers[i]; i++) {
		if ( strcmp(drivers[i]->name,"vc4000")==0) drivers[i]=&driver_vc4000;
	}
}
#endif
