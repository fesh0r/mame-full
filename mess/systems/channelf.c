
/****************************************/
/*  Fairchild Channel F driver  		*/
/*                              		*/
/*  TBD:                        		*/
/*    	- Setup a tmpbitmap, redraw		*/
/*			row on palette change		*/
/*   	- Split BIOS ROM in two			*/
/*   	- Sound							*/
/*      - Verify visible area on HW     */
/*      - f8dasm fixes					*/
/*                              		*/
/****************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#ifndef VERBOSE
#define VERBOSE 1
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

static UINT8 latch[4];
static int palette_code[64];
static int palette_offset[64];
static int val;
static int row;
static int col;

READ_HANDLER( channelf_port_0_r )
{
	data_t data = readinputport(0);
	data = (data ^ 0xff) | latch[0];
    LOG(("port_0_r: $%02x\n",data));
	return data;
}

READ_HANDLER( channelf_port_1_r )
{
	data_t data = readinputport(1);
	data = (data ^ 0xff) | latch[1];
    LOG(("port_1_r: $%02x\n",data));
	return data;
}

READ_HANDLER( channelf_port_4_r )
{
	data_t data = readinputport(2);
	data = (data ^ 0xff) | latch[2];
    LOG(("port_4_r: $%02x\n",data));
	return data;
}

READ_HANDLER( channelf_port_5_r )
{
	data_t data = 0xff;
	data = (data ^ 0xff) | latch[3];
    LOG(("port_5_r: $%02x\n",data));
	return data;
}

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

static UINT16 colormap[] = {
	BLACK,   WHITE, WHITE, WHITE,
	LTBLUE,  BLUE,  RED,   GREEN,
	LTGRAY,  BLUE,  RED,   GREEN,
	LTGREEN, BLUE,  RED,   GREEN,
};

static void plot_4_pixel(int x, int y, int color)
{
	int pen;

	if (x < Machine->visible_area.min_x ||
		x + 1 >= Machine->visible_area.max_x ||
		y < Machine->visible_area.min_y ||
		y + 1 >= Machine->visible_area.max_y)
		return;

	if (color >= 16)
		return;

    pen = Machine->pens[colormap[color]];

    plot_pixel(Machine->scrbitmap, x, y, pen);
	plot_pixel(Machine->scrbitmap, x+1, y, pen);
	plot_pixel(Machine->scrbitmap, x, y+1, pen);
	plot_pixel(Machine->scrbitmap, x+1, y+1, pen);
}

int recalc_palette_offset(int code)
{
	/* Note: This is based on the very strange decoding they    */
	/*       used to determine which palette this line is using */

	switch(code)
	{
		case 0:
			return 0;
		case 8:
			return 4;
		case 3:
			return 8;
		case 15:
			return 12;
		default:
			return 0; /* This is an error condition */
	}
}

WRITE_HANDLER( channelf_port_0_w )
{
	LOG(("port_0_w: $%02x\n",data));

/*
	if (data & 0x40)
		controller_enable = 1;
	else
		controller_enable = 0;
*/

    if (data & 0x20)
	{
        if (col == 125)
		{
			palette_code[row] &= 0x03;
			palette_code[row] |= (val << 2);
			palette_offset[row] = recalc_palette_offset(palette_code[row]);
		}
		else if (col == 126)
		{
			palette_code[row] &= 0x0c;
			palette_code[row] |= val;
			palette_offset[row] = recalc_palette_offset(palette_code[row]);
		}
		if (col < 118)
			plot_4_pixel(col * 2, row * 2, palette_offset[row]+val);
    }
	latch[0] = data;
}

WRITE_HANDLER( channelf_port_1_w )
{
	LOG(("port_1_w: $%02x\n",data));

    val = ((data ^ 0xff) >> 6) & 0x03;

	latch[1] = data;
}

WRITE_HANDLER( channelf_port_4_w )
{
	LOG(("port_4_w: $%02x\n",data));

    col = (data | 0x80) ^ 0xff;

    latch[2] = data;
}

WRITE_HANDLER( channelf_port_5_w )
{
	LOG(("port_5_w: $%02x\n",data));

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

    latch[3] = data;
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
	{ 0x00, 0x00,	channelf_port_0_r }, /* Front panel switches */
	{ 0x01, 0x01,	channelf_port_1_r }, /* Right controller     */
	{ 0x04, 0x04,	channelf_port_4_r }, /* Left controller      */
	{ 0x05, 0x05,	channelf_port_5_r }, /* ???                  */
    {-1}
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x00,	channelf_port_0_w }, /* Enable Controllers & ARM WRT */
	{ 0x01, 0x01,	channelf_port_1_w }, /* Video Write Data */
	{ 0x04, 0x04,	channelf_port_4_w }, /* Video Horiz */
	{ 0x05, 0x05,	channelf_port_5_w }, /* Video Vert & Sound */
    {-1}
};

INPUT_PORTS_START( channelf )
	PORT_START /* Front panel buttons */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_SELECT1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_SELECT2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SELECT3 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_SELECT4 )
	PORT_BIT ( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* Right controller */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 ) /* START (1) */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 ) /* HOLD  (2) */
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 ) /* MODE  (3) */
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER1 ) /* TIME  (4) */
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 	   | IPF_PLAYER1 ) /* C-CLOCKWISE */
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON3 	   | IPF_PLAYER1 ) /* CLOCKWISE   */
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 	   | IPF_PLAYER1 ) /* PULL UP     */
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 	   | IPF_PLAYER1 ) /* PUSH DOWN   */

	PORT_START /* unused */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

    PORT_START /* unused */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* Left controller */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 	   | IPF_PLAYER2 ) /* C-CLOCKWISE */
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON3 	   | IPF_PLAYER2 ) /* CLOCKWISE   */
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 	   | IPF_PLAYER2 ) /* PULL UP     */
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 	   | IPF_PLAYER2 ) /* PUSH DOWN   */

INPUT_PORTS_END

/* Initialise the palette */
static void init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,palette,sizeof(palette));
	memcpy(sys_colortable,colormap,0);
}

static struct MachineDriver machine_driver_channelf =
{
	/* basic machine hardware */
	{
		{
			CPU_F8,
			3579545/2,	/* Colorburst/2 */
			readmem,writemem,readport,writeport,
			ignore_interrupt, 1
        }
	},
	/* frames per second, VBL duration */
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,						/* single CPU */
	channelf_init_machine,
	NULL,					/* stop machine */

	/* video hardware */
	118*2, 64*2, { 0, 118*2 - 1, 0, 64*2 - 1},
	NULL,
	8, 0,
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
		"bin\0",            /* file extensions */
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

