/* interface between MAME and nes_apu.c */
/*	 Copyright (C) 2000 Matthew Conte	*/
/* This code may be freely distributed! */

#include "driver.h"
#include "nesintf.h"
#include "nes_apu.h"

static apu_t apu[MAX_NESPSG];
static int channel[MAX_NESPSG] = {-1,-1};
static const struct NESinterface *intf = NULL;

READ_HANDLER(NESPSG_0_r)
{
	apu_setcontext(&apu[0]);
	return apu_read(0x4000 + offset);
}

READ_HANDLER(NESPSG_1_r)
{
	apu_setcontext(&apu[1]);
	return apu_read(0x4000 + offset);
}


WRITE_HANDLER(NESPSG_0_w)
{
	if (offset == 0x14) /* OAM DMA */
	{
		if (intf->apu_callback_w[0])
			(intf->apu_callback_w[0])(0, data);
		else
			logerror ("NES apu reg %d write uncaught, data: %02x\n", offset, data);
	}
	else
	{
		if (channel[0] != -1)
			stream_update(channel[0], 0);
		apu_setcontext(&apu[0]);
		apu_write(0x4000 + offset, data);
		apu_getcontext(&apu[0]);
	}
}

WRITE_HANDLER(NESPSG_1_w)
{
	if (offset == 0x14) /* OAM DMA */
	{
		if (intf->apu_callback_w[1])
			(intf->apu_callback_w[1])(0, data);
		else
			logerror ("NES apu reg %d write uncaught, data: %02x\n", offset, data);
	}
	else
	{
		if (channel[1] != -1)
			stream_update(channel[1], 0);
		apu_setcontext(&apu[1]);
		apu_write(0x4000 + offset, data);
		apu_getcontext(&apu[1]);
	}
}

static void process(int param, INT16 *buffer, int length)
{
	apu_setcontext(&apu[param]);
	apu_process((void *) buffer, length);
	apu_getcontext(&apu[param]);
}

int NESPSG_sh_start(const struct MachineSound *msound)
{
	int i;
	char buf[80];

	for (i = 0; i < MAX_NESPSG; i++)
	{
		channel[i] = -1;
		memset(&apu[i], 0, sizeof(apu_t));
	}

	if (Machine->sample_rate == 0)
		return 0;

	intf = msound->sound_interface;

	for (i = 0; i < intf->num; i++)
	{
		apu_t *temp_apu;

		temp_apu = apu_create(intf->cpunum[i], intf->baseclock, Machine->sample_rate, Machine->drv->frames_per_second, 16);
		if (NULL == temp_apu)
			return 1;

		apu_setcontext(temp_apu);
		apu_reset();
		apu_getcontext(&apu[i]);

		sprintf(buf, "NES APU %d", i);
		channel[i] = stream_init(buf, intf->volume[i], Machine->sample_rate, i, process);

		if (-1 == channel[i])
			return 1;
	}

	return 0;
}

void NESPSG_sh_stop(void)
{
	int i;

	for (i = 0; i < intf->num; i++)
		apu_destroy(&apu[i]);
}

void NESPSG_sh_update(void)
{
	int i;

	if (!intf)
		return;

	for (i = 0; i < intf->num; i++)
		stream_update(channel[i], 0);
}

