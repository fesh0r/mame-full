/***************************************************************************

  hd6845s.c

  Functions to emulate the video controller HD6845S.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/hd6845s.h"



/* register write masks for HD6845S - anded with value
written to register */

static unsigned char HD6845S_RegisterWriteMask[18]=
{
	0x0ff,
	0x0ff,
	0x0ff,
	0x0ff,
	0x07f,
	0x01f,
	0x07f,
	0x07f,
	0x0f3,
	0x01f,
	0x07f,
	0x01f,
	0x03f,
	0x0ff,
	0x03f,
	0x0ff,
	0x03f,
	0x0ff
};

/* anded with value to read from register */
static unsigned char HD6845S_RegisterReadMask[18]=
{
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x03f,
	0x0ff,
	0x03f,
	0x0ff,
	0x03f,
	0x0ff
};

static  CRTC6845 crt;

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int     hd6845s_vh_start(void)
{
        if (generic_vh_start() != 0)
		return 1;

        return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void    hd6845s_vh_stop(void)
{
        generic_vh_stop();
}

/***************************************************************************

  Write to an indexed register of the 6845 CRTC

***************************************************************************/
void    hd6845s_register_w(int data)
{
        unsigned char val;

	/* ignore writes in range 15..31 */
        if (crt.RegIndex<15)
        {

                /* mask bits of depending on register we are writing to */
                val = data & HD6845S_RegisterWriteMask[crt.RegIndex];
                /* store data to register */
                crt.Registers[crt.RegIndex] = val;
        }
        else
        {
                crt.Registers[crt.RegIndex] = data;
        }

}

/***************************************************************************
  Write to the index register of the 6845 CRTC
***************************************************************************/
void    hd6845s_index_w(int data)
{
        crt.RegIndex = data & 0x01f;
}

/***************************************************************************
  Read from an indexed register of the 6845 CRTC
***************************************************************************/
int     hd6845s_register_r()
{
        unsigned char val;

	if (crt.RegIndex>17)
		return 0;

	val = crt.Registers[crt.RegIndex];

	return (val & HD6845S_RegisterReadMask[crt.RegIndex]);
}

int	hd6845s_getreg(int RegIndex)
{
	return crt.Registers[RegIndex];
}


/***************************************************************************
  Read the index register of the 6845 CRTC
***************************************************************************/
int     hd6845s_index_r()
{
	return 0x0ff;
}

void    hd6845s_update_line(void)
{
        crt.Registers[HD6845S_RA]++;
        crt.Registers[HD6845S_RA]&=31;


        if (crt.Registers[HD6845S_RA]==(crt.Registers[HD6845S_MAX_RASTER]+1))
        {
                crt.Registers[HD6845S_MA]+=crt.Registers[HD6845S_H_DISP];
                crt.Registers[HD6845S_RA] = 0;
                crt.Registers[HD6845S_LC]++;

                if (crt.Registers[HD6845S_LC]==crt.Registers[HD6845S_V_TOT])
                {
                        int MA;

                        MA = crt.Registers[HD6845S_SCREEN_ADDR_L] |
                              (crt.Registers[HD6845S_SCREEN_ADDR_H]<<8);

                        crt.Registers[HD6845S_LC] = 0;
                        crt.Registers[HD6845S_MA] = MA;

                        crt.Registers[HD6845S_STATE]|=HD6845S_VDISP;
                }

                if (crt.Registers[HD6845S_LC]==crt.Registers[HD6845S_V_DISP])
                {
                        crt.Registers[HD6845S_STATE]&=~HD6845S_VDISP;
                }
        }
}

void    hd6845s_update_clocks(int clocks)
{
        hd6845s_update_line();
}
