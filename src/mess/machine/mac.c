/* *************************************************************************
 * Mac hardware
 *
 * The hardware for the Mac Plus (SCSI, SCC, etc).
 *
 * Nate Woods
 * Ernesto Corvi
 *
 * TODO
 * - Fully implement SCSI
 * - Load/save PRAM to disk
 * - Call the RTC timer
 * *************************************************************************/


#include "driver.h"
#include "mess/machine/6522via.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"
#include "mess/machine/iwm.h"

#ifdef MAME_DEBUG

#define LOG_VIA			0
#define LOG_RTC			0
#define LOG_MAC_IWM		0
#define LOG_TRAPS		1
#define LOG_SCSI		1
#define LOG_SCC			0
#define LOG_GENERAL		0

#else

#define LOG_VIA			0
#define LOG_RTC			0
#define LOG_MAC_IWM		0
#define LOG_TRAPS		0
#define LOG_SCSI		0
#define LOG_SCC			0
#define LOG_GENERAL		0

#endif

/* from sndhrdw */
extern void macplus_enable_sound( int on );
extern void macplus_set_buffer( int buffer );
extern void macplus_set_volume( int volume );

/* from vidhrdw */
extern int macplus_mouse_x( void );
extern int macplus_mouse_y( void );

static int macplus_via_in_a( int offset );
static int macplus_via_in_b( int offset );
static void macplus_via_out_a( int offset, int val );
static void macplus_via_out_b( int offset, int val );
static void macplus_via_irq( int state );

static struct via6522_interface macplus_via6522_intf = {
	macplus_via_in_a, macplus_via_in_b,
	NULL, NULL,
	NULL, NULL,
	macplus_via_out_a, macplus_via_out_b,
	NULL, NULL,
	macplus_via_irq,
	NULL,
	NULL
};

static int mac_overlay = 0;

void macplus_autovector_w(int offset, int data)
{
#if LOG_GENERAL
	logerror("macplus_autovector_w: offset=0x%08x data=0x%04x\n", offset, data);
#endif

	/* This should throw an exception */

	/* Not yet implemented */
}

int macplus_autovector_r(int offset)
{
#if LOG_GENERAL
	logerror("macplus_autovector_r: offset=0x%08x\n", offset);
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

static void set_screen_buffer(int buffer)
{
#if LOG_GENERAL
	logerror("set_screen_buffer: buffer=%i\n", buffer);
#endif

	videoram = memory_region(REGION_CPU1) + 0x400000 - (buffer ? 0x5900 : 0xD900);
}

static void set_memory_overlay( int overlay ) {
	UINT8	*ram = memory_region(REGION_CPU1);

	if ( overlay ) {
		cpu_setbank( 1, &ram[0x400000] ); /* rom */
		/* HACK! - copy in the initial reset/stack */
		memcpy( ram, &ram[0x400000], 8 );
	} else {
		cpu_setbank( 1, ram ); /* ram */
	}

#if LOG_GENERAL
	logerror("set_memory_overlay: overlay=%i\n", overlay );
#endif

	mac_overlay = overlay;
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
 * Worth noting:
 *		The first call to _Read occurs in the Mac Plus at 0x00400734
 * *************************************************************************/

#if LOG_TRAPS
static char *cstrfrompstr(char *buf)
{
	static char newbuf[256];
	memcpy(newbuf, buf+1, buf[0]);
	newbuf[(int)buf[0]] = '\0';
	return newbuf;
}

void mac_tracetrap(const char *cpu_name_local, int addr, int trap)
{
	typedef struct {
		int trap;
		const char *name;
	} traptableentry;

	static const traptableentry traps[] = {
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

	typedef struct {
		int csCode;
		const char *name;
	} sonycscodeentry;

	static const sonycscodeentry cscodes[] = {
		{ 1, "KillIO" },
		{ 5, "VerifyDisk" },
		{ 6, "FormatDisk" },
		{ 7, "EjectDisk" },
		{ 8, "SetTagBuffer" },
		{ 9, "TrackCacheControl" },
		{ 23, "ReturnDriveInfo" }
	};

	static const char *scsisels[] = {
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

	buf[0] = '\0';
	for (i = 0; i < (sizeof(traps) / sizeof(traps[0])); i++) {
		if (traps[i].trap == trap) {
			strcpy(buf, traps[i].name);
			break;
		}
	}
	if (!buf[0])
		sprintf(buf, "Trap $%04x", trap);

	s = &buf[strlen(buf)];
	mem = memory_region(REGION_CPU1);
	a0 = cpu_get_reg(M68K_A0);
	a7 = cpu_get_reg(M68K_A7);
	d0 = cpu_get_reg(M68K_D0);
	d1 = cpu_get_reg(M68K_D1);

	switch(trap) {
	case 0xa004:	/* _Control */
		ioVRefNum = *((INT16*) (mem + a0 + 22));
		ioCRefNum = *((INT16*) (mem + a0 + 24));
		csCode = *((UINT16*) (mem + a0 + 26));
		sprintf(s, " ioVRefNum=%i ioCRefNum=%i csCode=%i", ioVRefNum, ioCRefNum, csCode);

		for (i = 0; i < (sizeof(cscodes) / sizeof(cscodes[0])); i++) {
			if (cscodes[i].csCode == csCode) {
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
static struct {
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

	if (scsi_chip.rst == 1) {
		scsi_state_bw(scsiRd + sICR, scsi_state_br(scsiRd + sICR) | 0x80);
		scsi_state_bw(scsiRd + sCSR, 0x80);
		scsi_state_bw(scsiRd + sBSR, 0x10);
		scsi_busreset();
	}
	else {
		scsi_state_bw(scsiRd + sICR, scsi_state_br(scsiRd + sICR) & ~0x80);
		scsi_state_bw(scsiRd + sCSR, scsi_state_br(scsiRd + sCSR) & ~0x80);
	}

	if (scsi_chip.sel == 1) {
		scsi_state_bw(scsiRd + sCSR, scsi_state_br(scsiRd + sCSR) | 0x02);
		scsi_state_bw(scsiRd + sBSR, 0x10);
	}
	else {
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
	if ((scsi_state_br(scsiWr + sODR) >> 7) == 1) {
		if ((scsi_state_br(scsiWr + sMR) & 1) == 1) {
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

int macplus_scsi_r( int offset )
{
#if LOG_SCSI
	logerror("macplus_scsi_r: offset=0x%08x pc=0x%08x\n", offset, (int) cpu_get_pc());
#endif

	offset %= sizeof(scsi_state);
	offset /= sizeof(scsi_state[0]);

	return scsi_state[offset];
}

void macplus_scsi_w(int offset, int data)
{
#if LOG_SCSI
	logerror("macplus_scsi_w: offset=0x%08x data=0x%04x pc=0x%08x\n", offset, data, (int) cpu_get_pc());
#endif

	offset %= sizeof(scsi_state);
	offset /= sizeof(scsi_state[0]);

	scsi_state[offset] &= (UINT16) (data >> 16);
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

static void scc_init( void ) {
	scc_mode = 0;
	scc_reg = 0;
	scc_status = 0;
}

void macplus_scc_mouse_irq( int x, int y ) {
	static int last_was_x = 0;

	if ( x && y ) {
		if ( last_was_x )
			scc_status = 0x0a;
		else
			scc_status = 0x02;

		last_was_x ^= 1;
	} else {
		if ( x )
			scc_status = 0x0a;
		else
			scc_status = 0x02;
	}

	cpu_set_irq_line( 0, 2, ASSERT_LINE );
}

static int scc_getareg( void ) {
	/* Not yet implemented */
	return 0;
}

static int scc_getbreg( void ) {

	if ( scc_reg == 2 )
		return scc_status;

	return 0;
}

static void scc_putareg( int data ) {
	if ( scc_reg == 0 ) {
		if ( data & 0x10 )
			cpu_set_irq_line( 0, 2, CLEAR_LINE ); /* ack irq */
	}
}

static void scc_putbreg( int data ) {
	if ( scc_reg == 0 ) {
		if ( data & 0x10 )
			cpu_set_irq_line( 0, 2, CLEAR_LINE ); /* ack irq */
	}
}

int macplus_scc_r( int offset ) {
	int result;

#if LOG_SCC
	logerror("macplus_scc_r: offset=0x%08x\n", offset);
#endif

	result = 0;
	offset &= 7;

	switch(offset >> 1) {
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

	return ( result << 8 ) | result;
}

void macplus_scc_w( int offset, int data ) {
	offset &= 7;

	data &= 0xff;

	switch(offset >> 1) {
		case 0:
			/* Channel B (Printer Port) Control */
			if ( scc_mode == 0 ) {
				scc_mode = 1;
				scc_reg = data & 0x0f;
				scc_putbreg( data & 0xf0 );
			} else {
				scc_mode = 0;
				scc_putbreg( data );
			}
		break;

		case 1:
			/* Channel A (Modem Port) Control */
			if ( scc_mode == 0 ) {
				scc_mode = 1;
				scc_reg = data & 0x0f;
				scc_putareg( data & 0xf0 );
			} else {
				scc_mode = 0;
				scc_putareg( data );
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

static unsigned char rtc_enabled;
static unsigned char rtc_clock;
static unsigned char rtc_mode;
static unsigned char rtc_cmd;
static unsigned char rtc_data;
static unsigned char rtc_counter;
static unsigned char rtc_seconds[8];
static unsigned char rtc_ram[20];
static unsigned char rtc_saved_ram[20];

static void rtc_init(void)
{
	rtc_enabled = rtc_clock = rtc_mode = rtc_cmd = rtc_data = rtc_counter = 0;
	memset(&rtc_seconds, 0, sizeof(rtc_seconds));
}

static void rtc_save(void)
{
	memcpy(rtc_saved_ram, rtc_ram, sizeof(rtc_ram));
}

static void rtc_incticks(void)
{
	if (++rtc_seconds[3] == 0)
		if (++rtc_seconds[2] == 0)
			if (++rtc_seconds[1] == 0)
				++rtc_seconds[0];

	if (++rtc_seconds[7] == 0)
		if (++rtc_seconds[6] == 0)
			if (++rtc_seconds[5] == 0)
				++rtc_seconds[4];
}

static int rtc_get(void)
{
	int result;
	result = (rtc_data >> --rtc_counter) & 0x01;

#if LOG_RTC
	logerror("rtc_get: result=%i\n", result);
#endif

	return result;
}

static void rtc_put(int data)
{
	int i;

#if LOG_RTC
	logerror("rtc_put: data=%i\n", data);
#endif

	rtc_data = (rtc_data << 1) | (data ? 1 : 0);
	if (++rtc_counter == 8) {
		/* Time to execute a command */
		if (rtc_mode) {
			/* Writing an RTC register */
			i = (rtc_cmd >> 2) & 0x1f;
			switch(i) {
			case 0: case 1: case 2: case 3:
			case 4: case 5: case 6: case 7:
				rtc_seconds[i] = rtc_data;
				break;

			case 8: case 9: case 10: case 11:
				rtc_ram[(i & 3) + 0x10] = rtc_data;
				break;

			case 12:
				/* Test register - do nothing */
				break;

			case 13:
				rtc_save();
				break;

			case 16: case 17: case 18: case 19:
			case 20: case 21: case 22: case 23:
			case 24: case 25: case 26: case 27:
			case 28: case 29: case 30: case 31:
				rtc_ram[i & 15] = rtc_data;
				break;
			}
		}
		else {
			rtc_cmd = rtc_data;
			rtc_data = 0;
			if (rtc_cmd & 0x80) {
				/* Reading an RTC register */
				i = (rtc_cmd >> 2) & 0x1f;
				switch(i) {
				case 0: case 1: case 2: case 3:
				case 4: case 5: case 6: case 7:
					rtc_data = rtc_seconds[i];
					break;

				case 8: case 9: case 10: case 11:
					rtc_data = rtc_ram[(i & 3) + 0x10];
					break;

				case 16: case 17: case 18: case 19:
				case 20: case 21: case 22: case 23:
				case 24: case 25: case 26: case 27:
				case 28: case 29: case 30: case 31:
					rtc_data = rtc_ram[i & 15];
					break;
				}
			}
			else {
				rtc_mode = 1;
				rtc_counter = 0;
			}
		}
	}
}

void macplus_nvram_handler(void *file, int read_or_write)
{
	if (read_or_write) {
		osd_fwrite(file, rtc_saved_ram, sizeof(rtc_saved_ram));
	}
	else {
		if (file) {
			osd_fread(file, rtc_saved_ram, sizeof(rtc_saved_ram));
		}
		else {
			memset(rtc_saved_ram, 0, sizeof(rtc_saved_ram));
			rtc_saved_ram[0] = 168;
			rtc_saved_ram[3] = 34;
			rtc_saved_ram[4] = 204;
			rtc_saved_ram[5] = 10;
			rtc_saved_ram[6] = 204;
			rtc_saved_ram[7] = 10;
			rtc_saved_ram[13] = 2;
			rtc_saved_ram[14] = 99;
			rtc_saved_ram[16] = 3;
			rtc_saved_ram[17] = 83;
			rtc_saved_ram[18] = 4;
			rtc_saved_ram[19] = 76;
		}
		memcpy(rtc_ram, rtc_saved_ram, sizeof(rtc_saved_ram));
	}
}

/* ********************************** *
 * IWM Code specific to the Mac Plus  *
 * ********************************** */

int macplus_iwm_r(int offset)
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
	logerror("macplus_iwm_r: offset=0x%08x\n", offset);
#endif

	result = iwm_r(offset >> 9);
	return (result << 8) | result;
}

void macplus_iwm_w(int offset, int data)
{
#if LOG_MAC_IWM
	logerror("macplus_iwm_w: offset=0x%08x data=0x%04x\n", offset, data);
#endif

	if ( ( data & 0x00ff0000 ) == 0 )
		iwm_w( offset >> 9, data & 0xff );
}

int macplus_floppy_init(int id)
{
	return iwm_floppy_init(id, IWM_FLOPPY_ALLOW400K | IWM_FLOPPY_ALLOW800K);
}

void macplus_floppy_exit(int id)
{
	iwm_floppy_exit(id);
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

static int macplus_via_in_a( int offset ) {
	return 0x80;
}

static int macplus_via_in_b( int offset ) {
	int val = 0;

	/* video beam in display ( !VBLANK && !HBLANK basically ) */
	if ( cpu_getvblank() )
		val |= 0x40;

	if ( macplus_mouse_y() )	/* Mouse Y2 */
		val |= 0x20;
	if ( macplus_mouse_x() )	/* Mouse X2 */
		val |= 0x10;
	if ( (readinputport(0) & 0x01) == 0 )
		val |= 0x08;
	if (!rtc_enabled && (rtc_clock == 1) && rtc_get())
		val |= 0x01;
	return val;
}

static void macplus_via_out_a( int offset, int val ) {

	set_scc_waitrequest( ( val & 0x80 ) >> 7 );
	set_screen_buffer( ( val & 0x40 ) >> 6 );
	iwm_set_sel_line( ( val & 0x20 ) >> 5 );
	set_memory_overlay( ( val & 0x10 ) >> 4 );
	macplus_set_buffer( ( val >> 3 ) & 0x01 );
	macplus_set_volume( val & 0x07 );
}

static void macplus_via_out_b( int offset, int val ) {

	macplus_enable_sound( ( val & 0x80 ) == 0 );
	rtc_enabled = (val >> 2) & 0x01;
	rtc_clock = (val >> 1) & 0x01;
	if ( ( rtc_enabled == 0 ) && ( rtc_clock == 1 ) )
		rtc_put( val & 0x01 );
}

static void macplus_via_irq( int state ) {
	/* interrupt the 68k (level 1) */
	cpu_set_irq_line( 0, 1, state );
}

int macplus_via_r( int offset ) {
	int data;

	offset >>= 9;
	offset &= 0x0f;

#if LOG_VIA
	logerror("macplus_via_r: offset=0x%02x\n", offset );
#endif
	data = via_0_r( offset );

	return ( data & 0xff ) | ( data << 8 );
}

void macplus_via_w( int offset, int data ) {
	offset >>= 9;
	offset &= 0x0f;

#if LOG_VIA
	logerror("macplus_via_w: offset=0x%02x data=0x%08x\n", offset, data );
#endif

	if ( ( data & 0xff000000 ) == 0 )
		via_0_w( offset, ( data >> 8 ) & 0xff );
}

/* *************************************************************************
 * Main
 * *************************************************************************/

void init_macplus( void ) {

	/* configure via */
	via_config( 0, &macplus_via6522_intf );
	via_set_clock( 0, 1000000 ); /* 6522 = 1 Mhz, 6522a = 2 Mhz */

	/* setup videoram */
	set_screen_buffer( 1 );
}

void macplus_init_machine( void )
{
	int i;
	UINT8 *ram = memory_region(REGION_CPU1);

	cpu_setbank( 2, ram ); /* ram */
	for ( i = 3; i <= 10; i++)
		cpu_setbank( i, &ram[0x400000] ); /* rom */

	/* initialize real-time clock */
	rtc_init();

	/* initialize serial */
	scc_init();

	/* initialize floppy */
	iwm_init();

	/* setup the memory overlay */
	set_memory_overlay( 1 );

	/* reset the via */
	via_reset();
}

int macplus_vblank_irq( void ) {
	static int irq_count = 0, ca1_data = 0, ca2_data = 0;

	/* signal VBlank on CA1 input on the VIA */
	ca1_data ^= 1;
	via_set_input_ca1( 0, ca1_data );

	if ( irq_count++ == 60 ) {
		irq_count = 0;

		ca2_data ^= 1;
		/* signal 1 Mhz irq on CA2 input on the VIA */
		via_set_input_ca2( 0, ca2_data );
	}

	rtc_incticks();

	return 0;
}

