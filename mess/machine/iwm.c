/* *************************************************************************
 * IWM (Integrated Woz Machine)
 *
 * Implementation of the IWM chip
 *
 * Nate Woods
 * Raphael Nabet
 *
 * Writing this code would not be possible if it weren't for the work of the
 * XGS and KEGS emulators which also contain IWM emulations
 *
 * This chip was used as the floppy disk controller for early Macs and the
 * Apple IIgs
 *
 * TODO
 * - Implement the unimplemented IWM modes (IWM_MODE_CLOCKSPEED,
 *   IWM_MODE_BITCELLTIME, IWM_MODE_HANDSHAKEPROTOCOLIWM_MODE_LATCHMODE
 * - Support 5 1/4" floppy drives
 * - Support writing to disks (currently all disks are read only) - WIP by R Nabet
 * *************************************************************************/


#include "iwm.h"

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
	union
	{
		/*struct
		{
		} bare;*/
		struct
		{
			long tag_offset;
		} apple_diskcopy;
	} format_specific;
	unsigned int disk_switched : 1;
	unsigned int wp : 1;
	unsigned int motor : 1;
	unsigned int step : 1;
	unsigned int head : 1;
	unsigned int sides : 1;	/* 0=single sided; 1=double sided */
	unsigned int track : 7;

	unsigned int loadedtrack_valid : 1;
	unsigned int loadedtrack_dirty : 1;
	unsigned int loadedtrack_head : 1;
	unsigned int loadedtrack_num : 7;
	UINT8 *loadedtrack_data;
	unsigned int loadedtrack_len;
	unsigned int loadedtrack_pos;
	unsigned int loadedtrack_bitpos;
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
static int iwm_sel_line;	/* Is 0 or 1 */

#define iwm_floppy_select	((iwm_lines & IWM_DRIVE) >> 5)

void iwm_init(void)
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

/*
	converts data to its nibblized representation, and generate checksum

	now handles 524-byte-long sectors

	tag data IS important, since it allows data recovery when the catalog is trashed
*/
static void iwm_nibblize35(const UINT8 *in, UINT8 *nib_ptr, UINT8 *csum)
{
	/* This code was virtually taken verbatim from XGS */
	int	i, j;
	UINT32 c1,c2,c3,c4;
	UINT8 val;
	UINT8 w1,w2,w3,w4;
	UINT8 b1[175],b2[175],b3[175];

	/* Copy from the user's buffer to our buffer, while computing
	 * the three-byte data checksum
	 */

	i = 0;
	j = 0;
	c1 = 0;
	c2 = 0;
	c3 = 0;
	while (1)
	{
		c1 = (c1 & 0xFF) << 1;
		if (c1 & 0x0100)
			c1++;

		val = in[i++];
		c3 += val;
		if (c1 & 0x0100)
		{
			c3++;
			c1 &= 0xFF;
		}
		b1[j] = (val ^ c1) & 0xFF;

		val = in[i++];
		c2 += val;
		if (c3 > 0xFF)
		{
			c2++;
			c3 &= 0xFF;
		}
		b2[j] = (val ^ c3) & 0xFF;

		if (i == 524) break;

		val = in[i++];
		c1 += val;
		if (c2 > 0xFF)
		{
			c1++;
			c2 &= 0xFF;
		}
		b3[j] = (val ^ c2) & 0xFF;
		j++;
	}
	c4 =  ((c1 & 0xC0) >> 6) | ((c2 & 0xC0) >> 4) | ((c3 & 0xC0) >> 2);
	b3[174] = 0;

	j = 0;
	for (i=0; i<=174; i++)
	{
		w1 = b1[i] & 0x3F;
		w2 = b2[i] & 0x3F;
		w3 = b3[i] & 0x3F;
		w4 =  ((b1[i] & 0xC0) >> 2);
		w4 |= ((b2[i] & 0xC0) >> 4);
		w4 |= ((b3[i] & 0xC0) >> 6);

		nib_ptr[j++] = w4;
		nib_ptr[j++] = w1;
		nib_ptr[j++] = w2;

		if (i != 174) nib_ptr[j++] = w3;
	}

	csum[0] = c1 & 0x3F;
	csum[1] = c2 & 0x3F;
	csum[2] = c3 & 0x3F;
	csum[3] = c4 & 0x3F;
}

/*
	does the reverse process of iwm_nibblize35
*/
static void iwm_denibblize35(UINT8 *out, const UINT8 *nib_ptr/*, UINT8 *csum*/)
{
	int	i, j;
	UINT32 c1,c2,c3,c4;
	UINT8 val;
	UINT8 w1,w2,w3=0,w4;
	UINT8 b1[175],b2[175],b3[175];

	j = 0;
	for (i=0; i<=174; i++)
	{
		w4 = nib_ptr[j++];
		w1 = nib_ptr[j++];
		w2 = nib_ptr[j++];

		if (i != 174) w3 = nib_ptr[j++];

		b1[i] = (w1 & 0x3F) | ((w4 << 2) & 0xC0);
		b2[i] = (w2 & 0x3F) | ((w4 << 4) & 0xC0);
		b3[i] = (w3 & 0x3F) | ((w4 << 6) & 0xC0);
	}

	/* Copy from the user's buffer to our buffer, while computing
	 * the three-byte data checksum
	 */

	i = 0;
	j = 0;
	c1 = 0;
	c2 = 0;
	c3 = 0;
	while (1)
	{
		c1 = (c1 & 0xFF) << 1;
		if (c1 & 0x0100)
			c1++;

		val = (b1[j] ^ c1) & 0xFF;
		c3 += val;
		if (c1 & 0x0100)
		{
			c3++;
			c1 &= 0xFF;
		}
		out[i++] = val;

		val = (b2[j] ^ c3) & 0xFF;
		c2 += val;
		if (c3 > 0xFF)
		{
			c2++;
			c3 &= 0xFF;
		}
		out[i++] = val;

		if (i == 524) break;

		val = (b3[j] ^ c2) & 0xFF;
		c1 += val;
		if (c2 > 0xFF)
		{
			c1++;
			c2 &= 0xFF;
		}
		out[i++] = val;
		j++;
	}
	c4 =  ((c1 & 0xC0) >> 6) | ((c2 & 0xC0) >> 4) | ((c3 & 0xC0) >> 2);
	b3[174] = 0;

	/*csum[0] = c1 & 0x3F;
	csum[1] = c2 & 0x3F;
	csum[2] = c3 & 0x3F;
	csum[3] = c4 & 0x3F;*/
}

/*
	Add one byte to track data (used only by iwm_get_track)
*/
static void iwm_filltrack(floppy *f, UINT8 data)
{
	/*f->loadedtrack_data[f->loadedtrack_pos++] = data;
	f->loadedtrack_pos %= f->loadedtrack_len;*/

	f->loadedtrack_data[f->loadedtrack_pos] &= 0xFF << (8 - f->loadedtrack_bitpos);
	f->loadedtrack_data[f->loadedtrack_pos] |= data >> f->loadedtrack_bitpos;
	f->loadedtrack_pos++;
	f->loadedtrack_pos %= f->loadedtrack_len;

	f->loadedtrack_data[f->loadedtrack_pos] &= 0xFF >> f->loadedtrack_bitpos;
	f->loadedtrack_data[f->loadedtrack_pos] |= data << (8 - f->loadedtrack_bitpos);
}

/*
	Fetch one byte from track data (used only by iwm_put_track)
*/
static UINT8 iwm_fetchtrack(floppy *f)
{
	UINT8 data;
	data = f->loadedtrack_data[f->loadedtrack_pos++] << f->loadedtrack_bitpos;
	f->loadedtrack_pos %= f->loadedtrack_len;

	data |= f->loadedtrack_data[f->loadedtrack_pos] >> (8 - f->loadedtrack_bitpos);

	while (! (data & 0x80))
	{
		/* let's shift one additionnal bit in from disk */
		f->loadedtrack_bitpos++;
		data <<= 1;
		data |= f->loadedtrack_data[f->loadedtrack_pos] >> (8 - f->loadedtrack_bitpos) /*& 0x01*/;
		if (f->loadedtrack_bitpos == 8)
		{
			f->loadedtrack_bitpos = 0;
			f->loadedtrack_pos++;
			f->loadedtrack_pos %= f->loadedtrack_len;
		}
	}

	return data;
}

/*
	read (generate !) the GCR (disk-level) data for the current track

	return 0 if successful
*/
static int iwm_get_track(void)
{
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
	 *	Each sector has 524 bytes, 512 of whom are really used by the Macintosh
	 *
	 *	(80 tracks) * (avg of 10 sectors) * (512 bytes) * (2 sides) = 800 kB
	 *
	 *  Data is nibblized : 3 data bytes -> 4 bytes on disk.
	 *
	 *	In addition to 512 logical bytes, each sector contains 800 physical
	 *	bytes.  Here is the layout of the physical sector:
	 *
	 *	Pos
	 *	0		0xFF (pad byte where head is turned on ???)
	 *  1-35	self synch 0xFFs (7*5) (42 bytes actually written to media)
	 *	36		0xD5
	 *	37		0xAA
	 *	38		0x96
	 *	39		diskbytes[(track number) & 0x3F]
	 *	40		diskbytes[(sector number)]
	 *	41		diskbytes[("side")]
	 *	42		diskbytes[0x22]
	 *	43		diskbytes[("sum")]
	 *	44		0xDE
	 *	45		0xAA
	 *	46		pad byte where head is turned off/on (0xFF here)
	 *  47-51	self synch 0xFFs (6 bytes actually written to media)
	 *	52		0xD5
	 *	53		0xAA
	 *	54		0xAD
	 *	55		diskbytes[(sector number)]
	 *	56-754	"nibblized" sector data	...
	 *	755-758	checksum
	 *	759		0xDE
	 *	760		0xAA
	 *	761		pad byte where head is turned off (0xFF here)
	 *
	 * Nota : "Self synch refers to a technique whereby two zeroes are inserted between each synch
	 *  byte written to the disk.", i.e. "0xFF, 0xFF, 0xFF, 0xFF, 0xFF" is actually "0xFF, 0x3F,
	 *  0xCF, 0xF3, 0xFC, 0xFF" on disk.  Since the IWM assumes the data transfer is complete when
	 *  the MSBit of its shift register is 1, we do read 4 0xFF, even though they are not
	 *  contiguous on the disk media.  Some reflexion shows that 4 synch bytes allow the IWM to
	 *  synchronize with the trailing data.
	 *
	 */

	static const UINT8 diskbytes[] =
	{
		0x96, 0x97, 0x9A, 0x9B,  0x9D, 0x9E, 0x9F, 0xA6, /* 0x00 */
		0xA7, 0xAB, 0xAC, 0xAD,  0xAE, 0xAF, 0xB2, 0xB3,
		0xB4, 0xB5, 0xB6, 0xB7,  0xB9, 0xBA, 0xBB, 0xBC, /* 0x10 */
		0xBD, 0xBE, 0xBF, 0xCB,  0xCD, 0xCE, 0xCF, 0xD3,
		0xD6, 0xD7, 0xD9, 0xDA,  0xDB, 0xDC, 0xDD, 0xDE, /* 0x20 */
		0xDF, 0xE5, 0xE6, 0xE7,  0xE9, 0xEA, 0xEB, 0xEC,
		0xED, 0xEE, 0xEF, 0xF2,  0xF3, 0xF4, 0xF5, 0xF6, /* 0x30 */
		0xF7, 0xF9, 0xFA, 0xFB,  0xFC, 0xFD, 0xFE, 0xFF
	};

	static const UINT8 blk1[] =
	{
		/*0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xD5, 0xAA, 0x96*/
		0xFF,
		0xFF, 0x3F, 0xCF, 0xF3, 0xFC, 0xFF,
		0xFF, 0x3F, 0xCF, 0xF3, 0xFC, 0xFF,
		0xFF, 0x3F, 0xCF, 0xF3, 0xFC, 0xFF,
		0xFF, 0x3F, 0xCF, 0xF3, 0xFC, 0xFF,
		0xFF, 0x3F, 0xCF, 0xF3, 0xFC, 0xFF,
		0xFF, 0x3F, 0xCF, 0xF3, 0xFC, 0xFF,
		0xFF, 0x3F, 0xCF, 0xF3, 0xFC, 0xFF,
		0xD5, 0xAA, 0x96
	};
	static const UINT8 blk2[] =
	{
		0xDE, 0xAA, 0xFF, 0xFF, 0x3F, 0xCF, 0xF3, 0xFC, 0xFF, 0xD5, 0xAA, 0xAD
	};
	static const UINT8 blk3[] =
	{
		0xDE, 0xAA, 0xFF
	};

	unsigned char buf[12 * 2 * 524];
	unsigned char nibbuf[699];
	unsigned char csum[4];
	floppy *f;
	unsigned int len;
	unsigned int sector;
	unsigned int oldpos, oldbitpos;
	int i, sum, side;
	int imgpos;

	f = &iwm_floppy[iwm_floppy_select];

	len = iwm_tracklen_800kb[f->track];
	imgpos = iwm_indextrack_800kb(f->track);

	if (f->sides)
	{
		imgpos *= 2;
		if (f->head)
			imgpos += len;
	}

	imgpos *= 512;

#if LOG_IWM
	logerror("iwm_get_track(): Loading track %d (%d sectors, imgpos=0x%08x)\n",
					(int) f->track, (int) len, (int) imgpos);
#endif

	if (osd_fseek(f->fd, (f->image_format == apple_diskcopy) ? imgpos + 84 : imgpos, SEEK_SET))
	{
		#if LOG_IWM
			logerror("iwm_get_track(): osd_fseek() failed!\n");
		#endif
		return 1;
	}

	if (osd_fread(f->fd, buf, len * 512) != (len * 512))
	{
		#if LOG_IWM
			logerror("iwm_get_track(): osd_fread() failed!\n");
		#endif
		return 1;
	}
	/* now insert tag data in */
	/* first insert enough space in */
	for (i = len-1; i >= 0; i--)
	{
		memmove(buf + i * 524 + 12, buf + i * 512, 512);
	}
	if ((f->image_format == apple_diskcopy) && (f->format_specific.apple_diskcopy.tag_offset))
	{	/* insert tag data from disk image */
		unsigned char tagbuf[12 * 2 * 12];

		imgpos = iwm_indextrack_800kb(f->track);

		if (f->sides)
		{
			imgpos *= 2;
			if (f->head)
				imgpos += len;
		}

		imgpos *= 12;

		if (osd_fseek(f->fd, f->format_specific.apple_diskcopy.tag_offset + imgpos, SEEK_SET))
		{
			#if LOG_IWM
				logerror("iwm_get_track(): osd_fseek() failed!\n");
			#endif
			return 1;
		}

		if (osd_fread(f->fd, tagbuf, len * 12) != (len * 12))
		{
			#if LOG_IWM
				logerror("iwm_get_track(): osd_fread() failed!\n");
			#endif
			return 1;
		}

		for (i = 0; i < len; i++)
		{
			memcpy(buf + i * 524, tagbuf + i * 12, 12);
		}
	}
	else
	{
		/* no tag data stored in disk image - so we zero the tag data */
		for (i = 0; i < len; i++)
		{
			memset(buf + i * 524, 0, 12);
		}
	}

	/* Unload current data, if any */
	if (f->loadedtrack_data)
	{
		free(f->loadedtrack_data);
		f->loadedtrack_data = NULL;
	}
	/* Set up the track data memory */
	f->loadedtrack_len = len * 800;
	f->loadedtrack_data = malloc(f->loadedtrack_len);
	if (!f->loadedtrack_data)
	{
		f->loadedtrack_valid = 0;
		#if LOG_IWM
			logerror("iwm_get_track(): malloc() failed!\n");
		#endif
		return 1;
	}
	f->loadedtrack_valid = 1;
	f->loadedtrack_dirty = 0;
	memset(f->loadedtrack_data, 0xff, f->loadedtrack_len);	/* fill with 0xffs */

	oldpos = f->loadedtrack_pos;
	oldbitpos = f->loadedtrack_bitpos;
	f->loadedtrack_pos = 0;
	f->loadedtrack_bitpos = 0;

	side = (f->sides && f->head) ? 0x20 : 0x00;
	if (f->track & 0x40)
		side |= 0x01;

	for (sector = 0; sector < len; sector++)
	{
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

		iwm_nibblize35(&buf[sector * 524], nibbuf, csum);
		for (i = 0; i < 699; i++)
			iwm_filltrack(f, diskbytes[nibbuf[i]]);

		for (i = 3; i >= 0; i--)
			iwm_filltrack(f, diskbytes[csum[i]]);

		for (i = 0; i < (sizeof(blk3) / sizeof(blk3[0])); i++)
			iwm_filltrack(f, blk3[i]);

		#if LOG_IWM_EXTRA
			logerror("iwm_get_track(): sector=%i csum[0..3]={0x%02x,0x%02x,0x%02x,0x%02x}\n",
				(int) sector, (int) csum[0], (int) csum[1], (int) csum[2], (int) csum[3]);
		#endif
	}

	f->loadedtrack_pos = oldpos % f->loadedtrack_len;	/* geez... who cares ? */
	f->loadedtrack_bitpos = oldbitpos;

	f->loadedtrack_num = f->track;
	f->loadedtrack_head = f->head;

	return 0;
}

/*
	put (parse !) some GCR track data
*/
static int iwm_put_track(void)
{
	/*	cf the format description iwm_get_track
     *
	 *  Here is the layout of the physical sector:
	 *
	 *	Pos
	 *	various 0xFFs (ignored)
	 *	0		0xD5
	 *	1		0xAA
	 *	2		0x96
	 *	3		diskbytes[(track number) & 0x3F]
	 *	4		diskbytes[(sector number)]
	 *	5		diskbytes[("side")]
	 *	6		diskbytes[0x22]
	 *	7		diskbytes[("sum")]
	 *	8		0xDE
	 *	9		0xAA
	 *	various 0xFFs (ignored)
	 *	0		0xD5
	 *	1		0xAA
	 *	2		0xAD
	 *	3		diskbytes[(sector number)]
	 *	4-702	"nibblized" sector data	...
	 *	703-706	checksum (ignored)
	 *	707		0xDE (ignored)
	 *	708		0xAA (ignored)
	 *	709		0xFF (ignored)
	 *
	 */

	static const UINT8 rev_diskbytes[] =
	{
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,   /* 0x00 */
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,   /* 0x10 */
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,   /* 0x20 */
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,   /* 0x30 */
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,   /* 0x40 */
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,   /* 0x50 */
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,   /* 0x60 */
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,   /* 0x70 */
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,   /* 0x80 */
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,
		-1,   -1,   -1,   -1,    -1,   -1,   0x00, 0x01, /* 0x90 */
		-1,   -1,   0x02, 0x03,  -1,   0x04, 0x05, 0x06,
		-1,   -1,   -1,   -1,    -1,   -1,   0x07, 0x08, /* 0xA0 */
		-1,   -1,   -1,   0x09,  0x0A, 0x0B, 0x0C, 0x0D,
		-1,   -1,   0x0E, 0x0F,  0x10, 0x11, 0x12, 0x13, /* 0xB0 */
		-1,   0x14, 0x15, 0x16,  0x17, 0x18, 0x19, 0x1A,
		-1,   -1,   -1,   -1,    -1,   -1,   -1,   -1,   /* 0xC0 */
		-1,   -1,   -1,   0x1B,  -1,   0x1C, 0x1D, 0x1E,
		-1,   -1,   -1,   0x1F,  -1,   -1,   0x20, 0x21, /* 0xD0 */
		-1,   0x22, 0x23, 0x24,  0x25, 0x26, 0x27, 0x28,
		-1,   -1,   -1,   -1,    -1,   0x29, 0x2A, 0x2B, /* 0xE0 */
		-1,   0x2C, 0x2D, 0x2E,  0x2F, 0x30, 0x31, 0x32,
		-1,   -1,   0x33, 0x34,  0x35, 0x36, 0x37, 0x38, /* 0xF0 */
		-1,   0x39, 0x3A, 0x3B,  0x3C, 0x3D, 0x3E, 0x3F
	};

	unsigned char buf[12 * 2 * 524];
	unsigned char nibbuf[699];
	/*unsigned char csum[4];*/
	floppy *f;
	unsigned int len;
	unsigned int sector = 0;
	unsigned int oldpos, oldbitpos;
	int i, j, sum, side;
	int imgpos;
	unsigned char sector_found[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	UINT8 val;

	/* NPW 10-Nov-2000 - Removed ={} and added memset(); Win32 doesn't like what you just did */
	/* R Nabet : how about the new initializer ? My previous code was wrong, anyway. */
	/*memset(sector_found, '\0', sizeof(sector_found));*/

	f = &iwm_floppy[iwm_floppy_select];
	len = iwm_tracklen_800kb[f->loadedtrack_num];

	/* Check there is any data */
	if ((! f->loadedtrack_data) || (! f->loadedtrack_valid))
	{
		#if LOG_IWM
			logerror("iwm_put_track(): no track data!\n");
		#endif
		return 1;
	}

	oldpos = f->loadedtrack_pos;
	oldbitpos = f->loadedtrack_bitpos;
	f->loadedtrack_pos = 0;
	f->loadedtrack_bitpos = 0;

	side = (f->sides &&  f->loadedtrack_head) ? 0x20 : 0x00;
	if (f->loadedtrack_num & 0x40)
		side |= 0x01;

	/* do 2 rotations, in case the bit slip stuff prevent us to read the first sector */
	for (j = 0; j < (f->loadedtrack_len * 2); j++)
	{
		if (iwm_fetchtrack(f) != 0xD5)
			continue;
		j++;

		if (iwm_fetchtrack(f) != 0xAA)
			continue;
		j++;

		if (iwm_fetchtrack(f) != 0x96)
			continue;
		j++;

		if (rev_diskbytes[iwm_fetchtrack(f)] != (f->loadedtrack_num & 0x3f))
			continue;
		j++;

		sector = rev_diskbytes[iwm_fetchtrack(f)];
		if ((sector < 0) || (sector >= len))
			continue;
		j++;

		if (rev_diskbytes[iwm_fetchtrack(f)] != side)
			continue;
		j++;

		if (rev_diskbytes[iwm_fetchtrack(f)] != 0x22)
			continue;
		j++;

		sum = (f->loadedtrack_num ^ sector ^ side ^ 0x22) & 0x3F;

		if (rev_diskbytes[iwm_fetchtrack(f)] != sum)
			continue;
		j++;

		if (iwm_fetchtrack(f) != 0xDE)
			continue;
		j++;

		if (iwm_fetchtrack(f) != 0xAA)
			continue;
		j++;

#if 1
		/* This code should work, but it does not */
		while ((val = iwm_fetchtrack(f)) == 0xFF)
			j++;
#else
		val = -1;
		i = 0;
		while ((i < 20) && ((val = iwm_fetchtrack(f)) != 0xD5))
		{
			i++;
			j++;
		}
#endif

		if (val != 0xD5)
		{
			logerror("iwm_put_track(): lost bit slip mark!");
			continue;
		}
		j++;

		if (iwm_fetchtrack(f) != 0xAA)
			continue;
		j++;

		if (iwm_fetchtrack(f) != 0xAD)
			continue;
		j++;

		if (rev_diskbytes[iwm_fetchtrack(f)] != sector)
			continue;
		j++;

		for (i = 0; i < 699; i++)
		{
			nibbuf[i] = rev_diskbytes[iwm_fetchtrack(f)];
			j++;
		}
		iwm_denibblize35(&buf[sector * 524], nibbuf/*, csum*/);


		for (i = 3; i >= 0; i--)
		{
			iwm_fetchtrack(f);
			/*if (rev_diskbytes[iwm_fetchtrack(f)] != csum[i])
				continue;*/
			j++;
		}

		iwm_fetchtrack(f);
		/*if (iwm_fetchtrack(f) != 0xDE)
			continue;*/
		j++;

		iwm_fetchtrack(f);
		/*if (iwm_fetchtrack(f) != 0xAA)
			continue;*/
		j++;

		iwm_fetchtrack(f);
		/*if (iwm_fetchtrack(f) != 0xFF)
			continue;*/

		sector_found[sector] = TRUE;

		#if LOG_IWM_EXTRA
			logerror("iwm_put_track(): sector=%i csum[0..3]={0x%02x,0x%02x,0x%02x,0x%02x}\n",
				(int) sector, (int) csum[0], (int) csum[1], (int) csum[2], (int) csum[3]);
		#endif
	}

	f->loadedtrack_pos = oldpos % f->loadedtrack_len;
	f->loadedtrack_bitpos = oldbitpos;

	val = 0;	/* error flag */
	for (i=0; i<len; i++)
	{
		if (! sector_found[i])
		{
			logerror("iwm_put_track(): aaarrgh, sector %d was not found, track %d is corrupt!\n",
				(int) sector, (int) f->loadedtrack_num);
			val = 1;
		}
	}
	if (val)
	{
		logerror("iwm_put_track(): raw track data dump :\n");
		for (i=0; i<f->loadedtrack_len; i++)
		{
			logerror("%02X ", (int) f->loadedtrack_data[i]);
			if (! ((i+1) % 32))
				logerror("\n");	/* insert one newline every 32 byte */
		}
		logerror((i % 32) ? "\n\n" : "\n");
	}

	if ((f->image_format == apple_diskcopy) && (f->format_specific.apple_diskcopy.tag_offset))
	{	/* write tag data to disk image */
		unsigned char tagbuf[12 * 2 * 12];

		for (i = 0; i < len; i++)
		{
			memcpy(tagbuf + i * 12, buf + i * 524, 12);
		}

		imgpos = iwm_indextrack_800kb(f->loadedtrack_num);

		if (f->sides)
		{
			imgpos *= 2;
			if (f->loadedtrack_head)
				imgpos += len;
		}

		imgpos *= 12;

		if (osd_fseek(f->fd, f->format_specific.apple_diskcopy.tag_offset + imgpos, SEEK_SET))
		{
			#if LOG_IWM
				logerror("iwm_put_track(): osd_fseek() failed!\n");
			#endif
			return 1;
		}

#if 1
		if (osd_fwrite(f->fd, tagbuf, len * 12) != (len * 12))
		{
			#if LOG_IWM
				logerror("iwm_put_track(): osd_fwrite() failed!\n");
			#endif
			return 1;
		}
#else
		/* do a test : has data changed ? */
		{
			unsigned char tagbuf2[12 * 2 * 12];

			if (osd_fread(f->fd, tagbuf2, len * 12) != (len * 12))
			{
				#if LOG_IWM
					logerror("iwm_put_track(): osd_fread() failed!\n");
				#endif
				return 1;
			}
			if (memcmp(tagbuf, tagbuf2, len * 12))
			{
				logerror("iwm_put_track(): data has somehow changed!\n");
			}
		}
#endif
	}
	/* now crush tag data */
	for (i = 0; i < len; i++)
	{
		memmove(buf + i * 512, buf + i * 524 + 12, 512);
	}

	/* do write to disk */

	imgpos = iwm_indextrack_800kb(f->loadedtrack_num);

	if (f->sides)
	{
		imgpos *= 2;
		if (f->loadedtrack_head)
			imgpos += len;
	}

	imgpos *= 512;

#if LOG_IWM
	logerror("iwm_put_track(): Saving track %d (%d sectors, imgpos=0x%08x)\n",
					(int) f->loadedtrack_num, (int) len, (int) imgpos);
#endif

	if (osd_fseek(f->fd, (f->image_format == apple_diskcopy) ? imgpos + 84 : imgpos, SEEK_SET))
	{
		#if LOG_IWM
			logerror("iwm_put_track(): osd_fseek() failed!\n");
		#endif
		return 1;
	}

#if 1
	if (osd_fwrite(f->fd, buf, len * 512) != (len * 512))
	{
		#if LOG_IWM
			logerror("iwm_put_track(): osd_fwrite() failed!\n");
		#endif
		return 1;
	}
#else
	/* do a test : has data changed ? */
	{
		unsigned char buf2[12 * 2 * 524];

		if (osd_fread(f->fd, buf2, len * 512) != (len * 512))
		{
			#if LOG_IWM
				logerror("iwm_put_track(): osd_fread() failed!\n");
			#endif
			return 1;
		}
		if (memcmp(buf, buf2, len * 512))
		{
			logerror("iwm_put_track(): data has somehow changed!\n");
		}
	}
#endif

	f->loadedtrack_dirty = 0;

	return 0;
}

static int iwm_readdata(void)
{
	int result = 0;

	floppy *f;

	f = &iwm_floppy[iwm_floppy_select];

	/* Bad track? Unloaded? */
	if ((!f->fd) || (f->track < 0) || (f->track >= 80)) {
	error:
		result = 0xFF;
	}
	else
	{
		/* Do we need to load the track? */
		if (!f->loadedtrack_valid || (f->loadedtrack_num != f->track) || (f->loadedtrack_head != f->head))
		{
			if (f->loadedtrack_valid && f->loadedtrack_dirty)
			{
				if (iwm_put_track())
					goto error;	/* write current track first */
			}
			if (iwm_get_track())
				goto error;
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
		result = iwm_fetchtrack(f);
		/*result = f->loadedtrack_data[f->loadedtrack_pos++];
		f->loadedtrack_pos %= f->loadedtrack_len;*/
	}

	#if LOG_IWM_EXTRA
		logerror("iwm_readdata(): result=%d pc=0x%08x\n", result, (int) cpu_get_pc());
	#endif

	return result;
}

static void iwm_writedata(int data)
{
	/* Not yet implemented */

	floppy *f;

	f = &iwm_floppy[iwm_floppy_select];

	if (f->wp)
		return;	/* no need to bother */

	/* Bad track? Unloaded? */
	if ((!f->fd) || (f->track < 0) || (f->track >= 80))
	{
	error:
		;
	}
	else
	{
		/* Do we need to load the track? */
		if (!f->loadedtrack_valid || (f->loadedtrack_num != f->track) || (f->loadedtrack_head != f->head))
		{
			if (f->loadedtrack_valid && f->loadedtrack_dirty)
			{
				if (iwm_put_track())
					goto error;	/* write current track first */
			}
			if (iwm_get_track())
				goto error;
		}

		/*
		 * Right now, this function assumes latch mode; which is always used for
		 * 3.5 inch drives.  Eventually we should check to see if latch mode is
		 * off
		 */
		#if LOG_IWM
			if ((iwm_mode & IWM_MODE_LATCHMODE) == 0)
				logerror("iwm_writedata(): latch mode off not implemented\n");
		#endif

		/* Now actually write the data */
		iwm_filltrack(f, data);
		/*f->loadedtrack_data[f->loadedtrack_pos++] = data;
		f->loadedtrack_pos %= f->loadedtrack_len;*/

		f->loadedtrack_dirty = 1;
	}

	#if LOG_IWM
		logerror("iwm_writedata(): data=%X\n", data);
	#endif
}

static int iwm_readenable2handshake(void)
{
	static int val = 0;

	if (val++ > 3)
		val = 0;

	return val ? 0xc0 : 0x80;
}

static int iwm_rpm(floppy *f)
{
	/*
	 * The Mac floppy controller was interesting in that its speed was adjusted
	 * while the thing was running.  On the tracks closer to the rim, it was
	 * sped up so that more data could be placed on it.  Hense, this function
	 * has different results depending on the track number
	 *
	 * The Mac Plus (and probably the other Macs that use the IWM) verify that
	 * the speed of the floppy drive is within a certain range depending on
	 * what track the floppy is at.  These RPM values are just guesses and are
	 * probably not fully accurate, but they are within the range that the Mac
	 * Plus expects and thus are probably in the right ballpark.
	 *
	 * Note - the timing values are the values returned by the Mac Plus routine
	 * that calculates the speed; I'm not sure what units they are in
	 */
	static int speeds[] = {
		500,	/* 00-15:	timing value 117B (acceptable range {1135-11E9} */
		550,	/* 16-31:	timing value ???? (acceptable range {12C6-138A} */
		600,	/* 32-47:	timing value ???? (acceptable range {14A7-157F} */
		675,	/* 48-63:	timing value ???? (acceptable range {16F2-17E2} */
		750		/* 64-79:	timing value ???? (acceptable range {19D0-1ADE} */
	};

	return speeds[f->track / 16];
}


static int iwm_status(void)
{
	int result = 1;
	int action;
	floppy *f;

	action = ((iwm_lines & (IWM_PH1 | IWM_PH0)) << 2) | (iwm_sel_line << 1) | ((iwm_lines & IWM_PH2) >> 2);

	#if LOG_IWM_EXTRA
		logerror("iwm_status(): action=%d pc=0x%08x%s\n",
			action, (int) cpu_get_pc(), (iwm_lines & IWM_MOTOR) ? "" : " (MOTOR OFF)");
	#endif

	if (iwm_lines & IWM_MOTOR) {
		f = &iwm_floppy[iwm_floppy_select];

		switch(action) {
		case 0x00:	/* Step direction */
			result = f->step;
			break;
		case 0x01:	/* Lower head activate */
			f->head = 0;
			result = 0;
			break;
		case 0x02:	/* Disk in place */
			result = !f->fd;	/* 0=disk 1=nodisk */
			break;
		case 0x03:	/* Upper head activate */
			f->head = 1;
			result = 0;
			break;
		case 0x04:	/* Disk is stepping 0=stepping 1=not stepping*/
			result = 1;
			break;
		case 0x06:	/* Disk is locked 0=locked 1=unlocked */
			result = f->wp ? 0 : 1;
			break;
		case 0x08:	/* Motor on 0=on 1=off */
			result = f->motor ? 0 : 1;
			break;
		case 0x09:	/* Number of sides: 0=single sided, 1=double sided */
			result = f->sides;
			break;
		case 0x0a:	/* At track 0 */
			result = f->track != 0;	/* 0=track zero 1=not track zero */
			break;
		case 0x0b:	/* Disk ready: 0=ready, 1=not ready */
			result = f->fd ? 0 : 1;
			break;
		case 0x0c:	/* Disk switched */
			result = f->disk_switched;
			break;
		case 0x0d:	/* Unknown */
			/* I'm not sure what this one does, but the Mac Plus executes the
			 * following code that uses this status:
			 *
			 *	417E52: moveq   #$d, D0		; Status 0x0d
			 *	417E54: bsr     4185fe		; Query IWM status
			 *	417E58: bmi     417e82		; If result=1, then skip
			 *
			 * This code is called in the Sony driver's open method, and
			 * _AddDrive does not get called if this status 0x0d returns 1.
			 * Hense, we are returning 0
			 */
			result = 0;
			break;
		case 0x0e:	/* Tachometer */
			/* (time in seconds) / (60 sec/minute) * (rounds/minute) * (60 pulses) * (2 pulse phases) */
			result = ((int) (timer_get_time() / 60.0 * iwm_rpm(f) * 60.0 * 2.0)) & 1;
			break;
		case 0x0f:	/* Drive installed: 0=drive connected, 1=drive not connected */
			result = 0;
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
			/*if (f->fd) {
				osd_fclose(f->fd);
				memset(f, 0, sizeof(*f));
			}*/
			/* somewhat hackish, but better method (?) */
			device_filename_change(IO_FLOPPY, iwm_floppy_select, NULL);
			break;
		default:
			#if LOG_IWM
				logerror("iwm_doaction(): unknown action\n");
			#endif
			break;
		}
	}
}

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

int iwm_r(int offset)
{
	offset &= 15;

#if LOG_IWM_EXTRA
	logerror("iwm_r: offset=%i\n", offset);
#endif

	iwm_access(offset);
	return (offset & 1) ? 0 : iwm_read_reg();
}

void iwm_w(int offset, int data)
{
	offset &= 15;

#if LOG_IWM_EXTRA
	logerror("iwm_w: offset=%i data=0x%02x\n", offset, data);
#endif

	iwm_access(offset);
	if ( offset & 1 )
		iwm_write_reg(data);
}

void iwm_set_sel_line(int sel)
{
	iwm_sel_line = sel ? 1 : 0;

	#if LOG_IWM_EXTRA
		logerror("iwm_set_sel_line(): %s line IWM_SEL\n", iwm_sel_line ? "setting" : "clearing");
	#endif
}

int iwm_get_sel_line(void)
{
	return iwm_sel_line;
}

/*
	opens a disk image (called from the floppy device init routine)

	the allowablesizes tells which formats should be supported
	(single-sided and double-sided 3.5'' GCR)
*/
int iwm_floppy_init(int id, int allowablesizes)
{
	floppy *f;
	long image_len=0;

	f = &iwm_floppy[id];

	memset(f, 0, sizeof(*f));

	if (!device_filename(IO_FLOPPY,id))
		return INIT_OK;

	f->fd = image_fopen(IO_FLOPPY, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_RW);
	if (!f->fd)
	{
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
			if ((header.diskName[0] <= 63) && (osd_fsize(f->fd) == (header.dataSize + header.tagSize + 84))
					&& (header.private == 0x0100))
			{
				f->image_format = apple_diskcopy;
				image_len = header.dataSize;
				f->format_specific.apple_diskcopy.tag_offset = (header.tagSize == (header.dataSize / 512 * 12))
																	? header.dataSize + 84
																	: 0;
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
	//f->wp = 1;

	f->disk_switched = 1;

#if LOG_IWM
	logerror("macplus_floppy_init(): Loaded %s-sided floppy; id=%i name='%s' wp=%i\n",
		(f->sides ? "double" : "single"), (int) id, device_filename(IO_FLOPPY,id), (int) f->wp);
#endif

	return INIT_OK;

error:
	if (f->fd)
		osd_fclose(f->fd);
	return INIT_FAILED;
}

void iwm_floppy_exit(int id)
{
	floppy *f;
	f = &iwm_floppy[id];

	if (f->fd)
	{
		if (f->loadedtrack_valid && f->loadedtrack_dirty)
		{
			(void) iwm_put_track();	/* write current track */
		}

		if ((f->image_format == apple_diskcopy) && (! f->wp))
		{
			/* we just zero the checksum fields - for now */
			UINT32 dataChecksum = 0, tagChecksum = 0;

			osd_fseek(f->fd, 72, SEEK_SET);

#ifdef LSB_FIRST
			dataChecksum = ((dataChecksum << 24) & 0xff000000)
							| ((dataChecksum << 8) & 0x00ff0000)
							| ((dataChecksum >> 8) & 0x0000ff00)
							| ((dataChecksum >> 24) & 0x000000ff);
			tagChecksum = ((tagChecksum << 24) & 0xff000000)
							| ((tagChecksum << 8) & 0x00ff0000)
							| ((tagChecksum >> 8) & 0x0000ff00)
							| ((tagChecksum >> 24) & 0x000000ff);
#endif

			if (osd_fwrite(f->fd, & dataChecksum, 4) == 4)
			{
				osd_fwrite(f->fd, & tagChecksum, 4);
			}
		}

		osd_fclose(f->fd);
	}
	if (f->loadedtrack_data)
	{
		free(f->loadedtrack_data);
		f->loadedtrack_data = NULL;
	}
}

