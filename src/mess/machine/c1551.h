#ifndef __C1551_H_
#define __C1551_H_

/* must be called before other functions */
void cbm_drive_open (void);
void cbm_drive_close (void);

#define IODEVICE_CBM_DRIVE \
{\
   IO_FLOPPY,          /* type */\
   2,				   /* count */\
   "d64\0",            /* G64 later *//*file extensions */\
   NULL,               /* private */\
   NULL,               /* id */\
   cbm_drive_attach_image,        /* init */\
   NULL,			   /* exit */\
   NULL,               /* info */\
   NULL,               /* open */\
   NULL,               /* close */\
   NULL,               /* status */\
   NULL,               /* seek */\
   NULL,			   /* tell */\
   NULL,               /* input */\
   NULL,               /* output */\
   NULL,               /* input_chunk */\
   NULL                /* output_chunk */\
}

#define IEC 1
#define SERIAL 2
void cbm_drive_0_config (int interface);
void cbm_drive_1_config (int interface);

/* open an d64 image */
int cbm_drive_attach_image (int id);

/* load *.prg files directy from filesystem (rom directory) */
int cbm_drive_attach_fs (int id);

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

#endif
