/******************************************************************************
    BBC Model B

    MESS Driver By:

	Gordon Jefferyes
	mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

int bbc_vh_init(void);
int bbc_vh_start(void);
void bbc_vh_stop(void);
void bbc_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);


WRITE_HANDLER ( crtc6845_w );
WRITE_HANDLER ( videoULA_w );
READ_HANDLER ( crtc6845_r );
READ_HANDLER ( videoULA_r );
void setscreenstart(int b4,int b5);



extern unsigned char vidmem[0x8000];

