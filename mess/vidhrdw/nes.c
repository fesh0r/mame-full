/***************************************************************************

  vidhrdw/nes.c

  Routines to control the unique NES video hardware/PPU.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/ppu2c03b.h"
#include "vidhrdw/generic.h"
#include "includes/nes.h"

int nes_vram_sprite[8]; /* Used only by mmc5 for now */

static void ppu_irq(int num)
{
	cpu_set_nmi_line(0, PULSE_LINE);
}

VIDEO_START( nes )
{
	static struct ppu2c03b_interface *ppu_interface;

	ppu_interface = auto_malloc(sizeof(struct ppu2c03b_interface));
	if (!ppu_interface)
		return 1;

	memset(ppu_interface, 0, sizeof(struct ppu2c03b_interface));
	ppu_interface->num				= 1;
	ppu_interface->vrom_region[0]	= nes.chr_chunks ? REGION_GFX1 : REGION_INVALID;
	ppu_interface->mirroring[0]		= PPU_MIRROR_NONE;
	ppu_interface->handler[0]		= ppu_irq;

	return ppu2c03b_init(ppu_interface);
}

PALETTE_INIT( nes )
{
	ppu2c03b_init_palette(0);
}

static void draw_sight(int playerNum, int x_center, int y_center)
{
	int x,y;
	UINT16 color;

	if (playerNum == 2)
		color = Machine->pens[0]; /* grey */
	else
		color = Machine->pens[0x30]; /* white */

	if (x_center<2)   x_center=2;
	if (x_center>253) x_center=253;

	if (y_center<2)   y_center=2;
	if (y_center>253) y_center=253;

	for(y = y_center-5; y < y_center+6; y++)
		if((y >= 0) && (y < 256))
			plot_pixel (Machine->scrbitmap, x_center, y, color);

	for(x = x_center-5; x < x_center+6; x++)
		if((x >= 0) && (x < 256))
			plot_pixel (Machine->scrbitmap, x, y_center, color);
}

/***************************************************************************

  Display refresh

***************************************************************************/
VIDEO_UPDATE( nes )
{
	/* render the ppu */
	ppu2c03b_render( 0, bitmap, 0, 0, 0, 0 );

	if (readinputport (2) == 0x01) /* zapper on port 1 */
	{
		draw_sight (1, readinputport (3), readinputport (4));
	}
	else if (readinputport (2) == 0x10) /* zapper on port 2 */
	{
		draw_sight (1, readinputport (3), readinputport (4));
	}
	else if (readinputport (2) == 0x11) /* zapper on both ports */
	{
		draw_sight (1, readinputport (3), readinputport (4));
		draw_sight (2, readinputport (5), readinputport (6));
	}

	/* if this is a disk system game, check for the flip-disk key */
	if (nes.mapper == 20)
	{
		if (readinputport (11) & 0x01)
		{
			while (readinputport (11) & 0x01) { update_input_ports (); };

			nes_fds.current_side ++;
			if (nes_fds.current_side > nes_fds.sides)
				nes_fds.current_side = 0;

			if (nes_fds.current_side == 0)
			{
				usrintf_showmessage ("No disk inserted.");
			}
			else
			{
				usrintf_showmessage ("Disk set to side %d", nes_fds.current_side);
			}
		}
	}
}
