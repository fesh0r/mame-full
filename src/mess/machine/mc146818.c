/* real time clock chip with batterie buffered ram
   used in ibm pc/at */

#include "driver.h"

#include "mc146818.h"

#if 0
#define DBG_LOG(level, text, print) \
		if (level>0) { \
				logerror("%s\t", text); \
				logerror print; \
		} 
#else
#define DBG_LOG(level, text, print) 
#endif

static struct {
	MC146818_TYPE type;
	UINT8 index;
	UINT8 data[0x40];
	UINT16 eindex;
	UINT8 edata[0x2000];
	void *timer;
} mc146818= {0};


static int bcd_adjust(int value)
{
	if ((value&0xf)>=0xa) value=value+0x10-0xa;
	if ((value&0xf0)>=0xa0) value=value-0xa0+0x100;
	return value;
}

static void mc146818_timer(int param)
{
	mc146818.data[0]=bcd_adjust(mc146818.data[0]++);
	if (mc146818.data[0]>=0x60) {
		mc146818.data[0]=0;
		mc146818.data[2]=bcd_adjust(mc146818.data[2]++);
		if (mc146818.data[2]>=0x60) {
			mc146818.data[2]=0;
			mc146818.data[4]=bcd_adjust(mc146818.data[4]++);
			if (mc146818.data[4]>=0x24) {
				mc146818.data[4]=0;
				mc146818.data[6]=bcd_adjust(mc146818.data[6]++);
				if (mc146818.data[6]>=7) {
					mc146818.data[6]=0;
				}
			}
		}
	}
}

void mc146818_init(MC146818_TYPE type)
{
	FILE *file;

	memset(&mc146818, 0, sizeof(mc146818));
	mc146818.type=type;

	mc146818.timer=timer_pulse(1.0,0,mc146818_timer);
	if ( (file=osd_fopen(Machine->gamedrv->name, 
						 Machine->gamedrv->name, OSD_FILETYPE_NVRAM, 0))==NULL)
		return;
	osd_fread(file,mc146818.data, sizeof(mc146818.data));
	mc146818.data[0xf]=0;
	osd_fclose(file);	
}

void mc146818_close(void)
{
	FILE *file;
	if ( (file=osd_fopen(Machine->gamedrv->name, 
						 Machine->gamedrv->name, OSD_FILETYPE_NVRAM, 1))==NULL)
		return;
	osd_fwrite(file, mc146818.data, sizeof(mc146818.data));
	osd_fclose(file);
//	timer_remove(mc146818.timer);
}

READ_HANDLER(mc146818_port_r)
{
	int data=0;
	switch (offset) {
	case 0:
		data=mc146818.index;
		break;
	case 1:
		switch (mc146818.index&0x3f) {
		case 0xd: 
			data=mc146818.data[mc146818.index&0x3f]|0x80;
			break;
		default:
			data=mc146818.data[mc146818.index&0x3f];
			break;
		}
		break;
	}
	DBG_LOG(1,"mc146818",("read %.2x %.2x\n",offset,data));
	return data;
}

WRITE_HANDLER(mc146818_port_w)
{
	DBG_LOG(1,"mc146818",("write %.2x %.2x\n",offset,data));
	switch (offset) {
	case 0:
		mc146818.index=data;
		break;
	case 1:
		switch (mc146818.index&0x3f) {
		default:
			mc146818.data[mc146818.index&0x3f]=data;
			break;
		}
		break;
	}
}
