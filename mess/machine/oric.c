/*********************************************************************

	machine/oric.c

	Paul Cook
	Kev Thacker

	Thankyou to Fabrice Frances for his ORIC documentation which helped with this driver
	http://oric.ifrance.com/oric/

	TODO:
	- there are problems loading some .wav's. Try to fix these.
	- fix more graphics display problems
	- check the printer works
	- fix more disc drive/wd179x problems so more software will run

*********************************************************************/


#include <stdio.h>
#include "driver.h"
#include "includes/oric.h"
#include "includes/wd179x.h"
#include "machine/6522via.h"
#include "includes/6551.h"
#include "includes/centroni.h"
#include "devices/basicdsk.h"
#include "devices/mfmdisk.h"
#include "devices/printer.h"
#include "devices/cassette.h"
#include "image.h"

static int enable_logging = 0;
/* static int save_done = 0; */


/* ==0 if oric1 or oric atmos, !=0 if telestrat */
static int oric_is_telestrat = 0;

/* This does not exist in the real hardware. I have used it to
know which sources are interrupting */
/* bit 2 = telestrat 2nd via interrupt, 1 = microdisc interface,
0 = oric 1st via interrupt */
static unsigned char oric_irqs;

enum
{
	ORIC_FLOPPY_NONE,
	ORIC_FLOPPY_MFM_DISK,
	ORIC_FLOPPY_BASIC_DISK
};

/* type of disc interface connected to oric/oric atmos */
/* telestrat always has a microdisc interface */
enum
{
	ORIC_FLOPPY_INTERFACE_NONE = 0,
	ORIC_FLOPPY_INTERFACE_MICRODISC = 1,
	ORIC_FLOPPY_INTERFACE_JASMIN = 2,
	ORIC_FLOPPY_INTERFACE_APPLE2 = 3,
	ORIC_FLOPPY_INTERFACE_APPLE2_V2 = 4
};

/* called when ints are changed - cleared/set */
static void oric_refresh_ints(void)
{
	/* telestrat has floppy hardware built-in! */
	if (oric_is_telestrat==0)
	{
		/* oric 1 or oric atmos */

		/* if floppy disc hardware is disabled, do not allow interrupts from it */
		if ((readinputport(9) & 0x07)==ORIC_FLOPPY_INTERFACE_NONE)
		{
			oric_irqs &=~(1<<1);
		}
	}

	/* any irq set? */
	if ((oric_irqs & 0x0f)!=0)
	{
		cpu_set_irq_line(0,0, HOLD_LINE);
	}
	else
	{
		cpu_set_irq_line(0,0, CLEAR_LINE);
	}
}

static	char *oric_ram_0x0c000 = NULL;


/* index of keyboard line to scan */
static char oric_keyboard_line;
/* sense result */
static char oric_key_sense_bit;
/* mask to read keys */
static char oric_keyboard_mask;


static unsigned char oric_via_port_a_data;

#define ORIC_DUMP_RAM


#ifdef ORIC_DUMP_RAM
static int previous_input_port5;
#endif



#ifdef ORIC_DUMP_RAM
/* load image */
static void oric_dump_ram(void)
{
	mame_file *file;

	file = mame_fopen(Machine->gamedrv->name, "oricram.bin", FILETYPE_MEMCARD,OSD_FOPEN_WRITE);

	if (file)
	{
		int i;
		for (i=0; i<65536; i++)
		{
			char data;

			data = program_read_byte(i);

			mame_fwrite(file, &data, 1);

		}

		/* close file */
		mame_fclose(file);
	}
}

static void    oric_dump_video(void)
{
        FILE *fh;

        fh = fopen("e:\\oricvid.txt","wb");

        if (fh!=NULL)
        {
                int i,j, addr;

                addr = 0x08000;
                for (j=0; j<1024; j++)
                {

			fprintf(fh,"%04x: ",addr);

                for (i=0; i<16; i++)
                {
                  fprintf(fh,"%02x ",program_read_byte(addr+i));
                }
		    fprintf(fh,"   ");
		    for (i=0; i<16; i++)
			{
				int code;

				code = program_read_byte(addr+i);

				if ((code<32) || (code>127))
				{
					code = '.';
				}

				fprintf(fh,"%c",code);
			}
			addr+=16;
                fprintf(fh,"\r\n");

                }


                fclose(fh);
        }
}

#endif
/* refresh keyboard sense */
static void oric_keyboard_sense_refresh(void)
{
	/* The following assumes that if a 0 is written, it can be used to detect if any key
	has been pressed.. */
	/* for each bit that is 0, it combines it's pressed state with the pressed state so far */

	int i;
	unsigned char key_bit = 0;


	/* what if data is 0, can it sense if any of the keys on a line are pressed? */
	int input_port_data;

 	input_port_data = readinputport(1+oric_keyboard_line);

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
}


/* PSG control pins */
/* bit 1 = BDIR state */
/* bit 0 = BC1 state */
static char oric_psg_control;

/* this port is also used to read printer data */
static READ_HANDLER ( oric_via_in_a_func )
{
	/*logerror("port a read\r\n"); */

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

	/* correct?? */
	return oric_via_port_a_data;
}

static READ_HANDLER ( oric_via_in_b_func )
{
	int data;

	oric_keyboard_sense_refresh();

	data = oric_key_sense_bit;
	data |= oric_keyboard_line & 0x07;

	return data;
}


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

static WRITE_HANDLER ( oric_via_out_a_func )
{
	oric_via_port_a_data = data;

	oric_psg_connection_refresh();


	if (oric_psg_control==0)
	{
		/* if psg not selected, write to printer */
		centronics_write_data(0,data);
	}
}

/*
PB0..PB2
 keyboard lines-demultiplexer line 7

PB3
 keyboard sense line 0

PB4
 printer strobe line 1

PB5
 (not connected) ?? 1

PB6
 tape connector motor control 0

PB7 
 tape connector output high 1

 */


static mess_image *cassette_device_image(void)
{
	return image_from_devtype_and_index(IO_CASSETTE, 0);
}

/* not called yet - this will update the via with the state of the tape data.
This allows the via to trigger on bit changes and issue interrupts */
static void oric_refresh_tape(int dummy)
{
	int data;
	int input_port_9;

	data = 0;

	if (cassette_input(cassette_device_image()) > 0.0038)
		data |= 1;

	/* "A simple cable to catch the vertical retrace signal !
		This cable connects the video output for the television/monitor
	to the via cb1 input. Interrupts can be generated from the vertical
	sync, and flicker free games can be produced */

	input_port_9 = readinputport(9);
	/* cable is enabled? */
	if ((input_port_9 & 0x08)!=0)
	{
		/* return state of vsync */
		data = input_port_9>>4;
	}

	via_set_input_cb1(0, data);
}

static unsigned char previous_portb_data = 0;
static WRITE_HANDLER ( oric_via_out_b_func )
{
	int printer_handshake;

	/* KEYBOARD */
	oric_keyboard_line = data & 0x07;

	/* CASSETTE */
	/* cassette motor control */
	if ((previous_portb_data^data) & (1<<6))
	{
		if (data & (1<<6))
			enable_logging = 1;
	}

	cassette_change_state(
		cassette_device_image(),
		(data & 0x40) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,
		CASSETTE_MOTOR_DISABLED);

	/* cassette data out */
	cassette_output(cassette_device_image(), (data & (1<<7)) ? -1.0 : +1.0);


	/* PRINTER STROBE */
	printer_handshake = 0;

	/* normal value is 1, 0 is the strobe */
	if ((data & (1<<4))!=0)
	{
		printer_handshake = CENTRONICS_STROBE;
	}

	/* assumption: select is tied low */
	centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);
	centronics_write_handshake(0, printer_handshake, CENTRONICS_STROBE);

	oric_psg_connection_refresh();
	previous_portb_data = data;

}

static void oric_printer_handshake_in(int number, int data, int mask)
{
	int acknowledge;

	acknowledge = 1;

	if (mask & CENTRONICS_ACKNOWLEDGE)
	{
		if (data & CENTRONICS_ACKNOWLEDGE)
		{
			acknowledge = 0;
		}
	}

    via_set_input_ca1(0, acknowledge);
}

static CENTRONICS_CONFIG oric_cent_config[1]={
	{
		PRINTER_CENTRONICS,
		oric_printer_handshake_in
	},
};


static READ_HANDLER ( oric_via_in_ca2_func )
{
	return oric_psg_control & 1;
}

static READ_HANDLER ( oric_via_in_cb2_func )
{
	return (oric_psg_control>>1) & 1;
}

static WRITE_HANDLER ( oric_via_out_ca2_func )
{
	oric_psg_control &=~1;

	if (data)
	{
		oric_psg_control |=1;
	}

	oric_psg_connection_refresh();
}

static WRITE_HANDLER ( oric_via_out_cb2_func )
{
	oric_psg_control &=~2;

	if (data)
	{
		oric_psg_control |=2;
	}

	oric_psg_connection_refresh();
}


static void	oric_via_irq_func(int state)
{
	oric_irqs &= ~(1<<0);

	if (state)
	{
		if (enable_logging)
		{
			logerror("oric via1 interrupt\r\n");
		}

		oric_irqs |=(1<<0);
	}

	oric_refresh_ints();
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
	NULL,				/* printer acknowledge - handled by callback*/
	NULL,				/* tape input - handled by timer */
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




/*********************/
/* APPLE 2 INTERFACE */

/*
apple2 disc drive accessed through 0x0310-0x031f (read/write)
oric via accessed through 0x0300-0x030f. (read/write)
disk interface rom accessed through 0x0320-0x03ff (read only)

CALL &320 to start, or use BOBY rom.
*/

extern READ_HANDLER ( apple2_c0xx_slot6_r );
extern WRITE_HANDLER ( apple2_c0xx_slot6_w );
extern void apple2_slot6_init(void);
extern void apple2_slot6_stop(void);

static READ_HANDLER(apple2_interface_r)
{
	/*logerror("apple 2 interface r: %04x\n",offset); */

	return apple2_c0xx_slot6_r(offset & 0x0f);
}

static WRITE_HANDLER(apple2_interface_w)
{
	/*logerror("apple 2 interface w: %04x %02x\n",offset,data); */

	apple2_c0xx_slot6_w(offset & 0x0f, data);
}

static void oric_install_apple2_interface(void)
{
	install_mem_read_handler(0, 0x0300, 0x030f, oric_IO_r);
	install_mem_read_handler(0, 0x0310, 0x031f, apple2_interface_r);
	install_mem_read_handler(0, 0x0320, 0x03ff, MRA8_BANK4);

	install_mem_write_handler(0, 0x0300, 0x030f, oric_IO_w);
	install_mem_write_handler(0, 0x0310, 0x031f, apple2_interface_w);
	cpu_setbank(4, 	memory_region(REGION_CPU1) + 0x014000 + 0x020);

}

#if 0
static void oric_uninstall_apple2_interface(void)
{


}
#endif

/************************/
/* APPLE 2 INTERFACE V2 */

/*
apple2 disc drive accessed through 0x0310-0x031f (read/write)
oric via accessed through 0x0300-0x030f. (read/write)
disk interface rom accessed through 0x0320-0x03ff (read only)
v2 registers accessed through 0x0380-0x0383 (write only)

CALL &320 to start, or use BOBY rom.
*/

static WRITE_HANDLER(apple2_v2_interface_w)
{
	/* data is ignored, address is used to decode operation */

/*	logerror("apple 2 interface v2 rom page: %01x\n",(offset & 0x02)>>1); */

	/* bit 0 is 0 for page 0, 1 for page 1 */
	cpu_setbank(4, memory_region(REGION_CPU1) + 0x014000 + 0x0100 + (((offset & 0x02)>>1)<<8));

	/* bit 1 is 0, rom enabled, bit 1 is 1 ram enabled */
	if ((offset & 0x01)==0)
	{
		unsigned char *rom_ptr;

		/* logerror("apple 2 interface v2: rom enabled\n"); */

		/* enable rom */
		memory_set_bankhandler_r(1, 0, MRA8_BANK1);
		memory_set_bankhandler_r(2, 0, MRA8_BANK2);
		memory_set_bankhandler_r(3, 0, MRA8_BANK3);
		memory_set_bankhandler_w(5, 0, MWA8_BANK5);
		memory_set_bankhandler_w(6, 0, MWA8_BANK6);
		memory_set_bankhandler_w(7, 0, MWA8_BANK7);

		rom_ptr = memory_region(REGION_CPU1) + 0x010000;
		cpu_setbank(1, rom_ptr);
		cpu_setbank(2, rom_ptr+0x02000);
		cpu_setbank(3, rom_ptr+0x03800);
		cpu_setbank(5, oric_ram_0x0c000);
		cpu_setbank(6, oric_ram_0x0c000+0x02000);
		cpu_setbank(7, oric_ram_0x0c000+0x03800);
	}
	else
	{
		/*logerror("apple 2 interface v2: ram enabled\n"); */

		/* enable ram */
		memory_set_bankhandler_r(1, 0, MRA8_BANK1);
		memory_set_bankhandler_r(2, 0, MRA8_BANK2);
		memory_set_bankhandler_r(3, 0, MRA8_BANK3);
		memory_set_bankhandler_w(5, 0, MWA8_BANK5);
		memory_set_bankhandler_w(6, 0, MWA8_BANK6);
		memory_set_bankhandler_w(7, 0, MWA8_BANK7);

		cpu_setbank(1, oric_ram_0x0c000);
		cpu_setbank(2, oric_ram_0x0c000+0x02000);
		cpu_setbank(3, oric_ram_0x0c000+0x03800);
		cpu_setbank(5, oric_ram_0x0c000);
		cpu_setbank(6, oric_ram_0x0c000+0x02000);
		cpu_setbank(7, oric_ram_0x0c000+0x03800);
	}
}


/* APPLE 2 INTERFACE V2 */
static void oric_install_apple2_v2_interface(void)
{
	install_mem_read_handler(0, 0x0300, 0x030f, oric_IO_r);
	install_mem_read_handler(0, 0x0310, 0x031f, apple2_interface_r);
	install_mem_read_handler(0, 0x0320, 0x03ff, MRA8_BANK4);

	install_mem_write_handler(0, 0x0300, 0x030f, oric_IO_w);
	install_mem_write_handler(0, 0x0310, 0x031f, apple2_interface_w);
	install_mem_write_handler(0, 0x0380, 0x0383, apple2_v2_interface_w);

	apple2_v2_interface_w(0,0);
}

#if 0
static void oric_uninstall_apple2_v2_interface(void)
{


}
#endif

/********************/
/* JASMIN INTERFACE */


/* bit 0: overlay ram access (1 means overlay ram enabled) */
static unsigned char port_3fa_w;

/* bit 0: ROMDIS (1 means internal Basic rom disabled) */
static unsigned char port_3fb_w;


static void oric_jasmin_set_mem_0x0c000(void)
{
	/* assumption:
	1. It is possible to access all 16k overlay ram.
	2. If os is enabled, and overlay ram is enabled, all 16k can be accessed.
	3. if os is disabled, and overlay ram is enabled, jasmin rom takes priority.
	*/

	/* the ram is disabled in the jasmin rom which indicates that jasmin takes
	priority over the ram */

	/* basic rom disabled? */
	if ((port_3fb_w & 0x01)==0)
	{
		/* no, it is enabled! */

		/* overlay ram enabled? */
		if ((port_3fa_w & 0x01)==0)
		{
			unsigned char *rom_ptr;

			/* no it is disabled */
			/*logerror("&c000-&ffff is os rom\n"); */

			memory_set_bankhandler_r(1, 0, MRA8_BANK1);
			memory_set_bankhandler_r(2, 0, MRA8_BANK2);
			memory_set_bankhandler_r(3, 0, MRA8_BANK3);
			memory_set_bankhandler_w(5, 0, MWA8_NOP);
			memory_set_bankhandler_w(6, 0, MWA8_NOP);
			memory_set_bankhandler_w(7, 0, MWA8_NOP);

			rom_ptr = memory_region(REGION_CPU1) + 0x010000;
			cpu_setbank(1, rom_ptr);
			cpu_setbank(2, rom_ptr+0x02000);
			cpu_setbank(3, rom_ptr+0x03800);
		}
		else
		{
			/*logerror("&c000-&ffff is ram\n"); */

			memory_set_bankhandler_r(1, 0, MRA8_BANK1);
			memory_set_bankhandler_r(2, 0, MRA8_BANK2);
			memory_set_bankhandler_r(3, 0, MRA8_BANK3);
			memory_set_bankhandler_w(5, 0, MWA8_BANK5);
			memory_set_bankhandler_w(6, 0, MWA8_BANK6);
			memory_set_bankhandler_w(7, 0, MWA8_BANK7);

			cpu_setbank(1, oric_ram_0x0c000);
			cpu_setbank(2, oric_ram_0x0c000+0x02000);
			cpu_setbank(3, oric_ram_0x0c000+0x03800);
			cpu_setbank(5, oric_ram_0x0c000);
			cpu_setbank(6, oric_ram_0x0c000+0x02000);
			cpu_setbank(7, oric_ram_0x0c000+0x03800);
		}
	}
	else
	{
		/* yes, basic rom is disabled */

		if ((port_3fa_w & 0x01)==0)
		{
			/* overlay ram disabled */

			/*logerror("&c000-&f8ff is nothing!\n"); */

			memory_set_bankhandler_r(1, 0, MRA8_NOP);
			memory_set_bankhandler_r(2, 0, MRA8_NOP);
			memory_set_bankhandler_r(3, 0, MRA8_NOP);
			memory_set_bankhandler_w(5, 0, MWA8_NOP);
			memory_set_bankhandler_w(6, 0, MWA8_NOP);
			memory_set_bankhandler_w(7, 0, MWA8_NOP);
		}
		else
		{
			/*logerror("&c000-&f8ff is ram!\n"); */

			memory_set_bankhandler_r(1, 0, MRA8_BANK1);
			memory_set_bankhandler_r(2, 0, MRA8_BANK2);
			memory_set_bankhandler_w(5, 0, MWA8_BANK5);
			memory_set_bankhandler_w(6, 0, MWA8_BANK6);

			cpu_setbank(1, oric_ram_0x0c000);
			cpu_setbank(2, oric_ram_0x0c000+0x02000);
			cpu_setbank(5, oric_ram_0x0c000);
			cpu_setbank(6, oric_ram_0x0c000+0x02000);
		}

		{
			/* basic rom disabled */
			unsigned char *rom_ptr;

			/*logerror("&f800-&ffff is jasmin rom\n"); */
			/* jasmin rom enabled */
			memory_set_bankhandler_r(3, 0, MRA8_BANK3);
			memory_set_bankhandler_w(7, 0, MWA8_BANK7);
			rom_ptr = memory_region(REGION_CPU1) + 0x010000+0x04000+0x02000;
			cpu_setbank(3, rom_ptr);
			cpu_setbank(7, rom_ptr);
		}
	}
}

static void oric_jasmin_wd179x_callback(int State)
{
	switch (State)
	{
		/* DRQ is connected to interrupt */
		case WD179X_DRQ_CLR:
		{
			oric_irqs &=~(1<<1);

			oric_refresh_ints();
		}
		break;

		case WD179X_DRQ_SET:
		{
			oric_irqs |= (1<<1);

			oric_refresh_ints();
		}
		break;

		default:
			break;
	}
}

static READ_HANDLER (oric_jasmin_r)
{
	unsigned char data = 0x0ff;

	switch (offset & 0x0f)
	{
		/* jasmin floppy disc interface */
		case 0x04:
			data = wd179x_status_r(0);
			break;
		case 0x05:
			data =wd179x_track_r(0);
			break;
		case 0x06:
			data = wd179x_sector_r(0);
			break;
		case 0x07:
			data = wd179x_data_r(0);
			break;
		default:
			data = via_0_r(offset & 0x0f);
			logerror("unhandled io read: %04x %02x\n", offset, data);
			break;

	}

	return data;
}

static WRITE_HANDLER(oric_jasmin_w)
{
	switch (offset & 0x0f)
	{
		/* microdisc floppy disc interface */
		case 0x04:
			logerror("cycles: %d\n",cycles_currently_ran());
			wd179x_command_w(0,data);
			break;
		case 0x05:
			wd179x_track_w(0,data);
			break;
		case 0x06:
			wd179x_sector_w(0,data);
			break;
		case 0x07:
			wd179x_data_w(0,data);
			break;
		/* bit 0 = side */
		case 0x08:
			wd179x_set_side(data & 0x01);
			break;
		/* any write will cause wd179x to reset */
		case 0x09:
			wd179x_reset();
			break;
		case 0x0a:
			logerror("jasmin overlay ram w: %02x PC: %04x\n",data,activecpu_get_pc());
			port_3fa_w = data;
			oric_jasmin_set_mem_0x0c000();
			break;
		case 0x0b:
			logerror("jasmin romdis w: %02x PC: %04x\n",data,activecpu_get_pc());
			port_3fb_w = data;
			oric_jasmin_set_mem_0x0c000();
			break;
		/* bit 0,1 of addr is the drive */
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
			wd179x_set_drive(offset & 0x03);
			break;

		default:
			via_0_w(offset & 0x0f, data);
			break;
	}
}


static void oric_install_jasmin_interface(void)
{
	/* romdis */
	port_3fb_w = 1;
	oric_jasmin_set_mem_0x0c000();

	install_mem_read_handler(0,0x0300, 0x03ef, oric_IO_r);
	install_mem_read_handler(0,0x03f0, 0x03ff, oric_jasmin_r);

	install_mem_write_handler(0,0x0300, 0x03ef, oric_IO_w);
	install_mem_write_handler(0,0x03f0, 0x03ff, oric_jasmin_w);
}

/*********************************/
/* MICRODISC INTERFACE variables */

/* used by Microdisc interfaces */
static unsigned char oric_wd179x_int_state = 0;

/* bit 7 is intrq state */
static unsigned char port_314_r;
/* bit 7 is drq state (active low) */
static unsigned char port_318_r;
/* bit 6,5: drive */
/* bit 4: side */
/* bit 3: double density enable */
/* bit 0: enable FDC IRQ to trigger IRQ on CPU */
static unsigned char port_314_w;


static void oric_microdisc_refresh_wd179x_ints(void)
{
	oric_irqs &=~(1<<1);

	if ((oric_wd179x_int_state) && (port_314_w & (1<<0)))
	{
		/*logerror("oric microdisc interrupt\n"); */

		oric_irqs |=(1<<1);
	}

	oric_refresh_ints();
}

static void oric_microdisc_wd179x_callback(int State)
{
	switch (State)
	{
		case WD179X_IRQ_CLR:
		{
			port_314_r |=(1<<7);

			oric_wd179x_int_state = 0;

			oric_microdisc_refresh_wd179x_ints();
		}
		break;

		case WD179X_IRQ_SET:
		{
			port_314_r &= ~(1<<7);

			oric_wd179x_int_state = 1;

			oric_microdisc_refresh_wd179x_ints();
		}
		break;

		case WD179X_DRQ_CLR:
		{
			port_318_r |= (1<<7);
		}
		break;

		case WD179X_DRQ_SET:
		{
			port_318_r &=~(1<<7);
		}
		break;
	}
}


static void	oric_microdisc_set_mem_0x0c000(void)
{
	if (oric_is_telestrat)
		return;

	/* for 0x0c000-0x0dfff: */
	/* if os disabled, ram takes priority */
	/* /ROMDIS */
	if ((port_314_w & (1<<1))==0)
	{
		/*logerror("&c000-&dfff is ram\n"); */
		/* rom disabled enable ram */
		memory_set_bankhandler_r(1, 0, MRA8_BANK1);
		memory_set_bankhandler_w(5, 0, MWA8_BANK5);
		cpu_setbank(1, oric_ram_0x0c000);
		cpu_setbank(5, oric_ram_0x0c000);
	}
	else
	{
		unsigned char *rom_ptr;
		/*logerror("&c000-&dfff is os rom\n"); */
		/* basic rom */
		memory_set_bankhandler_r(1, 0, MRA8_BANK1);
		memory_set_bankhandler_w(5, 0, MWA8_NOP);
		rom_ptr = memory_region(REGION_CPU1) + 0x010000;
		cpu_setbank(1, rom_ptr);
		cpu_setbank(5, rom_ptr);
	}

	/* for 0x0e000-0x0ffff */
	/* if not disabled, os takes priority */
	if ((port_314_w & (1<<1))!=0)
	{
		unsigned char *rom_ptr;
		/*logerror("&e000-&ffff is os rom\n"); */
		/* basic rom */
		memory_set_bankhandler_r(2, 0, MRA8_BANK2);
		memory_set_bankhandler_r(3, 0, MRA8_BANK3);
		memory_set_bankhandler_w(6, 0, MWA8_NOP);
		memory_set_bankhandler_w(7, 0, MWA8_NOP);
		rom_ptr = memory_region(REGION_CPU1) + 0x010000;
		cpu_setbank(2, rom_ptr+0x02000);
		cpu_setbank(3, rom_ptr+0x03800);
		cpu_setbank(6, rom_ptr+0x02000);
		cpu_setbank(7, rom_ptr+0x03800);

	}
	else
	{
		/* if eprom is enabled, it takes priority over ram */
		if ((port_314_w & (1<<7))==0)
		{
			unsigned char *rom_ptr;
			/*logerror("&e000-&ffff is disk rom\n"); */
			memory_set_bankhandler_r(2, 0, MRA8_BANK2);
			memory_set_bankhandler_r(3, 0, MRA8_BANK3);
			memory_set_bankhandler_w(6, 0, MWA8_NOP);
			memory_set_bankhandler_w(7, 0, MWA8_NOP);
			/* enable rom of microdisc interface */
			rom_ptr = memory_region(REGION_CPU1) + 0x014000;
			cpu_setbank(2, rom_ptr);
			cpu_setbank(3, rom_ptr+0x01800);
		}
		else
		{
			/*logerror("&e000-&ffff is ram\n"); */
			/* rom disabled enable ram */
			memory_set_bankhandler_r(2, 0, MRA8_BANK2);
			memory_set_bankhandler_r(3, 0, MRA8_BANK3);
			memory_set_bankhandler_w(6, 0, MWA8_BANK6);
			memory_set_bankhandler_w(7, 0, MWA8_BANK7);
			cpu_setbank(2, oric_ram_0x0c000+0x02000);
			cpu_setbank(3, oric_ram_0x0c000+0x03800);
			cpu_setbank(6, oric_ram_0x0c000+0x02000);
			cpu_setbank(7, oric_ram_0x0c000+0x03800);
		}
	}
}



static READ_HANDLER (oric_microdisc_r)
{
	unsigned char data = 0x0ff;

	switch (offset & 0x0ff)
	{
		/* microdisc floppy disc interface */
		case 0x00:
			data = wd179x_status_r(0);
			break;
		case 0x01:
			data =wd179x_track_r(0);
			break;
		case 0x02:
			data = wd179x_sector_r(0);
			break;
		case 0x03:
			data = wd179x_data_r(0);
			break;
		case 0x04:
			data = port_314_r | 0x07f;
/*			logerror("port_314_r: %02x\n",data); */
			break;
		case 0x08:
			data = port_318_r | 0x07f;
/*			logerror("port_318_r: %02x\n",data); */
			break;

		default:
			data = via_0_r(offset & 0x0f);
			break;

	}

	return data;
}

static WRITE_HANDLER(oric_microdisc_w)
{
	switch (offset & 0x0ff)
	{
		/* microdisc floppy disc interface */
		case 0x00:
			wd179x_command_w(0,data);
			break;
		case 0x01:
			wd179x_track_w(0,data);
			break;
		case 0x02:
			wd179x_sector_w(0,data);
			break;
		case 0x03:
			wd179x_data_w(0,data);
			break;
		case 0x04:
		{
			DENSITY density;

			port_314_w = data;

			logerror("port_314_w: %02x\n",data);

			/* bit 6,5: drive */
			/* bit 4: side */
			/* bit 3: double density enable */
			/* bit 0: enable FDC IRQ to trigger IRQ on CPU */
			wd179x_set_drive((data>>5) & 0x03);
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

			oric_microdisc_set_mem_0x0c000();
			oric_microdisc_refresh_wd179x_ints();
		}
		break;

		default:
			via_0_w(offset & 0x0f, data);
			break;
	}
}

static void oric_install_microdisc_interface(void)
{
	install_mem_read_handler(0,0x0300, 0x030f, oric_IO_r);
	install_mem_read_handler(0,0x0310, 0x031f, oric_microdisc_r);
	install_mem_read_handler(0,0x0320, 0x03ff, oric_IO_r);

	install_mem_write_handler(0,0x0300, 0x030f, oric_IO_w);
	install_mem_write_handler(0,0x0310, 0x031f, oric_microdisc_w);
	install_mem_write_handler(0,0x0320, 0x03ff, oric_IO_w);

	/* disable os rom, enable microdisc rom */
	/* 0x0c000-0x0dfff will be ram, 0x0e000-0x0ffff will be microdisc rom */
	port_314_w = 0x0ff^((1<<7) | (1<<1));

	oric_microdisc_set_mem_0x0c000();
}



/*********************************************************/


static void oric_wd179x_callback(int State)
{
	switch (readinputport(9) &  0x07)
	{
		default:
		case ORIC_FLOPPY_INTERFACE_NONE:
		case ORIC_FLOPPY_INTERFACE_APPLE2:
			return;
		case ORIC_FLOPPY_INTERFACE_MICRODISC:
			oric_microdisc_wd179x_callback(State);
			return;
		case ORIC_FLOPPY_INTERFACE_JASMIN:
			oric_jasmin_wd179x_callback(State);
			return;
	}
}

DEVICE_INIT( oric_floppy )
{
	/* TODO - THIS DOES NOT MULTITASK BETWEEN ORIC BASICDSKs AND MFM DISKS */
	return floppy_drive_init(image, NULL);
}



DEVICE_LOAD( oric_floppy )
{
	/* attempt to open mfm disk */
	if (device_load_mfm_disk(image, file) == INIT_PASS)
	{
		floppy_drive_set_disk_image_interface(image, &mfm_disk_floppy_interface);
		return INIT_PASS;
	}

	if (device_load_basicdsk_floppy(image, file) == INIT_PASS)
	{
		/* I don't know what the geometry of the disc image should be, so the
		default is 80 tracks, 2 sides, 9 sectors per track */
		basicdsk_set_geometry(image, 80, 2, 9, 512, 1, 0, FALSE);
		floppy_drive_set_disk_image_interface(image, &basicdsk_floppy_interface);
		return INIT_PASS;
	}
	return INIT_FAIL;
}

static void oric_common_init_machine(void)
{
    /* clear all irqs */
	oric_irqs = 0;
	oric_ram_0x0c000 = NULL;

    timer_pulse(TIME_IN_HZ(4800), 0, oric_refresh_tape);

	via_reset();
	via_config(0, &oric_6522_interface);
	via_set_clock(0,1000000);

	centronics_config(0, oric_cent_config);
	/* assumption: select is tied low */
	centronics_write_handshake(0, CENTRONICS_SELECT | CENTRONICS_NO_RESET, CENTRONICS_SELECT| CENTRONICS_NO_RESET);
    via_set_input_ca1(0, 1);


#ifdef ORIC_DUMP_RAM
	previous_input_port5 = readinputport(5);
#endif
}

MACHINE_INIT( oric )
{
	int disc_interface_id;

	oric_common_init_machine();
	
	oric_is_telestrat = 0;

	oric_ram_0x0c000 = auto_malloc(16384);

	disc_interface_id = readinputport(9) & 0x07;

	switch (disc_interface_id)
	{
		default:

		case ORIC_FLOPPY_INTERFACE_APPLE2:
		case ORIC_FLOPPY_INTERFACE_NONE:
		{
			/* setup memory when there is no disc interface */
			unsigned char *rom_ptr;

			/* os rom */
			memory_set_bankhandler_r(1, 0, MRA8_BANK1);
			memory_set_bankhandler_r(2, 0, MRA8_BANK2);
			memory_set_bankhandler_r(3, 0, MRA8_BANK3);
			memory_set_bankhandler_w(5, 0, MWA8_NOP);
			memory_set_bankhandler_w(6, 0, MWA8_NOP);
			memory_set_bankhandler_w(7, 0, MWA8_NOP);
			rom_ptr = memory_region(REGION_CPU1) + 0x010000;
			cpu_setbank(1, rom_ptr);
			cpu_setbank(2, rom_ptr+0x02000);
			cpu_setbank(3, rom_ptr+0x03800);
			cpu_setbank(5, rom_ptr);
			cpu_setbank(6, rom_ptr+0x02000);
			cpu_setbank(7, rom_ptr+0x03800);


			if (disc_interface_id==ORIC_FLOPPY_INTERFACE_APPLE2)
			{
				oric_install_apple2_interface();
			}
			else
			{
				install_mem_read_handler(0,0x0300, 0x03ff, oric_IO_r);
				install_mem_write_handler(0,0x0300, 0x03ff, oric_IO_w);
			}
		}
		break;

		case ORIC_FLOPPY_INTERFACE_APPLE2_V2:
		{
			oric_install_apple2_v2_interface();
		}
		break;


		case ORIC_FLOPPY_INTERFACE_MICRODISC:
		{
			oric_install_microdisc_interface();
		}
		break;

		case ORIC_FLOPPY_INTERFACE_JASMIN:
		{
			oric_install_jasmin_interface();
		}
		break;
	}


	wd179x_init(WD_TYPE_179X,oric_wd179x_callback);

	apple2_slot6_init();

}

MACHINE_STOP( oric )
{
#ifdef ORIC_DUMP_RAM
	oric_dump_ram();
#endif

	oric_ram_0x0c000 = NULL;
}


READ_HANDLER ( oric_IO_r )
{
#if 0
	switch (readinputport(9) & 0x07)
	{
		default:
		case ORIC_FLOPPY_INTERFACE_NONE:
			break;

		case ORIC_FLOPPY_INTERFACE_MICRODISC:
		{
			if ((offset>=0x010) && (offset<=0x01f))
			{
				return oric_microdisc_r(offset);
			}
		}
		break;

		case ORIC_FLOPPY_INTERFACE_JASMIN:
		{
			if ((offset>=0x0f4) && (offset<=0x0ff))
			{
				return oric_jasmin_r(offset);
			}
		}
		break;
	}
#endif
	if (enable_logging)
	{
		if ((offset & 0x0f)!=0x0d)
		{
			logerror("via 0 r: %04x %04x\n",offset,cpunum_get_pc(0)); 
		}
	}
	/* it is repeated */
	return via_0_r(offset & 0x0f);
}

WRITE_HANDLER ( oric_IO_w )
{
#if 0
	switch (readinputport(9) & 0x07)
	{
		default:
		case ORIC_FLOPPY_INTERFACE_NONE:
			break;

		case ORIC_FLOPPY_INTERFACE_MICRODISC:
		{
			if ((offset>=0x010) && (offset<=0x01f))
			{
				oric_microdisc_w(offset, data);
				return;
			}
		}
		break;

		case ORIC_FLOPPY_INTERFACE_JASMIN:
		{
			if ((offset>=0x0f4) && (offset<=0x0ff))
			{
				oric_jasmin_w(offset,data);
				return;
			}

		}
		break;
	}
#endif
	if (enable_logging)
	{
		logerror("via 0 w: %04x %02x %04x\n",offset,data,cpunum_get_pc(0)); 
	}

	via_0_w(offset & 0x0f,data);
}



/**** TELESTRAT ****/

/*
VIA lines
 Telestrat usage

PA0..PA2
 Memory bank selection

PA3
 "Midi" port pin 3

PA4
 RS232/Minitel selection

PA5
 Third mouse button (right joystick port pin 5)

PA6
 "Midi" port pin 5

PA7
 Second mouse button (right joystick port pin 9)

CA1
 "Midi" port pin 1

CA2
 not used ?

PB0..PB4
 Joystick ports

PB5
 Joystick doubler switch

PB6
 Select Left Joystick port

PB7
 Select Right Joystick port

CB1
 Phone Ring detection

CB2
 "Midi" port pin 4

*/

static unsigned char telestrat_bank_selection;
static unsigned char telestrat_via2_port_a_data;
static unsigned char telestrat_via2_port_b_data;

enum
{
	TELESTRAT_MEM_BLOCK_UNDEFINED,
	TELESTRAT_MEM_BLOCK_RAM,
	TELESTRAT_MEM_BLOCK_ROM
};

struct  telestrat_mem_block
{
	int		MemType;
	unsigned char *ptr;
};


static struct telestrat_mem_block	telestrat_blocks[8];

static void	telestrat_refresh_mem(void)
{
	struct telestrat_mem_block *mem_block = &telestrat_blocks[telestrat_bank_selection];

	switch (mem_block->MemType)
	{
		case TELESTRAT_MEM_BLOCK_RAM:
		{
			memory_set_bankhandler_r(1, 0, MRA8_BANK1);
			memory_set_bankhandler_w(2, 0, MWA8_BANK2);
			cpu_setbank(1, mem_block->ptr);
			cpu_setbank(2, mem_block->ptr);
		}
		break;

		case TELESTRAT_MEM_BLOCK_ROM:
		{
			memory_set_bankhandler_r(1, 0, MRA8_BANK1);
			memory_set_bankhandler_w(2, 0, MWA8_NOP);
			cpu_setbank(1, mem_block->ptr);
		}
		break;

		default:
		case TELESTRAT_MEM_BLOCK_UNDEFINED:
		{
			memory_set_bankhandler_r(1, 0, MRA8_NOP);
			memory_set_bankhandler_w(2, 0, MWA8_NOP);
		}
		break;
	}
}

static READ_HANDLER(telestrat_via2_in_a_func)
{
	logerror("via 2 - port a %02x\n",telestrat_via2_port_a_data);
	return telestrat_via2_port_a_data;
}


static WRITE_HANDLER(telestrat_via2_out_a_func)
{
	logerror("via 2 - port a w: %02x\n",data);

	telestrat_via2_port_a_data = data;

	if (((data^telestrat_bank_selection) & 0x07)!=0)
	{
		telestrat_bank_selection = data & 0x07;

		telestrat_refresh_mem();
	}
}

static READ_HANDLER(telestrat_via2_in_b_func)
{
	unsigned char data = 0x01f;

	/* left joystick selected? */
	if (telestrat_via2_port_b_data & (1<<6))
	{
		data &= readinputport(10);
	}

	/* right joystick selected? */
	if (telestrat_via2_port_b_data & (1<<7))
	{
		data &= readinputport(11);
	}

	data |= telestrat_via2_port_b_data & ((1<<7) | (1<<6) | (1<<5));

	return data;
}

static WRITE_HANDLER(telestrat_via2_out_b_func)
{
	telestrat_via2_port_b_data = data;
}


static void	telestrat_via2_irq_func(int state)
{
    oric_irqs &=~(1<<2);

	if (state)
	{
        logerror("telestrat via2 interrupt\n");

        oric_irqs |=(1<<2);
	}

    oric_refresh_ints();
}
struct via6522_interface telestrat_via2_interface=
{
	telestrat_via2_in_a_func,
	telestrat_via2_in_b_func,
	NULL,
	NULL,
	NULL,
	NULL,
	telestrat_via2_out_a_func,
	telestrat_via2_out_b_func,
	NULL,
	NULL,
	telestrat_via2_irq_func,
	NULL,
	NULL,
	NULL,
	NULL
};

#if 0
/* interrupt state from acia6551 */
static void telestrat_acia_callback(int irq_state)
{
	oric_irqs&=~(1<<3);

	if (irq_state)
	{
		oric_irqs |= (1<<3);
	}

	oric_refresh_ints();
}
#endif

MACHINE_INIT( telestrat )
{
	oric_common_init_machine();

	oric_is_telestrat = 1;

	/* initialise overlay ram */
	telestrat_blocks[0].MemType = TELESTRAT_MEM_BLOCK_RAM;
	telestrat_blocks[0].ptr = (unsigned char *) auto_malloc(16384);

	telestrat_blocks[1].MemType = TELESTRAT_MEM_BLOCK_RAM;
	telestrat_blocks[1].ptr = (unsigned char *) auto_malloc(16384);
	telestrat_blocks[2].MemType = TELESTRAT_MEM_BLOCK_RAM;
	telestrat_blocks[2].ptr = (unsigned char *) auto_malloc(16384);

	/* initialise default cartridge */
	telestrat_blocks[3].MemType = TELESTRAT_MEM_BLOCK_ROM;
	telestrat_blocks[3].ptr = memory_region(REGION_CPU1)+0x010000;

	telestrat_blocks[4].MemType = TELESTRAT_MEM_BLOCK_RAM;
	telestrat_blocks[4].ptr = (unsigned char *) auto_malloc(16384);

	/* initialise default cartridge */
	telestrat_blocks[5].MemType = TELESTRAT_MEM_BLOCK_ROM;
	telestrat_blocks[5].ptr = memory_region(REGION_CPU1)+0x014000;

	/* initialise default cartridge */
	telestrat_blocks[6].MemType = TELESTRAT_MEM_BLOCK_ROM;
	telestrat_blocks[6].ptr = memory_region(REGION_CPU1)+0x018000;

	/* initialise default cartridge */
	telestrat_blocks[7].MemType = TELESTRAT_MEM_BLOCK_ROM;
	telestrat_blocks[7].ptr = memory_region(REGION_CPU1)+0x01c000;

	telestrat_bank_selection = 7;
	telestrat_refresh_mem();

	/* disable os rom, enable microdisc rom */
	/* 0x0c000-0x0dfff will be ram, 0x0e000-0x0ffff will be microdisc rom */
    port_314_w = 0x0ff^((1<<7) | (1<<1));

	acia_6551_init();

	via_config(1, &telestrat_via2_interface);
	via_set_clock(1,1000000);

	wd179x_init(WD_TYPE_179X,oric_wd179x_callback);
}

MACHINE_STOP( telestrat )
{
	int i;

	/* this supports different cartridges now. In the init machine
	a cartridge is hard-coded, but if other cartridges exist it could
	be changed above */
	for (i=0; i<8; i++)
	{
		if (telestrat_blocks[i].MemType == TELESTRAT_MEM_BLOCK_RAM)
			telestrat_blocks[i].ptr = NULL;
	}
	machine_stop_oric();
}


WRITE_HANDLER(telestrat_IO_w)
{
	if (offset<0x010)
	{
		via_0_w(offset & 0x0f,data);
		return;
	}

	if ((offset>=0x020) && (offset<=0x02f))
	{
		via_1_w(offset & 0x0f,data);
		return;
	}

	if ((offset>=0x010) && (offset<=0x01b))
	{
		oric_microdisc_w(offset & 0x0f, data);
		return;
	}

	if ((offset>=0x01c) && (offset<=0x01f))
	{
		acia_6551_w(offset, data);
		return;
	}

	logerror("null write %04x %02x\n",offset,data);

}

READ_HANDLER(telestrat_IO_r)
{
	if (offset<0x010)
	{
		return via_0_r(offset & 0x0f);
	}

	if ((offset>=0x020) && (offset<=0x02f))
	{
		return via_1_r(offset & 0x0f);
	}

	if ((offset>=0x010) && (offset<=0x01b))
	{
		return oric_microdisc_r(offset & 0x0f);
	}

	if ((offset>=0x01c) && (offset<=0x01f))
	{
		return acia_6551_r(offset);
	}

	logerror("null read %04x\n",offset);
	return 0x0ff;
}

