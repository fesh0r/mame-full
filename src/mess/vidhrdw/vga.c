/* 
   IBM PC, PC/XT, PC/AT, PS2 25/30
   Video Graphics Adapter

   peter.trauner@jk.uni-linz.ac.at
*/

#include "driver.h"

#define VERBOSE_DBG 1
#include "mess/machine/cbm.h"

#include "vga.h"

/* vga
 standard compatible to mda, cga, hercules?, ega
 (mda, cga, hercules not real register compatible)
 several vga cards drive also mda, cga, ega monitors
 some vga cards have register compatible mda, cga, hercules modes

 ega/vga
 64k (early ega 16k) words of 32 bit memory
 */

unsigned char vga_palette[0x100*3] = { 0 };

/* grabbed from dac inited by s3 trio64 bios */
unsigned char ega_palette[0x40*3] = {
0x00, 0x00, 0x00, 
0x00, 0x00, 0xa8, 
0x00, 0xa8, 0x00, 
0x00, 0xa8, 0xa8, 
0xa8, 0x00, 0x00, 
0xa8, 0x00, 0xa8, 
0xa8, 0xa8, 0x00, 
0xa8, 0xa8, 0xa8, 
0x00, 0x00, 0x54, 
0x00, 0x00, 0xfc, 
0x00, 0xa8, 0x54, 
0x00, 0xa8, 0xfc, 
0xa8, 0x00, 0x54, 
0xa8, 0x00, 0xfc, 
0xa8, 0xa8, 0x54, 
0xa8, 0xa8, 0xfc, 
0x00, 0x54, 0x00, 
0x00, 0x54, 0xa8, 
0x00, 0xfc, 0x00, 
0x00, 0xfc, 0xa8, 
0xa8, 0x54, 0x00, 
0xa8, 0x54, 0xa8, 
0xa8, 0xfc, 0x00, 
0xa8, 0xfc, 0xa8, 
0x00, 0x54, 0x54, 
0x00, 0x54, 0xfc, 
0x00, 0xfc, 0x54, 
0x00, 0xfc, 0xfc, 
0xa8, 0x54, 0x54, 
0xa8, 0x54, 0xfc, 
0xa8, 0xfc, 0x54, 
0xa8, 0xfc, 0xfc, 
0x54, 0x00, 0x00, 
0x54, 0x00, 0xa8, 
0x54, 0xa8, 0x00, 
0x54, 0xa8, 0xa8, 
0xfc, 0x00, 0x00, 
0xfc, 0x00, 0xa8, 
0xfc, 0xa8, 0x00, 
0xfc, 0xa8, 0xa8, 
0x54, 0x00, 0x54, 
0x54, 0x00, 0xfc, 
0x54, 0xa8, 0x54, 
0x54, 0xa8, 0xfc, 
0xfc, 0x00, 0x54, 
0xfc, 0x00, 0xfc, 
0xfc, 0xa8, 0x54, 
0xfc, 0xa8, 0xfc, 
0x54, 0x54, 0x00, 
0x54, 0x54, 0xa8, 
0x54, 0xfc, 0x00, 
0x54, 0xfc, 0xa8, 
0xfc, 0x54, 0x00, 
0xfc, 0x54, 0xa8, 
0xfc, 0xfc, 0x00, 
0xfc, 0xfc, 0xa8, 
0x54, 0x54, 0x54, 
0x54, 0x54, 0xfc, 
0x54, 0xfc, 0x54, 
0x54, 0xfc, 0xfc, 
0xfc, 0x54, 0x54, 
0xfc, 0x54, 0xfc, 
0xfc, 0xfc, 0x54, 
0xfc, 0xfc, 0xfc
};

static UINT8 rotate_right[8][256];
static UINT8 color_bitplane_to_packed[4/*plane*/][8/*pixel*/][256];

static struct {
	mem_read_handler read_dipswitch;

	UINT8 *memory;
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
		bool retrace;
	} monitor;

	/* oak vga */
	struct { UINT8 reg; } oak;

	int log;
} vga;

#define DOUBLESCAN ((vga.crtc.data[9]&0x80)||((vga.crtc.data[9]&0x1f)!=0) )
#define CRTC_PORT_ADDR ((vga.miscellaneous_output&1)?0x3d0:0x3b0)

#define CRTC_ON (vga.crtc.data[0x17]&0x80)

#define LINES_HELPER ( (vga.crtc.data[0x12] \
				|((vga.crtc.data[7]&2)<<7) \
				|((vga.crtc.data[7]&0x40)<<3))+1 )
#define TEXT_LINES (LINES_HELPER)
#define LINES (DOUBLESCAN?LINES_HELPER>>1:LINES_HELPER)

#define GRAPHIC_MODE (vga.gc.data[6]&1) /* else textmodus */

#define EGA_COLUMNS (vga.crtc.data[1]+1)
#define EGA_START_ADDRESS ((vga.crtc.data[0xd]|(vga.crtc.data[0xc]<<8))<<2)
#define EGA_LINE_LENGTH (vga.crtc.data[0x13]<<3)

#define VGA_COLUMNS (EGA_COLUMNS>>1)
#define VGA_START_ADDRESS (EGA_START_ADDRESS)
#define VGA_LINE_LENGTH (EGA_LINE_LENGTH<<2)

#define CHAR_WIDTH ((vga.sequencer.data[1]&1)?8:9)
#define CHAR_HEIGHT ((vga.crtc.data[9]&0x1f)+1)

#define TEXT_COLUMNS (vga.crtc.data[1]+1)
#define TEXT_START_ADDRESS (EGA_START_ADDRESS)
#define TEXT_LINE_LENGTH (EGA_LINE_LENGTH>>2)

#define TEXT_COPY_9COLUMN(ch) ( (ch>=192)&&(ch<=223)&&(vga.attribute.data[0x10]&4))

#define CURSOR_ON (!(vga.crtc.data[0xa]&0x20))
#define CURSOR_STARTLINE (vga.crtc.data[0xa]&0x1f)
#define CURSOR_ENDLINE (vga.crtc.data[0xb]&0x1f)
#define CURSOR_POS (vga.crtc.data[0xf]|(vga.crtc.data[0xe]<<8))

#define FONT1 ( ((vga.sequencer.data[3]&3)|((vga.sequencer.data[3]&0x10)<<2))*0x2000)
#define FONT2 ( ((vga.sequencer.data[3]&c)>>2)|((vga.sequencer.data[3]&0x20)<<3))*0x2000)

static READ_HANDLER(vga_text_r)
{
	int data;
	data=vga.memory[((offset&~1)<<1)|(offset&1)];

	return data;
}

static WRITE_HANDLER(vga_text_w)
{
	vga.memory[((offset&~1)<<1)|(offset&1)]=data;
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
	exit(1);
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
	exit(1);
}

static WRITE_HANDLER(vga_ega_w)
{
	if (vga.sequencer.data[2]&1) 
		vga.memory[offset<<2]=vga_latch_write(0,data);
	if (vga.sequencer.data[2]&2) 
		vga.memory[(offset<<2)+1]=vga_latch_write(1,data);
	if (vga.sequencer.data[2]&4) 
		vga.memory[(offset<<2)+2]=vga_latch_write(2,data);
	if (vga.sequencer.data[2]&8)
		vga.memory[(offset<<2)+3]=vga_latch_write(3,data);
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
	vga.memory[((offset&~3)<<2)|(offset&3)]=data;
}

static void vga_cpu_interface(void)
{
	static int sequencer, gc;
	mem_read_handler read;
	mem_write_handler write;

	if ((gc==vga.gc.data[6])&&(sequencer==vga.sequencer.data[4])) return;

	gc=vga.gc.data[6];
	sequencer=vga.sequencer.data[4];

	if (vga.sequencer.data[4]&8) {
		read=vga_vga_r;
		write=vga_vga_w;
		DBG_LOG(1,"vga memory",(errorlog,"vga\n"));
	} else if (vga.sequencer.data[4]&4) {
		read=vga_ega_r;
		write=vga_ega_w;
		DBG_LOG(1,"vga memory",(errorlog,"ega\n"));
	} else {
		read=vga_text_r;
		write=vga_text_w;
		DBG_LOG(1,"vga memory",(errorlog,"text\n"));
	}
	switch (vga.gc.data[6]&0xc) {
	case 0:
		cpu_setbank(1,vga.memory);
		cpu_setbank(2,vga.memory+0x10000);
		cpu_setbank(3,vga.memory+0x18000);
		cpu_setbankhandler_r(1, MRA_BANK1);
		cpu_setbankhandler_r(2, MRA_BANK2);
		cpu_setbankhandler_r(3, MRA_BANK3);
		cpu_setbankhandler_w(1, MWA_BANK1);
		cpu_setbankhandler_w(2, MWA_BANK2);
		cpu_setbankhandler_w(3, MWA_BANK3);
		DBG_LOG(1,"vga memory",(errorlog,"a0000-bffff\n"));
		break;
	case 4:
		cpu_setbankhandler_r(1, read);
		cpu_setbankhandler_r(2, MRA_NOP);
		cpu_setbankhandler_r(3, MRA_NOP);
		cpu_setbankhandler_w(1, write);
		cpu_setbankhandler_w(2, MWA_NOP);
		cpu_setbankhandler_w(3, MWA_NOP);
		DBG_LOG(1,"vga memory",(errorlog,"a0000-affff\n"));
		break;
	case 8:
		cpu_setbankhandler_r(1, MRA_NOP);
		cpu_setbankhandler_r(2, read );
		cpu_setbankhandler_r(3, MRA_NOP);
		cpu_setbankhandler_w(1, MWA_NOP);
		cpu_setbankhandler_w(2, write );
		cpu_setbankhandler_w(3, MWA_NOP);
		DBG_LOG(1,"vga memory",(errorlog,"b0000-b7fff\n"));
		break;
	case 0xc:
		cpu_setbankhandler_r(1, MRA_NOP);
		cpu_setbankhandler_r(2, MRA_NOP);
		cpu_setbankhandler_r(3, read);
		cpu_setbankhandler_w(1, MWA_NOP);
		cpu_setbankhandler_w(2, MWA_NOP);
		cpu_setbankhandler_w(3, write);
		DBG_LOG(1,"vga memory",(errorlog,"b8000-bffff\n"));
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
		DBG_LOG(1,"vga crtc read",(errorlog,"%.2x %.2x\n",vga.crtc.index,data));
		break;
	case 0xa:
		vga.attribute.state=0;
		data=0;//4;
		if (vga.monitor.retrace) data|=9;
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

		//DBG_LOG(1,"vga crtc 0x3[bd]a",(errorlog,"%.2x %.2x\n",offset,data));
		break;
	case 0xf:
		/* oak test */
		data=0;
		DBG_LOG(1,"0x3[bd]0 read",(errorlog,"%.2x %.2x\n",offset,data));
		break;
	default:
		DBG_LOG(1,"0x3[bd]0 read",(errorlog,"%.2x %.2x\n",offset,data));
	}
	return data;
}

static WRITE_HANDLER(vga_crtc_w)
{
	switch (offset) {
	case 0xa:
		DBG_LOG(1,"vga feature control write",(errorlog,"%.2x %.2x\n",offset,data));
		vga.feature_control=data;
		break;
	case 4: vga.crtc.index=data;break;
	case 5: 
		DBG_LOG(1,"vga crtc write",(errorlog,"%.2x %.2x\n",vga.crtc.index,data));
		if (vga.crtc.index<sizeof(vga.crtc.data)) 
			vga.crtc.data[vga.crtc.index]=data;
		break;
	default:
		DBG_LOG(1,"0x3[bd]0 write",(errorlog,"%.2x %.2x\n",offset,data));
	}
}

READ_HANDLER( vga_port_03b0_r )
{
	int data=0xff;
	if (CRTC_PORT_ADDR==0x3b0)
		data=vga_crtc_r(offset);
	//DBG_LOG(1,"vga 0x3b0 read",(errorlog,"%.2x %.2x\n", offset, data));
	return data;
}

READ_HANDLER( ega_port_03c0_r)
{
	int data=0xff;
	switch (offset) {
	case 2: data=0xff;/*!*/break;
	}
	DBG_LOG(1,"ega 0x3c0 read",(errorlog,"%.2x %.2x\n", offset, data));
	return data;
}

READ_HANDLER( vga_port_03c0_r )
{
	int data=0xff;
	switch (offset) {
	case 1:
		if (vga.attribute.state==0) {
			data=vga.attribute.index;
			DBG_LOG(2,"vga attr index read",(errorlog,"%.2x %.2x\n",offset,data));
		} else {
			if ((vga.attribute.index&0x1f)<sizeof(vga.attribute.data))
				data=vga.attribute.data[vga.attribute.index&0x1f];
			DBG_LOG(1,"vga attr read",(errorlog,"%.2x %.2x\n",vga.attribute.index&0x1f,data));
		}
		break;
	case 2:
		data=0;
		switch ((vga.miscellaneous_output>>2)&3) {
		case 0:
			if (vga.read_dipswitch(0)&1) data|=0x10;
			break;
		case 1:
			if (vga.read_dipswitch(0)&2) data|=0x10;
			break;
		case 2:
			if (vga.read_dipswitch(0)&4) data|=0x10;
			break;
		case 3:
			if (vga.read_dipswitch(0)&8) data|=0x10;
			break;
		}
		DBG_LOG(1,"vga dipswitch read",(errorlog,"%.2x %.2x\n",offset,data));
		break;
	case 3: data=vga.oak.reg;break;
	case 4: 
		data=vga.sequencer.index;
		DBG_LOG(2,"vga sequencer index read",(errorlog,"%.2x %.2x\n",offset,data));
		break;
	case 5: 
		if (vga.sequencer.index<sizeof(vga.sequencer.data))
			data=vga.sequencer.data[vga.sequencer.index];
		DBG_LOG(1,"vga sequencer read",(errorlog,"%.2x %.2x\n",vga.sequencer.index,data));
		break;
	case 6: 
		data=vga.dac.mask;
		DBG_LOG(3,"vga dac mask read",(errorlog,"%.2x %.2x\n",offset,data));
		break;
	case 7: 
		if (vga.dac.read) data=0;
		else data=3;
		DBG_LOG(3,"vga dac status read",(errorlog,"%.2x %.2x\n",offset,data));
		break;
	case 8: 
		data=vga.dac.write_index;
		DBG_LOG(3,"vga dac writeindex read",(errorlog,"%.2x %.2x\n",offset,data));
		break;
	case 9:
		if (vga.dac.read) {
			switch (vga.dac.state++) {
			case 0: 
				data=vga.dac.color[vga.dac.read_index].red;
				DBG_LOG(3,"vga dac red read",(errorlog,"%.2x %.2x\n",offset,data));
				break;
			case 1: 
				data=vga.dac.color[vga.dac.read_index].green;
				DBG_LOG(3,"vga dac green read",(errorlog,"%.2x %.2x\n",offset,data));
				break;
			case 2: 
				data=vga.dac.color[vga.dac.read_index].blue;
				DBG_LOG(3,"vga dac blue read",(errorlog,"%.2x %.2x\n",offset,data));
				break;
			}
			if (vga.dac.state==3) {
				vga.dac.state=0; vga.dac.read_index++;
			}
		} else {
			DBG_LOG(1,"vga dac color read",(errorlog,"%.2x %.2x\n",offset,data));
		}
		break;
	case 0xa: 
		data=vga.feature_control;
		DBG_LOG(1,"vga feature control read",(errorlog,"%.2x %.2x\n",offset,data));
		break;
	case 0xc: 
		data=vga.miscellaneous_output;
		DBG_LOG(1,"vga miscellaneous read",(errorlog,"%.2x %.2x\n",offset,data));
		break;
	case 0xe: 
		data=vga.gc.index;
		DBG_LOG(2,"vga gc index read",(errorlog,"%.2x %.2x\n",offset,data));
		break;
	case 0xf: 
		if (vga.gc.index<sizeof(vga.gc.data)) {
			data=vga.gc.data[vga.gc.index];
		}
		DBG_LOG(1,"vga gc read",(errorlog,"%.2x %.2x\n",vga.gc.index,data));
		break;
	default:
		DBG_LOG(1,"03c0 read",(errorlog,"%.2x %.2x\n",offset,data));
	}
	return data;
}

READ_HANDLER(vga_port_03d0_r)
{
	int data=0xff;
	if (CRTC_PORT_ADDR==0x3d0)
		data=vga_crtc_r(offset);
	//DBG_LOG(1,"vga 0x3d0 read",(errorlog,"%.2x %.2x\n", offset, data));
	return data;
}

WRITE_HANDLER( vga_port_03b0_w )
{
	//DBG_LOG(1,"vga 0x3b0 write",(errorlog,"%.2x %.2x\n", offset, data));
	if (CRTC_PORT_ADDR!=0x3b0) return;
	vga_crtc_w(offset, data);
}

WRITE_HANDLER(vga_port_03c0_w)
{
	//DBG_LOG(1,"03c0 write",(errorlog,"%.2x %.2x\n",offset,data));
	switch (offset) {
	case 0:
		if (vga.attribute.state==0) {
			vga.attribute.index=data;
			DBG_LOG(2,"vga attr index write",(errorlog,"%.2x %.2x\n",offset,data));
		} else {
			if ((vga.attribute.index&0x1f)<sizeof(vga.attribute.data))
				vga.attribute.data[vga.attribute.index&0x1f]=data;
			DBG_LOG(1,"vga attr write",(errorlog,"%.2x %.2x\n",vga.attribute.index,data));
		}
		vga.attribute.state=!vga.attribute.state;
		break;
	case 2: 
		vga.miscellaneous_output=data;
		DBG_LOG(1,"vga miscellaneous write",(errorlog,"%.2x %.2x\n",offset,data));
		break;
	case 3: 
		vga.oak.reg=data;
		break;
	case 4: 
		vga.sequencer.index=data;
		DBG_LOG(2,"vga sequencer index write",(errorlog,"%.2x %.2x\n",offset,data));
		break;
	case 5:
		if (vga.sequencer.index<sizeof(vga.sequencer.data))
			vga.sequencer.data[vga.sequencer.index]=data;
		vga_cpu_interface();
		DBG_LOG(1,"vga sequencer write",(errorlog,"%.2x %.2x\n",vga.sequencer.index,data));
		break;
	case 6: 
		vga.dac.mask=data;
		DBG_LOG(3,"vga dac mask write",(errorlog,"%.2x %.2x\n",offset,data));
		break;
	case 7: 
		vga.dac.read_index=data;
		vga.dac.state=0;
		vga.dac.read=1;
		DBG_LOG(3,"vga dac readindex write",(errorlog,"%.2x %.2x\n",offset,data));
		break;
	case 8: 
		vga.dac.write_index=data;
		vga.dac.state=0;
		vga.dac.read=0;
		DBG_LOG(3,"vga dac writeindex write",(errorlog,"%.2x %.2x\n",offset,data));
		break;
	case 9:
		if (!vga.dac.read) {
			switch (vga.dac.state++) {
			case 0: 
				vga.dac.color[vga.dac.write_index].red=data;
				DBG_LOG(3,"vga dac red write",(errorlog,"%.2x %.2x\n",offset,data));
				break;
			case 1:
				vga.dac.color[vga.dac.write_index].green=data;
				DBG_LOG(3,"vga dac green write",(errorlog,"%.2x %.2x\n",offset,data));
				break;
			case 2: 
				vga.dac.color[vga.dac.write_index].blue=data;
				DBG_LOG(3,"vga dac blue write",(errorlog,"%.2x %.2x\n",offset,data));
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
			DBG_LOG(1,"vga dac color write",(errorlog,"%.2x %.2x\n",offset,data));
		}
		break;
	case 0xe: 
		vga.gc.index=data;
		DBG_LOG(2,"vga gc index write",(errorlog,"%.2x %.2x\n",offset,data));
		break;
	case 0xf:
		if (vga.gc.index<sizeof(vga.gc.data)) 
			vga.gc.data[vga.gc.index]=data;
		vga_cpu_interface();
		DBG_LOG(1,"vga gc data write",(errorlog,"%.2x %.2x\n",vga.gc.index,data));
		break;
	default:
		DBG_LOG(1,"vga write",(errorlog,"%.3x %.2x\n",0x3c0+offset,data));
	}
}

WRITE_HANDLER(vga_port_03d0_w)
{
	//DBG_LOG(1,"vga 0x3d0 write",(errorlog,"%.2x %.2x\n", offset, data));
	if (CRTC_PORT_ADDR==0x3d0)
		vga_crtc_w(offset,data);
}

void vga_reset(void)
{
	UINT8 *memory=vga.memory;
    mem_read_handler read_dipswitch=vga.read_dipswitch;

	memset(&vga,0, sizeof(vga));

	vga.memory=memory;
	vga.read_dipswitch=read_dipswitch;
	vga.log=0;
	vga.gc.data[6]=0xc; // prevent xtbios excepting vga ram as system ram
	vga_cpu_interface();
}

void vga_init(mem_read_handler read_dipswitch)
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
	vga.read_dipswitch=read_dipswitch;
	vga.memory=malloc(0x40000);
	vga_reset();
}

int vga_vh_start(void)
{
	return 0;
}

void vga_vh_stop(void)
{

}

void vga_vh_text(struct osd_bitmap *bitmap, int full_refresh)
{
	int width=CHAR_WIDTH, height=CHAR_HEIGHT;
	int pos, line, column, mask, w, h, addr;

	if (CURSOR_ON) {
		if (++vga.cursor.time>=0x10) {
			vga.cursor.visible^=1;
			vga.cursor.time=0;
		}		
	}

	for (addr=TEXT_START_ADDRESS, line=0; line<TEXT_LINES; 
		 line+=height, addr+=TEXT_LINE_LENGTH) {
		for (pos=addr, column=0; column<TEXT_COLUMNS; column++, pos++) {
			UINT8 ch=vga.memory[pos<<2], attr=vga.memory[(pos<<2)+1];
			UINT8 *font=vga.memory+2+(ch<<(5+2))+FONT1;

			for (h=0; (h<height)&&(line+h<TEXT_LINES); h++) {
				UINT8 bits=font[h<<2];
				for (mask=0x80, w=0; (w<width)&&(w<8); w++, mask>>=1) {
					if (bits&mask) {
						Machine->scrbitmap->line[line+h][column*width+w]=vga.pens[attr&0xf];
					} else {
						Machine->scrbitmap->line[line+h][column*width+w]=vga.pens[attr>>4];
					}
				}
				if (w<width) { // 9 column
					if (TEXT_COPY_9COLUMN(ch)&&(bits&1)) {
						Machine->scrbitmap->line[line+h][column*width+w]=vga.pens[attr&0xf];
					} else {
						Machine->scrbitmap->line[line+h][column*width+w]=vga.pens[attr>>4];
					}
				}
			}
			if (CURSOR_ON&&vga.cursor.visible&&(pos==CURSOR_POS)) {
				for (h=CURSOR_STARTLINE;
					 (h<=CURSOR_ENDLINE)&&(h<height)&&(line+h<TEXT_LINES);
					 h++) {
					for (w=0; w<width; w++)
						Machine->scrbitmap->line[line+h][column*width+w]=
							vga.pens[attr&0xf];
				}
			}			
		}
	}
}

void vga_vh_ega(struct osd_bitmap *bitmap, int full_refresh)
{
	int pos, line, column, c, addr;

	for (addr=EGA_START_ADDRESS, pos=0, line=0; line<LINES; 
		 line++, addr+=EGA_LINE_LENGTH) {
		for (pos=addr, c=0, column=0; column<EGA_COLUMNS; column++, c+=8, pos+=4) {
			Machine->scrbitmap->line[line][c]=
			    vga.pens[ega_bitplane_to_packed(vga.memory+pos,0)];
			Machine->scrbitmap->line[line][c+1]=
				vga.pens[ega_bitplane_to_packed(vga.memory+pos,1)];
			Machine->scrbitmap->line[line][c+2]=
			    vga.pens[ega_bitplane_to_packed(vga.memory+pos,2)];
			Machine->scrbitmap->line[line][c+3]=
				vga.pens[ega_bitplane_to_packed(vga.memory+pos,3)];
			Machine->scrbitmap->line[line][c+4]=
				vga.pens[ega_bitplane_to_packed(vga.memory+pos,4)];
			Machine->scrbitmap->line[line][c+5]=
				vga.pens[ega_bitplane_to_packed(vga.memory+pos,5)];
			Machine->scrbitmap->line[line][c+6]=
				vga.pens[ega_bitplane_to_packed(vga.memory+pos,6)];
			Machine->scrbitmap->line[line][c+7]=
				vga.pens[ega_bitplane_to_packed(vga.memory+pos,7)];
		}
	}
}

void vga_vh_vga(struct osd_bitmap *bitmap, int full_refresh)
{
	int pos, line, column, c, addr;

	for (addr=VGA_START_ADDRESS, line=0; line<LINES; line++, addr+=VGA_LINE_LENGTH) {
		for (pos=addr, c=0, column=0; column<VGA_COLUMNS; column++, c+=8, pos+=0x20) {
			Machine->scrbitmap->line[line][c]=
				Machine->pens[vga.memory[pos]];
			Machine->scrbitmap->line[line][c+1]=
				Machine->pens[vga.memory[pos+1]];
			Machine->scrbitmap->line[line][c+2]=
				Machine->pens[vga.memory[pos+2]];
			Machine->scrbitmap->line[line][c+3]=
				Machine->pens[vga.memory[pos+3]];
			Machine->scrbitmap->line[line][c+4]=
				Machine->pens[vga.memory[pos+0x10]];
			Machine->scrbitmap->line[line][c+5]=
				Machine->pens[vga.memory[pos+0x11]];
			Machine->scrbitmap->line[line][c+6]=
				Machine->pens[vga.memory[pos+0x12]];
			Machine->scrbitmap->line[line][c+7]=
				Machine->pens[vga.memory[pos+0x13]];
		}
	}
}

void ega_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	if (CRTC_ON) {
		int i;
		for (i=0; i<16;i++) {
			vga.pens[i]=Machine->pens[vga.attribute.data[i]&0x3f];
		}
		if (!GRAPHIC_MODE)
			vga_vh_text(bitmap, full_refresh);
		else 
			vga_vh_ega(bitmap, full_refresh);
	}
	vga.monitor.retrace=!vga.monitor.retrace;
}

void vga_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int i;
	if (CRTC_ON) {
		if (vga.dac.dirty) {
			for (i=0; i<256;i++) {
				palette_change_color(i,(vga.dac.color[i].red&0x3f)<<2,
									 (vga.dac.color[i].green&0x3f)<<2,
									 (vga.dac.color[i].blue&0x3f)<<2);
			}
			palette_recalc();
			vga.dac.dirty=0;
		}
		if (vga.attribute.data[0x10]&0x80) {
			for (i=0; i<16;i++) {
				vga.pens[i]=Machine->pens[(vga.attribute.data[i]&0x0f)
										 |((vga.attribute.data[0x14]&0xf)<<4)];
			}
		} else {
			for (i=0; i<16;i++) {
				vga.pens[i]=Machine->pens[(vga.attribute.data[i]&0x3f)
										 |((vga.attribute.data[0x14]&0xc)<<4)];
			}
		}
#if 0
		if (input_port_4_r(0)&2) {
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
		if (!GRAPHIC_MODE)
			vga_vh_text(bitmap, full_refresh);
		else if (vga.gc.data[5]&0x40)
			vga_vh_vga(bitmap, full_refresh);
		else
			vga_vh_ega(bitmap, full_refresh);
	}
	vga.monitor.retrace=!vga.monitor.retrace;
}
