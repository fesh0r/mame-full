#include "driver.h"
#include "vidhrdw/generic.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)	/* x */
#endif

void channelf_init_machine(void)
{
}

void init_channelf(void)
{
	UINT8 *mem = memory_region(REGION_GFX1);
	int i;

    for (i = 0; i < 256; i++)
		mem[i] = i;
}

int channelf_id_rom(int id)
{
    return ID_OK;
}

int channelf_load_rom(int id)
{
	UINT8 *mem = memory_region(REGION_CPU1);
    void *file;
	int size;

    if (device_filename(IO_CARTSLOT,id) == NULL)
		return INIT_OK;
	file = image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0);
	if (!file)
		return INIT_FAILED;
	size = osd_fread(file, &mem[0x0800], 0x0800);
	osd_fclose(file);

    if (size == 0x800)
		return INIT_OK;

    return INIT_FAILED;
}

int channelf_vh_start(void)
{
	videoram_size = 0x00c0;

    if (generic_vh_start())
        return 1;

    return 0;
}

void channelf_vh_stop(void)
{
	generic_vh_stop();
}

void channelf_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
    if( full_refresh )
	{
		fillbitmap(Machine->scrbitmap, Machine->pens[0], &Machine->visible_area);
		memset(dirtybuffer, 1, videoram_size);
    }
}

static int pal[64];
static int val = 0;
static int row = 0;
static int col = 0;

READ_HANDLER( channelf_read_ff )
{
	return 0xff;
}

WRITE_HANDLER( channelf_port_0_w )
{

/*
	if (data & 0x40)
		controller_enable = 1;
	else
		controller_enable = 0;
*/

    if (data & 0x20)
	{
		int sx, sy, pen;

        if (col == 125)
			pal[row % 64] = (pal[row % 64] & 2) | ((data >> 1) & 1);
		if (col == 126)
			pal[row % 64] = (pal[row % 64] & 1) | (data & 2);
		sx = row * 2;
		sy = col * 2;
		pen = Machine->pens[pal[row % 64] + val];
		plot_pixel(Machine->scrbitmap, sx, sy, pen);
		plot_pixel(Machine->scrbitmap, sx+1, sy, pen);
		plot_pixel(Machine->scrbitmap, sx, sy+1, pen);
		plot_pixel(Machine->scrbitmap, sx+1, sy+1, pen);
    }
}

WRITE_HANDLER( channelf_port_1_w )
{
    val = ((data ^ 0xff) >> 6) & 0x03;

#if 0   /* I just can't understand that in channelf emu memory.c :-) */
    int color;

    switch (val)
	{
	case 0x3:
		color = 4 + value;
		break;
	case 0x0:
		color = 0 + value;
		break;
	case 0x3ff:
		color = 8 + value;
		break;
	case 0x44:
		color = 12 + value;
		break;
	default:
		color = 4 + value;
	}
#endif
}

WRITE_HANDLER( channelf_port_4_w )
{
	col = data ^ 0xff;
}

WRITE_HANDLER( channelf_port_5_w )
{
	switch (data & 0xc0)
	{
	case 0x00:	/* sound off */
		break;
	case 0x40:	/* medium tone */
		break;
    case 0x80:  /* high tone */
		break;
    case 0xc0:  /* low (wierd) tone */
		break;
    }
    row = (data | 0xc0) ^ 0xff;
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x07ff, MRA_ROM },
	{ 0x0800, 0x0fff, MRA_ROM },
    {-1}
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_ROM },
	{ 0x0400, 0x07ff, MWA_ROM },
    {-1}
};

static struct IOReadPort readport[] =
{
	{ 0x00, 0x00,	input_port_0_r },
	{ 0x01, 0x01,	input_port_1_r },
	{ 0x04, 0x04,	input_port_4_r },
	{ 0x05, 0x05,	input_port_5_r },
	{ 0x40, 0x40,	channelf_read_ff },
    {-1}
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x00,	channelf_port_0_w },
	{ 0x01, 0x01,	channelf_port_1_w },
	{ 0x04, 0x04,	channelf_port_4_w },
	{ 0x05, 0x05,	channelf_port_5_w },
    {-1}
};

INPUT_PORTS_START( channelf )
	PORT_START /* Front panel buttons */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_SELECT1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_SELECT2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_SELECT3 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_SELECT4 )
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* Right controller */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 	   | IPF_PLAYER1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 	   | IPF_PLAYER1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 	   | IPF_PLAYER1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 	   | IPF_PLAYER1 )

	PORT_START /* unused */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

    PORT_START /* unused */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* Right controller */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 	   | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 	   | IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 	   | IPF_PLAYER2 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 	   | IPF_PLAYER2 )

INPUT_PORTS_END

static UINT8 palette[] = {
	0x00, 0x00, 0x00,	/* black */
	0xff, 0xff, 0xff,	/* white */
	0xff, 0x00, 0x00,	/* red	 */
	0x00, 0xff, 0x00,	/* green */
	0x00, 0x00, 0xff,	/* blue  */
	0xbf, 0xbf, 0xbf,	/* ltgray  */
	0xbf, 0xff, 0xbf,	/* ltgreen */
	0xbf, 0xbf, 0xff	/* ltblue  */
};

#define BLACK	0
#define WHITE   1
#define RED     2
#define GREEN   3
#define BLUE    4
#define LTGRAY  5
#define LTGREEN 6
#define LTBLUE	7

static UINT16 colortable[] = {
	BLACK,	BLACK,
	WHITE,	BLACK,
    WHITE,  BLACK,
    WHITE,  BLACK,

	LTBLUE, BLACK,
	BLUE,	BLACK,
	RED,	BLACK,
	GREEN,	BLACK,

	LTGRAY, BLACK,
	BLUE,	BLACK,
	RED,	BLACK,
    GREEN,  BLACK,

	LTGREEN,BLACK,
	BLUE,	BLACK,
	RED,	BLACK,
	GREEN,	BLACK
};

/* Initialise the palette */
static void init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,palette,sizeof(palette));
	memcpy(sys_colortable,colortable,sizeof(colortable));
}

static struct MachineDriver machine_driver_channelf =
{
	/* basic machine hardware */
	{
		{
			CPU_F8,
			2000000,	/* 2 MHz */
			readmem,writemem,readport,writeport,
			ignore_interrupt, 1
        }
	},
	/* frames per second, VBL duration */
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,						/* single CPU */
	channelf_init_machine,
	NULL,					/* stop machine */

	/* video hardware - include overscan */
	5*8*4, 112*2,	{ 0, 5*8*4-1, 0, 112*2 - 1},
	NULL,
	8, 16*2,
	init_palette,			/* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,	/* video flags */
	0,						/* obsolete */
	channelf_vh_start,
	channelf_vh_stop,
	channelf_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
};

ROM_START(channelf)
	ROM_REGION(0x10000,REGION_CPU1)
		ROM_LOAD("fcbios.rom",  0x0000, 0x0800, 0x2882c02d)
	ROM_REGION(0x00100,REGION_GFX1)
		/* bit pattern is stored here */
ROM_END

static const struct IODevice io_channelf[] = {
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"rom\0",            /* file extensions */
		NULL,               /* private */
		channelf_id_rom,	/* id */
		channelf_load_rom,	/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,               /* open */
		NULL,               /* close */
		NULL,               /* status */
		NULL,               /* seek */
		NULL,				/* tell */
        NULL,               /* input */
		NULL,               /* output */
		NULL,               /* input_chunk */
		NULL,				/* output_chunk */
		NULL				/* correct CRC */
    },
    { IO_END }
};

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY      FULLNAME */
CONS( 1976, channelf, 0,		channelf, channelf, channelf, "Fairchild", "Channel F" )

