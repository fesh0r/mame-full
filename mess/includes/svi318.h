/*
** Spectravideo SVI-318 and SVI-328
*/

/* disk emulation doesn't work yet! */
#define SVI_DISK */

typedef struct {
	/* general */
	int svi318;
	/* memory */
	UINT8 *banks[2][4], *empty_bank, bank_switch, bank1, bank2;
	/* printer */
	UINT8 prn_data, prn_strobe;
} SVI_318;

extern DRIVER_INIT( svi318 );
extern MACHINE_INIT( svi318 );
extern MACHINE_STOP( svi318 );

int svi318_load_rom (int id, void *fp, int open_mode);
void svi318_exit_rom (int id);
extern INTERRUPT_GEN( svi318_interrupt );
extern void svi318_vdp_interrupt (int i);

WRITE_HANDLER (svi318_writemem0);
WRITE_HANDLER (svi318_writemem1);

READ_HANDLER (svi318_printer_r);
WRITE_HANDLER (svi318_printer_w);

READ_HANDLER (svi318_ppi_r);
WRITE_HANDLER (svi318_ppi_w);

WRITE_HANDLER (svi318_psg_port_b_w);
READ_HANDLER (svi318_psg_port_a_r);

/* cassette functions */
int svi318_cassette_init (int id, void *fp, int open_mode);
int svi318_cassette_present (int id);

/* floppy functions */
#ifdef SVI_DISK
WRITE_HANDLER (fdc_disk_motor_w);
WRITE_HANDLER (fdc_density_side_w);
READ_HANDLER (svi318_fdc_status_r);
#endif

int svi318_floppy_init(int id, void *fp, int open_mode);

