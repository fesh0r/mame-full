/*
	Handlers for macintosh images.

	Currently, we only handle the MFS format.  This was the format I needed
	most urgently, because modern Macintoshes do not support it any more.
	In the future, I will implement HFS as well.

	Raphael Nabet, 2003
*/

/*
	terminology:
	disk block: 512-byte logical block
	allocation block: "The File Manager always allocates logical disk blocks to
		a file in groups called allocation blocks; an allocation block is
		simply a group of consecutive logical blocks.  The size of a volume's
		allocation blocks depends on the capacity of the volume; there can be
		at most 65,535 allocation blocks on a volume."


	Organization of an MFS volume:

	Logical		Contents									Allocation block
	block

	0 - 1:		System startup information
	2 - 3:		Master directory block (MDB)
				+ unidentified data
	4 - n:		Directory file
	n+1 - p-2:	Other files and free space					0 - (p-2)/k
	p-1:		Alternate MDB
	p:			Not used
	usually, n = 16, p = 799 (SSDD 3.5" floppy) or 1599 (DSDD 3.5" floppy),
	and k = 2
*/

#include <ctype.h>
#include <string.h>
#include "osdepend.h"
#include "imgtoolx.h"

#if 0
#pragma mark MISCELLANEOUS UTILITIES
#endif

typedef struct UINT16BE
{
	UINT8 bytes[2];
} UINT16BE;

typedef struct UINT24BE
{
	UINT8 bytes[3];
} UINT24BE;

typedef struct UINT32BE
{
	UINT8 bytes[4];
} UINT32BE;

INLINE UINT16 get_UINT16BE(UINT16BE word)
{
	return (word.bytes[0] << 8) | word.bytes[1];
}

INLINE void set_UINT16BE(UINT16BE *word, UINT16 data)
{
	word->bytes[0] = (data >> 8) & 0xff;
	word->bytes[1] = data & 0xff;
}

INLINE UINT32 get_UINT24BE(UINT24BE word)
{
	return (word.bytes[0] << 16) | (word.bytes[1] << 8) | word.bytes[2];
}

INLINE void set_UINT24BE(UINT24BE *word, UINT32 data)
{
	word->bytes[0] = (data >> 16) & 0xff;
	word->bytes[1] = (data >> 8) & 0xff;
	word->bytes[2] = data & 0xff;
}

INLINE UINT32 get_UINT32BE(UINT32BE word)
{
	return (word.bytes[0] << 24) | (word.bytes[1] << 16) | (word.bytes[2] << 8) | word.bytes[3];
}

INLINE void set_UINT32BE(UINT32BE *word, UINT32 data)
{
	word->bytes[0] = (data >> 24) & 0xff;
	word->bytes[1] = (data >> 16) & 0xff;
	word->bytes[2] = (data >> 8) & 0xff;
	word->bytes[3] = data & 0xff;
}

/*
	Macintosh string: first byte is lenght
*/
typedef UINT8 mac_str27[28];
typedef UINT8 mac_str63[64];
typedef UINT8 mac_str255[256];

/*
	Macintosh type/creator code: 4 char value
*/
typedef UINT32BE mac_type;

/*
	point record, with the y and x coordinates
*/
typedef struct mac_point
{
	UINT16BE v;		/* actually signed */
	UINT16BE h;		/* actually signed */
} mac_point;


/*
	FInfo (Finder Info) record
*/
typedef struct mac_FInfo
{
	mac_type  type;			/* file type */
	mac_type  creator;		/* file creator */
	UINT16BE  flags;		/* Finder flags */
	mac_point location;		/* file's location in window */
								/* If set to {0, 0}, and the initied flag is
								clear, the Finder will place the item
								automatically */
	UINT16BE  fldr;			/* MFS: window that contains file */
								/* -3: trash */
								/* -2: desktop */
								/* -1: new folder template?????? */
								/* 0: disk window ("root") */
								/* > 0: specific folder, index of FOBJ resource??? */
								/* FOBJ resource contains folder info. */
							/* HFS & HFS+: reserved (set to 0) */
} mac_FInfo;

/*
	defines for FInfo flags field
*/
enum
{
	fif_isOnDesk			= 0x0001,	/* System 6: set if item is located on desktop (files and folders) */
										/* System 7: Unused (set to 0) */
	fif_color				= 0x000E,	/* color coding (files and folders) */
	fif_colorReserved		= 0x0010,	/* System 6: reserved??? */
										/* System 7: Unused (set to 0) */
	fif_requiresSwitchLaunch= 0x0020,	/* System 6: ??? */
										/* System 7: Unused (set to 0) */


	fif_isShared			= 0x0040,	/* Applications files: if set, the */
											/* application can be executed by */
											/* multiple users simultaneously. */
										/* Otherwise, set to 0. */
	fif_hasNoINITs			= 0x0080,	/* Extensions/Control Panels: if set(?), */
											/* this file contains no INIT */
											/* resource */
										/* Otherwise, set to 0. */
	fif_hasBeenInited		= 0x0100,	/* System 6: The Finder has recorded information from
											the file’s bundle resource into the desktop
											database and given the file or folder a
											position on the desktop. */
										/* System 7? 8?: Clear if the file contains desktop database */
											/* resources ('BNDL', 'FREF', 'open', 'kind'...) */
											/* that have not been added yet. Set only by the Finder */
											/* Reserved for folders - make sure this bit is cleared for folders */

	/* bit 0x0200 was at a point (AOCE for system 7.x?) the letter bit for
	AOCE, but was reserved before and it is reserved again in recent MacOS
	releases. */

	fif_hasCustomIcon		= 0x0400,	/* (files and folders) */
	fif_isStationery		= 0x0800,	/* System 7: (files only) */
	fif_nameLocked			= 0x1000,	/* (files and folders) */
	fif_hasBundle			= 0x2000,	/* Files only */
	fif_isInvisible			= 0x4000,	/* (files and folders) */
	fif_isAlias				= 0x8000	/* System 7: (files only) */
};

/*
	mac_to_c_strncpy()

	Encode a macintosh string as a C string.  The NUL character is escaped,
	using the "%00" sequence.  To avoid any confusion, '%' is escaped with
	'%25'.

	dst (O): C string
	n (I): lenght of buffer pointed to by dst
	src (I): macintosh string (first byte is lenght)
*/
static void mac_to_c_strncpy(char *dst, int n, UINT8 *src)
{
	size_t len = src[0];
	int i, j;

	i = j = 0;
	while ((i < len) && (j < n-1))
	{
		switch (src[i+1])
		{
		case '\0':
			if (j >= n-3)
				goto exit;
			dst[j] = '%';
			dst[j+1] = '0';
			dst[j+2] = '0';
			j += 3;
			break;

		case '%':
			if (j >= n-3)
				goto exit;
			dst[j] = '%';
			dst[j+1] = '2';
			dst[j+2] = '5';
			j += 3;
			break;

		default:
			dst[j] = src[i+1];
			j++;
			break;
		}
		i++;
	}

exit:
	if (n > 0)
		dst[j] = '\0';
}

/*
	c_to_mac_strncpy()

	Decode a C string to a macintosh string.  The NUL character is escaped,
	using the "%00" sequence.  To avoid any confusion, '%' is escaped with
	'%25'.

	dst (O): macintosh string (first byte is lenght)
	src (I): C string
	n (I): lenght of string pointed to by src
*/
static void c_to_mac_strncpy(mac_str255 dst, const char *src, int n)
{
	size_t len;
	int i;
	char buf[3];


	len = 0;
	i = 0;
	while ((i < n) && (len < 255))
	{
		switch (src[i])
		{
		case '%':
			if (i >= n-2)
				goto exit;
			buf[0] = src[i+1];
			buf[1] = src[i+2];
			buf[2] = '\0';
			dst[len+1] = strtoul(buf, NULL, 16);
			i += 3;
			break;

		default:
			dst[len+1] = src[i];
			i++;
			break;
		}
		len++;
	}

exit:
	dst[0] = len;
}

/*
	mac_strcmp()

	Compare two macintosh strings

	s1 (I): the string to compare
	s2 (I): the comparison string

	Return a zero if s1 and s2 are equal, a negative value if s1 is less than
	s2, and a positive value if s1 is greater than s2.
*/
/*static int mac_strcmp(const UINT8 *s1, const UINT8 *s2)
{
	size_t common_len;

	common_len = (s1[0] <= s2[0]) ? s1[0] : s2[0];

	return memcmp(s1+1, s2+1, common_len) || ((int)s1[0] - s2[0]);
}*/

static int mac_stricmp(const UINT8 *s1, const UINT8 *s2)
{
	size_t common_len;
	int i;
	int c1, c2;

	common_len = (s1[0] <= s2[0]) ? s1[0] : s2[0];

	for (i=0; i<common_len; i++)
	{
		c1 = tolower(s1[i+1]);
		c2 = tolower(s2[i+1]);
		if (c1 != c2)
			return c1 - c2;
	}

	return ((int)s1[0] - s2[0]);
}

/*
	mac_strncpy()

	Copy a macintosh string

	dst (O): dest macintosh string
	n (I): max string lenght for dest (range 0-255, buffer lenght + 1)
	src (I): source macintosh string (first byte is lenght)

	Return a zero if s1 and s2 are equal, a negative value if s1 is less than
	s2, and a positive value if s1 is greater than s2.
*/
static void mac_strncpy(UINT8 *dest, int n, const UINT8 *src)
{
	size_t len;

	len = src[0];
	if (len > n)
		len = n;
	dest[0] = len;
	memcpy(dest+1, src+1, len);
}

/*
	check_fname()

	Check a file name.

	fname (I): file name (macintosh string)

	Return non-zero if the file name is invalid.

	TODO: how about script codes in HFS???
*/
static int check_fname(const UINT8 *fname)
{
	size_t len = fname[0];
	int i;

#if 0
	/* lenght >31 is incorrect with HFS, but not MFS */
	if (len > 31)
		return 1;
#endif

	/* check and copy file name */
	for (i=0; i<len; i++)
	{
		switch (fname[i+1])
		{
		case ':':
			/* illegal character */
			return 1;
		default:
			/* all other characters are legal (though control characters,
			particularly NUL characters, are not recommended) */
			break;
		}
	}

	return 0;
}

/*
	Check a file path.

	fpath (I): file path (C string)

	Return non-zero if the file path is invalid.

	TODO: how about foreign scripts in HFS???
*/
static int check_fpath(const char *fpath)
{
	const char *element_start, *element_end;
	int element_len;
	int i, mac_element_len;
	int level;


	/* check that each path element has an acceptable lenght */
	element_start = fpath;
	level = 0;
	do
	{
		/* find next path element */
		element_end = strchr(element_start, ':');
		element_len = element_end ? (element_end - element_start) : strlen(element_start);
		/* compute element len after decoding */
		mac_element_len = 0;
		i=0;
		while (i<element_len)
		{
			switch (element_start[i])
			{
			case '%':
				if (isxdigit(element_start[i+1]) && isxdigit(element_start[i+2]))
					i += 3;
				else
					return 1;
				break;

			default:
				i++;
				break;
			}
			mac_element_len++;
		}
		if (element_len > 255/*31*/)
			return 1;

		/* iterate */
		if (element_end)
			element_start = element_end+1;
		else
			element_start = NULL;

		if (element_len == 0)
		{	/* '::' refers to parent dir, ':::' to the parent's parent, etc */
			level--;
			if (level < 0)
				return 1;
		}
		else
			level++;
	}
	while (element_start);

	return 0;
}


/*
	MacBinary format - encode data and ressource forks into a single file

	AppleSingle is much more flexible than MacBinary IMHO, but MacBinary is
	supported by many more utilities.

	There are 3 versions of MacBinary: MacBinary ("v1"), MacBinaryII ("v2") and
	MacBinaryIII ("v3").  In addition, the comment field encoding was proposed
	as an extension to v1 well before v2 was introduced, so I call MacBinary
	with comment extension "v1.1".

	All three formats are backward- and forward-compatible with each other.
*/
/*
	Header

	Note that several fields are not aligned...
*/
typedef struct MBHeader /* MacBinary header */ 
{
	UINT8 version_old;		/* always 0 */
	mac_str63 fname;		/* file name (31 characters at most for HFS & */
								/* system 7 compatibility?) */
	UINT32BE ftype;			/* file type */
	UINT32BE fcreator;		/* file creator */
	UINT8 flags_MSB;		/* bits 15-8 of Finder flags */
	UINT8 zerofill0;		/* zero fill, must be zero for compatibility */
	mac_point location;		/* file's position within its window */
	UINT16BE fldr;			/* file's window or folder ID */
	UINT8 protect;			/* bit 0 -> protected flag */
	UINT8 zerofill1;		/* zero fill, must be zero for compatibility */
	UINT32BE data_len;		/* data Fork length (bytes, zero if no Data Fork) */
	UINT32BE rsrc_len;		/* resource Fork length (bytes, zero if no R. F.) */
	UINT32BE crDate;		/* file creation date */
	UINT32BE lsMod;			/* file "last modified" date */
	UINT16BE comment_len;	/* v1: unused - set to 0 */
							/* v1.1, v2 & v3: length of Get Info comment to be */
								/* sent after the resource fork (if */
								/* implemented, see below) */
	UINT8 flags_LSB;		/* v1: unused - set to 0 */
							/* v2 & v3: bits 7-0 of Finder flags */
	UINT32BE signature;		/* v1 & v2: unused - set to 0 */
							/* v3: signature for indentification purposes */
								/* ('mBIN') */
	UINT8 fname_script;		/* v1 & v2: unused - set to 0 */
							/* v3: script of file name (from the fdScript field */
								/* of fxInfo record) */
	UINT8 xflags;			/* v1 & v2: unused - set to 0 */
							/* v3: extended Finder flags (from the fdXFlags */
								/* field of fxInfo record) */
	UINT8 unused[8];		/* Unused (must be zeroed by creators, must be */
								/* ignored by readers) */
	UINT32BE reserved;		/* v1: unused - set to 0 */
							/* v2 & v3: this field is defined but has never */
								/* been used */
	UINT16BE sec_header_len;/* v1: unused - set to 0 */
							/* v2 & v3: Length of a secondary header. If this */
								/* is non-zero, skip this many bytes (rounded */
								/* up to the next multiple of 128). This is for */
								/* future expansion only, when sending files */
								/* with MacBinary, this word should be zero. */
	UINT8 version;			/* v1: unused - set to 0 */
							/* v2 & v3: Version number of MacBinary III that */
								/* the uploading program is written for (the */
								/*version is 130 for MacBinary III) */
	UINT8 compat_version;	/* v1: unused - set to 0 */
							/* v2 & v3: Minimum MacBinary version needed to */
								/* read this file (set this value at 129 for */
								/* backwards compatibility with MacBinary II) */
	UINT16BE crc;			/* v1: unused - set to 0 */
							/* v2 & v3: CRC of previous 124 bytes */
	UINT16BE reserved2;		/* v1: Reserved for computer type and OS ID (this */
								/* field will be zero for the current */
								/* Macintosh) */
							/* v2 & v3: undefined??? */
} MBHeader;

/*
	CalcMBCRC

	Compute CRC of macbinary header

	This piece of code was ripped from the MacBinary II+ source (and converted
	from 68k asm to C), as the specs for the CRC used by MacBinary are not
	published.

	header (I): pointer to MacBinary header (only 124 first bytes matter, i.e.
		CRC and reserved2 fields are ignored)

	Returns 16-bit CRC value.
*/
static UINT16 CalcMBCRC(const MBHeader *header)
{
	static UINT16 magic[256] =
	{
		0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
		0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
		0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
		0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
		0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
		0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
		0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
		0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
		0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
		0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
		0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
		0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
		0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
		0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
		0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
		0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
		0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
		0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
		0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
		0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
		0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
		0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
		0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
		0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
		0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
		0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
		0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
		0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
		0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
		0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
		0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
		0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
	};
	const UINT8 *ptr = (UINT8 *)header;
	int i;
	UINT16 crc;

	crc = 0;
	/* compute CRC on 124 bytes */
	for (i=0; i<124; i++)
		crc = magic[(crc >> 8) ^ ptr[i]] ^ (crc << 8);

	return crc;
}


#if 0
#pragma mark -
#pragma mark DISK IMAGE ROUTINES
#endif

/*
	header of diskcopy 4.2 images
*/
typedef struct diskcopy_header_t
{
	UINT8    diskName[64];	/* name of the disk */
	UINT32BE dataSize;		/* total size of data for all sectors (512*number_of_sectors) */
	UINT32BE tagSize;		/* total size of tag data for all sectors (12*number_of_sectors for GCR 3.5" floppies, 20*number_of_sectors for HD20, 0 otherwise) */
	UINT32BE dataChecksum;	/* CRC32 checksum of all sector data */
	UINT32BE tagChecksum;	/* CRC32 checksum of all tag data */
	UINT8    diskFormat;	/* 0 = 400K, 1 = 800K, 2 = 720K, 3 = 1440K  (other values reserved) */
	UINT8    formatByte;	/* should be $00 Apple II, $01 Lisa, $02 Mac MFS ??? */
							/* $12 = 400K, $22 = >400K Macintosh (DiskCopy uses this value for
							   all Apple II disks not 800K in size, and even for some of those),
							   $24 = 800K Apple II disk */
	UINT16BE private;		/* always $0100 (otherwise, the file may be in a different format. */
} diskcopy_header_t;

typedef struct mac_l1_imgref
{
	IMAGE base;
	STREAM *f;
	enum { bare, apple_diskcopy } format;
	union
	{
		/*struct
		{
		} bare;*/
		struct
		{
			long tag_offset;
		} apple_diskcopy;
	} u;
} mac_l1_imgref;

static int floppy_image_open(mac_l1_imgref *image)
{
	long image_len=0;
	diskcopy_header_t header;


	image->format = bare;	/* default */

	/* read image header */
	if (stream_read(image->f, & header, sizeof(header)) == sizeof(header))
	{
		UINT32 dataSize = get_UINT32BE(header.dataSize);
		UINT32 tagSize = get_UINT32BE(header.tagSize);

		/* various checks : */
		if ((header.diskName[0] <= 63)
				&& (stream_size(image->f) == (dataSize + tagSize + 84))
				&& (get_UINT16BE(header.private) == 0x0100))
		{
			image->format = apple_diskcopy;
			image_len = dataSize;
			image->u.apple_diskcopy.tag_offset = (tagSize == (dataSize / 512 * 12))
													? dataSize + 84
													: 0;
			/*image->formatByte = header.formatByte & 0x3F;*/
		}
	}

	if (image->format == bare)
	{
		image_len = stream_size(image->f);
	}

#if 0
	switch(image_len) {
	case 80*10*512*1:
		/* Single sided (400k) */
		if (image->format == bare)
			image->formatByte = 0x02;	/* single-sided Macintosh (?) */
		break;
	case 80*10*512*2:
		/* Double sided (800k) */
		if (image->format == bare)
			image->formatByte = 0x22;	/* double-sided Macintosh (?) */
		break;
	default:
		/* Bad floppy size */
		goto error;
		break;
	}
#endif

	return 0;
}


static int image_read_block(mac_l1_imgref *image, UINT64 block, void *dest)
{
	if (stream_seek(image->f, (image->format == apple_diskcopy) ? block*512 + 84 : block*512, SEEK_SET))
		return IMGTOOLERR_READERROR;

	if (stream_read(image->f, dest, 512) != 512)
		return IMGTOOLERR_READERROR;

#if 0
	/* now append tag data */
	if ((image->format == apple_diskcopy) && (image->u.apple_diskcopy.tag_offset))
	{	/* insert tag data from disk image */
		if (stream_seek(image->f, image->u.apple_diskcopy.tag_offset + block*12, SEEK_SET))
			return IMGTOOLERR_READERROR;

		if (stream_read(image->f, (UINT8 *) dest + 512, 12) != 12)
			return IMGTOOLERR_READERROR;
	}
	else
	{
		/* no tag data stored in disk image - so we zero the tag data */
		for (i = 0; i < len; i++)
		{
			memset((UINT8 *) dest + 512, 0, 12);
		}
	}
#endif

	return 0;
}

static int image_write_block(mac_l1_imgref *image, UINT64 block, const void *src)
{
	if (stream_seek(image->f, (image->format == apple_diskcopy) ? block*512 + 84 : block*512, SEEK_SET))
		return IMGTOOLERR_READERROR;

	if (stream_write(image->f, src, 512) != 512)
		return IMGTOOLERR_READERROR;

#if 0
	/* now append tag data */
	if ((image->format == apple_diskcopy) && (image->u.apple_diskcopy.tag_offset))
	{	/* insert tag data from disk image */
		if (stream_seek(image->f, image->u.apple_diskcopy.tag_offset + block*12, SEEK_SET))
			return IMGTOOLERR_READERROR;

		if (stream_read(image->f, (UINT8 *) src + 512, 12) != 12)
			return IMGTOOLERR_READERROR;
	}
	else
	{
		/* no tag data stored in disk image - so we zero the tag data */
		for (i = 0; i < len; i++)
		{
			memset((UINT8 *) src + 512, 0, 12);
		}
	}
#endif

	return 0;
}

#if 0
#pragma mark -
#pragma mark MFS IMPLEMENTATION
#endif

/*
	Master Directory Block
*/
typedef struct mfs_mdb
{
	UINT8    sigWord[2];	/* volume signature - always $D2D7 */
	UINT32BE crDate;		/* date and time of volume creation */
	UINT32BE lsMod/*lsBkUp???*/;/* date and time of last modification (backup???) */
	UINT16BE atrb;			/* volume attributes (0x0000) */
								/* bit 15 is set if volume is locked by software */
	UINT16BE nmFls;			/* number of files in directory */
	UINT16BE dirSt;			/* first block of directory */

	UINT16BE blLn;			/* lenght of directory in blocks (0x000C) */
	UINT16BE nmBlks;		/* number of allocation blocks (0x0187) */
	UINT32BE alBlkSiz;		/* size of allocation blocks (0x00000400) */
	UINT32BE clpSiz;		/* default clump size - number of bytes to allocate (0x00002000) */
	UINT16BE alBlSt;		/* first allocation block in block map (0x0010) */

	UINT32BE nxtFNum;		/* next unused file number */
	UINT16BE freeBks;		/* number of unused allocation blocks */

	mac_str27 VN;			/* volume name */

	UINT8    unknown[512-64];	/* ??? */

	/* the rest of this block and the next one seems to include a list of
	sorts; or maybe it is 68k code.  Anyway, it is not included in the
	alternate VCB. */
} mfs_mdb;

/*
	directory entry for use in the directory file

	Note the structure is variable lenght.  It is always word-aligned, and
	cannot cross block boundaries.
*/
typedef struct mfs_dir_entry
{
	UINT8    flags;				/* bit 7=1 if entry used, bit 0=1 if file locked */
								/* 0x00 means end of block: if we are not done
								with reading the directory, the remnants will
								be read from next block */
	UINT8    flVersNum;			/* version number (usually 0x00, but I don't have the IM volume that describes it) */

	mac_FInfo flFinderInfo;		/* information used byt the Finder */

	UINT32BE flNum;				/* file number */

	UINT16BE dataStartBlock;	/* first allocation block of data fork */
	UINT32BE dataLogicalSize;	/* logical EOF of data fork */
	UINT32BE dataPhysicalSize;	/* physical EOF of data fork */
	UINT16BE rsrcStartBlock;	/* first allocation block of ressource fork */
	UINT32BE rsrcLogicalSize;	/* logical EOF of resource fork */
	UINT32BE rsrcPhysicalSize;	/* physical EOF of resource fork */

	UINT32BE createDate;		/* date and time of creation */
	UINT32BE modifyDate;		/* date and time of last modification */

	UINT8    name[1];			/* first char is lenght of file name */
								/* next chars are file name - 255 chars at most */
								/* IIRC, Finder 7 only supports 31 chars,
								wheareas earlier versions support 63 chars */
} mfs_dir_entry;

/*
	FOBJ desktop resource: describes a folder, or the location of the volume
	icon.
*/
typedef struct mfs_FOBJ
{
	UINT8 unknown0[2];		/* $0004 for disk, $0008 for folder??? */
	mac_point location;		/* location in parent window??? */
	UINT8 unknown1[6];		/* ??? */
	UINT16BE par_fldr;		/* parent folder ID */
	UINT8 unknown2[12];		/* ??? */
	UINT32BE createDate;	/* date and time of creation??? */
	UINT32BE modifyDate;	/* date and time of last modification??? */

	/* among the next fields, there is number of items (folders and files) in
	this folder (2 bytes), followed by item list: 4 bytes per item (folder or
	file).  However, the offset is not constant */
} mfs_FOBJ;

/*
	MFS image ref
*/
typedef struct mfs_l2_imgref
{
	mac_l1_imgref l1_img;

	UINT16 dir_num_files;
	UINT16 dir_start;
	UINT16 dir_blk_len;

	UINT16 blocksperAB;
	UINT16 ABStart;

	UINT16 num_free_blocks;

	mac_str27 volname;
} mfs_l2_imgref;

/*
	MFS open dir ref
*/
typedef struct mfs_dirref
{
	mfs_l2_imgref *l2_img;			/* image pointer */
	UINT16 index;					/* current file index in the disk directory */
	UINT16 cur_block;				/* current block offset in directory file */
	UINT16 cur_offset;				/* current byte offset in current block of directory file */
	UINT8 block_buffer[512];		/* buffer with current directory block */
} mfs_dirref;

/*
	MFS open file ref
*/
typedef struct mfs_fileref
{
	mfs_l2_imgref *l2_img;			/* image pointer */

	UINT16 stBlk;					/* first allocation block of file */
	UINT32 eof;						/* logical end-of-file */
	UINT32 pLen;					/* physical end-of-file */

	UINT32 crPs;					/* current position in file */

	UINT8 reload_buf;
	UINT8 block_buffer[512];		/* buffer with current file block */
} mfs_fileref;

/*
	mfs_image_open

	Open a MFS image.  Image must already be open on level 1.
*/
static int mfs_image_open(mfs_l2_imgref *l2_img)
{
	mfs_mdb mdb;
	int errorcode;

	errorcode = image_read_block(&l2_img->l1_img, 2, &mdb);
	if (errorcode)
		return errorcode;

	if ((mdb.sigWord[0] != 0xd2) || (mdb.sigWord[1] != 0xd7) || (mdb.VN[0] > 27))
		return IMGTOOLERR_CORRUPTIMAGE;

	l2_img->dir_num_files = get_UINT16BE(mdb.nmFls);
	l2_img->dir_start = get_UINT16BE(mdb.dirSt);
	l2_img->dir_blk_len = get_UINT16BE(mdb.blLn);

	/*UINT16BE nmBlks;*/		/* number of allocation blocks (0x0187) */

	if (get_UINT32BE(mdb.alBlkSiz) % 512)
		return IMGTOOLERR_CORRUPTIMAGE;

	l2_img->blocksperAB = get_UINT32BE(mdb.alBlkSiz) / 512;
	l2_img->ABStart = get_UINT16BE(mdb.alBlSt);

	l2_img->num_free_blocks = get_UINT16BE(mdb.freeBks);

	memcpy(l2_img->volname, mdb.VN, mdb.VN[0]+1);


	return 0;
}

/*
	mfs_dir_open

	Open the directory file
*/
static int mfs_dir_open(mfs_l2_imgref *l2_img, mfs_dirref *dirref)
{
	dirref->l2_img = l2_img;
	dirref->index = 0;

	dirref->cur_block = 0;
	dirref->cur_offset = 0;
	image_read_block(&l2_img->l1_img, l2_img->dir_start + dirref->cur_block, dirref->block_buffer);

	return 0;
}

/*
	mfs_dir_read_next

	Read one entry of directory file

	dirref (I/O): open directory file
	dir_entry (O): set to point to the entry read: set to NULL if EOF or error
*/
static int mfs_dir_read_next(mfs_dirref *dirref, mfs_dir_entry **dir_entry)
{
	mfs_dir_entry *cur_dir_entry;
	size_t cur_dir_entry_len;

	if (dir_entry)
		*dir_entry = NULL;

	if (dirref->index == dirref->l2_img->dir_num_files)
		/* EOF */
		return 0;
	
	/* get cat entry pointer */
	cur_dir_entry = (mfs_dir_entry *) (dirref->block_buffer + dirref->cur_offset);
	while ((dirref->cur_block == 512) || ! (cur_dir_entry->flags & 0x80))
	{
		dirref->cur_block++;
		dirref->cur_offset = 0;
		if (dirref->cur_block > dirref->l2_img->dir_blk_len)
			/* aargh! */
			return IMGTOOLERR_CORRUPTIMAGE;
		image_read_block(&dirref->l2_img->l1_img, dirref->l2_img->dir_start + dirref->cur_block, dirref->block_buffer);
		cur_dir_entry = (mfs_dir_entry *) (dirref->block_buffer + dirref->cur_offset);
	}

	cur_dir_entry_len = offsetof(mfs_dir_entry, name) + cur_dir_entry->name[0] + 1;

	if ((dirref->cur_offset + cur_dir_entry_len) > 512)
		/* aargh! */
		return IMGTOOLERR_CORRUPTIMAGE;

	/* entry looks valid: set pointer to entry */
	if (dir_entry)
		*dir_entry = cur_dir_entry;

	/* update offset in block */
	dirref->cur_offset += cur_dir_entry_len;
	/* align to word boundary */
	dirref->cur_offset = (dirref->cur_offset + 1) & ~1;

	/* update file count */
	dirref->index++;

	return 0;
}

/*
	mfs_find_dir_entry

	Find a file in an MFS directory
*/
static int mfs_find_dir_entry(mfs_dirref *dirref, const mac_str255 fname, mfs_dir_entry **dir_entry)
{
	mfs_dir_entry *cur_dir_entry;
	int errorcode;

	if (dir_entry)
		*dir_entry = NULL;

	/*  scan dir for file */
	while (1)
	{
		errorcode = mfs_dir_read_next(dirref, &cur_dir_entry);
		if (errorcode)
			return errorcode;
		if (!cur_dir_entry)
			/* EOF */
			break;
		if ((! mac_stricmp(fname, cur_dir_entry->name)) && (cur_dir_entry->flVersNum == 0))
		{	/* file found */

			if (dir_entry)
				*dir_entry = cur_dir_entry;

			return 0;
		}
	}

	return IMGTOOLERR_FILENOTFOUND;
}

/*
	mfs_file_open_DF

	Open a file data fork
*/
static int mfs_file_open_DF(mfs_l2_imgref *l2_img, mfs_dir_entry *dir_entry, mfs_fileref *fileref)
{
	fileref->l2_img = l2_img;

	fileref->stBlk = get_UINT16BE(dir_entry->dataStartBlock);
	fileref->eof = get_UINT32BE(dir_entry->dataLogicalSize);
	fileref->pLen = get_UINT32BE(dir_entry->dataPhysicalSize);

	fileref->crPs = 0;

	return 0;
}

/*
	mfs_file_open_RF

	Open a file resource fork
*/
static int mfs_file_open_RF(mfs_l2_imgref *l2_img, mfs_dir_entry *dir_entry, mfs_fileref *fileref)
{
	fileref->l2_img = l2_img;

	fileref->stBlk = get_UINT16BE(dir_entry->rsrcStartBlock);
	fileref->eof = get_UINT32BE(dir_entry->rsrcLogicalSize);
	fileref->pLen = get_UINT32BE(dir_entry->rsrcPhysicalSize);

	fileref->crPs = 0;

	fileref->reload_buf = TRUE;

	return 0;
}

/*
	mfs_file_read
*/
static int mfs_file_read(mfs_fileref *fileref, UINT32 len, void *dest)
{
	UINT32 block;
	int errorcode;
	int run_len;

	if ((fileref->crPs + len) > fileref->eof)
		/* EOF */
		return IMGTOOLERR_UNEXPECTED;

	while (len > 0)
	{
		if (fileref->reload_buf)
		{
			block = fileref->l2_img->ABStart + ((fileref->stBlk-2)*fileref->l2_img->blocksperAB)
						+ fileref->crPs / 512;
			errorcode = image_read_block(&fileref->l2_img->l1_img, block, fileref->block_buffer);
			if (errorcode)
				return errorcode;
			fileref->reload_buf = FALSE;
		}
		run_len = 512 - (fileref->crPs % 512);
		if (run_len > len)
			run_len = len;

		memcpy(dest, fileref->block_buffer+(fileref->crPs % 512), run_len);
		len -= run_len;
		dest = (UINT8 *)dest + run_len;
		fileref->crPs += run_len;
		if ((fileref->crPs % 512) == 0)
			fileref->reload_buf = TRUE;
	}

	return 0;
}

/*
	mfs_file_tell
*/
static int mfs_file_tell(mfs_fileref *fileref, UINT32 *filePos)
{
	*filePos = fileref->crPs;

	return 0;
}

/*
	mfs_file_seek
*/
static int mfs_file_seek(mfs_fileref *fileref, UINT32 filePos)
{
	if ((fileref->crPs / 512) != (filePos / 512))
		fileref->reload_buf = TRUE;

	fileref->crPs = filePos;

	return 0;
}

/*
	mfs_hashString

	Hash string to het the resource ID of comment (FCMT ressource type).

	Ripped from Apple TB06 (converted from 68k ASM to C)
*/
static int mfs_hashString(mac_str255 string)
{
	int reply;
	int len;
	int i;

	len = string[0];

	reply = 0;
	for (i=0; i<len; i++)
	{
		reply ^= string[i+1];
		if (reply & 1)
			reply = ((reply >> 1) & 0x7fff) | ~0x7fff;
		else
			reply = ((reply >> 1) & 0x7fff);
		if (! (reply & 0x8000))
			reply = - reply;
	}

	return reply;
}

#if 0
#pragma mark -
#pragma mark RESOURCE IMPLEMENTATION
#endif

/*
	Resource manager

	The resource manager stores arbitrary chunks of data (resource) identified
	by a type/id pair.  The resource type is a 4-char code, which generally
	implies the format of the data (e.g. 'PICT' is for a quickdraw picture,
	'STR ' for a macintosh string, 'CODE' for 68k binary code, etc).  The
	resource id is a signed 16-bit number that uniquely identifies each
	resource of a given type.  Note that, with most ressource types, resources
	with id < 128 are system resources that are available to all applications,
	whereas resources with id >= 128 are application resources visible only to
	the application that define them.

	Each resource can optionally have a resource name, which is a macintosh
	string of 255 chars at most.

	Limits:
	16MBytes of data
	64kbytes of type+reference lists
	64kbytes of resource names

	The Macintosh toolbox can open several resource files simulteanously to
	overcome these restrictions.

	Resources are used virtually everywhere in the Macintosh Toolbox, so it is
	no surprise that file comments and MFS folders are stored in resource files.
*/

/*
	Resource header
*/
typedef struct rsrc_header
{
	UINT32BE data_offs;		/* Offset from beginning of resource fork to resource data */
	UINT32BE map_offs;		/* Offset from beginning of resource fork to resource map */
	UINT32BE data_len;		/* Length of resource data */
	UINT32BE map_len;		/* Length of resource map */
} rsrc_header;

/*
	Resource data: each data entry is preceded by its len (UINT32BE)
	Offset to specific data fields are gotten from the resource map
*/

/*
	Resource map:
*/
typedef struct rsrc_map_header
{
	rsrc_header reserved0;	/* Reserved for copy of resource header */
	UINT32BE reserved1;		/* Reserved for handle to next resource map */
	UINT16BE reserved2;		/* Reserved for file reference number */

	UINT16BE attr;			/* Resource fork attributes */
	UINT16BE typelist_offs;	/* Offset from beginning of map to resource type list */
	UINT16BE namelist_offs;	/* Offset from beginning of map to resource name list */
	UINT16BE type_count;	/* Number of types in the map minus 1 */
} rsrc_map_header;

/*
	Resource type list entry
*/
typedef struct rsrc_type_entry
{
	UINT32BE type;			/* Resource type */
	UINT16BE ref_count;		/* Number of resources of this type in map minus 1 */
	UINT16BE ref_offs;		/* Offset from beginning of resource type list to reference list for this type */
} rsrc_type_entry;

/*
	Resource reference list entry
*/
typedef struct rsrc_ref_entry
{
	UINT16BE id;			/* Resource ID */
	UINT16BE name_offs;		/* Offset from beginning of resource name list to resource name */
							/* (-1 if none) */
	UINT8 attr;				/* Resource attributes */
	UINT24BE data_offs;		/* Offset from beginning of resource data to data for this resource */
	UINT32BE reserved;		/* Reserved for handle to resource */
} rsrc_ref_entry;

/*
	Resource name list entry: this is just a standard macintosh string
*/

typedef struct mac_resfileref
{
	mfs_fileref fileref;	/* open resource fork ref (you may open resources
								files in data fork, too, if you ever need to,
								but Classic MacOS never does such a thing
								(MacOS X often does so, though) */
	UINT32 data_offs;		/* Offset from beginning of resource file to resource data */
	UINT32 map_offs;		/* Offset from beginning of resource file to resource data */

	UINT16 typelist_offs;	/* Offset from beginning of map to resource type list */
	UINT16 namelist_offs;	/* Offset from beginning of map to resource name list */
	UINT16 type_count;		/* Number of types in the map minus 1 */
							/* This is actually part of the type list, which matters for offsets */
} mac_resfileref;


/*
	resfile_open
*/
static int resfile_open(mac_resfileref *resfileref)
{
	int errorcode;
	rsrc_header header;
	rsrc_map_header map_header;

	/* seek to resource header */
	errorcode = mfs_file_seek(&resfileref->fileref, 0);
	if (errorcode)
		return errorcode;

	errorcode = mfs_file_read(&resfileref->fileref, sizeof(header), &header);
	if (errorcode)
		return errorcode;

	resfileref->data_offs = get_UINT32BE(header.data_offs);
	resfileref->map_offs = get_UINT32BE(header.map_offs);

	/* seek to resource map header */
	errorcode = mfs_file_seek(&resfileref->fileref, resfileref->map_offs);
	if (errorcode)
		return errorcode;

	errorcode = mfs_file_read(&resfileref->fileref, sizeof(map_header), &map_header);
	if (errorcode)
		return errorcode;

	resfileref->typelist_offs = get_UINT16BE(map_header.typelist_offs);
	resfileref->namelist_offs = get_UINT16BE(map_header.namelist_offs);
	resfileref->type_count = get_UINT16BE(map_header.type_count);

	return 0;
}

/*
	resfile_get_entry
*/
static int resfile_get_entry(mac_resfileref *resfileref, UINT32 type, UINT16 id, rsrc_ref_entry *entry)
{
	int errorcode;
	rsrc_type_entry type_entry;
	UINT16 ref_count;
	int i;

	/* seek to resource type list in resource map */
	errorcode = mfs_file_seek(&resfileref->fileref, resfileref->map_offs+resfileref->typelist_offs+2);
	if (errorcode)
		return errorcode;

	if (resfileref->type_count == 0xffff)
		/* type list is empty */
		return IMGTOOLERR_FILENOTFOUND;

	for (i=0; i<=resfileref->type_count; i++)
	{
		errorcode = mfs_file_read(&resfileref->fileref, sizeof(type_entry), &type_entry);
		if (errorcode)
			return errorcode;
		if (type == get_UINT32BE(type_entry.type))
			break;
	}
	if (i > resfileref->type_count)
		/* type not found in list */
		return IMGTOOLERR_FILENOTFOUND;

	ref_count = get_UINT16BE(type_entry.ref_count);

	/* seek to resource ref list for this type in resource map */
	errorcode = mfs_file_seek(&resfileref->fileref, resfileref->map_offs+resfileref->typelist_offs+get_UINT16BE(type_entry.ref_offs));
	if (errorcode)
		return errorcode;

	if (ref_count == 0xffff)
		/* ref list is empty */
		return IMGTOOLERR_FILENOTFOUND;

	for (i=0; i<=ref_count; i++)
	{
		errorcode = mfs_file_read(&resfileref->fileref, sizeof(*entry), entry);
		if (errorcode)
			return errorcode;
		if (id == get_UINT16BE(entry->id))
			break;
	}
	if (i > ref_count)
		/* id not found in list */
		return IMGTOOLERR_FILENOTFOUND;

	/* type+id have been found... */
	return 0;
}

/*
	resfile_get_resname
*/
static int resfile_get_resname(mac_resfileref *resfileref, const rsrc_ref_entry *entry, mac_str255 string)
{
	int errorcode;
	UINT16 name_offs;
	UINT8 len;

	name_offs = get_UINT16BE(entry->name_offs);

	if (name_offs == 0xffff)
		/* ref list is empty */
		return IMGTOOLERR_UNEXPECTED;

	/* seek to resource name in name list in resource map */
	errorcode = mfs_file_seek(&resfileref->fileref, resfileref->map_offs+name_offs);
	if (errorcode)
		return errorcode;

	/* get string lenght */
	errorcode = mfs_file_read(&resfileref->fileref, 1, &len);
	if (errorcode)
		return errorcode;

	string[0] = len;

	/* get string data */
	errorcode = mfs_file_read(&resfileref->fileref, len, string+1);
	if (errorcode)
		return errorcode;

	return 0;
}

/*
	resfile_get_reslen
*/
static int resfile_get_reslen(mac_resfileref *resfileref, const rsrc_ref_entry *entry, UINT32 *len)
{
	int errorcode;
	UINT32 data_offs;
	UINT32BE llen;

	data_offs = get_UINT24BE(entry->data_offs);

	/* seek to resource data in resource data section */
	errorcode = mfs_file_seek(&resfileref->fileref, resfileref->data_offs+data_offs);
	if (errorcode)
		return errorcode;

	/* get data lenght */
	errorcode = mfs_file_read(&resfileref->fileref, sizeof(llen), &llen);
	if (errorcode)
		return errorcode;

	*len = get_UINT32BE(llen);

	return 0;
}

/*
	resfile_get_resdata
*/
static int resfile_get_resdata(mac_resfileref *resfileref, const rsrc_ref_entry *entry, UINT32 offset, UINT32 len, void *dest)
{
	int errorcode;
	UINT32 data_offs;
	UINT32BE llen;

	data_offs = get_UINT24BE(entry->data_offs);

	/* seek to resource data in resource data section */
	errorcode = mfs_file_seek(&resfileref->fileref, resfileref->data_offs+data_offs);
	if (errorcode)
		return errorcode;

	/* get data lenght */
	errorcode = mfs_file_read(&resfileref->fileref, sizeof(llen), &llen);
	if (errorcode)
		return errorcode;

	/* check we are within tolerances */
	if ((offset + len) > get_UINT32BE(llen))
		return IMGTOOLERR_UNEXPECTED;

	if (offset)
	{	/* seek to resource data offset in resource data section */
		errorcode = mfs_file_seek(&resfileref->fileref, resfileref->data_offs+data_offs+4+offset);
		if (errorcode)
			return errorcode;
	}

	/* get data */
	errorcode = mfs_file_read(&resfileref->fileref, len, dest);
	if (errorcode)
		return errorcode;

	return 0;
}

#if 0
#pragma mark -
#pragma mark DESKTOP FILE IMPLEMENTATION
#endif

static int get_comment(mfs_l2_imgref *l2_img, UINT16 id, mac_str255 comment)
{
	UINT8 desktop_fname[] = {'\7','D','e','s','k','t','o','p'};
	#define restype_FCMT (('F' << 24) | ('C' << 16) | ('M' << 8) | 'T')
	mfs_dirref dirref;
	mfs_dir_entry *dir_entry;
	mac_resfileref resfileref;
	rsrc_ref_entry resentry;
	UINT32 reslen;
	int errorcode;

	/* open dir */
	mfs_dir_open(l2_img, &dirref);

	/* find file */
	errorcode = mfs_find_dir_entry(&dirref, desktop_fname, &dir_entry);
	if (errorcode)
		return errorcode;

	/* open file RF */
	errorcode = mfs_file_open_RF(l2_img, dir_entry, &resfileref.fileref);
	if (errorcode)
		return errorcode;

	/* open resource structure */
	errorcode = resfile_open(&resfileref);
	if (errorcode)
		return errorcode;

	/* look for resource FCMT #id */
	errorcode = resfile_get_entry(&resfileref, restype_FCMT, id, &resentry);
	if (errorcode)
		return errorcode;

	/* extract comment len */
	errorcode = resfile_get_reslen(&resfileref, &resentry, &reslen);
	if (errorcode)
		return errorcode;

	/* check comment len */
	if (reslen > 256)
		/* hurk */
		/*return IMGTOOLERR_CORRUPTIMAGE;*/
		/* people willing to extend the MFM comment field (you know, the kind
		of masochists that try to support 20-year-old OSes) might append extra
		fields, so we just truncate the resource */
		reslen = 256;
		
	/* extract comment data */
	errorcode = resfile_get_resdata(&resfileref, &resentry, 0, reslen, comment);
	if (errorcode)
		return errorcode;

	/* phew, we are done! */
	return 0;
}

#if 0
#pragma mark -
#pragma mark IMGTOOL MODULE IMPLEMENTATION
#endif

/*
	MFS catalog iterator, used when imgtool reads the catalog
*/
typedef struct mfs_iterator
{
	IMAGEENUM base;
	mfs_dirref dirref;				/* open directory reference */
} mfs_iterator;



static int mfs_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
static void mfs_image_exit(IMAGE *img);
static void mfs_image_info(IMAGE *img, char *string, const int len);
static int mfs_image_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int mfs_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void mfs_image_closeenum(IMAGEENUM *enumeration);
static size_t mfs_image_freespace(IMAGE *img);
static int mfs_image_readfile(IMAGE *img, const char *fname, STREAM *destf);
static int mfs_image_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const ResolvedOption *options_);
static int mfs_image_deletefile(IMAGE *img, const char *fname);
static int mfs_image_create(const struct ImageModule *mod, STREAM *f, const ResolvedOption *options_);

static int mfs_read_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, void *buffer, int length);
static int mfs_write_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, const void *buffer, int length);

#if 0
static struct OptionTemplate mfs_createopts[] =
{
	/*{ "label",	"Volume name", IMGOPTION_FLAG_TYPE_STRING | IMGOPTION_FLAG_HASDEFAULT,	0,	0,	NULL},*/
	{ NULL, NULL, 0, 0, 0, 0 }
};
#endif

/*enum
{
	mfs_createopts_volname = 0
};*/

IMAGEMODULE(
	mfs,
	"MFS macintosh image",			/* human readable name */
	"img",							/* file extension */
	NULL,							/* crcfile */
	NULL,							/* crc system name */
	EOLN_CR,						/* eoln */
	0,								/* flags */
	mfs_image_init,					/* init function */
	mfs_image_exit,					/* exit function */
	mfs_image_info,					/* info function */
	mfs_image_beginenum,			/* begin enumeration */
	mfs_image_nextenum,				/* enumerate next */
	mfs_image_closeenum,			/* close enumeration */
	mfs_image_freespace,			/* free space on image */
	mfs_image_readfile,				/* read file */
	/*mfs_image_writefile*/NULL,	/* write file */
	/*mfs_image_deletefile*/NULL,	/* delete file */
	/*mfs_image_create*/NULL,		/* create image */
	/*mfs_read_sector*/NULL,
	/*mfs_write_sector*/NULL,
	NULL,							/* file options */
	/*mfs_createopts*/NULL			/* create options */
)

/*
	Open a file as a ti99_image (common code).
*/
static int mfs_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg)
{
	mfs_l2_imgref *image;
	int errorcode;


	image = malloc(sizeof(mfs_l2_imgref));
	* (mfs_l2_imgref **) outimg = image;
	if (image == NULL)
		return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(mfs_l2_imgref));
	image->l1_img.base.module = mod;
	image->l1_img.f = f;

	errorcode = floppy_image_open(&image->l1_img);
	if (errorcode)
		return errorcode;

	return mfs_image_open(image);
}

/*
	close a ti99_image
*/
static void mfs_image_exit(IMAGE *img)
{
	mfs_l2_imgref *image = (mfs_l2_imgref *) img;

	free(image);
}

/*
	get basic information on a ti99_image

	Currently returns the volume name
*/
static void mfs_image_info(IMAGE *img, char *string, const int len)
{
	mfs_l2_imgref *image = (mfs_l2_imgref *) img;

	mac_to_c_strncpy(string, len, image->volname);
}

/*
	Open the disk catalog for enumeration 
*/
static int mfs_image_beginenum(IMAGE *img, IMAGEENUM **outenum)
{
	mfs_l2_imgref *image = (mfs_l2_imgref *) img;
	mfs_iterator *iter;
	int errorcode;


	iter = malloc(sizeof(mfs_iterator));
	*((mfs_iterator **) outenum) = iter;
	if (iter == NULL)
		return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = img->module;
	errorcode = mfs_dir_open(image, &iter->dirref);
	if (errorcode)
	{
		free(iter);
		*((mfs_iterator **) outenum) = NULL;
		return errorcode;
	}

	return 0;
}

/*
	Enumerate disk catalog next entry
*/
static int mfs_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
{
	mfs_iterator *iter = (mfs_iterator *) enumeration;
	mfs_dir_entry *cur_dir_entry;
	int errorcode;


	ent->corrupt = 0;
	ent->eof = 0;

	errorcode = mfs_dir_read_next(&iter->dirref, &cur_dir_entry);
	if (errorcode)
	{
		/* error */
		ent->corrupt = 1;
		return errorcode;
	}
	else if (!cur_dir_entry)
	{
		/* EOF */
		ent->eof = 1;
		return 0;
	}

	/* copy info */
	mac_to_c_strncpy(ent->fname, ent->attr_len, cur_dir_entry->name);
	snprintf(ent->attr, ent->attr_len, "%s", "");
	ent->filesize = get_UINT32BE(cur_dir_entry->dataPhysicalSize)
						+ get_UINT32BE(cur_dir_entry->rsrcPhysicalSize);

	return 0;
}

/*
	Free enumerator
*/
static void mfs_image_closeenum(IMAGEENUM *enumeration)
{
	free(enumeration);
}

/*
	Compute free space on disk image (in allocation blocks?)
*/
static size_t mfs_image_freespace(IMAGE *img)
{
	mfs_l2_imgref *image = (mfs_l2_imgref *) img;

	return image->num_free_blocks;
}

/*
	Extract a file from a mfs image.  The file is saved in macbinary format.
*/
static int mfs_image_readfile(IMAGE *img, const char *fname, STREAM *destf)
{
	mfs_l2_imgref *image = (mfs_l2_imgref *) img;
	mac_str255 mac_fname;
	mfs_dirref dirref;
	mfs_dir_entry *dir_entry;
	mfs_fileref fileref;
	int errorcode;
	UINT32 data_len, rsrc_len;
	mac_str255 comment;
	UINT8 buf[512];
	UINT32 i, run_len;


	if (check_fpath(fname))
		return IMGTOOLERR_BADFILENAME;

	if (strchr(fname, ':'))
		return IMGTOOLERR_BADFILENAME;

	/* extract file name */
	c_to_mac_strncpy(mac_fname, fname, strlen(fname));

	/* open dir */
	mfs_dir_open(image, &dirref);

	/* find file */
	errorcode = mfs_find_dir_entry(&dirref, mac_fname, &dir_entry);
	if (errorcode)
		return errorcode;

	/* get lenght of both forks */
	data_len = get_UINT32BE(dir_entry->dataLogicalSize);
	rsrc_len = get_UINT32BE(dir_entry->rsrcLogicalSize);

	/* get comment from Desktop file */
	errorcode = get_comment(image, mfs_hashString(dir_entry->name), comment);
	if (errorcode)
	{	/*
			Ignore any error, as:
			a) get_comment will mistakenly return an error if a file has no
			  attached comment on an MFS volume
			b) we do not really care if the Desktop file is corrupt and we
			  cannot read comments: we can still read the file's data, and it's
			  what really matters
		*/
		comment[0] = '\0';
		errorcode = 0;
	}

	/* write MB header */
	{
		MBHeader header;

		header.version_old = 0;
		mac_strncpy(header.fname, 63, dir_entry->name);
		header.ftype = dir_entry->flFinderInfo.type;
		header.fcreator = dir_entry->flFinderInfo.creator;
		header.flags_MSB = dir_entry->flFinderInfo.flags.bytes[0];
		header.zerofill0 = 0;
		header.location = dir_entry->flFinderInfo.location;
		header.fldr = dir_entry->flFinderInfo.fldr;
		header.protect = dir_entry->flags & 0x01;
		header.zerofill1 = 0;
		header.data_len = dir_entry->dataLogicalSize;
		header.rsrc_len = dir_entry->rsrcLogicalSize;
		header.crDate = dir_entry->createDate;
		header.lsMod = dir_entry->modifyDate;
		set_UINT16BE(&header.comment_len, comment[0]);	/* comment lenght */
		header.flags_LSB = dir_entry->flFinderInfo.flags.bytes[1];
		set_UINT32BE(&header.signature, 0x6d42494e/*'mBIN'*/);
		header.fname_script = 0;	/* IIRC, 0 normally means system script */
									/* I don't think MFS stores the system */
									/* script code anywhere, so it should be a */
									/* reasonable approximation */
		header.xflags = 0;			/* not on a MFS volume */
		memset(header.unused, 0, sizeof(header.unused));
		set_UINT32BE(&header.reserved, 0);
		set_UINT16BE(&header.sec_header_len, 0);
		header.version = 130;
		header.compat_version = 129;

		/* last step: compute CRC... */
		set_UINT16BE(&header.crc, CalcMBCRC(&header));

		set_UINT16BE(&header.reserved2, 0);

		if (stream_write(destf, &header, sizeof(header)) != sizeof(header))
			return IMGTOOLERR_WRITEERROR;
	}

	/* open file DF */
	errorcode = mfs_file_open_DF(image, dir_entry, &fileref);
	if (errorcode)
		return errorcode;

	/* extract DF */
	for (i=0; i<data_len;)
	{
		run_len = data_len - i;
		if (run_len > 512)
			run_len = 512;
		errorcode = mfs_file_read(&fileref, run_len, buf);
		if (errorcode)
			return errorcode;
		if (stream_write(destf, buf, run_len) != run_len)
			return IMGTOOLERR_WRITEERROR;
		i += run_len;
	}
	/* pad to multiple of 128 */
	if (data_len % 128)
	{
		run_len = 128 - (data_len % 128);
		memset(buf, 0, run_len);
		if (stream_write(destf, buf, run_len) != run_len)
			return IMGTOOLERR_WRITEERROR;
	}

	/* open file RF */
	errorcode = mfs_file_open_RF(image, dir_entry, &fileref);
	if (errorcode)
		return errorcode;

	/* extract RF */
	for (i=0; i<rsrc_len;)
	{
		run_len = rsrc_len - i;
		if (run_len > 512)
			run_len = 512;
		errorcode = mfs_file_read(&fileref, run_len, buf);
		if (errorcode)
			return errorcode;
		if (stream_write(destf, buf, run_len) != run_len)
			return IMGTOOLERR_WRITEERROR;
		i += run_len;
	}
	/* pad to multiple of 128 */
	if (rsrc_len % 128)
	{
		run_len = 128 - (rsrc_len % 128);
		memset(buf, 0, run_len);
		if (stream_write(destf, buf, run_len) != run_len)
			return IMGTOOLERR_WRITEERROR;
	}

	/* append comments */
	if (stream_write(destf, comment+1, comment[0]) != comment[0])
		return IMGTOOLERR_WRITEERROR;
	/* pad to multiple of 128 */
	if (comment[0] % 128)
	{
		run_len = 128 - (comment[0] % 128);
		memset(buf, 0, run_len);
		if (stream_write(destf, buf, run_len) != run_len)
			return IMGTOOLERR_WRITEERROR;
	}

	return 0;
}

/*
	Add a file to a mfs image.  The file must be in macbinary format.
*/
static int mfs_image_writefile(IMAGE *img, const char *fpath, STREAM *sourcef, const ResolvedOption *in_options)
{
	return IMGTOOLERR_UNIMPLEMENTED;
}

/*
	Delete a file from a mfs image.
*/
static int mfs_image_deletefile(IMAGE *img, const char *fname)
{
	return IMGTOOLERR_UNIMPLEMENTED;
}

/*
	Create a blank mfs image.
*/
static int mfs_image_create(const struct ImageModule *mod, STREAM *f, const ResolvedOption *in_options)
{
	return IMGTOOLERR_UNIMPLEMENTED;
}

/*
	Read one sector from a mfs image.
*/
static int mfs_read_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, void *buffer, int length)
{
	(void) img; (void) head; (void) track; (void) sector; (void) offset; (void) buffer; (void) length;
	/* not yet implemented */
	return IMGTOOLERR_UNIMPLEMENTED;
}

/*
	Write one sector to a mfs image.
*/
static int mfs_write_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, const void *buffer, int length)
{
	(void) img; (void) head; (void) track; (void) sector; (void) offset; (void) buffer; (void) length;
	/* not yet implemented */
	return IMGTOOLERR_UNIMPLEMENTED;
}
