#include "driver.h"

#include "includes/pckeybrd.h"

#include "includes/pcshare.h"
#include "includes/tandy1t.h"
//#include "includes/pc_t1t.h"


/* tandy 1000 eeprom
  hx and later
  clock, and data out lines at 0x37c
  data in at 0x62 bit 4 (0x10)

  8x read 16 bit word at x
  30 cx 30 4x 16bit 00 write 16bit at x
*/
static struct {
	int state;
	int clock;
	UINT8 oper;
	UINT16 data;
	struct {
		UINT8 low, high;
	} ee[0x40]; /* only 0 to 4 used in hx, addressing seems to allow this */
} eeprom={0};

NVRAM_HANDLER( tandy1000 )
{
	if (file==NULL) {
		// init only 
	} else if (read_or_write) {
		mame_fwrite(file, eeprom.ee, sizeof(eeprom.ee));
	} else {
		mame_fread(file, eeprom.ee, sizeof(eeprom.ee));
	}
}

static int tandy1000_read_eeprom(void)
{
	if ((eeprom.state>=100)&&(eeprom.state<=199))
		return eeprom.data&0x8000;
	return 1;
}

static void tandy1000_write_eeprom(UINT8 data)
{
	if (!eeprom.clock && (data&4) ) {
//				logerror("!!!tandy1000 eeprom %.2x %.2x\n",eeprom.state, data);
		switch (eeprom.state) {
		case 0:
			if ((data&3)==0) eeprom.state++;
			break;
		case 1:
			if ((data&3)==2) eeprom.state++;
			break;
		case 2:
			if ((data&3)==3) eeprom.state++;
			break;
		case 3:
			eeprom.oper=data&1;
			eeprom.state++;
			break;
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
			eeprom.oper=(eeprom.oper<<1)|(data&1);
			eeprom.state++;
			break;
		case 10:
			eeprom.oper=(eeprom.oper<<1)|(data&1);
			logerror("!!!tandy1000 eeprom %.2x\n",eeprom.oper);
			if ((eeprom.oper&0xc0)==0x80) {
				eeprom.state=100;
				eeprom.data=eeprom.ee[eeprom.oper&0x3f].low
					|(eeprom.ee[eeprom.oper&0x3f].high<<8);
				logerror("!!!tandy1000 eeprom read %.2x,%.4x\n",eeprom.oper,eeprom.data);
			} else if ((eeprom.oper&0xc0)==0x40) {
				eeprom.state=200;
			} else eeprom.state=0;
			break;
			
			/* read 16 bit */
		case 100:
			eeprom.state++;
			break;
		case 101:
		case 102:
		case 103:
		case 104:
		case 105:
		case 106:
		case 107:
		case 108:
		case 109:
		case 110:
		case 111:
		case 112:
		case 113:
		case 114:
		case 115:
			eeprom.data<<=1;
			eeprom.state++;
			break;
		case 116:
			eeprom.data<<=1;
			eeprom.state=0;
			break;
			
			/* write 16 bit */
		case 200:
		case 201:
		case 202:
		case 203:
		case 204:
		case 205:
		case 206:
		case 207:
		case 208:
		case 209:
		case 210:
		case 211:
		case 212:
		case 213:
		case 214:
			eeprom.data=(eeprom.data<<1)|(data&1);
			eeprom.state++;
			break;
		case 215:
			eeprom.data=(eeprom.data<<1)|(data&1);
			logerror("tandy1000 %.2x %.4x written\n",eeprom.oper,eeprom.data);
			eeprom.ee[eeprom.oper&0x3f].low=eeprom.data&0xff;
			eeprom.ee[eeprom.oper&0x3f].high=eeprom.data>>8;
			eeprom.state=0;
			break;
		}
	}
	eeprom.clock=data&4;
}

static struct {
	UINT8 data[8];
} tandy={ {0}};

WRITE8_HANDLER ( pc_t1t_p37x_w )
{
//	DBG_LOG(2,"T1T_p37x_w",("%.5x #%d $%02x\n", activecpu_get_pc(),offset, data));
	if (offset!=4)
		logerror("T1T_p37x_w %.5x #%d $%02x\n", activecpu_get_pc(),offset, data);
	tandy.data[offset]=data;
	switch( offset )
	{
		case 4:
			tandy1000_write_eeprom(data);
			break;
	}
}

 READ8_HANDLER ( pc_t1t_p37x_r )
{
	int data = tandy.data[offset];
//	DBG_LOG(1,"T1T_p37x_r",("%.5x #%d $%02x\n", activecpu_get_pc(), offset, data));
    return data;
}

/* this is for tandy1000hx
   hopefully this works for all x models
   must not be a ppi8255 chip
   (think custom chip)
   port c:
   bit 4 input eeprom data in
   bit 3 output turbo mode
*/
static struct {
	UINT8 portb, portc;
} tandy_ppi= { 0 };

WRITE8_HANDLER ( tandy1000_pio_w )
{
	switch (offset) {
	case 1:
		tandy_ppi.portb = data;
		pc_sh_speaker(data&3);
		pc_keyb_set_clock(data&0x40);
		break;
	case 2:
		tandy_ppi.portc = data;
		if (data&8) cpunum_set_clockscale(0, 1);
		else cpunum_set_clockscale(0, 4.77/8);
		break;
	}
}

 READ8_HANDLER(tandy1000_pio_r)
{
	int data=0xff;
	switch (offset) {
	case 0:
		data = pc_keyb_read();
		break;
	case 1:
		data=tandy_ppi.portb;
		break;
	case 2:
//		if (tandy1000hx) {
//		data=tandy_ppi.portc; // causes problems (setuphx)
		if (!tandy1000_read_eeprom()) data&=~0x10;
//	}
		break;
	}
	return data;
}

