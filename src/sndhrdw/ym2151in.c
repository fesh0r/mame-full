/***************************************************************************

  ym2151in.c

  That stands for YM 2151 interface.
  Many games use the YM 2151 to produce sound; the functions contained in
  this file make it easier to interface with it.

***************************************************************************/

#include "driver.h"
#include "sndhrdw/ym2151.h"
#include "sndhrdw/ym2151in.h"

static int emulation_rate;
static int buffer_len;

#define MIN_SLICE 44

static struct YM2151interface2 *intf;
static unsigned char *buffer[MAX_YM2151];
static int buffer_pos[MAX_YM2151];

static int channel;

int YM2151_1_sh_start(struct YM2151interface2 *interface)
{
	int i;
	int rate=Machine->sample_rate;

	intf = interface;

	buffer_len = rate / Machine->drv->frames_per_second;
	emulation_rate = buffer_len * Machine->drv->frames_per_second;

	for (i = 0;i < MAX_YM2151;i++)
	{
		buffer[i] = 0;
		buffer_pos[i]=0;
	}
	for (i = 0;i < intf->num;i++)
	{
		if ((buffer[i] = malloc( sizeof(SAMPLE)*buffer_len)) == 0)
		{
			while (--i >= 0) free(buffer[i]);
			return 1;
		}
		memset(buffer[i],0, buffer_len);
	}

	if (YMInit(intf->num,emulation_rate,buffer_len,buffer) == 0)
	{
		channel=get_play_channels(intf->num);
		for (i = 0; i < intf->num; i++)
			YMSetIrqHandler(i,intf->handler[i]);
		return 0;
	}
	else return 1;
}



void YM2151_1_sh_stop(void)
{
	int i;

	YMShutdown();
	for (i = 0;i < intf->num;i++) free(buffer[i]);
}


static inline void update_ym2151(int num)
{
	int newpos;


	newpos = cpu_scalebyfcount(buffer_len);	/* get current position based on the timer */

	if( newpos - buffer_pos[num] < MIN_SLICE ) return;
	YM2151UpdateOne(num , newpos );
	buffer_pos[num] = newpos;
}

static int lastreg0,lastreg1,lastreg2;	/* YM2151 register currently selected */

int YM2151_read_port_0_r(int offset)
{
	update_ym2151(0);
	return YMReadReg(0);
}
int YM2151_read_port_1_r(int offset)
{
	update_ym2151(1);
	return YMReadReg(1);
}
int YM2151_read_port_2_r(int offset)
{
	update_ym2151(2);
	return YMReadReg(2);
}



void YM2151_control_port_0_w(int offset,int data)
{
	lastreg0 = data;
}
void YM2151_control_port_1_w(int offset,int data)
{
	lastreg1 = data;
}
void YM2151_control_port_2_w(int offset,int data)
{
	lastreg2 = data;
}



void YM2151_write_port_0_w(int offset,int data)
{
	update_ym2151(0);
	YMWriteReg(0,lastreg0,data);
}
void YM2151_write_port_1_w(int offset,int data)
{
	update_ym2151(1);
	YMWriteReg(1,lastreg1,data);
}
void YM2151_write_port_2_w(int offset,int data)
{
	update_ym2151(2);
	YMWriteReg(2,lastreg2,data);
}


void YM2151_1_sh_update(void)
{
	int i;

	if (Machine->sample_rate == 0) return;

	YM2151Update();

	for (i = 0;i < intf->num;i++)
		buffer_pos[i]=0;

	for (i = 0;i < intf->num;i++)
		osd_play_streamed_sample(channel+i,buffer[i],buffer_len,emulation_rate,intf->volume[i]);
}


