/***************************************************************************

       commodore vc1541 floppy disk drive

***************************************************************************/
#include "mess/machine/6522via.h"
#include "driver.h"

/* test with preliminary VC1541 emulation */
//#define VC1541

extern struct MemoryReadAddress vc1541_readmem[];
extern struct MemoryWriteAddress vc1541_writemem[];

extern struct MemoryReadAddress dolphin_readmem[];
extern struct MemoryWriteAddress dolphin_writemem[];

extern struct MemoryReadAddress c1551_readmem[];
extern struct MemoryWriteAddress c1551_writemem[];

#define VC1541_CPU \
          {\
			CPU_M6502,\
			1000000,\
			vc1541_readmem,vc1541_writemem,\
			0,0,\
			0,0,\
       	  }

#define VC1540_CPU VC1541_CPU

#define DOLPHIN_CPU \
          {\
			CPU_M6502,\
			1000000,\
			dolphin_readmem,dolphin_writemem,\
			0,0,\
			0,0,\
       	  }

#define C1551_CPU \
          {\
			CPU_M6510,\
			1000000,\
			c1551_readmem,c1551_writemem,\
			0,0,\
			0,0,\
       	  }

#define VC1540_ROM(cpu) \
	ROM_REGION(0x10000,cpu) \
	ROM_LOAD("325302.01",  0xc000, 0x2000, 0x29ae9752) \
	ROM_LOAD("325303.01",  0xe000, 0x2000, 0x10b39158)

#define VC1541_ROM(cpu) \
	ROM_REGION(0x10000,cpu) \
	ROM_LOAD("325302.01",  0xc000, 0x2000, 0x29ae9752) \
	ROM_LOAD("901229.05",  0xe000, 0x2000, 0x361c9f37)

#define DOLPHIN_ROM(cpu) \
	ROM_REGION(0x10000,cpu) \
	ROM_LOAD("c1541.rom",  0xa000, 0x6000, 0xbd8e42b2)

#define C1551_ROM(cpu) \
	ROM_REGION(0x10000,cpu) \
	ROM_LOAD("1551.318008-01.bin",  0xc000, 0x4000, 0x6d16d024)

#if 0
	ROM_LOAD("1540-c000.325302-01.bin",  0xc000, 0x2000, 0x29ae9752)
	ROM_LOAD("1540-e000.325303-01.bin",  0xe000, 0x2000, 0x10b39158)

	ROM_LOAD("1541-e000.901229-01.bin",  0xe000, 0x2000, 0x9a48d3f0)
	ROM_LOAD("1541-e000.901229-02.bin",  0xe000, 0x2000, 0xb29bab75)
	ROM_LOAD("1541-e000.901229-03.bin",  0xe000, 0x2000, 0x9126e74a)
	ROM_LOAD("1541-e000.901229-05.bin",  0xe000, 0x2000, 0x361c9f37)

	ROM_LOAD("1541-II.251968-03.bin",  0xe000, 0x2000, 0x899fa3c5)

	ROM_LOAD("1541C.251968-01.bin",  0xc000, 0x4000, 0x1b3ca08d)
	ROM_LOAD("1541C.251968-02.bin",  0xc000, 0x4000, 0x2d862d20)

	 
	ROM_LOAD("dos1541.c0",  0xc000, 0x2000, 0x5b84bcef)
	ROM_LOAD("dos1541.e0",  0xe000, 0x2000, 0x2d8c1fde)
	 /* merged gives 0x899fa3c5 */

	 /* 0x29ae9752 and 0x361c9f37 merged */
	ROM_LOAD("vc1541",  0xc000, 0x4000, 0x57224cde)

	ROM_LOAD("",  0xc000, 0x4000, 0x)
#endif

void vc1541_driver_init(int cpunumber);
void vc1541_machine_init(void);
void vc1541_drive_status(char *text, int size);

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
