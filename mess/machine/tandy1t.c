#include "driver.h"

#include "includes/pckeybrd.h"

#include "includes/pc.h"
#include "includes/tandy1t.h"
#include "includes/pc_t1t.h"

extern void init_t1000hx(void)
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x1000];
	int i;
    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;
	pc_init_setup(pc_setup_t1000hx);
	init_pc_common();
	at_keyboard_set_type(AT_KEYBOARD_TYPE_PC);
}

void pc_t1t_init_machine(void)
{
	pc_keyboard_init();
}

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

void tandy1000_nvram_handler(void* file, int write)
{
	if (file==NULL) {
		// init only 
	} else if (write) {
		osd_fwrite(file, eeprom.ee, sizeof(eeprom.ee));
	} else {
		osd_fread(file, eeprom.ee, sizeof(eeprom.ee));
	}
}

static int tandy1000_read_eeprom(void)
{
	if ((eeprom.state>=100)&&(eeprom.state<=199))
		return eeprom.data&0x8000;
	return 1;
}

WRITE_HANDLER ( pc_t1t_p37x_w )
{
//	DBG_LOG(2,"T1T_p37x_w",("%.5x #%d $%02x\n", cpu_get_pc(),offset, data));
	if (offset!=4)
		logerror("T1T_p37x_w %.5x #%d $%02x\n", cpu_get_pc(),offset, data);
	switch( offset )
	{
		case 0: pc_port[0x378] = data; break;
		case 1: pc_port[0x379] = data; break;
		case 2: pc_port[0x37a] = data; break;
		case 3: pc_port[0x37b] = data; break;
		case 4:
			pc_port[0x37c] = data;
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
//					tandy1000_close();
					eeprom.state=0;
					break;
				}
			}
			eeprom.clock=data&4;
			break;
		case 5: pc_port[0x37d] = data; break;
		case 6: pc_port[0x37e] = data; break;
		case 7: pc_port[0x37f] = data; break;
	}
}

READ_HANDLER ( pc_t1t_p37x_r )
{
	int data = 0xff;
	switch( offset )
	{
		case 0: data = pc_port[0x378]; break;
		case 1: data = pc_port[0x379]; break;
		case 2: data = pc_port[0x37a]; break;
		case 3: data = pc_port[0x37b]; break;
		case 4: data = pc_port[0x37c]; break;
		case 5: data = pc_port[0x37d]; break;
		case 6: data = pc_port[0x37e]; break;
		case 7: data = pc_port[0x37f]; break;
	}
//	DBG_LOG(1,"T1T_p37x_r",("%.5x #%d $%02x\n", cpu_get_pc(), offset, data));
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
WRITE_HANDLER ( tandy1000_pio_w )
{
	switch (offset) {
	case 1:
#if 0
	/* KB controller port B */
	PIO_LOG(1,"PIO_B_w",("$%02x\n", data));
	pc_port[0x61] = data;
	switch( data & 3 )
	{
		case 0: pc_sh_speaker(0); break;
		case 1: pc_sh_speaker(1); break;
		case 2: pc_sh_speaker(1); break;
		case 3: pc_sh_speaker(2); break;
	}
#endif
		pc_ppi_portb_w(0,data);
		break;
	case 2:
//		PIO_LOG(1,"PIO_C_w",("$%02x\n", data));
		pc_port[0x62] = data;
//		if (tandy1000hx) {
			if (data&8) timer_set_overclock(0, 1);
			else timer_set_overclock(0, 4.77/8);
//		}
		break;
	}
}

READ_HANDLER(tandy1000_pio_r)
{
	int data=0xff;
	switch (offset) {
	case 0:
		data=pc_ppi_porta_r(0);
		break;
	case 1:
		data=pc_port[0x61];
		break;
	case 2:
//		if (tandy1000hx) {
//		data=pc_port[0x62]; // causes problems (setuphx)
		if (!tandy1000_read_eeprom()) data&=~0x10;
//	}
		break;
	}
	return data;
}

int tandy1000_frame_interrupt (void)
{
	static int turboswitch=-1;

	if (turboswitch !=(input_port_3_r(0)&2)) {
		if (input_port_3_r(0)&2)
			timer_set_overclock(0, 1);
		else
			timer_set_overclock(0, 4.77/12);
		turboswitch=input_port_3_r(0)&2;
	}

	pc_t1t_timer();

    if( !onscrd_active() && !setup_active() )
		pc_keyboard();

    return ignore_interrupt ();
}
