#include <stdio.h>
#include "driver.h"
#include "includes/oric.h"
#include "includes/basicdsk.h"
#include "includes/wd179x.h"
#include "machine/6522via.h"
/*

TODO's / BUGLIST

 ** done ** Multipart loads from .TAP file  (cload patched, look for next file / filename match
 ** done ** CLOAD"" loads next file from tape image if there are any, else hang on 'Searching....'
cloads, check for autorun flag, ... to run, or not ... (need .TAP specification ...)
HIRES attrs in mid/screen   (split hires modes ...)
 ** done ** HIRES colour attributes need attention ...
 ** done ** Inverse ATTRS's .. check behaviour of colours
fix 'RUN' command for cloads to 0x0501 ... (works for cloads, not for command line yet ..
FIX any broken keyboard mappings ....
fix ROM patch to work on 1.1 roms, extraction of filename for cloads etc ..
 ** done ** set the timing of the IRQ's correctly (is set to 60hz, should be 100hz)
introduce a pause after loading individual tape files to simulate effect of loading screens ...

switch from HIRES to TEXT.. does not alter internal variables ..
the ULTRA does not run,  does this need 6522 timers?
RATSPLAT .. no colission detection?  timers?  other forms of IRQ??
emulate 6522 timers properly ...
NMI for reset button (map to f12 or something ..

low priority,

add Inport_ports structures so key info menu in mess GUI shows keys
loading tape image to update MESS GUI settings for current image

  KT:
	Added preliminary floppy disc support from Fabrice Frances document.
	rewrote code to use 6522 via.

  TODO:
  - Microdisc interface: ROMDIS and Eprom select 
  - Jasmin: overlay ram access, ROMDIS, fdc reset

*/


/* index of keyboard line to scan */
static char oric_keyboard_line;
/* sense result */
static char oric_key_sense_bit;
/* mask to read keys */
static char oric_keyboard_mask;


/* refresh keyboard sense */
static void oric_keyboard_sense_refresh(void)
{
	/* The following assumes that if a 0 is written, it can be used to detect if any key
	has been pressed.. */
	/* for each bit that is 0, it combines it's pressed state with the pressed state so far */
	
	int i;
	unsigned char key_bit = 0;

	/* what if data is 0, can it sense if any of the keys on a line are pressed? */
	int input_port_data = readinputport(1+oric_keyboard_line);

	/* go through all bits in line */
	for (i=0; i<8; i++)
	{
		/* sense this bit? */
		if (((~oric_keyboard_mask) & (1<<i))!=0)
		{
			/* is key pressed? */
			if (input_port_data & (1<<i))
			{
				/* yes */
				key_bit |= 1;
			}
		}
	}

	/* clear sense result */
	oric_key_sense_bit = 0;
	/* any keys pressed on this line? */
	if (key_bit!=0)
	{
		/* set sense result */
		oric_key_sense_bit = (1<<3);
	}
}


/* this is executed when a write to psg port a is done */
WRITE_HANDLER (oric_psg_porta_write)
{
	oric_keyboard_mask = data;

	oric_keyboard_sense_refresh();

}


/* PSG control pins */
/* bit 1 = BDIR state */
/* bit 0 = BC1 state */
static char oric_psg_control;

/* this port is also used to read printer data */
int oric_via_in_a_func(int offset)
{
	logerror("port a read\r\n");

	/* access psg? */
	if (oric_psg_control!=0)
	{
		/* if psg is in read register state return reg data */
		if (oric_psg_control==0x01)
		{
			return AY8910_read_port_0_r(0);
		}

		/* return high-impedance */
		return 0x0ff;
	}
	
	/* return printer data?? */
	return 0x0ff;
}
 
int oric_via_in_b_func(int offset)
{
	int data;

	data = oric_key_sense_bit;
	data |= oric_keyboard_line & 0x07;

	return data;
}


static unsigned char oric_via_port_a_data;

/* read/write data depending on state of bdir, bc1 pins and data output to psg */
static void oric_psg_connection_refresh(void)
{
	if (oric_psg_control!=0)
	{
		switch (oric_psg_control)
		{
			/* write register data */
			case 2:
			{
				AY8910_write_port_0_w (0, oric_via_port_a_data);
			}
			break;
			/* write register index */
			case 3:
			{
				AY8910_control_port_0_w (0, oric_via_port_a_data);
			}
			break;

			default:
				break;
		}
	
		return;
	}
}

void oric_via_out_a_func(int offset, int data)
{
	oric_via_port_a_data = data;

	/* access printer? */
}

/*
PB0..PB2
 keyboard lines-demultiplexer
 
PB3
 keyboard sense line
 
PB4
 printer strobe line
 
PB5
 (not connected)
 
PB6
 tape connector motor control
 
PB7
 tape connector output

 */

int oric_via_in_cb1_func(int offset)
{
	int data;

	data = 0;

	if (device_input(IO_CASSETTE,0) > 255)
		data |=1;

	return data;
}



void oric_via_out_b_func(int offset, int data)
{
	oric_keyboard_line = data & 0x07;

	/* cassette motor control */
	device_status(IO_CASSETTE, 0, ((data>>6) & 0x01));

	/* cassette data out */
	device_output(IO_CASSETTE, 0, (data & (1<<7)) ? -32768 : 32767);

	oric_psg_connection_refresh();
	oric_keyboard_sense_refresh();
}

int oric_via_in_ca2_func(int offset)
{
	return oric_psg_control & 1;
}

int oric_via_in_cb2_func(int offset)
{
	return (oric_psg_control>>1) & 1;
}

void oric_via_out_ca2_func(int offset, int val)
{
	oric_psg_control &=~1;

	if (val)
	{
		oric_psg_control |=1;
	}

	oric_psg_connection_refresh();
}

void oric_via_out_cb2_func(int offset, int val)
{
	oric_psg_control &=~2;

	if (val)
	{
		oric_psg_control |=2;
	}

	oric_psg_connection_refresh();
}

void	oric_via_irq_func(int state)
{
	if (state)
	{
		cpu_set_irq_line(0,0, HOLD_LINE);
	}
	else
	{
		cpu_set_irq_line(0,0, CLEAR_LINE);
	}
}


/*
VIA Lines
 Oric usage
 
PA0..PA7
 PSG data bus, printer data lines
 
CA1
 printer acknowledge line
 
CA2
 PSG BC1 line
 
PB0..PB2
 keyboard lines-demultiplexer
 
PB3
 keyboard sense line
 
PB4
 printer strobe line
 
PB5
 (not connected)
 
PB6
 tape connector motor control
 
PB7
 tape connector output
 
CB1
 tape connector input
 
CB2
 PSG BDIR line
 
*/

struct via6522_interface oric_6522_interface=
{
	oric_via_in_a_func,
	oric_via_in_b_func,
	NULL,				/* printer acknowledge */
	oric_via_in_cb1_func,
	oric_via_in_ca2_func,
	oric_via_in_cb2_func,
	oric_via_out_a_func,
	oric_via_out_b_func,
	oric_via_out_ca2_func,
	oric_via_out_cb2_func,
	oric_via_irq_func,
	NULL,
	NULL,
	NULL,
	NULL
};


/* if defined, enable microdisc floppy disc interface */
/* if not defined, enable Jasmin floppy disc interface */
#define MICRODISC_FLOPPY_DISC_INTERFACE	


/* following are used for tape and need cleaning up */
int oric_interrupt_counter;
int oric_countdown_1;
unsigned char *oric_tape_data;
unsigned short oric_runadd;
char oric_checkfor_filename[20];
char oric_tape_filename[20];
int oric_tape_datasize;
int oric_tape_from_rom_patch;
int oric_tape_position;
char oric_rom_version[40];
/*static const char *rom_name = NULL; */


#ifdef MICRODISC_FLOPPY_DISC_INTERFACE
/* bit 7 is intrq state */
unsigned char port_314_r;
/* bit 7 is drq state (active low) */
unsigned char port_318_r;
/* bit 6,5: drive */
/* bit 4: side */
/* bit 3: double density enable */
/* bit 0: enable FDC IRQ to trigger IRQ on CPU */
unsigned char port_314_w;
#endif


static void oric_wd179x_callback(int State)
{
	switch (State)
	{
		case WD179X_IRQ_CLR:
		{
#ifdef MICRODISC_FLOPPY_DISC_INTERFACE
			port_314_r &=~(1<<7);

			if (port_314_w & (1<<0))
			{
				cpu_set_irq_line(0,0,CLEAR_LINE);
			}
#endif
		}
		break;

		case WD179X_IRQ_SET:
		{
#ifdef MICRODISC_FLOPPY_DISC_INTERFACE
			port_314_r |= (1<<7);

			if (port_314_r & (1<<0))
			{
				cpu_set_irq_line(0,0,HOLD_LINE);
			}
#endif
		}
		break;

		case WD179X_DRQ_CLR:
		{
#ifdef MICRODISC_FLOPPY_DISC_INTERFACE
			port_318_r |= (1<<7);
#else
			/* on Jasmin interface, DRQ is connected to IRQ on CPU */
			cpu_set_irq_line(0,0, CLEAR_LINE);
#endif
		}
		break;

		case WD179X_DRQ_SET:
		{
#ifdef MICRODISC_FLOPPY_DISC_INTERFACE
			port_318_r &=~(1<<7);
#else
			/* on Jasmin interface, DRQ is connected to IRQ on CPU */
			cpu_set_irq_line(0,0, HOLD_LINE);
#endif
		}
		break;
	}
}

int oric_floppy_init(int id)
{
	if (basicdsk_floppy_init(id))
	{
		/* I don't know what the geometry of the disc image should be, so the
		default is 80 tracks, 2 sides, 9 sectors per track */
		basicdsk_set_geometry(id, 80, 2, 9, 512, 1);

		return INIT_OK;
	}

	return INIT_FAILED;
}


void oric_init_machine (void)
{
	int i;

	//int found,f2;
	unsigned char *RAM;

	RAM = memory_region(REGION_CPU1);
	/*
	 * gives those delightfull pages of 'U's when programms go belly-up!
	 * The Kernal ROM does this as one of its first boot procedures.
	 * we shall have to patch that out of the rom as it kicks the rom
	 * once oric_load_rom() has loaded its rom, so we dont want the kernal
	 * to splosh 0x55's all over our nice new image ...
	 */
	for (i = 0; i <= 0xbfff; i++)
		RAM[i] = 0x55;

	/* Boot ROM Patches ..
	 * Patch 1 .. Temporary untill i work our how the timers and stuff
	 * work,  the kernal sticks some values in memory at about 0x270
	 * and seems to wait for it to hit zero before proceeding ..
	 * perhapps its sound HW initializing? perhapps some weird timer?
	 * who knows!!!!
	 * we wont need this patch once ive got the IO kicking properly ..
	 */

	// dont need this patch, enabling IRQ's takes care of it ...
	//RAM[0xedb5] = 0xea;
	//RAM[0xedb6] = 0xea;
	//RAM[0xedb9] = 0xea;
	//RAM[0xedba] = 0xea;


	// oric_load_rom(); # do this after a few seconds on a timer ..

	/* Patch ROM
	 * patch the rom's 'CLOAD' routine to push a write into a bogus IO region
	 * trap this IO Write and action a tape image load ...
	 *
	 * Oric1  (1.0 Rom)
	 *
	 * $e4b2 :: LDA #$69;STA $03FA;RTS    (a9 69 8d fa 03 60)
	 *
	 * Oric Atmos (1.1 Rom)
	 *
	 * $e4b2 :: LDA #$69;STA $03FA;RTS    (a9 69 8d fa 03 60)
	 *
	 */

	RAM[0xe4b2] = 0xa9;
	RAM[0xe4b3] = 0x69;
	RAM[0xe4b4] = 0x8d;
	RAM[0xe4b5] = 0xfa;
	RAM[0xe4b6] = 0x03;
	RAM[0xe4b7] = 0x60;

	RAM[0xe7b6] = 0xa9;
	RAM[0xe7b7] = 0x69;
	RAM[0xe7b8] = 0x8d;
	RAM[0xe7b9] = 0xfa;
	RAM[0xe7ba] = 0x03;
	RAM[0xe7bb] = 0x28;
	RAM[0xe7bc] = 0x4c;
	RAM[0xe7bd] = 0xc8;
	RAM[0xe7be] = 0xe7;

	oric_tape_from_rom_patch = 0;
	oric_tape_data = 0;

	strcpy (oric_rom_version, "1.0");

	// V1.1 ???

	if (RAM[0xedaa] == 0x46)
		if (RAM[0xedab] == 0x31)
			if (RAM[0xedac] == 0x2e)
				if (RAM[0xedad] == 0x31)
					if (RAM[0xedae] == 0x0d)
						if (RAM[0xedaf] == 0x0a)
							strcpy (oric_rom_version, "1.1");


	via_config(0, &oric_6522_interface);
	via_set_clock(0,1000000);
	via_reset();


	wd179x_init(oric_wd179x_callback);
}

void oric_shutdown_machine (void)
{
	wd179x_exit();
}


READ_HANDLER ( oric_IO_r )
{
	unsigned char data;

	/* it is repeated 16 times */
	data = via_0_r(offset & 0x0f);
	
#if 0

	switch (offset & 0x0ff)
	{
#ifdef MICRODISC_FLOPPY_DISC_INTERFACE
		/* microdisc floppy disc interface */
		case 0x010:
			data = wd179x_status_r(0);
		case 0x011:
			data =wd179x_track_r(0);
		case 0x012:
			data = wd179x_sector_r(0);
		case 0x013:
			data = wd179x_data_r(0);
		case 0x014:
			data = port_314_r;
		case 0x018:
			data = port_318_r;
#else
		/* Jasmin floppy disc interface */
		case 0x0f4:
			data = wd179x_status_r(0);
		case 0x0f5:
			data = wd179x_track_r(0);
		case 0x0f6:
			data = wd179x_sector_r(0);
		case 0x0f7:
			data = wd179x_data_r(0);
#endif
	}
#endif

	return data;
}

WRITE_HANDLER ( oric_IO_w )
{
	/* Trap Bogus write to IO to load Tape Image!! */

	if ((offset & 0xff) == 0xfa)
	{
		unsigned char *RAM;
	
		RAM = memory_region(REGION_CPU1);

		if (data == 0x69)
		{
			// CLOAD ROM PATCH HAS HIT HERE ....

			int fnadd;
			int i;

			// *TODO*  .. change addr of filename for ATMOS rom..

			fnadd = 0x35;
			if (!strcmp (oric_rom_version, "1.1"))
				fnadd = 0x27f;

			i = 0;
			while (RAM[fnadd] != 0)
				oric_checkfor_filename[i++] = RAM[fnadd++];
			oric_runadd = 0;

			oric_tape_from_rom_patch = 1;

			if (!oric_extract_file_from_tape (0))
			{
				device_filename_change(IO_CARTSLOT,0,oric_checkfor_filename);
				if (!oric_load_rom(0))
				{
                    oric_checkfor_filename[i++] = '.';
                    oric_checkfor_filename[i++] = 't';
                    oric_checkfor_filename[i++] = 'a';
                    oric_checkfor_filename[i++] = 'p';
                    oric_checkfor_filename[i++] = 0;
					device_filename_change(IO_CARTSLOT,0,oric_checkfor_filename);
					oric_load_rom(0);
				}
			}
			// if (oric_runadd != 0) cpu_set_pc(oric_runadd);
			return;
		}
	}

	/* it is repeated 16 times, but for now only use first few entries */
	via_0_w(offset & 0x0f,data);

	switch (offset & 0x0ff)
	{
#ifdef MICRODISC_FLOPPY_DISC_INTERFACE
		/* microdisc floppy disc interface */
		case 0x010:
			wd179x_command_w(0,data);
			break;
		case 0x011:
			wd179x_track_w(0,data);
			break;
		case 0x012:
			wd179x_sector_w(0,data);
			break;
		case 0x013:
			wd179x_data_w(0,data);
			break;
		case 0x014:
		{
			int density;

			port_314_w = data;

			/* bit 6,5: drive */
			/* bit 4: side */
			/* bit 3: double density enable */
			/* bit 0: enable FDC IRQ to trigger IRQ on CPU */
			wd179x_set_drive((data>>6) & 0x03);
			wd179x_set_side((data>>4) & 0x01);
			if (data & (1<<3))
			{
				density = DEN_MFM_LO;
			}
			else
			{
				density = DEN_FM_HI;
			}

			wd179x_set_density(density);
		}
		break;		
#else
		/* Jasmin floppy disc interface */
		case 0x0f4:
			wd179x_command_w(0);
			break;
		case 0x0f5:
			wd179x_track_w(0);
			break;
		case 0x0f6:
			wd179x_sector_w(0);
			break;
		case 0x0f7:
			wd179x_data_w(0);
			break;
		case 0x0f8:
		{
			/* bit 0: side select */
			wd179x_set_side(data & 0x01);
		}
		break;

		/* port 0x03fc selects drive 0, port 0x03fd selects drive 1.. */
		case 0x0fc:
		case 0x0fd:
		case 0x0fe:
		case 0x0ff:
			wd179x_set_drive(offset & 0x03);
			break;
#endif		

	}
}

#if 0
int oric_interrupt (void)
{

	// Not Emulated .. Variable IRQ Rates,  100hz

	/* Not a lot happening right now .... */

	/* Probably use a timed trigger here to splosh the tape
	 * image back into ram once the kernal has done its bits (give it 2
	 * or 3 seconds...  will also use this to draw that real funky power
	 * on screen (must be generated in HW on the real thing, the kernal
	 * does nothing of the sort .... */

	/*
	 *
	 * Defaults (altered by setting timers on 6522)
	 * Fast IRQ 100 hz
	 * Slow IRQ 1 hz
	 *
	 */

	unsigned char *RAM;
	int x, y;
	int exec_irq;
	int exec_slow_irq;

	exec_irq = 0;
	exec_slow_irq = 0;

	oric_interrupt_counter++;
	if (oric_interrupt_counter == 6)
	{
		exec_slow_irq = 1;
		oric_interrupt_counter = 0;
	}

	if (exec_slow_irq == 0)
		return 0;

	exec_irq = 1;

	RAM = memory_region(REGION_CPU1);

	if (exec_irq == 1)
	{
		/* once this timer has dec'd we can splosh the tape image in
		 * once the kernal has set up memory the way it likes it ...
		 * saves patching the kernal too much ... */
		if (oric_countdown_1 != 0)
		{
			oric_countdown_1--;
			if (oric_countdown_1 == 0)
			{
				device_filename_change(IO_CASSETTE,0,rom_name);
                // Try to load tape image and execute it ...
				if (!oric_load_rom (0))
				{
					if (oric_runadd == 0x5555)
						oric_runadd = 0;
					if (oric_runadd != 0)
						cpu_set_pc (oric_runadd);
				}
			}
		}

		// kick Interrupts back in
		// nasty hack .. will hang this off some 6522 stuff later
		//if (oric_countdown_3 != 0) {
		//        oric_countdown_3--;
		//        if (oric_countdown_3 == 0) interrupt_enable_w(0,1);
		//}

		/*
		 * if (keyboard_pressed(KEYCODE_DEL)) {
		 * unsigned char m,l;
		 * unsigned short o;
		 * l = RAM[0xfffc];
		 * m = RAM[0xfffd];
		 * o = (m * 0x100) + l;
		 * cpu_set_pc(o);
		 * return 0;
		 * }
		 */
	}

	return ignore_interrupt();
}
#endif

int oric_extract_file_from_tape (int filenum)
{
	// Find Loading Address from TAP Image ...
	// Splosh the Data into RAM ...

	int z, fn;
	int o;

	// int i;
	int notfinished;
	int endadd;
	unsigned char msblad, lsblad;
	unsigned char msblend, lsblend;
	unsigned char *RAM;

	RAM = memory_region(REGION_CPU1);

	if (oric_tape_data == NULL)
		return 0;					   // no tape image in memory yet ..

	if (oric_tape_position + 10 > oric_tape_datasize)
		return 0;					   // We've Hit the END!!!

	fn = 0;
	z = oric_tape_position + 0x08;
	msblend = oric_tape_data[z++];
	lsblend = oric_tape_data[z++];
	msblad = oric_tape_data[z++];
	lsblad = oric_tape_data[z++];
	z++;

	while ((unsigned char) oric_tape_data[z++] != (unsigned char) 0)
		oric_tape_filename[fn++] = oric_tape_data[z - 1];
	oric_tape_filename[fn] = 0;
	// if (z == 13) z++; // .TAP files with no filename have 2 0x00's
	o = (msblad * 0x100) + lsblad;
	oric_runadd = o;
	//for (i=z;i<oric_tape_datasize;i++) RAM[o++] = oric_tape_data[i];
	//i=z;
	endadd = (msblend * 0x100) + lsblend;
	notfinished = 1;
	while (notfinished == 1)
	{
		RAM[o++] = oric_tape_data[z++];
		if (z > oric_tape_datasize)
			notfinished = 0;
		if (o >= endadd)
			notfinished = 0;
	}
	oric_tape_position = z + 1;
	o += 2;
	msblend = (o / 0x100);
	lsblend = (o & 0xff);
	RAM[0x9c] = lsblend;
	RAM[0x9d] = msblend;
	RAM[0x9e] = lsblend;
	RAM[0x9f] = msblend;
	RAM[0xa0] = lsblend;
	RAM[0xa1] = msblend;
	RAM[0x5f] = lsblad;
	RAM[0x60] = msblad;
	RAM[0x61] = lsblend;
	RAM[0x62] = msblend;
	RAM[0x63] = 0xff;				   // autorun flag
	RAM[0x64] = 0xff;				   // basic or MC flag
	if (oric_runadd == 0x501)
		RAM[0x64] = 0;
	// if (oric_runadd == 0x501) RAM[0x63]=0;

	// 0xc773 -- list;
	// 0xc733 -- run;
	// 0xc708 -- run (Atmos 1.1 rom)
	if (oric_runadd == 0x501)
		oric_runadd = 0xc96b;

	return 1;
}

int oric_load_rom(int id)
{
	void *file;

    if (oric_tape_from_rom_patch == 0)
	{
		file = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_RW, 0);
	}
	else
	{
		file = image_fopen (IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, 0);
	}
	//file = osd_fopen(Machine->gamedrv->name, "ultra.tap", OSD_FILETYPE_IMAGE_RW, 0);
	//file = osd_fopen(Machine->gamedrv->name, "blitz.tap", OSD_FILETYPE_IMAGE_RW, 0);
	//file = osd_fopen(Machine->gamedrv->name, "hopper.tap", OSD_FILETYPE_IMAGE_RW, 0);
	//file = osd_fopen(Machine->gamedrv->name, "ratsplat.tap", OSD_FILETYPE_IMAGE_RW, 0);

	if (file)
	{
		oric_tape_datasize = osd_fsize (file);

		if (oric_tape_datasize != 0)
		{
			if (oric_tape_data == NULL)
				oric_tape_data = malloc (oric_tape_datasize);

			if (oric_tape_data != NULL)
			{
				osd_fread (file, oric_tape_data, oric_tape_datasize);
				osd_fclose (file);

				oric_tape_position = 0;

				oric_checkfor_filename[0] = 0;
				oric_extract_file_from_tape (0);

				// free(data);
				return 0;
			}
			else
			{
				osd_fclose (file);
				return 1;			   // noway jose!!!
			}
		}

	}

	return 1;

}


int oric_cassette_init(int id)
{
	void *file;

	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
	if (file)
	{
		struct wave_args wa = {0,};
		wa.file = file;
		wa.display = 1;

		if (device_open(IO_CASSETTE, id, 0, &wa))
			return INIT_FAILED;

		return INIT_OK;
	}

	/* HJB 02/18: no file, create a new file instead */
	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_WRITE);
	if (file)
	{
		struct wave_args wa = {0,};
		wa.file = file;
		wa.display = 1;
		wa.smpfreq = 22050; /* maybe 11025 Hz would be sufficient? */
		/* open in write mode */
        if (device_open(IO_CASSETTE, id, 1, &wa))
            return INIT_FAILED;
		return INIT_OK;
    }

	return INIT_FAILED;
}

void oric_cassette_exit(int id)
{
	device_close(IO_CASSETTE, id);
}
