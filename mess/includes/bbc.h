/******************************************************************************
    BBC Model B

    MESS Driver By:

	Gordon Jefferyes
	mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

int bbcb_keyscan(void);
int startbank;

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

int bbcb_load_rom(int id);

void init_machine_bbca(void);

void init_machine_bbcb(void);
void stop_machine_bbcb(void);

void init_machine_bbcb1770(void);
void stop_machine_bbcb1770(void);

void init_machine_bbcbp(void);
void stop_machine_bbcbp(void);

void init_machine_bbcb6502(void);
void stop_machine_bbcb6502(void);


int bbc_floppy_init(int);

void bbc_floppy_exit(int);
void check_disc_status(void);

READ_HANDLER ( bbc_wd1770_read );
WRITE_HANDLER ( bbc_wd1770_write );

READ_HANDLER( bbc_i8271_read );
WRITE_HANDLER( bbc_i8271_write );



int bbc_vh_starta(void);
int bbc_vh_startb(void);
int bbc_vh_startbp(void);

void bbc_vh_stop(void);
void bbc_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);

void bbc_frameclock(void);

WRITE_HANDLER ( videoULA_w );

void setscreenstart(int b4,int b5);
void bbcbp_setvideoshadow(int vdusel);

WRITE_HANDLER ( BBC_6845_w );
READ_HANDLER ( BBC_6845_r );

extern unsigned char vidmem[0x10000];

