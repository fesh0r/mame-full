/***************************************************************************

  amstrad.c.c

  Functions to emulate the video hardware of the Amstrad CPC.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "mess/vidhrdw/amstrad.h"
#include "mess/vidhrdw/hd6845s.h"

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/

static unsigned long Mode0Lookup[256];
static unsigned long Mode1Lookup[256];


int amstrad_vh_start(void)
{

	int i;

	for (i=0; i<256; i++)
	{
		int pen;

		pen = (
			((i & (1<<7))>>(7-0)) |
			((i & (1<<3))>>(3-1)) |
			((i & (1<<5))>>(5-2)) |
			((i & (1<<1))>>(3-1))
			);

		Mode0Lookup[i] = pen;

		pen = (
			( ( (i & (1<<7)) >>7) <<0) |
		        ( ( (i & (1<<3)) >>3) <<1)
			);

		Mode1Lookup[i] = pen;

	}

	return 0;
}

void    amstrad_vh_stop(void) {
}

extern unsigned char *Amstrad_Memory;
extern unsigned char AmstradCPC_PenColours[18];
extern unsigned char AmstradCPC_GA_RomConfiguration;

static void amstrad_vh_decodebyte(unsigned char byte1, unsigned char
byte2, unsigned char *bm)
{
	unsigned char *dest = bm;

	/* depending on the mode! */
	switch (AmstradCPC_GA_RomConfiguration & 0x03)
	{
		/* mode 0 - low resolution - 16 colours */
		case 0:
		{
			//int i;
			int cpcpen,messpen;
			unsigned char data;

			data = byte1;

			{
				cpcpen = Mode0Lookup[data];
				messpen = Machine->pens[AmstradCPC_PenColours[cpcpen]];
				dest[0] = messpen;
				dest[1] = messpen;
				dest[2] = messpen;
				dest[3] = messpen;
				dest+=4;

				data = data<<1;

				cpcpen = Mode0Lookup[data];
				messpen = Machine->pens[AmstradCPC_PenColours[cpcpen]];
				dest[0] = messpen;
				dest[1] = messpen;
				dest[2] = messpen;
				dest[3] = messpen;
				dest+=4;
			}

			data = byte2;

			{
				cpcpen = Mode0Lookup[data];
				messpen = Machine->pens[AmstradCPC_PenColours[cpcpen]];
				dest[0] = messpen;
				dest[1] = messpen;
				dest[2] = messpen;
				dest[3] = messpen;
				dest+=4;

				data = data<<1;

				cpcpen = Mode0Lookup[data];
				messpen = Machine->pens[AmstradCPC_PenColours[cpcpen]];
				dest[0] = messpen;
				dest[1] = messpen;
				dest[2] = messpen;
				dest[3] = messpen;
				dest+=4;
			}
		}
		break;

		/* mode 1 - medium resolution - 4 colours */
		case 1:
		{
			int i;
			int cpcpen;
			int messpen;
			unsigned char data;

			data = byte1;

			for (i=0; i<4; i++)
			{
				cpcpen = Mode1Lookup[data & 0x0ff];
				messpen =Machine->pens[AmstradCPC_PenColours[cpcpen]];
				dest[0] = messpen;
				dest[1] = messpen;
				dest+=2;

				data = data<<1;
			}

			data = byte2;

			for (i=0; i<4; i++)
			{
				cpcpen = Mode1Lookup[data & 0x0ff];
				messpen =Machine->pens[AmstradCPC_PenColours[cpcpen]];
				dest[0] = messpen;
				dest[1] = messpen;
				dest+=2;

				data = data<<1;
			}

		}
		break;

		case 2:
		{
			int i;
			unsigned long Data = (byte1<<8) | byte2;
			int cpcpen,messpen;

			for (i=0; i<16; i++)
			{
				cpcpen = (Data>>15) & 0x01;
				messpen =Machine->pens[AmstradCPC_PenColours[cpcpen]];

				dest[0] = messpen;

				dest++;
				Data = Data<<1;

			}

		}
		break;
	}


}

/***************************************************************************
  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
void amstrad_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	unsigned char *bm;

	int vdisp = hd6845s_getreg(6);
	int hdisp = hd6845s_getreg(1);
	int maxras = hd6845s_getreg(9);

	int scr_addr_hi = hd6845s_getreg(12);
	int scr_addr_lo = hd6845s_getreg(13);

	int v,r,h;

	int ma_store;
	int ma;
	int l;

	l = 8;

	/* initial ma */
	ma = ((scr_addr_hi & 0x0ff)<<8) | (scr_addr_lo & 0x0ff);

	for (v=0; v<vdisp; v++)
	{
		ma_store = ma;

		for (r=0; r<(maxras+1); r++)
		{
			/* reload ma from ma_store */
			ma = ma_store;

			/* setup render ptr for this line */
			bm = bitmap->line[l];

			/* render visible part of display */
			for (h=0; h<hdisp; h++)
			{
				unsigned char byte0, byte1;
				unsigned long Addr;

				/* calc mem addr to fetch data from
				based on ma, and ra */
				Addr = (((ma>>(4+8)) & 0x03)<<14) |
					((r & 0x07)<<11) |
					((ma & 0x03ff)<<1);

				/* get data from memory */
				byte0 = Amstrad_Memory[Addr];
				byte1 = Amstrad_Memory[Addr+1];

				/* decode byte 0 and 1 depending on mode*/
				amstrad_vh_decodebyte(byte0,byte1, bm);
				bm+=16;

				/* update ma */
				ma++;
			}

			l++;
		}
	}

}

