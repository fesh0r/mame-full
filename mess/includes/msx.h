/*
** msx.h : part of MSX1 emulation.
**
** By Sean Young 1999
*/

#define MSX_MAX_ROMSIZE (512*1024)
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
    UINT8 *empty, *ram;
    /* memory status */
    MSX_CART cart[MSX_MAX_CARTS];
	/* printer */
	UINT8 prn_data, prn_strobe;
	/* mouse */
	UINT16 mouse[2];
    int mouse_stat[2];
} MSX;

/* start/stop functions */
void init_msx(void);
void msx_ch_reset (void);
void msx_ch_stop (void);
int msx_load_rom (int id);
int msx_id_rom (int id);
void msx_exit_rom (int id);
int msx_interrupt (void);

/* I/O functions */
WRITE_HANDLER ( msx_printer_w );
READ_HANDLER ( msx_printer_r );
WRITE_HANDLER ( msx_vdp_w );
READ_HANDLER ( msx_vdp_r );
WRITE_HANDLER ( msx_psg_w );
WRITE_HANDLER ( msx_dsk_w );
READ_HANDLER ( msx_psg_r );
WRITE_HANDLER ( msx_psg_port_a_w );
READ_HANDLER ( msx_psg_port_a_r );
WRITE_HANDLER ( msx_psg_port_b_w );
READ_HANDLER ( msx_psg_port_b_r );
WRITE_HANDLER ( msx_fmpac_w );

/* memory functions */
WRITE_HANDLER ( msx_writemem0 );
WRITE_HANDLER ( msx_writemem1 );
WRITE_HANDLER ( msx_writemem2 );
WRITE_HANDLER ( msx_writemem3 );

/* cassette functions */
int msx_cassette_init (int id);
void msx_cassette_exit (int id);

/* disk functions */
int msx_dsk_id (int id);
int msx_dsk_init (int id);
void msx_dsk_exit (int id);
int msx_dsk_seek (int id, int offset, int whence);
int msx_dsk_tell (int id);
int msx_dsk_output_chunk (int id, void *src, int chunks);
int msx_dsk_input_chunk (int id, void *dst, int chunks);
void msx_dsk_output (int id, int);
int msx_dsk_status (int id, int);
int msx_dsk_input (int id);

#define MSX_DSK_ERR_OFFLINE			(0x10000000)
#define MSX_DSK_ERR_SECTORNOTFOUND	(0x20000000)
#define MSX_DSK_ERR_WRITEPROTECTED 	(0x30000000)
#define MSX_DSK_ERR_MASK 			(0xf0000000)

