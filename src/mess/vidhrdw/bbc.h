/******************************************************************************
    BBC Model B

    MESS Driver By:

	Gordon Jefferyes
	mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

int bbc_vh_starta(void);
int bbc_vh_startb(void);
void bbc_vh_stop(void);
void bbc_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

WRITE_HANDLER ( videoULA_w );
READ_HANDLER ( videoULA_r );

void setscreenstart(int b4,int b5);

WRITE_HANDLER ( BBC_6845_w );
READ_HANDLER (BBC_6845_r);

extern unsigned char vidmem[0x8000];

