#ifndef __CBM_SERIAL_BUS_H_
#define __CBM_SERIAL_BUS_H_

#include "cbmdrive.h"

/* must be called before other functions */
void cbm_drive_open (void);
void cbm_drive_close (void);

#define CONFIG_DEVICE_FLOPPY_CBM \
	CONFIG_DEVICE_LEGACY(IO_FLOPPY, 2, "d64\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_READ, NULL, NULL, device_load_cbm_drive, NULL, NULL)

#define IEC 1
#define SERIAL 2
#define IEEE 3
void cbm_drive_0_config (int interface, int serialnr);
void cbm_drive_1_config (int interface, int serialnr);

/* open an d64 image */
DEVICE_LOAD(cbm_drive);

/* delivers status for displaying */
extern void cbm_drive_0_status (char *text, int size);
extern void cbm_drive_1_status (char *text, int size);

/* iec interface c16/c1551 */
void c1551_0_write_data (int data);
int c1551_0_read_data (void);
void c1551_0_write_handshake (int data);
int c1551_0_read_handshake (void);
int c1551_0_read_status (void);

void c1551_1_write_data (int data);
int c1551_1_read_data (void);
void c1551_1_write_handshake (int data);
int c1551_1_read_handshake (void);
int c1551_1_read_status (void);

/* serial bus vc20/c64/c16/vc1541 and some printer */
void cbm_serial_reset_write (int level);
int cbm_serial_atn_read (void);
void cbm_serial_atn_write (int level);
int cbm_serial_data_read (void);
void cbm_serial_data_write (int level);
int cbm_serial_clock_read (void);
void cbm_serial_clock_write (int level);
int cbm_serial_request_read (void);
void cbm_serial_request_write (int level);

/* private */
extern CBM_Drive cbm_drive[2];

#endif
