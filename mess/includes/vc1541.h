/***************************************************************************

       commodore vc1541 floppy disk drive

***************************************************************************/
#include "machine/6522via.h"
#include "driver.h"

#ifdef PET_TEST_CODE
/* test with preliminary VC1541 emulation */
#define VC1541
/*#define CPU_SYNC */
#endif

extern const struct Memory_ReadAddress vc1541_readmem[];
extern const struct Memory_WriteAddress vc1541_writemem[];

extern const struct Memory_ReadAddress dolphin_readmem[];
extern const struct Memory_WriteAddress dolphin_writemem[];

extern const struct Memory_ReadAddress c1551_readmem[];
extern const struct Memory_WriteAddress c1551_writemem[];

typedef struct {
	int cpunr;
	int devicenr;
} VC1541_CONFIG;

int vc1541_init(mess_image *img, mame_file *fp, int open_mode);
void vc1541_exit(mess_image *img);

int vc1541_config(int id, int mode, VC1541_CONFIG*config);
void vc1541_reset(void);
void vc1541_drive_status(char *text, int size);

typedef struct {
	int cpunr;
} C1551_CONFIG;

int c1551_config(int id, int mode, C1551_CONFIG*config);
#define c1551_reset vc1541_reset

#define CONFIG_DEVICE_VC1541	\
	CONFIG_DEVICE_LEGACYX(IO_FLOPPY, 1, "d64\0", DEVICE_LOAD_RESETS_CPU, OSD_FOPEN_READ, NULL, NULL, vc1541_init, vc1541_exit,	\
		((int(*)(mess_image*,int,void*))vc1541_config), NULL)

#define CONFIG_DEVICE_C2031	CONFIG_DEVICE_VC1541

#define CONFIG_DEVICE_C1551		\
	CONFIG_DEVICE_LEGACYX(IO_FLOPPY, 1, "d64\0", DEVICE_LOAD_RESETS_CPU, OSD_FOPEN_READ, NULL, NULL, vc1541_init, vc1541_exit,	\
		((int(*)(mess_image*,int,void*))c1551_config), NULL)

#define CONFIG_DEVICE_C1571	CONFIG_DEVICE_VC1541

extern MACHINE_DRIVER_EXTERN( cpu_vc1540 );
extern MACHINE_DRIVER_EXTERN( cpu_vc1541 );
extern MACHINE_DRIVER_EXTERN( cpu_c2031 );
extern MACHINE_DRIVER_EXTERN( cpu_dolphin );
extern MACHINE_DRIVER_EXTERN( cpu_c1551 );
extern MACHINE_DRIVER_EXTERN( cpu_c1571 );

#define VC1540_ROM(cpu) \
	ROM_REGION(0x10000,cpu,0) \
	ROM_LOAD("325302.01",  0xc000, 0x2000, CRC(29ae9752)) \
	ROM_LOAD("325303.01",  0xe000, 0x2000, CRC(10b39158))

#define C2031_ROM(cpu) \
		ROM_REGION(0x10000,cpu,0) \
		ROM_LOAD("dos2031",  0xc000, 0x4000, CRC(21b80fdf))

#if 1
#define VC1541_ROM(cpu) \
	ROM_REGION(0x10000,cpu,0) \
	ROM_LOAD("325302.01",  0xc000, 0x2000, CRC(29ae9752)) \
	ROM_LOAD("901229.05",  0xe000, 0x2000, CRC(361c9f37))
#else
/* for this I have the documented rom listing in german */
#define VC1541_ROM(cpu) \
	ROM_REGION(0x10000,cpu,0) \
	ROM_LOAD("325302.01",  0xc000, 0x2000, CRC(29ae9752)) \
	ROM_LOAD("901229.03",  0xe000, 0x2000, CRC(9126e74a))
#endif

#define DOLPHIN_ROM(cpu) \
	ROM_REGION(0x10000,cpu,0) \
	ROM_LOAD("c1541.rom",  0xa000, 0x6000, CRC(bd8e42b2))

#define C1551_ROM(cpu) \
	ROM_REGION(0x10000,cpu,0) \
	ROM_LOAD("318008.01",  0xc000, 0x4000, CRC(6d16d024))

#define C1570_ROM(cpu) \
	ROM_REGION(0x10000,cpu,0) \
	ROM_LOAD("315090.01",  0x8000, 0x8000, CRC(5a0c7937))

#define C1571_ROM(cpu) \
	ROM_REGION(0x10000,cpu,0) \
	ROM_LOAD("310654.03",  0x8000, 0x8000, CRC(3889b8b8))

#if 0
	ROM_LOAD("dos2040",  0x?000, 0x2000, CRC(d04c1fbb))

	ROM_LOAD("dos3040",  0x?000, 0x3000, CRC(f4967a7f))

	ROM_LOAD("dos4040",  0x?000, 0x3000, CRC(40e0ebaa))

	ROM_LOAD("dos1001",  0xc000, 0x4000, CRC(87e6a94e))

	/* vc1541 drive hardware */
	ROM_LOAD("dos2031",  0xc000, 0x4000, CRC(21b80fdf))

	ROM_LOAD("1540-c000.325302-01.bin",  0xc000, 0x2000, CRC(29ae9752))
	ROM_LOAD("1540-e000.325303-01.bin",  0xe000, 0x2000, CRC(10b39158))

	ROM_LOAD("1541-e000.901229-01.bin",  0xe000, 0x2000, CRC(9a48d3f0))
	ROM_LOAD("1541-e000.901229-02.bin",  0xe000, 0x2000, CRC(b29bab75))
	ROM_LOAD("1541-e000.901229-03.bin",  0xe000, 0x2000, CRC(9126e74a))
	ROM_LOAD("1541-e000.901229-05.bin",  0xe000, 0x2000, CRC(361c9f37))

	ROM_LOAD("1541-II.251968-03.bin",  0xe000, 0x2000, CRC(899fa3c5))

	ROM_LOAD("1541C.251968-01.bin",  0xc000, 0x4000, CRC(1b3ca08d))
	ROM_LOAD("1541C.251968-02.bin",  0xc000, 0x4000, CRC(2d862d20))

	ROM_LOAD("dos1541.c0",  0xc000, 0x2000, CRC(5b84bcef))
	ROM_LOAD("dos1541.e0",  0xe000, 0x2000, CRC(2d8c1fde))
	 /* merged gives 0x899fa3c5 */

	 /* 0x29ae9752 and 0x361c9f37 merged */
	ROM_LOAD("vc1541",  0xc000, 0x4000, CRC(57224cde))

	 /* 0x29ae9752 and 0xb29bab75 merged */
	ROM_LOAD("vc1541",  0xc000, 0x4000, CRC(d3a5789c))

	/* dolphin vc1541 */
	ROM_LOAD("c1541.rom",  0xa000, 0x6000, CRC(bd8e42b2))

	ROM_LOAD("1551.318008-01.bin",  0xc000, 0x4000, CRC(6d16d024))

	/* bug fixes introduced bugs for 1541 mode
	 jiffydos to have fixed 1571 and working 1541 mode */
	ROM_LOAD("1570-rom.315090-01.bin",  0x8000, 0x8000, CRC(5a0c7937))
	ROM_LOAD("1571-rom.310654-03.bin",  0x8000, 0x8000, CRC(3889b8b8))
	ROM_LOAD("1571-rom.310654-05.bin",  0x8000, 0x8000, CRC(5755bae3))
	ROM_LOAD("1571cr-rom.318047-01.bin",  0x8000, 0x8000, CRC(f24efcc4))

	ROM_LOAD("1581-rom.318045-01.bin",  0x8000, 0x8000, CRC(113af078))
	ROM_LOAD("1581-rom.318045-02.bin",  0x8000, 0x8000, CRC(a9011b84))
	ROM_LOAD("1581-rom.beta.bin",  0x8000, 0x8000, CRC(ecc223cd))

	/* modified drive 0x2000-0x3ffe ram, 0x3fff 6529 */
	ROM_LOAD("1581rom5.bin",  0x8000, 0x8000, CRC(e08801d7))

	ROM_LOAD("",  0xc000, 0x4000, CRC())
#endif

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

void c1551x_0_write_data (int data);
int c1551x_0_read_data (void);
void c1551x_0_write_handshake (int data);
int c1551x_0_read_handshake (void);
int c1551x_0_read_status (void);
