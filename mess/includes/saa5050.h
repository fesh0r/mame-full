/***********************************************************************

	SAA5050 header for systems file

***********************************************************************/

int		saa5050_vh_start (void);
void	saa5050_vh_stop (void);
void	saa5050_vh_screenrefresh (struct mame_bitmap *, int);

struct	GfxLayout	saa5050_charlayout =
{
	6, 10,
	256,
	1,
	{ 0 },
	{ 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8,
	  5*8, 6*8, 7*8, 8*8, 9*8 },
	8 * 10
};

struct	GfxLayout	saa5050_hilayout =
{
	6, 10,
	256,
	1,
	{ 0 },
	{ 2, 3, 4, 5, 6, 7 },
	{ 0*8, 0*8, 1*8, 1*8, 2*8,
	  2*8, 3*8, 3*8, 4*8, 4*8 },
	8 * 10
};

struct	GfxLayout	saa5050_lolayout =
{
	6, 10,
	256,
	1,
	{ 0 },
	{ 2, 3, 4, 5, 6, 7 },
	{ 5*8, 5*8, 6*8, 6*8, 7*8,
	  7*8, 8*8, 8*8, 9*8, 9*8 },
	8 * 10
};

static	struct	GfxDecodeInfo	saa5050_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &saa5050_charlayout, 0, 128},
	{ REGION_GFX1, 0x0000, &saa5050_hilayout, 0, 128},
	{ REGION_GFX1, 0x0000, &saa5050_lolayout, 0, 128},
	{-1}
};

static	unsigned	char	saa5050_palette[8 * 3] =
{
	0x00, 0x00, 0x00,	/* black */
	0xff, 0x00, 0x00,	/* red */
	0x00, 0xff, 0x00,	/* green */
	0xff, 0xff, 0x00,	/* yellow */
	0x00, 0x00, 0xff,	/* blue */
	0xff, 0x00, 0xff,	/* magenta */
	0x00, 0xff, 0xff,	/* cyan */
	0xff, 0xff, 0xff	/* white */
};

static	unsigned	short	saa5050_colortable[64 * 2] =	/* bgnd, fgnd */
{
	0,1, 0,1, 0,2, 0,3, 0,4, 0,5, 0,6, 0,7,
	1,0, 1,1, 1,2, 1,3, 1,4, 1,5, 1,6, 1,7,
	2,0, 2,1, 2,2, 2,3, 2,4, 2,5, 2,6, 2,7,
	3,0, 3,1, 3,2, 3,3, 3,4, 3,5, 3,6, 3,7,
	4,0, 4,1, 4,2, 4,3, 4,4, 4,5, 4,6, 4,7,
	5,0, 5,1, 5,2, 5,3, 5,4, 5,5, 5,6, 5,7,
	6,0, 6,1, 6,2, 6,3, 6,4, 6,5, 6,6, 6,7,
	7,0, 7,1, 7,2, 7,3, 7,4, 7,5, 7,6, 7,7
};

static	void	saa5050_init_palette (unsigned char *sys_palette,
			unsigned short *sys_colortable, const unsigned char *color_prom)

{

memcpy (sys_palette, saa5050_palette, sizeof (saa5050_palette));
memcpy (sys_colortable, saa5050_colortable, sizeof (saa5050_colortable));

}

#define SAA5050_VBLANK 2500

#define SAA5050_VIDHRDW 40 * 6,\
	24 * 10,\
	{ 0, 40 * 6 - 1, 0, 24 * 10 - 1},\
	saa5050_gfxdecodeinfo,\
	8, 128,\
	saa5050_init_palette,\
	VIDEO_TYPE_RASTER,\
	0,\
	saa5050_vh_start,\
	saa5050_vh_stop,\
	saa5050_vh_screenrefresh,
