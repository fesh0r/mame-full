/***************************************************************************
        Spectrum/Inves/TK90X etc. memory map:

	CPU:
		0000-3fff ROM
		4000-ffff RAM

        Spectrum 128/+2/+2a/+3 memory map:

        CPU:
                0000-3fff Banked ROM/RAM (banked rom only on 128/+2)
                4000-7fff Banked RAM
                8000-bfff Banked RAM
                c000-ffff Banked RAM

        TS2068 memory map: (Can't have both EXROM and DOCK active)
        The 8K EXROM can be loaded into multiple pages.

	CPU:
                0000-1fff     ROM / EXROM / DOCK (Cartridge)
                2000-3fff     ROM / EXROM / DOCK
                4000-5fff \
                6000-7fff  \
                8000-9fff  |- RAM / EXROM / DOCK
                a000-bfff  |
                c000-dfff  /
                e000-ffff /


Interrupts:

Changes:

29/1/2000       KT - Implemented initial +3 emulation
30/1/2000       KT - Improved input port decoding for reading
                     and therefore correct keyboard handling for Spectrum and +3
31/1/2000       KT - Implemented buzzer sound for Spectrum and +3.
                     Implementation copied from Paul Daniel's Jupiter driver.
                     Fixed screen display problems with dirty chars.
                     Added support to load .Z80 snapshots. 48k support so far.
13/2/2000       KT - Added Interface II, Kempston, Fuller and Mikrogen joystick support
17/2/2000       DJR - Added full key descriptions and Spectrum+ keys.
                Fixed Spectrum +3 keyboard problems.
17/2/2000       KT - Added tape loading from WAV/Changed from DAC to generic speaker code
18/2/2000       KT - Added tape saving to WAV
27/2/2000       KT - Took DJR's changes and added my changes.
27/2/2000       KT - Added disk image support to Spectrum +3 driver.
27/2/2000       KT - Added joystick I/O code to the Spectrum +3 I/O handler.
14/3/2000       DJR - Tape handling dipswitch.
26/3/2000       DJR - Snapshot files are now classifed as snapshots not cartridges.
04/4/2000       DJR - Spectrum 128 / +2 Support.
13/4/2000       DJR - +4 Support (unofficial 48K hack).
13/4/2000       DJR - +2a Support (rom also used in +3 models).
13/4/2000       DJR - TK90X, TK95 and Inves support (48K clones).
21/4/2000       DJR - TS2068 and TC2048 support (TC2048 Supports extra video
                modes but doesn't have bank switching or sound chip).
09/5/2000       DJR - Spectrum +2 (France, Spain), +3 (Spain).
17/5/2000       DJR - Dipswitch to enable/disable disk drives on +3 and clones.
27/6/2000       DJR - Changed 128K/+3 port decoding (sound now works in Zub 128K).
06/8/2000       DJR - Fixed +3 Floppy support

Notes:

128K emulation is not perfect - the 128K machines crash and hang while
running quite a lot of games.
The TK90X and TK95 roms output 0 to port #df on start up.
The purpose of this port is unknown (probably display mode as TS2068) and
thus is not emulated.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/spectrum.h"
#include "eventlst.h"

/* +3 hardware */
#include "includes/nec765.h"
#include "cpuintrf.h"
#include "includes/dsk.h"

/*-----------------27/02/00 10:42-------------------
 bit 7-5: not used
 bit 4: Ear output/Speaker
 bit 3: MIC/Tape Output
 bit 2-0: border colour
--------------------------------------------------*/

int PreviousFE = 0;

void spectrum_port_fe_w (int offset,int data)
{
	unsigned char Changed;

	Changed = PreviousFE^data;

	/* border colour changed? */
	if ((Changed & 0x07)!=0)
	{
		/* yes - send event */
                EventList_AddItemOffset(0x0fe, data & 0x07, cpu_getcurrentcycles());
	}

	if ((Changed & (1<<4))!=0)
	{
		/* DAC output state */
        speaker_level_w(0,(data>>4) & 0x01);
	}

	if ((Changed & (1<<3))!=0)
	{
        /*-----------------27/02/00 10:41-------------------
         write cassette data
        --------------------------------------------------*/
        device_output(IO_CASSETTE, 0, (data & (1<<3)) ? -32768: 32767);
	}

	PreviousFE = data;
}



extern void spectrum_128_update_memory(void);
extern void spectrum_plus3_update_memory(void);
extern void ts2068_update_memory(void);

int spectrum_128_port_7ffd_data = -1;
int spectrum_plus3_port_1ffd_data = -1;
/* Initialisation values used when determining which model is being emulated.
   48K     Spectrum doesn't use either port.
   128K/+2 Bank switches with port 7ffd only.
   +3/+2a  Bank switches with both ports. */

static unsigned char *spectrum_128_ram = NULL;
unsigned char *spectrum_128_screen_location = NULL;

int ts2068_port_ff_data = -1; /* Display enhancement control */
int ts2068_port_f4_data = -1; /* Horizontal Select Register */
unsigned char *ts2068_ram = NULL;


static nec765_interface spectrum_plus3_nec765_interface =
{
        NULL,
        NULL
};



void spectrum_128_init_machine(void)
{
        spectrum_128_ram = (unsigned char *)malloc(128*1024);
        if(!spectrum_128_ram) return;
        memset(spectrum_128_ram, 0, 128*1024);

        cpu_setbankhandler_r(1, MRA_BANK1);
        cpu_setbankhandler_r(2, MRA_BANK2);
        cpu_setbankhandler_r(3, MRA_BANK3);
        cpu_setbankhandler_r(4, MRA_BANK4);

        /* 0x0000-0x3fff always holds ROM */
        cpu_setbankhandler_w(5, MWA_ROM);
        cpu_setbankhandler_w(6, MWA_BANK6);
        cpu_setbankhandler_w(7, MWA_BANK7);
        cpu_setbankhandler_w(8, MWA_BANK8);


        /* Bank 5 is always in 0x4000 - 0x7fff */
        cpu_setbank(2, spectrum_128_ram + (5<<14));
        cpu_setbank(6, spectrum_128_ram + (5<<14));

        /* Bank 2 is always in 0x8000 - 0xbfff */
        cpu_setbank(3, spectrum_128_ram + (2<<14));
        cpu_setbank(7, spectrum_128_ram + (2<<14));

        /* set initial ram config */
        spectrum_128_port_7ffd_data = 0;
        spectrum_128_update_memory();

        spectrum_init_machine();
}


void spectrum_plus3_init_machine(void)
{
        spectrum_128_ram = (unsigned char *)malloc(128*1024);
        if(!spectrum_128_ram) return;
        memset(spectrum_128_ram, 0, 128*1024);

        cpu_setbankhandler_r(1, MRA_BANK1);
        cpu_setbankhandler_r(2, MRA_BANK2);
        cpu_setbankhandler_r(3, MRA_BANK3);
        cpu_setbankhandler_r(4, MRA_BANK4);

        cpu_setbankhandler_w(5, MWA_BANK5);
        cpu_setbankhandler_w(6, MWA_BANK6);
        cpu_setbankhandler_w(7, MWA_BANK7);
        cpu_setbankhandler_w(8, MWA_BANK8);

        nec765_init(&spectrum_plus3_nec765_interface, NEC765A);

		floppy_drive_set_geometry(0, FLOPPY_DRIVE_SS_40);
		floppy_drive_set_geometry(1, FLOPPY_DRIVE_SS_40);
        floppy_drive_set_flag_state(0, FLOPPY_DRIVE_PRESENT, 1);
        floppy_drive_set_flag_state(1, FLOPPY_DRIVE_PRESENT, 1);

        /* Initial configuration */
        spectrum_128_port_7ffd_data = 0;
        spectrum_plus3_port_1ffd_data = 0;
        spectrum_plus3_update_memory();

        spectrum_init_machine();
}

void  spectrum_128_exit_machine(void)
{
        if (spectrum_128_ram!=NULL)
                free(spectrum_128_ram);
}


void ts2068_init_machine(void)
{
        ts2068_ram = (unsigned char *)malloc(48*1024);
        if(!ts2068_ram) return;
        memset(ts2068_ram, 0, 48*1024);

        cpu_setbankhandler_r(1, MRA_BANK1);
        cpu_setbankhandler_r(2, MRA_BANK2);
        cpu_setbankhandler_r(3, MRA_BANK3);
        cpu_setbankhandler_r(4, MRA_BANK4);
        cpu_setbankhandler_r(5, MRA_BANK5);
        cpu_setbankhandler_r(6, MRA_BANK6);
        cpu_setbankhandler_r(7, MRA_BANK7);
        cpu_setbankhandler_r(8, MRA_BANK8);

        /* 0x0000-0x3fff always holds ROM */
        cpu_setbankhandler_w(9, MWA_ROM);
        cpu_setbankhandler_w(10, MWA_ROM);
        cpu_setbankhandler_w(11, MWA_BANK11);
        cpu_setbankhandler_w(12, MWA_BANK12);
        cpu_setbankhandler_w(13, MWA_BANK13);
        cpu_setbankhandler_w(14, MWA_BANK14);
        cpu_setbankhandler_w(15, MWA_BANK15);
        cpu_setbankhandler_w(16, MWA_BANK16);

        ts2068_port_ff_data = 0;
        ts2068_port_f4_data = 0;
        ts2068_update_memory();

        spectrum_init_machine();
}

void tc2048_init_machine(void)
{
        ts2068_ram = (unsigned char *)malloc(48*1024);
        if(!ts2068_ram) return;
        memset(ts2068_ram, 0, 48*1024);

        cpu_setbankhandler_r(1, MRA_BANK1);
        cpu_setbankhandler_w(2, MWA_BANK2);
        cpu_setbank(1, ts2068_ram);
        cpu_setbank(2, ts2068_ram);
        ts2068_port_ff_data = 0;

        spectrum_init_machine();
}


void ts2068_exit_machine(void)
{
        if (ts2068_ram!=NULL)
                free(ts2068_ram);
}


static void spectrum_128_port_bffd_w(int offset, int data)
{
        AY8910_write_port_0_w(0, data);
}

static void spectrum_plus3_port_3ffd_w(int offset, int data)
{
        if (~readinputport(16) & 0x20)
                nec765_data_w(0,data);
}

static int spectrum_plus3_port_3ffd_r(int offset)
{
        if (readinputport(16) & 0x20)
                return 0xff;
        else
                return nec765_data_r(0);
}


static int spectrum_plus3_port_2ffd_r(int offset)
{
        if (readinputport(16) & 0x20)
                return 0xff;
        else
                return nec765_status_r(0);
}

static void spectrum_128_port_fffd_w(int offset, int data)
{
        AY8910_control_port_0_w(0, data);
}

/* +3 manual is confused about this */

static int spectrum_128_port_fffd_r(int offset)
{
        return AY8910_read_port_0_r(0);
}


static int ts2068_port_f4_r(int offset)
{
        return ts2068_port_f4_data;
}

static void ts2068_port_f4_w(int offset, int data)
{
        ts2068_port_f4_data = data;
        ts2068_update_memory();
}

static void ts2068_port_f5_w(int offset, int data)
{
        AY8910_write_port_0_w(0, data);
}

static int ts2068_port_f6_r(int offset)
{
        /* TODO - Reading from register 14 reads the joystick ports
           set bit 8 of address to read joystick #1
           set bit 9 of address to read joystick #2
           if both bits are set then OR values
           Bit 0 up, 1 down, 2 left, 3 right, 7 fire active low. Other bits 1
        */
        return AY8910_read_port_0_r(0);
}

static void ts2068_port_f6_w(int offset, int data)
{
        AY8910_control_port_0_w(0, data);
}

static int ts2068_port_ff_r(int offset)
{
        return ts2068_port_ff_data;
}

static void ts2068_port_ff_w(int offset, int data)
{
        /* Bits 0-2 Video Mode Select
           Bits 3-5 64 column mode ink/paper selection
                    (See ts2068_vh_screenrefresh for more info)
           Bit  6   17ms Interrupt Inhibit
           Bit  7   Cartridge (0) / EXROM (1) select
        */
        ts2068_port_ff_data = data;
        ts2068_update_memory();
        logerror("Port %04x write %02x\n", offset, data);
}

static void tc2048_port_ff_w(int offset, int data)
{
        ts2068_port_ff_data = data;
        logerror("Port %04x write %02x\n", offset, data);
}

static int spectrum_plus3_memory_selections[]=
{
        0,1,2,3,
        4,5,6,7,
        4,5,6,3,
        4,7,6,3
};

extern void spectrum_128_update_memory(void)
{
        unsigned char *ChosenROM;
        int ROMSelection;

        if (spectrum_128_port_7ffd_data & 8)
        {
                logerror("SCREEN 1: BLOCK 7\n");
                spectrum_128_screen_location = spectrum_128_ram + (7<<14);
        }
        else
        {
                logerror("SCREEN 0: BLOCK 5\n");
                spectrum_128_screen_location = spectrum_128_ram + (5<<14);
        }

        /* select ram at 0x0c000-0x0ffff */
        {
                int ram_page;
                unsigned char *ram_data;

                ram_page = spectrum_128_port_7ffd_data & 0x07;
                ram_data = spectrum_128_ram + (ram_page<<14);

                cpu_setbank(4, ram_data);
                cpu_setbank(8, ram_data);

                logerror("RAM at 0xc000: %02x\n",ram_page);
        }

        /* ROM switching */
        ROMSelection = ((spectrum_128_port_7ffd_data>>4) & 0x01);

        /* rom 1 is 128K rom, rom 1 is 48 BASIC */

        ChosenROM = memory_region(REGION_CPU1) + 0x010000 + (ROMSelection<<14);

        cpu_setbank(1, ChosenROM);

        logerror("rom switch: %02x\n", ROMSelection);
}


extern void spectrum_plus3_update_memory(void)
{
        if (spectrum_128_port_7ffd_data & 8)
        {
                logerror("+3 SCREEN 1: BLOCK 7\n");
                spectrum_128_screen_location = spectrum_128_ram + (7<<14);
        }
        else
        {
                logerror("+3 SCREEN 0: BLOCK 5\n");
                spectrum_128_screen_location = spectrum_128_ram + (5<<14);
        }

        if ((spectrum_plus3_port_1ffd_data & 0x01)==0)
        {
                int ram_page;
                unsigned char *ram_data;

                /* ROM switching */
                unsigned char *ChosenROM;
                int ROMSelection;

                /* select ram at 0x0c000-0x0ffff */
                ram_page = spectrum_128_port_7ffd_data & 0x07;
                ram_data = spectrum_128_ram + (ram_page<<14);

                cpu_setbank(4, ram_data);
                cpu_setbank(8, ram_data);

                logerror("RAM at 0xc000: %02x\n",ram_page);

                /* Reset memory between 0x4000 - 0xbfff in case extended paging was being used */
                /* Bank 5 in 0x4000 - 0x7fff */
                cpu_setbank(2, spectrum_128_ram + (5<<14));
                cpu_setbank(6, spectrum_128_ram + (5<<14));

                /* Bank 2 in 0x8000 - 0xbfff */
                cpu_setbank(3, spectrum_128_ram + (2<<14));
                cpu_setbank(7, spectrum_128_ram + (2<<14));


                ROMSelection = ((spectrum_128_port_7ffd_data>>4) & 0x01) |
                    ((spectrum_plus3_port_1ffd_data>>1) & 0x02);

                /* rom 0 is editor, rom 1 is syntax, rom 2 is DOS, rom 3 is 48 BASIC */

                ChosenROM = memory_region(REGION_CPU1) + 0x010000 + (ROMSelection<<14);

                cpu_setbank(1, ChosenROM);
                cpu_setbankhandler_w(5, MWA_ROM);

                logerror("rom switch: %02x\n", ROMSelection);
        }
        else
        {
                /* Extended memory paging */

                int *memory_selection;
                int MemorySelection;
                unsigned char *ram_data;

                MemorySelection = (spectrum_plus3_port_1ffd_data>>1) & 0x03;

                memory_selection = &spectrum_plus3_memory_selections[(MemorySelection<<2)];

                ram_data = spectrum_128_ram + (memory_selection[0]<<14);
                cpu_setbank(1, ram_data);
                cpu_setbank(5, ram_data);
                /* allow writes to 0x0000-0x03fff */
                cpu_setbankhandler_w(5, MWA_BANK5);

                ram_data = spectrum_128_ram + (memory_selection[1]<<14);
                cpu_setbank(2, ram_data);
                cpu_setbank(6, ram_data);

                ram_data = spectrum_128_ram + (memory_selection[2]<<14);
                cpu_setbank(3, ram_data);
                cpu_setbank(7, ram_data);

                ram_data = spectrum_128_ram + (memory_selection[3]<<14);
                cpu_setbank(4, ram_data);
                cpu_setbank(8, ram_data);

                logerror("extended memory paging: %02x\n",MemorySelection);
         }
}


/*******************************************************************
 *
 *      Bank switch between the 3 internal memory banks HOME, EXROM
 *      and DOCK (Cartridges). The HOME bank contains 16K ROM in the
 *      0-16K area and 48K RAM fills the rest. The EXROM contains 8K
 *      ROM and can appear in every 8K segment (ie 0-8K, 8-16K etc).
 *      The DOCK is empty and is meant to be occupied by cartridges
 *      you can plug into the cartridge dock of the 2068.
 *
 *      The address space is divided into 8 8K chunks. Bit 0 of port
 *      #f4 corresponds to the 0-8K chunk, bit 1 to the 8-16K chunk
 *      etc. If the bit is 0 then the chunk is controlled by the HOME
 *      bank. If the bit is 1 then the chunk is controlled by either
 *      the DOCK or EXROM depending on bit 7 of port #ff. Note this
 *      means that that the Z80 can't see chunks of the EXROM and DOCK
 *      at the same time.
 *
 *******************************************************************/
extern void ts2068_update_memory(void)
{
        unsigned char *ChosenROM, *ExROM;

        ExROM = memory_region(REGION_CPU1) + 0x014000;
        if (ts2068_port_f4_data & 0x01)
        {
                if (ts2068_port_ff_data & 0x80)
                {
                        cpu_setbank(1, ExROM);
                        cpu_setbankhandler_r(1, MRA_BANK1);
                        logerror("0000-1fff EXROM\n");
                }
                else
                {
                        /* Cartridges not implemented so assume absent */
                        cpu_setbankhandler_r(1, MRA_NOP);
                        logerror("0000-1fff Cartridge\n");
                }
        }
        else
        {
                ChosenROM = memory_region(REGION_CPU1) + 0x010000;
                cpu_setbank(1, ChosenROM);
                cpu_setbankhandler_r(1, MRA_BANK1);
                logerror("0000-1fff HOME\n");
        }

        if (ts2068_port_f4_data & 0x02)
        {
                if (ts2068_port_ff_data & 0x80)
                {
                        cpu_setbank(2, ExROM);
                        cpu_setbankhandler_r(2, MRA_BANK2);
                        logerror("2000-3fff EXROM\n");
                }
                else
                {
                        cpu_setbankhandler_r(2, MRA_NOP);
                        logerror("2000-3fff Cartridge\n");
                }
        }
        else
        {
                ChosenROM = memory_region(REGION_CPU1) + 0x012000;
                cpu_setbank(2, ChosenROM);
                cpu_setbankhandler_r(2, MRA_BANK2);
                logerror("2000-3fff HOME\n");
        }

        if (ts2068_port_f4_data & 0x04)
        {
                if (ts2068_port_ff_data & 0x80)
                {
                        cpu_setbank(3, ExROM);
                        cpu_setbankhandler_r(3, MRA_BANK3);
                        cpu_setbankhandler_w(11, MWA_ROM);
                        logerror("4000-5fff EXROM\n");
                }
                else
                {
                        cpu_setbankhandler_r(3, MRA_NOP);
                        cpu_setbankhandler_w(11, MWA_ROM);
                        logerror("4000-5fff Cartridge\n");
                }
        }
        else
        {
                cpu_setbank(3, ts2068_ram);
                cpu_setbank(11, ts2068_ram);
                cpu_setbankhandler_r(3, MRA_BANK3);
                cpu_setbankhandler_w(11, MWA_BANK11);
                logerror("4000-5fff RAM\n");
        }

        if (ts2068_port_f4_data & 0x08)
        {
                if (ts2068_port_ff_data & 0x80)
                {
                        cpu_setbank(4, ExROM);
                        cpu_setbankhandler_r(4, MRA_BANK4);
                        cpu_setbankhandler_w(12, MWA_ROM);
                        logerror("6000-7fff EXROM\n");
                }
                else
                {
                        cpu_setbankhandler_r(4, MRA_NOP);
                        cpu_setbankhandler_w(12, MWA_ROM);
                        logerror("6000-7fff Cartridge\n");
                }
        }
        else
        {
                cpu_setbank(4, ts2068_ram + 0x2000);
                cpu_setbank(12, ts2068_ram + 0x2000);
                cpu_setbankhandler_r(4, MRA_BANK4);
                cpu_setbankhandler_w(12, MWA_BANK12);
                logerror("6000-7fff RAM\n");
        }

        if (ts2068_port_f4_data & 0x10)
        {
                if (ts2068_port_ff_data & 0x80)
                {
                        cpu_setbank(5, ExROM);
                        cpu_setbankhandler_r(5, MRA_BANK5);
                        cpu_setbankhandler_w(13, MWA_ROM);
                        logerror("8000-9fff EXROM\n");
                }
                else
                {
                        cpu_setbankhandler_r(5, MRA_NOP);
                        cpu_setbankhandler_w(13, MWA_ROM);
                        logerror("8000-9fff Cartridge\n");
                }
        }
        else
        {
                cpu_setbank(5, ts2068_ram + 0x4000);
                cpu_setbank(13, ts2068_ram + 0x4000);
                cpu_setbankhandler_r(5, MRA_BANK5);
                cpu_setbankhandler_w(13, MWA_BANK13);
                logerror("8000-9fff RAM\n");
        }

        if (ts2068_port_f4_data & 0x20)
        {
                if (ts2068_port_ff_data & 0x80)
                {
                        cpu_setbank(6, ExROM);
                        cpu_setbankhandler_r(6, MRA_BANK6);
                        cpu_setbankhandler_w(14, MWA_ROM);
                        logerror("a000-bfff EXROM\n");
                }
                else
                {
                        cpu_setbankhandler_r(6, MRA_NOP);
                        cpu_setbankhandler_w(14, MWA_ROM);
                        logerror("a000-bfff Cartridge\n");
                }
        }
        else
        {
                cpu_setbank(6, ts2068_ram + 0x6000);
                cpu_setbank(14, ts2068_ram + 0x6000);
                cpu_setbankhandler_r(6, MRA_BANK6);
                cpu_setbankhandler_w(14, MWA_BANK14);
                logerror("a000-bfff RAM\n");
        }

        if (ts2068_port_f4_data & 0x40)
        {
                if (ts2068_port_ff_data & 0x80)
                {
                        cpu_setbank(7, ExROM);
                        cpu_setbankhandler_r(7, MRA_BANK7);
                        cpu_setbankhandler_w(15, MWA_ROM);
                        logerror("c000-dfff EXROM\n");
                }
                else
                {
                        cpu_setbankhandler_r(7, MRA_NOP);
                        cpu_setbankhandler_w(15, MWA_ROM);
                        logerror("c000-dfff Cartridge\n");
                }
        }
        else
        {
                cpu_setbank(7, ts2068_ram + 0x8000);
                cpu_setbank(15, ts2068_ram + 0x8000);
                cpu_setbankhandler_r(7, MRA_BANK7);
                cpu_setbankhandler_w(15, MWA_BANK15);
                logerror("c000-dfff RAM\n");
        }

        if (ts2068_port_f4_data & 0x80)
        {
                if (ts2068_port_ff_data & 0x80)
                {
                        cpu_setbank(8, ExROM);
                        cpu_setbankhandler_r(8, MRA_BANK8);
                        cpu_setbankhandler_w(16, MWA_ROM);
                        logerror("e000-ffff EXROM\n");
                }
                else
                {
                        cpu_setbankhandler_r(8, MRA_NOP);
                        cpu_setbankhandler_w(16, MWA_ROM);
                        logerror("e000-ffff Cartridge\n");
                }
        }
        else
        {
                cpu_setbank(8, ts2068_ram + 0xa000);
                cpu_setbank(16, ts2068_ram + 0xa000);
                cpu_setbankhandler_r(8, MRA_BANK8);
                cpu_setbankhandler_w(16, MWA_BANK16);
                logerror("e000-ffff RAM\n");
        }
}

static void spectrum_128_port_7ffd_w(int offset, int data)
{
       /* D0-D2: RAM page located at 0x0c000-0x0ffff */
       /* D3 - Screen select (screen 0 in ram page 5, screen 1 in ram page 7 */
       /* D4 - ROM select - which rom paged into 0x0000-0x03fff */
       /* D5 - Disable paging */

        /* disable paging? */
        if (spectrum_128_port_7ffd_data & 0x20)
                return;

        /* store new state */
        spectrum_128_port_7ffd_data = data;

        /* update memory */
        spectrum_128_update_memory();
}

static void spectrum_plus3_port_7ffd_w(int offset, int data)
{
       /* D0-D2: RAM page located at 0x0c000-0x0ffff */
       /* D3 - Screen select (screen 0 in ram page 5, screen 1 in ram page 7 */
       /* D4 - ROM select - which rom paged into 0x0000-0x03fff */
       /* D5 - Disable paging */

        /* disable paging? */
        if (spectrum_128_port_7ffd_data & 0x20)
                return;

        /* store new state */
        spectrum_128_port_7ffd_data = data;

        /* update memory */
        spectrum_plus3_update_memory();
}

static void spectrum_plus3_port_1ffd_w(int offset, int data)
{

        /* D0-D1: ROM/RAM paging */
        /* D2: Affects if d0-d1 work on ram/rom */
        /* D3 - Disk motor on/off */
        /* D4 - parallel port strobe */

        floppy_drive_set_motor_state(0, data & (1<<3));
        floppy_drive_set_motor_state(1, data & (1<<3));
        floppy_drive_set_ready_state(0, 1, 1);
        floppy_drive_set_ready_state(1, 1, 1);

        spectrum_plus3_port_1ffd_data = data;

        /* disable paging? */
        if ((spectrum_128_port_7ffd_data & 0x20)==0)
        {
                /* no */
                spectrum_plus3_update_memory();
        }
}



static struct MemoryReadAddress spectrum_readmem[] = {
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x57ff, spectrum_characterram_r },
	{ 0x5800, 0x5aff, spectrum_colorram_r },
	{ 0x5b00, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress spectrum_writemem[] = {
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x57ff, spectrum_characterram_w },
	{ 0x5800, 0x5aff, spectrum_colorram_w },
	{ 0x5b00, 0xffff, MWA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress spectrum_128_readmem[] = {
        { 0x0000, 0x3fff, MRA_BANK1 },
        { 0x4000, 0x7fff, MRA_BANK2 },
        { 0x8000, 0xbfff, MRA_BANK3 },
        { 0xc000, 0xffff, MRA_BANK4 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress spectrum_128_writemem[] = {
        { 0x0000, 0x3fff, MWA_BANK5 },
        { 0x4000, 0x7fff, MWA_BANK6 },
        { 0x8000, 0xbfff, MWA_BANK7 },
        { 0xc000, 0xffff, MWA_BANK8 },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress ts2068_readmem[] = {
        { 0x0000, 0x1fff, MRA_BANK1 },
        { 0x2000, 0x3fff, MRA_BANK2 },
        { 0x4000, 0x5fff, MRA_BANK3 },
        { 0x6000, 0x7fff, MRA_BANK4 },
        { 0x8000, 0x9fff, MRA_BANK5 },
        { 0xa000, 0xbfff, MRA_BANK6 },
        { 0xc000, 0xdfff, MRA_BANK7 },
        { 0xe000, 0xffff, MRA_BANK8 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress ts2068_writemem[] = {
        { 0x0000, 0x1fff, MWA_BANK9 },
        { 0x2000, 0x3fff, MWA_BANK10 },
        { 0x4000, 0x5fff, MWA_BANK11 },
        { 0x6000, 0x7fff, MWA_BANK12 },
        { 0x8000, 0x9fff, MWA_BANK13 },
        { 0xa000, 0xbfff, MWA_BANK14 },
        { 0xc000, 0xdfff, MWA_BANK15 },
        { 0xe000, 0xffff, MWA_BANK16 },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress tc2048_readmem[] = {
	{ 0x0000, 0x3fff, MRA_ROM },
        { 0x4000, 0xffff, MRA_BANK1 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress tc2048_writemem[] = {
	{ 0x0000, 0x3fff, MWA_ROM },
        { 0x4000, 0xffff, MWA_BANK2 },
	{ -1 }  /* end of table */
};

/* KT: more accurate keyboard reading */
/* DJR: Spectrum+ keys added */
int spectrum_port_fe_r(int offset)
{
   int lines = offset>>8;
   int data = 0xff;

   int cs_extra1 = readinputport(8)  & 0x1f;
   int cs_extra2 = readinputport(9)  & 0x1f;
   int cs_extra3 = readinputport(10) & 0x1f;
   int ss_extra1 = readinputport(11) & 0x1f;
   int ss_extra2 = readinputport(12) & 0x1f;

   /* Caps - V */
   if ((lines & 1)==0)
   {
        data &= readinputport(0);
        /* CAPS for extra keys */
        if (cs_extra1 != 0x1f || cs_extra2 != 0x1f || cs_extra3 != 0x1f)
            data &= ~0x01;
   }

   /* A - G */
   if ((lines & 2)==0)
        data &= readinputport(1);

   /* Q - T */
   if ((lines & 4)==0)
        data &= readinputport(2);

   /* 1 - 5 */
   if ((lines & 8)==0)
        data &= readinputport(3) & cs_extra1;

   /* 6 - 0 */
   if ((lines & 16)==0)
        data &= readinputport(4) & cs_extra2;

   /* Y - P */
   if ((lines & 32)==0)
        data &= readinputport(5) & ss_extra1;

   /* H - Enter */
   if ((lines & 64)==0)
        data &= readinputport(6);

   /* B - Space */
   if ((lines & 128)==0)
   {
        data &= readinputport(7) & cs_extra3 & ss_extra2;
        /* SYMBOL SHIFT for extra keys */
        if (ss_extra1 != 0x1f || ss_extra2 != 0x1f)
            data &= ~0x02;
   }

   data |= (0xe0); /* Set bits 5-7 - as reset above */

	 /*-----------------27/02/00 10:46-------------------
		cassette input from wav
	 --------------------------------------------------*/
	if (device_input(IO_CASSETTE, 0)>255 )
	{
                        data &= ~0x40;
	}

   /* Issue 2 Spectrums default to having bits 5, 6 & 7 set.
      Issue 3 Spectrums default to having bits 5 & 7 set and bit 6 reset. */
   if (readinputport(16) & 0x80)
        data ^= (0x40);

   return data;
}

/* kempston joystick interface */
int spectrum_port_1f_r(int offset)
{
  return readinputport(13) & 0x1f;
}

/* fuller joystick interface */
int spectrum_port_7f_r(int offset)
{
  return readinputport(14) | (0xff^0x8f);
}

/* mikrogen joystick interface */
int spectrum_port_df_r(int offset)
{
  return readinputport(15) | (0xff^0x1f);
}

READ_HANDLER ( spectrum_port_r )
{
        if ((offset & 1)==0)
            return spectrum_port_fe_r(offset);

        if ((offset & 0xff)==0x1f)
            return spectrum_port_1f_r(offset);

        if ((offset & 0xff)==0x7f)
            return spectrum_port_7f_r(offset);

        if ((offset & 0xff)==0xdf)
            return spectrum_port_df_r(offset);

        logerror("Read from port: %04x\n", offset);

        return 0xff;
}

WRITE_HANDLER ( spectrum_port_w )
{
        if ((offset & 1)==0)
            spectrum_port_fe_w(offset,data);
        else
        {
            logerror("Write %02x to Port: %04x\n", data, offset);
        }
}

/* KT: Changed it to this because the ports are not decoded fully.
The function decodes the ports appropriately */
static struct IOReadPort spectrum_readport[] = {
        {0x0000, 0xffff, spectrum_port_r},
	{ -1 }
};

/* KT: Changed it to this because the ports are not decoded fully.
The function decodes the ports appropriately */
static struct IOWritePort spectrum_writeport[] = {
        {0x0000, 0xffff, spectrum_port_w},
	{ -1 }
};


READ_HANDLER ( spectrum_128_port_r )
{
     if ((offset & 1)==0)
     {
         return spectrum_port_fe_r(offset);
     }

     /* KT: the following is not decoded exactly, need to check what
     is correct */
     if ((offset & 2)==0)
     {
         switch ((offset>>8) & 0xff)
         {
                case 0xff:
                        return spectrum_128_port_fffd_r(offset);
         }
     }

     if ((offset & 0xff)==0x1f)
         return spectrum_port_1f_r(offset);

     if ((offset & 0xff)==0x7f)
         return spectrum_port_7f_r(offset);

     if ((offset & 0xff)==0xdf)
         return spectrum_port_df_r(offset);

     logerror("Read from 128 port: %04x\n", offset);

     return 0xff;
}

READ_HANDLER ( spectrum_plus3_port_r )
{
     if ((offset & 1)==0)
     {
         return spectrum_port_fe_r(offset);
     }

     /* KT: the following is not decoded exactly, need to check what
     is correct */
     if ((offset & 2)==0)
     {
         switch ((offset>>8) & 0xff)
         {
                case 0xff: return spectrum_128_port_fffd_r(offset);
                case 0x2f: return spectrum_plus3_port_2ffd_r(offset);
                case 0x3f: return spectrum_plus3_port_3ffd_r(offset);
                case 0x1f: return spectrum_port_1f_r(offset);
                case 0x7f: return spectrum_port_7f_r(offset);
                case 0xdf: return spectrum_port_df_r(offset);
         }
     }

     logerror("Read from +3 port: %04x\n", offset);

     return 0xff;
}

WRITE_HANDLER ( spectrum_128_port_w )
{
        if ((offset & 1)==0)
                spectrum_port_fe_w(offset,data);

        /* Only decodes on A15, A14 & A1 */
        else if ((offset & 2)==0)
        {
                switch ((offset>>8) & 0xc0)
                {
                        case 0x40:
                                spectrum_128_port_7ffd_w(offset, data);
                                break;
                        case 0x80:
                                spectrum_128_port_bffd_w(offset, data);
                                break;
                        case 0xc0:
                                spectrum_128_port_fffd_w(offset, data);
                                break;
                        default:
                                logerror("Write %02x to 128 port: %04x\n", data, offset);
                }
        }
        else
        {
            logerror("Write %02x to 128 port: %04x\n", data, offset);
        }
}

WRITE_HANDLER ( spectrum_plus3_port_w )
{
        if ((offset & 1)==0)
                spectrum_port_fe_w(offset,data);

        /* the following is not decoded exactly, need to check
        what is correct! */
        else if ((offset & 2)==0)
        {
                switch ((offset>>8) & 0xf0)
                {
                        case 0x70:
                                spectrum_plus3_port_7ffd_w(offset, data);
                                break;
                        case 0xb0:
                                spectrum_128_port_bffd_w(offset, data);
                                break;
                        case 0xf0:
                                spectrum_128_port_fffd_w(offset, data);
                                break;
                        case 0x10:
                                spectrum_plus3_port_1ffd_w(offset, data);
                                break;
                        case 0x30:
                                spectrum_plus3_port_3ffd_w(offset, data);
                        default:
                                logerror("Write %02x to +3 port: %04x\n", data, offset);
                }
        }
        else
        {
            logerror("Write %02x to +3 port: %04x\n", data, offset);
        }
}

READ_HANDLER ( ts2068_port_r )
{
        switch (offset & 0xff)
        {
                /* Note: keys only decoded on port #fe not all even ports so
                   ports #f4 & #f6 correctly read */
                case 0xf4: return ts2068_port_f4_r(offset);
                case 0xf6: return ts2068_port_f6_r(offset);
                case 0xff: return ts2068_port_ff_r(offset);

                case 0xfe: return spectrum_port_fe_r(offset);
                case 0x1f: return spectrum_port_1f_r(offset);
                case 0x7f: return spectrum_port_7f_r(offset);
                case 0xdf: return spectrum_port_df_r(offset);
        }
        logerror("Read from port: %04x\n", offset);

        return 0xff;
}

WRITE_HANDLER ( ts2068_port_w )
{
/* Ports #fd & #fc were reserved by Timex for bankswitching and are not used
   by either the hardware or system software.
   Port #fb is the Thermal printer port and works exactly as the Sinclair
   Printer - ie not yet emulated.
*/
        switch (offset & 0xff)
        {
                case 0xfe: spectrum_port_fe_w(offset,data); break;
                case 0xf4: ts2068_port_f4_w(offset,data); break;
                case 0xf5: ts2068_port_f5_w(offset,data); break;
                case 0xf6: ts2068_port_f6_w(offset,data); break;
                case 0xff: ts2068_port_ff_w(offset,data); break;
                default:
                        logerror("Write %02x to Port: %04x\n", data, offset);
        }
}

READ_HANDLER ( tc2048_port_r )
{
        if ((offset & 1)==0)
                return spectrum_port_fe_r(offset);
        switch (offset & 0xff)
        {
                case 0xff: return ts2068_port_ff_r(offset);
                case 0x1f: return spectrum_port_1f_r(offset);
                case 0x7f: return spectrum_port_7f_r(offset);
                case 0xdf: return spectrum_port_df_r(offset);
        }

        logerror("Read from port: %04x\n", offset);
        return 0xff;
}

WRITE_HANDLER ( tc2048_port_w )
{
        if ((offset & 1)==0)
                spectrum_port_fe_w(offset,data);
        else if ((offset & 0xff)==0xff)
                tc2048_port_ff_w(offset,data);
        else
        {
                logerror("Write %02x to Port: %04x\n", data, offset);
        }
}

static struct IOReadPort spectrum_128_readport[] = {
        {0x0000, 0xffff, spectrum_128_port_r},
        { -1 }
};

static struct IOWritePort spectrum_128_writeport[] = {
        {0x0000, 0xffff, spectrum_128_port_w},
        { -1 }
};

/* KT: Changed it to this because the ports are not decoded fully.
The function decodes the ports appropriately */
static struct IOReadPort spectrum_plus3_readport[] = {
        {0x0000, 0xffff, spectrum_plus3_port_r},
        { -1 }
};

/* KT: Changed it to this because the ports are not decoded fully.
The function decodes the ports appropriately */
static struct IOWritePort spectrum_plus3_writeport[] = {
        {0x0000, 0xffff, spectrum_plus3_port_w},
        { -1 }
};


static struct IOReadPort ts2068_readport[] = {
        {0x0000, 0x0ffff, ts2068_port_r},
	{ -1 }
};

static struct IOWritePort ts2068_writeport[] = {
        {0x0000, 0x0ffff, ts2068_port_w},
	{ -1 }
};


static struct IOReadPort tc2048_readport[] = {
        {0x0000, 0x0ffff, tc2048_port_r},
	{ -1 }
};

static struct IOWritePort tc2048_writeport[] = {
        {0x0000, 0x0ffff, tc2048_port_w},
	{ -1 }
};


static struct AY8910interface spectrum_128_ay_interface =
{
        1,
        1000000,
        {25,25},
        {0},
        {0},
        {0}
};


static struct GfxLayout spectrum_charlayout = {
	8,8,
	256,
	1,                      /* 1 bits per pixel */

	{ 0 },                  /* no bitplanes; 1 bit per pixel */

	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 8*256, 16*256, 24*256, 32*256, 40*256, 48*256, 56*256 },

	8		        /* every char takes 1 consecutive byte */
};

static struct GfxDecodeInfo spectrum_gfxdecodeinfo[] = {
	{ 0, 0x0, &spectrum_charlayout, 0, 0x80 },
	{ 0, 0x0, &spectrum_charlayout, 0, 0x80 },
	{ 0, 0x0, &spectrum_charlayout, 0, 0x80 },
	{ -1 } /* end of array */
};

INPUT_PORTS_START( spectrum )
	PORT_START /* 0xFEFE */
        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "CAPS SHIFT",                       KEYCODE_LSHIFT,  IP_JOY_NONE )
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z  COPY    :      LN       BEEP",  KEYCODE_Z,  IP_JOY_NONE )
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "X  CLEAR   Pound  EXP      INK",   KEYCODE_X,  IP_JOY_NONE )
        PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C  CONT    ?      LPRINT   PAPER", KEYCODE_C,  IP_JOY_NONE )
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "V  CLS     /      LLIST    FLASH", KEYCODE_V,  IP_JOY_NONE )

	PORT_START /* 0xFDFE */
        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "A  NEW     STOP   READ     ~",  KEYCODE_A,  IP_JOY_NONE )
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "S  SAVE    NOT    RESTORE  |",  KEYCODE_S,  IP_JOY_NONE )
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "D  DIM     STEP   DATA     \\", KEYCODE_D,  IP_JOY_NONE )
        PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F  FOR     TO     SGN      {",  KEYCODE_F,  IP_JOY_NONE )
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "G  GOTO    THEN   ABS      }",  KEYCODE_G,  IP_JOY_NONE )

	PORT_START /* 0xFBFE */
        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q  PLOT    <=     SIN      ASN",    KEYCODE_Q,  IP_JOY_NONE )
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "W  DRAW    <>     COS      ACS",    KEYCODE_W,  IP_JOY_NONE )
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "E  REM     >=     TAN      ATN",    KEYCODE_E,  IP_JOY_NONE )
        PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "R  RUN     <      INT      VERIFY", KEYCODE_R,  IP_JOY_NONE )
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T  RAND    >      RND      MERGE",  KEYCODE_T,  IP_JOY_NONE )

        /* interface II uses this port for joystick */
	PORT_START /* 0xF7FE */
        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1          !      BLUE     DEF FN", KEYCODE_1,  IP_JOY_NONE )
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2          @      RED      FN",     KEYCODE_2,  IP_JOY_NONE )
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "3          #      MAGENTA  LINE",   KEYCODE_3,  IP_JOY_NONE )
        PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4          $      GREEN    OPEN#",  KEYCODE_4,  IP_JOY_NONE )
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5          %      CYAN     CLOSE#", KEYCODE_5,  IP_JOY_NONE )

        /* protek clashes with interface II! uses 5 = left, 6 = down, 7 = up, 8 = right, 0 = fire */
	PORT_START /* 0xEFFE */
        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0          _      BLACK    FORMAT", KEYCODE_0,  IP_JOY_NONE )
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9          )               POINT",  KEYCODE_9,  IP_JOY_NONE )
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8          (               CAT",    KEYCODE_8,  IP_JOY_NONE )
        PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7          '      WHITE    ERASE",  KEYCODE_7,  IP_JOY_NONE )
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "6          &      YELLOW   MOVE",   KEYCODE_6,  IP_JOY_NONE )

	PORT_START /* 0xDFFE */
        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P  PRINT   \"      TAB      (c)", KEYCODE_P,  IP_JOY_NONE )
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O  POKE    ;      PEEK     OUT", KEYCODE_O,  IP_JOY_NONE )
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "I  INPUT   AT     CODE     IN",  KEYCODE_I,  IP_JOY_NONE )
        PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "U  IF      OR     CHR$     ]",   KEYCODE_U,  IP_JOY_NONE )
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y  RETURN  AND    STR$     [",   KEYCODE_Y,  IP_JOY_NONE )

	PORT_START /* 0xBFFE */
        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER",                              KEYCODE_ENTER,  IP_JOY_NONE )
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L  LET     =      USR      ATTR",    KEYCODE_L,  IP_JOY_NONE )
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "K  LIST    +      LEN      SCREEN$", KEYCODE_K,  IP_JOY_NONE )
        PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "J  LOAD    -      VAL      VAL$",    KEYCODE_J,  IP_JOY_NONE )
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "H  GOSUB   ^      SQR      CIRCLE",  KEYCODE_H,  IP_JOY_NONE )

	PORT_START /* 0x7FFE */
        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE",                              KEYCODE_SPACE,   IP_JOY_NONE )
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "SYMBOL SHIFT",                       KEYCODE_RSHIFT,  IP_JOY_NONE )
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "M  PAUSE   .      PI       INVERSE", KEYCODE_M,  IP_JOY_NONE )
        PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "N  NEXT    ,      INKEY$   OVER",    KEYCODE_N,  IP_JOY_NONE )
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "B  BORDER  *      BIN      BRIGHT",  KEYCODE_B,  IP_JOY_NONE )

        PORT_START /* Spectrum+ Keys (set CAPS + 1-5) */
        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "EDIT          (CAPS + 1)",  KEYCODE_F1,         IP_JOY_NONE )
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "CAPS LOCK     (CAPS + 2)",  KEYCODE_CAPSLOCK,   IP_JOY_NONE )
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "TRUE VID      (CAPS + 3)",  KEYCODE_F2,         IP_JOY_NONE )
        PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "INV VID       (CAPS + 4)",  KEYCODE_F3,         IP_JOY_NONE )
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Cursor left   (CAPS + 5)",  KEYCODE_LEFT,       IP_JOY_NONE )
        PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

        PORT_START /* Spectrum+ Keys (set CAPS + 6-0) */
        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "DEL           (CAPS + 0)",  KEYCODE_BACKSPACE,  IP_JOY_NONE )
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "GRAPH         (CAPS + 9)",  KEYCODE_LALT,       IP_JOY_NONE )
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Cursor right  (CAPS + 8)",  KEYCODE_RIGHT,      IP_JOY_NONE )
        PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Cursor up     (CAPS + 7)",  KEYCODE_UP,         IP_JOY_NONE )
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Cursor down   (CAPS + 6)",  KEYCODE_DOWN,       IP_JOY_NONE )
        PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

        PORT_START /* Spectrum+ Keys (set CAPS + SPACE and CAPS + SYMBOL */
        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK",                     KEYCODE_PAUSE,      IP_JOY_NONE )
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "EXT MODE",                  KEYCODE_LCONTROL,   IP_JOY_NONE )
        PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)

        PORT_START /* Spectrum+ Keys (set SYMBOL SHIFT + O/P */
        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "\"", KEYCODE_F4,  IP_JOY_NONE )
/*        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "\"", KEYCODE_QUOTE,  IP_JOY_NONE ) */
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_COLON,  IP_JOY_NONE )
        PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)

        PORT_START /* Spectrum+ Keys (set SYMBOL SHIFT + N/M */
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP,   IP_JOY_NONE )
        PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA,  IP_JOY_NONE )
        PORT_BIT(0xf3, IP_ACTIVE_LOW, IPT_UNUSED)

        PORT_START /* Kempston joystick interface */
        PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "KEMPSTON JOYSTICK RIGHT",     IP_KEY_NONE,    JOYCODE_1_RIGHT )
        PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "KEMPSTON JOYSTICK LEFT",      IP_KEY_NONE,   JOYCODE_1_LEFT )
        PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "KEMPSTON JOYSTICK DOWN",         IP_KEY_NONE,        JOYCODE_1_DOWN )
        PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "KEMPSTON JOYSTICK UP",         IP_KEY_NONE,        JOYCODE_1_UP)
        PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "KEMPSTON JOYSTICK FIRE",         IP_KEY_NONE,        JOYCODE_1_BUTTON1 )

        PORT_START /* Fuller joystick interface */
        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "FULLER JOYSTICK UP",     IP_KEY_NONE,    JOYCODE_1_UP )
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "FULLER JOYSTICK DOWN",      IP_KEY_NONE,   JOYCODE_1_DOWN )
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "FULLER JOYSTICK LEFT",         IP_KEY_NONE,        JOYCODE_1_LEFT )
        PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "FULLER JOYSTICK RIGHT",         IP_KEY_NONE,        JOYCODE_1_RIGHT)
        PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "FULLER JOYSTICK FIRE",         IP_KEY_NONE,        JOYCODE_1_BUTTON1)

        PORT_START /* Mikrogen joystick interface */
        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "MIKROGEN JOYSTICK UP",     IP_KEY_NONE,    JOYCODE_1_UP )
        PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "MIKROGEN JOYSTICK DOWN",      IP_KEY_NONE,   JOYCODE_1_DOWN )
        PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "MIKROGEN JOYSTICK RIGHT",         IP_KEY_NONE,        JOYCODE_1_RIGHT )
        PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "MIKROGEN JOYSTICK LEFT",         IP_KEY_NONE,        JOYCODE_1_LEFT)
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "MIKROGEN JOYSTICK FIRE",         IP_KEY_NONE,        JOYCODE_1_BUTTON1)


        PORT_START
        PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Quickload", KEYCODE_F8, IP_JOY_NONE)
        PORT_DIPNAME(0x80, 0x00, "Hardware Version")
        PORT_DIPSETTING(0x00, "Issue 2" )
        PORT_DIPSETTING(0x80, "Issue 3" )
        PORT_DIPNAME(0x40, 0x00, "End of .TAP action")
        PORT_DIPSETTING(0x00, "Disable .TAP support" )
        PORT_DIPSETTING(0x40, "Rewind tape to start (to reload earlier levels)" )
        PORT_DIPNAME(0x20, 0x00, "+3/+2a etc. Disk Drive")
        PORT_DIPSETTING(0x00, "Enabled" )
        PORT_DIPSETTING(0x20, "Disabled" )
        PORT_BIT(0x1f, IP_ACTIVE_LOW, IPT_UNUSED)

INPUT_PORTS_END

static unsigned char spectrum_palette[16*3] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0xbf,
	0xbf, 0x00, 0x00, 0xbf, 0x00, 0xbf,
	0x00, 0xbf, 0x00, 0x00, 0xbf, 0xbf,
	0xbf, 0xbf, 0x00, 0xbf, 0xbf, 0xbf,

	0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
	0xff, 0x00, 0x00, 0xff, 0x00, 0xff,
	0x00, 0xff, 0x00, 0x00, 0xff, 0xff,
	0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
};

static unsigned short spectrum_colortable[128*2] = {
	0,0, 0,1, 0,2, 0,3, 0,4, 0,5, 0,6, 0,7,
	1,0, 1,1, 1,2, 1,3, 1,4, 1,5, 1,6, 1,7,
	2,0, 2,1, 2,2, 2,3, 2,4, 2,5, 2,6, 2,7,
	3,0, 3,1, 3,2, 3,3, 3,4, 3,5, 3,6, 3,7,
	4,0, 4,1, 4,2, 4,3, 4,4, 4,5, 4,6, 4,7,
	5,0, 5,1, 5,2, 5,3, 5,4, 5,5, 5,6, 5,7,
	6,0, 6,1, 6,2, 6,3, 6,4, 6,5, 6,6, 6,7,
	7,0, 7,1, 7,2, 7,3, 7,4, 7,5, 7,6, 7,7,

	 8,8,  8,9,  8,10,  8,11,  8,12,  8,13,  8,14,  8,15,
	 9,8,  9,9,  9,10,  9,11,  9,12,  9,13,  9,14,  9,15,
	10,8, 10,9, 10,10, 10,11, 10,12, 10,13, 10,14, 10,15,
	11,8, 11,9, 11,10, 11,11, 11,12, 11,13, 11,14, 11,15,
	12,8, 12,9, 12,10, 12,11, 12,12, 12,13, 12,14, 12,15,
	13,8, 13,9, 13,10, 13,11, 13,12, 13,13, 13,14, 13,15,
	14,8, 14,9, 14,10, 14,11, 14,12, 14,13, 14,14, 14,15,
	15,8, 15,9, 15,10, 15,11, 15,12, 15,13, 15,14, 15,15
};
/* Initialise the palette */
static void spectrum_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,spectrum_palette,sizeof(spectrum_palette));
	memcpy(sys_colortable,spectrum_colortable,sizeof(spectrum_colortable));
}

int spec_interrupt (void)
{
	static int quickload = 0;

        if (!quickload && (readinputport(16) & 0x8000))
        {
                spec_quick_open (0, 0, NULL);
                quickload = 1;
        }
        else
                quickload = 0;

        return interrupt ();
}

static struct Speaker_interface spectrum_speaker_interface=
{
 1,
 {50},
};

static struct Wave_interface spectrum_wave_interface=
{
	1,    /* number of cassette drives = number of waves to mix */
	{25},	/* default mixing level */

};

static struct MachineDriver machine_driver_spectrum =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
			3500000,        /* 3.5 Mhz */
                        spectrum_readmem,spectrum_writemem,
			spectrum_readport,spectrum_writeport,
                        spec_interrupt,1,
		},
	},
	50, 2500,       /* frames per second, vblank duration */
	1,
	spectrum_init_machine,
        spectrum_shutdown_machine,

	/* video hardware */
        SPEC_SCREEN_WIDTH,              /* screen width */
        SPEC_SCREEN_HEIGHT,             /* screen height */
        { 0, SPEC_SCREEN_WIDTH-1, 0, SPEC_SCREEN_HEIGHT-1},  /* visible_area */
	spectrum_gfxdecodeinfo,	             /* graphics decode info */
	16, 256,                             /* colors used for the characters */
	spectrum_init_palette,               /* initialise palette */

	VIDEO_TYPE_RASTER,
        spectrum_eof_callback,
	spectrum_vh_start,
	spectrum_vh_stop,
	spectrum_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
        {
                /* standard spectrum sound */
                {
                        SOUND_SPEAKER,
                        &spectrum_speaker_interface
                },
                /*-----------------27/02/00 10:40-------------------
                 cassette wave interface
                --------------------------------------------------*/
                {
                        SOUND_WAVE,
                        &spectrum_wave_interface,
                }
        }
};

static struct MachineDriver machine_driver_spectrum_128 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
                        3546900,        /* 3.54690 Mhz */
                        spectrum_128_readmem,spectrum_128_writemem,
                        spectrum_128_readport,spectrum_128_writeport,
                        spec_interrupt,1,
		},
	},
	50, 2500,       /* frames per second, vblank duration */
	1,
        spectrum_128_init_machine,
        spectrum_128_exit_machine,

	/* video hardware */
        SPEC_SCREEN_WIDTH,              /* screen width */
        SPEC_SCREEN_HEIGHT,             /* screen height */
        { 0, SPEC_SCREEN_WIDTH-1, 0, SPEC_SCREEN_HEIGHT-1},  /* visible_area */
	spectrum_gfxdecodeinfo,	             /* graphics decode info */
	16, 256,                             /* colors used for the characters */
	spectrum_init_palette,               /* initialise palette */

	VIDEO_TYPE_RASTER,
        spectrum_eof_callback,
        spectrum_128_vh_start,
        spectrum_128_vh_stop,
        spectrum_128_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
        {
                /* +3 Ay-3-8912 sound */
                {
                        SOUND_AY8910,
                        &spectrum_128_ay_interface,
                },
                /* standard spectrum buzzer sound */
                {
                        SOUND_SPEAKER,
                        &spectrum_speaker_interface,
                },
                /*-----------------27/02/00 10:40-------------------
                 cassette wave interface
                --------------------------------------------------*/
                {
                        SOUND_WAVE,
                        &spectrum_wave_interface,
                }
        }
};

static struct MachineDriver machine_driver_spectrum_plus3 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
                        3546900,        /* 3.54690 Mhz */
                        spectrum_128_readmem,spectrum_128_writemem,
                        spectrum_plus3_readport,spectrum_plus3_writeport,
                        spec_interrupt,1,
		},
	},
	50, 2500,       /* frames per second, vblank duration */
	1,
        spectrum_plus3_init_machine,
        spectrum_128_exit_machine,

	/* video hardware */
        SPEC_SCREEN_WIDTH,              /* screen width */
        SPEC_SCREEN_HEIGHT,             /* screen height */
        { 0, SPEC_SCREEN_WIDTH-1, 0, SPEC_SCREEN_HEIGHT-1},  /* visible_area */
	spectrum_gfxdecodeinfo,	             /* graphics decode info */
	16, 256,                             /* colors used for the characters */
	spectrum_init_palette,               /* initialise palette */

	VIDEO_TYPE_RASTER,
        spectrum_eof_callback,
        spectrum_128_vh_start,
        spectrum_128_vh_stop,
        spectrum_128_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
        {
                /* +3 Ay-3-8912 sound */
                {
                        SOUND_AY8910,
                        &spectrum_128_ay_interface,
                },
                /* standard spectrum buzzer sound */
                {
                        SOUND_SPEAKER,
                        &spectrum_speaker_interface,
                },
                /*-----------------27/02/00 10:40-------------------
                 cassette wave interface
                --------------------------------------------------*/
                {
                        SOUND_WAVE,
                        &spectrum_wave_interface,
                }
        }
};

static struct MachineDriver machine_driver_ts2068 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
                        3580000,        /* 3.58 Mhz */
                        ts2068_readmem,ts2068_writemem,
                        ts2068_readport,ts2068_writeport,
                        spec_interrupt,1,
		},
	},
        60, 2500,       /* frames per second, vblank duration */
	1,
        ts2068_init_machine,
        ts2068_exit_machine,

	/* video hardware */
        TS2068_SCREEN_WIDTH,            /* screen width */
        TS2068_SCREEN_HEIGHT,           /* screen height */
        { 0, TS2068_SCREEN_WIDTH-1, 0, TS2068_SCREEN_HEIGHT-1},  /* visible_area */
	spectrum_gfxdecodeinfo,	             /* graphics decode info */
	16, 256,                             /* colors used for the characters */
	spectrum_init_palette,               /* initialise palette */

	VIDEO_TYPE_RASTER,
        ts2068_eof_callback,
        spectrum_128_vh_start,
        spectrum_128_vh_stop,
        ts2068_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
        {
                /* +3 Ay-3-8912 sound */
                {
                        SOUND_AY8910,
                        &spectrum_128_ay_interface,
                },
                /* standard spectrum sound */
                {
                        SOUND_SPEAKER,
                        &spectrum_speaker_interface
                },
                /*-----------------27/02/00 10:40-------------------
                 cassette wave interface
                --------------------------------------------------*/
                {
                        SOUND_WAVE,
                        &spectrum_wave_interface,
                }
        }
};

static struct MachineDriver machine_driver_tc2048 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
			3500000,        /* 3.5 Mhz */
                        tc2048_readmem,tc2048_writemem,
                        tc2048_readport,tc2048_writeport,
                        spec_interrupt,1,
		},
	},
	50, 2500,       /* frames per second, vblank duration */
	1,
        tc2048_init_machine,
        ts2068_exit_machine,

	/* video hardware */
        TS2068_SCREEN_WIDTH,            /* screen width */
        SPEC_SCREEN_HEIGHT,             /* screen height */
        { 0, TS2068_SCREEN_WIDTH-1, 0, SPEC_SCREEN_HEIGHT-1},  /* visible_area */
	spectrum_gfxdecodeinfo,	             /* graphics decode info */
	16, 256,                             /* colors used for the characters */
	spectrum_init_palette,               /* initialise palette */

	VIDEO_TYPE_RASTER,
        spectrum_eof_callback,
        spectrum_128_vh_start,
        spectrum_128_vh_stop,
        tc2048_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
        {
                /* standard spectrum sound */
                {
                        SOUND_SPEAKER,
                        &spectrum_speaker_interface
                },
                /*-----------------27/02/00 10:40-------------------
                 cassette wave interface
                --------------------------------------------------*/
                {
                        SOUND_WAVE,
                        &spectrum_wave_interface,
                }
        }
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(spectrum)
	ROM_REGION(0x10000,REGION_CPU1)
	ROM_LOAD("spectrum.rom", 0x0000, 0x4000, 0xddee531f)
ROM_END

ROM_START(specbusy)
	ROM_REGION(0x10000,REGION_CPU1)
	ROM_LOAD("48-busy.rom", 0x0000, 0x4000, 0x1511cddb)
ROM_END

ROM_START(specgrot)
	ROM_REGION(0x10000,REGION_CPU1)
	ROM_LOAD("48-groot.rom", 0x0000, 0x4000, 0xabf18c45)
ROM_END

ROM_START(specimc)
	ROM_REGION(0x10000,REGION_CPU1)
	ROM_LOAD("48-imc.rom", 0x0000, 0x4000, 0xd1be99ee)
ROM_END

ROM_START(speclec)
	ROM_REGION(0x10000,REGION_CPU1)
	ROM_LOAD("80-lec.rom", 0x0000, 0x4000, 0x5b5c92b1)
ROM_END

ROM_START(spec128)
        ROM_REGION(0x18000,REGION_CPU1)
	ROM_LOAD("zx128_0.rom",0x10000,0x4000, 0xe76799d2)
	ROM_LOAD("zx128_1.rom",0x14000,0x4000, 0xb96a36be)
ROM_END

ROM_START(spec128s)
        ROM_REGION(0x18000,REGION_CPU1)
	ROM_LOAD("zx128s0.rom",0x10000,0x4000, 0x453d86b2)
	ROM_LOAD("zx128s1.rom",0x14000,0x4000, 0x6010e796)
ROM_END

ROM_START(specpls2)
        ROM_REGION(0x18000,REGION_CPU1)
	ROM_LOAD("zxp2_0.rom",0x10000,0x4000, 0x5d2e8c66)
	ROM_LOAD("zxp2_1.rom",0x14000,0x4000, 0x98b1320b)
ROM_END

ROM_START(specpl2a)
        ROM_REGION(0x20000,REGION_CPU1)
        ROM_LOAD("p2a41_0.rom",0x10000,0x4000, 0x30c9f490)
        ROM_LOAD("p2a41_1.rom",0x14000,0x4000, 0xa7916b3f)
        ROM_LOAD("p2a41_2.rom",0x18000,0x4000, 0xc9a0b748)
        ROM_LOAD("p2a41_3.rom",0x1c000,0x4000, 0xb88fd6e3)
ROM_END

ROM_START(specpls3)
        ROM_REGION(0x20000,REGION_CPU1)
	ROM_LOAD("pl3-0.rom",0x10000,0x4000, 0x17373da2)
	ROM_LOAD("pl3-1.rom",0x14000,0x4000, 0xf1d1d99e)
	ROM_LOAD("pl3-2.rom",0x18000,0x4000, 0x3dbf351d)
	ROM_LOAD("pl3-3.rom",0x1c000,0x4000, 0x04448eaa)
ROM_END

ROM_START(specpls4)
        ROM_REGION(0x10000,REGION_CPU1)
        ROM_LOAD("plus4.rom",0x0000,0x4000, 0x7e0f47cb)
ROM_END

ROM_START(tk90x)
        ROM_REGION(0x10000,REGION_CPU1)
        ROM_LOAD("tk90x.rom",0x0000,0x4000, 0x3e785f6f)
ROM_END

ROM_START(tk95)
        ROM_REGION(0x10000,REGION_CPU1)
        ROM_LOAD("tk95.rom",0x0000,0x4000, 0x17368e07)
ROM_END

ROM_START(inves)
        ROM_REGION(0x10000,REGION_CPU1)
        ROM_LOAD("inves.rom",0x0000,0x4000, 0x8ff7a4d1)
ROM_END

ROM_START(tc2048)
        ROM_REGION(0x10000,REGION_CPU1)
        ROM_LOAD("tc2048.rom",0x0000,0x4000, 0xf1b5fa67)
ROM_END

ROM_START(ts2068)
        ROM_REGION(0x16000,REGION_CPU1)
        ROM_LOAD("ts2068_h.rom",0x10000,0x4000, 0xbf44ec3f)
        ROM_LOAD("ts2068_x.rom",0x14000,0x2000, 0xae16233a)
ROM_END

ROM_START(specp2fr)
        ROM_REGION(0x18000,REGION_CPU1)
        ROM_LOAD("plus2fr0.rom",0x10000,0x4000, 0xc684c535)
        ROM_LOAD("plus2fr1.rom",0x14000,0x4000, 0xf5e509c5)
ROM_END

ROM_START(specp2sp)
        ROM_REGION(0x18000,REGION_CPU1)
        ROM_LOAD("plus2sp0.rom",0x10000,0x4000, 0xe807d06e)
        ROM_LOAD("plus2sp1.rom",0x14000,0x4000, 0x41981d4b)
ROM_END

ROM_START(specp3sp)
        ROM_REGION(0x20000,REGION_CPU1)
        ROM_LOAD("plus3sp0.rom",0x10000,0x4000, 0x1f86147a)
        ROM_LOAD("plus3sp1.rom",0x14000,0x4000, 0xa8ac4966)
        ROM_LOAD("plus3sp2.rom",0x18000,0x4000, 0xf6bb0296)
        ROM_LOAD("plus3sp3.rom",0x1c000,0x4000, 0xf6d25389)
ROM_END

ROM_START(specpl3e)
        ROM_REGION(0x20000,REGION_CPU1)
        ROM_LOAD("roma.bin",0x10000,0x8000, 0x7c20e2c9)
        ROM_LOAD("romb.bin",0x18000,0x8000, 0x4a700c7e)
ROM_END

#define IODEVICE_SPEC_QUICK \
{\
   IO_QUICKLOAD,       /* type */\
   1,                  /* count */\
   "scr\0",            /*file extensions */\
   NULL,               /* private */\
   NULL,               /* id */\
   spec_quick_init,    /* init */\
   spec_quick_exit,    /* exit */\
   NULL,               /* info */\
   spec_quick_open,    /* open */\
   NULL,               /* close */\
   NULL,               /* status */\
   NULL,               /* seek */\
   NULL,               /* input */\
   NULL,               /* output */\
   NULL,               /* input_chunk */\
   NULL                /* output_chunk */\
}

static const struct IODevice io_spectrum[] = {
	{
        IO_SNAPSHOT,            /* type */
        1,                      /* count */
        "sna\0z80\0",           /* file extensions */
        NULL,                   /* private */
        spectrum_rom_id,        /* id */
        spectrum_rom_load,      /* init */
        spectrum_rom_exit,      /* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
        IODEVICE_SPEC_QUICK,
        IO_CASSETTE_WAVE(1,"wav\0tap\0", NULL,spectrum_cassette_init, spectrum_cassette_exit),
        { IO_END }
};

static const struct IODevice io_specpls3[] = {
        {
        IO_SNAPSHOT,            /* type */
        1,                      /* count */
        "sna\0z80\0",           /* file extensions */
        NULL,                   /* private */
        spectrum_rom_id,        /* id */
        spectrum_rom_load,      /* init */
        spectrum_rom_exit,      /* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
        IODEVICE_SPEC_QUICK,
        IO_CASSETTE_WAVE(1,"wav\0tap\0", NULL,spectrum_cassette_init, spectrum_cassette_exit),
	{
                IO_FLOPPY,              /* type */
                2,                      /* count */
                "dsk\0",                /* file extensions */
                NULL,                   /* private */
		dsk_floppy_id,		/* id */
		dsk_floppy_load,	/* init */
		dsk_floppy_exit,	/* exit */
                NULL,                   /* info */
                NULL,                   /* open */
                NULL,                   /* close */
                NULL,                   /* status */
                NULL,                   /* seek */
                NULL,                   /* input */
                NULL,                   /* output */
                NULL,                   /* input_chunk */
                NULL,                   /* output chunk */
	},
	{ IO_END }
};

#define io_spec128  io_spectrum
#define io_spec128s io_spectrum
#define io_specpls2 io_spectrum
#define io_specbusy io_spectrum
#define io_specgrot io_spectrum
#define io_specimc  io_spectrum
#define io_speclec  io_spectrum
#define io_specpls4 io_spectrum
#define io_inves    io_spectrum
#define io_tk90x    io_spectrum
#define io_tk95     io_spectrum
#define io_tc2048   io_spectrum
#define io_ts2068   io_spectrum
#define io_specpl2a io_specpls3
#define io_specp2fr io_spectrum
#define io_specp2sp io_spectrum
#define io_specp3sp io_specpls3
#define io_specpl3e io_specpls3

/*     YEAR  NAME      PARENT    MACHINE         INPUT     INIT          COMPANY                 FULLNAME */
COMP ( 1982, spectrum, 0,        spectrum,       spectrum, 0,            "Sinclair Research",    "ZX Spectrum" )
COMPX( 2000, specpls4, spectrum, spectrum,       spectrum, 0,            "Amstrad plc",          "ZX Spectrum +4", GAME_COMPUTER_MODIFIED )
COMPX( 1994, specbusy, spectrum, spectrum,       spectrum, 0,            "Amstrad plc",          "ZX Spectrum (BusySoft Upgrade)", GAME_COMPUTER_MODIFIED )
COMPX( ????, specgrot, spectrum, spectrum,       spectrum, 0,            "Amstrad plc",          "ZX Spectrum (De Groot's Upgrade)", GAME_COMPUTER_MODIFIED )
COMPX( 1985, specimc,  spectrum, spectrum,       spectrum, 0,            "Amstrad plc",          "ZX Spectrum (Collier's Upgrade)", GAME_COMPUTER_MODIFIED )
COMPX( 1987, speclec,  spectrum, spectrum,       spectrum, 0,            "Amstrad plc",          "ZX Spectrum (LEC Upgrade)", GAME_COMPUTER_MODIFIED )
COMP ( 1986, inves,    spectrum, spectrum,       spectrum, 0,            "Investronica",         "Inves Spectrum 48K+" )
COMP ( 1985, tk90x,    spectrum, spectrum,       spectrum, 0,            "Micro Digital",        "TK90x Color Computer" )
COMP ( 1986, tk95,     spectrum, spectrum,       spectrum, 0,            "Micro Digital",        "TK95 Color Computer" )
COMP ( 198?, tc2048,   spectrum, tc2048,         spectrum, 0,            "Timex of Portugal",    "TC2048" )
COMP ( 1983, ts2068,   spectrum, ts2068,         spectrum, 0,            "Timex Sinclair",       "TS2068" )

COMPX( 1986, spec128,  0,        spectrum_128,   spectrum, 0,            "Sinclair Research",    "ZX Spectrum 128" ,GAME_NOT_WORKING)
COMPX( 1985, spec128s, spec128,  spectrum_128,   spectrum, 0,            "Sinclair Research",    "ZX Spectrum 128 (Spain)" ,GAME_NOT_WORKING)
COMPX( 1986, specpls2, spec128,  spectrum_128,   spectrum, 0,            "Amstrad plc",          "ZX Spectrum +2" ,GAME_NOT_WORKING)
COMPX( 1987, specpl2a, spec128,  spectrum_plus3, spectrum, 0,            "Amstrad plc",          "ZX Spectrum +2a" ,GAME_NOT_WORKING)
COMPX( 1987, specpls3, spec128,  spectrum_plus3, spectrum, 0,            "Amstrad plc",          "ZX Spectrum +3" ,GAME_NOT_WORKING)

COMPX( 1986, specp2fr, spec128,  spectrum_128,   spectrum, 0,            "Amstrad plc",          "ZX Spectrum +2 (France)" ,GAME_NOT_WORKING)
COMPX( 1986, specp2sp, spec128,  spectrum_128,   spectrum, 0,            "Amstrad plc",          "ZX Spectrum +2 (Spain)" ,GAME_NOT_WORKING)
COMPX( 1987, specp3sp, spec128,  spectrum_plus3, spectrum, 0,            "Amstrad plc",          "ZX Spectrum +3 (Spain)" ,GAME_NOT_WORKING)
COMPX( 2000, specpl3e, spec128,  spectrum_plus3, spectrum, 0,            "Amstrad plc",          "ZX Spectrum +3e" , GAME_NOT_WORKING|GAME_COMPUTER_MODIFIED )
