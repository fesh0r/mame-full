/*

                        ________________                                            ________________
           TPA   1  ---|       \/       |---  40  Vdd          PREDISPLAY_   1  ---|       \/       |---  40  Vdd
           TPB   2  ---|                |---  39  PMSEL          *DISPLAY_   2  ---|                |---  39  PAL/NTSC_
          MRD_   3  ---|                |---  38  PMWR_                PCB   3  ---|                |---  38  CPUCLK
          MWR_   4  ---|                |---  37  *CMSEL              CCB1   4  ---|                |---  37  XTAL (DOT)
         MA0/8   5  ---|                |---  36  CMWR_               BUS7   5  ---|                |---  36  XTAL (DOT)_
         MA1/9   6  ---|                |---  35  PMA0                CCB0   6  ---|                |---  35  *ADDRSTB_
        MA2/10   7  ---|                |---  34  PMA1                BUS6   7  ---|                |---  34  MRD_
        MA3/11   8  ---|                |---  33  PMA2                CDB5   8  ---|                |---  33  TPB
        MA4/12   9  ---|                |---  32  PMA3                BUS5   9  ---|                |---  32  *CMSEL
        MA5/13  10  ---|    CDP1869C    |---  31  PMA4                CDB4  10  ---|  CDP1870/76C   |---  31  BURST
        MA6/14  11  ---|    top view    |---  30  PMA5                BUS4  11  ---|    top view    |---  30  *H SYNC_
        MA7/15  12  ---|                |---  29  PMA6                CDB3  12  ---|                |---  29  COMPSYNC_
            N0  13  ---|                |---  28  PMA7                BUS3  13  ---|                |---  28  LUM / (RED)^
            N1  14  ---|                |---  27  PMA8                CDB2  14  ---|                |---  27  PAL CHROM / (BLUE)^
            N2  15  ---|                |---  26  PMA9                BUS2  15  ---|                |---  26  NTSC CHROM / (GREEN)^
      *H SYNC_  16  ---|                |---  25  CMA3/PMA10          CDB1  16  ---|                |---  25  XTAL_ (CHROM)
     *DISPLAY_  17  ---|                |---  24  CMA2                BUS1  17  ---|                |---  24  XTAL (CHROM)
     *ADDRSTB_  18  ---|                |---  23  CMA1                CDB0  18  ---|                |---  23  EMS_
         SOUND  19  ---|                |---  22  CMA0                BUS0  19  ---|                |---  22  EVS_
           VSS  20  ---|________________|---  21  *N=3_                Vss  20  ---|________________|---  21  *N=3_


                 * = INTERCHIP CONNECTIONS      ^ = FOR THE RGB BOND-OUT OPTION (CDP1876C)      _ = ACTIVE LOW

*/

#ifndef __CDP1869_VIDEO__
#define __CDP1869_VIDEO__

#define CDP1870_DOT_CLK_PAL			5626000.0
#define CDP1870_DOT_CLK_NTSC		5670000.0
#define CDP1870_COLOR_CLK_PAL		8867236.0
#define CDP1870_COLOR_CLK_NTSC		7159090.0

#define CDP1870_CPU_CLK_PAL			(CDP1870_DOT_CLK_PAL / 2)
#define CDP1870_CPU_CLK_NTSC		(CDP1870_DOT_CLK_NTSC / 2)

#define CDP1870_CHAR_WIDTH			6

#define CDP1870_SCREEN_WIDTH		(60 * CDP1870_CHAR_WIDTH)
#define CDP1870_SCREEN_START		(10 * CDP1870_CHAR_WIDTH)
#define CDP1870_SCREEN_END			(50 * CDP1870_CHAR_WIDTH)
#define CDP1870_HBLANK_START		(56 * CDP1870_CHAR_WIDTH)

#define CDP1870_SCANLINES_PAL		312
#define CDP1870_SCANLINES_NTSC		262

#define CDP1870_FPS_PAL				CDP1870_DOT_CLK_PAL / CDP1870_SCREEN_WIDTH / CDP1870_SCANLINES_PAL
#define CDP1870_FPS_NTSC			CDP1870_DOT_CLK_NTSC / CDP1870_SCREEN_WIDTH / CDP1870_SCANLINES_NTSC

#define CPD1870_TOTAL_SCANLINES_PAL				312
#define CPD1870_SCANLINE_VBLANK_START_PAL		308
#define CPD1870_SCANLINE_DISPLAY_START_PAL		44
#define CPD1870_SCANLINE_DISPLAY_END_PAL		260
#define CPD1870_SCANLINE_PREDISPLAY_START_PAL	43
#define CPD1870_SCANLINE_PREDISPLAY_END_PAL		260

#define CPD1870_TOTAL_SCANLINES_NTSC			262
#define CPD1870_SCANLINE_VBLANK_START_NTSC		258
#define CPD1870_SCANLINE_DISPLAY_START_NTSC		36
#define CPD1870_SCANLINE_DISPLAY_END_NTSC		228
#define CPD1870_SCANLINE_PREDISPLAY_START_NTSC	35
#define CPD1870_SCANLINE_PREDISPLAY_END_NTSC	228

WRITE8_HANDLER ( cdp1870_out3_w );
WRITE8_HANDLER ( cdp1869_out_w );

PALETTE_INIT( cdp1869 );
VIDEO_START( cdp1869 );
VIDEO_UPDATE( cdp1869 );

READ8_HANDLER ( cdp1869_charram_r );
READ8_HANDLER ( cdp1869_pageram_r );
WRITE8_HANDLER ( cdp1869_charram_w );
WRITE8_HANDLER ( cdp1869_pageram_w );

#endif
