/****************************************************************************

 PC-Engine / Turbo Grafx 16 driver
 by Charles Mac Donald
 E-Mail: cgfm2@hooked.net

 Thanks to David Shadoff and Brian McPhail for help with the driver.

****************************************************************************/

#include <assert.h>
#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/vdc.h"
#include "cpu/h6280/h6280.h"
#include "includes/pce.h"
#include "devices/cartslot.h"

static INTERRUPT_GEN( pce_interrupt )
{
    int ret = 0;

    /* bump current scanline */
    vdc.curline = (vdc.curline + 1) % VDC_LPF;

    /* draw a line of the display */
    if(vdc.curline < vdc.physical_height)
    {
        pce_refresh_line(vdc.curline);
    }

    /* generate interrupt on line compare if necessary*/
    if(vdc.vdc_data[CR].w & CR_RC)
    if(vdc.curline == ((vdc.vdc_data[RCR].w & 0x03FF) - 64))
    {
        vdc.status |= VDC_RR;
        ret = 1;
    }

    /* handle frame events */
    if(vdc.curline == 256)
    {
        vdc.status |= VDC_VD;   /* set vblank flag */

        /* do VRAM > SATB DMA if the enable bit is set or the DVSSR reg. was written to */
        if((vdc.vdc_data[DCR].w & DCR_DSR) || vdc.dvssr_write)
        {
            if(vdc.dvssr_write) vdc.dvssr_write = 0;
#ifdef MAME_DEBUG
			assert(((vdc.vdc_data[DVSSR].w<<1) + 512) <= 0x10000);
#endif
            memcpy(&vdc.sprite_ram, &vdc.vram[vdc.vdc_data[DVSSR].w<<1], 512);
            vdc.status |= VDC_DS;   /* set satb done flag */

            /* generate interrupt if needed */
            if(vdc.vdc_data[DCR].w & DCR_DSC)
            {
                ret = 1;
            }
        }

        if(vdc.vdc_data[CR].w & CR_VR)  /* generate IRQ1 if enabled */
        {
            ret = 1;
        }
    }
	if (ret)
		cpu_set_irq_line(0, 0, PULSE_LINE);
}

/* stubs for the irq/psg/timer code */

static WRITE_HANDLER ( pce_irq_w)
{
}

static READ_HANDLER ( pce_irq_r )
{
    return 0x00;
}

static WRITE_HANDLER ( pce_timer_w )
{
}

static READ_HANDLER ( pce_timer_r )
{
    return 0x00;
}

static WRITE_HANDLER ( pce_psg_w )
{
}

static READ_HANDLER ( pce_psg_r )
{
    return 0x00;
}

ADDRESS_MAP_START( pce_mem , ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE( 0x000000, 0x1EDFFF) AM_ROM
    AM_RANGE( 0x1EE000, 0x1EFFFF) AM_RAM
    AM_RANGE( 0x1F0000, 0x1F1FFF) AM_RAM	AM_BASE( &pce_user_ram )
    AM_RANGE( 0x1FE000, 0x1FE003) AM_READWRITE( vdc_r, vdc_w )
    AM_RANGE( 0x1FE400, 0x1FE407) AM_READWRITE( vce_r, vce_w )
    AM_RANGE( 0x1FE800, 0x1FE80F) AM_READWRITE( pce_psg_r, pce_psg_w )
    AM_RANGE( 0x1FEC00, 0x1FEC01) AM_READWRITE( pce_timer_r, pce_timer_w )
    AM_RANGE( 0x1FF000, 0x1FF000) AM_READWRITE( pce_joystick_r, pce_joystick_w )
    AM_RANGE( 0x1FF402, 0x1FF403) AM_READWRITE( pce_irq_r, pce_irq_w )
ADDRESS_MAP_END

ADDRESS_MAP_START( pce_io , ADDRESS_SPACE_IO, 8)
    AM_RANGE( 0x00, 0x03) AM_READWRITE( vdc_r, vdc_w )
ADDRESS_MAP_END

/* todo: alternate forms of input (multitap, mouse, etc.) */
INPUT_PORTS_START( pce )

    PORT_START  /* Player 1 controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* button I */
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* button II */
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON4 ) /* select */
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) /* run */
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
#if 0
    PORT_START  /* Fake dipswitches for system config */
    PORT_DIPNAME( 0x01, 0x01, "Console type")
    PORT_DIPSETTING(    0x00, "Turbo-Grafx 16")
    PORT_DIPSETTING(    0x01, "PC-Engine")

    PORT_DIPNAME( 0x01, 0x01, "Joystick type")
    PORT_DIPSETTING(    0x00, "2 Button")
    PORT_DIPSETTING(    0x01, "6 Button")
#endif
INPUT_PORTS_END


#if 0
static struct GfxLayout pce_bg_layout =
{
        8, 8,
        2048,
        4,
        {0x00*8, 0x01*8, 0x10*8, 0x11*8 },
        {0, 1, 2, 3, 4, 5, 6, 7 },
        { 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
        32*8,
};

static struct GfxLayout pce_obj_layout =
{
        16, 16,
        512,
        4,
        {0x00*8, 0x20*8, 0x40*8, 0x60*8},
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
        { 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, 8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
        128*8,
};

static struct GfxDecodeInfo pce_gfxdecodeinfo[] =
{
   { 1, 0x0000, &pce_bg_layout, 0, 0x10 },
   { 1, 0x0000, &pce_obj_layout, 0x100, 0x10 },
	{-1}
};
#endif


static MACHINE_DRIVER_START( pce )
	/* basic machine hardware */
	MDRV_CPU_ADD(H6280, 7195090)
	MDRV_CPU_PROGRAM_MAP(pce_mem, 0)
	MDRV_CPU_IO_MAP(pce_io, 0)
	MDRV_CPU_VBLANK_INT(pce_interrupt, VDC_LPF)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(45*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 45*8-1, 0*8, 32*8-1)
	/* MDRV_GFXDECODE( pce_gfxdecodeinfo ) */
	MDRV_PALETTE_LENGTH(512)
	MDRV_COLORTABLE_LENGTH(512)

	MDRV_VIDEO_START( pce )
	MDRV_VIDEO_UPDATE( pce )

	MDRV_NVRAM_HANDLER( pce )
MACHINE_DRIVER_END

SYSTEM_CONFIG_START(pce)
	CONFIG_DEVICE_CARTSLOT_REQ(1, "pce\0", NULL, NULL, device_load_pce_cart, NULL, NULL, NULL)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

#define rom_pce NULL

/*	   YEAR  NAME	   PARENT	COMPAT	MACHINE	INPUT	 INIT	CONFIG  COMPANY	 FULLNAME */
CONSX( 1987, pce,	   0,		0,		pce,	pce, 	 0,		pce,	"Nippon Electronic Company", "PC Engine/TurboGrafx 16", GAME_NOT_WORKING | GAME_NO_SOUND )

