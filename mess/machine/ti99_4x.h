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

int ti99_floppy_init(int id);

int ti99_cassette_init(int id);
void ti99_cassette_exit(int id);

int ti99_load_rom(int id);
void ti99_rom_cleanup(int id);
int ti99_id_rom(int id);

int ti99_4_vh_start(void);
int ti99_4a_vh_start(void);
int ti99_vblank_interrupt(void);

READ_HANDLER ( ti99_rw_null8bits );
WRITE_HANDLER ( ti99_ww_null8bits );

READ_HANDLER ( ti99_rw_xramlow );
WRITE_HANDLER ( ti99_ww_xramlow );
READ_HANDLER ( ti99_rw_xramhigh );
WRITE_HANDLER ( ti99_ww_xramhigh );

READ_HANDLER ( ti99_rw_cartmem );
WRITE_HANDLER ( ti99_ww_cartmem );

READ_HANDLER ( ti99_rw_scratchpad );
WRITE_HANDLER ( ti99_ww_scratchpad );

WRITE_HANDLER( ti99_ww_wsnd );
READ_HANDLER ( ti99_rw_rvdp );
WRITE_HANDLER ( ti99_ww_wvdp );
READ_HANDLER ( ti99_rw_rspeech );
WRITE_HANDLER ( ti99_ww_wspeech );
READ_HANDLER ( ti99_rw_rgpl );
WRITE_HANDLER( ti99_ww_wgpl );

READ_HANDLER ( ti99_rw_disk );
WRITE_HANDLER ( ti99_ww_disk );
READ_HANDLER ( ti99_DSKget );
WRITE_HANDLER ( ti99_DSKROM );
WRITE_HANDLER ( ti99_DSKhold);
WRITE_HANDLER ( ti99_DSKheads );
WRITE_HANDLER ( ti99_DSKsel );
WRITE_HANDLER ( ti99_DSKside );



