/* real time clock chip with batterie buffered ram
   used in ibm pc/at */
#include <time.h>

#include "driver.h"

#include "includes/mc146818.h"

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

	double last_refresh;
	void *timer;
} mc146818= {0};

#define HOURS_24 mc146818.data[0xb]&2
#define BCD_MODE !(mc146818.data[0xb]&4) // book has other description!

static int bcd_adjust(int value)
{
	if ((value&0xf)>=0xa) value=value+0x10-0xa;
	if ((value&0xf0)>=0xa0) value=value-0xa0+0x100;
	return value;
}

static int dec_2_bcd(int a)
{
	return (a%10)|((a/10)<<4);
}

static void mc146818_from_gmtime(struct tm *tmtime)
{
	if (BCD_MODE) {
		mc146818.data[0]=dec_2_bcd(tmtime->tm_sec);
		mc146818.data[2]=dec_2_bcd(tmtime->tm_min);
		if ((mc146818.data[0xb]&2)||(tmtime->tm_hour<12))
			mc146818.data[4]=dec_2_bcd(tmtime->tm_hour);
		else
			mc146818.data[4]=dec_2_bcd(tmtime->tm_hour-12)|0x80;
		
		mc146818.data[7]=dec_2_bcd(tmtime->tm_mday);
		mc146818.data[8]=dec_2_bcd(tmtime->tm_mon+1);
		mc146818.data[9]=dec_2_bcd(tmtime->tm_year%100);
		
		if (mc146818.type!=MC146818_IGNORE_CENTURY)
			mc146818.data[50]=dec_2_bcd((tmtime->tm_year+1900)/100);
	} else {
		mc146818.data[0]=tmtime->tm_sec;
		mc146818.data[2]=tmtime->tm_min;
		if ((mc146818.data[0xb]&2)||(tmtime->tm_hour<12))
			mc146818.data[4]=tmtime->tm_hour;
		else
			mc146818.data[4]=(tmtime->tm_hour-12)|0x80;
		
		mc146818.data[7]=tmtime->tm_mday;
		mc146818.data[8]=tmtime->tm_mon+1;
		mc146818.data[9]=tmtime->tm_year%100;
		if (mc146818.type!=MC146818_IGNORE_CENTURY)
			mc146818.data[50]=(tmtime->tm_year+1900)/100;
	}
	mc146818.data[6]=tmtime->tm_wday;
	if (tmtime->tm_isdst) mc146818.data[0xb]|=1;
	else mc146818.data[0xb]&=~1;
}

static void mc146818_timer(int param)
{
#if 0
	// this is a way to avoid doing date calculation myself
	struct tm tmtime, *tp;
	time_t t;

	mc146818_to_gmtime(&tmtime);
	t=mktime(&tmtime)+1;
	tp=gmtime(&t);
	mc146818_from_gmtime(tp);
#else
	if (BCD_MODE) {
		mc146818.data[0]=bcd_adjust(mc146818.data[0]+1);
		if (mc146818.data[0]>=0x60) {
			mc146818.data[0]=0;
			mc146818.data[2]=bcd_adjust(mc146818.data[2]+1);
			if (mc146818.data[2]>=0x60) {
				mc146818.data[2]=0;
				mc146818.data[4]=bcd_adjust(mc146818.data[4]+1);
				// different handling of hours
				if (mc146818.data[4]>=0x24) {
					mc146818.data[4]=0;
					mc146818.data[6]=bcd_adjust(mc146818.data[6]+1);
					if (mc146818.data[6]>=7) {
						mc146818.data[6]=0;
					}
					mc146818.data[7]=bcd_adjust(mc146818.data[7]+1);
					// day in month overrun
					// month overrun
				}
			}
		}
	} else {
		mc146818.data[0]=mc146818.data[0]+1;
		if (mc146818.data[0]>=60) {
			mc146818.data[0]=0;
			mc146818.data[2]=mc146818.data[2]+1;
			if (mc146818.data[2]>=60) {
				mc146818.data[2]=0;
				mc146818.data[4]=mc146818.data[4]+1;
				// different handling of hours
				if (mc146818.data[4]>=24) {
					mc146818.data[4]=0;
					mc146818.data[6]=mc146818.data[6]+1;
					if (mc146818.data[6]>=7) {
						mc146818.data[6]=0;
					}
					// day in month overrun
					// month overrun
				}
			}
		}
	}
#endif
	mc146818.last_refresh=timer_get_time();	
}

void mc146818_init(MC146818_TYPE type)
{
	FILE *file;
	time_t t;
	struct tm *tmtime;

	memset(&mc146818, 0, sizeof(mc146818));
	mc146818.type=type;
	mc146818.last_refresh=timer_get_time();

	mc146818.timer=timer_pulse(1.0,0,mc146818_timer);
	if ( (file=osd_fopen(Machine->gamedrv->name, 
						 Machine->gamedrv->name, OSD_FILETYPE_NVRAM, 0))==NULL)
		return;
	osd_fread(file,mc146818.data, sizeof(mc146818.data));
	osd_fclose(file);	

	t=time(NULL);
	if (t==-1) return;

	tmtime=gmtime(&t);

	mc146818_from_gmtime(tmtime);
	// freeing of gmtime??
}

void mc146818_close(void)
{
	FILE *file;
	if ( (file=osd_fopen(Machine->gamedrv->name, 
						 Machine->gamedrv->name, OSD_FILETYPE_NVRAM, 1))==NULL)
		return;
	osd_fwrite(file, mc146818.data, sizeof(mc146818.data));
	osd_fclose(file);
/*	timer_remove(mc146818.timer); */
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
		case 0xa:
			data=mc146818.data[mc146818.index&0x3f];
			if (timer_get_time()-mc146818.last_refresh<1.0/32768) data|=0x80;
#if 0
			/* for pc1512 bios realtime clock test */
			mc146818.data[mc146818.index&0x3f]^=0x80; /* 0x80 update in progress */
#endif
			break;
		case 0xd: 
			data=mc146818.data[mc146818.index&0x3f]|0x80; /* batterie ok */
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
