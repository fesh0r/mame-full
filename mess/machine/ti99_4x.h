/*
  header file for /machine/ti99_4x.c
*/

/* variables */

extern UINT16 *ti99_scratch_RAM;
extern UINT16 *ti99_xRAM_low;
extern UINT16 *ti99_xRAM_high;

/*extern UINT16 *ti99_cart_mem;*/
extern UINT16 *ti99_DSR_mem;


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

READ16_HANDLER ( ti99_rw_null8bits );
WRITE16_HANDLER ( ti99_ww_null8bits );

READ16_HANDLER ( ti99_rw_xramlow );
WRITE16_HANDLER ( ti99_ww_xramlow );
READ16_HANDLER ( ti99_rw_xramhigh );
WRITE16_HANDLER ( ti99_ww_xramhigh );

READ16_HANDLER ( ti99_rw_cartmem );
WRITE16_HANDLER ( ti99_ww_cartmem );

READ16_HANDLER ( ti99_rw_scratchpad );
WRITE16_HANDLER ( ti99_ww_scratchpad );

WRITE16_HANDLER( ti99_ww_wsnd );
READ16_HANDLER ( ti99_rw_rvdp );
WRITE16_HANDLER ( ti99_ww_wvdp );
READ16_HANDLER ( ti99_rw_rspeech );
WRITE16_HANDLER ( ti99_ww_wspeech );
READ16_HANDLER ( ti99_rw_rgpl );
WRITE16_HANDLER( ti99_ww_wgpl );

READ16_HANDLER ( ti99_rw_disk );
WRITE16_HANDLER ( ti99_ww_disk );

READ_HANDLER ( ti99_DSKget );
WRITE_HANDLER ( ti99_DSKROM );
WRITE_HANDLER ( ti99_DSKhold);
WRITE_HANDLER ( ti99_DSKheads );
WRITE_HANDLER ( ti99_DSKsel );
WRITE_HANDLER ( ti99_DSKside );



