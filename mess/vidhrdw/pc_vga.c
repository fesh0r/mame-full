/***************************************************************************

  Video Graphics Adapter (VGA) section

	Peter Trauner	PeT mess@utanet.at
	Nathan Woods	npwoods@mess.org

   include in memory read list
   { 0xa0000, 0xaffff, MRA_BANK1 }
   { 0xb0000, 0xb7fff, MRA_BANK2 }
   { 0xb8000, 0xbffff, MRA_BANK3 }
   { 0xc0000, 0xc7fff, MRA_ROM }

   and in memory write list
   { 0xa0000, 0xaffff, MWA_BANK1 }
   { 0xb0000, 0xb7fff, MWA_BANK2 }
   { 0xb8000, 0xbffff, MWA_BANK3 }
   { 0xc0000, 0xc7fff, MWA_ROM }

	(oti 037 chip)
    ROM_LOAD("oakvga.bin", 0xc0000, 0x8000, 0x318c5f43)
	(tseng labs famous et4000 isa vga card (oem))
    ROM_LOAD("et4000b.bin", 0xc0000, 0x8000, 0xa903540d)	
	(tseng labs famous et4000 isa vga card)
    ROM_LOAD("et4000.bin", 0xc0000, 0x8000, 0xf01e4be0)

***************************************************************************/

#include "driver.h"

#include "includes/crtc6845.h"
#include "includes/pc_vga.h"
#include "includes/pc_video.h"
#include "mscommon.h"

/***************************************************************************

	Static declarations

***************************************************************************/

#define DBG_LOG(n,m,a)

#ifdef MAME_DEBUG
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

/* vga
 standard compatible to mda, cga, hercules?, ega
 (mda, cga, hercules not real register compatible)
 several vga cards drive also mda, cga, ega monitors
 some vga cards have register compatible mda, cga, hercules modes

 ega/vga
 64k (early ega 16k) words of 32 bit memory
 */


static PALETTE_INIT( ega );
static PALETTE_INIT( vga );
static VIDEO_START( ega );
static VIDEO_START( vga );
static pc_video_update_proc pc_vga_choosevideomode(int *width, int *height, struct crtc6845 *crtc);
static pc_video_update_proc pc_ega_choosevideomode(int *width, int *height, struct crtc6845 *crtc);

/***************************************************************************

	MachineDriver stuff

***************************************************************************/

/* grabbed from dac inited by et4000 bios */
static unsigned char ega_palette[] =
{
	0x00, 0x00, 0x00,
	0x00, 0x00, 0xa8,
	0x00, 0xa8, 0x00,
	0x00, 0xa8, 0xa8,
	0xa8, 0x00, 0x00,
	0xa8, 0x00, 0xa8,
	0xa8, 0x54, 0x00,
	0xa8, 0xa8, 0xa8,
	0x00, 0x00, 0x00,
	0x00, 0x00, 0xa8,
	0x00, 0xa8, 0x00,
	0x00, 0xa8, 0xa8,
	0xa8, 0x00, 0x00,
	0xa8, 0x00, 0xa8,
	0xa8, 0x54, 0x00,
	0xa8, 0xa8, 0xa8,
	0x54, 0x54, 0x54,
	0x54, 0x54, 0xfc,
	0x54, 0xfc, 0x54,
	0x54, 0xfc, 0xfc,
	0xfc, 0x54, 0x54,
	0xfc, 0x54, 0xfc,
	0xfc, 0xfc, 0x54,
	0xfc, 0xfc, 0xfc,
	0x54, 0x54, 0x54,
	0x54, 0x54, 0xfc,
	0x54, 0xfc, 0x54,
	0x54, 0xfc, 0xfc,
	0xfc, 0x54, 0x54,
	0xfc, 0x54, 0xfc,
	0xfc, 0xfc, 0x54,
	0xfc, 0xfc, 0xfc,
	0x00, 0x00, 0x00,
	0x00, 0x00, 0xa8,
	0x00, 0xa8, 0x00,
	0x00, 0xa8, 0xa8,
	0xa8, 0x00, 0x00,
	0xa8, 0x00, 0xa8,
	0xa8, 0x54, 0x00,
	0xa8, 0xa8, 0xa8,
	0x00, 0x00, 0x00,
	0x00, 0x00, 0xa8,
	0x00, 0xa8, 0x00,
	0x00, 0xa8, 0xa8,
	0xa8, 0x00, 0x00,
	0xa8, 0x00, 0xa8,
	0xa8, 0x54, 0x00,
	0xa8, 0xa8, 0xa8,
	0x54, 0x54, 0x54,
	0x54, 0x54, 0xfc,
	0x54, 0xfc, 0x54,
	0x54, 0xfc, 0xfc,
	0xfc, 0x54, 0x54,
	0xfc, 0x54, 0xfc,
	0xfc, 0xfc, 0x54,
	0xfc, 0xfc, 0xfc,
	0x54, 0x54, 0x54,
	0x54, 0x54, 0xfc,
	0x54, 0xfc, 0x54,
	0x54, 0xfc, 0xfc,
	0xfc, 0x54, 0x54,
	0xfc, 0x54, 0xfc,
	0xfc, 0xfc, 0x54,
	0xfc, 0xfc, 0xfc
};

static unsigned short vga_colortable[] =
{
     0, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0,10, 0,11, 0,12, 0,13, 0,14, 0,15,
     1, 0, 1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 1, 8, 1, 9, 1,10, 1,11, 1,12, 1,13, 1,14, 1,15,
     2, 0, 2, 1, 2, 2, 2, 3, 2, 4, 2, 5, 2, 6, 2, 7, 2, 8, 2, 9, 2,10, 2,11, 2,12, 2,13, 2,14, 2,15,
     3, 0, 3, 1, 3, 2, 3, 3, 3, 4, 3, 5, 3, 6, 3, 7, 3, 8, 3, 9, 3,10, 3,11, 3,12, 3,13, 3,14, 3,15,
     4, 0, 4, 1, 4, 2, 4, 3, 4, 4, 4, 5, 4, 6, 4, 7, 4, 8, 4, 9, 4,10, 4,11, 4,12, 4,13, 4,14, 4,15,
     5, 0, 5, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 5, 8, 5, 9, 5,10, 5,11, 5,12, 5,13, 5,14, 5,15,
     6, 0, 6, 1, 6, 2, 6, 3, 6, 4, 6, 5, 6, 6, 6, 7, 6, 8, 6, 9, 6,10, 6,11, 6,12, 6,13, 6,14, 6,15,
     7, 0, 7, 1, 7, 2, 7, 3, 7, 4, 7, 5, 7, 6, 7, 7, 7, 8, 7, 9, 7,10, 7,11, 7,12, 7,13, 7,14, 7,15,
/* flashing is done by dirtying the videoram buffer positions with attr bit #7 set */
     8, 0, 8, 1, 8, 2, 8, 3, 8, 4, 8, 5, 8, 6, 8, 7, 8, 8, 8, 9, 8,10, 8,11, 8,12, 8,13, 8,14, 8,15,
     9, 0, 9, 1, 9, 2, 9, 3, 9, 4, 9, 5, 9, 6, 9, 7, 9, 8, 9, 9, 9,10, 9,11, 9,12, 9,13, 9,14, 9,15,
    10, 0,10, 1,10, 2,10, 3,10, 4,10, 5,10, 6,10, 7,10, 8,10, 9,10,10,10,11,10,12,10,13,10,14,10,15,
    11, 0,11, 1,11, 2,11, 3,11, 4,11, 5,11, 6,11, 7,11, 8,11, 9,11,10,11,11,11,12,11,13,11,14,11,15,
    12, 0,12, 1,12, 2,12, 3,12, 4,12, 5,12, 6,12, 7,12, 8,12, 9,12,10,12,11,12,12,12,13,12,14,12,15,
    13, 0,13, 1,13, 2,13, 3,13, 4,13, 5,13, 6,13, 7,13, 8,13, 9,13,10,13,11,13,12,13,13,13,14,13,15,
    14, 0,14, 1,14, 2,14, 3,14, 4,14, 5,14, 6,14, 7,14, 8,14, 9,14,10,14,11,14,12,14,13,14,14,14,15,
    15, 0,15, 1,15, 2,15, 3,15, 4,15, 5,15, 6,15, 7,15, 8,15, 9,15,10,15,11,15,12,15,13,15,14,15,15
};

MACHINE_DRIVER_START( pcvideo_vga )
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(720, 480)
	MDRV_VISIBLE_AREA(0,720-1, 0,480-1)
	MDRV_PALETTE_LENGTH(0x100)
	MDRV_COLORTABLE_LENGTH(0x100*2 /*sizeof(vga_colortable) / sizeof(vga_colortable[0])*/)
	MDRV_PALETTE_INIT(vga)

	MDRV_VIDEO_START(vga)
	MDRV_VIDEO_UPDATE(pc_video)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( pcvideo_pc1640 )
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(720, 350)
	MDRV_VISIBLE_AREA(0,720-1, 0,350-1)
	MDRV_PALETTE_LENGTH(sizeof(ega_palette) / 3)
	MDRV_COLORTABLE_LENGTH(0x100*2 /*sizeof(vga_colortable) / sizeof(vga_colortable[0])*/)
	MDRV_PALETTE_INIT(ega)

	MDRV_VIDEO_START(ega)
	MDRV_VIDEO_UPDATE(pc_video)
MACHINE_DRIVER_END

/***************************************************************************/

static PALETTE_INIT( ega )
{
	palette_set_colors(0, ega_palette, sizeof(ega_palette) / 3);
	memcpy(colortable, vga_colortable,0x200);
}

static PALETTE_INIT( vga )
{
	int i;
	for (i = 0; i < 0x100; i++)
		palette_set_color(i, 0, 0, 0);

	memcpy(colortable, vga_colortable,0x200);
}

static UINT8 rotate_right[8][256];
static UINT8 color_bitplane_to_packed[4/*plane*/][8/*pixel*/][256];

static struct {
	read8_handler read_dipswitch;

	UINT8 *memory;
	UINT8 *dirty;
	UINT8 *fontdirty;
	UINT16 pens[16]; /* the current 16 pens */

	UINT8 miscellaneous_output;
	UINT8 feature_control;
	struct { UINT8 index, data[5]; } sequencer;
	struct { UINT8 index, data[0x19]; } crtc;
	struct { UINT8 index, data[9]; UINT8 latch[4]; } gc;
	struct { UINT8 index, data[0x15]; bool state; } attribute;


	struct {
		UINT8 read_index, write_index, mask;
		bool read;
		int state;
		struct { UINT8 red, green, blue; } color[0x100];
		int dirty;
	} dac;

	struct {
		int time;
		bool visible;
	} cursor;

	struct {
		int (*get_clock)(void);

		int (*get_lines)(void);
		int (*get_sync_lines)(void);

		int (*get_columns)(void);
		int (*get_sync_columns)(void);

		double start_time;
		int retrace;
	} monitor;

	/* oak vga */
	struct { UINT8 reg; } oak;

	int log;
} vga;


// to use the crtc6845 macros
#define REG(x) vga.crtc.data[x]

#define DOUBLESCAN ((vga.crtc.data[9]&0x80)||((vga.crtc.data[9]&0x1f)!=0) )
#define CRTC_PORT_ADDR ((vga.miscellaneous_output&1)?0x3d0:0x3b0)

#define CRTC_ON (vga.crtc.data[0x17]&0x80)

#define LINES_HELPER ( (vga.crtc.data[0x12] \
				|((vga.crtc.data[7]&2)<<7) \
				|((vga.crtc.data[7]&0x40)<<3))+1 )
//#define TEXT_LINES (LINES_HELPER)
#define LINES (DOUBLESCAN?LINES_HELPER>>1:LINES_HELPER)
#define TEXT_LINES (LINES_HELPER >> ((vga.crtc.data[9]&0x80) ? 1 : 0))

#define GRAPHIC_MODE (vga.gc.data[6]&1) /* else textmodus */

#define EGA_COLUMNS (vga.crtc.data[1]+1)
#define EGA_START_ADDRESS ((vga.crtc.data[0xd]|(vga.crtc.data[0xc]<<8))<<2)
#define EGA_LINE_LENGTH (vga.crtc.data[0x13]<<3)

#define VGA_COLUMNS (EGA_COLUMNS>>1)
#define VGA_START_ADDRESS (EGA_START_ADDRESS)
#define VGA_LINE_LENGTH (EGA_LINE_LENGTH<<2)

#define CHAR_WIDTH ((vga.sequencer.data[1]&1)?8:9)
//#define CHAR_HEIGHT ((vga.crtc.data[9]&0x1f)+1)

#define TEXT_COLUMNS (vga.crtc.data[1]+1)
#define TEXT_START_ADDRESS (EGA_START_ADDRESS)
#define TEXT_LINE_LENGTH (EGA_LINE_LENGTH>>2)

#define TEXT_COPY_9COLUMN(ch) ( (ch>=192)&&(ch<=223)&&(vga.attribute.data[0x10]&4))

//#define CURSOR_ON (!(vga.crtc.data[0xa]&0x20))
//#define CURSOR_STARTLINE (vga.crtc.data[0xa]&0x1f)
//#define CURSOR_ENDLINE (vga.crtc.data[0xb]&0x1f)
//#define CURSOR_POS (vga.crtc.data[0xf]|(vga.crtc.data[0xe]<<8))

#define FONT1 ( ((vga.sequencer.data[3]&3)|((vga.sequencer.data[3]&0x10)<<2))*0x2000)
#define FONT2 ( ((vga.sequencer.data[3]&c)>>2)|((vga.sequencer.data[3]&0x20)<<3))*0x2000)


static int ega_get_clock(void)
{
	int clck=0;
	switch(vga.miscellaneous_output&0xc) {
	case 0: clck=14000000;break;
	case 4: clck=16000000;break;
	/* case 8: external */
	/* case 0xc: reserved */
	}
	if (vga.sequencer.data[1]&8) clck/=2;
	return clck;
}

static int vga_get_clock(void)
{
	int clck=0;
	switch(vga.miscellaneous_output&0xc) {
	case 0: clck=25000000;break;
	case 4: clck=28000000;break;
	/* case 8: external */
	/* case 0xc: reserved */
	}
	if (vga.sequencer.data[1]&8) clck/=2;
	return clck;
}

static int ega_get_crtc_columns(void) /* in clocks! */
{
	int columns=vga.crtc.data[0]+2;
	if (!GRAPHIC_MODE) {
		columns*=CHAR_WIDTH;
	} else {
		columns*=8;
	}
	return columns;
}

static int ega_get_crtc_lines(void)
{
	int lines=vga.crtc.data[6]|((vga.crtc.data[7]&1));

	return lines;
}

static int vga_get_crtc_columns(void) /* in clocks! */
{
	int columns=vga.crtc.data[0]+5;

	if (!GRAPHIC_MODE)
		columns *= CHAR_WIDTH;
	else if (vga.gc.data[5]&0x40)
		columns *= 4;
	else
		columns *= 8;

	return columns;
}

static int vga_get_crtc_lines(void)
{
	int lines=(vga.crtc.data[6]
			   |((vga.crtc.data[7]&1)<<8)
			   |((vga.crtc.data[7]&0x20)<<(8-4)))+2;

	return lines;
}

static int vga_get_crtc_sync_lines(void)
{
	return 10;
}

static int vga_get_crtc_sync_columns(void)
{
	return 40;
}

INLINE WRITE_HANDLER(vga_dirty_w)
{
	if (vga.memory[offset]!=data)
	{
		vga.memory[offset]=data;
		vga.dirty[offset]=1;
	}
}

INLINE WRITE_HANDLER(vga_dirty_font_w)
{
	if (vga.memory[offset]!=data)
	{
		vga.memory[offset]=data;
		vga.dirty[offset]=1;
		if ((offset&3)==2)
			vga.fontdirty[offset>>7]=1;
	}
}

static READ_HANDLER(vga_text_r)
{
	int data;
	data=vga.memory[((offset&~1)<<1)|(offset&1)];

	return data;
}

static WRITE_HANDLER(vga_text_w)
{
	vga_dirty_w(((offset&~1)<<1)|(offset&1),data);
}

INLINE UINT8 ega_bitplane_to_packed(UINT8 *latch, int number)
{
	return color_bitplane_to_packed[0][number][latch[0]]
		|color_bitplane_to_packed[1][number][latch[1]]
		|color_bitplane_to_packed[2][number][latch[2]]
		|color_bitplane_to_packed[3][number][latch[3]];
}

static READ_HANDLER(vga_ega_r)
{
	int data;
	vga.gc.latch[0]=vga.memory[(offset<<2)];
	vga.gc.latch[1]=vga.memory[(offset<<2)+1];
	vga.gc.latch[2]=vga.memory[(offset<<2)+2];
	vga.gc.latch[3]=vga.memory[(offset<<2)+3];
	if (vga.gc.data[5]&8) {
		data=0;
		if (!(ega_bitplane_to_packed(vga.gc.latch, 0)^(vga.gc.data[2]&0xf&~vga.gc.data[7]))) data|=1;
		if (!(ega_bitplane_to_packed(vga.gc.latch, 1)^(vga.gc.data[2]&0xf&~vga.gc.data[7]))) data|=2;
		if (!(ega_bitplane_to_packed(vga.gc.latch, 2)^(vga.gc.data[2]&0xf&~vga.gc.data[7]))) data|=4;
		if (!(ega_bitplane_to_packed(vga.gc.latch, 3)^(vga.gc.data[2]&0xf&~vga.gc.data[7]))) data|=8;
		if (!(ega_bitplane_to_packed(vga.gc.latch, 4)^(vga.gc.data[2]&0xf&~vga.gc.data[7]))) data|=0x10;
		if (!(ega_bitplane_to_packed(vga.gc.latch, 5)^(vga.gc.data[2]&0xf&~vga.gc.data[7]))) data|=0x20;
		if (!(ega_bitplane_to_packed(vga.gc.latch, 6)^(vga.gc.data[2]&0xf&~vga.gc.data[7]))) data|=0x40;
		if (!(ega_bitplane_to_packed(vga.gc.latch, 7)^(vga.gc.data[2]&0xf&~vga.gc.data[7]))) data|=0x80;
	} else {
		data=vga.gc.latch[vga.gc.data[4]&3];
	}

	return data;
}

INLINE UINT8 vga_latch_helper(UINT8 cpu, UINT8 latch, UINT8 mask)
{
	switch (vga.gc.data[3]&0x18) {
	case 0:
		return rotate_right[vga.gc.data[3]&7][(cpu&mask)|(latch&~mask)];
	case 8:
		return rotate_right[vga.gc.data[3]&7][((cpu&latch)&mask)|(latch&~mask)];
	case 0x10:
		return rotate_right[vga.gc.data[3]&7][((cpu|latch)&mask)|(latch&~mask)];
	case 0x18:
		return rotate_right[vga.gc.data[3]&7][((cpu^latch)&mask)|(latch&~mask)];
;
	}
	return 0; /* must not be reached, suppress compiler warning */
}

INLINE UINT8 vga_latch_write(int offs, UINT8 data)
{
	switch (vga.gc.data[5]&3) {
	case 0:
		if (vga.gc.data[1]&(1<<offs)) {
			return vga_latch_helper( (vga.gc.data[0]&(1<<offs))?vga.gc.data[8]:0,
									  vga.gc.latch[offs],vga.gc.data[8] );
		} else {
			return vga_latch_helper(data, vga.gc.latch[offs], vga.gc.data[8]);
		}
		break;
	case 1:
		return vga.gc.latch[offs];
		break;
	case 2:
		if (data&(1<<offs)) {
			return vga_latch_helper(0xff, vga.gc.latch[offs], vga.gc.data[8]);
		} else {
			return vga_latch_helper(0, vga.gc.latch[offs], vga.gc.data[8]);
		}
		break;
	case 3:
		if (vga.gc.data[0]&(1<<offs)) {
			return vga_latch_helper(0xff, vga.gc.latch[offs], data&vga.gc.data[8]);
		} else {
			return vga_latch_helper(0, vga.gc.latch[offs], data&vga.gc.data[8]);
		}
		break;
	}
	return 0; /* must not be reached, suppress compiler warning */
}

static WRITE_HANDLER(vga_ega_w)
{
	if (vga.sequencer.data[2]&1)
		vga_dirty_w(offset<<2, vga_latch_write(0,data));
	if (vga.sequencer.data[2]&2)
		vga_dirty_w((offset<<2)+1, vga_latch_write(1,data));
	if (vga.sequencer.data[2]&4)
		vga_dirty_font_w((offset<<2)+2, vga_latch_write(2,data));
	if (vga.sequencer.data[2]&8)
		vga_dirty_w((offset<<2)+3, vga_latch_write(3,data));
	if ((offset==0xffff)&&(data==0)) vga.log=1;
}

static READ_HANDLER(vga_vga_r)
{
	int data;
	data=vga.memory[((offset&~3)<<2)|(offset&3)];

	return data;
}

static WRITE_HANDLER(vga_vga_w)
{
	vga_dirty_font_w(((offset&~3)<<2)|(offset&3),data);
}

static void vga_cpu_interface(void)
{
	static int sequencer, gc;
	read8_handler read_handler;
	write8_handler write_handler;

	if ((gc==vga.gc.data[6])&&(sequencer==vga.sequencer.data[4])) return;

	gc=vga.gc.data[6];
	sequencer=vga.sequencer.data[4];

	if (vga.sequencer.data[4]&8)
	{
		read_handler = vga_vga_r;
		write_handler = vga_vga_w;
		DBG_LOG(1,"vga memory",("vga\n"));
	}
	else if (vga.sequencer.data[4] & 4)
	{
		read_handler = vga_ega_r;
		write_handler = vga_ega_w;
		DBG_LOG(1,"vga memory",("ega\n"));
	}
	else
	{
		read_handler = vga_text_r;
		write_handler = vga_text_w;
		DBG_LOG(1,"vga memory",("text\n"));
	}
	switch (vga.gc.data[6]&0xc) {
	case 0:
		cpu_setbank(1,vga.memory);
		cpu_setbank(2,vga.memory+0x10000);
		cpu_setbank(3,vga.memory+0x18000);
		memory_set_bankhandler_r(1, 0, MRA_BANK1);
		memory_set_bankhandler_r(2, 0, MRA_BANK2);
		memory_set_bankhandler_r(3, 0, MRA_BANK3);
		memory_set_bankhandler_w(1, 0, MWA_BANK1);
		memory_set_bankhandler_w(2, 0, MWA_BANK2);
		memory_set_bankhandler_w(3, 0, MWA_BANK3);
		DBG_LOG(1,"vga memory",("a0000-bffff\n"));
		break;
	case 4:
		memory_set_bankhandler_r(1, 0, read_handler);
		memory_set_bankhandler_r(2, 0, MRA_NOP);
		memory_set_bankhandler_r(3, 0, MRA_NOP);
		memory_set_bankhandler_w(1, 0, write_handler);
		memory_set_bankhandler_w(2, 0, MWA_NOP);
		memory_set_bankhandler_w(3, 0, MWA_NOP);
		DBG_LOG(1,"vga memory",("a0000-affff\n"));
		break;
	case 8:
		memory_set_bankhandler_r(1, 0, MRA_NOP);
		memory_set_bankhandler_r(2, 0, read_handler);
		memory_set_bankhandler_r(3, 0, MRA_NOP);
		memory_set_bankhandler_w(1, 0, MWA_NOP);
		memory_set_bankhandler_w(2, 0, write_handler);
		memory_set_bankhandler_w(3, 0, MWA_NOP);
		DBG_LOG(1,"vga memory",("b0000-b7fff\n"));
		break;
	case 0xc:
		memory_set_bankhandler_r(1, 0, MRA_NOP);
		memory_set_bankhandler_r(2, 0, MRA_NOP);
		memory_set_bankhandler_r(3, 0, read_handler);
		memory_set_bankhandler_w(1, 0, MWA_NOP);
		memory_set_bankhandler_w(2, 0, MWA_NOP);
		memory_set_bankhandler_w(3, 0, write_handler);
		DBG_LOG(1,"vga memory",("b8000-bffff\n"));
		break;
	}
}

static READ_HANDLER(vga_crtc_r)
{
	int data=0xff;

	switch (offset) {
	case 4: data=vga.crtc.index;break;
	case 5:
		if (vga.crtc.index<sizeof(vga.crtc.data))
			data=vga.crtc.data[vga.crtc.index];
		DBG_LOG(1,"vga crtc read",("%.2x %.2x\n",vga.crtc.index,data));
		break;
	case 0xa:
		vga.attribute.state=0;
		data=0;/*4; */
#if 0 /* slow */
		{
			int clock=vga.monitor.get_clock();
			int lines=vga.monitor.get_lines();
			int columns=vga.monitor.get_columns();
			int diff=(int)((timer_get_time()-vga.monitor.start_time)
				*clock)%(lines*columns);
			if (diff<columns*vga.monitor.get_sync_lines()) data|=8;
			diff=diff/lines;
			if (diff%columns<vga.monitor.get_sync_columns()) data|=1;
		}
#elif 1
		if (vga.monitor.retrace) {
			data|=9;
			if (timer_get_time()-vga.monitor.start_time>300e-6) vga.monitor.retrace=0;
		} else {
			if (timer_get_time()-vga.monitor.start_time>15e-3) vga.monitor.retrace=1;
			vga.monitor.start_time=timer_get_time();
		}
#else
		// not working with ps2m30
		if (vga.monitor.retrace) data|=9;
		vga.monitor.retrace=0;
#endif
		/* ega diagnostic readback enough for oak bios */
		switch (vga.attribute.data[0x12]&0x30) {
		case 0:
			if (vga.attribute.data[0x11]&1) data|=0x10;
			if (vga.attribute.data[0x11]&4) data|=0x20;
			break;
		case 0x10:
			data|=(vga.attribute.data[0x11]&0x30);
			break;
		case 0x20:
			if (vga.attribute.data[0x11]&2) data|=0x10;
			if (vga.attribute.data[0x11]&8) data|=0x20;
			break;
		case 0x30:
			data|=(vga.attribute.data[0x11]&0xc0)>>2;
			break;
		}

		/*DBG_LOG(1,"vga crtc 0x3[bd]a",("%.2x %.2x\n",offset,data)); */
		break;
	case 0xf:
		/* oak test */
		data=0;
		/* pega bios on/off */
		data=0x80;
		DBG_LOG(1,"0x3[bd]0 read",("%.2x %.2x\n",offset,data));
		break;
	default:
		DBG_LOG(1,"0x3[bd]0 read",("%.2x %.2x\n",offset,data));
	}
	return data;
}

static WRITE_HANDLER(vga_crtc_w)
{
	switch (offset) {
	case 0xa:
		DBG_LOG(1,"vga feature control write",("%.2x %.2x\n",offset,data));
		vga.feature_control=data;
		break;

	case 4:
		vga.crtc.index = data;
		break;

	case 5:
		DBG_LOG(1,"vga crtc write",("%.2x %.2x\n",vga.crtc.index,data));
		if (vga.crtc.index<sizeof(vga.crtc.data))
		{
			switch (vga.crtc.index) {
			case 0xa:case 0xb: case 0xe: case 0xf:
				vga.dirty[CRTC6845_CURSOR_POS<<2]=1;
				break;
			}
			vga.crtc.data[vga.crtc.index]=data;
		}
		break;

	default:
		DBG_LOG(1,"0x3[bd]0 write",("%.2x %.2x\n",offset,data));
		break;
	}
}

READ_HANDLER( vga_port_03b0_r )
{
	int data=0xff;
	if (CRTC_PORT_ADDR==0x3b0)
		data=vga_crtc_r(offset);
	/*DBG_LOG(1,"vga 0x3b0 read",("%.2x %.2x\n", offset, data)); */
	return data;
}

READ_HANDLER( ega_port_03c0_r)
{
	int data=0xff;
	switch (offset) {
	case 2: data=0xff;/*!*/break;
	}
	DBG_LOG(1,"ega 0x3c0 read",("%.2x %.2x\n", offset, data));
	return data;
}

READ_HANDLER( vga_port_03c0_r )
{
	int data=0xff;
	switch (offset) {
	case 1:
		if (vga.attribute.state==0) {
			data=vga.attribute.index;
			DBG_LOG(2,"vga attr index read",("%.2x %.2x\n",offset,data));
		} else {
			if ((vga.attribute.index&0x1f)<sizeof(vga.attribute.data))
				data=vga.attribute.data[vga.attribute.index&0x1f];
			DBG_LOG(1,"vga attr read",("%.2x %.2x\n",vga.attribute.index&0x1f,data));
		}
		break;
	case 2:
		data=0;
		switch ((vga.miscellaneous_output>>2)&3) {
		case 3:
			if (vga.read_dipswitch(0)&1) data|=0x10;
			break;
		case 2:
			if (vga.read_dipswitch(0)&2) data|=0x10;
			break;
		case 1:
			if (vga.read_dipswitch(0)&4) data|=0x10;
			break;
		case 0:
			if (vga.read_dipswitch(0)&8) data|=0x10;
			break;
		}
		DBG_LOG(1,"vga dipswitch read",("%.2x %.2x\n",offset,data));
		break;
	case 3: data=vga.oak.reg;break;
	case 4:
		data=vga.sequencer.index;
		DBG_LOG(2,"vga sequencer index read",("%.2x %.2x\n",offset,data));
		break;
	case 5:
		if (vga.sequencer.index<sizeof(vga.sequencer.data))
			data=vga.sequencer.data[vga.sequencer.index];
		DBG_LOG(1,"vga sequencer read",("%.2x %.2x\n",vga.sequencer.index,data));
		break;
	case 6:
		data=vga.dac.mask;
		DBG_LOG(3,"vga dac mask read",("%.2x %.2x\n",offset,data));
		break;
	case 7:
		if (vga.dac.read) data=0;
		else data=3;
		DBG_LOG(3,"vga dac status read",("%.2x %.2x\n",offset,data));
		break;
	case 8:
		data=vga.dac.write_index;
		DBG_LOG(3,"vga dac writeindex read",("%.2x %.2x\n",offset,data));
		break;
	case 9:
		if (vga.dac.read) {
			switch (vga.dac.state++) {
			case 0:
				data=vga.dac.color[vga.dac.read_index].red;
				DBG_LOG(3,"vga dac red read",("%.2x %.2x\n",offset,data));
				break;
			case 1:
				data=vga.dac.color[vga.dac.read_index].green;
				DBG_LOG(3,"vga dac green read",("%.2x %.2x\n",offset,data));
				break;
			case 2:
				data=vga.dac.color[vga.dac.read_index].blue;
				DBG_LOG(3,"vga dac blue read",("%.2x %.2x\n",offset,data));
				break;
			}
			if (vga.dac.state==3) {
				vga.dac.state=0; vga.dac.read_index++;
			}
		} else {
			DBG_LOG(1,"vga dac color read",("%.2x %.2x\n",offset,data));
		}
		break;
	case 0xa:
		data=vga.feature_control;
		DBG_LOG(1,"vga feature control read",("%.2x %.2x\n",offset,data));
		break;
	case 0xc:
		data=vga.miscellaneous_output;
		DBG_LOG(1,"vga miscellaneous read",("%.2x %.2x\n",offset,data));
		break;
	case 0xe:
		data=vga.gc.index;
		DBG_LOG(2,"vga gc index read",("%.2x %.2x\n",offset,data));
		break;
	case 0xf:
		if (vga.gc.index<sizeof(vga.gc.data)) {
			data=vga.gc.data[vga.gc.index];
		}
		DBG_LOG(1,"vga gc read",("%.2x %.2x\n",vga.gc.index,data));
		break;
	default:
		DBG_LOG(1,"03c0 read",("%.2x %.2x\n",offset,data));
	}
	return data;
}

READ_HANDLER(vga_port_03d0_r)
{
	int data=0xff;
	if (CRTC_PORT_ADDR==0x3d0)
		data=vga_crtc_r(offset);
	/*DBG_LOG(1,"vga 0x3d0 read",("%.2x %.2x\n", offset, data)); */
	return data;
}

WRITE_HANDLER( vga_port_03b0_w )
{
	LOG(("vga_port_03b0_w(): port=0x%04x data=0x%02x\n", offset + 0x3b0, data));

	if (CRTC_PORT_ADDR!=0x3b0) return;
	vga_crtc_w(offset, data);
}

WRITE_HANDLER(vga_port_03c0_w)
{
	LOG(("vga_port_03c0_w(): port=0x%04x data=0x%02x\n", offset + 0x3c0, data));

	switch (offset) {
	case 0:
		if (vga.attribute.state==0) {
			vga.attribute.index=data;
			DBG_LOG(2,"vga attr index write",("%.2x %.2x\n",offset,data));
		} else {
			if ((vga.attribute.index&0x1f)<sizeof(vga.attribute.data))
				vga.attribute.data[vga.attribute.index&0x1f]=data;
			DBG_LOG(1,"vga attr write",("%.2x %.2x\n",vga.attribute.index,data));
		}
		vga.attribute.state=!vga.attribute.state;
		break;
	case 2:
		vga.miscellaneous_output=data;
		DBG_LOG(1,"vga miscellaneous write",("%.2x %.2x\n",offset,data));
		break;
	case 3:
		vga.oak.reg=data;
		break;
	case 4:
		vga.sequencer.index=data;
		DBG_LOG(2,"vga sequencer index write",("%.2x %.2x\n",offset,data));
		break;
	case 5:
		if (vga.sequencer.index<sizeof(vga.sequencer.data))
			vga.sequencer.data[vga.sequencer.index]=data;
		vga_cpu_interface();
		if (vga.sequencer.index==0) vga.monitor.start_time=timer_get_time();
		DBG_LOG(1,"vga sequencer write",("%.2x %.2x\n",vga.sequencer.index,data));
		break;
	case 6:
		vga.dac.mask=data;
		DBG_LOG(3,"vga dac mask write",("%.2x %.2x\n",offset,data));
		break;
	case 7:
		vga.dac.read_index=data;
		vga.dac.state=0;
		vga.dac.read=1;
		DBG_LOG(3,"vga dac readindex write",("%.2x %.2x\n",offset,data));
		break;
	case 8:
		vga.dac.write_index=data;
		vga.dac.state=0;
		vga.dac.read=0;
		DBG_LOG(3,"vga dac writeindex write",("%.2x %.2x\n",offset,data));
		break;
	case 9:
		if (!vga.dac.read) {
			switch (vga.dac.state++) {
			case 0:
				vga.dac.color[vga.dac.write_index].red=data;
				DBG_LOG(3,"vga dac red write",("%.2x %.2x\n",offset,data));
				break;
			case 1:
				vga.dac.color[vga.dac.write_index].green=data;
				DBG_LOG(3,"vga dac green write",("%.2x %.2x\n",offset,data));
				break;
			case 2:
				vga.dac.color[vga.dac.write_index].blue=data;
				DBG_LOG(3,"vga dac blue write",("%.2x %.2x\n",offset,data));
				break;
			}
			vga.dac.dirty=1;
			if (vga.dac.state==3) {
				vga.dac.state=0; vga.dac.write_index++;
#if 0
				if (vga.dac.write_index==64) {
					int i;
					printf("start palette\n");
					for (i=0;i<64;i++) {
						printf(" 0x%.2x, 0x%.2x, 0x%.2x,\n",
							   vga.dac.color[i].red*4,
							   vga.dac.color[i].green*4,
							   vga.dac.color[i].blue*4);
					}
				}
#endif
			}
		} else {
			DBG_LOG(1,"vga dac color write",("%.2x %.2x\n",offset,data));
		}
		break;
	case 0xe:
		vga.gc.index=data;
		DBG_LOG(2,"vga gc index write",("%.2x %.2x\n",offset,data));
		break;
	case 0xf:
		if (vga.gc.index<sizeof(vga.gc.data))
			vga.gc.data[vga.gc.index]=data;
		vga_cpu_interface();
		DBG_LOG(1,"vga gc data write",("%.2x %.2x\n",vga.gc.index,data));
		break;
	default:
		DBG_LOG(1,"vga write",("%.3x %.2x\n",0x3c0+offset,data));
	}
}

WRITE_HANDLER(vga_port_03d0_w)
{
	LOG(("vga_port_03d0_w(): port=0x%04x data=0x%02x\n", offset + 0x3d0, data));

	if (CRTC_PORT_ADDR==0x3d0)
		vga_crtc_w(offset,data);
}

READ_HANDLER( paradise_ega_03c0_r )
{
	int data=vga_port_03c0_r(offset);
	if (offset==2) {
		if ( (vga.feature_control&3)==2 ) {
			data=(data&~0x60)|((vga.read_dipswitch(0)&0xc0)>>1);
		} else if ((vga.feature_control&3)==1 ) {
			data=(data&~0x60)|((vga.read_dipswitch(0)&0x30)<<1);
		}
	}
	return data;
}

void vga_reset(void)
{
	UINT8 *memory=vga.memory, *dirty=vga.dirty, *fontdirty=vga.fontdirty;

    read8_handler read_dipswitch=vga.read_dipswitch;

	memset(&vga,0, sizeof(vga));

	vga.memory=memory;
	vga.dirty=dirty;
	vga.fontdirty=fontdirty;

	vga.read_dipswitch=read_dipswitch;
	vga.log=0;
	vga.gc.data[6]=0xc; /* prevent xtbios excepting vga ram as system ram */
/* amstrad pc1640 bios relies on the position of
   the video memory area,
   so I introduced the reset to switch to b8000 area */
	vga.sequencer.data[4]=0;
	vga_cpu_interface();
}

void vga_init(read8_handler read_dipswitch)
{
	int i, j, k, mask;

	for (j=0; j<8; j++) {
		for (i=0; i<256; i++) {
			rotate_right[j][i]=i>>j;
			rotate_right[j][i]|=i<<(8-j);
		}
	}

	for (k=0;k<4;k++) {
		for (mask=0x80, j=0; j<8; j++, mask>>=1) {
			for  (i=0; i<256; i++) {
				color_bitplane_to_packed[k][j][i]=(i&mask)?(1<<k):0;
			}
		}
	}
	vga.read_dipswitch = read_dipswitch;
	vga.memory =	(UINT8*) auto_malloc(0x40000);
	vga.dirty =		(UINT8*) auto_malloc(0x40000);
	vga.fontdirty =	(UINT8*) auto_malloc(0x800);
	vga_reset();
}

static void vga_timer(int param)
{
	vga.monitor.retrace=1;
}

VIDEO_START( ega )
{
	vga.monitor.get_clock = ega_get_clock;
	vga.monitor.get_lines = ega_get_crtc_lines;
	vga.monitor.get_columns = ega_get_crtc_columns;
	vga.monitor.get_sync_lines = vga_get_crtc_sync_lines;
	vga.monitor.get_sync_columns = vga_get_crtc_sync_columns;
	timer_pulse(1.0/60, 0, vga_timer);
	pc_video_start(NULL, pc_ega_choosevideomode, 0);
	return 0;
}

VIDEO_START( vga )
{
	vga.monitor.get_clock=vga_get_clock;
	vga.monitor.get_lines=vga_get_crtc_lines;
	vga.monitor.get_columns=vga_get_crtc_columns;
	vga.monitor.get_sync_lines=vga_get_crtc_sync_lines;
	vga.monitor.get_sync_columns=vga_get_crtc_sync_columns;
	timer_pulse(1.0/60, 0, vga_timer);
	pc_video_start(NULL, pc_vga_choosevideomode, 0);
	return 0;
}

#define myMIN(a, b) ((a) < (b) ? (a) : (b))
#define myMAX(a, b) ((a) > (b) ? (a) : (b))

static void vga_vh_text(struct mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	UINT8 ch, attr;
	UINT8 bits;
	UINT8 *font;
	UINT16 *bitmapline;
	int width=CHAR_WIDTH, height=CRTC6845_CHAR_HEIGHT;
	int pos, line, column, mask, w, h, addr;
	pen_t pen;

	if (CRTC6845_CURSOR_MODE!=CRTC6845_CURSOR_OFF)
	{
		if (++vga.cursor.time>=0x10)
		{
			vga.cursor.visible^=1;
			vga.cursor.time=0;
		}
	}

	for (addr = TEXT_START_ADDRESS, line = -CRTC6845_SKEW; line < TEXT_LINES;
		 line += height, addr += TEXT_LINE_LENGTH)
	{
		for (pos = addr, column=0; column<TEXT_COLUMNS; column++, pos++)
		{
			ch   = vga.memory[(pos<<2) + 0];
			attr = vga.memory[(pos<<2) + 1];
			font = vga.memory+2+(ch<<(5+2))+FONT1;

			for (h = myMAX(-line, 0); (h < height) && (line+h < myMIN(TEXT_LINES, bitmap->height)); h++)
			{
				bitmapline = (UINT16 *) bitmap->line[line+h];
				bits = font[h<<2];

				assert(bitmapline);

				for (mask=0x80, w=0; (w<width)&&(w<8); w++, mask>>=1)
				{
					if (bits&mask)
						pen = vga.pens[attr & 0x0f];
					else
						pen = vga.pens[attr >> 4];
					bitmapline[column*width+w] = pen;
				}
				if (w<width)
				{
					/* 9 column */
					if (TEXT_COPY_9COLUMN(ch)&&(bits&1))
						pen = vga.pens[attr & 0x0f];
					else
						pen = vga.pens[attr >> 4];
					bitmapline[column*width+w] = pen;
				}
			}
			if ((CRTC6845_CURSOR_MODE!=CRTC6845_CURSOR_OFF)
				&&vga.cursor.visible&&(pos==CRTC6845_CURSOR_POS))
			{
				for (h=CRTC6845_CURSOR_TOP;
					 (h<=CRTC6845_CURSOR_BOTTOM)&&(h<height)&&(line+h<TEXT_LINES);
					 h++)
				{
					plot_box(bitmap, column*width, line+h, width, 1, vga.pens[attr&0xf]);
				}
			}
		}
	}
}

static void vga_vh_ega(struct mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	int pos, line, column, c, addr, i;
	int height = CRTC6845_CHAR_HEIGHT;
	UINT16 *bitmapline;
	UINT16 *newbitmapline;
	pen_t pen;

	for (addr=EGA_START_ADDRESS, pos=0, line=0; line<LINES;
		 line += height, addr=(addr+EGA_LINE_LENGTH)&0x3ffff)
	{
		bitmapline = (UINT16 *) Machine->scrbitmap->line[line];

		for (pos=addr, c=0, column=0; column<EGA_COLUMNS; column++, c+=8, pos=(pos+4)&0x3ffff)
		{
			int data[4];

			data[0]=vga.memory[pos];
			data[1]=vga.memory[pos+1]<<1;
			data[2]=vga.memory[pos+2]<<2;
			data[3]=vga.memory[pos+3]<<3;

			for (i = 7; i >= 0; i--)
			{
				pen = vga.pens[(data[0]&1) | (data[1]&2) | (data[2]&4) | (data[3]&8)];
				bitmapline[c+i] = pen;

				data[0]>>=1;
				data[1]>>=1;
				data[2]>>=1;
				data[3]>>=1;
			}
		}

		for (i = 1; i < height; i++)
		{
			if (line + i >= LINES)
				break;

			newbitmapline = (UINT16 *) Machine->scrbitmap->line[line+i];
			memcpy(newbitmapline, bitmapline, EGA_COLUMNS * 8 * sizeof(UINT16));
		}
	}
}

static void vga_vh_vga(struct mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	int pos, line, column, c, addr;
	UINT16 *bitmapline;

	for (addr=VGA_START_ADDRESS, line=0; line<LINES; line++, addr+=VGA_LINE_LENGTH)
	{
		bitmapline = (UINT16 *) bitmap->line[line];
		for (pos=addr, c=0, column=0; column<VGA_COLUMNS; column++, c+=8, pos+=0x20)
		{
			//if (*(UINT32*)(vga.dirty+pos))
			{
				bitmapline[c+0] = vga.memory[pos+0];
				bitmapline[c+1] = vga.memory[pos+1];
				bitmapline[c+2] = vga.memory[pos+2];
				bitmapline[c+3] = vga.memory[pos+3];
				//*(UINT32*)(vga.dirty+pos)=0;
			}
			//if (*(UINT32*)(vga.dirty+pos+0x10))
			{
				bitmapline[c+4] = vga.memory[pos+0x10];
				bitmapline[c+5] = vga.memory[pos+0x11];
				bitmapline[c+6] = vga.memory[pos+0x12];
				bitmapline[c+7] = vga.memory[pos+0x13];
				//*(UINT32*)(vga.dirty+pos+0x10)=0;
			}
		}
	}
}

static pc_video_update_proc pc_ega_choosevideomode(int *width, int *height, struct crtc6845 *crtc)
{
	pc_video_update_proc proc = NULL;
	int i;

	if (CRTC_ON)
	{
		for (i = 0; i < 16; i++)
			vga.pens[i]=Machine->pens[i/*vga.attribute.data[i]&0x3f*/];

		if (!GRAPHIC_MODE)
		{
			proc = vga_vh_text;
			*height = TEXT_LINES;
			*width = TEXT_COLUMNS*CHAR_WIDTH;
		}
		else
		{
			proc = vga_vh_ega;
			*height = LINES;
			*width = EGA_COLUMNS*8;
		}
	}
	return proc;
}

static pc_video_update_proc pc_vga_choosevideomode(int *width, int *height, struct crtc6845 *crtc)
{
	pc_video_update_proc proc = NULL;
	int i;

	if (CRTC_ON)
	{
		if (vga.dac.dirty)
		{
			for (i=0; i<256;i++)
			{
				palette_set_color(i,(vga.dac.color[i].red & 0x3f) << 2,
									 (vga.dac.color[i].green & 0x3f) << 2,
									 (vga.dac.color[i].blue & 0x3f) << 2);
			}
			vga.dac.dirty = 0;
		}

		if (vga.attribute.data[0x10] & 0x80)
		{
			for (i=0; i<16;i++)
			{
				vga.pens[i] = Machine->pens[(vga.attribute.data[i]&0x0f)
										 |((vga.attribute.data[0x14]&0xf)<<4)];
			}
		}
		else
		{
			for (i=0; i<16;i++)
			{
				vga.pens[i]=Machine->pens[(vga.attribute.data[i]&0x3f)
										 |((vga.attribute.data[0x14]&0xc)<<4)];
			}
		}

		if (!GRAPHIC_MODE)
		{
			proc = vga_vh_text;
			*height = TEXT_LINES;
			*width = TEXT_COLUMNS * CHAR_WIDTH;
		}
		else if (vga.gc.data[5]&0x40)
		{
			proc = vga_vh_vga;
			*height = LINES;
			*width = VGA_COLUMNS * 8;
		}
		else
		{
			proc = vga_vh_ega;
			*height = LINES;
			*width = EGA_COLUMNS * 8;
		}
	}
	return proc;
}
