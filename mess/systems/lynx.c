/******************************************************************************
 Peter.Trauner@jk.uni-linz.ac.at December 2000

 info found in bastian schick's bll
 and in cc65 for lynx

******************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"

#include "includes/lynx.h"
#include <zlib.h>

static MEMORY_READ_START( lynx_readmem )
	{ 0x0000, 0xfbff, MRA_RAM },
	{ 0xfc00, 0xfcff, MRA_BANK1 },
	{ 0xfd00, 0xfdff, MRA_BANK2 },
	{ 0xfe00, 0xfff7, MRA_BANK3 },
	{ 0xfff8, 0xfff9, MRA_RAM },
    { 0xfffa, 0xffff, MRA_BANK4 },
MEMORY_END

static MEMORY_WRITE_START( lynx_writemem )
	{ 0x0000, 0xfbff, MWA_RAM },
	{ 0xfc00, 0xfcff, MWA_BANK1 },
	{ 0xfd00, 0xfdff, MWA_BANK2 },
	{ 0xfe00, 0xfff8, MWA_RAM },
    { 0xfff9, 0xfff9, lynx_memory_config },
    { 0xfffa, 0xffff, MWA_RAM },
MEMORY_END

INPUT_PORTS_START( lynx )
	PORT_START
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1, "A", CODE_DEFAULT, CODE_DEFAULT )
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "B", CODE_DEFAULT, CODE_DEFAULT )
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Opt 2", KEYCODE_2, IP_JOY_DEFAULT )
	PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Opt 1",  KEYCODE_1, IP_JOY_DEFAULT )
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP   )
	PORT_START
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Pause",  KEYCODE_3, IP_JOY_DEFAULT )
	// power on and power off buttons
	PORT_START
	PORT_DIPNAME ( 0x03, 0, "90 Degree Rotation")
	PORT_DIPSETTING(	1, "clockwise" )
	PORT_DIPSETTING(	0, "None" )
	PORT_DIPSETTING(	2, "counterclockwise" )
INPUT_PORTS_END

static int lynx_frame_int(void)
{
	return 0;
}

static unsigned char lynx_palette[16][3]=
{
	{ 0 },
};

int lynx_vh_start(void)
{
    return 0;
}

void lynx_vh_stop(void)
{
}

/*
DISPCTL EQU $FD92       ; set to $D by INITMIKEY

; B7..B4        0
; B3    1 EQU color
; B2    1 EQU 4 bit mode
; B1    1 EQU flip screen
; B0    1 EQU video DMA enabled
*/
void lynx_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
	static int height=-1, width=-1;
	int h,w;
	int x, y;
	UINT16 j; // clipping needed!
	UINT8 *mem=memory_region(REGION_CPU1);

	if( palette_recalc() ) full_refresh = 1;

	if (readinputport(2)&3) { // rotation
		h=160; w=102;
		if ( ( ((readinputport(2)&3)==1)&&(mikey.data[0x92]&2))
			 ||( ((readinputport(2)&3)==2)&&!(mikey.data[0x92]&2)) ) {
			for (j=mikey.data[0x94]|(mikey.data[0x95]<<8),y=0; y<102;y++) {
				for (x=160-2;x>=0;j++,x-=2) {
					plot_pixel(bitmap, y, x+1, Machine->pens[mem[j]>>4]);
					plot_pixel(bitmap, y, x, Machine->pens[mem[j]&0xf]);
				}
			}
		} else {
			for (j=mikey.data[0x94]|(mikey.data[0x95]<<8),y=102-1; y>=0;y--) {
				for (x=0;x<160;j++,x+=2) {
					plot_pixel(bitmap, y, x, Machine->pens[mem[j]>>4]);
					plot_pixel(bitmap, y, x+1, Machine->pens[mem[j]&0xf]);
				}
			}
		}
	} else {
		w=160; h=102;
		if ( mikey.data[0x92]&2) {
			for (j=mikey.data[0x94]|(mikey.data[0x95]<<8),y=102-1; y>=0;y--) {
				for (x=160-2;x>=0;j++,x-=2) {
					plot_pixel(bitmap, x+1, y, Machine->pens[mem[j]>>4]);
					plot_pixel(bitmap, x, y, Machine->pens[mem[j]&0xf]);
				}
			}
		} else {
			for (j=mikey.data[0x94]|(mikey.data[0x95]<<8),y=0; y<102;y++) {
				for (x=0;x<160;j++,x+=2) {
					plot_pixel(bitmap, x, y, Machine->pens[mem[j]>>4]);
					plot_pixel(bitmap, x+1, y, Machine->pens[mem[j]&0xf]);
				}
			}
		}
	}
	if ((w!=width)||(h!=height)) {
		width=w;
		height=h;
		osd_set_visible_area(0,width-1,0, height-1);
	}
	lynx_audio_debug(bitmap);
}

static void lynx_init_colors (unsigned char *sys_palette,
							  unsigned short *sys_colortable,
							  const unsigned char *color_prom)
{
	memcpy (sys_palette, lynx_palette, sizeof (lynx_palette));
//	memcpy(sys_colortable,lynx_colortable,sizeof(lynx_colortable));
}

struct CustomSound_interface lynx_sound_interface =
{
	lynx_custom_start,
	lynx_custom_stop,
	lynx_custom_update
};

static struct MachineDriver machine_driver_lynx =
{
	/* basic machine hardware */
	{
		{
			CPU_M65SC02, // integrated in vlsi, stz, but not bbr bbs
			4000000,
			lynx_readmem,lynx_writemem,0,0,
			lynx_frame_int, 1,
        }
	},
	/* frames per second, VBL duration */
	60, DEFAULT_60HZ_VBLANK_DURATION, // lcd!
	1,				/* single CPU */
	lynx_machine_init,
	0,//pc1401_machine_stop,

	// 160 x 102
//	160, 102, { 0, 160 - 1, 0, 102 - 1},
	160, 160, { 0, 160 - 1, 0, 102 - 1},
	0, //lynx_gfxdecodeinfo,			   /* graphics decode info */
	// 16 out of 4096
	sizeof (lynx_palette) / sizeof (lynx_palette[0]) ,
	0, //sizeof (lynx_colortable) / sizeof(lynx_colortable[0][0]),
	lynx_init_colors,		/* convert color prom */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,	/* video flags */
	0,						/* obsolete */
    lynx_vh_start,
	lynx_vh_stop,
	lynx_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{SOUND_CUSTOM, &lynx_sound_interface},
		{ 0 }
    }
};

/* these 2 dumps are saved from an running machine,
   and therefor the rom byte at 0xff09 is not readable!
   (memory configuration)
   these 2 dumps differ only in this byte!
*/
ROM_START(lynx)
	ROM_REGION(0x10200,REGION_CPU1, 0)
	ROM_LOAD("lynx.bin", 0x10000, 0x200, 0xe1ffecb6)
	ROM_REGION(0x100,REGION_GFX1, 0)
	ROM_REGION(0x100000, REGION_USER1, 0)
ROM_END

ROM_START(lynxa)
	ROM_REGION(0x10200,REGION_CPU1, 0)
	ROM_LOAD("lynxa.bin", 0x10000, 0x200, 0x0d973c9d)
	ROM_REGION(0x100,REGION_GFX1, 0)
	ROM_REGION(0x100000, REGION_USER1, 0)
ROM_END

ROM_START(lynx2)
	ROM_REGION(0x10200,REGION_CPU1, 0)
	ROM_LOAD("lynx2.bin", 0x10000, 0x200, 0x0)
	ROM_REGION(0x100,REGION_GFX1, 0)
	ROM_REGION(0x100000, REGION_USER1, 0)
ROM_END

// extern unsigned int crc32 (unsigned int crc, const unsigned char *buf, unsigned int len);

UINT32 lynx_partialcrc(const unsigned char *buf,unsigned int size)
{
	unsigned int crc;

	if (size < 65) return 0;
	crc = (UINT32) crc32(0L,&buf[64],size-64);
	logerror("Lynx Partial CRC: %08lx %ld\n",crc,size);
	/* printf("Lynx Partial CRC: %08x %d\n",crc,size); */
	return (UINT32)crc;
}

int lynx_id_rom (int id)
{
	FILE *romfile;

	char header[64];

	logerror("Lynx IDROM\n");
	/* If no file was specified, don't bother */
	if (device_filename(IO_CARTSLOT,id) == NULL ||
		strlen(device_filename(IO_CARTSLOT,id)) == 0)
		return ID_FAILED;

	if (!(romfile = (FILE*)image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0))) {
		logerror("returning ID_FAILED\n");
		return ID_FAILED;
	}
	osd_fread(romfile,header,sizeof(header));
	osd_fclose (romfile);
	logerror("Trying Header Compare\n");

	if (strncmp("LYNX",&header[0],4) && strncmp("BS9",&header[6],3)) {
		logerror("Not an valid Lynx image\n");
		return ID_FAILED;
	}
	logerror("returning ID_OK\n");
	return ID_OK;
}

static int lynx_load_rom(int id)
{
	FILE *cartfile;
	UINT8 *rom = memory_region(REGION_USER1);
	int size;
	UINT8 header[0x40];
/* 64 byte header
   LYNX
   intelword lower counter size
   0 0 1 0
   32 chars name
   22 chars manufacturer
*/

	if (device_filename(IO_CARTSLOT, id) == NULL)
	{
		return 0;
	}

	if (!(cartfile = (FILE*)image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0)))
	{
		logerror("%s not found\n",device_filename(IO_CARTSLOT,id));
		return 1;
	}
	size=osd_fsize(cartfile);
	if (osd_fread(cartfile, header, 0x40)!=0x40) {
		logerror("%s load error\n",device_filename(IO_CARTSLOT,id));
		osd_fclose(cartfile);
		return 1;
	}
	size-=0x40;
	lynx_granularity=header[4]|(header[5]<<8);

	logerror ("%s %dkb cartridge with %dbyte granularity from %s\n",
			  header+10,size/1024,lynx_granularity, header+42);

	if (osd_fread(cartfile, rom, size)!=size) {
		logerror("%s load error\n",device_filename(IO_CARTSLOT,id));
		osd_fclose(cartfile);
		return 1;
	}
	osd_fclose(cartfile);
	return 0;
}

static int lynx_quickload(int id)
{
	FILE *cartfile;
	UINT8 *rom = memory_region(REGION_CPU1);
	int size;
	UINT8 header[10]; // 80 08 dw Start dw Len B S 9 3
	// maybe the first 2 bytes must be used to identify the endianess of the file
	UINT16 start;

	if (device_filename(IO_QUICKLOAD, id) == NULL)
	{
		return 0;
	}

	if (!(cartfile = (FILE*)image_fopen(IO_QUICKLOAD, id, OSD_FILETYPE_IMAGE_R, 0)))
	{
		logerror("%s not found\n",device_filename(IO_QUICKLOAD,id));
		return 1;
	}
	size=osd_fsize(cartfile);

	if (osd_fread(cartfile, header, sizeof(header))!=sizeof(header)) {
		logerror("%s load error\n",device_filename(IO_QUICKLOAD,id));
		osd_fclose(cartfile);
		return 1;
	}
	size-=sizeof(header);
	start=header[3]|(header[2]<<8); //! big endian format in file format for little endian cpu

	if (osd_fread(cartfile, rom+start, size)!=size) {
		logerror("%s load error\n",device_filename(IO_QUICKLOAD,id));
		osd_fclose(cartfile);
		return 1;
	}
	osd_fclose(cartfile);

	rom[0xfffc+0x200]=start&0xff;
	rom[0xfffd+0x200]=start>>8;
	return 0;
}

static const struct IODevice io_lynx[] = {
	{
		IO_CARTSLOT,					/* type */
		1,								/* count */
		"lnx\0",                        /* file extensions */
		IO_RESET_ALL,					/* reset if file changed */
		lynx_id_rom,					/* id */
		lynx_load_rom, 				/* init */
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
		NULL,							/* output_chunk */
		lynx_partialcrc,				/* partial crc */
	},
	{
		IO_QUICKLOAD,					/* type */
		1,								/* count */
		"o\0",                        /* file extensions */
		IO_RESET_ALL,					/* reset if file changed */
		lynx_id_rom,					/* id */
		lynx_quickload, 				/* init */
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
		NULL,							/* output_chunk */
//		lynx_partialcrc,				/* partial crc */
	},
    { IO_END }
};

void init_lynx(void)
{
	int i;
	UINT8 *gfx=memory_region(REGION_GFX1);
	for (i=0; i<256; i++) gfx[i]=i;
	lynx_quickload(0);
}

#define io_lynxa io_lynx
#define io_lynx2 io_lynx

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      MONITOR	COMPANY   FULLNAME */
CONSX( 1989, lynx,	  0, 		lynx,  lynx, 	lynx,	  "Atari",  "Lynx", GAME_NOT_WORKING|GAME_IMPERFECT_SOUND)
CONSX( 1989, lynxa,	  lynx, 	lynx,  lynx, 	lynx,	  "Atari",  "Lynx (alternate rom save!)", GAME_NOT_WORKING|GAME_IMPERFECT_SOUND)
CONSX( 1990, lynx2,	  lynx, 	lynx,  lynx, 	lynx,	  "Atari",  "Lynx II", GAME_NOT_WORKING|GAME_IMPERFECT_SOUND)

#ifdef RUNTIME_LOADER
extern void lynx_runtime_loader_init(void)
{
	int i;
	for (i=0; drivers[i]; i++) {
		if ( strcmp(drivers[i]->name,"lynx")==0) drivers[i]=&driver_lynx;
		if ( strcmp(drivers[i]->name,"lynxa")==0) drivers[i]=&driver_lynxa;
		if ( strcmp(drivers[i]->name,"lynx2")==0) drivers[i]=&driver_lynx2;
	}
}
#endif

