/*********************************************************************

	mc146818.c

	Implementation of the MC146818 chip

	Real time clock chip with batteru buffered ram
	Used in IBM PC/AT, several PC clones, Amstrad NC200

	Peter Trauner (peter.trauner@jk.uni-linz.ac.at)

*********************************************************************/

#include "includes/mc146818.h"
#include "mscommon.h"


static struct
{
	MC146818_TYPE type;

	UINT8 index;
	UINT8 data[0x40];

	UINT16 eindex;
	UINT8 edata[0x2000];

	double last_refresh;
} mc146818;

#define HOURS_24 (mc146818.data[0xb]&2)
#define BCD_MODE !(mc146818.data[0xb]&4) // book has other description!
#define CENTURY mc146818.data[50]
#define YEAR mc146818.data[9]
#define MONTH mc146818.data[8]
#define DAY mc146818.data[7]
#define WEEK_DAY mc146818.data[6]

static void mc146818_timer(int param)
{
	int year, month;

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
					WEEK_DAY=bcd_adjust(WEEK_DAY+1)%7;
					DAY=bcd_adjust(DAY+1);
					month=bcd_2_dec(MONTH);
					year=bcd_2_dec(YEAR);
					if (mc146818.type!=MC146818_IGNORE_CENTURY) year+=bcd_2_dec(CENTURY)*100;
					else year+=2000; // save for julian_days_in_month calculation
					DAY=bcd_adjust(DAY+1);
					if (DAY>gregorian_days_in_month(MONTH, year)) {
						DAY=1;
						MONTH=bcd_adjust(MONTH+1);
						if (MONTH>0x12) {
							MONTH=1;
							YEAR=year=bcd_adjust(YEAR+1);
							if (mc146818.type!=MC146818_IGNORE_CENTURY) {
								if (year>=0x100) { 
									CENTURY=bcd_adjust(CENTURY+1);
								}
							}
						}
					}
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
				// different handling of hours //?
				if (mc146818.data[4]>=24) {
					mc146818.data[4]=0;
					WEEK_DAY=(WEEK_DAY+1)%7;
					year=YEAR;
					if (mc146818.type!=MC146818_IGNORE_CENTURY) year+=CENTURY*100;
					else year+=2000; // save for julian_days_in_month calculation
					if (++DAY>gregorian_days_in_month(MONTH, year)) {
						DAY=1;
						if (++MONTH>12) {
							MONTH=1;
							YEAR++;
							if (mc146818.type!=MC146818_IGNORE_CENTURY) {
								if (YEAR>=100) { CENTURY++;YEAR=0; }
							} else {
								YEAR%=100;
							}
                       }
					}
				}
			}
		}
	}

	mc146818.last_refresh = timer_get_time();	
}



void mc146818_init(MC146818_TYPE type)
{
	memset(&mc146818, 0, sizeof(mc146818));
	mc146818.type = type;
	mc146818.last_refresh = timer_get_time();
    timer_pulse(TIME_IN_HZ(1.0), 0, mc146818_timer);
}



void mc146818_load(void)
{
	mame_file *file;

	file = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_NVRAM, 0);
	if (file)
	{
		mc146818_load_stream(file);
		mame_fclose(file);
	}
}



void mc146818_load_stream(mame_file *file)
{
	mame_fread(file, mc146818.data, sizeof(mc146818.data));
}
 


void mc146818_set_gmtime(struct tm *tmtime)
{
	if (BCD_MODE) {
		mc146818.data[0]=dec_2_bcd(tmtime->tm_sec);
		mc146818.data[2]=dec_2_bcd(tmtime->tm_min);
		if (HOURS_24||(tmtime->tm_hour<12))
			mc146818.data[4]=dec_2_bcd(tmtime->tm_hour);
		else
			mc146818.data[4]=dec_2_bcd(tmtime->tm_hour-12)|0x80;
		
		DAY=dec_2_bcd(tmtime->tm_mday);
		MONTH=dec_2_bcd(tmtime->tm_mon+1);
		YEAR=dec_2_bcd(tmtime->tm_year%100);
		
		if (mc146818.type!=MC146818_IGNORE_CENTURY)
			CENTURY=dec_2_bcd((tmtime->tm_year+1900)/100);
	} else {
		mc146818.data[0]=tmtime->tm_sec;
		mc146818.data[2]=tmtime->tm_min;
		if (HOURS_24||(tmtime->tm_hour<12))
			mc146818.data[4]=tmtime->tm_hour;
		else
			mc146818.data[4]=(tmtime->tm_hour-12)|0x80;
		
		DAY=tmtime->tm_mday;
		MONTH=tmtime->tm_mon+1;
		YEAR=tmtime->tm_year%100;
		if (mc146818.type!=MC146818_IGNORE_CENTURY)
			CENTURY=(tmtime->tm_year+1900)/100;
	}
	WEEK_DAY=tmtime->tm_wday;
	if (tmtime->tm_isdst) mc146818.data[0xb]|=1;
	else mc146818.data[0xb]&=~1;
}



void mc146818_set_time(void)
{
	time_t t;
	struct tm *tmtime;

	t=time(NULL);
	if (t==-1) return;

	tmtime=gmtime(&t);

	mc146818_set_gmtime(tmtime);
	// freeing of gmtime??
}



void mc146818_save(void)
{
	mame_file *file;
	
	file = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_NVRAM, 1);
	if (file)
	{
		mame_fwrite(file, mc146818.data, sizeof(mc146818.data));
		mame_fclose(file);
	}
}



void mc146818_save_stream(mame_file *file)
{
	mame_fwrite(file, mc146818.data, sizeof(mc146818.data));
}



NVRAM_HANDLER( mc146818 )
{
	if (file==NULL) {
		mc146818_set_time();
		// init only 
	} else if (read_or_write) {
		mc146818_save_stream(file);
	} else {
		mc146818_load_stream(file);
	}
}



READ8_HANDLER(mc146818_port_r)
{
	data8_t data = 0;
	switch (offset) {
	case 0:
		data = mc146818.index;
		break;

	case 1:
		switch (mc146818.index&0x3f) {
		case 0xa:
			data=mc146818.data[mc146818.index&0x3f];
			if (timer_get_time()-mc146818.last_refresh<TIME_IN_SEC(1.0/32768.0f)) data|=0x80;
#if 0
			/* for pc1512 bios realtime clock test */
			mc146818.data[mc146818.index&0x3f]^=0x80; /* 0x80 update in progress */
#endif
			break;

		case 0xd: 
			data = mc146818.data[mc146818.index&0x3f]|0x80; /* battery ok */
			break;

		default:
			data = mc146818.data[mc146818.index&0x3f];
			break;
		}
		break;
	}
	return data;
}



WRITE8_HANDLER(mc146818_port_w)
{
	switch (offset) {
	case 0:
		mc146818.index=data;
		break;

	case 1:
		switch (mc146818.index & 0x3f) {
		default:
			mc146818.data[mc146818.index&0x3f]=data;
			break;
		}
		break;
	}
}



READ32_HANDLER(mc146818_port32_r)
{
	return ((data32_t) mc146818_port_r(0)) << 0 
		|  ((data32_t) mc146818_port_r(1)) << 8; 
}



WRITE32_HANDLER(mc146818_port32_w)
{
	if ((mem_mask & 0x000000FF) == 0)
		mc146818_port_w(offset * 4 + 0, data >> 0);
	if ((mem_mask & 0x0000FF00) == 0)
		mc146818_port_w(offset * 4 + 1, data >> 8);
}




