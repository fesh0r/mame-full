/******************************************************************************
    BBC Model B

    MESS Driver By:

	Gordon Jefferyes
	mess_bbc@gjeffery.dircon.co.uk

******************************************************************************/

extern int bbcb_keyscan(void);

extern void init_machine_bbca(void);
extern void init_machine_bbcb(void);
extern void stop_machine_bbcb(void);

int bbc_floppy_init(int);

void bbc_floppy_exit(int);
void check_disc_status(void);

READ_HANDLER ( bbc_wd1770_read);
WRITE_HANDLER ( bbc_wd1770_write );



READ_HANDLER(bbc_i8271_read);
WRITE_HANDLER(bbc_i8271_write);

