#include "includes/pc1500.h"

static struct {
	UINT8 data[0x10];
} lh5811= { { 0 } };

/* upd1990ac (clock) connected to port c0..c5 b5 b6 
   40 bit serial data
   seconds minutes hours day week month

*/

READ_HANDLER( lh5811_r )
{
	int data=lh5811.data[offset];
	logerror("lh5811 read %x %.2x\n", offset, data);
	return data;
}

WRITE_HANDLER( lh5811_w )
{
	logerror("lh5811 write %x %.2x\n", offset, data);
	lh5811.data[offset]=data;
}

UINT8 pc1500_in(void)
{
	int data=0;
	if (lh5811.data[0xc]&1) {
		if (KEY_2) data|=1;
		if (KEY_5) data|=2;
		if (KEY_8) data|=4;
		if (KEY_H) data|=8;
		if (KEY_SHIFT) data|=0x10;
		if (KEY_Y) data|=0x20;
		if (KEY_N) data|=0x40;
		if (KEY_UP) data|=0x80;
	}
	if (lh5811.data[0xc]&2) {
		if (KEY_POINT) data|=1;
		if (KEY_MINUS) data|=2;
		if (KEY_OFF) data|=4;
		if (KEY_S) data|=8;
		if (KEY_CALLSIGN) data|=0x10;
		if (KEY_W) data|=0x20;
		if (KEY_X) data|=0x40;
		if (KEY_RESERVE) data|=0x80;
	}
	if (lh5811.data[0xc]&4) {
		if (KEY_1) data|=1;
		if (KEY_4) data|=2;
		if (KEY_7) data|=4;
		if (KEY_J) data|=8;
		if (KEY_PERCENT) data|=0x10;
		if (KEY_U) data|=0x20;
		if (KEY_M) data|=0x40;
		if (KEY_0) data|=0x80;
	}
	if (lh5811.data[0xc]&8) {
		if (KEY_CLOSEBRACE) data|=1;
		if (KEY_L) data|=2;
		if (KEY_O) data|=4;
		if (KEY_K) data|=8;
		if (KEY_AMBERSAND) data|=0x10;
		if (KEY_I) data|=0x20;
		if (KEY_OPENBRACE) data|=0x40;
		if (KEY_ENTER) data|=0x80;
	}
	if (lh5811.data[0xc]&0x10) {
		if (KEY_PLUS) data|=1;
		if (KEY_ASTERIX) data|=2;
		if (KEY_SLASH) data|=4;
		if (KEY_D) data|=8;
		if (KEY_QUOTE) data|=0x10;
		if (KEY_E) data|=0x20;
		if (KEY_C) data|=0x40;
		if (KEY_RCL) data|=0x80;
	}
	if (lh5811.data[0xc]&0x20) {
		if (KEY_MINUS) data|=1;
		if (KEY_LEFT) data|=2;
		if (KEY_P) data|=4;
		if (KEY_F) data|=8;
		if (KEY_NUMBER) data|=0x10;
		if (KEY_R) data|=0x20;
		if (KEY_V) data|=0x40;
		if (KEY_SPACE) data|=0x80;
	}
	if (lh5811.data[0xc]&0x40) {
		if (KEY_RIGHT) data|=1;
		if (KEY_MODE) data|=2;
		if (KEY_CL) data|=4;
		if (KEY_A) data|=8;
		if (KEY_DEF) data|=0x10;
		if (KEY_Q) data|=0x20;
		if (KEY_Z) data|=0x40;
		if (KEY_SML) data|=0x80;
	}
	if (lh5811.data[0xc]&0x80) {
		if (KEY_3) data|=1;
		if (KEY_6) data|=2;
		if (KEY_9) data|=4;
		if (KEY_G) data|=8;
		if (KEY_STRING) data|=0x10;
		if (KEY_T) data|=0x20;
		if (KEY_B) data|=0x40;
		if (KEY_DOWN) data|=0x80;
	}
	return data^0xff;
}
