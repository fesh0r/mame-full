/* interface between MAME and nes_apu.c */
/*   Copyright (C) 2000 Matthew Conte   */
/* This code may be freely distributed! */

#include "driver.h"
#include "nesintf.h"
#include "nes_apu2.h"

static apu_t *apu[MAX_NESPSG];
static int channel[MAX_NESPSG];
static const struct NESinterface *intf = NULL;

READ_HANDLER(NESPSG_0_r)
{
	apu_setcontext(apu[0]);
	return apu_read(0x4000 + offset);
}

READ_HANDLER(NESPSG_1_r)
{
	apu_setcontext(apu[1]);
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
		stream_update(channel[0], 0);
		apu_setcontext(apu[0]);
		apu_write(0x4000 + offset, data);
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
		stream_update(channel[1], 0);
		apu_setcontext(apu[1]);
		apu_write(0x4000 + offset, data);
	}
}

static void process(int param, INT16 *buffer, int length)
{
	apu_setcontext(apu[param]);
	apu_process((void *) buffer, length);
}

int NESPSG_sh_start(const struct MachineSound *msound)
{
	int i;
	char buf[80];

	for (i = 0; i < MAX_NESPSG; i++)
	{
		channel[i] = -1;
		apu[i] = NULL;
	}

	intf = msound->sound_interface;

	for (i = 0; i < intf->num; i++)
	{
		int sample_rate;

		sample_rate = Machine->sample_rate;

		/* hack it out as 60Hz for now - bleh! */
		/* NPW 11-Oct-2001 - special case for when sample_rate is zero */
		apu[i] = apu_create(intf->cpunum[i], intf->basefreq, sample_rate ? sample_rate : 44100, Machine->drv->frames_per_second, 16);
		if (NULL == apu[i])
			return 1;

		apu_setcontext(apu[i]);
		apu_reset();

		if (sample_rate != 0)
		{
			sprintf(buf, "NES APU %d", i);
			channel[i] = stream_init(buf, intf->volume[i], Machine->sample_rate,
						i, process);

			if (-1 == channel[i])
				return 1;
		}
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

	for (i = 0; i < intf->num; i++)
		stream_update(channel[i], 0);
}
