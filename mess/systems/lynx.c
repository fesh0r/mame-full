/******************************************************************************
 PeT mess@utanet.at 2000,2001

 info found in bastian schick's bll
 and in cc65 for lynx

******************************************************************************/

#include <zlib.h>

#include "driver.h"
#include "image.h"
#include "cpu/m6502/m6502.h"
#include "devices/snapquik.h"
#include "devices/cartslot.h"
#include "includes/lynx.h"

static int rotate=0;
int lynx_rotate;
static int lynx_line_y;
UINT32 lynx_palette[0x10];

static ADDRESS_MAP_START( lynx_readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0xfbff) AM_READ( MRA8_RAM )
	AM_RANGE( 0xfc00, 0xfcff) AM_READ( MRA8_BANK1 )
	AM_RANGE( 0xfd00, 0xfdff) AM_READ( MRA8_BANK2 )
	AM_RANGE( 0xfe00, 0xfff7) AM_READ( MRA8_BANK3 )
	AM_RANGE( 0xfff8, 0xfff9) AM_READ( MRA8_RAM )
    AM_RANGE( 0xfffa, 0xffff) AM_READ( MRA8_BANK4 )
ADDRESS_MAP_END

static ADDRESS_MAP_START( lynx_writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0xfbff) AM_WRITE( MWA8_RAM )
	AM_RANGE( 0xfc00, 0xfcff) AM_WRITE( MWA8_BANK1 )
	AM_RANGE( 0xfd00, 0xfdff) AM_WRITE( MWA8_BANK2 )
	AM_RANGE( 0xfe00, 0xfff8) AM_WRITE( MWA8_RAM )
    AM_RANGE( 0xfff9, 0xfff9) AM_WRITE( lynx_memory_config )
    AM_RANGE( 0xfffa, 0xffff) AM_WRITE( MWA8_RAM )
ADDRESS_MAP_END

INPUT_PORTS_START( lynx )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("A")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("B")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Opt 2") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Opt 1") PORT_CODE(KEYCODE_1)
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP   )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(DEF_STR(Pause)) PORT_CODE(KEYCODE_3)
	// power on and power off buttons
	PORT_START
	PORT_DIPNAME ( 0x03, 3, "90 Degree Rotation")
	PORT_DIPSETTING(	2, "Counterclockwise" )
	PORT_DIPSETTING(	1, "Clockwise" )
	PORT_DIPSETTING(	0, DEF_STR( None ) )
	PORT_DIPSETTING(	3, "Crcfile" )
INPUT_PORTS_END

static INTERRUPT_GEN( lynx_frame_int )
{
    lynx_rotate=rotate;
    if ((readinputport(2)&3)!=3)
		lynx_rotate=readinputport(2)&3;
}

static VIDEO_START( lynx )
{
    return 0;
}

/*
DISPCTL EQU $FD92       ; set to $D by INITMIKEY

; B7..B4        0
; B3    1 EQU color
; B2    1 EQU 4 bit mode
; B1    1 EQU flip screen
; B0    1 EQU video DMA enabled
*/
void lynx_draw_lines(int newline)
{
	static int height=-1, width=-1;
	int h,w;
	int x, yend;
	UINT16 j; // clipping needed!
	UINT8 *mem=memory_region(REGION_CPU1);

	if (osd_skip_this_frame()) newline=-1;

	if (newline==-1)
		yend = 102;
	else
		yend = newline;

	if (yend > 102)
		yend=102;

	if (yend==lynx_line_y)
	{
		if (newline==-1)
			lynx_line_y=0;
		return;
	}

	j=(mikey.data[0x94]|(mikey.data[0x95]<<8))+lynx_line_y*160/2;
	if (mikey.data[0x92]&2)
		j-=160*102/2-1;

	if (lynx_rotate&3)
	{
		/* rotation */
		h=160; w=102;
		if ( ((lynx_rotate==1)&&(mikey.data[0x92]&2))
				||( (lynx_rotate==2)&&!(mikey.data[0x92]&2)) )
		{
			for (;lynx_line_y<yend;lynx_line_y++)
			{
				for (x=160-2;x>=0;j++,x-=2)
				{
					plot_pixel(Machine->scrbitmap, lynx_line_y, x+1, lynx_palette[mem[j]>>4]);
					plot_pixel(Machine->scrbitmap, lynx_line_y, x, lynx_palette[mem[j]&0xf]);
				}
			}
		}
		else
		{
			for (;lynx_line_y<yend;lynx_line_y++)
			{
				for (x=0;x<160;j++,x+=2)
				{
					plot_pixel(Machine->scrbitmap, 102-1-lynx_line_y, x, lynx_palette[mem[j]>>4]);
					plot_pixel(Machine->scrbitmap, 102-1-lynx_line_y, x+1, lynx_palette[mem[j]&0xf]);
				}
			}
		}
	}
	else
	{
		w=160; h=102;
		if ( mikey.data[0x92]&2)
		{
			for (;lynx_line_y<yend;lynx_line_y++)
			{
				for (x=160-2;x>=0;j++,x-=2)
				{
					plot_pixel(Machine->scrbitmap, x+1, 102-1-lynx_line_y, lynx_palette[mem[j]>>4]);
					plot_pixel(Machine->scrbitmap, x, 102-1-lynx_line_y, lynx_palette[mem[j]&0xf]);
				}
			}
		}
		else
		{
			for (;lynx_line_y<yend;lynx_line_y++)
			{
				for (x=0;x<160;j++,x+=2)
				{
					plot_pixel(Machine->scrbitmap, x, lynx_line_y, lynx_palette[mem[j]>>4]);
					plot_pixel(Machine->scrbitmap, x+1, lynx_line_y, lynx_palette[mem[j]&0xf]);
				}
			}
		}
	}
	if (newline==-1)
	{
		lynx_line_y=0;
		if ((w!=width)||(h!=height))
		{
			width=w;
			height=h;
			set_visible_area(0,width-1,0, height-1);
		}
	}
}

static VIDEO_UPDATE( lynx )
{
	lynx_audio_debug(bitmap);
}

static PALETTE_INIT( lynx )
{
    int i;

    for (i=0; i< 0x1000; i++)
	{
		palette_set_color(i,
			((i >> 0) & 0x0f) * 0x11,
			((i >> 4) & 0x0f) * 0x11,
			((i >> 8) & 0x0f) * 0x11);
	}
}

struct CustomSound_interface lynx_sound_interface =
{
	lynx_custom_start,
	NULL,
	lynx_custom_update
};

struct CustomSound_interface lynx2_sound_interface =
{
	lynx2_custom_start,
	NULL,
	lynx_custom_update
};


static MACHINE_DRIVER_START( lynx )
	/* basic machine hardware */
	MDRV_CPU_ADD(M65SC02, 4000000)        /* vti core, integrated in vlsi, stz, but not bbr bbs */
	MDRV_CPU_PROGRAM_MAP(lynx_readmem,lynx_writemem)
	MDRV_CPU_VBLANK_INT(lynx_frame_int, 1)
	MDRV_FRAMES_PER_SECOND(LCD_FRAMES_PER_SECOND)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( lynx )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	/*MDRV_SCREEN_SIZE(160, 102)*/
	MDRV_SCREEN_SIZE(160, 160)
	MDRV_VISIBLE_AREA(0, 160-1, 0, 102-1)
	MDRV_PALETTE_LENGTH(0x1000)
	MDRV_COLORTABLE_LENGTH(0)
	MDRV_PALETTE_INIT( lynx )

	MDRV_VIDEO_START( lynx )
	MDRV_VIDEO_UPDATE( lynx )

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("main", CUSTOM, lynx_sound_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( lynx2 )
	MDRV_IMPORT_FROM( lynx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
	MDRV_SOUND_REPLACE("main", CUSTOM, lynx2_sound_interface)
MACHINE_DRIVER_END


/* these 2 dumps are saved from an running machine,
   and therefor the rom byte at 0xff09 is not readable!
   (memory configuration)
   these 2 dumps differ only in this byte!
*/
ROM_START(lynx)
	ROM_REGION(0x10200,REGION_CPU1, 0)
	ROM_LOAD("lynx.bin", 0x10000, 0x200, CRC(e1ffecb6) SHA1(de60f2263851bbe10e5801ef8f6c357a4bc077e6))
	ROM_REGION(0x100,REGION_GFX1, 0)
	ROM_REGION(0x100000, REGION_USER1, 0)
ROM_END

ROM_START(lynxa)
	ROM_REGION(0x10200,REGION_CPU1, 0)
	ROM_LOAD("lynxa.bin", 0x10000, 0x200, CRC(0d973c9d) SHA1(e4ed47fae31693e016b081c6bda48da5b70d7ccb))
	ROM_REGION(0x100,REGION_GFX1, 0)
	ROM_REGION(0x100000, REGION_USER1, 0)
ROM_END

ROM_START(lynx2)
	ROM_REGION(0x10200,REGION_CPU1, 0)
	ROM_LOAD("lynx2.bin", 0x10000, 0x200, NO_DUMP)
	ROM_REGION(0x100,REGION_GFX1, 0)
	ROM_REGION(0x100000, REGION_USER1, 0)
ROM_END



void lynx_partialhash(char *dest, const unsigned char *data,
	unsigned long length, unsigned int functions)
{
	if (length <= 64)
		return;
	hash_compute(dest, &data[64], length - 64, functions);
}



static int lynx_verify_cart (char *header)
{

	logerror("Trying Header Compare\n");

	if (strncmp("LYNX",&header[0],4) && strncmp("BS9",&header[6],3)) {
		logerror("Not an valid Lynx image\n");
		return IMAGE_VERIFY_FAIL;
	}
	logerror("returning ID_OK\n");
	return IMAGE_VERIFY_PASS;
}

static void lynx_crc_keyword(mess_image *image)
{
    const char *info;

    info = image_extrainfo(image);
    rotate = 0;

    if (info)
	{
		if(strcmp(info, "ROTATE90DEGREE")==0)
			rotate = 1;
		else if (strcmp(info, "ROTATE270DEGREE")==0)
			rotate = 2;
    }
}

static DEVICE_LOAD( lynx_cart )
{
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

	size = mame_fsize(file);
	if (mame_fread(file, header, 0x40)!=0x40)
	{
		logerror("%s load error\n", image_filename(image));
		return 1;
	}

	/* Check the image */
	if (lynx_verify_cart((char*)header) == IMAGE_VERIFY_FAIL)
	{
		return INIT_FAIL;
	}

	size-=0x40;
	lynx_granularity=header[4]|(header[5]<<8);

	logerror ("%s %dkb cartridge with %dbyte granularity from %s\n",
			  header+10,size/1024,lynx_granularity, header+42);

	if (mame_fread(file, rom, size) != size)
	{
		logerror("%s load error\n", image_filename(image));
		return 1;
	}

	lynx_crc_keyword(image);

	return 0;
}

static QUICKLOAD_LOAD( lynx )
{
	UINT8 *rom = memory_region(REGION_CPU1);
	UINT8 header[10]; // 80 08 dw Start dw Len B S 9 3
	// maybe the first 2 bytes must be used to identify the endianess of the file
	UINT16 start;

	if (mame_fread(fp, header, sizeof(header)) != sizeof(header))
		return INIT_FAIL;

	quickload_size -= sizeof(header);
	start = header[3] | (header[2]<<8); //! big endian format in file format for little endian cpu

	if (mame_fread(fp, rom+start, quickload_size) != quickload_size)
		return INIT_FAIL;

	rom[0xfffc+0x200] = start&0xff;
	rom[0xfffd+0x200] = start>>8;

	lynx_crc_keyword(image_from_devtype_and_index(IO_QUICKLOAD, 0));
	return 0;
}

SYSTEM_CONFIG_START(lynx)
	CONFIG_DEVICE_CARTSLOT_OPT(1, "lnx\0", NULL, NULL, device_load_lynx_cart, NULL, NULL, lynx_partialhash)
	CONFIG_DEVICE_QUICKLOAD(  "o\0", lynx )
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

static DRIVER_INIT( lynx )
{
	int i;
	UINT8 *gfx = memory_region(REGION_GFX1);

	for (i=0; i<256; i++)
		gfx[i]=i;
}

/*    YEAR  NAME      PARENT    COMPAT	MACHINE	INPUT	INIT	CONFIG	MONITOR	COMPANY   FULLNAME */
CONSX( 1989, lynx,	  0, 		0,		lynx,	lynx,	lynx,	lynx,	"Atari",  "Lynx", GAME_NOT_WORKING|GAME_IMPERFECT_SOUND)
CONSX( 1989, lynxa,	  lynx, 	0,		lynx,	lynx,	lynx,	lynx,	"Atari",  "Lynx (alternate rom save!)", GAME_NOT_WORKING|GAME_IMPERFECT_SOUND)
CONSX( 1991, lynx2,	  lynx, 	0,		lynx2,	lynx,	lynx,	lynx,	"Atari",  "Lynx II", GAME_NOT_WORKING|GAME_IMPERFECT_SOUND)
