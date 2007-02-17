#include "driver.h"
#include "konamiic.h"

/* Konami K056800 (MIRAC) */

static UINT8 K056800_host_reg[8];
static UINT8 K056800_sound_reg[8];

static void *K056800_sound_cpu_timer;
static int K056800_sound_cpu_irq1_enable;
static void (*K056800_sound_irq_callback)(int);

static UINT8 K056800_host_reg_r(int reg)
{
	return K056800_host_reg[reg];
}

static void K056800_host_reg_w(int reg, UINT8 data)
{
	K056800_sound_reg[reg] = data;

	if (reg == 7)
	{
		K056800_sound_irq_callback(1);
	}
}

static UINT8 K056800_sound_reg_r(int reg)
{
	return K056800_sound_reg[reg];
}

static void K056800_sound_reg_w(int reg, UINT8 data)
{
	if (reg == 4)
	{
		K056800_sound_cpu_irq1_enable = data & 0x01;
	}

	K056800_host_reg[reg] = data;
}

static void K056800_sound_cpu_timer_tick(int param)
{
	if (K056800_sound_cpu_irq1_enable)
	{
		K056800_sound_irq_callback(0);
	}
}

void K056800_init(void (* irq_callback)(int))
{
	double timer_period;
	K056800_sound_irq_callback = irq_callback;

	memset(K056800_host_reg, 0, sizeof(K056800_host_reg));
	memset(K056800_sound_reg, 0, sizeof(K056800_sound_reg));

	timer_period = 1000.0/(44100.0/128.0);	// roughly 2.9us

	K056800_sound_cpu_timer = timer_alloc(K056800_sound_cpu_timer_tick);
	timer_adjust(K056800_sound_cpu_timer, TIME_IN_MSEC(timer_period), 0, TIME_IN_MSEC(timer_period));
}

READ32_HANDLER(K056800_host_r)
{
	UINT32 r = 0;

	if (!(mem_mask & 0xff000000))
	{
		r |= K056800_host_reg_r((offset*4)+0) << 24;
	}
	if (!(mem_mask & 0x00ff0000))
	{
		r |= K056800_host_reg_r((offset*4)+1) << 16;
	}
	if (!(mem_mask & 0x0000ff00))
	{
		r |= K056800_host_reg_r((offset*4)+2) << 8;
	}
	if (!(mem_mask & 0x000000ff))
	{
		r |= K056800_host_reg_r((offset*4)+3) << 0;
	}

	return r;
}

WRITE32_HANDLER(K056800_host_w)
{
	if (!(mem_mask & 0xff000000))
	{
		K056800_host_reg_w((offset*4)+0, (data >> 24) & 0xff);
	}
	if (!(mem_mask & 0x00ff0000))
	{
		K056800_host_reg_w((offset*4)+1, (data >> 16) & 0xff);
	}
	if (!(mem_mask & 0x0000ff00))
	{
		K056800_host_reg_w((offset*4)+2, (data >> 8) & 0xff);
	}
	if (!(mem_mask & 0x000000ff))
	{
		K056800_host_reg_w((offset*4)+3, (data >> 0) & 0xff);
	}
}

READ16_HANDLER(K056800_sound_r)
{
	return K056800_sound_reg_r(offset);
}

WRITE16_HANDLER(K056800_sound_w)
{
	K056800_sound_reg_w(offset, data);
}
