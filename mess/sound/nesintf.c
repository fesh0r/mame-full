/* interface between MAME and nes_apu.c */
/*   Copyright (C) 2000 Matthew Conte   */
/* This code may be freely distributed! */

#include "driver.h"
#include "nesintf.h"
#include "nes_apu2.h"

struct nes_sound
{
	apu_t *apu;
	const struct NESinterface *intf;
	sound_stream *stream;
};



struct nes_sound *get_info(int indx)
{
	return (struct nes_sound *) sndti_token(SOUND_NES, indx);
}



READ8_HANDLER(NESPSG_0_r)
{
	apu_setcontext(get_info(0)->apu);
	return apu_read(0x4000 + offset);
}



READ8_HANDLER(NESPSG_1_r)
{
	apu_setcontext(get_info(1)->apu);
	return apu_read(0x4000 + offset);
}



WRITE8_HANDLER(NESPSG_0_w)
{
	apu_t *apu;

	if (offset == 0x14) /* OAM DMA */
	{
		if (get_info(0)->intf->apu_callback_w)
			get_info(0)->intf->apu_callback_w(0, data);
		else
			logerror ("NES apu reg %d write uncaught, data: %02x\n", offset, data);
	}
	else
	{
		apu = (apu_t *) sndti_token(SOUND_NES, 0);
		apu_setcontext(apu);

		stream_update(get_info(0)->stream, 0);
		apu_setcontext(get_info(0)->apu);
		apu_write(0x4000 + offset, data);
	}
}



WRITE8_HANDLER(NESPSG_1_w)
{
	if (offset == 0x14) /* OAM DMA */
	{
		if (get_info(1)->intf->apu_callback_w)
			get_info(1)->intf->apu_callback_w(0, data);
		else
			logerror ("NES apu reg %d write uncaught, data: %02x\n", offset, data);
	}
	else
	{
		stream_update(get_info(1)->stream, 0);
		apu_setcontext(get_info(1)->apu);
		apu_write(0x4000 + offset, data);
	}
}



static void process(void *param,stream_sample_t **inputs, stream_sample_t **_buffer,int length)
{
	struct nes_sound *info;
	info = (struct nes_sound *) param;
	apu_setcontext(info->apu);
	apu_process((void *) _buffer[0], length);
}



static void *NESPSG_sh_start(int sndindex, int clock, const void *config)
{
	int sample_rate;
	struct nes_sound *info;
	const struct NESinterface *intf;

	info = (struct nes_sound *) auto_malloc(sizeof(*info));
	intf = (const struct NESinterface *) config;

	sample_rate = Machine->sample_rate;

	/* hack it out as 60Hz for now - bleh! */
	/* NPW 11-Oct-2001 - special case for when sample_rate is zero */
	info->apu = apu_create(intf->cpunum, intf->basefreq, sample_rate ? sample_rate : 44100, Machine->drv->frames_per_second, 16);
	if (!info->apu)
		return NULL;

	info->intf = intf;

	apu_setcontext(info->apu);
	apu_reset();


	if (sample_rate != 0)
	{
		info->stream = stream_create(0, 1, Machine->sample_rate, info, process);
	}

	return (void *) info;
}



static void NESPSG_sh_stop(void *param)
{
	struct nes_sound *info;
	info = (struct nes_sound *) param;
	apu_destroy(&info->apu);
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void nesapu_set_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}



void nesapu_get_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = nesapu_set_info;		break;
		case SNDINFO_PTR_START:							info->start = NESPSG_sh_start;			break;
		case SNDINFO_PTR_STOP:							info->stop = NESPSG_sh_stop;			break;
		case SNDINFO_PTR_RESET:							/* nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "NES";						break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "NES";						break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2005, The MESS Team"; break;
	}
}


