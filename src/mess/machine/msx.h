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
} MSX;

/* start/stop functions */
void init_msx(void);
void msx_ch_reset (void);
void msx_ch_stop (void);
int msx_load_rom (int id);
int msx_id_rom (int id);
void msx_exit_rom (int id);

/* I/O functions */
void msx_printer_w (int offset, int data);
int msx_printer_r (int offset);
void msx_vdp_w (int offset, int data);
int msx_vdp_r (int offset);
void msx_psg_w (int offset, int data);
int msx_psg_r (int offset);
void msx_psg_port_a_w (int offset, int data);
int msx_psg_port_a_r (int offset);
void msx_psg_port_b_w (int offset, int data);
int msx_psg_port_b_r (int offset);
void msx_fmpac_w (int offset, int data);

/* memory functions */
void msx_writemem0 (int offset, int data);
void msx_writemem1 (int offset, int data);
void msx_writemem2 (int offset, int data);
void msx_writemem3 (int offset, int data);

/* cassette functions */
int msx_cassette_init (int id);
void msx_cassette_exit (int id);
