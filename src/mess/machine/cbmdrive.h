/* 
 private area for the
 commodore cbm floppy drives vc1541 c1551
 synthetic simulation

 contains state machines and file system accesses

 */
#ifndef __CBMDRIVE_H_
#define __CBMDRIVE_H_

#if 0
#define IEC 1
#define SERIAL 2
#endif

/* data for one drive */
typedef struct {
  int interface;
  unsigned char cmdbuffer[32]; 
  int cmdpos;
#define OPEN 1
#define READING 2
#define WRITING 3
  int state;  /*0 nothing */
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
      int forme; /* i am selected */
      int last; /* last byte to be sent */
      int broadcast; /* sent to all */
      double time;
    } serial;
  } i;
#define D64_IMAGE 1
#define FILESYSTEM 2
  int drive;
  union {
    struct {
      /* for visualization */
      char filename[20];
    } fs;
    struct {
      unsigned char *image; /*d64 image */
      /*	int track, sector; */
      /*	int sectorbuffer[256]; */
      
      /* for visualization */
      const char *imagename;
      char filename[20];
    } d64;
  } d;
} CBM_Drive;

typedef struct {
  int count;
  CBM_Drive *drives[4];
  /* whole + computer + drives */
  int /*reset, request[6],*/ data[6], clock[6], atn[6];
} CBM_Serial;

extern CBM_Serial cbm_serial;

void cbm_drive_open_helper(void);
void c1551_state(CBM_Drive *c1551);
void vc1541_state(CBM_Drive *vc1541);

#if 0
void c1551_write_data(CBM_Drive* drive,int data);
int c1551_read_data(CBM_Drive* drive);
void c1551_write_handshake(CBM_Drive* drive,int data);
int c1551_read_handshake(CBM_Drive* drive);
int c1551_read_status(CBM_Drive* drive);
#endif

#endif

