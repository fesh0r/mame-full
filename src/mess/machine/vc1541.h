/***************************************************************************

       commodore vc1541 floppy disk drive

***************************************************************************/
#include "mess/machine/6522via.h"
#include "driver.h"

// test with preliminary VC1541 emulation
//#define VC1541

extern struct MemoryReadAddress vc1541_readmem[];
extern struct MemoryWriteAddress vc1541_writemem[];

#define VC1541_CPU(cpu) \
		{\
			CPU_M6502,\
			1000000,\
			cpu,\
			vc1541_readmem,vc1541_writemem,\
			0,0,\
			0,0,\
		  },

#define VC1541_ROM(cpu) \
	ROM_REGIONX(0x10000,cpu) \
	ROM_LOAD("vc1541.c0",  0xc000, 0x2000, 0x29ae9752) \
	ROM_LOAD("vc1541.e0",  0xe000, 0x2000, 0x361c9f37)


//	ROM_LOAD("dos1541",  0xc000, 0x4000, 0x899fa3c5)

void vc1541_driver_init(void);
void vc1541_machine_init(void);

/* serial bus vc20/c64/c16/vc1541 and some printer */

#ifdef VC1541
#define cbm_serial_reset_write(level)   vc1541_serial_reset_write(0,level)
#define cbm_serial_atn_read()           vc1541_serial_atn_read(0)
#define cbm_serial_atn_write(level)     vc1541_serial_atn_write(0,level)
#define cbm_serial_data_read()          vc1541_serial_data_read(0)
#define cbm_serial_data_write(level)    vc1541_serial_data_write(0,level)
#define cbm_serial_clock_read()         vc1541_serial_clock_read(0)
#define cbm_serial_clock_write(level)   vc1541_serial_clock_write(0,level)
#define cbm_serial_request_read()       vc1541_serial_request_read(0)
#define cbm_serial_request_write(level) vc1541_serial_request_write(0,level)
#endif

void vc1541_serial_reset_write(int which,int level);
int vc1541_serial_atn_read(int which);
void vc1541_serial_atn_write(int which,int level);
int vc1541_serial_data_read(int which);
void vc1541_serial_data_write(int which,int level);
int vc1541_serial_clock_read(int which);
void vc1541_serial_clock_write(int which,int level);
int vc1541_serial_request_read(int which);
void vc1541_serial_request_write(int which,int level);
