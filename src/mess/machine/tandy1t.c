#include "driver.h"

#include "pc.h"
#include "tandy1t.h"


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

void tandy1000_init(void)
{
	FILE *file;
	
	if ( (file=osd_fopen(Machine->gamedrv->name, 
						 Machine->gamedrv->name, OSD_FILETYPE_NVRAM, 0))==NULL)
		return;
	osd_fread(file, eeprom.ee, sizeof(eeprom.ee));

	osd_fclose(file); 	
}

void tandy1000_close(void)
{
	FILE *file;
	if ( (file=osd_fopen(Machine->gamedrv->name, 
						 Machine->gamedrv->name, OSD_FILETYPE_NVRAM, 1))==NULL)
		return;
	osd_fwrite(file, eeprom.ee, sizeof(eeprom.ee));
	osd_fclose(file);
}

int tandy1000_read_eeprom(void)
{
	return eeprom.data&0x8000;
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
				//logerror("!!!tandy1000 eeprom %.2x %.2x\n",eeprom.state, data);
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
//					logerror("!!!tandy1000 eeprom %.2x\n",eeprom.oper);
					if ((eeprom.oper&0xc0)==0x80) {
						eeprom.state=100;
						eeprom.data=eeprom.ee[eeprom.oper&0x3f].low
							|(eeprom.ee[eeprom.oper&0x3f].high<<8);
//						logerror("!!!tandy1000 eeprom read %.2x,%.4x\n",eeprom.oper,eeprom.data);
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
//					logerror("tandy1000 %.2x %.4x written\n",eeprom.oper,eeprom.data);
					eeprom.ee[eeprom.oper&0x3f].low=eeprom.data&0xff;
					eeprom.ee[eeprom.oper&0x3f].high=eeprom.data>>8;
					tandy1000_close();
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

