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


void crtc6845_w(int offset, int data);
void videoULA_w(int offset, int data);
int crtc6845_r(int offset);
int videoULA_r(int offset);
void setscreenstart(int b4,int b5);



extern unsigned char vidmem[0x8000];

