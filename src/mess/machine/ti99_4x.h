/*
  header file for /machine/ti99_4x.c
*/

/* variables */

extern unsigned char *ti99_scratch_RAM;
extern unsigned char *ti99_xRAM_low;
extern unsigned char *ti99_xRAM_high;

extern unsigned char *ti99_cart_mem;
extern unsigned char *ti99_DSR_mem;


/* protos for support code */

void ti99_init_machine(void);
void ti99_stop_machine(void);

int ti99_floppy_init(int id, const char *name);

int ti99_load_rom(int id, const char *name);
void ti99_rom_cleanup(int id);
int ti99_id_rom(const char *name, const char *gamename);

int ti99_vh_start(void);
int ti99_vblank_interrupt(void);

int ti99_rw_null8bits(int offset);
void ti99_ww_null8bits(int offset, int data);

int ti99_rw_xramlow(int offset);
void ti99_ww_xramlow(int offset, int data);
int ti99_rw_xramhigh(int offset);
void ti99_ww_xramhigh(int offset, int data);

int ti99_rw_cartmem(int offset);
void ti99_ww_cartmem(int offset, int data);

int ti99_rw_scratchpad(int offset);
void ti99_ww_scratchpad(int offset, int data);

void ti99_ww_wsnd(int offset, int data);
int ti99_rw_rvdp(int offset);
void ti99_ww_wvdp(int offset, int data);
int ti99_rw_rspeech(int offset);
void ti99_ww_wspeech(int offset, int data);
int ti99_rw_rgpl(int addr);
void ti99_ww_wgpl(int offset, int data);

int ti99_rw_disk(int offset);
void ti99_ww_disk(int offset, int data);
int ti99_DSKget(int offset);
void ti99_DSKROM(int offset, int data);
void ti99_DSKhold(int offset, int data);
void ti99_DSKheads(int offset, int data);
void ti99_DSKsel(int offset, int data);
void ti99_DSKside(int offset, int data);



