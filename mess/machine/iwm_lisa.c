/* *************************************************************************
 * IWM (Integrated Woz Machine)
 *
 * Implementation of the IWM chip
 *
 * Nate Woods
 *
 * Writing this code would not be possible if it wern't for the work of the
 * XGS and KEGS emulators which also contain IWM emulations
 *
 * This chip was used as the floppy disk controller for early Macs and the
 * Apple IIgs
 *
 * TODO
 * - Implement the unimplemented IWM modes (IWM_MODE_CLOCKSPEED,
 *   IWM_MODE_BITCELLTIME, IWM_MODE_HANDSHAKEPROTOCOLIWM_MODE_LATCHMODE
 * - Support 5 1/4" floppy drives
 * - Support writing to disks (currently all disks are read only)
 * *************************************************************************/


#include "iwm_lisa.h"

#ifdef MAME_DEBUG
#define LOG_IWM			1
#define LOG_IWM_EXTRA	0
#else
#define LOG_IWM			0
#define LOG_IWM_EXTRA	0
#endif

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
	enum { bare, apple_diskcopy } image_format;
	unsigned int disk_switched : 1;
	unsigned int wp : 1;
	unsigned int motor : 1;
	unsigned int step : 1;
	//unsigned int head : 1;
	unsigned int sides : 1;	/* 0=single sided; 1=double sided */
	unsigned int track : 7;
	unsigned int track_times_8;

	unsigned int loadedtrack_valid : 1;
	unsigned int loadedtrack_head : 1;
	unsigned int loadedtrack_num : 7;
	UINT8 *loadedtrack_data;
	unsigned int loadedtrack_len;
	unsigned int loadedtrack_pos;
} floppy;

static const UINT8 iwm_tracklen_800kb[80] = {
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
	 9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	 8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8
};

static int iwm_lines;		/* flags from IWM_MOTOR - IWM_Q7 */

/*
 * IWM mode (iwm_mode):	The IWM mode has the following values:
 *
 * Bit 7	  Reserved
 * Bit 6	  Reserved
 * Bit 5	  Reserved
 * Bit 4	! Clock speed
 *				0=7MHz;	used by apple IIgs
 *				1=8MHz;	used by mac (I believe)
 * Bit 3	! Bit cell time
 *				0=4usec/bit	(used for 5.25" drives)
 *				1=2usec/bit (used for 3.5" drives)
 * Bit 2	  Motor-off delay
 *				0=leave on for 1 sec after system turns it off
 *				1=turn off immediately
 * Bit 1	! Handshake protocol
 *				0=synchronous (software supplies timing for writing data; used for 5.25" drives)
 *				1=asynchronous (IWM supplies timing; used for 3.5" drives)
 * Bit 0	! Latch mode
 *				0=read data stays valid for 7usec (used for 5.25" drives)
 *				1=read data stays valid for full byte time (used for 3.5" drives)
 */
enum {
	IWM_MODE_CLOCKSPEED			= 0x10,
	IWM_MODE_BITCELLTIME		= 0x08,
	IWM_MODE_MOTOROFFDELAY		= 0x04,
	IWM_MODE_HANDSHAKEPROTOCOL	= 0x02,
	IWM_MODE_LATCHMODE			= 0x01
};

static int iwm_mode;		/* 0-31 */

static floppy iwm_floppy[2];
//static int iwm_floppy_select;
//static int iwm_sel_line;	/* Is 0 or 1 */
static int iwm_head_line;	/* Is 0 or 1 */

#define iwm_floppy_select	((iwm_lines & IWM_DRIVE) >> 5)

void iwm_lisa_init(void)
{
	iwm_lines = 0;
	iwm_mode = 0;
}

static int iwm_enable2(void)
{
	return (iwm_lines & IWM_PH1) && (iwm_lines & IWM_PH3);
}

static int iwm_indextrack(int track, const UINT8 *tracklen)
{
	int i, len;

	len = 0;
	for (i = 0; i < track; i++)
		len += tracklen[i];
	return len;
}

#define iwm_indextrack_800kb(track)	iwm_indextrack((track), iwm_tracklen_800kb)

static void iwm_nibblize35(const UINT8 *in, UINT8 *nib_ptr, UINT8 *csum)
{
	/* This code was virtually taken verbatim from XGS */
	int	i, j;
	UINT32 c1,c2,c3,c4;
	UINT8 val;
	UINT8 w1,w2,w3,w4;
	UINT8 b1[175],b2[175],b3[175];

	/* We always return 00s for the twelve 'tag' bytes in each of	*/
	/* the 524-byte disk sectors, since the IIgs never uses them.	*/

	b1[171] = 0;
	b1[172] = 0;
	b1[173] = 0;
	b1[174] = 0;
	b2[171] = 0;
	b2[172] = 0;
	b2[173] = 0;
	b2[174] = 0;
	b3[171] = 0;
	b3[172] = 0;
	b3[173] = 0;
	b3[174] = 0;

	/* Copy from the user's buffer to our buffer, while computing
	 * the three-byte data checksum
	 */

	i = 0;
	j = 170;
	c1 = 0;
	c2 = 0;
	c3 = 0;
	while(1) {
		c1 = (c1 & 0xFF) << 1;
		if (c1 & 0x0100) c1++;

		val = in[i++];
		c3 += val;
		if (c1 & 0x0100) {
			c3++;
			c1 &= 0xFF;
		}
		b1[j] = (val ^ c1) & 0xFF;

		val = in[i++];
		c2 += val;
		if (c3 > 0xFF) {
			c2++;
			c3 &= 0xFF;
		}
		b2[j] = (val ^ c3) & 0xFF;

		if (--j < 0) break;

		val = in[i++];
		c1 += val;
		if (c2 > 0xFF) {
			c1++;
			c2 &= 0xFF;
		}
		b3[j+1] = (val ^ c2) & 0xFF;
	}
	c4 =  ((c1 & 0xC0) >> 6);
	c4 |= ((c2 & 0xC0) >> 4);
	c4 |= ((c3 & 0xC0) >> 2);

	i = 174;
	j = 0;
	while(i >= 0) {
		w1 = b1[i] & 0x3F;
		w2 = b2[i] & 0x3F;
		w3 = b3[i] & 0x3F;
		w4 =  ((b1[i] & 0xC0) >> 2);
		w4 |= ((b2[i] & 0xC0) >> 4);
		w4 |= ((b3[i] & 0xC0) >> 6);

		nib_ptr[j++] = w4;
		nib_ptr[j++] = w1;
		nib_ptr[j++] = w2;

		if (i) nib_ptr[j++] = w3;
		i--;
	}

	csum[0] = c1 & 0x3F;
	csum[1] = c2 & 0x3F;
	csum[2] = c3 & 0x3F;
	csum[3] = c4 & 0x3F;
}

static void iwm_filltrack(floppy *f, UINT8 data)
{
	f->loadedtrack_data[f->loadedtrack_pos++] = data;
	f->loadedtrack_pos %= f->loadedtrack_len;
}

static int iwm_readdata(void)
{
	int result = 0;

	/*	The IWM 3.5" drive has the following properties:
	 *
	 *	80 tracks
	 *	 2 heads
	 *
	 *	Tracks  0-15 have 12 sectors each
	 *	Tracks 16-31 have 11 sectors each
	 *	Tracks 32-47 have 10 sectors each
	 *	Tracks 48-63 have  9 sectors each
	 *	Tracks 64-79 have  8 sectors each
	 *
	 *	Each sector has 512 logical bytes
	 *
	 *	(80 tracks) * (avg of 10 sectors) * (512 bytes) * (2 sides) = 800 kB
	 *
	 *	In addition to 512 logical bytes, each sector contains 800 physical
	 *	bytes.  Here is the layout of the physical sector:
	 *
	 *	Pos
	 *	0-65	0xFF
	 *	66		0xD5
	 *	67		0xAA
	 *	68		0x96
	 *	69		diskbytes[(track number) & 0x3F]
	 *	70		diskbytes[(sector number)]
	 *	71		diskbytes[("side")]
	 *	72		diskbytes[0x22]
	 *	73		diskbytes[("sum")]
	 *	74		0xDE
	 *	75		0xAA
	 *	76-81	0xFF
	 *	82		0xD5
	 *	83		0xAA
	 *	84		0xAD
	 *	85		diskbytes[(sector number)]
	 *	86-784	"nibblized" sector data	...
	 *	785-788	checksum
	 *	789		0xDE
	 *	790		0xAA
	 *	791		0xFF
	 *
	 */

	static const UINT8 diskbytes[] = {
		0x96, 0x97, 0x9A, 0x9B,  0x9D, 0x9E, 0x9F, 0xA6, /* 0x00 */
		0xA7, 0xAB, 0xAC, 0xAD,  0xAE, 0xAF, 0xB2, 0xB3,
		0xB4, 0xB5, 0xB6, 0xB7,  0xB9, 0xBA, 0xBB, 0xBC, /* 0x10 */
		0xBD, 0xBE, 0xBF, 0xCB,  0xCD, 0xCE, 0xCF, 0xD3,
		0xD6, 0xD7, 0xD9, 0xDA,  0xDB, 0xDC, 0xDD, 0xDE, /* 0x20 */
		0xDF, 0xE5, 0xE6, 0xE7,  0xE9, 0xEA, 0xEB, 0xEC,
		0xED, 0xEE, 0xEF, 0xF2,  0xF3, 0xF4, 0xF5, 0xF6, /* 0x30 */
		0xF7, 0xF9, 0xFA, 0xFB,  0xFC, 0xFD, 0xFE, 0xFF
	};

	static const UINT8 blk1[] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xD5, 0xAA, 0x96
	};
	static const UINT8 blk2[] = {
		0xDE, 0xAA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xD5, 0xAA, 0xAD
	};
	static const UINT8 blk3[] = {
		0xDE, 0xAA, 0xFF
	};

	unsigned char buf[12 * 2 * 512];
	unsigned char nibbuf[699];
	unsigned char csum[4];
	floppy *f;
	unsigned int len;
	unsigned int sector;
	unsigned int oldpos;
	int i, sum, side;
	int imgpos;

	f = &iwm_floppy[iwm_floppy_select];

	/* Bad track? Unloaded? */
	if ((!f->fd) || (f->track < 0) || (f->track >= 80)) {
	error:
		result = 0xFF;
	}
	else {
		/* Do we need to load the track? */
		if (!f->loadedtrack_valid || (f->loadedtrack_num != f->track) || (f->loadedtrack_head != iwm_head_line)) {
			len = iwm_tracklen_800kb[f->track];
			imgpos = iwm_indextrack_800kb(f->track);

			if (f->sides) {
				imgpos *= 2;
				if (iwm_head_line)
					imgpos += len;
			}

			imgpos *= 512;

			#if LOG_IWM
				logerror("iwm_readdata(): Loading track %d (%d sectors, imgpos=0x%08x)\n",
					(int) f->track, (int) len, (int) imgpos);
			#endif

			if (osd_fseek(f->fd, (f->image_format == apple_diskcopy) ? imgpos + 84 : imgpos, SEEK_SET)) {
				#if LOG_IWM
					logerror("iwm_readdata(): osd_fseek() failed!\n");
				#endif
				goto error;
			}

			if (osd_fread(f->fd, buf, len * 512) != (len * 512)) {
				#if LOG_IWM
					logerror("iwm_readdata(): osd_fread() failed!\n");
				#endif
				goto error;
			}

			/* Unload current data, if any */
			if (f->loadedtrack_data) {
				free(f->loadedtrack_data);
				f->loadedtrack_data = NULL;
			}
			/* Set up the track data memory */
			f->loadedtrack_len = len * 800;
			f->loadedtrack_data = malloc(f->loadedtrack_len);
			if (!f->loadedtrack_data) {
				f->loadedtrack_valid = 0;
				#if LOG_IWM
					logerror("iwm_readdata(): malloc() failed!\n");
				#endif
				goto error;
			}
			f->loadedtrack_valid = 1;
			memset(f->loadedtrack_data, 0xff, f->loadedtrack_len);

			oldpos = f->loadedtrack_pos;
			f->loadedtrack_pos = 0;

			side = (f->sides && iwm_head_line) ? 0x20 : 0x00;
			if (f->track & 0x40)
				side |= 0x01;

			for (sector = 0; sector < len; sector++) {
				sum = (f->track ^ sector ^ side ^ 0x22) & 0x3F;

				for (i = 0; i < (sizeof(blk1) / sizeof(blk1[0])); i++)
					iwm_filltrack(f, blk1[i]);

				iwm_filltrack(f, diskbytes[f->track & 0x3f]);
				iwm_filltrack(f, diskbytes[sector]);
				iwm_filltrack(f, diskbytes[side]);
				iwm_filltrack(f, diskbytes[0x22]);
				iwm_filltrack(f, diskbytes[sum]);

				for (i = 0; i < (sizeof(blk2) / sizeof(blk2[0])); i++)
					iwm_filltrack(f, blk2[i]);

				iwm_filltrack(f, diskbytes[sector]);

				iwm_nibblize35(&buf[sector * 512], nibbuf, csum);
				for (i = 0; i < 699; i++)
					iwm_filltrack(f, diskbytes[nibbuf[i]]);

				for (i = 3; i >= 0; i--)
					iwm_filltrack(f, diskbytes[csum[i]]);

				for (i = 0; i < (sizeof(blk3) / sizeof(blk3[0])); i++)
					iwm_filltrack(f, blk3[i]);

				#if LOG_IWM_EXTRA
					logerror("iwm_readdata(): sector=%i csum[0..3]={0x%02x,0x%02x,0x%02x,0x%02x}\n",
						(int) sector, (int) csum[0], (int) csum[1], (int) csum[2], (int) csum[3]);
				#endif
			}

			f->loadedtrack_pos = oldpos % f->loadedtrack_len;
		}

		/*
		 * Right now, this function assumes latch mode; which is always used for
		 * 3.5 inch drives.  Eventually we should check to see if latch mode is
		 * off
		 */
		#if LOG_IWM
			if ((iwm_mode & IWM_MODE_LATCHMODE) == 0)
				logerror("iwm_readdata(): latch mode off not implemented\n");
		#endif

		/* Now actually read the data */
		result = f->loadedtrack_data[f->loadedtrack_pos++];
		f->loadedtrack_pos %= f->loadedtrack_len;
		f->loadedtrack_num = f->track;
		f->loadedtrack_head = iwm_head_line;
	}

	#if LOG_IWM_EXTRA
		logerror("iwm_readdata(): result=%d pc=0x%08x\n", result, (int) cpu_get_pc());
	#endif

	return result;
}

static void iwm_writedata(int data)
{
	#if LOG_IWM
		logerror("iwm_writedata(): data=%d\n", data);
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
	floppy *f;

	action = (iwm_lines & IWM_PH0) | (iwm_head_line << 1);

	#if LOG_IWM_EXTRA
		logerror("iwm_status(): action=%d pc=0x%08x%s\n",
			action, (int) cpu_get_pc(), (iwm_lines & IWM_MOTOR) ? "" : " (MOTOR OFF)");
	#endif

	if (iwm_lines & IWM_MOTOR) {
		f = &iwm_floppy[iwm_floppy_select];

		switch(action) {
		case 0x00:	/* write protect */
			result = f->wp ? 0 : 1;
			break;
		case 0x01:	/* optical recalibration (At track -1 ????) */
			result = /*f->track != 0*/0;	/* 0=track zero 1=not track zero */
			break;
		case 0x02:	/* eject button */
			result = 0;
			break;
		case 0x03:	/* Disk in place */
			result = !f->fd;	/* 0=disk 1=nodisk */
			break;
		default:
			#if LOG_IWM
				logerror("iwm_status(): unknown action\n");
			#endif
			break;
		}
	}

	return result;
}

#if 0
static void iwm_doaction(void)
{
	int action;
	floppy *f;

	action = ((iwm_lines & (IWM_PH1 | IWM_PH0)) << 2) | ((iwm_lines & IWM_PH2) >> 2) | (iwm_sel_line << 1);

	#if LOG_IWM
		logerror("iwm_doaction(): action=%d pc=0x%08x%s\n",
			action, (int) cpu_get_pc(), (iwm_lines & IWM_MOTOR) ? "" : " (MOTOR OFF)");
	#endif

	if (iwm_lines & IWM_MOTOR) {
		f = &iwm_floppy[iwm_floppy_select];

		switch(action) {
		case 0x00:	/* Set step inward (higher tracks) */
			f->step = 0;
			break;
		case 0x01:	/* Set step outward (lower tracks) */
			f->step = 1;
			break;
		case 0x03:	/* Reset diskswitched */
			f->disk_switched = 0;
			break;
		case 0x04:	/* Step disk */
			if (f->step) {
				if (f->track > 0)
					f->track--;
			}
			else {
				if (f->track < 79)
					f->track++;
			}
			#if LOG_IWM
				logerror("iwm_doaction(): stepping to track %i\n", (int) f->step);
			#endif
			break;
		case 0x08:	/* Turn motor on */
			f->motor = 1;
			break;
		case 0x09:	/* Turn motor off */
			f->motor = 0;
			break;
		case 0x0d:	/* Eject disk */
			#if LOG_IWM
				logerror("iwm_doaction(): ejecting disk pc=0x%08x\n", (int) cpu_get_pc());
			#endif
			if (f->fd) {
				osd_fclose(f->fd);
				memset(f, 0, sizeof(*f));
			}
			break;
		default:
			#if LOG_IWM
				logerror("iwm_doaction(): unknown action\n");
			#endif
			break;
		}
	}
}
#endif

static void iwm_turnmotoroff(int dummy)
{
	iwm_lines &= ~IWM_MOTOR;

	#if LOG_IWM_EXTRA
		logerror("iwm_turnmotoroff(): Turning motor off\n");
	#endif
}

static void iwm_access(int offset)
{
#if LOG_IWM_EXTRA
	{
		static char *lines[] = {
			"IWM_PH0",
			"IWM_PH1",
			"IWM_PH2",
			"IWM_PH3",
			"IWM_MOTOR",
			"IWM_DRIVE",
			"IWM_Q6",
			"IWM_Q7"
		};
		logerror("iwm_access(): %s line %s\n",
			(offset & 1) ? "setting" : "clearing", lines[offset >> 1]);
	}
#endif

	int old_iwm_lines = iwm_lines;

	if (offset & 1)
		iwm_lines |= (1 << (offset >> 1));
	else
		iwm_lines &= ~(1 << (offset >> 1));

	if ((offset <= 0x07) && (iwm_lines != old_iwm_lines) && (iwm_lines & IWM_MOTOR))
	{	/* PH0-3 control the motor */
		floppy *f = &iwm_floppy[iwm_floppy_select];

		int bit = (offset >> 1);
		int set = (offset & 1);

		/* trick */
		if (((iwm_lines >> ((bit + 1) & 3)) & 1) ^ set)
			f->track_times_8++;
		else
			f->track_times_8--;

		/* round to get f->track */
		f->track = (f->track_times_8 + 4) / 8;
	}

	switch(offset) {
	/*case 0x07:
		if (iwm_lines & IWM_MOTOR)
			iwm_doaction();
		break;*/

	case 0x08:
		/* Turn off motor */
		if (iwm_mode & IWM_MODE_MOTOROFFDELAY) {
			/* Immediately */
			iwm_lines &= ~IWM_MOTOR;

			#if LOG_IWM_EXTRA
				logerror("iwm_access(): Turning motor off\n");
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

		#if LOG_IWM_EXTRA
			logerror("iwm_access(): Turning motor on\n");
		#endif
		break;
	}
}

static int iwm_statusreg_r(void)
{
	/* IWM status:
	 *
	 * Bit 7	Sense input (write protect for 5.25" drive and general status line for 3.5")
	 * Bit 6	Reserved
	 * Bit 5	Drive enable (is 1 if drive is on)
	 * Bits 4-0	Same as IWM mode bits 4-0
	 */

	int result;
	int status;

	status = iwm_enable2() ? 1 : iwm_status();
	result = (status << 7) | (((iwm_lines & IWM_MOTOR) ? 1 : 0) << 5) | iwm_mode;
	return result;
}

static void iwm_modereg_w(int data)
{
	iwm_mode = data & 0x1f;	/* Write mode register */

	#if LOG_IWM_EXTRA
	logerror("iwm_modereg_w: iwm_mode=0x%02x\n", (int) iwm_mode);
	#endif
}

static int iwm_read_reg(void)
{
	int result = 0;

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
		result = iwm_statusreg_r();
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
		if (!(iwm_lines & IWM_MOTOR)) {
			iwm_modereg_w(data);
		}
		else if (!iwm_enable2()) {
			iwm_writedata(data);
		}
		break;
	}
}

int iwm_lisa_r(int offset)
{
	offset &= 15;

#if LOG_IWM_EXTRA
	logerror("iwm_r: offset=%i\n", offset);
#endif

	iwm_access(offset);
	return (offset & 1) ? 0 : iwm_read_reg();
}

void iwm_lisa_w(int offset, int data)
{
	offset &= 15;

#if LOG_IWM_EXTRA
	logerror("iwm_w: offset=%i data=0x%02x\n", offset, data);
#endif

	iwm_access(offset);
	if ( offset & 1 )
		iwm_write_reg(data);
}

void iwm_lisa_set_head_line(int head)
{
	iwm_head_line = head ? 1 : 0;

	#if LOG_IWM_EXTRA
		logerror("iwm_set_head_line(): %s line IWM_SEL\n", iwm_sel_line ? "setting" : "clearing");
	#endif
}

/*int iwm_lisa_get_sel_line(void)
{
	return iwm_sel_line;
}*/

int iwm_lisa_floppy_init(int id, int allowablesizes)
{
	const char *name;
	floppy *f;
	long image_len=0;

	f = &iwm_floppy[id];

	memset(f, 0, sizeof(*f));

	name = device_filename(IO_FLOPPY,id);
	if (!name)
		return INIT_OK;

	f->fd = image_fopen(IO_FLOPPY, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_RW);
	if (!f->fd) {
		f->fd = image_fopen(IO_FLOPPY, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
		if (!f->fd)
			goto error;
		f->wp = 1;
	}

	/* R. Nabet : added support for the diskcopy format to allow exchanges with real-world macs */
	{
		struct
		{
			UINT8 diskName[64];
			UINT32 dataSize;
			UINT32 tagSize;
			UINT32 dataChecksum;
			UINT32 tagChecksum;
			UINT8 diskFormat;	/* 0 = 400K, 1 = 800K, 2 = 720K, 3 = 1440K  (other values reserved) */
			UINT8 formatByte;	/* $12 = 400K, $22 = >400K Macintosh (DiskCopy uses this value for
								   all Apple II disks not 800K in size, and even for some of those),
								   $24 = 800K Apple II disk */
			UINT16 private;		/* always $0100 (otherwise, the file may be in a different format. */
		} header;

		f->image_format = bare;	/* default */

		/* read image header */
		if (osd_fread(f->fd, & header, sizeof(header)) == sizeof(header))
		{

#ifdef LSB_FIRST
			header.dataSize = ((header.dataSize << 24) & 0xff000000)
								| ((header.dataSize << 8) & 0x00ff0000)
								| ((header.dataSize >> 8) & 0x0000ff00)
								| ((header.dataSize >> 24) & 0x000000ff);
			header.tagSize = ((header.tagSize << 24) & 0xff000000)
								| ((header.tagSize << 8) & 0x00ff0000)
								| ((header.tagSize >> 8) & 0x0000ff00)
								| ((header.tagSize >> 24) & 0x000000ff);
			header.dataChecksum = ((header.dataChecksum << 24) & 0xff000000)
								| ((header.dataChecksum << 8) & 0x00ff0000)
								| ((header.dataChecksum >> 8) & 0x0000ff00)
								| ((header.dataChecksum >> 24) & 0x000000ff);
			header.tagChecksum = ((header.tagChecksum << 24) & 0xff000000)
								| ((header.tagChecksum << 8) & 0x00ff0000)
								| ((header.tagChecksum >> 8) & 0x0000ff00)
								| ((header.tagChecksum >> 24) & 0x000000ff);
			header.private = ((header.private << 8) & 0xff00)
								| ((header.private >> 8) & 0x00ff);
#endif
			/* various checks : */
			logerror("ninou : %d\n", osd_fsize(f->fd));
			if ((header.diskName[0] <= 63) && (osd_fsize(f->fd) == (header.dataSize + header.tagSize + 84))
					&& (header.private == 0x0100))
			{
				f->image_format = apple_diskcopy;
				image_len = header.dataSize;
			}
		}
	}

	if (f->image_format == bare)
	{
		image_len = osd_fsize(f->fd);
	}


	switch(image_len) {
	case 80*10*512*1:
		/* Single sided (400k) */
		if ((allowablesizes & IWM_FLOPPY_ALLOW400K) == 0)
			goto error;
		f->sides = 0;
		break;
	case 80*10*512*2:
		/* Double sided (800k) */
		if ((allowablesizes & IWM_FLOPPY_ALLOW800K) == 0)
			goto error;
		f->sides = 1;
		break;
	default:
		/* Bad floppy size */
		goto error;
		break;
	}

	/* For now we are always write protect */
	f->wp = 1;

	f->disk_switched = 1;

#if LOG_IWM
	logerror("macplus_floppy_init(): Loaded %s-sided floppy; id=%i name='%s' wp=%i\n",
		(f->sides ? "double" : "single"), (int) id, name, (int) f->wp);
#endif

	return INIT_OK;

error:
	if (f->fd)
		osd_fclose(f->fd);
	return INIT_FAILED;
}

void iwm_lisa_floppy_exit(int id)
{
	floppy *f;
	f = &iwm_floppy[id];

	if (f->fd)
		osd_fclose(f->fd);
	if (f->loadedtrack_data) {
		free(f->loadedtrack_data);
		f->loadedtrack_data = NULL;
	}
}

