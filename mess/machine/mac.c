/* *************************************************************************
 * Mac hardware
 *
 * The hardware for Mac 128k, 512k, 512ke, Plus (SCSI, SCC, etc).
 *
 * Nate Woods
 * Ernesto Corvi
 * Raphael Nabet
 *
 * TODO
 * - Fully implement SCSI
 * - Call the RTC timer
 *
 * - Support Mac 128k, 512k (easy, we just need the ROM image)
 * - Support Mac SE, Classic (we need to find the ROMs and implement ADB ; SE FDHD and Classic
 *   require SIWM support, too)
 * - Check that 0x600000-0x6fffff still address RAM when overlay bit is off (IM-III seems to say it
 *   does not on Mac 128k, 512k, and 512ke).
 * - What on earth are 0x700000-0x7fffff mapped to ?
 * - What additional features does the MacPlus RTC have ? (IM IV just states : "The new chip
 *   includes additionnal parameter RAM that's reserved by Apple.")
 * *************************************************************************/


#include "driver.h"
#include "state.h"
#include "machine/6522via.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"
#include "machine/iwm.h"
#include "machine/sonydriv.h"
#include "includes/mac.h"

#include <time.h>

#ifdef MAME_DEBUG
#define LOG_VIA			0
#define LOG_RTC			0
#define LOG_MAC_IWM		0
#define LOG_TRAPS		1
#define LOG_SCSI		1
#define LOG_SCC			0
#define LOG_GENERAL		0
#define LOG_KEYBOARD	0
#define LOG_MEMORY		0
#else
#define LOG_VIA			0
#define LOG_RTC			0
#define LOG_MAC_IWM		0
#define LOG_TRAPS		0
#define LOG_SCSI		0
#define LOG_SCC			0
#define LOG_GENERAL		0
#define LOG_KEYBOARD	0
#define LOG_MEMORY		0
#endif

static int scan_keyboard(void);
static void inquiry_timeout_func(int unused);
static void keyboard_receive(int val);
static void keyboard_send_reply(void);
static READ_HANDLER(mac_via_in_a);
static READ_HANDLER(mac_via_in_b);
static WRITE_HANDLER(mac_via_out_a);
static WRITE_HANDLER(mac_via_out_b);
static void mac_via_irq(int state);

static struct via6522_interface mac_via6522_intf =
{
	mac_via_in_a, mac_via_in_b,
	NULL, NULL,
	NULL, NULL,
	mac_via_out_a, mac_via_out_b,
	NULL, NULL,
	mac_via_irq,
	NULL,
	NULL,
	keyboard_receive,
	keyboard_send_reply
};

/* tells which model is being emulated (set by macxxx_init) */
typedef enum
{
	model_mac128k512k,
	model_mac512ke,
	model_macplus
} mac_model_t;

static int mac_overlay = 0;

static const char *lookup_trap(UINT16 opcode);



static void mac_install_memory(offs_t memory_begin, offs_t memory_end,
	offs_t memory_size, void *memory_data, int is_rom, int bank)
{
	offs_t memory_mask;
	read16_handler rh;
	write16_handler wh;

	memory_size = MIN(memory_size, (memory_end + 1 - memory_begin));
	memory_mask = memory_size - 1;

	rh = (read16_handler) bank;
	wh = is_rom ? MWA16_ROM : (write16_handler) bank;

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, memory_begin,
		memory_end, memory_mask, rh);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, memory_begin,
		memory_end, memory_mask, wh);

	memory_set_bankptr(bank, memory_data);

#if LOG_MEMORY
	logerror("mac_install_memory(): bank=%d range=[0x%06x...0x%06x] mask=0x%06x ptr=0x%08x\n",
		bank, memory_begin, memory_end, memory_mask, memory_data);
#endif
}



/*
	Interrupt handling
*/

static int scc_interrupt, via_interrupt;

static void mac_field_interrupts(void)
{
	if (scc_interrupt)
		/* SCC interrupt */
		cpu_set_irq_line(0, 2, ASSERT_LINE);
	else if (via_interrupt)
		/* VIA interrupt */
		cpu_set_irq_line(0, 1, ASSERT_LINE);
	else
		/* clear all interrupts */
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}

static void set_scc_interrupt(int value)
{
	scc_interrupt = value;
	mac_field_interrupts();
}

static void set_via_interrupt(int value)
{
	via_interrupt = value;
	mac_field_interrupts();
}


WRITE16_HANDLER ( mac_autovector_w )
{
#if LOG_GENERAL
	logerror("mac_autovector_w: offset=0x%08x data=0x%04x\n", offset, data);
#endif

	/* This should throw an exception */

	/* Not yet implemented */
}

READ16_HANDLER ( mac_autovector_r )
{
#if LOG_GENERAL
	logerror("mac_autovector_r: offset=0x%08x\n", offset);
#endif

	/* This should throw an exception */

	/* Not yet implemented */
	return 0;
}

static void set_scc_waitrequest(int waitrequest)
{
#if LOG_GENERAL
	logerror("set_scc_waitrequest: waitrequest=%i\n", waitrequest);
#endif

	/* Not Yet Implemented */
}



static void set_memory_overlay(int overlay)
{
	offs_t memory_size;
	UINT8 *memory_data;
	int is_rom;

	/* normalize overlay */
	overlay = overlay ? TRUE : FALSE;

	if (overlay != mac_overlay)
	{
		/* set up either main RAM area or ROM mirror at 0x000000-0x3fffff */
		if (overlay)
		{
			/* ROM mirror */
			memory_size = memory_region_length(REGION_USER1);
			memory_data = memory_region(REGION_USER1);
			is_rom = TRUE;

			/* HACK! - copy in the initial reset/stack */
			memcpy(mess_ram, memory_data, 8);
		}
		else
		{
			/* RAM */
			memory_size = mess_ram_size;
			memory_data = mess_ram;
			is_rom = FALSE;
		}

		/* install the memory */
		mac_install_memory(0x000000, 0x3fffff, memory_size, memory_data, is_rom, 1);

		mac_overlay = overlay;

#if LOG_GENERAL
		logerror("set_memory_overlay: overlay=%i\n", overlay);
#endif
	}
}



/* *************************************************************************
 * Trap Tracing
 *
 * This is debug code that will output diagnostics regarding OS traps called
 *
 * To use this code you need to enable LOG_TRAPS and call this function from
 * m68000_1010() in m68k_in.c.  This also requires using the C version of the
 * M68k cpu core.
 *
 * For whatever it is worth:
 *		The first call to _Read occurs in the Mac Plus at 0x00400734
 * *************************************************************************/

static char *cstrfrompstr(char *buf)
{
	static char newbuf[256];
	memcpy(newbuf, buf+1, buf[0]);
	newbuf[(int)buf[0]] = '\0';
	return newbuf;
}



static const char *lookup_trap(UINT16 opcode)
{
	int i;

	static const struct
	{
		int trap;
		const char *name;
	} traps[] =
	{
		/* Of course, only a subset of the traps are listed here */
		{ 0xa000, "_Open" },
		{ 0xa001, "_Close" },
		{ 0xa002, "_Read" },
		{ 0xa003, "_Write" },
		{ 0xa004, "_Control" },
		{ 0xa005, "_Status" },
		{ 0xa00f, "_MountVol" },
		{ 0xa019, "_InitZone" },
		{ 0xa024, "_SetHandleSize" },
		{ 0xa02e, "_BlockMove" },
		{ 0xa03c, "_CmpString" },
		{ 0xa03f, "_InitUtil" },
		{ 0xa04e, "_AddDrive" },
		{ 0xa122, "_NewHandle" },
		{ 0xa851, "_SetCursor" },
		{ 0xa8a2, "_PaintRect" },
		{ 0xa8b4, "_FillRoundRect" },
		{ 0xa869, "_FixRatio" },
		{ 0xa867, "_LongMul" },
		{ 0xa852, "_HideCursor" },
		{ 0xa853, "_ShowCursor" },
		{ 0xa815, "_SCSIDispatch" },
		{ 0xa22e, "_BlockMoveData" },
		{ 0xa86f, "_OpenPort" },
		{ 0xa86e, "_InitGraf" },
		{ 0xa126, "_HandleZone" },
		{ 0xa9a0, "_GetResource" },
		{ 0xa9a8, "_GetResInfo" },
		{ 0xa9a1, "_GetNamedResource" },
		{ 0xa995, "_InitResources" },
		{ 0xa11e, "_NewPtr" },
		{ 0xa9c9, "_SysError" },
		{ 0xa895, "_Shutdown" },
		{ 0xa9fe, "_PutScrap" }
	};

	for (i = 0; i < (sizeof(traps) / sizeof(traps[0])); i++)
	{
		if (traps[i].trap == opcode)
			return traps[i].name;
	}
	return NULL;
}



static unsigned mac_dasm_override(int cpunum, char *buffer, unsigned pc)
{
	UINT16 opcode;
	unsigned result = 0;
	const char *trap;

	opcode = cpu_readop16(pc);
	if ((opcode & 0xF000) == 0xA000)
	{
		trap = lookup_trap(opcode);
		if (trap)
		{
			strcpy(buffer, trap);
			result = 1;
		}
	}
	return result;
}



#ifdef MAC_TRACETRAP
static void mac_tracetrap(const char *cpu_name_local, int addr, int trap)
{
	typedef struct
	{
		int csCode;
		const char *name;
	} sonycscodeentry;

	static const sonycscodeentry cscodes[] =
	{
		{ 1, "KillIO" },
		{ 5, "VerifyDisk" },
		{ 6, "FormatDisk" },
		{ 7, "EjectDisk" },
		{ 8, "SetTagBuffer" },
		{ 9, "TrackCacheControl" },
		{ 23, "ReturnDriveInfo" }
	};

	static const char *scsisels[] =
	{
		"SCSIReset",	/* $00 */
		"SCSIGet",		/* $01 */
		"SCSISelect",	/* $02 */
		"SCSICmd",		/* $03 */
		"SCSIComplete",	/* $04 */
		"SCSIRead",		/* $05 */
		"SCSIWrite",	/* $06 */
		NULL,			/* $07 */
		"SCSIRBlind",	/* $08 */
		"SCSIWBlind",	/* $09 */
		"SCSIStat",		/* $0A */
		"SCSISelAtn",	/* $0B */
		"SCSIMsgIn",	/* $0C */
		"SCSIMsgOut",	/* $0D */
	};

	int i, a0, a7, d0, d1;
	int csCode, ioVRefNum, ioRefNum, ioCRefNum, ioCompletion, ioBuffer, ioReqCount, ioPosOffset;
	char *s;
	unsigned char *mem;
	char buf[256];
	const char *trapstr;

	trapstr = lookup_trap(trap);
	if (trapstr)
		strcpy(buf, trapstr);
	else
		sprintf(buf, "Trap $%04x", trap);

	s = &buf[strlen(buf)];
	mem = mac_ram_ptr;
	a0 = cpu_get_reg(M68K_A0);
	a7 = cpu_get_reg(M68K_A7);
	d0 = cpu_get_reg(M68K_D0);
	d1 = cpu_get_reg(M68K_D1);

	switch(trap)
	{
	case 0xa004:	/* _Control */
		ioVRefNum = *((INT16*) (mem + a0 + 22));
		ioCRefNum = *((INT16*) (mem + a0 + 24));
		csCode = *((UINT16*) (mem + a0 + 26));
		sprintf(s, " ioVRefNum=%i ioCRefNum=%i csCode=%i", ioVRefNum, ioCRefNum, csCode);

		for (i = 0; i < (sizeof(cscodes) / sizeof(cscodes[0])); i++)
		{
			if (cscodes[i].csCode == csCode)
			{
				strcat(s, "=");
				strcat(s, cscodes[i].name);
				break;
			}
		}
		break;

	case 0xa002:	/* _Read */
		ioCompletion = (*((INT16*) (mem + a0 + 12)) << 16) + *((INT16*) (mem + a0 + 14));
		ioVRefNum = *((INT16*) (mem + a0 + 22));
		ioRefNum = *((INT16*) (mem + a0 + 24));
		ioBuffer = (*((INT16*) (mem + a0 + 32)) << 16) + *((INT16*) (mem + a0 + 34));
		ioReqCount = (*((INT16*) (mem + a0 + 36)) << 16) + *((INT16*) (mem + a0 + 38));
		ioPosOffset = (*((INT16*) (mem + a0 + 46)) << 16) + *((INT16*) (mem + a0 + 48));
		sprintf(s, " ioCompletion=0x%08x ioVRefNum=%i ioRefNum=%i ioBuffer=0x%08x ioReqCount=%i ioPosOffset=%i",
			ioCompletion, ioVRefNum, ioRefNum, ioBuffer, ioReqCount, ioPosOffset);
		break;

	case 0xa04e:	/* _AddDrive */
		sprintf(s, " drvrRefNum=%i drvNum=%i qEl=0x%08x", (int) (INT16) d0, (int) (INT16) d1, a0);
		break;

	case 0xa9a0:	/* _GetResource */
		/* HACKHACK - the 'type' output assumes that the host is little endian
		 * since this is just trace code it isn't much of an issue
		 */
		sprintf(s, " type='%c%c%c%c' id=%i", (char) mem[a7+3], (char) mem[a7+2],
			(char) mem[a7+5], (char) mem[a7+4], *((INT16*) (mem + a7)));
		break;

	case 0xa815:	/* _SCSIDispatch */
		i = *((UINT16*) (mem + a7));
		if (i < (sizeof(scsisels) / sizeof(scsisels[0])))
			if (scsisels[i])
				sprintf(s, " (%s)", scsisels[i]);
		break;
	}

	logerror("mac_trace_trap: %s at 0x%08x: %s\n",cpu_name_local, addr, buf);
}
#endif



/*
	R Nabet 000531 : added keyboard code
*/

/* *************************************************************************
 * non-ADB keyboard support
 *
 * The keyboard uses a i8021 (?) microcontroller.
 * It uses a bidirectional synchonous serial line, connected to the VIA (SR feature)
 *
 * Our emulation is more a hack than anything else - the keyboard controller is
 * not emulated, instead we interpret keyboard commands directly.  I made
 * many guesses, which may be wrong
 *
 * todo :
 * * find the correct model number for the Mac Plus keyboard ?
 * * emulate original Macintosh keyboards (2 layouts : US and international)
 *
 * references :
 * * IM III-29 through III-32 and III-39 through III-42
 * * IM IV-250
 * *************************************************************************/

/* used to store the reply to most keyboard commands */
static int keyboard_reply;

/* flag set when inquiry command is in progress */
static int inquiry_in_progress;
/* timer which is used to time out inquiry */
static mame_timer *inquiry_timeout;
/* flag which is true while the keyboard data line is not ready to receive the keyboard reply */
static int hold_keyboard_reply;

/*
	scan_keyboard()

	scan the keyboard, and returns key transition code (or NULL ($7B) if none)
*/

/* keyboard matrix to detect transition */
static int key_matrix[7];

/* keycode buffer (used for keypad/arrow key transition) */
static int keycode_buf[2];
static int keycode_buf_index;

static int scan_keyboard()
{
	int i, j;
	int keybuf;
	int keycode;

	if (keycode_buf_index)
	{
		return keycode_buf[--keycode_buf_index];
	}

	for (i=0; i<7; i++)
	{
		keybuf = readinputport(i+3);

		if (keybuf != key_matrix[i])
		{	/* if state has changed, find first bit which has changed */
#if LOG_KEYBOARD
			logerror("keyboard state changed, %d %X\n", i, keybuf);
#endif

			for (j=0; j<16; j++)
			{
				if (((keybuf ^ key_matrix[i]) >> j) & 1)
				{	/* update key_matrix */
					key_matrix[i] = (key_matrix[i] & ~ (1 << j)) | (keybuf & (1 << j));

					if (i < 4)
					{
						/* create key code */
						keycode = (i << 5) | (j << 1) | 0x01;
						if (! (keybuf & (1 << j)))
						{	/* key up */
							keycode |= 0x80;
						}
						return keycode;
					}
					else if (i < 6)
					{
						/* create key code */
						keycode = ((i & 3) << 5) | (j << 1) | 0x01;

						if ((keycode == 0x05) || (keycode == 0x0d) || (keycode == 0x11) || (keycode == 0x1b))
						{	/* these keys cause shift to be pressed (for compatibility with mac 128/512) */
							if (keybuf & (1 << j))
							{	/* key down */
								if (! (key_matrix[3] & 0x0100))
								{	/* shift key is really up */
									keycode_buf[0] = keycode;
									keycode_buf[1] = 0x79;
									keycode_buf_index = 2;
									return 0x71;	/* "presses" shift down */
								}
							}
							else
							{	/* key up */
								if (! (key_matrix[3] & 0x0100))
								{	/* shift key is really up */
									keycode_buf[0] = keycode | 0x80;;
									keycode_buf[1] = 0x79;
									keycode_buf_index = 2;
									return 0xF1;	/* "releases" shift */
								}
							}
						}

						if (! (keybuf & (1 << j)))
						{	/* key up */
							keycode |= 0x80;
						}
						keycode_buf[0] = keycode;
						keycode_buf_index = 1;
						return 0x79;
					}
					else /* i == 6 */
					{
						/* create key code */
						keycode = (j << 1) | 0x01;
						if (! (keybuf & (1 << j)))
						{	/* key up */
							keycode |= 0x80;
						}
						keycode_buf[0] = keycode;
						keycode_buf_index = 1;
						return 0x79;
					}
				}
			}
		}
	}

	return 0x7B;	/* return NULL */
}

/*
	power-up init
*/
static void keyboard_init(void)
{
	int i;

	/* init flag */
	inquiry_in_progress = FALSE;

	/* clear key matrix */
	for (i=0; i<7; i++)
	{
		key_matrix[i] = 0;
	}

	/* purge transmission buffer */
	keycode_buf_index = 0;
}

/*
	called when inquiry times out (1/4s)
*/
static void inquiry_timeout_func(int unused)
{
#if LOG_KEYBOARD
	logerror("keyboard enquiry timeout\n");
#endif

	inquiry_in_progress = FALSE;

	if (hold_keyboard_reply)
		keyboard_reply = 0x7B;
	else
		via_set_input_si(0, 0x7B);	/* always send NULL */
}

/*
	called when a command is received from the mac
*/
static void keyboard_receive(int val)
{
	hold_keyboard_reply = TRUE;

	if (inquiry_in_progress)
	{	/* new command aborts last inquiry */
		inquiry_in_progress = FALSE;
		mame_timer_reset(inquiry_timeout, time_never);
	}

	switch (val)
	{
	case 0x10:
		/* inquiry - returns key transition code, or NULL ($7B) if time out (1/4s) */
#if LOG_KEYBOARD
		logerror("keyboard command : inquiry\n");
#endif
		keyboard_reply = scan_keyboard();
		if (keyboard_reply == 0x7B)
		{	
			/* if NULL, wait until key pressed or timeout */
			inquiry_in_progress = TRUE;
			mame_timer_adjust(inquiry_timeout,
				make_mame_time(0, DOUBLE_TO_SUBSECONDS(0.25)),
				0, time_zero);
		}
		break;

	case 0x14:
		/* instant - returns key transition code, or NULL ($7B) */
#if LOG_KEYBOARD
		logerror("keyboard command : instant\n");
#endif
		keyboard_reply = scan_keyboard();
		break;

	case 0x16:
		/* model number - resets keyboard, return model number */
#if LOG_KEYBOARD
		logerror("keyboard command : model number\n");
#endif

		{	/* reset */
			int i;

			/* clear key matrix */
			for (i=0; i<7; i++)
			{
				key_matrix[i] = 0;
			}

			/* purge transmission buffer */
			keycode_buf_index = 0;
		}

		/* format : 1 if another device (-> keypad ?) connected | next device (-> keypad ?) number 1-8
							| keyboard model number 1-8 | 1  */
		/* keyboards :
			3 : mac 512k, US and international layout ? Mac plus ???
			other values : Apple II keyboards ?
		*/
		/* keypads :
			??? : standard keypad (always available on Mac Plus) ???
		*/
		keyboard_reply = 0x17;	/* probably wrong */
		break;

	case 0x36:
		/* test - resets keyboard, return ACK ($7D) or NAK ($77) */
#if LOG_KEYBOARD
		logerror("keyboard command : test\n");
#endif
		keyboard_reply = 0x7D;	/* ACK */
		break;

	default:
#if LOG_KEYBOARD
		logerror("unknown keyboard command 0x%X\n", val);
#endif
		keyboard_reply = 0;
		break;
	}
}

/*
	called when the VIA SR is set as input
	(this is seen by the keyboard because it causes the keyboard data line to go high)
*/
static void keyboard_send_reply(void)
{
	hold_keyboard_reply = FALSE;

	if (! inquiry_in_progress)
	{
#if LOG_KEYBOARD
		logerror("keyboard reply sent 0x%X\n", keyboard_reply);
#endif
		via_set_input_si(0, keyboard_reply);
	}
}

/* *************************************************************************
 * Mouse
 * *************************************************************************/

static int mouse_bit_x = 0, mouse_bit_y = 0;

static void mouse_callback(void)
{
	static int	last_mx = 0, last_my = 0;
	static int	count_x = 0, count_y = 0;

	int			new_mx, new_my;
	int			x_needs_update = 0, y_needs_update = 0;

	new_mx = readinputport(1);
	new_my = readinputport(2);

	/* see if it moved in the x coord */
	if (new_mx != last_mx)
	{
		int		diff = new_mx - last_mx;

		/* check for wrap */
		if (diff > 0x80)
			diff = 0x100-diff;
		if  (diff < -0x80)
			diff = -0x100-diff;

		count_x += diff;

		last_mx = new_mx;
	}
	/* see if it moved in the y coord */
	if (new_my != last_my)
	{
		int		diff = new_my - last_my;

		/* check for wrap */
		if (diff > 0x80)
			diff = 0x100-diff;
		if  (diff < -0x80)
			diff = -0x100-diff;

		count_y += diff;

		last_my = new_my;
	}

	/* update any remaining count and then return */
	if (count_x)
	{
		if (count_x < 0)
		{
			count_x++;
			mouse_bit_x = 0;
		}
		else
		{
			count_x--;
			mouse_bit_x = 1;
		}
		x_needs_update = 1;
	}
	else if (count_y)
	{
		if (count_y < 0)
		{
			count_y++;
			mouse_bit_y = 1;
		}
		else
		{
			count_y--;
			mouse_bit_y = 0;
		}
		y_needs_update = 1;
	}

	if (x_needs_update || y_needs_update)
		/* assert Port B External Interrupt on the SCC */
		mac_scc_mouse_irq( x_needs_update, y_needs_update );
}

/* *************************************************************************
 * SCSI
 *
 * NCR/Symbios 5380 SCSI control chip
 * *************************************************************************/

#define scsiRd           0x000
#define scsiWr           0x001

#define sCDR             0x000       /* current scsi data register  (r/o) */
#define sODR             0x000       /* output data register        (w/o) */
#define sICR             0x010       /* initiator command register  (r/w) */
#define sMR              0x020       /* mode register               (r/w) */
#define sTCR             0x030       /* target command register     (r/w) */
#define sCSR             0x040       /* current SCSI bus status     (r/o) */
#define sSER             0x040       /* select enable register      (w/o) */
#define sBSR             0x050       /* bus and status register     (r/o) */
#define sDMAtx           0x050       /* start DMA send              (w/o) */
#define sIDR             0x060       /* input data register         (r/o) */
#define sTDMArx          0x060       /* start DMA target receive    (w/o) */
#define sRESET           0x070       /* reset parity/interrupt      (r/o) */
#define sIDMArx          0x070       /* start DMA initiator receive (w/o) */
#define dackWr           0x200       /* DACK write */
#define dackRd           0x200       /* DACK read */

static UINT16 scsi_state[0x10000];
static struct
{
	UINT8 reset;
	UINT8 IRQ;
	UINT8 DRQ;
	UINT8 EOP;
	UINT8 DACK;
	UINT8 ready;
	UINT8 A0;
	UINT8 A1;
	UINT8 A2;
	UINT8 CS;
	UINT8 I_OW;
	UINT8 I_OR;
	UINT8 D0_D7;
	UINT8 MSG;
	UINT8 C_D;
	UINT8 I_O;
	UINT8 ACK;
	UINT8 REQ;
	UINT8 DBP;
	UINT8 DB0_DB7;
	UINT8 rst;
	UINT8 bsy;
	UINT8 sel;
	UINT8 atn;
} scsi_chip;

static int scsi_do_check;

static UINT8 scsi_state_br(int offset)
{
	UINT16 res;
	res = scsi_state[offset / 2];
	if ((offset & 1) == 0)
		res >>= 8;
	return (UINT8) res;
}

static void scsi_state_bw(int offset, UINT8 b)
{
	scsi_state[offset / 2] &= (offset % 2) ? 0xff00 : 0x00ff;
	scsi_state[offset / 2] |= (offset % 2) ? ((UINT16) b) : (((UINT16) b) << 8);
}

static void scsi_chipreset(void)
{
#if LOG_SCSI
	logerror("scsi_busreset(): reseting chip\n");
#endif

	scsi_state_bw(scsiRd+sCDR, 0);
	scsi_state_bw(scsiWr+sODR, 0);
	scsi_state_bw(scsiRd+sICR, 0);
	scsi_state_bw(scsiWr+sICR, 0);
	scsi_state_bw(scsiRd+sMR, 0);
	scsi_state_bw(scsiWr+sMR, 0);
	scsi_state_bw(scsiRd+sTCR, 0);
	scsi_state_bw(scsiWr+sTCR, 0);
	scsi_state_bw(scsiRd+sCSR, 0);
	scsi_state_bw(scsiWr+sSER, 0);
	scsi_state_bw(scsiRd+sBSR, 0);
	scsi_state_bw(scsiWr+sDMAtx, 0);
	scsi_state_bw(scsiRd+sIDR, 0);
	scsi_state_bw(scsiWr+sTDMArx, 0);
	scsi_state_bw(scsiRd+sRESET, 0);
	scsi_state_bw(scsiWr+sIDMArx, 0);
	scsi_state_bw(scsiRd+sODR+dackWr, 0);
	scsi_state_bw(scsiWr+sIDR+dackRd, 0);
}

static void scsi_busreset(void)
{
#if LOG_SCSI
	logerror("scsi_busreset(): reseting bus\n");
#endif

	scsi_state_bw(scsiRd+sCDR, 0);
	scsi_state_bw(scsiWr+sODR, 0);
	scsi_state_bw(scsiRd+sICR, scsi_state_br(scsiRd+sICR) & 0x80);
	scsi_state_bw(scsiWr+sICR, scsi_state_br(scsiWr+sICR) & 0x80);
	scsi_state_bw(scsiRd+sMR, scsi_state_br(scsiRd+sMR) & 0x40);
	scsi_state_bw(scsiWr+sMR, scsi_state_br(scsiWr+sMR) & 0x40);
	scsi_state_bw(scsiRd+sTCR, 0);
	scsi_state_bw(scsiWr+sTCR, 0);
	scsi_state_bw(scsiRd+sCSR, 0x80);
	scsi_state_bw(scsiWr+sSER, 0);
	scsi_state_bw(scsiRd+sBSR, 0x10);
	scsi_state_bw(scsiWr+sDMAtx, 0);
	scsi_state_bw(scsiRd+sIDR, 0);
	scsi_state_bw(scsiWr+sTDMArx, 0);
	scsi_state_bw(scsiRd+sRESET, 0);
	scsi_state_bw(scsiWr+sIDMArx, 0);
	scsi_state_bw(scsiRd+sODR+dackWr, 0);
	scsi_state_bw(scsiWr+sIDR+dackRd, 0);

	scsi_chip.IRQ = 1;
	scsi_chip.DRQ = 0;
	scsi_chip.EOP = 0;
	scsi_chip.DACK = 0;
	scsi_chip.ready = 0;
	scsi_chip.A0 = 0;
	scsi_chip.A1 = 0;
	scsi_chip.A2 = 0;
	scsi_chip.CS = 0;
	scsi_chip.I_OW = 0;
	scsi_chip.I_OR = 0;
	scsi_chip.D0_D7 = 0;
	scsi_chip.MSG = 0;
	scsi_chip.C_D = 0;
	scsi_chip.I_O = 0;
	scsi_chip.ACK = 0;
	scsi_chip.REQ = 0;
	scsi_chip.DBP = 0;
	scsi_chip.DB0_DB7 = 0;
	scsi_chip.bsy = 0;
	scsi_chip.sel = 0;
	scsi_chip.atn = 0;

	/* In vMac, there is code at this point that ORs the word at 0xb22 with
	 * 0x8000.  The purpose of that eludes me
	 */
}

static void scsi_checkpins(void)
{
	if (scsi_chip.reset == 1)
		scsi_chipreset();

	if (scsi_chip.rst == 1)
	{
		scsi_state_bw(scsiRd + sICR, scsi_state_br(scsiRd + sICR) | 0x80);
		scsi_state_bw(scsiRd + sCSR, 0x80);
		scsi_state_bw(scsiRd + sBSR, 0x10);
		scsi_busreset();
	}
	else
	{
		scsi_state_bw(scsiRd + sICR, scsi_state_br(scsiRd + sICR) & ~0x80);
		scsi_state_bw(scsiRd + sCSR, scsi_state_br(scsiRd + sCSR) & ~0x80);
	}

	if (scsi_chip.sel == 1)
	{
		scsi_state_bw(scsiRd + sCSR, scsi_state_br(scsiRd + sCSR) | 0x02);
		scsi_state_bw(scsiRd + sBSR, 0x10);
	}
	else
	{
		scsi_state_bw(scsiRd + sCSR, scsi_state_br(scsiRd + sCSR) & ~0x02);
	}
}

static void scsi_check(void)
{
	scsi_checkpins();

	if ((scsi_state_br(scsiWr + sICR) >> 7) == 1)	/* Check assert RST */
		scsi_chip.rst = 1;
	else
		scsi_chip.rst = 0;

	if ((scsi_state_br(scsiWr + sICR) >> 2) == 1)	/* Check assert SEL */
		scsi_chip.sel = 1;
	else
		scsi_chip.sel = 0;

	/* Arbitration select/reselect */
	if ((scsi_state_br(scsiWr + sODR) >> 7) == 1)
	{
		if ((scsi_state_br(scsiWr + sMR) & 1) == 1)
		{
			/* Dummy - indicate arbitration in process */
			scsi_state_bw(scsiRd + sICR, scsi_state_br(scsiRd + sICR) | 0x40);
			/* that we didn't lose arbitration */
			scsi_state_bw(scsiRd + sICR, scsi_state_br(scsiRd + sICR) & ~0x20);
			/* and no higher priority present */
			scsi_state_bw(scsiRd + sCDR, 0x00);

			/* This will make arbitration work, but when it actually tries to
			 * connect it will fail
			 */
		}
	}
	scsi_checkpins();
}

READ16_HANDLER ( macplus_scsi_r )
{
#if LOG_SCSI
	logerror("macplus_scsi_r: offset=0x%08x pc=0x%08x\n", offset, (int) activecpu_get_pc());
#endif

	offset <<= 1;
	offset %= sizeof(scsi_state);
	offset /= sizeof(scsi_state[0]);

	return scsi_state[offset];
}

WRITE16_HANDLER ( macplus_scsi_w )
{
#if LOG_SCSI
	logerror("macplus_scsi_w: offset=0x%08x data=0x%04x pc=0x%08x\n", offset, data, (int) activecpu_get_pc());
#endif

	offset <<= 1;
	offset %= sizeof(scsi_state);
	offset /= sizeof(scsi_state[0]);

	scsi_state[offset] &= (UINT16) mem_mask;
	scsi_state[offset] |= (UINT16) data;
	scsi_do_check = 1;

	scsi_check();
}

/* *************************************************************************
 * SCC
 *
 * Serial Control Chip
 * *************************************************************************/

static int scc_mode;
static int scc_reg;
static int scc_status;

static void scc_init(void)
{
	scc_mode = 0;
	scc_reg = 0;
	scc_status = 0;
}

void mac_scc_mouse_irq( int x, int y)
{
	static int last_was_x = 0;

	if (x && y)
	{
		if (last_was_x)
			scc_status = 0x0a;
		else
			scc_status = 0x02;

		last_was_x ^= 1;
	}
	else
	{
		if (x)
			scc_status = 0x0a;
		else
			scc_status = 0x02;
	}

	//cpu_set_irq_line(0, 2, ASSERT_LINE);
	set_scc_interrupt(1);
}

static int scc_getareg(void)
{
	/* Not yet implemented */
	return 0;
}

static int scc_getbreg(void)
{

	if (scc_reg == 2)
		return scc_status;

	return 0;
}

static void scc_putareg(int data)
{
	if (scc_reg == 0)
	{
		if (data & 0x10)
			//cpu_set_irq_line(0, 2, CLEAR_LINE);	/* ack irq */
			set_scc_interrupt(0);
	}
}

static void scc_putbreg(int data)
{
	if (scc_reg == 0)
	{
		if (data & 0x10)
			//cpu_set_irq_line(0, 2, CLEAR_LINE);	/* ack irq */
			set_scc_interrupt(0);
	}
}

READ16_HANDLER ( mac_scc_r )
{
	int result;

#if LOG_SCC
	logerror("mac_scc_r: offset=0x%08x\n", offset);
#endif

	result = 0;
	offset &= 3;

	switch(offset)
	{
	case 0:
		/* Channel B (Printer Port) Control */
		if (scc_mode == 1)
			scc_mode = 0;
		else
			scc_reg = 0;

		result = scc_getbreg();
		break;

	case 1:
		/* Channel A (Modem Port) Control */
		if (scc_mode == 1)
			scc_mode = 0;
		else
			scc_reg = 0;

		result = scc_getareg();
		break;

	case 2:
		/* Channel B (Printer Port) Data */
		/* Not yet implemented */
		break;

	case 3:
		/* Channel A (Modem Port) Data */
		/* Not yet implemented */
		break;
	}

	return (result << 8) | result;
}

WRITE16_HANDLER ( mac_scc_w )
{
	offset &= 3;

	data &= 0xff;

	switch(offset)
	{
	case 0:
		/* Channel B (Printer Port) Control */
		if (scc_mode == 0)
		{
			scc_mode = 1;
			scc_reg = data & 0x0f;
			scc_putbreg(data & 0xf0);
		}
		else
		{
			scc_mode = 0;
			scc_putbreg(data);
		}
		break;

	case 1:
		/* Channel A (Modem Port) Control */
		if (scc_mode == 0)
		{
			scc_mode = 1;
			scc_reg = data & 0x0f;
			scc_putareg(data & 0xf0);
		}
		else
		{
			scc_mode = 0;
			scc_putareg(data);
		}
		break;

	case 2:
		/* Channel B (Printer Port) Data */
		/* Not yet implemented */
		break;

	case 3:
		/* Channel A (Modem Port) Data */
		/* Not yet implemented */
		break;
	}
}

/* *************************************************************************
 * RTC
 *
 * Real Time Clock chip - contains clock information and PRAM.  This chip is
 * accessed through the VIA
 * *************************************************************************/

/* state of rTCEnb and rTCClk lines */
static unsigned char rtc_rTCEnb;
static unsigned char rtc_rTCClk;

/* serial transmit/receive register : bits are shifted in/out of this byte */
static unsigned char rtc_data_byte;
/* serial transmitted/received bit count */
static unsigned char rtc_bit_count;
/* direction of the current transfer (0 : VIA->RTC, 1 : RTC->VIA) */
static unsigned char rtc_data_dir;
/* when rtc_data_dir == 1, state of rTCData as set by RTC (-> data bit seen by VIA) */
static unsigned char rtc_data_out;

/* set to 1 when a write command is in progress (-> requires another byte with immediate data) */
static unsigned char rtc_write_cmd_in_progress;
/* set to 1 when command in progress */
static unsigned char rtc_cmd;

/* write protect flag */
static unsigned char rtc_write_protect;

/* internal seconds register */
static unsigned char rtc_seconds[/*8*/4];
/* 20-byte long PRAM */
static unsigned char rtc_ram[20];

/* a few protos */
static void rtc_write_rTCEnb(int data);
static void rtc_execute_cmd(int data);

/* init the rtc core */
static void rtc_init(void)
{
	rtc_rTCClk = 0;

	rtc_write_protect = TRUE;	/* Mmmmh... Should be saved with the NVRAM, actually... */
	//rtc_write_cmd_in_progress = rtc_data_byte = rtc_bit_count = rtc_data_dir = rtc_data_out = 0;
	rtc_rTCEnb = 0;
	rtc_write_rTCEnb(1);
}

/* write the rTCEnb state */
static void rtc_write_rTCEnb(int val)
{
	if (val && (! rtc_rTCEnb))
	{
		/* rTCEnb goes high (inactive) */
		rtc_rTCEnb = 1;
		/* abort current transmission */
		rtc_write_cmd_in_progress = rtc_data_byte = rtc_bit_count = rtc_data_dir = rtc_data_out = 0;
	}
	else if ((! val) && rtc_rTCEnb)
	{
		/* rTCEnb goes low (active) */
		rtc_rTCEnb = 0;
		/* abort current transmission */
		rtc_write_cmd_in_progress = rtc_data_byte = rtc_bit_count = rtc_data_dir = rtc_data_out = 0;
	}
}

/* shift data (called on rTCClk high-to-low transition (?)) */
static void rtc_shift_data(int data)
{
	if (rtc_rTCEnb)
		/* if enable line inactive (high), do nothing */
		return;

	if (rtc_data_dir)
	{	/* RTC -> VIA transmission */
		rtc_data_out = (rtc_data_byte >> --rtc_bit_count) & 0x01;
#if LOG_RTC
		logerror("RTC shifted new data %d\n", rtc_data_out);
#endif
	}
	else
	{	/* VIA -> RTC transmission */
		rtc_data_byte = (rtc_data_byte << 1) | (data ? 1 : 0);

		if (++rtc_bit_count == 8)
		{	/* if one byte received, send to command interpreter */
			rtc_execute_cmd(rtc_data_byte);
		}
	}
}

/* called every second, to increment the Clock count */
static void rtc_incticks(void)
{
#if LOG_RTC
	logerror("rtc_incticks called\n");
#endif

	if (++rtc_seconds[0] == 0)
		if (++rtc_seconds[1] == 0)
			if (++rtc_seconds[2] == 0)
				++rtc_seconds[3];

	/*if (++rtc_seconds[4] == 0)
		if (++rtc_seconds[5] == 0)
			if (++rtc_seconds[6] == 0)
				++rtc_seconds[7];*/
}

/* Executes a command.
Called when the first byte after "enable" is received, and when the data byte after a write command
is received. */
static void rtc_execute_cmd(int data)
{
	int i;

#if LOG_RTC
	logerror("rtc_execute_cmd: data=%i\n", data);
#endif

	/* Time to execute a command */
	if (rtc_write_cmd_in_progress)
	{
		/* Writing an RTC register */
		i = (rtc_cmd >> 2) & 0x1f;
		if (rtc_write_protect && (i != 13))
			/* write-protection : only write-protect can be written again */
			return;
		switch(i)
		{
		case 0: case 1: case 2: case 3:	/* seconds register */
		case 4: case 5: case 6: case 7:	/* ??? (not described in IM III) */
			/* after various tries, I assumed rtc_seconds[4+i] is mapped to rtc_seconds[i] */
#if LOG_RTC
			logerror("RTC clock write, address = %X, data = %X\n", i, (int) rtc_data_byte);
#endif
			rtc_seconds[i & 3] = rtc_data_byte;
			break;

		case 8: case 9: case 10: case 11:	/* RAM address $10-$13 */
#if LOG_RTC
			logerror("RTC RAM write, address = %X, data = %X\n", (i & 3) + 0x10, (int) rtc_data_byte);
#endif
			rtc_ram[(i & 3) + 0x10] = rtc_data_byte;
			break;

		case 12:
			/* Test register - do nothing */
#if LOG_RTC
			logerror("RTC write to test register, data = %X\n", (int) rtc_data_byte);
#endif
			break;

		case 13:
			/* Write-protect register  */
#if LOG_RTC
			logerror("RTC write to write-protect register, data = %X\n", (int) rtc_data_byte);
#endif
			rtc_write_protect = (rtc_data_byte & 0x80) ? TRUE : FALSE;
			break;

		case 16: case 17: case 18: case 19:	/* RAM address $00-$0f */
		case 20: case 21: case 22: case 23:
		case 24: case 25: case 26: case 27:
		case 28: case 29: case 30: case 31:
#if LOG_RTC
			logerror("RTC RAM write, address = %X, data = %X\n", i & 15, (int) rtc_data_byte);
#endif
			rtc_ram[i & 15] = rtc_data_byte;
			break;

		default:
			logerror("Unknown RTC write command : %X, data = %d\n", (int) rtc_cmd, (int) rtc_data_byte);
			break;
		}
		//rtc_write_cmd_in_progress = FALSE;
	}
	else
	{
		if ((rtc_data_byte & 0x03) != 0x01)
		{
			logerror("Unknown RTC command : %X\n", (int) rtc_cmd);
			return;
		}

		rtc_cmd = rtc_data_byte;
		if (rtc_cmd & 0x80)
		{
			/* Reading an RTC register */
			rtc_data_dir = 1;
			i = (rtc_cmd >> 2) & 0x1f;
			switch(i)
			{
			case 0: case 1: case 2: case 3:
			case 4: case 5: case 6: case 7:
				rtc_data_byte = rtc_seconds[i & 3];
#if LOG_RTC
				logerror("RTC clock read, address = %X -> data = %X\n", i, rtc_data_byte);
#endif
				break;

			case 8: case 9: case 10: case 11:
#if LOG_RTC
				logerror("RTC RAM read, address = %X\n", (i & 3) + 0x10);
#endif
				rtc_data_byte = rtc_ram[(i & 3) + 0x10];
				break;

			case 16: case 17: case 18: case 19:
			case 20: case 21: case 22: case 23:
			case 24: case 25: case 26: case 27:
			case 28: case 29: case 30: case 31:
#if LOG_RTC
				logerror("RTC RAM read, address = %X\n", i & 15);
#endif
				rtc_data_byte = rtc_ram[i & 15];
				break;

			default:
				logerror("Unknown RTC read command : %X\n", (int) rtc_cmd);
				rtc_data_byte = 0;
				break;
			}
		}
		else
		{
			/* Writing an RTC register */
			/* wait for extra data byte */
#if LOG_RTC
			logerror("RTC write, waiting for data byte\n", rtc_cmd);
#endif
			rtc_write_cmd_in_progress = TRUE;
			rtc_data_byte = 0;
			rtc_bit_count = 0;
		}
	}
}

/* should save PRAM to file */
/* TODO : save write_protect flag, save time difference with host clock */
NVRAM_HANDLER( mac )
{
	if (read_or_write)
	{
#if LOG_RTC
		logerror("Writing PRAM to file\n");
#endif
		mame_fwrite(file, rtc_ram, sizeof(rtc_ram));
	}
	else
	{
		if (file)
		{
#if LOG_RTC
			logerror("Reading PRAM from file\n");
#endif
			mame_fread(file, rtc_ram, sizeof(rtc_ram));
		}
		else
		{
#if LOG_RTC
			logerror("trashing PRAM\n");
#endif
			memset(rtc_ram, 0, sizeof(rtc_ram));
			/* Geez, what's the point, Mac ROMs can create default values quite well. */
			/*rtc_ram[0] = 168;
			rtc_ram[3] = 34;
			rtc_ram[4] = 204;
			rtc_ram[5] = 10;
			rtc_ram[6] = 204;
			rtc_ram[7] = 10;
			rtc_ram[13] = 2;
			rtc_ram[14] = 99;
			rtc_ram[16] = 3;
			rtc_ram[17] = 83;
			rtc_ram[18] = 4;
			rtc_ram[19] = 76;*/
		}

		{
			/* Now we copy the host clock into the Mac clock */
			/* Cool, isn't it ? :-) */
			/* All these functions should be ANSI */
			struct tm mac_reference;
			UINT32 seconds;

			/* The count starts on 1st January 1904 */
			mac_reference.tm_sec = 0;
			mac_reference.tm_min = 0;
			mac_reference.tm_hour = 0;
			mac_reference.tm_mday = 1;
			mac_reference.tm_mon = 0;
			mac_reference.tm_year = 4;
			mac_reference.tm_isdst = 0;

			seconds = difftime(time(NULL), mktime(& mac_reference));

#if LOG_RTC
			logerror("second count 0x%lX\n", (unsigned long) seconds);
#endif

			rtc_seconds[0] = seconds & 0xff;
			rtc_seconds[1] = (seconds >> 8) & 0xff;
			rtc_seconds[2] = (seconds >> 16) & 0xff;
			rtc_seconds[3] = (seconds >> 24) & 0xff;
		}
	}
}

/* ********************************** *
 * IWM Code specific to the Mac Plus  *
 * ********************************** */

READ16_HANDLER ( mac_iwm_r )
{
	/* The first time this is called is in a floppy test, which goes from
	 * $400104 to $400126.  After that, all access to the floppy goes through
	 * the disk driver in the MacOS
	 *
	 * I just thought this would be on interest to someone trying to further
	 * this driver along
	 */

	int result = 0;

#if LOG_MAC_IWM
	logerror("mac_iwm_r: offset=0x%08x\n", offset);
#endif

	result = iwm_r(offset >> 8);
	return (result << 8) | result;
}

WRITE16_HANDLER ( mac_iwm_w )
{
#if LOG_MAC_IWM
	logerror("mac_iwm_w: offset=0x%08x data=0x%04x\n", offset, data);
#endif

	if (ACCESSING_LSB)
		iwm_w(offset >> 8, data & 0xff);
}

DEVICE_LOAD(mac_floppy)
{
#if 0
	if ((mac_model == model_Mac128k512k) && (id == 0))
		/* on Mac 128k/512k, internal floppy is single sided */
		return sony_floppy_load(image, file, SONY_FLOPPY_ALLOW400K);
	else
#endif
		return sony_floppy_load(image, file, SONY_FLOPPY_ALLOW400K | SONY_FLOPPY_ALLOW800K);
}

DEVICE_UNLOAD(mac_floppy)
{
	sony_floppy_unload(image);
}

/* *************************************************************************
 * VIA
 * *************************************************************************
 *
 *
 * PORT A
 *
 *	bit 7	R	SCC Wait/Request
 *	bit 6	W	Main/Alternate screen buffer select
 *	bit 5	W	Floppy Disk Line Selection
 *	bit 4	W	Overlay/Normal memory mapping
 *	bit 3	W	Main/Alternate sound buffer
 *	bit 2-0	W	Sound Volume
 *
 *
 * PORT B
 *
 *	bit 7	W	Sound enable
 *	bit 6	R	Video beam in display
 *	bit 5	R	Mouse Y2
 *	bit	4	R	Mouse X2
 *	bit 3	R	Mouse button (active low)
 *	bit 2	W	Real time clock enable
 *	bit 1	W	Real time clock data clock
 *	bit 0	RW	Real time clock data
 *
 */

static READ_HANDLER(mac_via_in_a)
{
	return 0x80;
}

static READ_HANDLER(mac_via_in_b)
{
	int val = 0;

	/* video beam in display (! VBLANK && ! HBLANK basically) */
	if (cpu_getvblank())
		val |= 0x40;

	if (mouse_bit_y)	/* Mouse Y2 */
		val |= 0x20;
	if (mouse_bit_x)	/* Mouse X2 */
		val |= 0x10;
	if ((readinputport(0) & 0x01) == 0)
		val |= 0x08;
	if (rtc_data_out)
		val |= 1;

	return val;
}

static WRITE_HANDLER(mac_via_out_a)
{
	set_scc_waitrequest((data & 0x80) >> 7);
	mac_set_screen_buffer((data & 0x40) >> 6);
	sony_set_sel_line((data & 0x20) >> 5);
	set_memory_overlay((data & 0x10) >> 4);
	mac_set_sound_buffer((data & 0x08) >> 3);
	mac_set_volume(data & 0x07);
}

static WRITE_HANDLER(mac_via_out_b)
{
	int new_rtc_rTCClk;

	mac_enable_sound((data & 0x80) == 0);
	rtc_write_rTCEnb(data & 0x04);
	new_rtc_rTCClk = (data >> 1) & 0x01;
	if ((! new_rtc_rTCClk) && (rtc_rTCClk))
		rtc_shift_data(data & 0x01);
	rtc_rTCClk = new_rtc_rTCClk;
}

static void mac_via_irq(int state)
{
	/* interrupt the 68k (level 1) */
	//cpu_set_irq_line(0, 1, state);
	set_via_interrupt(state);
}

READ16_HANDLER ( mac_via_r )
{
	int data;

	offset >>= 8;
	offset &= 0x0f;

#if LOG_VIA
	logerror("mac_via_r: offset=0x%02x\n", offset);
#endif
	data = via_0_r(offset);

	return (data & 0xff) | (data << 8);
}

WRITE16_HANDLER ( mac_via_w )
{
	offset >>= 8;
	offset &= 0x0f;

#if LOG_VIA
	logerror("mac_via_w: offset=0x%02x data=0x%08x\n", offset, data);
#endif

	if (ACCESSING_MSB)
		via_0_w(offset, (data >> 8) & 0xff);
}



/* *************************************************************************
 * Main
 * *************************************************************************/

MACHINE_INIT(mac)
{
	/* initialize real-time clock */
	rtc_init();

	/* initialize serial */
	scc_init();

	/* initialize floppy */
	{
		iwm_interface intf =
		{
			sony_set_lines,
			sony_set_enable_lines,

			sony_read_data,
			sony_write_data,
			sony_read_status
		};

		iwm_init(& intf);
	}

	/* setup the memory overlay */
	set_memory_overlay(1);

	/* reset the via */
	via_reset();

	/* setup videoram */
	mac_set_screen_buffer(1);
}



static void mac_state_load(void)
{
	int overlay = mac_overlay;
	mac_overlay = -1;
	set_memory_overlay(overlay);
}



static void mac_driver_init(mac_model_t model)
{
	mac_overlay = -1;

	/* set up RAM mirror at 0x600000-0x6fffff (0x7fffff ???) */
	mac_install_memory(0x600000, 0x6fffff, mess_ram_size, mess_ram, FALSE, 2);

	/* set up ROM at 0x400000-0x4fffff (-0x5fffff for mac 128k/512k/512ke) */
	mac_install_memory(0x400000, (model == model_macplus) ? 0x4fffff : 0x5fffff,
		memory_region_length(REGION_USER1), memory_region(REGION_USER1), TRUE, 3);

	set_memory_overlay(1);

	/* configure via */
	via_config(0, &mac_via6522_intf);
	via_set_clock(0, 1000000);	/* 6522 = 1 Mhz, 6522a = 2 Mhz */

	/* setup keyboard */
	keyboard_init();

	inquiry_timeout = mame_timer_alloc(inquiry_timeout_func);

	cpuintrf_set_dasm_override(mac_dasm_override);

	/* save state stuff */
	state_save_register_int("mac", 0, "overlay", &mac_overlay);
	state_save_register_func_postload(mac_state_load);
}



DRIVER_INIT(mac128k512k)
{
	mac_driver_init(model_mac128k512k);
}

DRIVER_INIT(mac512ke)
{
	mac_driver_init(model_mac512ke);
}

DRIVER_INIT(macplus)
{
	mac_driver_init(model_macplus);
}



static void mac_vblank_irq(void)
{
	static int irq_count = 0, ca1_data = 0, ca2_data = 0;

	/* handle keyboard */
	if (inquiry_in_progress)
	{
		int keycode = scan_keyboard();

		if (keycode != 0x7B)
		{
			/* if key pressed, send the code */

			logerror("keyboard enquiry successful, keycode %X\n", keycode);

			inquiry_in_progress = FALSE;
			timer_reset(inquiry_timeout, TIME_NEVER);

			if (hold_keyboard_reply)
				keyboard_reply = keycode;
			else
				via_set_input_si(0, keycode);
		}
	}

	/* signal VBlank on CA1 input on the VIA */
	ca1_data ^= 1;
	via_set_input_ca1(0, ca1_data);

	if (++irq_count == 60)
	{
		irq_count = 0;

		ca2_data ^= 1;
		/* signal 1 Hz irq on CA2 input on the VIA */
		via_set_input_ca2(0, ca2_data);

		rtc_incticks();
	}
}



INTERRUPT_GEN( mac_interrupt )
{
	int scanline;

	mac_sh_updatebuffer();

	scanline = cpu_getscanline();
	if (scanline == 342)
		mac_vblank_irq();

	/* check for mouse changes at 10 irqs per frame */
	if (!(scanline % 10))
		mouse_callback();
}
