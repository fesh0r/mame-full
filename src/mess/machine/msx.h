/*
** msx.h : part of MSX1 emulation.
**
** By Sean Young 1999
*/

#define MSX_MAX_ROMSIZE (512*1024)
#define MSX_MAX_CARTS   (2)

typedef struct {
    int type,bank_mask,banks[4],scc_active;
    UINT8 *mem;
} MSX_CART;
 
typedef struct {
    /* PSG and PPI */
    int ppi_c, psg_b;
    /* memory */
    UINT8 *empty, *ram;
    /* memory status */
    int ppi_a;
    MSX_CART cart[MSX_MAX_CARTS];
} MSX;

/* start/stop functions */
void msx_init (void);
void msx_ch_reset (void);
void msx_ch_stop (void);
int msx_id_rom (const char *name, const char *gamename);
int msx_load_rom (void);

/* I/O functions */
void msx_ppi_w (int offset, int data);
int msx_ppi_r (int offset);
void msx_vdp_w (int offset, int data);
int msx_vdp_r (int offset);
void msx_psg_w (int offset, int data);
int msx_psg_r (int offset);
void msx_psg_port_a_w (int offset, int data);
int msx_psg_port_a_r (int offset);
void msx_psg_port_b_w (int offset, int data);
int msx_psg_port_b_r (int offset);

/* memory functions */
void msx_writemem0 (int offset, int data);
void msx_writemem1 (int offset, int data);
void msx_writemem2 (int offset, int data);
void msx_writemem3 (int offset, int data);

