/*
** msx.h : part of MSX1 emulation.
**
** By Sean Young 1999
*/

#define MSX_MAX_CARTS   (2)

typedef struct {
    int type,bank_mask,banks[4];
    UINT8 *mem;
    char *sramfile;
    int pacsram;
} MSX_CART;

typedef struct {
    int run; /* set after init_msx () */
    /* PSG */
    int psg_b,opll_active;
    /* memory */
    UINT8 *empty, *ram, ramp[4];
    /* memory status */
    MSX_CART cart[MSX_MAX_CARTS];
	/* printer */
	UINT8 prn_data, prn_strobe;
	/* mouse */
	UINT16 mouse[2];
    int mouse_stat[2];
	/* rtc */
	int rtc_latch;
	/* disk */
    UINT8 dsk_stat, *disk;
} MSX;

/* start/stop functions */
extern DRIVER_INIT( msx );
extern DRIVER_INIT( msx2 );
extern MACHINE_INIT( msx );
extern MACHINE_INIT( msx2 );
extern MACHINE_STOP( msx );
extern INTERRUPT_GEN( msx_interrupt );
extern INTERRUPT_GEN( msx2_interrupt );
extern NVRAM_HANDLER( msx2 );

DEVICE_LOAD( msx_cart );
DEVICE_UNLOAD( msx_cart );

void msx_vdp_interrupt (int);

/* I/O functions */
WRITE_HANDLER ( msx_printer_w );
READ_HANDLER ( msx_printer_r );
WRITE_HANDLER ( msx_psg_w );
WRITE_HANDLER ( msx_dsk_w );
READ_HANDLER ( msx_psg_r );
WRITE_HANDLER ( msx_psg_port_a_w );
READ_HANDLER ( msx_psg_port_a_r );
WRITE_HANDLER ( msx_psg_port_b_w );
READ_HANDLER ( msx_psg_port_b_r );
WRITE_HANDLER ( msx_fmpac_w );
READ_HANDLER ( msx_rtc_reg_r );
WRITE_HANDLER ( msx_rtc_reg_w );
WRITE_HANDLER ( msx_rtc_latch_w );
WRITE_HANDLER ( msx_mapper_w );
READ_HANDLER ( msx_mapper_r );

/* memory functions */
WRITE_HANDLER ( msx_writemem0 );
WRITE_HANDLER ( msx_writemem1 );
WRITE_HANDLER ( msx_writemem2 );
WRITE_HANDLER ( msx_writemem3 );

/* cassette functions */
DEVICE_LOAD( msx_cassette );

/* disk functions */
DEVICE_LOAD( msx_floppy );


