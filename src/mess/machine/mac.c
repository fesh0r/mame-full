#include "driver.h"
#include "mess/machine/6522via.h"
#include "vidhrdw/generic.h"

#define LOG_VIA		0
#define LOG_IWM		1
#define LOG_RTC		1

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

int macplus_scsi_r( int offset ) {
	if ( errorlog )
		fprintf( errorlog, "macplus_scsi_r: offset=0x%08x\n", offset );

	/* Not yet implemented */
	return 0;
}

void macplus_scsi_w(int offset, int data)
{
	if (errorlog)	fprintf(errorlog, "macplus_scsi_w: offset=0x%08x data=0x%04x\n", offset, data);

	/* Not yet implemented */
}

void macplus_autovector_w(int offset, int data)
{
	if (errorlog)	fprintf(errorlog, "macplus_autovector_w: offset=0x%08x data=0x%04x\n", offset, data);

	/* This should throw an exception */

	/* Not yet implemented */
}

int macplus_autovector_r(int offset)
{
	if (errorlog)	fprintf(errorlog, "macplus_autovector_r: offset=0x%08x\n", offset);

	/* This should throw an exception */

	/* Not yet implemented */
	return 0;
}

static void set_scc_waitrequest(int waitrequest)
{
	if (errorlog)	fprintf(errorlog, "set_scc_waitrequest: waitrequest=%i\n", waitrequest);

	/* Not Yet Implemented */
}

static void set_screen_buffer(int buffer)
{
	if (errorlog)	fprintf(errorlog, "set_screen_buffer: buffer=%i\n", buffer);

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

	if ( errorlog )
		fprintf( errorlog, "set_memory_overlay: overlay=%i\n", overlay );
	
	mac_overlay = overlay;
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

	if ( errorlog )
		fprintf(errorlog, "macplus_scc_r: offset=0x%08x\n", offset);

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

typedef union {
	struct {
		unsigned long seconds1;	/* big endian */
		unsigned long seconds2;	/* big endian */
	} l;
	unsigned char b[8];
} rtc_seconds_struct;

static unsigned char rtc_enabled;
static unsigned char rtc_clock;
static unsigned char rtc_mode;
static unsigned char rtc_cmd;
static unsigned char rtc_data;
static unsigned char rtc_counter;
static rtc_seconds_struct rtc_seconds;
static unsigned char rtc_ram[20];

static void rtc_init(void)
{
	rtc_enabled = rtc_clock = rtc_mode = rtc_cmd = rtc_data = rtc_counter = 0;
	memset(&rtc_seconds, 0, sizeof(rtc_seconds));
	memset(&rtc_ram, 0, sizeof(rtc_ram));

	rtc_ram[0] = 168;
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
	rtc_ram[19] = 76;

	/* TODO - Load the PRAM file */
}

static void rtc_save(void)
{
	/* TODO - Save the PRAM file */
}

#ifdef LSB_FIRST
#define ENDIANIZEW(x)	((((x) >> 8) + ((x) << 8)) & 0xffff)
#define ENDIANIZE(x)	(ENDIANIZEW((x) >> 16) + (ENDIANIZEW((x) & 0xffff) << 16))
#else
#define ENDIANIZE(x)	(x)
#endif

#if 0
/* TODO - Someone needs to call this */
static void rtc_incticks(void)
{
	unsigned long l;

	l = ENDIANIZE(rtc_seconds.l.seconds1);
	l++;
	rtc_seconds.l.seconds1 = ENDIANIZE(l);

	l = ENDIANIZE(rtc_seconds.l.seconds2);
	l++;
	rtc_seconds.l.seconds2 = ENDIANIZE(l);
}
#endif

static int rtc_get(void)
{
	int result;
	result = (rtc_data >> --rtc_counter) & 0x01;

#if LOG_RTC
	if (errorlog)	fprintf(errorlog, "rtc_get: result=%i\n", result);
#endif

	return result;
}

static void rtc_put(int data)
{
	int i;

#if LOG_RTC
	if (errorlog)	fprintf(errorlog, "rtc_put: data=%i\n", data);
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
				rtc_seconds.b[i] = rtc_data;
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
				rtc_ram[i & 7] = rtc_data;
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
					rtc_data = rtc_seconds.b[i];
					break;

				case 8: case 9: case 10: case 11:
					rtc_data = rtc_ram[(i & 3) + 0x10];
					break;
	
				case 16: case 17: case 18: case 19:
				case 20: case 21: case 22: case 23:
				case 24: case 25: case 26: case 27:
				case 28: case 29: case 30: case 31:
					rtc_data = rtc_ram[i & 7];
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

/* *************************************************************************
 * IWM (Integrated Woz Machine)
 * *************************************************************************/

enum {
	IWM_PH0		= 0x01,
	IWM_PH1		= 0x02,
	IWM_PH2		= 0x04,
	IWM_PH3		= 0x08,
	IWM_MOTOR	= 0x10,
	IWM_DRIVE	= 0x20,
	IWM_Q6		= 0x40,
	IWM_Q7		= 0x80
};

typedef struct {
	void *fd;
	unsigned int disk_switched : 1;
	unsigned int wp : 1;
	unsigned int motor : 1;
	unsigned int step : 1;
	unsigned int head : 1;
	unsigned int track : 7;
} floppy;

static int iwm_lines;		/* flags from IWM_MOTOR - IWM_Q7 */
static int iwm_mode;		/* 0-31 */
static floppy iwm_floppy;

static void iwm_init(void)
{
	iwm_lines = 0;
	iwm_mode = 0;
}

static int iwm_enable2(void)
{
	return (iwm_lines & IWM_PH1) && (iwm_lines & IWM_PH3);
}

static int iwm_readdata(void)
{
	int result = 0;

	/* Not yet implemented */

	#if LOG_IWM
		if(errorlog) fprintf(errorlog, "iwm_readdata(): result=%d\n", result);
	#endif
	return result;
}

static void iwm_writedata(int data)
{
	#if LOG_IWM
		if(errorlog) fprintf(errorlog, "iwm_writedata(): data=%d\n", data);
	#endif

	/* Not yet implemented */
}

static int iwm_readenable2handshake(void)
{
	static int val = 0;

	if (val++ > 3)
		val = 0;

	return val ? 0xc0 : 0x80;
}

static int iwm_status(void)
{
	int result = 1;
	int action;

	/* TODO - Find out how bit 0 of the action is specified... is it
	 * set_floppy_line? */
	action = (iwm_lines & (IWM_PH2 | IWM_PH1 | IWM_PH0)) << 1;

	#if LOG_IWM
		if(errorlog) fprintf(errorlog, "iwm_status(): action=%d\n", action);
	#endif

	switch(action) {
	case 0x00:	/* Step direction */
		result = iwm_floppy.step;
		break;
	case 0x01:	/* Disk in place */
		result = !iwm_floppy.fd;	/* 0=disk 1=nodisk */
		break;
	case 0x02:	/* Disk is stepping */
		result = 1;
		break;
	case 0x03:	/* Disk is locked */
		result = iwm_floppy.wp;
		break;
	case 0x04:	/* Motor on */
		result = iwm_floppy.motor;
		break;
	case 0x05:	/* At track 0 */
		result = iwm_floppy.track != 0;	/* 0=track zero 1=not track zero */
		break;
	case 0x06:	/* Disk switched */
		result = iwm_floppy.disk_switched;
		break;
	case 0x07:	/* Tachometer */
		/* Not yet implemented */
		break;
	case 0x08:	/* Lower head activate */
		iwm_floppy.head = 0;
		result = 0;
		break;
	case 0x09:	/* Upper head activate */
		iwm_floppy.head = 1;
		result = 0;
		break;
	case 0x0c:	/* Number of sides */
		result = 1;
		break;
	case 0x0d:	/* Disk ready */
		result = 1;
		break;
	case 0x0f:	/* Drive installed */
		result = 0;
		break;
	}

	return result;
}

static void iwm_doaction(void)
{
	int action;

	/* TODO - Find out how bit 1 of the action is specified... is it
	 * set_floppy_line? */
	action = ((iwm_lines & (IWM_PH1 | IWM_PH0)) << 2) | ((iwm_lines & IWM_PH2) >> 2);

	#if LOG_IWM
		if(errorlog) fprintf(errorlog, "iwm_doaction(): action=%d\n", action);
	#endif

	switch(action) {
	case 0x00:	/* Set step inward (higher tracks) */
		iwm_floppy.step = 0;
		break;
	case 0x01:	/* Set step outward (lower tracks) */
		iwm_floppy.step = 1;
		break;
	case 0x03:	/* Reset diskswitched */
		iwm_floppy.disk_switched = 0;
		break;
	case 0x04:	/* Step disk */
		if (iwm_floppy.step) {
			if (iwm_floppy.track < 79)
				iwm_floppy.track++;
		}
		else {
			if (iwm_floppy.track > 0)
				iwm_floppy.track--;
		}
		break;
	case 0x08:	/* Turn motor on */
		iwm_floppy.motor = 1;
		break;
	case 0x09:	/* Turn motor off */
		iwm_floppy.motor = 0;
		break;
	case 0x0d:	/* Eject disk */
		if (iwm_floppy.fd) {
			osd_fclose(iwm_floppy.fd);
			memset(&iwm_floppy, 0, sizeof(iwm_floppy));
		}
		break;
	}
}

static void iwm_turnmotoroff(int dummy)
{
	iwm_lines &= ~IWM_MOTOR;

	#if LOG_IWM
		if(errorlog) fprintf(errorlog, "iwm_turnmotoroff(): Turning motor off\n");
	#endif
}

static void iwm_access(int offset)
{

	if (offset & 1)
		iwm_lines |= (1 << (offset >> 1));
	else
		iwm_lines &= ~(1 << (offset >> 1));

	switch(offset) {
	case 0x07:
		if (iwm_lines & IWM_MOTOR)
			iwm_doaction();
		break;

	case 0x08:
		/* Turn off motor */
		if (iwm_mode & 0x04) {
			/* Immediately */
			iwm_lines &= ~IWM_MOTOR;

			#if LOG_IWM
				if(errorlog) fprintf(errorlog, "iwm_access(): Turning motor off\n");
			#endif
		}
		else {
			/* One second delay */
			timer_set(TIME_IN_SEC(1), 0, iwm_turnmotoroff);
		}
		break;

	case 0x09:
		/* Turn on motor */
		iwm_lines |= IWM_MOTOR;

		#if LOG_IWM
			if(errorlog) fprintf(errorlog, "iwm_access(): Turning motor on\n");
		#endif
		break;
	}
}

static int iwm_read_reg(void)
{
	int result = 0;
	int status;

	switch(iwm_lines & (IWM_Q6 | IWM_Q7)) {
	case 0:
		/* Read data register */
		if (iwm_enable2() || !(iwm_lines & IWM_MOTOR)) {
			result = 0xff;
		}
		else {
			result = iwm_readdata();	
		}
		break;

	case IWM_Q6:
		/* Read status register */
		status = iwm_enable2() ? 1 : iwm_status();
		result = (status << 7) | (((iwm_lines & IWM_MOTOR) ? 1 : 0) << 5) | iwm_mode;
		break;

	case IWM_Q7:
		/* Read handshake register */
		result = iwm_enable2() ? iwm_readenable2handshake() : 0x80;
		break;
	}
	return result;
}

static void iwm_write_reg(int data)
{
	switch(iwm_lines & (IWM_Q6 | IWM_Q7)) {
	case IWM_Q6 | IWM_Q7:
		if (!(iwm_lines & IWM_MOTOR))
			iwm_mode = data & 0x1f;	/* Write mode register */
		else if (!iwm_enable2())
			iwm_writedata(data);
		break;
	}
}

static int iwm_r(int offset)
{
	offset &= 15;
	iwm_access(offset);
	return (offset & 1) ? 0 : iwm_read_reg();
}

static void iwm_w(int offset, int data)
{
	offset &= 15;
	iwm_access(offset);
	if ( offset & 1 )
		iwm_write_reg(data);
}

/* ********************************** *
 * IWM Code specific to the Mac Plus  *
 * ********************************** */

static void set_floppy_line(int line)
{
#if LOG_IWM
	if (errorlog)	fprintf(errorlog, "set_floppy_line: line=%i\n", line);
#endif

	/* Not Yet Implemented */
}

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

#if LOG_IWM
	if (errorlog)	fprintf(errorlog, "macplus_iwm_r: offset=0x%08x\n", offset);
#endif

	result = iwm_r(offset >> 9);
	return (result << 8) | result;
}

void macplus_iwm_w(int offset, int data)
{
#if LOG_IWM
	if (errorlog)	fprintf(errorlog, "macplus_iwm_w: offset=0x%08x data=0x%04x\n", offset, data);
#endif

	if ( ( data & 0x00ff0000 ) == 0 )
		iwm_w( offset >> 9, data & 0xff );
}

int macplus_floppy_init(int id)
{
	const char *name;

	memset(&iwm_floppy, 0, sizeof(iwm_floppy));

	name = device_filename(IO_FLOPPY,id);
	if (!name)
		return INIT_OK;

#if LOG_IWM
	if(errorlog) fprintf(errorlog,"macplus_floppy_init - name is %s\n", name);
#endif

	iwm_floppy.fd = image_fopen(IO_FLOPPY, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_RW);
	if (!iwm_floppy.fd) {
		iwm_floppy.fd = image_fopen(IO_FLOPPY, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
		if (!iwm_floppy.fd)
			return INIT_FAILED;
		iwm_floppy.wp = 1;
	}

	iwm_floppy.disk_switched = 1;
	return INIT_OK;
}

void macplus_floppy_exit(int id)
{
	if (iwm_floppy.fd)
		osd_fclose(iwm_floppy.fd);
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
 *	bit 3	R	Mouse button
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
	if ( readinputport(0) & 0x01 )
		val |= 0x08;
	if (!rtc_enabled && (rtc_clock == 1) && rtc_get())
		val |= 0x01;
	return val;
}

static void macplus_via_out_a( int offset, int val ) {

	set_scc_waitrequest( ( val & 0x80 ) >> 7 );
	set_screen_buffer( ( val & 0x40 ) >> 6 );
	set_floppy_line( ( val & 0x20 ) >> 5 );
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
	if ( errorlog )
		fprintf( errorlog, "macplus_via_r: offset=0x%02x\n", offset );
#endif
	data = via_0_r( offset );
	
	return ( data & 0xff ) | ( data << 8 );
}

void macplus_via_w( int offset, int data ) {
	offset >>= 9;
	offset &= 0x0f;

#if LOG_VIA
	if ( errorlog )
		fprintf( errorlog, "macplus_via_w: offset=0x%02x data=0x%08x\n", offset, data );
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
	UINT8	*ram = memory_region(REGION_CPU1);
	
	cpu_setbank( 2, ram ); /* ram */
	cpu_setbank( 3, &ram[0x400000] ); /* rom */

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
	
	return 0;
}

