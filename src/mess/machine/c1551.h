#ifndef __C1551_H_
#define __C1551_H_

/* must be called before other functions */
void cbm_drive_init(void);

/* data for one drive */
typedef struct {
#define IEC 1
#define SERIAL 2
  int interface;
  int cmdbuffer[32], cmdpos;
#define OPEN 1
#define READING 2
#define WRITING 3
  int state;  //0 nothing
  unsigned char *buffer;int size;int pos;
  union {
    struct {
      int handshakein, handshakeout;
      int datain, dataout;
      int status;
      int state;
    } iec;
    struct {
      int device;
      int data, clock, atn;
      int state, value;
      int forme; // i am selected
      int last; // last byte to be sent
      int broadcast; // sent to all
      double time;
    } serial;
  } i;
#define D64_IMAGE 1
#define FILESYSTEM 2
  int drive;
  union {
    struct {
      // for visualization
      char filename[20];
    } fs;
    struct {
      unsigned char *image; //d64 image
      //	int track, sector;
      //	int sectorbuffer[256];
      
      // for visualization
      char *imagename;
      char filename[20];
    } d64;
  } d;
} CBM_Drive;

/* open an d64 image */
int c1551_d64_open(CBM_Drive *drive, char *imagename);
/* load *.prg files directy from filesystem (rom directory) */
int c1551_fs_open(CBM_Drive *drive);
/* open an d64 image */
int vc1541_d64_open(int devicenr, CBM_Drive *drive, char *imagename);
/* load *.prg files directy from filesystem (rom directory) */
int vc1541_fs_open(int devicenr, CBM_Drive *drive);
void cbm_drive_close(CBM_Drive *drive);

// delivers status for displaying
extern void cbm_drive_status(CBM_Drive *c1551,char *text, int size);

/* iec interface c16/c1551 */
void c1551_write_data(CBM_Drive *c1551, int data);
int c1551_read_data(CBM_Drive *c1551);
void c1551_write_handshake(CBM_Drive *c1551, int data);
int c1551_read_handshake(CBM_Drive *c1551);
int c1551_read_status(CBM_Drive *c1551);

/* serial bus vc20/c64/c16/vc1541 and some printer */
void cbm_serial_reset_write(int level);
int cbm_serial_atn_read(void);
void cbm_serial_atn_write(int level);
int cbm_serial_data_read(void);
void cbm_serial_data_write(int level);
int cbm_serial_clock_read(void);
void cbm_serial_clock_write(int level);
int cbm_serial_request_read(void);
void cbm_serial_request_write(int level);

#endif

