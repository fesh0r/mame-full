/*
	experimental LISA driver

	Runs most ROM start-up code successfully, but a floppy bug causes boot to fail.

	TODO :
	* debug floppy controller emulation (indispensable to boot)
	* fix COPS support (what I assumed to be COPS reset line is NO reset line)
	* finish keyboard/mouse support
	* finish clock support
	* write SCC support
	* finish sound support (involves adding new features to the 6522 VIA core)
	* fix warm-reset (I think I need to use a callback when 68k RESET instruction is called)
	* write support for additionnal hardware (hard disk, etc...)
	* emulate Macintosh XL
	* emulate LISA1 (?)
	* optimize MMU emulation !

	Credits :
	* the lisaemu project (<http://www.sundernet.com/>) has gathered much hardware information
	(books, schematics...) without which this driver could never have been written
	* The driver raised the interest of several MESS regulars (Paul Lunga, Dennis Munsie...)
	who supported my feeble efforts

	Raphael Nabet, 2000
*/

#include "driver.h"
#include "mess/machine/6522via.h"
#include "mess/machine/iwm.h"
#include "mess/machine/lisa.h"
#include "m68k.h"


/*
	pointers with RAM & ROM location
*/

/* up to 2MB of 68k RAM (normally 1MB or 512kb), generally 16kb of ROM */
UINT8 *lisa_ram_ptr;
UINT8 *lisa_rom_ptr;

#define RAM_OFFSET 0x004000
#define ROM_OFFSET 0x000000

/* 1kb of RAM for 6504 floppy disk controller (shared with 68000), and 4kb of ROM (8kb,
actually, but only one 4kb bank is selected, according to the drive type (TWIGGY or 3.5'')) */
static UINT8 *fdc_ram;
static UINT8 *fdc_rom;

#define FDC_RAM_OFFSET 0x0000
#define FDC_ROM_OFFSET 0x1000

/* special ROM (includes S/N) */
UINT8 *videoROM_ptr;


/*
	MMU regs
*/

static int setup;	/* MMU setup mode : allows to edit the MMU regs */

static int seg;		/* current SEG0-1 state (-> MMU register bank) */

/* lisa MMU segment regs */
typedef struct real_mmu_entry
{
	UINT16 sorg;
	UINT16 slim;
} real_mmu_entry;

/* MMU regs translated into a more efficient format */
typedef struct mmu_entry
{
	offs_t sorg;	/* (real_sorg & 0x0fff) << 9 */
	enum { RAM_stack_r, RAM_r, RAM_stack_rw, RAM_rw, IO, invalid, special_IO } type;	/* <-> (real_slim & 0x0f00) */
	int slim;	/* (~ ((real_slim & 0x00ff) << 9)) & 0x01ffff */
} mmu_entry;

static real_mmu_entry real_mmu_regs[4][128];	/* 4 banks of 128 regs */
static mmu_entry mmu_regs[4][128];


/*
	parity logic - only hard errors are emulated for now, since
	a) the ROMs only test these
	b) most memory boards do not use soft errors (i.e. they only generate 1 parity bit to
	detect errors, instead of generating several bits to fix errors)
*/

static int diag2;			/* -> writes wrong parity data into RAM */
static int test_parity;		/* detect parity hard errors */
static UINT16 mem_err_addr_latch;	/* address when parity error occured */
static int parity_error_pending;	/* parity error interrupt pending */

static int bad_parity_count;	/* number of RAM bytes which have wrong parity */
static UINT8 *bad_parity_table;	/* array : 1 bit set for each RAM byte with wrong parity */


/*
	video
*/
static int VTMSK;				/* VBI enable */
static int VTIR;				/* VBI pending */
static UINT16 video_address_latch;	/* register : MSBs of screen bitmap address (LSBs are 0s) */
static UINT8 *videoram_ptr;		/* screen bitmap base address (derived from video_address_latch) */

static UINT16 *old_display;		/* points to a copy of the screen, so that we can see which pixel change */


/*
	2 vias : one is used for communication with COPS ; the other may be used to interface
	a hard disk
*/

static int COPS_via_in_b(int offset);
static void COPS_via_out_a(int offset, int val);
static void COPS_via_out_b(int offset, int val);
static void COPS_via_out_ca2(int offset, int val);
static void COPS_via_out_cb2(int offset, int val);
static void COPS_via_irq_func(int val);
static int parallel_via_in_b(int offset);

static int KBIR;	/* COPS VIA interrupt pending */

static struct via6522_interface lisa_via6522_intf[2] =
{
	{	/* COPS via */
		NULL, COPS_via_in_b,
		NULL, NULL,
		NULL, NULL,
		COPS_via_out_a, COPS_via_out_b,
		COPS_via_out_ca2, COPS_via_out_cb2,
		COPS_via_irq_func,
		NULL,
		NULL,
		NULL,
		NULL,
	},
	{	/* parallel interface via - incomplete */
		NULL, parallel_via_in_b,
		NULL, NULL,
		NULL, NULL,
		NULL, NULL,
		NULL, NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
	}
};

/*
	floppy disk interface
*/

static int FDIR;
static int DISK_DIAG;


/*
	protos
*/

static READ_HANDLER ( lisa_IO_r );
static WRITE_HANDLER ( lisa_IO_w );


/*
	Interrupt handling
*/

static void lisa_field_interrupts(void)
{
	if (parity_error_pending)
		return;	/* don't touch anything... */

	/*if (RSIR)
		// serial interrupt
		cpu_set_irq_line(0, M68K_IRQ_6, ASSERT_LINE);
	else if (int0)
		// external interrupt
		cpu_set_irq_line(0, M68K_IRQ_5, ASSERT_LINE);
	else if (int1)
		// external interrupt
		cpu_set_irq_line(0, M68K_IRQ_4, ASSERT_LINE);
	else if (int2)
		// external interrupt
		cpu_set_irq_line(0, M68K_IRQ_3, ASSERT_LINE);
	else*/ if (KBIR)
		/* COPS VIA interrupt */
		cpu_set_irq_line(0, M68K_IRQ_2, ASSERT_LINE);
	else if (FDIR || VTIR)
		/* floppy disk or VBl */
		cpu_set_irq_line(0, M68K_IRQ_1, ASSERT_LINE);
	else
		/* clear all interrupts */
		cpu_set_irq_line(0, M68K_IRQ_1, CLEAR_LINE);
}

static void set_parity_error_pending(int value)
{
#if 0
	/* does not work well due to bugs in 68k cores */
	parity_error_pending = value;
	if (parity_error_pending)
	{
		cpu_set_irq_line(0, M68K_IRQ_7, ASSERT_LINE);
		cpu_irq_line_vector_w(0, M68K_IRQ_7, M68K_INT_ACK_AUTOVECTOR);
	}
	else
	{
		cpu_set_irq_line(0, M68K_IRQ_7, CLEAR_LINE);
	}
#else
	/* work-around... */
	if ((! parity_error_pending) && value)
	{
		parity_error_pending = TRUE;
		cpu_set_irq_line(0, M68K_IRQ_7, PULSE_LINE);
		cpu_irq_line_vector_w(0, M68K_IRQ_7, M68K_INT_ACK_AUTOVECTOR);
	}
	else if (parity_error_pending && (! value))
	{
		parity_error_pending = FALSE;
		lisa_field_interrupts();
	}
#endif
}

INLINE void set_VTIR(int value)
{
	if (VTIR != value)
	{
		VTIR = value;
		lisa_field_interrupts();
	}
}



/*
	keyboard interface
*/

static int COPS_Ready;		/* COPS is about to read a command from VIA port A */

static int COPS_command;	/* keeps the data sent by the VIA to the COPS */

static int fifo_data[8];	/* 8-byte FIFO used by COPS to buffer data sent to VIA */
static int fifo_size;			/* current data in FIFO */
static int fifo_head;
static int fifo_tail;
static int mouse_data_offset;	/* current offset for mouse data in FIFO (!) */

static int COPS_reset_line;		/* RESET line for COPS (comes from VIA) */

static void *mouse_timer = NULL;	/* timer called for mouse setup */

static int hold_COPS_data;	/* mirrors the state of CA2 - COPS does not send data until it is low */

/* clock registers */
static struct
{
	long alarm;		/* alarm (20-bit binary) */
	int years;		/* years (4-bit binary ) */
	int days1;		/* days (BCD : 1-366) */
	int days2;
	int days3;
	int hours1;		/* hours (BCD : 0-23) */
	int hours2;
	int minutes1;	/* minutes (BCD : 0-59) */
	int minutes2;
	int seconds1;	/* seconds (BCD : 0-59) */
	int seconds2;
	int tenths;		/* tenths of second (BCD : 0-9) */
} clock_regs;


/* sends data from the FIFO if possible */
INLINE void COPS_send_data_if_possible(void)
{
	if ((! hold_COPS_data) && fifo_size && (! COPS_Ready))
	{
		/*logerror("Pushing one byte of data to VIA\n");*/

		via_set_input_a(0, fifo_data[fifo_head]);	/* output data */
		if (fifo_head == mouse_data_offset)
			mouse_data_offset = -1;	/* we just phased out the mouse data in buffer */
		fifo_head = (fifo_head+1) & 0x7;
		fifo_size--;
		via_set_input_ca1(0, 1);		/* pulse ca1 so that VIA reads it */
		via_set_input_ca1(0, 0);		/* BTW, I have no idea how a real COPS does it ! */
	}
}

/* send data (queue it into the FIFO if needed) */
static void COPS_queue_data(UINT8 *data, int len)
{
#if 0
	if (fifo_size + len <= 8)
#else
	/* trash old data */
	while (fifo_size > 8 - len)
	{
		if (fifo_head == mouse_data_offset)
			mouse_data_offset = -1;	/* we just phased out the mouse data in buffer */
		fifo_head = (fifo_head+1) & 0x7;
		fifo_size--;
	}
#endif

	{
		/*logerror("Adding %d bytes of data to FIFO\n", len);*/

		while (len--)
		{
			fifo_data[fifo_tail] = * (data++);
			fifo_tail = (fifo_tail+1) & 0x7;
			fifo_size++;
		}

		/*logerror("COPS_queue_data : trying to send data to VIA\n");*/
		COPS_send_data_if_possible();
	}
}

/*
	scan_keyboard()

	scan the keyboard, and add key transition codes to buffer as needed
*/
/* shamelessly stolen from machine/mac.c :-) */

/* keyboard matrix to detect transition */
static int key_matrix[8];

static void scan_keyboard( void )
{
	int i, j;
	int keybuf;
	UINT8 keycode;

	for (i=0; i<8; i++)
	{
		keybuf = readinputport(i+2);

		if (keybuf != key_matrix[i])
		{	/* if state has changed, find first bit which has changed */
			/*logerror("keyboard state changed, %d %X\n", i, keybuf);*/

			for (j=0; j<16; j++)
			{
				if (((keybuf ^ key_matrix[i]) >> j) & 1)
				{
					/* update key_matrix */
					key_matrix[i] = (key_matrix[i] & ~ (1 << j)) | (keybuf & (1 << j));

					/* create key code */
					keycode = (i << 4) | j;
					if (keybuf & (1 << j))
					{	/* key down */
						keycode |= 0x80;
					}
					COPS_queue_data(& keycode, 1);
				}
			}
		}
	}
}

/* handle mouse moves */
/* shamelessly stolen from machine/mac.c :-) */
static void handle_mouse(int unused)
{
	static int last_mx = 0, last_my = 0;

	int diff_x = 0, diff_y = 0;
	int new_mx, new_my;

	new_mx = readinputport(0);
	new_my = readinputport(1);

	/* see if it moved in the x coord */
	if (new_mx != last_mx)
	{
		diff_x = new_mx - last_mx;

		/* check for wrap */
		/* does this code really work ??? */
		if ((diff_x > 200) || (diff_x < -200))
		{
			if (diff_x < 0)
				diff_x = - (256 + diff_x);
			else
				diff_x = 256 - diff_x;
		}

		last_mx = new_mx;
	}
	/* see if it moved in the y coord */
	if (new_my != last_my)
	{
		diff_y = new_my - last_my;

		/* check for wrap */
		/* does this code really work ??? */
		if (diff_y > 200 || diff_y < -200)
		{
			if (diff_y < 0)
				diff_y = - (256 + diff_y);
			else
				diff_y = 256 - diff_y;
		}

		last_my = new_my;
	}

	/* update any remaining count and then return */
	if (diff_x || diff_y)
	{
		if (mouse_data_offset != -1)
		{
			fifo_data[mouse_data_offset] += diff_x;
			fifo_data[(mouse_data_offset+1) & 0x7] += diff_y;
		}
		else
		{
#if 0
			if (fifo_size <= 5)
#else
			/* trash old data */
			while (fifo_size > 5)
			{
				fifo_head = (fifo_head+1) & 0x7;
				fifo_size--;
			}
#endif

			{
				/*logerror("Adding 3 bytes of mouse data to FIFO\n");*/

				fifo_data[fifo_tail] = 0;
				mouse_data_offset = fifo_tail = (fifo_tail+1) & 0x7;
				fifo_data[fifo_tail] = diff_x;
				fifo_tail = (fifo_tail+1) & 0x7;
				fifo_data[fifo_tail] = diff_y;
				fifo_tail = (fifo_tail+1) & 0x7;
				fifo_size += 3;

				/*logerror("handle_mouse : trying to send data to VIA\n");*/
				COPS_send_data_if_possible();
			}
			/* else, mouse data is lost forever (correct ??) */
		}
	}
}

/* read command from the VIA port A */
static void read_COPS_command(int unused)
{
	int command;

	COPS_Ready = FALSE;

	/*logerror("read_COPS_command : trying to send data to VIA\n");*/
	COPS_send_data_if_possible();

	/* some pull-ups allow the COPS to read 1s when the VIA port is not set as output */
	command = (COPS_command | (~ via_read(0, VIA_DDRA))) & 0xff;

	if (command & 0x80)
		return;	/* NOP */

	if (command & 0xF0)
	{	/* commands with 4-bit immediate operand */
		int immediate = command & 0xf;

		switch ((command & 0xF0) >> 4)
		{
		case 0x1:	/* write clock data */

			break;

		case 0x2:	/* set clock mode */
#if 0
			if (immediate & 0x8)
			{	/* start setting the clock */
			}

			if (! (immediate & 0x4))
			{	/* enter sleep mode */
			}
			else
			{	/* wake up */
			}

			switch (immediate & 0x3)
			{
			case 0x0:	/* clock/timer disable */

				break;

			case 0x1:	/* timer disable */

				break;

			case 0x2:	/* timer underflow generates interrupt */

				break;

			case 0x3:	/* timer underflow turns system on if it is off and gens interrupt */

				break;
			}
#endif
			break;

		case 0x3:	/* write 4 keyboard LEDs */

			break;

		case 0x4:	/* write next 4 keyboard LEDs */

			break;

		case 0x5:	/* set high nibble of NMI character to nnnn */

			break;

		case 0x6:	/* set low nibble of NMI character to nnnn */

			break;

		case 0x7:	/* send mouse command */
			if (mouse_timer)
			{
				/* disable mouse timer -> disable mouse */
				timer_remove(mouse_timer);
				mouse_timer = NULL;
			}

			if (immediate & 0x8)
				/* enable mouse */
				mouse_timer = timer_pulse(TIME_IN_MSEC((immediate & 0x7)*4), 0, handle_mouse);
			break;
		}
	}
	else
	{	/* operand-less commands */
		switch (command)
		{
		case 0x0:	/*Turn I/O port on (???) */

			break;

		case 0x1:	/*Turn I/O port off (???) */

			break;

		case 0x2:	/* Read clock data */
			{
				/* format and send reply */

				UINT8 reply[7];

				reply[0] = 0x80;
				reply[1] = 0xE0 | clock_regs.years;
				reply[2] = (clock_regs.days1 << 4) | clock_regs.days2;
				reply[3] = (clock_regs.days3 << 4) | clock_regs.hours1;
				reply[4] = (clock_regs.hours2 << 4) | clock_regs.minutes1;
				reply[5] = (clock_regs.minutes2 << 4) | clock_regs.seconds1;
				reply[6] = (clock_regs.seconds2 << 4) | clock_regs.tenths;

				COPS_queue_data(reply, 7);
			}
			break;
		}
	}
}

static void set_COPS_ready(int unused)
{
	COPS_Ready = TRUE;

	/* impulsion width : +/- 20us */
	timer_set(TIME_IN_USEC(20), 0, read_COPS_command);
}

static void reset_COPS(void)
{
	int i;

	fifo_size = 0;
	fifo_head = 0;
	fifo_tail = 0;
	mouse_data_offset = -1;

	for (i=0; i<8; i++)
		key_matrix[i] = 0;

	if (mouse_timer)
	{
		/* disable mouse timer -> disable mouse */
		timer_remove(mouse_timer);
		mouse_timer = NULL;
	}

	{
		UINT8 cmd1[2] =
		{
			0x80,	/* RESET code */
			0xFD	/* keyboard unplugged */
		};

		COPS_queue_data(cmd1, 2);
	}

	{
		UINT8 cmd2[2] =
		{
			0x80,	/* RESET code */
			0x30	/* keyboard ID - US for now */
		};

		COPS_queue_data(cmd2, 2);
	}

	/*
		keyboard ID according to ROMs

		2 MSBs : "mfg code"
		6 LSBs :
			0x0x : "old US keyboard"
			0x3d : Canadian keyboard
			0x3x : US keyboard
			0x2f : UK
			0x2e : German
			0x2d : French
			0x27 : Swiss-French
			0x26 : Swiss-German
	*/
}

/* called at power-up */
static void init_COPS(void)
{
	COPS_Ready = FALSE;

	/* read command every ms (don't know the real value) */
	timer_pulse(TIME_IN_MSEC(1), 0, set_COPS_ready);

	clock_regs.alarm = 0xfffffL;
	clock_regs.years = 0;
	clock_regs.days1 = 0;
	clock_regs.days2 = 0;
	clock_regs.days3 = 1;
	clock_regs.hours1 = 0;
	clock_regs.hours2 = 0;
	clock_regs.minutes1 = 0;
	clock_regs.minutes2 = 0;
	clock_regs.seconds1 = 0;
	clock_regs.seconds2 = 0;
	clock_regs.tenths = 0;

	reset_COPS();
}



/* VIA1 accessors (COPS, sound, and 2 hard disk lines) */

/*
	PA0-7 (I/O) : VIA <-> COPS data bus
	CA1 (I) : COPS sending valid data
	CA2 (O) : VIA -> COPS handshake
*/
static void COPS_via_out_a(int offset, int val)
{
	COPS_command = val;
}

static void COPS_via_out_ca2(int offset, int val)
{
	hold_COPS_data = val;

	/*logerror("COPS CA2 line state : %d\n", val);*/

	/*logerror("COPS_via_out_ca2 : trying to send data to VIA\n");*/
	COPS_send_data_if_possible();
}

/*
	PB7 (O) : CR* ("Controller Reset", used by hard disk interface) ???
	PB6 (I) : CRDY ("COPS ready") : set low by the COPS for 20us when it is reading a command
		from the data bus (command latched at low-to-high transition)
	PB5 (I/O) : PR* ; as output : "parity error latch reset" (only when CR* and RESET* are
		inactive) ; as input : low when CR* or RESET are low.
	PB4 (I) : FDIR (floppy disk interrupt request - the fdc shared RAM should not be accessed
		unless this bit is 1)
	PB1-3 (O) : sound volume
	PB0 (O) : forces disconnection of keyboard and mouse (allows to retrive keyboard ID, etc.)

	CB1 : not used
	CB2 (O) : sound output
*/
static int COPS_via_in_b(int offset)
{
	int val = 0;

	if (! COPS_Ready)
		val |= 0x40;

	if (FDIR)
		val |= 0x10;

	return val;
}

static void COPS_via_out_b(int offset, int val)
{
	if (val & 0x01)
		COPS_reset_line = FALSE;
	else if (! COPS_reset_line)
	{
		COPS_reset_line = TRUE;
		reset_COPS();
	}
}

static void COPS_via_out_cb2(int offset, int val)
{
	speaker_level_w(0, val);
}

static void COPS_via_irq_func(int val)
{
	if (KBIR != val)
	{
		KBIR = val;
		lisa_field_interrupts();
	}
}

/* VIA2 accessors (hard disk, and a few floppy disk lines) */

/*
	PA0-7 (I/O) : VIA <-> hard disk data bus (cf PB3)
	CA1 (I) : hard disk BSY line
	CA2 (O) : hard disk PSTRB* line
*/

/*
	PB7 (O) : WCNT line : set contrast latch on low-to-high transition ?
	PB6 (I) : floppy disk DISK DIAG line
	PB5 (I) : hard disk data DD0-7 current parity (does not depend on PB2)
	PB4 (O) : hard disk CMD* line
	PB3 (O) : hard disk DR/W* line ; controls the direction of the drivers on the data bus
	PB2 (O) : when low, disables hard disk interface drivers
	PB1 (I) : hard disk BSY line
	PB0 (I) : hard disk OCD (Open Cable Detect) line : 0 when hard disk attached
	CB1 : not used
	CB2 (I) : current parity latch value
*/
static int parallel_via_in_b(int offset)
{
	int val = 0;

	if (DISK_DIAG)
		val |= 0x40;

	/* tell there is no hard disk : */
	val |= 0x1;

	return val;
}






/*
	LISA video emulation
*/

int lisa_vh_start(void)
{
	size_t videoram_size = (720 * 360 / 8);

	old_display = (UINT16 *) malloc(videoram_size);
	if (! old_display)
	{
		return 1;
	}
	memset(old_display, 0, videoram_size);

	return 0;
}

void lisa_vh_stop(void)
{
	free(old_display);
}

void lisa_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	UINT16	data;
	UINT16	*old;
	UINT8	*v;
	int		fg, bg, x, y;

	v = videoram_ptr;
	bg = Machine->pens[0];
	fg = Machine->pens[1];
	old = old_display;

	for (y = 0; y < 360; y++) {
		for ( x = 0; x < 45; x++ ) {
			data = READ_WORD( v );
			if (full_refresh || (data != *old)) {
				plot_pixel( bitmap, ( x << 4 ) + 0x00, y, ( data & 0x8000 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x01, y, ( data & 0x4000 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x02, y, ( data & 0x2000 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x03, y, ( data & 0x1000 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x04, y, ( data & 0x0800 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x05, y, ( data & 0x0400 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x06, y, ( data & 0x0200 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x07, y, ( data & 0x0100 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x08, y, ( data & 0x0080 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x09, y, ( data & 0x0040 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x0a, y, ( data & 0x0020 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x0b, y, ( data & 0x0010 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x0c, y, ( data & 0x0008 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x0d, y, ( data & 0x0004 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x0e, y, ( data & 0x0002 ) ? fg : bg );
				plot_pixel( bitmap, ( x << 4 ) + 0x0f, y, ( data & 0x0001 ) ? fg : bg );
				*old = data;
			}
			v += 2;
			old++;
		}
	}
}


static OPBASE_HANDLER (lisa_OPbaseoverride)
{
	offs_t answer;

	/* upper 7 bits -> segment # */
	int segment = (address >> 17) & 0x7f;

	int the_seg = seg;

	/*logerror("logical address%lX\n", address);*/


	if (setup)
	{
		if (address & 0x004000)
		{
			the_seg = 0;	/* correct ??? */
		}
		else
		{
			if (address & 0x008000)
			{	/* MMU register : BUS error ??? */
				answer = 0;
			}
			else
			{	/* system ROMs */
				OP_ROM = OP_RAM = lisa_rom_ptr - (address & 0xffc000);
				/*logerror("ROM (setup mode)\n");*/
			}

			return -1;
		}

	}


	{
		int seg_offset = address & 0x01ffff;

		/* add revelant origin -> address */
		offs_t mapped_address = (mmu_regs[the_seg][segment].sorg + seg_offset) & 0x1fffff;

		switch (mmu_regs[the_seg][segment].type)
		{

		case RAM_r:
		case RAM_rw:
			if (mapped_address > mmu_regs[the_seg][segment].slim)
			{
				/* out of segment limits : bus error */

			}
			OP_ROM = OP_RAM = lisa_ram_ptr + mapped_address - address;
			/*logerror("RAM\n");*/
			break;

		case RAM_stack_r:
		case RAM_stack_rw:	/* stack : bus error ??? */
		case IO:			/* I/O : bus error ??? */
		case invalid:		/* unmapped segment */
			/* bus error */

			break;

		case special_IO:
			OP_ROM = OP_RAM = lisa_rom_ptr + (mapped_address & 0x003fff) - address;
			/*logerror("ROM\n");*/
			break;
		}
	}


	/*logerror("resulting offset%lX\n", answer);*/

	return -1;
}

static OPBASE_HANDLER (lisa_fdc_OPbaseoverride)
{
	/* 8kb of address space -> wraparound */
	return (address & 0x1fff);
}


int lisa_floppy_init(int id)
{
	return iwm_floppy_init(id, IWM_FLOPPY_ALLOW400K /*| IWM_FLOPPY_ALLOW800K*/);
}

void lisa_floppy_exit(int id)
{
	iwm_floppy_exit(id);
}


void lisa_init_machine()
{
	lisa_ram_ptr = memory_region(REGION_CPU1) + RAM_OFFSET;
	lisa_rom_ptr = memory_region(REGION_CPU1) + ROM_OFFSET;

	fdc_ram = memory_region(REGION_CPU2) + FDC_RAM_OFFSET;
	fdc_rom = memory_region(REGION_CPU2) + FDC_ROM_OFFSET;

	videoROM_ptr = memory_region(REGION_GFX1);

	cpu_setOPbaseoverride(0, lisa_OPbaseoverride);
	cpu_setOPbaseoverride(1, lisa_fdc_OPbaseoverride);

	/* int MMU */

	setup = TRUE;

	seg = 0;

	/* init parity */

	diag2 = FALSE;
	test_parity = FALSE;
	parity_error_pending = FALSE;

	bad_parity_count = 0;
	bad_parity_table = memory_region(REGION_USER1);
	memset(bad_parity_table, 0, memory_region_length(REGION_USER1));	/* Clear */

	/* init video */

	VTMSK = FALSE;
	set_VTIR(FALSE);

	video_address_latch = 0;
	videoram_ptr = lisa_ram_ptr;

	/* reset COPS keyboard/mouse controller */
	init_COPS();

	/* configure via */
	via_config(0, & lisa_via6522_intf[0]);
	via_set_clock(0, 1000000);	/* 6522 = 1 Mhz, 6522a = 2 Mhz */
	via_config(1, & lisa_via6522_intf[1]);
	via_set_clock(1, 1000000);	/* 6522 = 1 Mhz, 6522a = 2 Mhz */

	via_reset();
	COPS_via_out_ca2(0, 0);	/* VIA core forgets to do so */

	/* initialize floppy */
	iwm_init();
}

int lisa_interrupt(void)
{
	static int frame_count = 0;

	if ((++frame_count) == 6)
	{	/* increment clock every 1/10s */
		frame_count = 0;

		if ((++clock_regs.tenths) == 10)
		{
			clock_regs.tenths = 0;

			if (clock_regs.alarm == 0)
			{
				/* interrupt... */
				clock_regs.alarm = 0xfffffL;
			}
			else
			{
				clock_regs.alarm--;
			}

			if ((++clock_regs.seconds2) == 10)
			{
				clock_regs.seconds2 = 0;

				if ((++clock_regs.seconds1) == 6)
				{
					clock_regs.seconds1 = 0;

					if ((++clock_regs.minutes2) == 10)
					{
						clock_regs.minutes2 = 0;

						if ((++clock_regs.minutes1) == 6)
						{
							clock_regs.minutes1 = 0;

							if ((++clock_regs.hours2) == 10)
							{
								clock_regs.hours2 = 0;

								clock_regs.hours1++;
							}

							if ((clock_regs.hours1*10 + clock_regs.hours2) == 24)
							{
								clock_regs.hours1 = clock_regs.hours2 = 0;

								if ((++clock_regs.days3) == 10)
								{
									clock_regs.days3 = 0;

									if ((++clock_regs.days2) == 10)
									{
										clock_regs.days2 = 0;

										clock_regs.days1++;
									}
								}

								if ((clock_regs.days1*100 + clock_regs.days2*10 + clock_regs.days3) ==
									((clock_regs.years % 4) ? 366 : 367))
								{
									clock_regs.days1 = clock_regs.days2 = clock_regs.days3 = 0;

									clock_regs.years = (clock_regs.years + 1) & 0xf;
								}
							}
						}
					}
				}
			}
		}
	}

	/* set VBI */
	if (VTMSK)
		set_VTIR(TRUE);

	/* do keyboard scan */
	scan_keyboard();

	return 0;
}


READ_HANDLER ( lisa_fdc_io_r )
{
	int answer=0;

	switch ((offset & 0x0030) >> 4)
	{
	case 0:	/* IWM */
		answer = iwm_r(offset);
		break;

	case 1:	/* TTL glue */
		switch ((offset & 0x000E) >> 1)
		{
		case 0:
			/*stop = offset & 1;*/	/* ???? */
			break;
		case 2:
			/*MT0 = offset & 1;*/	/* ???? */
			break;
		case 3:
			/*MT1 = offset & 1;*/	/* ???? */
			break;
		case 4:
			/*DIS = offset & 1;*/	/* ???? */
			break;
		case 5:
			/*HDs = offset & 1;*/	/* ???? */
			break;
		case 6:
			DISK_DIAG = offset & 1;
			break;
		case 7:
			FDIR = offset & 1;	/* Interrupt request to 68k */
			lisa_field_interrupts();
			break;
		}
		answer =  0;	/* ??? */
		break;

	case 2:	/* pulses the PWM LOAD line (mistake !) */
		answer =  0;	/* ??? */
		break;

	case 3:	/* not used */
		answer =  0;	/* ??? */
		break;
	}

	return answer;
}

WRITE_HANDLER ( lisa_fdc_io_w )
{
	switch ((offset & 0x0030) >> 4)
	{
	case 0:	/* IWM */
		iwm_w(offset, data);
		break;

	case 1:	/* TTL glue */
		switch ((offset & 0x000E) >> 1)
		{
		case 0:
			/*stop = offset & 1;*/	/* stop/run a clock (same as PWM LOAD -  never used) */
			break;
		case 2:
			/*MT0 = offset & 1;*/	/* ???? */
			break;
		case 3:
			/*MT1 = offset & 1;*/	/* enable/disable the clock compare (same as PWM LOAD) */
			break;
		case 4:
			/*DIS = offset & 1;*/	/* ???? */
			break;
		case 5:
			/*HDs = offset & 1;*/	/* ???? */
			break;
		case 6:
			DISK_DIAG = offset & 1;
			break;
		case 7:
			FDIR = offset & 1;	/* Interrupt request to 68k */
			lisa_field_interrupts();
			break;
		}
		break;

	case 2:	/* pulses the PWM LOAD line (never used) */
		/* reload a clock with value written */
		break;

	case 3:	/* not used */
		break;
	}
}

READ_HANDLER ( lisa_fdc_r )
{
	if (! (offset & 0x1000))
	{
		if (! (offset & 0x0800))
			if (! (offset & 0x0400))
				return fdc_ram[offset & 0x03ff];
			else
				return lisa_fdc_io_r(offset & 0x03ff);
		else
			return 0;	/* ??? */
	}
	else
		return fdc_rom[offset & 0x0fff];
}

WRITE_HANDLER ( lisa_fdc_w )
{
	if (! (offset & 0x1000))
	{
		if (! (offset & 0x0800))
		{
			if (! (offset & 0x0400))
				fdc_ram[offset & 0x0fff] = data;
			else
				lisa_fdc_io_w(offset & 0x03ff, data);
		}

	}
}


READ_HANDLER ( lisa_r )
{
	int answer=0;

	/* segment register set */
	int the_seg = seg;

	/* upper 7 bits -> segment # */
	int segment = (offset >> 17) & 0x7f;


	/*logerror("read, logical address%lX\n", offset);*/

	if (setup)
	{	/* special setup mode */
		if (offset & 0x004000)
		{
			the_seg = 0;	/* correct ??? */
		}
		else
		{
			if (offset & 0x008000)
			{	/* read MMU register */
				/*logerror("read from segment registers (%X:%X) ", the_seg, segment);*/
				if (offset & 0x000008)
				{	/* sorg register */
					answer = real_mmu_regs[the_seg][segment].sorg;
					/*logerror("sorg, data = %X\n", answer);*/
				}
				else
				{	/* slim register */
					answer = real_mmu_regs[the_seg][segment].slim;
					/*logerror("slim, data = %X\n", answer);*/
				}
			}
			else
			{	/* system ROMs */
				answer = READ_WORD(lisa_rom_ptr + (offset & 0x003fff));

				/*logerror("dst address in ROM (setup mode)\n");*/
			}

			return answer;
		}
	}

	{
		/* offset in segment */
		int seg_offset = offset & 0x01ffff;

		/* add revelant origin -> address */
		offs_t address = (mmu_regs[the_seg][segment].sorg + seg_offset) & 0x1fffff;

		/*logerror("read, logical address%lX\n", offset);
		logerror("physical address%lX\n", address);*/

		switch (mmu_regs[the_seg][segment].type)
		{

		case RAM_stack_r:
		case RAM_stack_rw:
			if (address <= mmu_regs[the_seg][segment].slim)
			{
				/* out of segment limits : bus error */

			}
			answer = READ_WORD(lisa_ram_ptr + address);

			if (bad_parity_count && test_parity
					&& (bad_parity_table[address >> 3] & (0x3 << (address & 0x7))))
			{
				mem_err_addr_latch = address >> 5;
				set_parity_error_pending(TRUE);
			}
			break;

		case RAM_r:
		case RAM_rw:
			if (address > mmu_regs[the_seg][segment].slim)
			{
				/* out of segment limits : bus error */

			}
			answer = READ_WORD(lisa_ram_ptr + address);

			if (bad_parity_count && test_parity
					&& (bad_parity_table[address >> 3] & (0x3 << (address & 0x7))))
			{
				mem_err_addr_latch = address >> 5;
				set_parity_error_pending(TRUE);
			}
			break;

		case IO:
			answer = lisa_IO_r(address & 0x00ffff);
			break;

		case invalid:		/* unmapped segment */
			/* bus error */

			answer = 0;
			break;

		case special_IO:
			if (! (address & 0x008000))
				answer = READ_WORD(lisa_rom_ptr + (address & 0x003fff));
			else
			{	/* read serial number from ROM */
				/* this has to be be the least efficient way to read a ROM :-) */
				/* this emulation is not guaranteed accurate */

				/* problem : due to collisions with video, timings of the LISA CPU
				are slightly different from timings of a bare 68k */
				/* so we use a kludge... */
#if 0
				/* theory - probably partially wrong, anyway */
				int time_in_frame = cpu_getcurrentcycles();
				int videoROM_address;

				videoROM_address = (time_in_frame / 4) & 0x7f;

				if ((time_in_frame >= 70000) || (time_in_frame <= 74000))	/* these values are approximative */
				{	/* if VSyncing, read ROM 2nd half ? */
					videoROM_address |= 0x80;
				}
#else
				/* kludge */
				/* this code assumes the program always tries to read consecutive bytes */
				int time_in_frame = cpu_getcurrentcycles();
				static int videoROM_address = 0;

				videoROM_address = (videoROM_address + 1) & 0x7f;
				/* the BOOT test ROM only reads 56 bits, so there must be some wrap-around for
				videoROM_address <= 56 */
				if (videoROM_address == 56)
					videoROM_address = 0;

				if ((time_in_frame >= 70000) && (time_in_frame <= 74000))	/* these values are approximative */
				{	/* if VSyncing, read ROM 2nd half ? */
					videoROM_address |= 0x80;
				}
#endif

				answer = videoROM_ptr[videoROM_address] << 8;

				/*logerror("%X %X\n", videoROM_address, answer);*/
			}
			break;
		}
	}

	/*logerror("result %X\n", answer);*/

	return answer;
}

WRITE_HANDLER ( lisa_w )
{
	/* segment register set */
	int the_seg = seg;

	/* upper 7 bits -> segment # */
	int segment = (offset >> 17) & 0x7f;


	if (setup)
	{
		if (offset & 0x004000)
		{
			the_seg = 0;	/* correct ??? */
		}
		else
		{
			if (offset & 0x008000)
			{	/* write to MMU register */
				/*logerror("write to segment registers (%X:%X) ", the_seg, segment);*/
				if (offset & 0x000008)
				{	/* sorg register */
					/*logerror("sorg, data = %X\n", data);*/
					real_mmu_regs[the_seg][segment].sorg = data;
					mmu_regs[the_seg][segment].sorg = (data & 0x0fff) << 9;
				}
				else
				{	/* slim register */
					/*logerror("slim, data = %X\n", data);*/
					real_mmu_regs[the_seg][segment].slim = data;
					mmu_regs[the_seg][segment].slim = (~ (data << 9)) & 0x01ffff;
					switch ((data & 0x0f00) >> 8)
					{
					case 0x4:
						/*logerror("type : RAM stack r\n");*/
						mmu_regs[the_seg][segment].type = RAM_stack_r;
						break;
					case 0x5:
						/*logerror("type : RAM r\n");*/
						mmu_regs[the_seg][segment].type = RAM_r;
						break;
					case 0x6:
						/*logerror("type : RAM stack rw\n");*/
						mmu_regs[the_seg][segment].type = RAM_stack_rw;
						break;
					case 0x7:
						/*logerror("type : RAM rw\n");*/
						mmu_regs[the_seg][segment].type = RAM_rw;
						break;
					case 0x8:
					case 0x9:	/* not documented, but used by ROMs (?) */
						/*logerror("type : I/O\n");*/
						mmu_regs[the_seg][segment].type = IO;
						break;
					case 0xC:
						/*logerror("type : invalid\n");*/
						mmu_regs[the_seg][segment].type = invalid;
						break;
					case 0xF:
						/*logerror("type : special I/O\n");*/
						mmu_regs[the_seg][segment].type = special_IO;
						break;
					default:	/* "unpredictable results" */
						/*logerror("type : unknown\n");*/
						mmu_regs[the_seg][segment].type = invalid;
						break;
					}
				}
			}
			else
			{	/* system ROMs : read-only ??? */
				/* bus error ??? */
			}
			return;
		}
	}

	{
		/* offset in segment */
		int seg_offset = offset & 0x01ffff;

		/* add revelant origin -> address */
		offs_t address = (mmu_regs[the_seg][segment].sorg + seg_offset) & 0x1fffff;

		switch (mmu_regs[the_seg][segment].type)
		{

		case RAM_stack_rw:
			if (address <= mmu_regs[the_seg][segment].slim)
			{
				/* out of segment limits : bus error */

			}
			COMBINE_WORD_MEM(lisa_ram_ptr + address, data);
			if (diag2)
			{
				if (! (data & 0x00ff0000))
				{
					bad_parity_table[address >> 3] |= 0x1 << (address & 0x7);
					bad_parity_count++;
				}
				if (! (data & 0xff000000))
				{
					bad_parity_table[address >> 3] |= 0x2 << (address & 0x7);
					bad_parity_count++;
				}
			}
			else if (bad_parity_table[address >> 3] & (0x3 << (address & 0x7)))
			{
				if ((! (data & 0x00ff0000))
					&& (bad_parity_table[address >> 3] & (0x1 << (address & 0x7))))
				{
					bad_parity_table[address >> 3] &= ~ (0x1 << (address & 0x7));
					bad_parity_count--;
				}
				if ((! (data & 0xff000000))
					&& (bad_parity_table[address >> 3] & (0x2 << (address & 0x7))))
				{
					bad_parity_table[address >> 3] &= ~ (0x2 << (address & 0x7));
					bad_parity_count--;
				}
			}
			break;

		case RAM_rw:
			if (address > mmu_regs[the_seg][segment].slim)
			{
				/* out of segment limits : bus error */

			}
			COMBINE_WORD_MEM(lisa_ram_ptr + address, data);
			if (diag2)
			{
				if (! (data & 0x00ff0000))
				{
					bad_parity_table[address >> 3] |= 0x1 << (address & 0x7);
					bad_parity_count++;
				}
				if (! (data & 0xff000000))
				{
					bad_parity_table[address >> 3] |= 0x2 << (address & 0x7);
					bad_parity_count++;
				}
			}
			else if (bad_parity_table[address >> 3] & (0x3 << (address & 0x7)))
			{
				if ((! (data & 0x00ff0000))
					&& (bad_parity_table[address >> 3] & (0x1 << (address & 0x7))))
				{
					bad_parity_table[address >> 3] &= ~ (0x1 << (address & 0x7));
					bad_parity_count--;
				}
				if ((! (data & 0xff000000))
					&& (bad_parity_table[address >> 3] & (0x2 << (address & 0x7))))
				{
					bad_parity_table[address >> 3] &= ~ (0x2 << (address & 0x7));
					bad_parity_count--;
				}
			}
			break;

		case IO:
			lisa_IO_w(address, data);
			break;

		case RAM_stack_r:	/* read-only */
		case RAM_r:			/* read-only */
		case special_IO:	/* system ROMs : read-only ??? */
		case invalid:		/* unmapped segment */
			/* bus error */

			break;
		}
	}
}


/**************************************************************************************\
* I/O Slot Memory                                                                      *
*                                                                                      *
* 000000 - 001FFF Slot 0 Low  Decode                                                   *
* 002000 - 003FFF Slot 0 High Decode                                                   *
* 004000 - 005FFF Slot 1 Low  Decode                                                   *
* 006000 - 007FFF Slot 1 High Decode                                                   *
* 008000 - 009FFF Slot 2 Low  Decode                                                   *
* 00A000 - 00BFFF Slot 2 High Decode                                                   *
* 00C000 - 00CFFF Floppy Disk Controller shared RAM                                    *
*   00c001-00c7ff floppy disk control                                                  *
* 00D000 - 00DFFF I/O Board Devices                                                    *
*   00d000-00d3ff serial ports control                                                 *
*   00d800-00dbff paralel port                                                         *
*   00dc00-00dfff keyboard/mouse cops via                                              *
* 00E000 - 00FFFF CPU Board Devices                                                    *
*   00e000-00e01e cpu board control                                                    *
*   00e01f-00e7ff unused                                                               *
*   00e8xx-video address latch                                                         *
*   00f0xx memory error address latch                                                  *
*   00f8xx status register                                                             *
*                                                                                      *
\**************************************************************************************/

static READ_HANDLER ( lisa_IO_r )
{
	int answer=0;

	switch ((offset & 0xe000) >> 13)
	{
	case 0x0:
		/* Slot 0 Low */
		break;

	case 0x1:
		/* Slot 0 High */
		break;

	case 0x2:
		/* Slot 1 Low */
		break;

	case 0x3:
		/* Slot 1 High */
		break;

	case 0x4:
		/* Slot 2 Low */
		break;

	case 0x5:
		/* Slot 2 High */
		break;

	case 0x6:
		if (! (offset & 0x1000))
		{
			if (! (offset & 0x0800))
			{
				answer = fdc_ram[(offset >> 1) & 0x03ff] & 0xff;	/* right ??? */
			}
		}
		else
		{
			/* I/O Board Devices */
			switch ((offset & 0x0c00) >> 10)
			{
			case 0:	/* serial ports control */
				/*SCCBCTL	        .EQU    $FCD241	        ;SCC channel B control
				ACTL	        .EQU    2	        ;offset to SCC channel A control
				SCCDATA	        .EQU    4	        ;offset to SCC data regs*/
				break;

			case 2:	/* parallel port */
				/* 1 VIA located at 0xD901 */
				return via_read(1, (offset >> 3) & 0xf);
				break;

			case 3:	/* keyboard/mouse cops via */
				/* 1 VIA located at 0xDD81 */
				return via_read(0, (offset >> 1) & 0xf);
				break;
			}
		}
		break;

	case 0x7:
		/* CPU Board Devices */
		switch ((offset & 0x1800) >> 11)
		{
		case 0x0:	/* cpu board control */
			switch (offset & 0x07ff)
			{
			case 0x0002:	/* Set DIAG1 Latch */
			case 0x0000:	/* Reset DIAG1 Latch */
				break;
			case 0x0006:	/* Set Diag2 Latch */
				diag2 = TRUE;
				break;
			case 0x0004:	/* ReSet Diag2 Latch */
				diag2 = FALSE;
				break;
			case 0x000A:	/* SEG1 Context Selection bit SET */
				/*logerror("seg bit 0 set\n");*/
				seg |= 1;
				break;
			case 0x0008:	/* SEG1 Context Selection bit RESET */
				/*logerror("seg bit 0 clear\n");*/
				seg &= ~1;
				break;
			case 0x000E:	/* SEG2 Context Selection bit SET */
				/*logerror("seg bit 1 set\n");*/
				seg |= 2;
				break;
			case 0x000C:	/* SEG2 Context Selection bit RESET */
				/*logerror("seg bit 1 clear\n");*/
				seg &= ~2;
				break;
			case 0x0010:	/* SETUP register SET */
				setup = TRUE;
				break;
			case 0x0012:	/* SETUP register RESET */
				setup = FALSE;
				break;
			case 0x001A:	/* Enable Vertical Retrace Interrupt */
				VTMSK = TRUE;
				break;
			case 0x0018:	/* Disable Vertical Retrace Interrupt */
				VTMSK = FALSE;
				set_VTIR(FALSE);
				break;
			case 0x0016:	/* Enable Soft Error Detect. */
			case 0x0014:	/* Disable Soft Error Detect. */
				break;
			case 0x001E:	/* Enable Hard Error Detect */
				test_parity = TRUE;
				break;
			case 0x001C:	/* Disable Hard Error Detect */
				test_parity = FALSE;
				set_parity_error_pending(FALSE);
				break;
			}
			break;

		case 0x1:	/* Video Address Latch */
			answer = video_address_latch;
			break;

		case 0x2:	/* Memory Error Address Latch */
			answer = mem_err_addr_latch;
			break;

		case 0x3:	/* Status Register */
			answer = 0;
			if (! parity_error_pending)
				answer |= 0x02;
			if (! VTIR)
				answer |= 0x04;
			/* huh... we need to emulate some other bits */
			break;
		}
		break;
	}

	return answer;
}

static WRITE_HANDLER ( lisa_IO_w )
{
	switch ((offset & 0xe000) >> 13)
	{
	case 0x0:
		/* Slot 0 Low */
		break;

	case 0x1:
		/* Slot 0 High */
		break;

	case 0x2:
		/* Slot 1 Low */
		break;

	case 0x3:
		/* Slot 1 High */
		break;

	case 0x4:
		/* Slot 2 Low */
		break;

	case 0x5:
		/* Slot 2 High */
		break;

	case 0x6:
		if (! (offset & 0x1000))
		{
			/* Floppy Disk Controller shared RAM */
			if (! (offset & 0x0800))
			{
				if (! (data & 0x00ff0000))
					fdc_ram[(offset >> 1) & 0x03ff] = data & 0xff;
			}
		}
		else
		{
			/* I/O Board Devices */
			switch ((offset & 0x0c00) >> 10)
			{
			case 0:	/* serial ports control */
				break;

			case 2:	/* paralel port */
				if (! (data & 0x00ff0000))
					via_write(1, (offset >> 3) & 0xf, data & 0xff);
				break;

			case 3:	/* keyboard/mouse cops via */
				if (! (data & 0x00ff0000))
					via_write(0, (offset >> 1) & 0xf, data & 0xff);
				break;
			}
		}
		break;

	case 0x7:
		/* CPU Board Devices */
		switch ((offset & 0x1800) >> 11)
		{
		case 0x0:	/* cpu board control */
			switch (offset & 0x07ff)
			{
			case 0x0002:	/* Set DIAG1 Latch */
			case 0x0000:	/* Reset DIAG1 Latch */
				break;
			case 0x0006:	/* Set Diag2 Latch */
				diag2 = TRUE;
				break;
			case 0x0004:	/* ReSet Diag2 Latch */
				diag2 = FALSE;
				break;
			case 0x000A:	/* SEG1 Context Selection bit SET */
				seg |= 1;
				break;
			case 0x0008:	/* SEG1 Context Selection bit RESET */
				seg &= ~1;
				break;
			case 0x000E:	/* SEG2 Context Selection bit SET */
				seg |= 2;
				break;
			case 0x000C:	/* SEG2 Context Selection bit RESET */
				seg &= ~2;
				break;
			case 0x0010:	/* SETUP register SET */
				setup = TRUE;
				break;
			case 0x0012:	/* SETUP register RESET */
				setup = FALSE;
				break;
			case 0x001A:	/* Enable Vertical Retrace Interrupt */
				VTMSK = TRUE;
				break;
			case 0x0018:	/* Disable Vertical Retrace Interrupt */
				VTMSK = FALSE;
				break;
			case 0x0016:	/* Enable Soft Error Detect. */
			case 0x0014:	/* Disable Soft Error Detect. */
				break;
			case 0x001E:	/* Enable Hard Error Detect */
				test_parity = TRUE;
				break;
			case 0x001C:	/* Disable Hard Error Detect */
				test_parity = FALSE;
				set_parity_error_pending(FALSE);
				break;
			}
			break;

		case 0x1:	/* Video Address Latch */
			/*logerror("video address latch write offs=%X, data=%X\n", offset, data);*/
			COMBINE_WORD_MEM(& video_address_latch, data);
			videoram_ptr = lisa_ram_ptr + ((video_address_latch << 7) & 0x1f8000);
			/*logerror("video address latch %X -> base address %X\n", video_address_latch,
							(video_address_latch << 7) & 0x1f8000);*/
			break;
		}
		break;
	}
}


