/***************************************************************************

  vidhrdw/oric.c

  do all the real wacky stuff to render an Oric Text / Hires screen

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

void oric_vh_stop(void);
void oric_init_char_attrs(void);
int oric_powerup_screen;

typedef struct{
        int tcolour;
        int pcolour;
        int inverse;
        int dblchar;
        int flash;
        int alternative;
        int hires;
        int hires_this_line;
        int flashshow;
        int attr;
} ORIC_VIDEO_STRUCT;

ORIC_VIDEO_STRUCT _ORIC;

ORIC_VIDEO_STRUCT _ORIC_tmp;

ORIC_VIDEO_STRUCT _ORIC_HIRES[10];

unsigned char inverse_attrs[] = { 7,6,5,4,3,2,1,0 };

int oric_vh_start(void)
{
        oric_init_char_attrs();
        _ORIC.hires = 0;
        return 0;
}

void oric_vh_stop(void)
{
	return;
}

void oric_set_powerscreen_mode(int mode)
{
        oric_powerup_screen = mode;
}

void oric_set_flash_show(int mode)
{
        _ORIC.flashshow = mode;
}

void oric_init_char_attrs(void)
{
        int i;

        _ORIC.pcolour = 0;
        _ORIC.tcolour = 7;
        _ORIC.dblchar = 0;
        _ORIC.flash = 0;
        _ORIC.inverse = 0;
        _ORIC.alternative = 0;
        _ORIC.hires_this_line = 0;
        _ORIC_tmp.pcolour = 0;
        _ORIC_tmp.tcolour = 7;
        _ORIC_tmp.dblchar = 0;
        _ORIC_tmp.flash = 0;
        _ORIC_tmp.inverse = 0;
        _ORIC_tmp.alternative = 0;
        _ORIC_tmp.hires_this_line = 0;
        for (i=0;i<=8;i++) {
                _ORIC_HIRES[i].pcolour = 0;
                _ORIC_HIRES[i].tcolour = 7;
                _ORIC_HIRES[i].flash = 0;
                _ORIC_HIRES[i].inverse = 0;
                _ORIC_HIRES[i].attr = 0;
        }
}

/***************************************************************************
  oric_vh_screenrefresh
***************************************************************************/
void oric_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{

/**** TODO ... HIRES ... how do the hires attr's set HIRES mode?? ****/
/**** TODO ... HIRES MODE LINES,  render cell from HIRES BITMAP MEM AREA ****/

        int x,y,i,xx,yy;
	int offset;
        int half;
        int do_bmp;
        int hires_offset;
        int ink_colour;
        int paper_colour;
        unsigned short o1,o2,o3;
        unsigned short *cdst[8];
	unsigned char *text_ram;
        unsigned char *hires_ram;
        unsigned char *char_ram;
        unsigned char *achar_ram;
        unsigned char *hchar_ram;
        unsigned char *hachar_ram;
        unsigned char *tchar_ram;
        unsigned char *tachar_ram;
        unsigned char c,b;
        unsigned char d, hires_c;
        unsigned char *RAM = memory_region(REGION_CPU1);
        text_ram    = &RAM[0xBB80] ;
        hires_ram   = &RAM[0xA000] ;
        char_ram    = &RAM[0xB400] ;
        achar_ram   = &RAM[0xB800] ;
        hchar_ram   = &RAM[0x9800] ;
        hachar_ram  = &RAM[0x9c00] ;
        tchar_ram   = &RAM[0xB400] ;
        tachar_ram  = &RAM[0xB800] ;


        /*  Render the Charcters  /  Hires cells .. */

        /*
                attribute char's ...

                0-7 text colour
                8 normal / noflash / nodouble
                9 alternative / noflash / nodouble
                10 double normal noflash
                11 double alternative noflash
                12 normal flash single
                13 flash alt single
                14 flash normal double
                15 flash alt double
                16-23 paper colour
                24  ???
                25  ???
                26  ???
                27  ???
                28 hires?
                29 hires?
                30 hires?
                31 hires?
        */

        if (_ORIC.hires == 0) {
                c = text_ram[ (28*40)-1 ];
                if (c == 0x1c) _ORIC.hires = 1;
                if (c == 0x1d) _ORIC.hires = 1;
                if (c == 0x1e) _ORIC.hires = 1;
                if (c == 0x1f) _ORIC.hires = 1;
        }
        if (_ORIC.hires == 1) {
                if (RAM[0xbfdf] == 0x18) _ORIC.hires = 0;
                if (RAM[0xbfdf] == 0x19) _ORIC.hires = 0;
                if (RAM[0xbfdf] == 0x1a) _ORIC.hires = 0;
                if (RAM[0xbfdf] == 0x1b) _ORIC.hires = 0;
        }
        oric_init_char_attrs();
        for (offset = 0; offset < (28*40); offset++)
        {
                c = text_ram[offset];
                d = c;
                c &= 0x7f;
                y = (offset / 40);
                x = (offset % 40) * 3;
                half = 0;
                if ( ( y & 1 ) == 1 ) half = 4;
                y *= 8;
                if ( x == 0 ) oric_init_char_attrs();
                // colors and text attrs ...

                if (c <= (unsigned char)7) _ORIC.tcolour = c;
                if (c >= (unsigned char)16)
                      if (c <= (unsigned char)23) _ORIC.pcolour = c-16;
                if (c == (unsigned char)8) { _ORIC.alternative = 0;
                      _ORIC.dblchar = 0; _ORIC.flash = 0; }
                if (c == (unsigned char)9) { _ORIC.alternative = 1;
                      _ORIC.dblchar = 0; _ORIC.flash = 0; }
                if (c == (unsigned char)10) { _ORIC.alternative = 0;
                      _ORIC.dblchar = 1; _ORIC.flash = 0; }
                if (c == (unsigned char)11) { _ORIC.alternative = 1;
                      _ORIC.dblchar = 1; _ORIC.flash = 0; }
                if (c == (unsigned char)12) { _ORIC.alternative = 0;
                      _ORIC.dblchar = 0; _ORIC.flash = 1; }
                if (c == (unsigned char)13) { _ORIC.alternative = 1;
                      _ORIC.dblchar = 0; _ORIC.flash = 1; }
                if (c == (unsigned char)14) { _ORIC.alternative = 0;
                      _ORIC.dblchar = 1; _ORIC.flash = 1; }
                if (c == (unsigned char)15) { _ORIC.alternative = 1;
                      _ORIC.dblchar = 1; _ORIC.flash = 1; }

                for (i=1;i<=8;i++) {
                      _ORIC_HIRES[i].attr = 0;
                      hires_offset = 0xA000 + ( (y+i-1) * 40 );
                      hires_offset += ( offset % 40 );
                      hires_c = RAM[hires_offset];
                      if ( hires_c <= 0x1f )
                      {
                        _ORIC_HIRES[i].attr = 1;
                        if (hires_c <= (unsigned char)7) _ORIC_HIRES[i].tcolour = hires_c;
                        if (hires_c >= (unsigned char)16)
                          if (hires_c <= (unsigned char)23) _ORIC_HIRES[i].pcolour = hires_c-16;

                        // _ORIC_HIRES[i].flash       =  < something!! >
                      }
                      _ORIC_HIRES[i].inverse = 0;
                      if ( (hires_c & 0x80) == 0x80) _ORIC_HIRES[i].inverse = 1;
                }

                if (oric_powerup_screen == 1) {
                   /* all black spaces appart from
                   17,7 -- 40,7
                   32,12 -- 40,12
                   8,20 -- 40,20
                   24,26 -- 40,26
                   based uponthe screen that Euphoric shows */
                   c = 0;
                   yy = (offset / 40);
                   xx = (offset % 40);
                   if ( yy == 7  && xx >= 17 ) c = 1;
                   if ( yy == 12 && xx >= 32 ) c = 1;
                   if ( yy == 20 && xx >= 8  ) c = 1;
                   if ( yy == 26 && xx >= 24 ) c = 1;
                }
                for (i=0;i<8;i++) cdst[i]=(unsigned short *)bitmap->line[(y)+i];
                for (i=0;i<8;i++) {
                        _ORIC.inverse = 0;
                        if ( d > 0x7f ) _ORIC.inverse = 1;
                        char_ram = tchar_ram;
                        achar_ram = tachar_ram;
                        if (_ORIC.hires == 1) {
                                char_ram = hchar_ram;
                                achar_ram = hachar_ram;
                        }
                        b = char_ram[ (c * 8) + i];
                        if ( _ORIC.dblchar == 1 )
                                b = char_ram[ (c * 8) + (int)(i/2) + half];
                        if ( _ORIC.alternative == 1 )
                        {
                                b = achar_ram[ (c * 8) + i];
                                if ( _ORIC.dblchar == 1 )
                                        b = achar_ram[ (c * 8) + (int)(i/2) + half ];
                        }
                        do_bmp = 0;
                        if (_ORIC.hires == 1) if ( (offset / 40) < 25) do_bmp = 1;
                        ink_colour = _ORIC.tcolour;
                        paper_colour = _ORIC.pcolour;
                        if ( _ORIC.inverse == 1 ) {
                                ink_colour = inverse_attrs[_ORIC.tcolour];
                                paper_colour = inverse_attrs[_ORIC.pcolour];
                        }
                        if (do_bmp == 1) {
                                hires_offset = 0xA000 + ( (y+i) * 40 );
                                hires_offset += ( offset % 40 );
                                b = RAM[hires_offset];
                                if (_ORIC_HIRES[i+1].attr == 1) b = 0;
                                c = 0x55;
                        }
                        if ( c < 0x20 ) b = 0;
                        if (_ORIC.flash == 1) if (_ORIC.flashshow == 0) b = 0;
                        if (oric_powerup_screen == 1) b = (c * 63);
                        o1 = o2 = o3 = 0;
                        if (do_bmp == 1) {
                           ink_colour = _ORIC_HIRES[i+1].tcolour;
                           paper_colour = _ORIC_HIRES[i+1].pcolour;
                           if ( _ORIC_HIRES[i+1].inverse == 1) {
                              ink_colour = inverse_attrs[_ORIC_HIRES[i+1].tcolour];
                              paper_colour = inverse_attrs[_ORIC_HIRES[i+1].pcolour];
                           }
                           if (_ORIC_HIRES[i+1].attr == 1) b = 0;
                        }
                        if (b & 32) o1 += Machine->pens[ink_colour];
                        else        o1 += Machine->pens[paper_colour];
                        if (b & 16) o1 += ( Machine->pens[ink_colour] * 0x100 );
                        else        o1 += ( Machine->pens[paper_colour] * 0x100 );
                        if (b & 8 ) o2 += Machine->pens[ink_colour];
                        else        o2 += Machine->pens[paper_colour];
                        if (b & 4 ) o2 += ( Machine->pens[ink_colour] * 0x100 );
                        else        o2 += ( Machine->pens[paper_colour] * 0x100 );
                        if (b & 2 ) o3 += Machine->pens[ink_colour];
                        else        o3 += Machine->pens[paper_colour];
                        if (b & 1 ) o3 += ( Machine->pens[ink_colour] * 0x100 );
                        else        o3 += ( Machine->pens[paper_colour] * 0x100 );
                        cdst[i][x+0] = o1;
                        cdst[i][x+1] = o2;
                        cdst[i][x+2] = o3;
                }
        }
}

