#include <stdio.h>
#include "driver.h"
#include "includes/oric.h"

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

*/

unsigned char *oric_ram;
int oric_load_rom (int id);

unsigned char *oric_IO;
unsigned char *oric_tape_data;

int oric_countdown_1;
int oric_countdown_2;
int oric_countdown_3;

int oric_flashcount;

int oric_interrupt_counter;

unsigned char oric_portmask_a;
unsigned char oric_portmask_b;
unsigned char oric_porta;
unsigned char oric_portb;
unsigned char oric_porta_r;
unsigned char oric_portb_r;
unsigned char oric_last_porta;
unsigned char oric_last_portb;
unsigned char oric_CA1;
unsigned char oric_CA2;
unsigned char oric_CB1;
unsigned char oric_CB2;
unsigned char oric_sound_reg;
unsigned char oric_sound_data;
unsigned char oric_8910_porta;
unsigned char oric_6522_control_lines;

unsigned short oric_runadd;

char oric_checkfor_filename[20];
char oric_tape_filename[20];
int oric_tape_datasize;
int oric_tape_from_rom_patch;

int oric_tape_position;

char oric_rom_version[40];

static const char *rom_name = NULL;

void oric_init_machine (void)
{
	int i;

	//int found,f2;
	unsigned char *RAM;

	oric_IO = malloc (0x10);
	if (!oric_IO) return;
	memset (oric_IO, 0, sizeof (oric_IO));
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

	oric_countdown_1 = 100 * 4;
	oric_countdown_2 = 50 * 5;
	oric_countdown_3 = 60 * 4;

	oric_set_powerscreen_mode (1);
	oric_set_flash_show (1);
	oric_flashcount = 0;

	oric_portmask_a = 0;
	oric_portmask_b = 0;
	oric_porta = 0;
	oric_portb = 0;
	oric_porta_r = 0;
	oric_portb_r = 0;
	oric_last_porta = 0;
	oric_last_portb = 0;

	oric_6522_control_lines = 0;
	oric_CA1 = 0;
	oric_CA2 = 0;
	oric_CB1 = 0;
	oric_CB2 = 0;

	oric_sound_reg = 0;
	oric_sound_data = 0;

	oric_8910_porta = 0;

	oric_interrupt_counter = 0;

	interrupt_enable_w (0, 0);

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

	/*
	 * // temp
	 *
	 * found = 0;
	 * f2 = 0x4000;
	 * for (i = 0xc000; i < 0xffff;i++) {
	 * if (RAM[i] == 0xa8)
	 * if (RAM[i+1] == 0xe4)
	 * {
	 * found = i;
	 * RAM[f2++] = (found/256);
	 * RAM[f2++] = (found&255);
	 * }
	 * }
	 */
}

void oric_shutdown_machine (void)
{
	/* Not a Lot happens here yet ... */
	free (oric_IO);
	if (oric_tape_data != NULL)
		free (oric_tape_data);
}

#if 0
int oric_ram_r (int offset)
{
	return (oric_ram[offset]);
}

void oric_ram_w (int offset, int data)
{
	oric_ram[offset] = data;
}
#endif

READ_HANDLER ( oric_IO_r )
{
	switch( offset & 0x0f )
	{
		case 0x00:
			// return (oric_portb_r & ( oric_portmask_b ^ 0xff ) );
			return (oric_portb_r);
		case 0x01:
			return (oric_porta_r & (oric_portmask_a ^ 0xff));
		case 0x02:
			return oric_portmask_b;
		case 0x03:
			return oric_portmask_a;
		case 0x0c:
			return oric_6522_control_lines;
		case 0x0f:
			return (oric_porta_r & (oric_portmask_a ^ 0xff));
    }
	return (oric_IO[(offset & 0x0f)]);
}

/*  guesswork ..

0x01 CA1
0x02 CA2
0x10 CB1
0x20 CB2

*/

WRITE_HANDLER ( oric_IO_w )
{
//	int keypressed;
//	int kbrow, kbcol;
	int porta_write;
	unsigned char *RAM;

	RAM = memory_region(REGION_CPU1);
	oric_IO[offset & 0x0f] = data;
	porta_write = 0;
	//RAM[offset] = data; UHh?? what *WAS* i thinking!?!?!?!

	/* todo .. Bosh the appropriate bits towards the appropriate
	 * IO reg's, fire off soundchip and stuff like that ...
	 */


	/* Trap Bogus write to IO to load Tape Image!! */

	if ((offset & 0xff) == 0xfa)
	{

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


	if ((offset & 0x0f) == 0x02)
	{
		oric_portmask_b = data;
	}
	if ((offset & 0x0f) == 0x03)
	{
		oric_portmask_a = data;
	}

	if (((offset & 0x0f) == 0x01) || ((offset & 0x01) == 0x01))
	{
		oric_porta = data;
		oric_porta_r = data;
		porta_write = 1;
	}

	// if ( (offset&0x0f) == 0x0f) { oric_porta = data; oric_porta_r = data; }

	if ((offset & 0x0f) == 0x00)
	{
		// write to 6522 portb
		oric_portb = data;
		if( readinputport (1 + (data & 7)) & ~oric_8910_porta )
			oric_portb_r = data | 8;
		else
			oric_portb_r = data & ~8;
	}

	if (((offset & 0x0f) == 0x0c) || porta_write == 1)
	{
		// set 6522 control lines
		if ((offset & 0x0f) == 0x0c)
			oric_6522_control_lines = data;
		oric_CA1 = (unsigned char) 0;
		oric_CA2 = (unsigned char) 0;
		oric_CB1 = (unsigned char) 0;
		oric_CB2 = (unsigned char) 0;
		if ((oric_6522_control_lines & 0x01) != 0)
			oric_CA1 = 1;
		if ((oric_6522_control_lines & 0x02) != 0)
			oric_CA2 = 1;
		if ((oric_6522_control_lines & 0x10) != 0)
			oric_CB1 = 1;
		if ((oric_6522_control_lines & 0x20) != 0)
			oric_CB2 = 1;
		if (oric_CA2 && oric_CB2)
			oric_sound_reg = oric_porta;
		if ((oric_CB2) && !(oric_CA2))
		{
			oric_sound_data = oric_porta;
			AY8910_control_port_0_w (0, oric_sound_reg);
			AY8910_write_port_0_w (0, oric_sound_data);
			if (oric_sound_reg == 14)
			{
				// Keyboard through Port A on 8912
				oric_8910_porta = oric_sound_data;
			}
		}
	}


}

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
		// do internal timers against 100hz Clock ...

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

		// here, we shall draw a funky screen for power up
		if (oric_countdown_2 != 0)
		{
			oric_countdown_2--;
			if (oric_countdown_2 == 0)
				oric_set_powerscreen_mode (0);
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


	// Flash Attrs
	oric_flashcount++;
	if (oric_flashcount == 30)
		oric_set_flash_show (0);
	if (oric_flashcount >= 60)
	{
		oric_set_flash_show (1);
		oric_flashcount = 0;
	}

	/*
	 * set bit 0x40 in 0x030d in 6522, Interrupt enable flag,
	 * IRQ routine in rom checks this and clears it .. (Fast IRQ)
	 */


	x = oric_IO_r (0x030e);
	// x &= 0x40;
	x &= 0xf0;
	if (x == 0)
		exec_irq = 0;
	y = x;

	if (exec_irq == 1)
	{
		interrupt_enable_w (0, 1);
	}
	else
	{
		interrupt_enable_w (0, 1);
	}

	x = oric_IO_r (0x030d);
	x |= 0x40;
	x |= y;

	// x = 0x40;
	//if (oric_countdown_1 > 0) { x = 0x00; return 0; }
	//x &= 0xbf;
	oric_IO_w (0x030d, x);
	return interrupt ();
}

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
