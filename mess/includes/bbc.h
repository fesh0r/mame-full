/******************************************************************************
    BBC Model B

    MESS Driver By:

	Gordon Jefferyes
	mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

extern INTERRUPT_GEN( bbcb_keyscan );
extern int startbank;

WRITE_HANDLER ( page_selecta_w );
WRITE_HANDLER ( page_selectb_w );

WRITE_HANDLER ( memory_w );

READ_HANDLER ( BBC_NOP_00_r );
READ_HANDLER ( BBC_NOP_FE_r );
READ_HANDLER ( BBC_NOP_FF_r );


WRITE_HANDLER ( page_selectbp_w );

WRITE_HANDLER ( memorybp0_w );

READ_HANDLER ( memorybp1_r );
WRITE_HANDLER ( memorybp1_w );
WRITE_HANDLER ( memorybp3_w );

WRITE_HANDLER ( memorybp3_128_w );
WRITE_HANDLER ( memorybp4_128_w );

DEVICE_LOAD ( bbcb_cart );

MACHINE_INIT( bbca );
MACHINE_INIT( bbcb );
MACHINE_INIT( bbcb1770 );
MACHINE_INIT( bbcbp );
MACHINE_INIT( bbcb6502 );

DEVICE_LOAD( bbc_floppy );

void check_disc_status(void);

READ_HANDLER ( bbc_wd1770_read );
WRITE_HANDLER ( bbc_wd1770_write );

READ_HANDLER( bbc_i8271_read );
WRITE_HANDLER( bbc_i8271_write );


extern VIDEO_START( bbca );
extern VIDEO_START( bbcb );
extern VIDEO_START( bbcbp );
extern VIDEO_UPDATE( bbc );
void bbc_frameclock(void);

WRITE_HANDLER ( videoULA_w );

void setscreenstart(int b4,int b5);
void bbcbp_setvideoshadow(int vdusel);

WRITE_HANDLER ( BBC_6845_w );
READ_HANDLER ( BBC_6845_r );

extern unsigned char vidmem[0x10000];

