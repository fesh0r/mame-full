/*
	Handlers for macintosh images.

	Currently, we only handle the MFS format.  This was the format I needed
	most urgently, because modern Macintoshes do not support it any more.
	In the future, I will implement HFS as well.

	Raphael Nabet, 2003
*/

/*
	terminology:
	disk block: 512-byte logical block.  With sectors of 512 bytes, one logical
		block is equivalent to one sector; when the sector size is not 512
		bytes, sectors are split or grouped to 
	allocation block: The File Manager always allocates logical disk blocks to
		a file in groups called allocation blocks; an allocation block is
		simply a group of consecutive logical blocks.  The size of a volume's
		allocation blocks depends on the capacity of the volume; there can be
		at most 4094 (MFS) or 65535 (HFS) allocation blocks on a volume.


	Organization of an MFS volume:

	Logical		Contents									Allocation block
	block

	0 - 1:		System startup information
	2 - m:		Master directory block (MDB)
				+ allocation block link pointers
	m+1 - n:	Directory file
	n+1 - p-2:	Other files and free space					0 - ((p-2)-(n+1))/k
	p-1:		Alternate MDB
	p:			Not used
	usually, k = 2, m = 3, n = 16, p = 799 (SSDD 3.5" floppy)
	with DSDD 3.5" floppy, I assume that p = 1599, but I don't know the other
	values
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
typedef UINT8 mac_str31[32];
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
	rect record, with the corner coordinates
*/
typedef struct mac_rect
{
	UINT16BE top;	/* actually signed */
	UINT16BE left;	/* actually signed */
	UINT16BE bottom;/* actually signed */
	UINT16BE right;	/* actually signed */
} mac_rect;

/*
	FInfo (Finder file info) record
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
								/* The FOBJ resource contains folder info, its
								name if the folder name. */
							/* HFS & HFS+:
								System 7: The window in which the file’s icon appears
								System 8: reserved (set to 0) */
} mac_FInfo;

/*
	FXInfo (Finder extended file info) record -- not found in MFS
*/
typedef struct mac_FXInfo
{
	UINT16BE iconID;		/* System 7: An ID number for the file’s icon; the
								numbers that identify icons are assigned by the
								Finder */
							/* System 8: Reserved (set to 0) */
	UINT16BE reserved[3];	/* Reserved (set to 0) */
	UINT8    script;		/* System 7: if high-bit is set, the script code
								for displaying the file name; ignored otherwise */
							/* System 8: Extended flags MSB(?) */
	UINT8    XFlags;		/* Extended flags */
	UINT16BE comment;		/* System 7: Comment ID if high-bit is clear */
							/* System 8: Reserved (set to 0) */
	UINT32BE putAway;		/* Put away folder ID (i.e. if the user moves the
								file onto the desktop, the directory ID of the
								folder from which the user moves the file is
								saved here) */
} mac_FXInfo;

/*
	DInfo (Finder folder info) record -- not found in MFS
*/
typedef struct mac_DInfo
{
	mac_rect rect;			/* Folder's window bounds */
	UINT16BE flags;			/* Finder flags, e.g. kIsInvisible, kNameLocked, etc */
	mac_point location;		/* Location of the folder in parent window */
								/* If set to {0, 0}, and the initied flag is
								clear, the Finder will place the item
								automatically */
	UINT16BE view;			/* System 7: The manner in which folders are
								displayed */
							/* System 8: reserved (set to 0) */
} mac_DInfo;

/*
	DXInfo (Finder extended folder info) record -- not found in MFS
*/
typedef struct mac_DXInfo
{
	mac_point scroll;		/* Scroll position */
	UINT32BE openChain;		/* System 7: chain of directory IDs for open folders */
							/* System 8: reserved (set to 0) */
	UINT8    script;		/* System 7: if high-bit is set, the script code
								for displaying the folder name; ignored otherwise */
							/* System 8: Extended flags MSB(?) */
	UINT8    XFlags;		/* Extended flags */
	UINT16BE comment;		/* System 7: Comment ID if high-bit is clear */
							/* System 8: Reserved (set to 0) */
	UINT32BE putAway;		/* Put away folder ID (i.e. if the user moves the
								folder onto the desktop, the directory ID of
								the folder from which the user moves it is
								saved here) */
} mac_DXInfo;

/*
	defines for FInfo & DInfo flags fields
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
	using the "%00" sequence.  To avoid any ambiguity, '%' is escaped with
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
	using the "%00" sequence.  To avoid any ambiguity, '%' is escaped with
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

/*
	This table mimics the way HFS (and MFS???) sorts string: this is equivalent
	to the RelString macintosh toolbox call with the caseSensitive parameter as
	false and the diacSensitive parameter as true.

	This sort of makes sense with the MacRoman encoding, as it means the
	computer will regard "DeskTop File", "Desktop File", "Desktop file", etc,
	as the same file.  (UNIX users will probably regard this as an heresy, but
	you must consider that, unlike UNIX, the Macintosh was not designed for
	droids, but human beings that may forget about case.)

	(Also, in the HFS catalog file, letters with diatrical signs will follow
	the corresponding letters without diacritical signs; however, this actually
	does not matter as HFS.)

	However, with other text encodings, it is plainly absurd.  For instance,
	with the Greek encoding, it will think that the degree symbol is the same
	letter (with different case) as the upper-case Psi, so that if a program
	asks for a file called "90°" on a greek HFS volume, and there is a file
	called "90[Psi]" on this volume, file "90[Psi]" will be opened.  Results
	will probably be even weirder with 2-byte script like Japanese or Chinese.
*/
static unsigned char mac_char_sort_table[256] =
{
	0x00,	/* \x00 */
	0x01,	/* \x01 */
	0x02,	/* \x02 */
	0x03,	/* \x03 */
	0x04,	/* \x04 */
	0x05,	/* \x05 */
	0x06,	/* \x06 */
	0x07,	/* \x07 */
	0x08,	/* \x08 */
	0x09,	/* \x09 */
	0x0a,	/* \x0a */
	0x0b,	/* \x0b */
	0x0c,	/* \x0c */
	0x0d,	/* \x0d */
	0x0e,	/* \x0e */
	0x0f,	/* \x0f */
	0x10,	/* \x10 */
	0x11,	/* \x11 */
	0x12,	/* \x12 */
	0x13,	/* \x13 */
	0x14,	/* \x14 */
	0x15,	/* \x15 */
	0x16,	/* \x16 */
	0x17,	/* \x17 */
	0x18,	/* \x18 */
	0x19,	/* \x19 */
	0x1a,	/* \x1a */
	0x1b,	/* \x1b */
	0x1c,	/* \x1c */
	0x1d,	/* \x1d */
	0x1e,	/* \x1e */
	0x1f,	/* \x1f */
	0x20,	/* \x20 */
	0x21,	/* \x21 */
	0x22,	/* \x22 */
	0x27,	/* \x23 */
	0x28,	/* \x24 */
	0x29,	/* \x25 */
	0x2a,	/* \x26 */
	0x2b,	/* \x27 */
	0x2e,	/* \x28 */
	0x2f,	/* \x29 */
	0x30,	/* \x2a */
	0x31,	/* \x2b */
	0x32,	/* \x2c */
	0x33,	/* \x2d */
	0x34,	/* \x2e */
	0x35,	/* \x2f */
	0x36,	/* \x30 */
	0x37,	/* \x31 */
	0x38,	/* \x32 */
	0x39,	/* \x33 */
	0x3a,	/* \x34 */
	0x3b,	/* \x35 */
	0x3c,	/* \x36 */
	0x3d,	/* \x37 */
	0x3e,	/* \x38 */
	0x3f,	/* \x39 */
	0x40,	/* \x3a */
	0x41,	/* \x3b */
	0x42,	/* \x3c */
	0x43,	/* \x3d */
	0x44,	/* \x3e */
	0x45,	/* \x3f */
	0x46,	/* \x40 */
	0x47,	/* \x41 */
	0x51,	/* \x42 */
	0x52,	/* \x43 */
	0x54,	/* \x44 */
	0x55,	/* \x45 */
	0x5a,	/* \x46 */
	0x5b,	/* \x47 */
	0x5c,	/* \x48 */
	0x5d,	/* \x49 */
	0x62,	/* \x4a */
	0x63,	/* \x4b */
	0x64,	/* \x4c */
	0x65,	/* \x4d */
	0x66,	/* \x4e */
	0x68,	/* \x4f */
	0x71,	/* \x50 */
	0x72,	/* \x51 */
	0x73,	/* \x52 */
	0x74,	/* \x53 */
	0x76,	/* \x54 */
	0x77,	/* \x55 */
	0x7c,	/* \x56 */
	0x7d,	/* \x57 */
	0x7e,	/* \x58 */
	0x7f,	/* \x59 */
	0x81,	/* \x5a */
	0x82,	/* \x5b */
	0x83,	/* \x5c */
	0x84,	/* \x5d */
	0x85,	/* \x5e */
	0x86,	/* \x5f */
	0x4d,	/* \x60 */
	0x47,	/* \x61 */
	0x51,	/* \x62 */
	0x52,	/* \x63 */
	0x54,	/* \x64 */
	0x55,	/* \x65 */
	0x5a,	/* \x66 */
	0x5b,	/* \x67 */
	0x5c,	/* \x68 */
	0x5d,	/* \x69 */
	0x62,	/* \x6a */
	0x63,	/* \x6b */
	0x64,	/* \x6c */
	0x65,	/* \x6d */
	0x66,	/* \x6e */
	0x68,	/* \x6f */
	0x71,	/* \x70 */
	0x72,	/* \x71 */
	0x73,	/* \x72 */
	0x74,	/* \x73 */
	0x76,	/* \x74 */
	0x77,	/* \x75 */
	0x7c,	/* \x76 */
	0x7d,	/* \x77 */
	0x7e,	/* \x78 */
	0x7f,	/* \x79 */
	0x81,	/* \x7a */
	0x87,	/* \x7b */
	0x88,	/* \x7c */
	0x89,	/* \x7d */
	0x8a,	/* \x7e */
	0x8b,	/* \x7f */
	0x49,	/* \x80 */
	0x4b,	/* \x81 */
	0x53,	/* \x82 */
	0x56,	/* \x83 */
	0x67,	/* \x84 */
	0x69,	/* \x85 */
	0x78,	/* \x86 */
	0x4e,	/* \x87 */
	0x48,	/* \x88 */
	0x4f,	/* \x89 */
	0x49,	/* \x8a */
	0x4a,	/* \x8b */
	0x4b,	/* \x8c */
	0x53,	/* \x8d */
	0x56,	/* \x8e */
	0x57,	/* \x8f */
	0x58,	/* \x90 */
	0x59,	/* \x91 */
	0x5e,	/* \x92 */
	0x5f,	/* \x93 */
	0x60,	/* \x94 */
	0x61,	/* \x95 */
	0x67,	/* \x96 */
	0x6d,	/* \x97 */
	0x6e,	/* \x98 */
	0x6f,	/* \x99 */
	0x69,	/* \x9a */
	0x6a,	/* \x9b */
	0x79,	/* \x9c */
	0x7a,	/* \x9d */
	0x7b,	/* \x9e */
	0x78,	/* \x9f */
	0x8c,	/* \xa0 */
	0x8d,	/* \xa1 */
	0x8e,	/* \xa2 */
	0x8f,	/* \xa3 */
	0x90,	/* \xa4 */
	0x91,	/* \xa5 */
	0x92,	/* \xa6 */
	0x75,	/* \xa7 */
	0x93,	/* \xa8 */
	0x94,	/* \xa9 */
	0x95,	/* \xaa */
	0x96,	/* \xab */
	0x97,	/* \xac */
	0x98,	/* \xad */
	0x4c,	/* \xae */
	0x6b,	/* \xaf */
	0x99,	/* \xb0 */
	0x9a,	/* \xb1 */
	0x9b,	/* \xb2 */
	0x9c,	/* \xb3 */
	0x9d,	/* \xb4 */
	0x9e,	/* \xb5 */
	0x9f,	/* \xb6 */
	0xa0,	/* \xb7 */
	0xa1,	/* \xb8 */
	0xa2,	/* \xb9 */
	0xa3,	/* \xba */
	0x50,	/* \xbb */
	0x70,	/* \xbc */
	0xa4,	/* \xbd */
	0x4c,	/* \xbe */
	0x6b,	/* \xbf */
	0xa5,	/* \xc0 */
	0xa6,	/* \xc1 */
	0xa7,	/* \xc2 */
	0xa8,	/* \xc3 */
	0xa9,	/* \xc4 */
	0xaa,	/* \xc5 */
	0xab,	/* \xc6 */
	0x25,	/* \xc7 */
	0x26,	/* \xc8 */
	0xac,	/* \xc9 */
	0x20,	/* \xca */
	0x48,	/* \xcb */
	0x4a,	/* \xcc */
	0x6a,	/* \xcd */
	0x6c,	/* \xce */
	0x6c,	/* \xcf */
	0xad,	/* \xd0 */
	0xae,	/* \xd1 */
	0x23,	/* \xd2 */
	0x24,	/* \xd3 */
	0x2c,	/* \xd4 */
	0x2d,	/* \xd5 */
	0xaf,	/* \xd6 */
	0xb0,	/* \xd7 */
	0x80,	/* \xd8 */
	0xb1,	/* \xd9 */
	0xb2,	/* \xda */
	0xb3,	/* \xdb */
	0xb4,	/* \xdc */
	0xb5,	/* \xdd */
	0xb6,	/* \xde */
	0xb7,	/* \xdf */
	0xb8,	/* \xe0 */
	0xb9,	/* \xe1 */
	0xba,	/* \xe2 */
	0xbb,	/* \xe3 */
	0xbc,	/* \xe4 */
	0xbd,	/* \xe5 */
	0xbe,	/* \xe6 */
	0xbf,	/* \xe7 */
	0xc0,	/* \xe8 */
	0xc1,	/* \xe9 */
	0xc2,	/* \xea */
	0xc3,	/* \xeb */
	0xc4,	/* \xec */
	0xc5,	/* \xed */
	0xc6,	/* \xee */
	0xc7,	/* \xef */
	0xc8,	/* \xf0 */
	0xc9,	/* \xf1 */
	0xca,	/* \xf2 */
	0xcb,	/* \xf3 */
	0xcc,	/* \xf4 */
	0xcd,	/* \xf5 */
	0xce,	/* \xf6 */
	0xcf,	/* \xf7 */
	0xd0,	/* \xf8 */
	0xd1,	/* \xf9 */
	0xd2,	/* \xfa */
	0xd3,	/* \xfb */
	0xd4,	/* \xfc */
	0xd5,	/* \xfd */
	0xd6,	/* \xfe */
	0xd7	/* \xff */
};

static int mac_stricmp(const UINT8 *s1, const UINT8 *s2)
{
	size_t common_len;
	int i;
	int c1, c2;

	common_len = (s1[0] <= s2[0]) ? s1[0] : s2[0];

	for (i=0; i<common_len; i++)
	{
		c1 = mac_char_sort_table[s1[i+1]];
		c2 = mac_char_sort_table[s2[i+1]];
		if (c1 != c2)
			return c1 - c2;
	}

	return ((int)s1[0] - s2[0]);
}

/*
	mac_strcpy()

	Copy a macintosh string

	dst (O): dest macintosh string
	src (I): source macintosh string (first byte is lenght)

	Return a zero if s1 and s2 are equal, a negative value if s1 is less than
	s2, and a positive value if s1 is greater than s2.
*/
INLINE void mac_strcpy(UINT8 *dest, const UINT8 *src)
{
	memcpy(dest, src, src[0]+1);
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
	MB_calcCRC

	Compute CRC of macbinary header

	This piece of code was ripped from the MacBinary II+ source (and converted
	from 68k asm to C), as AFAIK the specs for the CRC used by MacBinary are
	not published.

	header (I): pointer to MacBinary header (only 124 first bytes matter, i.e.
		CRC and reserved2 fields are ignored)

	Returns 16-bit CRC value.
*/
static UINT16 MB_calcCRC(const MBHeader *header)
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


static int image_read_block(mac_l1_imgref *image, UINT32 block, void *dest)
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

static int image_write_block(mac_l1_imgref *image, UINT32 block, const void *src)
{
	if (stream_seek(image->f, (image->format == apple_diskcopy) ? block*512 + 84 : block*512, SEEK_SET))
		return IMGTOOLERR_WRITEERROR;

	if (stream_write(image->f, src, 512) != 512)
		return IMGTOOLERR_WRITEERROR;

#if 0
	/* now append tag data */
	if ((image->format == apple_diskcopy) && (image->u.apple_diskcopy.tag_offset))
	{	/* insert tag data from disk image */
		if (stream_seek(image->f, image->u.apple_diskcopy.tag_offset + block*12, SEEK_SET))
			return IMGTOOLERR_WRITEERROR;

		if (stream_read(image->f, (UINT8 *) src + 512, 12) != 12)
			return IMGTOOLERR_WRITEERROR;
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
#pragma mark MFS/HFS WRAPPERS
#endif

typedef enum
{
	L2I_MFS,
	L2I_HFS
} mac_format;

typedef enum { data_fork, rsrc_fork } mac_forkID;

typedef struct mac_l2_imgref mac_l2_imgref;

/*
	MFS image ref
*/
typedef struct mfs_l2_imgref
{
	UINT16 dir_num_files;
	UINT16 dir_start;
	UINT16 dir_blk_len;

	UINT16 ABStart;

	mac_str27 volname;

	UINT8 ABlink[6141];
} mfs_l2_imgref;

/*
	HFS extent descriptor
*/
typedef struct hfs_extent
{
	UINT16BE stABN;			/* first allocation block */
	UINT16BE numABlks;		/* number of allocation blocks */
} hfs_extent;

/*
	HFS likes to group extents by 3 (it is 8 with HFS+), so we create a
	specific type.
*/
typedef hfs_extent hfs_extent_3[3];

/*
	MFS open file ref
*/
typedef struct mfs_fileref
{
	UINT16 stBlk;					/* first allocation block of file */
} mfs_fileref;

/*
	HFS open file ref
*/
typedef struct hfs_fileref
{
	UINT8 forkType;					/* 0x00 for data, 0xff for resource */
	UINT32 fileID;					/* CNID */

	hfs_extent_3 extents;			/* first 3 file extents */
} hfs_fileref;

/*
	MFS/HFS open file ref
*/
typedef struct mac_fileref
{
	mac_l2_imgref *l2_img;			/* image pointer */

	UINT32 eof;						/* logical end-of-file */
	UINT32 pLen;					/* physical end-of-file */

	UINT32 crPs;					/* current position in file */

	UINT8 reload_buf;
	UINT8 block_buffer[512];		/* buffer with current file block */

	union
	{
		mfs_fileref mfs;
		hfs_fileref hfs;
	} u;
} mac_fileref;

/*
	open BT ref
*/
typedef struct mac_BTref
{
	mac_fileref fileref;	/* open B-tree file ref */

	UINT16 nodeSize;		/* size of a node, in bytes */
	UINT32 rootNode;		/* node number of root node */
	UINT32 firstLeafNode;	/* node number of first leaf node */
	UINT32 attributes;		/* persistent attributes about the tree */
	UINT16 treeDepth;		/* maximum height (usually leaf nodes) */

	/* function to compare keys during tree searches */
	int (*key_compare_func)(const void *key1, const void *key2);

	UINT16 node_numRecords;	/* extracted from current node header */
	void *node_buf;			/* current node buffer */
} mac_BTref;

/*
	HFS image ref
*/
typedef struct hfs_l2_imgref
{
	UINT16 VBM_start;

	UINT16 ABStart;

	mac_str27 volname;

	mac_BTref extents_BT;
	mac_BTref cat_BT;

	UINT8 VBM[8192];
} hfs_l2_imgref;

/*
	MFS/HFS image ref
*/
struct mac_l2_imgref
{
	mac_l1_imgref l1_img;

	UINT16 numABs;
	UINT16 blocksperAB;

	UINT16 freeABs;

	mac_format format;
	union
	{
		mfs_l2_imgref mfs;
		hfs_l2_imgref hfs;
	} u;
};

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
	UINT16BE nmAlBlks;		/* number of allocation blocks in volume (0x0187) */
	UINT32BE alBlkSiz;		/* size (in bytes) of allocation blocks (0x00000400) */
	UINT32BE clpSiz;		/* default clump size - number of bytes to allocate
								when a file grows (0x00002000) */
	UINT16BE alBlSt;		/* first allocation block in volume (0x0010) */

	UINT32BE nxtFNum;		/* next unused file number */
	UINT16BE freeABs;		/* number of unused allocation blocks */

	mac_str27 VN;			/* volume name */

	UINT8    ABlink[512-64];/* Link array for file ABs.  Array of nmAlBlks
							12-bit-long entries, indexed by AB address.  If an
							AB belongs to no file, the entry is 0; if an AB is
							the last in the file, the entry is 1; if an AB
							belongs to the file and is not the last one, the
							entry is the AB address of the next file AB plus 1.
							Note that the array extends on as many consecutive
							disk blocks as needed (usually the MDB block plus
							the next one).  Incidentally, this array is not
							saved in the secondary MDB: presumably, the idea
							was that the disk utility could rely on the tag
							data to rebuild the link array if it should ever
							be corrupted. */
} mfs_mdb;

/*
	Master Directory Block
*/
typedef struct hfs_mdb
{
/* First fields are similar to MFS, though several fields have a different meaning */
	UINT8    sigWord[2];	/* volume signature - always $D2D7 */
	UINT32BE crDate;		/* date and time of volume creation */
	UINT32BE lsMod;			/* date and time of last modification */
	UINT16BE atrb;			/* volume attributes (0x0000) */
								/* bit 15 is set if volume is locked by software */
	UINT16BE nmFls;			/* number of files in root folder */
	UINT16BE VBMSt;			/* first block of volume bitmap */
	UINT16BE allocPtr;		/* start of next allocation search */

	UINT16BE nmAlBlks;		/* number of allocation blocks in volume */
	UINT32BE alBlkSiz;		/* size (in bytes) of allocation blocks */
	UINT32BE clpSiz;		/* default clump size - number of bytes to allocate
								when a file grows */
	UINT16BE alBlSt;		/* first allocation block in volume (0x0010) */
	UINT32BE nxtCNID;		/* next unused catalog node ID */
	UINT16BE freeABs;		/* number of unused allocation blocks */
	mac_str27 VN;			/* volume name */

/* next fields are HFS-specific */

	UINT32BE volBkUp;		/* date and time of last backup */
	UINT16BE vSeqNum;		/* volume backup sequence number */
	UINT32BE wrCnt;			/* volume write count */
	UINT32BE xtClpSiz;		/* clump size for extents overflow file */
	UINT32BE ctClpSiz;		/* clump size for catalog file */
	UINT16BE nmRtDirs;		/* number of directories in root folder */
	UINT32BE filCnt;		/* number of files in volume */
	UINT32BE dirCnt;		/* number of directories in volume */
	UINT8    fndrInfo[32];	/* information used by the Finder */

	union
	{
		struct
		{
			UINT16BE VCSize;		/* size (in blocks) of volume cache */
			UINT16BE VBMCSize;		/* size (in blocks) of volume bitmap cache */
			UINT16BE ctlCSize;		/* size (in blocks) of common volume cache */
		} v1;
		struct
		{
			UINT16BE embedSigWord;	/* embedded volume signature */
			hfs_extent embedExtent;	/* embedded volume location and size */
		} v2;
	} u;

	UINT32BE xtFlSize;		/* size of extents overflow file */
	hfs_extent_3 xtExtRec;	/* extent record for extents overflow file */
	UINT32BE ctFlSize;		/* size of catalog file */
	hfs_extent_3 ctExtRec;	/* extent record for catalog file */
} hfs_mdb;

/* to save a little stack space, we use the same buffer for MDB and next blocks */
typedef union img_open_buf
{
	mfs_mdb mfs_mdb;
	hfs_mdb hfs_mdb;
	UINT8 raw[512];
} img_open_buf;

/*
	Information extracted from catalog/directory
*/
typedef struct mac_cat_info
{
	mac_FInfo flFinderInfo;		/* information used by the Finder */
	mac_FXInfo flXFinderInfo;	/* information used by the Finder */

	UINT8  flags;				/* bit 0=1 if file locked */

	UINT32 fileID;				/* file ID in directory/catalog */

	UINT32 dataLogicalSize;		/* logical EOF of data fork */
	UINT32 dataPhysicalSize;	/* physical EOF of data fork */
	UINT32 rsrcLogicalSize;		/* logical EOF of resource fork */
	UINT32 rsrcPhysicalSize;	/* physical EOF of resource fork */

	UINT32 createDate;			/* date and time of creation */
	UINT32 modifyDate;			/* date and time of last modification */
} mac_cat_info;

static int mfs_image_open(mac_l2_imgref *l2_img, img_open_buf *buf);
static int hfs_image_open(mac_l2_imgref *l2_img, img_open_buf *buf);
static void hfs_image_close(mac_l2_imgref *l2_img);
static int mfs_file_get_nth_block_address(mac_fileref *fileref, UINT32 block_num, UINT32 *block_address);
static int hfs_file_get_nth_block_address(mac_fileref *fileref, UINT32 block_num, UINT32 *block_address);
static int mfs_resolve_fpath(mac_l2_imgref *l2_img, const char *fpath, mac_str255 fname, mac_cat_info *cat_info);
static int hfs_resolve_fpath(mac_l2_imgref *l2_img, const char *fpath, UINT32 *parID, mac_str255 fname, mac_cat_info *cat_info);
static int mfs_file_open(mac_l2_imgref *l2_img, const mac_str255 fname, mac_fileref *fileref, mac_forkID fork);
static int hfs_file_open(mac_l2_imgref *l2_img, UINT32 parID, const mac_str255 fname, mac_fileref *fileref, mac_forkID fork);

/*
	mac_image_open

	Open a macintosh image.  Image must already be open on level 1.
*/
static int mac_image_open(mac_l2_imgref *l2_img)
{
	img_open_buf buf;
	int errorcode;

	errorcode = image_read_block(&l2_img->l1_img, 2, &buf.raw);
	if (errorcode)
		return errorcode;

	switch ((buf.raw[0] << 8) | buf.raw[1])
	{
	case 0xd2d7:
		l2_img->format = L2I_MFS;
		return mfs_image_open(l2_img, &buf);

	case 0x4244:
		l2_img->format = L2I_HFS;
		return hfs_image_open(l2_img, &buf);
	}

	return IMGTOOLERR_CORRUPTIMAGE;
}

static void mac_image_close(mac_l2_imgref *l2_img)
{
	switch (l2_img->format)
	{
	case L2I_MFS:
		break;

	case L2I_HFS:
		hfs_image_close(l2_img);
		break;
	}
}

static int mac_resolve_fpath(mac_l2_imgref *l2_img, const char *fpath, UINT32 *parID, mac_str255 fname, mac_cat_info *cat_info)
{
	switch (l2_img->format)
	{
	case L2I_MFS:
		*parID = 0;
		return mfs_resolve_fpath(l2_img, fpath, fname, cat_info);

	case L2I_HFS:
		return hfs_resolve_fpath(l2_img, fpath, parID, fname, cat_info);
	}

	return IMGTOOLERR_UNEXPECTED;
}

/*
	mac_file_open

	Open a file
*/
static int mac_file_open(mac_l2_imgref *l2_img, UINT32 parID, const mac_str255 fname, mac_fileref *fileref, mac_forkID fork)
{
	switch (l2_img->format)
	{
	case L2I_MFS:
		return mfs_file_open(l2_img, fname, fileref, fork);

	case L2I_HFS:
		return hfs_file_open(l2_img, parID, fname, fileref, fork);
	}

	return IMGTOOLERR_UNEXPECTED;
}

/*
	mac_file_read
*/
static int mac_file_read(mac_fileref *fileref, UINT32 len, void *dest)
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
			switch (fileref->l2_img->format)
			{
			case L2I_MFS:
				errorcode = mfs_file_get_nth_block_address(fileref, fileref->crPs/512, &block);
				break;

			case L2I_HFS:
				errorcode = hfs_file_get_nth_block_address(fileref, fileref->crPs/512, &block);
				break;
			}
			if (errorcode)
				return errorcode;
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
	mac_file_tell
*/
static int mac_file_tell(mac_fileref *fileref, UINT32 *filePos)
{
	*filePos = fileref->crPs;

	return 0;
}

/*
	mac_file_seek
*/
static int mac_file_seek(mac_fileref *fileref, UINT32 filePos)
{
	if ((fileref->crPs / 512) != (filePos / 512))
		fileref->reload_buf = TRUE;

	fileref->crPs = filePos;

	return 0;
}

#if 0
#pragma mark -
#pragma mark MFS IMPLEMENTATION
#endif

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
	UINT8    flVersNum;			/* version number (usually 0x00, but I don't
									have the IM volume that describes it) */
	mac_FInfo flFinderInfo;		/* information used by the Finder */

	UINT32BE fileID;			/* file ID */

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
	UINT8 unknown3[22];		/* ??? */
	union
	{	/* I think there are two versions of the structure */
		struct
		{
			UINT16BE item_count;	/* number of items (folders and files) in
										this folder */
			UINT32BE item_descs[1];	/* this variable-lenght array has
										item_count entries */
		} v1;
		struct
		{
			UINT16BE zerofill;		/* always 0? */
			UINT16BE unknown0;		/* always 0??? */
			UINT16BE item_count;	/* number of items (folders and files) in
										this folder */
			UINT8 unknown1[20];		/* ??? */
			UINT8 name[1];			/* variable-lenght macintosh string */
		} v2;
	} u;
} mfs_FOBJ;

/*
	MFS open dir ref
*/
typedef struct mfs_dirref
{
	mac_l2_imgref *l2_img;			/* image pointer */
	UINT16 index;					/* current file index in the disk directory */
	UINT16 cur_block;				/* current block offset in directory file */
	UINT16 cur_offset;				/* current byte offset in current block of directory file */
	UINT8 block_buffer[512];		/* buffer with current directory block */
} mfs_dirref;

/*
	mfs_image_open

	Open a MFS image.  Image must already be open on level 1.
*/
static int mfs_image_open(mac_l2_imgref *l2_img, img_open_buf *buf)
{
	int errorcode;

	assert(l2_img->format == L2I_MFS);

	if ((buf->mfs_mdb.sigWord[0] != 0xd2) || (buf->mfs_mdb.sigWord[1] != 0xd7)
			|| (buf->mfs_mdb.VN[0] > 27))
		return IMGTOOLERR_CORRUPTIMAGE;

	l2_img->u.mfs.dir_num_files = get_UINT16BE(buf->mfs_mdb.nmFls);
	l2_img->u.mfs.dir_start = get_UINT16BE(buf->mfs_mdb.dirSt);
	l2_img->u.mfs.dir_blk_len = get_UINT16BE(buf->mfs_mdb.blLn);

	l2_img->numABs = get_UINT16BE(buf->mfs_mdb.nmAlBlks);
	if (get_UINT32BE(buf->mfs_mdb.alBlkSiz) % 512)
		return IMGTOOLERR_CORRUPTIMAGE;
	l2_img->blocksperAB = get_UINT32BE(buf->mfs_mdb.alBlkSiz) / 512;
	l2_img->u.mfs.ABStart = get_UINT16BE(buf->mfs_mdb.alBlSt);

	l2_img->freeABs = get_UINT16BE(buf->mfs_mdb.freeABs);

	mac_strcpy(l2_img->u.mfs.volname, buf->mfs_mdb.VN);

	if (l2_img->numABs > 4094)
		return IMGTOOLERR_CORRUPTIMAGE;

	/* extract link array */
	{
		int byte_len = l2_img->numABs + ((l2_img->numABs + 1) >> 1);
		int cur_byte = 0;
		int cur_block = 2;
		int block_len = sizeof(buf->mfs_mdb.ABlink);

		/* append the chunk after MDB to link array */
		if (block_len > (byte_len - cur_byte))
			block_len = byte_len - cur_byte;
		memcpy(l2_img->u.mfs.ABlink+cur_byte, buf->mfs_mdb.ABlink, block_len);
		cur_byte += block_len;
		while (cur_byte < byte_len)
		{
			/* read next block */
			cur_block++;
			errorcode = image_read_block(&l2_img->l1_img, cur_block, buf->raw);
			if (errorcode)
				return errorcode;
			block_len = 512;

			/* append this block to link array */
			if (block_len > (byte_len - cur_byte))
				block_len = byte_len - cur_byte;
			memcpy(l2_img->u.mfs.ABlink+cur_byte, buf->raw, block_len);
			cur_byte += block_len;
		}
		/* check that link array and directory don't overlap */
		if (cur_block >= l2_img->u.mfs.dir_start)
			return IMGTOOLERR_CORRUPTIMAGE;
	}

	return 0;
}

/*
	mfs_dir_open

	Open the directory file
*/
static int mfs_dir_open(mac_l2_imgref *l2_img, mfs_dirref *dirref)
{
	assert(l2_img->format == L2I_MFS);

	dirref->l2_img = l2_img;
	dirref->index = 0;

	dirref->cur_block = 0;
	dirref->cur_offset = 0;
	image_read_block(&l2_img->l1_img, l2_img->u.mfs.dir_start + dirref->cur_block, dirref->block_buffer);

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

	if (dirref->index == dirref->l2_img->u.mfs.dir_num_files)
		/* EOF */
		return 0;

	/* get cat entry pointer */
	cur_dir_entry = (mfs_dir_entry *) (dirref->block_buffer + dirref->cur_offset);
	while ((dirref->cur_block == 512) || ! (cur_dir_entry->flags & 0x80))
	{
		dirref->cur_block++;
		dirref->cur_offset = 0;
		if (dirref->cur_block > dirref->l2_img->u.mfs.dir_blk_len)
			/* aargh! */
			return IMGTOOLERR_CORRUPTIMAGE;
		image_read_block(&dirref->l2_img->l1_img, dirref->l2_img->u.mfs.dir_start + dirref->cur_block, dirref->block_buffer);
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

static int mfs_resolve_fpath(mac_l2_imgref *l2_img, const char *fpath, mac_str255 fname, mac_cat_info *cat_info)
{
	mfs_dirref dirref;
	mfs_dir_entry *dir_entry;
	int errorcode;

	/* rapid check */
	if (strchr(fpath, ':'))
		return IMGTOOLERR_BADFILENAME;

	/* extract file name */
	c_to_mac_strncpy(fname, fpath, strlen(fpath));

	/* open dir */
	mfs_dir_open(l2_img, &dirref);

	/* find file */
	errorcode = mfs_find_dir_entry(&dirref, fname, &dir_entry);
	if (errorcode)
		return errorcode;

	mac_strcpy(fname, dir_entry->name);

	if (cat_info)
	{
		cat_info->flFinderInfo = dir_entry->flFinderInfo;
		memset(&cat_info->flXFinderInfo, 0, sizeof(cat_info->flXFinderInfo));
		cat_info->flags = dir_entry->flags;
		cat_info->fileID = get_UINT32BE(dir_entry->fileID);
		cat_info->dataLogicalSize = get_UINT32BE(dir_entry->dataLogicalSize);
		cat_info->dataPhysicalSize = get_UINT32BE(dir_entry->dataPhysicalSize);
		cat_info->rsrcLogicalSize = get_UINT32BE(dir_entry->rsrcLogicalSize);
		cat_info->rsrcPhysicalSize = get_UINT32BE(dir_entry->rsrcPhysicalSize);
		cat_info->createDate = get_UINT32BE(dir_entry->createDate);
		cat_info->modifyDate = get_UINT32BE(dir_entry->modifyDate);
	}

	return 0;
}

/*
	mfs_file_open

	Open a file data fork
*/
static int mfs_file_open_internal(mac_l2_imgref *l2_img, const mfs_dir_entry *dir_entry, mac_fileref *fileref, mac_forkID fork)
{
	assert(l2_img->format == L2I_MFS);

	fileref->l2_img = l2_img;

	switch (fork)
	{
	case data_fork:
		fileref->u.mfs.stBlk = get_UINT16BE(dir_entry->dataStartBlock);
		fileref->eof = get_UINT32BE(dir_entry->dataLogicalSize);
		fileref->pLen = get_UINT32BE(dir_entry->dataPhysicalSize);
		break;

	case rsrc_fork:
		fileref->u.mfs.stBlk = get_UINT16BE(dir_entry->rsrcStartBlock);
		fileref->eof = get_UINT32BE(dir_entry->rsrcLogicalSize);
		fileref->pLen = get_UINT32BE(dir_entry->rsrcPhysicalSize);
		break;
	}

	fileref->crPs = 0;

	fileref->reload_buf = TRUE;

	return 0;
}

static int mfs_file_open(mac_l2_imgref *l2_img, const mac_str255 fname, mac_fileref *fileref, mac_forkID fork)
{
	mfs_dirref dirref;
	mfs_dir_entry *dir_entry;
	int errorcode;

	/* open dir */
	mfs_dir_open(l2_img, &dirref);

	/* find file */
	errorcode = mfs_find_dir_entry(&dirref, fname, &dir_entry);
	if (errorcode)
		return errorcode;

	/* open it */
	return mfs_file_open_internal(l2_img, dir_entry, fileref, fork);
}

/*
	mfs_get_ABlink
*/
static UINT16 mfs_get_ABlink(mac_l2_imgref *l2_img, UINT16 AB_address)
{
	UINT16 reply;
	int base;

	assert(l2_img->format == L2I_MFS);

	base = (AB_address >> 1) * 3;

	if (! (AB_address & 1))
		reply = (l2_img->u.mfs.ABlink[base] << 4) | ((l2_img->u.mfs.ABlink[base+1] >> 4) & 0x0f);
	else
		reply = ((l2_img->u.mfs.ABlink[base+1] << 8) & 0xf00) | l2_img->u.mfs.ABlink[base+2];

	return reply;
}

/*
	mfs_file_get_nth_block_address
*/
static int mfs_file_get_nth_block_address(mac_fileref *fileref, UINT32 block_num, UINT32 *block_address)
{
	UINT32 AB_num;
	UINT32 i;
	UINT16 AB_address;

	assert(fileref->l2_img->format == L2I_MFS);

	AB_num = block_num / fileref->l2_img->blocksperAB;

	AB_address = fileref->u.mfs.stBlk;
	if ((AB_address == 0) || (AB_address >= fileref->l2_img->numABs+2))
		/* 0 -> ??? */
		return IMGTOOLERR_CORRUPTIMAGE;
	if (AB_address == 1)
		/* EOF */
		return IMGTOOLERR_UNEXPECTED;
	AB_address -= 2;
	for (i=0; i<AB_num; i++)
	{
		AB_address = mfs_get_ABlink(fileref->l2_img, AB_address);
		if ((AB_address == 0) || (AB_address >= fileref->l2_img->numABs+2))
			/* 0 -> empty block: there is no way an empty block could make it
			into the link chain!!! */
			return IMGTOOLERR_CORRUPTIMAGE;
		if (AB_address == 1)
			/* EOF */
			return IMGTOOLERR_UNEXPECTED;
		AB_address -= 2;
	}

	*block_address = fileref->l2_img->u.mfs.ABStart + AB_address * fileref->l2_img->blocksperAB
						+ block_num % fileref->l2_img->blocksperAB;

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
#pragma mark HFS IMPLEMENTATION
#endif

/*
	HFS extents B-tree key
*/
typedef struct hfs_extentKey
{
	UINT8    keyLength;		/* length of key, excluding this field */
	UINT8    forkType;		/* 0 = data fork, FF = resource fork */
	UINT32BE fileID;		/* file ID */
	UINT16BE startBlock;	/* first file allocation block number in this extent */
} hfs_extentKey;
enum
{
	keyLength_hfs_extentKey = sizeof(hfs_extentKey) - sizeof(UINT8)
};

/*
	HFS catalog B-tree key
*/
typedef struct hfs_catKey
{
	UINT8    keyLen;		/* key length */
	UINT8    resrv1;		/* reserved */
	UINT32BE parID;			/* parent directory ID */
	mac_str31 cName;		/* catalog node name */
							/* note that in index nodes, it is a mac_str31, but
								in leaf keys it's a variable-lenght string */
} hfs_catKey;

/*
	HFS catalog data record for a folder - 70 bytes
*/
typedef struct hfs_catFolderData
{
	UINT16BE recordType;		/* record type */
	UINT16BE flags;				/* folder flags */
	UINT16BE valence;			/* folder valence */
	UINT32BE folderID;			/* folder ID */
	UINT32BE createDate;		/* date and time of creation */
	UINT32BE modifyDate;		/* date and time of last modification */
	UINT32BE backupDate;		/* date and time of last backup */
	mac_DInfo userInfo;			/* Finder information */
	mac_DXInfo finderInfo;		/* additional Finder information */
	UINT32BE reserved[4];		/* reserved - set to zero */
} hfs_catFolderData;

/*
	HFS catalog data record for a file - 102 bytes
*/
typedef struct hfs_catFileData
{
	UINT16BE recordType;		/* record type */
	UINT8    flags;				/* file flags */
	UINT8    fileType;			/* file type (reserved, always 0?) */
	mac_FInfo userInfo;			/* Finder information */
	UINT32BE fileID;			/* file ID */
	UINT16BE dataStartBlock;	/* not used - set to zero */
	UINT32BE dataLogicalSize;	/* logical EOF of data fork */
	UINT32BE dataPhysicalSize;	/* physical EOF of data fork */
	UINT16BE rsrcStartBlock;	/* not used - set to zero */
	UINT32BE rsrcLogicalSize;	/* logical EOF of resource fork */
	UINT32BE rsrcPhysicalSize;	/* physical EOF of resource fork */
	UINT32BE createDate;		/* date and time of creation */
	UINT32BE modifyDate;		/* date and time of last modification */
	UINT32BE backupDate;		/* date and time of last backup */
	mac_FXInfo finderInfo;		/* additional Finder information */
	UINT16BE clumpSize;			/* file clump size (not used) */
	hfs_extent_3 dataExtents;	/* first data fork extent record */
	hfs_extent_3 rsrcExtents;	/* first resource fork extent record */
	UINT32BE reserved;			/* reserved - set to zero */
} hfs_catFileData;

/*
	HFS catalog data record for a thread - 46 bytes

	The key for a thread record features the CNID of the item and an empty
	name, instead of the CNID of the parent and the item name.
*/
typedef struct hfs_catThreadData
{
  UINT16BE recordType;			/* record type */
  UINT32BE reserved[2];			/* reserved - set to zero */
  UINT32BE parID;				/* parent ID for this catalog node */
  mac_str31 nodeName;			/* name of this catalog node */
} hfs_catThreadData;

/*
	HFS catalog record types
*/
enum
{
	hcrt_Folder			= 0x0100,	/* Folder record */
	hcrt_File			= 0x0200,	/* File record */
	hcrt_FolderThread	= 0x0300,	/* Folder thread record */
	hcrt_FileThread		= 0x0400	/* File thread record */
};

/*
	Catalog file record flags

	This is not unlike the MFS catalog flag field, but the "thread exists" flag
	(0x02) is specific to HFS/HFS+, whereas the "Record in use" flag (0x80) is
	only used by MFS.
*/
enum
{
	cfrf_fileLocked		= 0x01,		/* file is locked and cannot be written to */
	cfrf_threadExists	= 0x02		/* a file thread record exists for this file */
};

static int BT_open(mac_BTref *BTref, int (*key_compare_func)(const void *key1, const void *key2));
static void BT_close(mac_BTref *BTref);
static int BT_search_leaf_rec(mac_BTref *BTref, void *search_key,
								UINT32 *node_ID, int *record_ID, void **record_ptr);

static int hfs_open_extents_file(mac_l2_imgref *l2_img, const hfs_mdb *mdb, mac_fileref *fileref)
{
	assert(l2_img->format == L2I_HFS);

	fileref->l2_img = l2_img;

	fileref->u.hfs.forkType = 0x00;
	fileref->u.hfs.fileID = 3;

	fileref->eof = fileref->pLen = get_UINT32BE(mdb->xtFlSize);
	memcpy(fileref->u.hfs.extents, mdb->xtExtRec, sizeof(hfs_extent_3));

	fileref->crPs = 0;

	fileref->reload_buf = TRUE;

	return 0;
}

static int hfs_open_cat_file(mac_l2_imgref *l2_img, const hfs_mdb *mdb, mac_fileref *fileref)
{
	assert(l2_img->format == L2I_HFS);

	fileref->l2_img = l2_img;

	fileref->u.hfs.forkType = 0x00;
	&fileref->u.hfs.fileID, 4;

	fileref->eof = fileref->pLen = get_UINT32BE(mdb->ctFlSize);
	memcpy(fileref->u.hfs.extents, mdb->ctExtRec, sizeof(hfs_extent_3));

	fileref->crPs = 0;

	fileref->reload_buf = TRUE;

	return 0;
}

static int hfs_extentKey_compare(const void *p1, const void *p2)
{
	const hfs_extentKey *key1 = p1;
	const hfs_extentKey *key2 = p2;

	/* let's keep it simple for now */
	return memcmp(key1, key2, sizeof(hfs_extentKey));
}

static int hfs_catKey_compare(const void *p1, const void *p2)
{
	const hfs_catKey *key1 = p1;
	const hfs_catKey *key2 = p2;

	if (get_UINT32BE(key1->parID) != get_UINT32BE(key2->parID))
		return (get_UINT32BE(key1->parID) < get_UINT32BE(key2->parID)) ? -1 : +1;

	return mac_stricmp(key1->cName, key2->cName);
}

/*
	hfs_image_open

	Open a HFS image.  Image must already be open on level 1.
*/
static int hfs_image_open(mac_l2_imgref *l2_img, img_open_buf *buf)
{
	int errorcode;

	assert(l2_img->format == L2I_HFS);

	if ((buf->hfs_mdb.sigWord[0] != 0x42) || (buf->hfs_mdb.sigWord[1] != 0x44)
			|| (buf->hfs_mdb.VN[0] > 27))
		return IMGTOOLERR_CORRUPTIMAGE;

	l2_img->u.hfs.VBM_start = get_UINT16BE(buf->hfs_mdb.VBMSt);

	l2_img->numABs = get_UINT16BE(buf->hfs_mdb.nmAlBlks);
	if (get_UINT32BE(buf->hfs_mdb.alBlkSiz) % 512)
		return IMGTOOLERR_CORRUPTIMAGE;
	l2_img->blocksperAB = get_UINT32BE(buf->hfs_mdb.alBlkSiz) / 512;
	l2_img->u.hfs.ABStart = get_UINT16BE(buf->hfs_mdb.alBlSt);

	l2_img->freeABs = get_UINT16BE(buf->hfs_mdb.freeABs);

	mac_strcpy(l2_img->u.hfs.volname, buf->hfs_mdb.VN);

	/* open extents and catalog BT */
	errorcode = hfs_open_extents_file(l2_img, &buf->hfs_mdb, &l2_img->u.hfs.extents_BT.fileref);
	if (errorcode)
		return errorcode;
	errorcode = BT_open(&l2_img->u.hfs.extents_BT, hfs_extentKey_compare);
	if (errorcode)
		return errorcode;
	errorcode = hfs_open_cat_file(l2_img, &buf->hfs_mdb, &l2_img->u.hfs.cat_BT.fileref);
	if (errorcode)
		return errorcode;
	errorcode = BT_open(&l2_img->u.hfs.cat_BT, hfs_catKey_compare);
	if (errorcode)
		return errorcode;

	/* extract volume bitmap */
	{
		int byte_len = (l2_img->numABs + 7) / 8;
		int cur_byte = 0;
		int cur_block = l2_img->u.hfs.VBM_start;

		while (cur_byte < byte_len)
		{
			/* read next block */
			errorcode = image_read_block(&l2_img->l1_img, cur_block, buf->raw);
			if (errorcode)
				return errorcode;
			cur_block++;

			/* append this block to VBM */
			memcpy(l2_img->u.hfs.VBM+cur_byte, buf->raw, 512);
			cur_byte += 512;
		}
	}

	return 0;
}

static void hfs_image_close(mac_l2_imgref *l2_img)
{
	assert(l2_img->format == L2I_HFS);

	BT_close(&l2_img->u.hfs.extents_BT);
	BT_close(&l2_img->u.hfs.cat_BT);
}

static int hfs_cat_search(mac_l2_imgref *l2_img, UINT32 parID, const mac_str31 cName, void **record_ptr)
{
	hfs_catKey search_key;

	assert(l2_img->format == L2I_HFS);

	if (cName[0] > 31)
		return IMGTOOLERR_UNEXPECTED;

	search_key.keyLen = search_key.resrv1 = 0;	/* these fields do not matter
												to the compare function, so we
												don't fill them */
	set_UINT32BE(&search_key.parID, parID);
	mac_strcpy(search_key.cName, cName);


	return BT_search_leaf_rec(&l2_img->u.hfs.cat_BT, &search_key, NULL, NULL, record_ptr);
}

static int hfs_resolve_fpath(mac_l2_imgref *l2_img, const char *fpath, UINT32 *parID, mac_str255 fname, mac_cat_info *cat_info)
{
	const char *element_start, *element_end;
	int element_len;
	mac_str255 mac_element_name;
	UINT32 lparID = 2;
	int level;
	int errorcode;
	void *catrec;
	hfs_catKey *catrec_key;
	union
	{
		UINT16BE dataType;
		hfs_catFolderData folder;
		hfs_catFileData file;
		hfs_catThreadData thread;
	} *catrec_data;
	UINT16 dataRecType;


	/* iterate each path element */
	element_start = fpath;
	level = 0;
	do
	{
		/* find next path element */
		element_end = strchr(element_start, ':');
		element_len = element_end ? (element_end - element_start) : strlen(element_start);
		/* decode path element name */
		c_to_mac_strncpy(mac_element_name, element_start, element_len);
		/* iterate element */
		if (mac_element_name[0] == 0)
		{
			if (!element_end)
				/* trailing ':' or empty string */
				/* this means a directory is expected: not supported for now */
				return IMGTOOLERR_BADFILENAME;
			else if (level == 0)
				/* '::' sequence -> parent directory, but we simply can't get
				the parent of the root dir */
				return IMGTOOLERR_BADFILENAME;
		}

		errorcode = hfs_cat_search(l2_img, lparID, mac_element_name, &catrec);
		if (errorcode)
			return errorcode;

		catrec_key = catrec;
		/* data is past the key, but aligned on 16-bit boundary */
		catrec_data = (void *) ((UINT8 *)catrec + catrec_key->keyLen + (! (catrec_key->keyLen & 1)) + 1);
		dataRecType = get_UINT16BE(catrec_data->dataType);

		/* parse data record */
		if (mac_element_name[0] == 0)
		{	/* '::' sequence -> parent directory */
			if (dataRecType != hcrt_FolderThread)
				/* Hurk! */
				return IMGTOOLERR_CORRUPTIMAGE;

			lparID = get_UINT32BE(catrec_data->thread.parID);
			level--;
		}
		else
		{	/* regular folder/file name */
			if (!element_end)
			{
				/* file name expected for now */
				if (dataRecType != hcrt_File)
					return IMGTOOLERR_BADFILENAME;
			}
			else
			{
				/* folder expected */
				if (dataRecType != hcrt_Folder)
					return IMGTOOLERR_BADFILENAME;
				lparID = get_UINT32BE(catrec_data->folder.folderID);
				level++;
			}
		}

		/* iterate */
		if (element_end)
			element_start = element_end+1;
		else
			element_start = NULL;
	}
	while (element_start);

	/* save ref */
	*parID = get_UINT32BE(catrec_key->parID);
	mac_strcpy(fname, catrec_key->cName);


	if (cat_info)
	{
		cat_info->flFinderInfo = catrec_data->file.userInfo;
		cat_info->flXFinderInfo = catrec_data->file.finderInfo;
		cat_info->flags = catrec_data->file.flags;
		cat_info->fileID = get_UINT32BE(catrec_data->file.fileID);
		cat_info->dataLogicalSize = get_UINT32BE(catrec_data->file.dataLogicalSize);
		cat_info->dataPhysicalSize = get_UINT32BE(catrec_data->file.dataPhysicalSize);
		cat_info->rsrcLogicalSize = get_UINT32BE(catrec_data->file.rsrcLogicalSize);
		cat_info->rsrcPhysicalSize = get_UINT32BE(catrec_data->file.rsrcPhysicalSize);
		cat_info->createDate = get_UINT32BE(catrec_data->file.createDate);
		cat_info->modifyDate = get_UINT32BE(catrec_data->file.modifyDate);
	}

	return 0;
}

static int hfs_file_open_internal(mac_l2_imgref *l2_img, const hfs_catFileData *file_rec, mac_fileref *fileref, mac_forkID fork)
{
	assert(l2_img->format == L2I_HFS);

	fileref->l2_img = l2_img;

	fileref->u.hfs.fileID = get_UINT32BE(file_rec->fileID);

	switch (fork)
	{
	case data_fork:
		fileref->u.hfs.forkType = 0x00;
		fileref->eof = get_UINT32BE(file_rec->dataLogicalSize);
		fileref->pLen = get_UINT32BE(file_rec->dataPhysicalSize);
		memcpy(fileref->u.hfs.extents, file_rec->dataExtents, sizeof(hfs_extent_3));
		break;

	case rsrc_fork:
		fileref->u.hfs.forkType = 0xff;
		fileref->eof = get_UINT32BE(file_rec->rsrcLogicalSize);
		fileref->pLen = get_UINT32BE(file_rec->rsrcPhysicalSize);
		memcpy(fileref->u.hfs.extents, file_rec->rsrcExtents, sizeof(hfs_extent_3));
		break;
	}

	fileref->crPs = 0;

	fileref->reload_buf = TRUE;

	return 0;
}

static int hfs_file_open(mac_l2_imgref *l2_img, UINT32 parID, const mac_str255 fname, mac_fileref *fileref, mac_forkID fork)
{
	void *catrec;
	hfs_catKey *catrec_key;
	union
	{
		UINT16BE dataType;
		hfs_catFolderData folder;
		hfs_catFileData file;
		hfs_catThreadData thread;
	} *catrec_data;
	UINT16 dataRecType;
	int errorcode;

	/* lookup file in catalog */
	errorcode = hfs_cat_search(l2_img, parID, fname, &catrec);
	if (errorcode)
		return errorcode;

	catrec_key = catrec;
	/* data is past the key, but aligned on 16-bit boundary */
	catrec_data = (void *) ((UINT8 *)catrec + catrec_key->keyLen + (! (catrec_key->keyLen & 1)) + 1);
	dataRecType = get_UINT16BE(catrec_data->dataType);

	/* file expected */
	if (dataRecType != hcrt_File)
		return IMGTOOLERR_BADFILENAME;

	/* open it */
	return hfs_file_open_internal(l2_img, &catrec_data->file, fileref, fork);
}

/*
	hfs_file_get_nth_block_address
*/
static int hfs_file_get_nth_block_address(mac_fileref *fileref, UINT32 block_num, UINT32 *block_address)
{
	UINT32 AB_num;
	UINT32 cur_AB;
	UINT32 i;
	hfs_extent *cur_extents;
	void *extents_BT_rec;
	int errorcode;
	UINT16 AB_address;

	assert(fileref->l2_img->format == L2I_HFS);

	AB_num = block_num / fileref->l2_img->blocksperAB;
	cur_AB = 0;
	cur_extents = fileref->u.hfs.extents;

	while (1)
	{
		for (i=0; i<3; i++)
		{
			if (AB_num < cur_AB+get_UINT16BE(cur_extents[i].numABlks))
				break;
			cur_AB += get_UINT16BE(cur_extents[i].numABlks);
		}
		if (i < 3)
			/* extent found */
			break;
		else
		{
			/* extent not found: read next extents from extents BT */
			hfs_extentKey search_key;

			search_key.keyLength = keyLength_hfs_extentKey;
			search_key.forkType = fileref->u.hfs.forkType;
			set_UINT32BE(&search_key.fileID, fileref->u.hfs.fileID);
			set_UINT16BE(&search_key.startBlock, cur_AB);

			errorcode = BT_search_leaf_rec(&fileref->l2_img->u.hfs.extents_BT, &search_key,
											NULL, NULL, &extents_BT_rec);
			if (errorcode)
				return errorcode;

			cur_extents = (hfs_extent *) ((UINT8 *)extents_BT_rec + 8/*((hfs_extentKey *)extents_BT_rec)->keyLengthkeyLen + (! (((hfs_extentKey *)extents_BT_rec)->keyLen & 1)) + 1*/);
		}
	}

	AB_address = get_UINT16BE(cur_extents[i].stABN) + (AB_num-cur_AB);

	if (AB_address >= fileref->l2_img->numABs)
		return IMGTOOLERR_CORRUPTIMAGE;

	*block_address = fileref->l2_img->u.hfs.ABStart + AB_address * fileref->l2_img->blocksperAB
						+ block_num % fileref->l2_img->blocksperAB;

	return 0;
}

#if 0
#pragma mark -
#pragma mark B-TREE IMPLEMENTATION
#endif

/*
	B-tree files are used by HFS and HFS+ file system: the Extents and Catalog
	are both B-Tree

	These files are normal files, except for the fact that they are not
	referenced from the catalog but the MDB.  They are allocated in fixed-size
	records of 512 bytes (HFS).  (HFS+ supports any power of two from 512
	through 32768, and uses a default of 1024 for Extents, and 4096 for both
	Catalog and Attributes.)

	Nodes can contain any number of records.  The nodes can be of any of four
	types: header node (unique node with b-tree information, pointer to root
	node and start of the node allocation bitmap), map nodes (which are created
	when the node allocation bitmap outgrows the header node), index nodes
	(root and branch node that enable to efficiently search the leaf nodes for
	a specific key value), and leaf nodes (which hold the actual user data
	records with keys and data).  The first node is a header node.
*/

/*
	BTNodeHeader

	Header of a node record
*/
typedef struct BTNodeHeader
{
	UINT32BE fLink;			/* (index of?) next node at this level */
	UINT32BE bLink;			/* (index of?) previous node at this level */
	UINT8    kind;			/* kind of node (leaf, index, header, map) */
	UINT8    height;		/* zero for header, map; child is one more than parent */
	UINT16BE numRecords;	/* number of records in this node */
	UINT16BE reserved;		/* reserved; set to zero */
} BTNodeHeader;

/*
	Constants for BTNodeHeader kind field
*/
enum
{
	btnk_leafNode	= 0xff,	/* leaf nodes hold the actual user data records
								with keys and data */
	btnk_indexNode	= 0,	/* root and branch node that enable to efficiently
								search the leaf nodes for a specific key value */
	btnk_headerNode	= 1,	/* unique node with b-tree information, pointer to
								root node and start of the node allocation
								bitmap */
	btnk_mapNode	= 2		/* map nodes are created when the node allocation
								bitmap outgrows the header node */
};

/*
	BTHeaderRecord: first record of a B-tree header node (second record is
	unused, and third is node allocation bitmap).
*/
typedef struct BTHeaderRecord
{
	UINT16BE treeDepth;		/* maximum height (usually leaf nodes) */
	UINT32BE rootNode;		/* node number of root node */
	UINT32BE leafRecords;	/* number of leaf records in all leaf nodes */
	UINT32BE firstLeafNode;	/* node number of first leaf node */
	UINT32BE lastLeafNode;	/* node number of last leaf node */
	UINT16BE nodeSize;		/* size of a node, in bytes */
	UINT16BE maxKeyLength;	/* reserved */
	UINT32BE totalNodes;	/* total number of nodes in tree */
	UINT32BE freeNodes;		/* number of unused (free) nodes in tree */
	UINT16BE reserved1;		/* unused */
	UINT32BE clumpSize;		/* reserved */
	UINT8    btreeType;		/* reserved */
	UINT8    reserved2;		/* reserved */
	UINT32BE attributes;	/* persistent attributes about the tree */
	UINT32BE reserved3[16];	/* reserved */
} BTHeaderRecord;

/*
	Constants for BTHeaderRec attributes field
*/
enum
{
	btha_badCloseMask			= 0x00000001,	/* reserved */
	btha_bigKeysMask			= 0x00000002,	/* key length field is 16 bits */
	btha_variableIndexKeysMask	= 0x00000004	/* keys in index nodes are variable length */
};

typedef struct BT_leaf_rec_enumerator
{
	mac_BTref *BTref;
	UINT32 cur_node;
	int cur_rec;
} BT_leaf_rec_enumerator;

/*
	BT_open

	Open a file as a B-tree.  The file must be already open as a macintosh
	file.

	resfileref (I/O): ressource file handle to open (the resfileref->fileref
		must have been open previously)

	Returns imgtool error code
*/
static int BT_open(mac_BTref *BTref, int (*key_compare_func)(const void *key1, const void *key2))
{
	int errorcode;
	BTNodeHeader node_header;
	BTHeaderRecord header_rec;

	/* seek to node 0 */
	errorcode = mac_file_seek(&BTref->fileref, 0);
	if (errorcode)
		return errorcode;

	/* read node header */
	errorcode = mac_file_read(&BTref->fileref, sizeof(node_header), &node_header);
	if (errorcode)
		return errorcode;

	if ((node_header.kind != btnk_headerNode) || (get_UINT16BE(node_header.numRecords) < 3))
		return IMGTOOLERR_CORRUPTIMAGE;	/* right??? */

	/* CHEESY HACK: we assume that the header record immediately follows the
	node header.  This is because we need to know the node lenght to know where
	the record pointers are located, but we need to read the header record to
	know the node lenght. */
	errorcode = mac_file_read(&BTref->fileref, sizeof(header_rec), &header_rec);
	if (errorcode)
		return errorcode;

	BTref->nodeSize = get_UINT16BE(header_rec.nodeSize);
	BTref->rootNode = get_UINT32BE(header_rec.rootNode);
	BTref->firstLeafNode = get_UINT32BE(header_rec.firstLeafNode);
	BTref->attributes = get_UINT32BE(header_rec.attributes);
	BTref->treeDepth = get_UINT16BE(header_rec.treeDepth);

	BTref->key_compare_func = key_compare_func;

	BTref->node_buf = malloc(BTref->nodeSize);
	if (!BTref->node_buf)
		return IMGTOOLERR_OUTOFMEMORY;

	return 0;
}

static void BT_close(mac_BTref *BTref)
{
	free(BTref->node_buf);
}

static int BT_read_node(mac_BTref *BTref, UINT32 node_ID)
{
	int errorcode;

	/* seek to node */
	errorcode = mac_file_seek(&BTref->fileref, node_ID*BTref->nodeSize);
	if (errorcode)
		return errorcode;

	/* read it */
	errorcode = mac_file_read(&BTref->fileref, BTref->nodeSize, BTref->node_buf);
	if (errorcode)
		return errorcode;

	/* extract numRecords field */
	BTref->node_numRecords = get_UINT16BE(((BTNodeHeader *) BTref->node_buf)->numRecords);

	return 0;
}

static int BT_get_node_record(mac_BTref *BTref, unsigned recnum, void **rec_ptr)
{
	UINT16 offset;

	if (recnum >= BTref->node_numRecords)
		return IMGTOOLERR_UNEXPECTED;

	offset = get_UINT16BE(((UINT16BE *)((UINT8 *) BTref->node_buf + BTref->nodeSize))[-recnum-1]);

	if ((offset < sizeof(BTNodeHeader)) || (offset > BTref->nodeSize-2*BTref->node_numRecords))
		return IMGTOOLERR_CORRUPTIMAGE;

	*rec_ptr = (UINT8 *)BTref->node_buf + offset;

	return 0;
}

static int BT_search_leaf_rec(mac_BTref *BTref, void *search_key,
								UINT32 *node_ID, int *record_ID, void **record_ptr)
{
	int errorcode;
	int i;
	UINT32 cur_node;
	void *cur_record;
	void *last_record;
	int key_len;
	int depth;
	int compare_result;

	/* start with root node */
	if (!BTref->rootNode)
		/* tree is empty */
		return IMGTOOLERR_FILENOTFOUND;

	cur_node = BTref->rootNode;
	depth = 1;

	while (1)
	{
		/* check depth is in bound (only think that prevent us from locking if
		there is a cycle in the tree structure) */
		if (depth > BTref->treeDepth)
			return IMGTOOLERR_CORRUPTIMAGE;

		/* read current node */
		errorcode = BT_read_node(BTref, cur_node);
		if (errorcode)
			return errorcode;

		/* check node */
		if ((((BTNodeHeader *) BTref->node_buf)->kind != btnk_indexNode)
				&& (((BTNodeHeader *) BTref->node_buf)->kind != btnk_leafNode))
			/* bad node */
			return IMGTOOLERR_CORRUPTIMAGE;

		last_record = cur_record = NULL;
		for (i=0; i<BTref->node_numRecords; i++)
		{
			errorcode = BT_get_node_record(BTref, i, &cur_record);
			if (errorcode)
				return errorcode;

			compare_result = (*BTref->key_compare_func)(cur_record, search_key);
			if (compare_result > 0)
				break;
			last_record = cur_record;
			if (compare_result == 0)
				break;
		}

		if (! last_record)
			/* all keys are greater than the search key: the search key is
			nowhere in the tree */
			return IMGTOOLERR_FILENOTFOUND;

		if (((BTNodeHeader *) BTref->node_buf)->kind == btnk_leafNode)
			/* leaf node -> end of search */
			break;

		/* iterate to next level */
		key_len = (BTref->attributes & btha_bigKeysMask)
					? * (UINT16 *)last_record :
					* (UINT8 *)last_record;

		/* align to word */
		if (key_len & 1)
			key_len++;

		cur_node = get_UINT32BE(* (UINT32BE *)((UINT8 *)last_record + key_len));
		depth++;
	}

	if (compare_result != 0)
		/* key not found */
		return IMGTOOLERR_FILENOTFOUND;

	if (node_ID)
		*node_ID = cur_node;

	if (record_ID)
		*record_ID = i;

	if (record_ptr)
		*record_ptr = last_record;

	return 0;
}

static int BT_leaf_rec_enumerator_open(mac_BTref *BTref, BT_leaf_rec_enumerator *enumerator)
{
	enumerator->BTref = BTref;
	enumerator->cur_node = BTref->firstLeafNode;
	enumerator->cur_rec = 0;

	return 0;
}

static int BT_leaf_rec_enumerator_read(BT_leaf_rec_enumerator *enumerator, void **record_ptr)
{
	int errorcode;

	*record_ptr = NULL;

	if (enumerator->cur_node == 0)
	{
		return 0;
	}

	errorcode = BT_read_node(enumerator->BTref, enumerator->cur_node);
	if (errorcode)
		return errorcode;

	errorcode = BT_get_node_record(enumerator->BTref, enumerator->cur_rec, record_ptr);
	if (errorcode)
		return errorcode;

	enumerator->cur_rec++;
	if (enumerator->cur_rec == enumerator->BTref->node_numRecords)
	{
		enumerator->cur_node = get_UINT32BE(((BTNodeHeader *) enumerator->BTref->node_buf)->fLink);
		enumerator->cur_rec = 0;
	}

	return 0;
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
							/* This is actually part of the type list, which matters for offsets */
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
	mac_fileref fileref;	/* open resource fork ref (you may open resources
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

	Open a file as a resource file.  The file must be already open as a
	macintosh file.

	resfileref (I/O): ressource file handle to open (the resfileref->fileref
		must have been open previously)

	Returns imgtool error code
*/
static int resfile_open(mac_resfileref *resfileref)
{
	int errorcode;
	rsrc_header header;
	rsrc_map_header map_header;

	/* seek to resource header */
	errorcode = mac_file_seek(&resfileref->fileref, 0);
	if (errorcode)
		return errorcode;

	errorcode = mac_file_read(&resfileref->fileref, sizeof(header), &header);
	if (errorcode)
		return errorcode;

	resfileref->data_offs = get_UINT32BE(header.data_offs);
	resfileref->map_offs = get_UINT32BE(header.map_offs);

	/* seek to resource map header */
	errorcode = mac_file_seek(&resfileref->fileref, resfileref->map_offs);
	if (errorcode)
		return errorcode;

	errorcode = mac_file_read(&resfileref->fileref, sizeof(map_header), &map_header);
	if (errorcode)
		return errorcode;

	resfileref->typelist_offs = get_UINT16BE(map_header.typelist_offs);
	resfileref->namelist_offs = get_UINT16BE(map_header.namelist_offs);
	resfileref->type_count = get_UINT16BE(map_header.type_count);

	return 0;
}

/*
	resfile_get_entry

	Get the resource entry in the resource map associated with a given type/id
	pair.

	resfileref (I/O): open ressource file handle
	type (I): type of the resource
	id (I): id of the resource
	entry (O): resource entry that has been read

	Returns imgtool error code
*/
static int resfile_get_entry(mac_resfileref *resfileref, UINT32 type, UINT16 id, rsrc_ref_entry *entry)
{
	int errorcode;
	rsrc_type_entry type_entry;
	UINT16 ref_count;
	int i;

	/* seek to resource type list in resource map */
	errorcode = mac_file_seek(&resfileref->fileref, resfileref->map_offs+resfileref->typelist_offs+2);
	if (errorcode)
		return errorcode;

	if (resfileref->type_count == 0xffff)
		/* type list is empty */
		return IMGTOOLERR_FILENOTFOUND;

	for (i=0; i<=resfileref->type_count; i++)
	{
		errorcode = mac_file_read(&resfileref->fileref, sizeof(type_entry), &type_entry);
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
	errorcode = mac_file_seek(&resfileref->fileref, resfileref->map_offs+resfileref->typelist_offs+get_UINT16BE(type_entry.ref_offs));
	if (errorcode)
		return errorcode;

	if (ref_count == 0xffff)
		/* ref list is empty */
		return IMGTOOLERR_FILENOTFOUND;

	for (i=0; i<=ref_count; i++)
	{
		errorcode = mac_file_read(&resfileref->fileref, sizeof(*entry), entry);
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

	Get the name of a resource.

	resfileref (I/O): open ressource file handle
	entry (I): resource entry in the resource map (returned by
		resfile_get_entry)
	string (O): ressource name

	Returns imgtool error code
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
	errorcode = mac_file_seek(&resfileref->fileref, resfileref->map_offs+name_offs);
	if (errorcode)
		return errorcode;

	/* get string lenght */
	errorcode = mac_file_read(&resfileref->fileref, 1, &len);
	if (errorcode)
		return errorcode;

	string[0] = len;

	/* get string data */
	errorcode = mac_file_read(&resfileref->fileref, len, string+1);
	if (errorcode)
		return errorcode;

	return 0;
}

/*
	resfile_get_reslen

	Get the data lenght for a given resource.

	resfileref (I/O): open ressource file handle
	entry (I): resource entry in the resource map (returned by
		resfile_get_entry)
	len (O): ressource lenght

	Returns imgtool error code
*/
static int resfile_get_reslen(mac_resfileref *resfileref, const rsrc_ref_entry *entry, UINT32 *len)
{
	int errorcode;
	UINT32 data_offs;
	UINT32BE llen;

	data_offs = get_UINT24BE(entry->data_offs);

	/* seek to resource data in resource data section */
	errorcode = mac_file_seek(&resfileref->fileref, resfileref->data_offs+data_offs);
	if (errorcode)
		return errorcode;

	/* get data lenght */
	errorcode = mac_file_read(&resfileref->fileref, sizeof(llen), &llen);
	if (errorcode)
		return errorcode;

	*len = get_UINT32BE(llen);

	return 0;
}

/*
	resfile_get_resdata

	Get the data for a given resource.

	resfileref (I/O): open ressource file handle
	entry (I): resource entry in the resource map (returned by
		resfile_get_entry)
	offset (I): offset the data should be read from, usually 0
	len (I): lenght of the data to read, usually the value returned by
		resfile_get_reslen
	dest (O): ressource data

	Returns imgtool error code
*/
static int resfile_get_resdata(mac_resfileref *resfileref, const rsrc_ref_entry *entry, UINT32 offset, UINT32 len, void *dest)
{
	int errorcode;
	UINT32 data_offs;
	UINT32BE llen;

	data_offs = get_UINT24BE(entry->data_offs);

	/* seek to resource data in resource data section */
	errorcode = mac_file_seek(&resfileref->fileref, resfileref->data_offs+data_offs);
	if (errorcode)
		return errorcode;

	/* get data lenght */
	errorcode = mac_file_read(&resfileref->fileref, sizeof(llen), &llen);
	if (errorcode)
		return errorcode;

	/* check we are within tolerances */
	if ((offset + len) > get_UINT32BE(llen))
		return IMGTOOLERR_UNEXPECTED;

	if (offset)
	{	/* seek to resource data offset in resource data section */
		errorcode = mac_file_seek(&resfileref->fileref, resfileref->data_offs+data_offs+4+offset);
		if (errorcode)
			return errorcode;
	}

	/* get data */
	errorcode = mac_file_read(&resfileref->fileref, len, dest);
	if (errorcode)
		return errorcode;

	return 0;
}

#if 0
#pragma mark -
#pragma mark DESKTOP FILE IMPLEMENTATION
#endif

/*
	get_comment

	Get a comment from the Desktop file

	l2_img (I): macintosh image the data should be read from
	id (I): comment id (from mfs_hashString())
	comment (O): comment that has been read

	Returns imgtool error code
*/
static int get_comment(mac_l2_imgref *l2_img, UINT16 id, mac_str255 comment)
{
	UINT8 desktop_fname[] = {'\7','D','e','s','k','t','o','p'};
	#define restype_FCMT (('F' << 24) | ('C' << 16) | ('M' << 8) | 'T')
	mac_resfileref resfileref;
	rsrc_ref_entry resentry;
	UINT32 reslen;
	int errorcode;

	/* open rsrc fork of file Desktop in root directory */
	errorcode = mac_file_open(l2_img, 2, desktop_fname, &resfileref.fileref, rsrc_fork);
	if (errorcode)
		return errorcode;

	/* open resource structures */
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

static int mac_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
static void mac_image_exit(IMAGE *img);
static void mac_image_info(IMAGE *img, char *string, const int len);
static int mac_image_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int mac_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void mac_image_closeenum(IMAGEENUM *enumeration);
static size_t mac_image_freespace(IMAGE *img);
static int mac_image_readfile(IMAGE *img, const char *fname, STREAM *destf);
static int mac_image_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const ResolvedOption *options_);
static int mac_image_deletefile(IMAGE *img, const char *fname);
static int mac_image_create(const struct ImageModule *mod, STREAM *f, const ResolvedOption *options_);

static int mac_read_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, void *buffer, int length);
static int mac_write_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, const void *buffer, int length);

#if 0
static struct OptionTemplate mac_createopts[] =
{
	/*{ "label",	"Volume name", IMGOPTION_FLAG_TYPE_STRING | IMGOPTION_FLAG_HASDEFAULT,	0,	0,	NULL},*/
	{ NULL, NULL, 0, 0, 0, 0 }
};
#endif

/*enum
{
	mac_createopts_volname = 0
};*/

IMAGEMODULE(
	mac,
	"Macintosh disk image (MFS & HFS)",	/* human readable name */
	"img",							/* file extension */
	NULL,							/* crcfile */
	NULL,							/* crc system name */
	EOLN_CR,						/* eoln */
	0,								/* flags */
	mac_image_init,					/* init function */
	mac_image_exit,					/* exit function */
	mac_image_info,					/* info function */
	mac_image_beginenum,			/* begin enumeration */
	mac_image_nextenum,				/* enumerate next */
	mac_image_closeenum,			/* close enumeration */
	mac_image_freespace,			/* free space on image */
	mac_image_readfile,				/* read file */
	/*mac_image_writefile*/NULL,	/* write file */
	/*mac_image_deletefile*/NULL,	/* delete file */
	/*mac_image_create*/NULL,		/* create image */
	/*mac_read_sector*/NULL,
	/*mac_write_sector*/NULL,
	NULL,							/* file options */
	/*mac_createopts*/NULL			/* create options */
)

/*
	Open a file as a mfs image (common code).
*/
static int mac_image_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg)
{
	mac_l2_imgref *image;
	int errorcode;


	image = malloc(sizeof(mac_l2_imgref));
	* (mac_l2_imgref **) outimg = image;
	if (image == NULL)
		return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(mac_l2_imgref));
	image->l1_img.base.module = mod;
	image->l1_img.f = f;

	errorcode = floppy_image_open(&image->l1_img);
	if (errorcode)
		return errorcode;

	return mac_image_open(image);
}

/*
	close a mfs image
*/
static void mac_image_exit(IMAGE *img)
{
	mac_l2_imgref *image = (mac_l2_imgref *) img;

	mac_image_close(image);
	free(image);
}

/*
	get basic information on a mfs image

	Currently returns the volume name
*/
static void mac_image_info(IMAGE *img, char *string, const int len)
{
	mac_l2_imgref *image = (mac_l2_imgref *) img;

	switch (image->format)
	{
	case L2I_MFS:
		mac_to_c_strncpy(string, len, image->u.mfs.volname);
		break;

	case L2I_HFS:
		mac_to_c_strncpy(string, len, image->u.hfs.volname);
		break;
	}
}

/*
	MFS/HFS catalog iterator, used when imgtool reads the catalog
*/
typedef struct mac_iterator
{
	IMAGEENUM base;
	mac_format format;
	mac_l2_imgref *l2_img;
	union
	{
		struct
		{
			mfs_dirref dirref;				/* open directory reference */
		} mfs;
		struct
		{
			BT_leaf_rec_enumerator catref;	/* catalog file enumerator */
		} hfs;
	} u;
} mac_iterator;

/*
	Open the disk catalog for enumeration 
*/
static int mac_image_beginenum(IMAGE *img, IMAGEENUM **outenum)
{
	mac_l2_imgref *image = (mac_l2_imgref *) img;
	mac_iterator *iter;
	int errorcode;


	iter = malloc(sizeof(mac_iterator));
	*((mac_iterator **) outenum) = iter;
	if (iter == NULL)
		return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = img->module;
	iter->format = image->format;
	iter->l2_img = image;

	switch (iter->format)
	{
	case L2I_MFS:
		errorcode = mfs_dir_open(image, &iter->u.mfs.dirref);
		break;

	case L2I_HFS:
		errorcode = BT_leaf_rec_enumerator_open(&image->u.hfs.cat_BT, &iter->u.hfs.catref);
		break;

	default:
		errorcode = IMGTOOLERR_UNEXPECTED;
		break;
	}

	if (errorcode)
	{
		free(iter);
		*((mac_iterator **) outenum) = NULL;
		return errorcode;
	}

	return 0;
}

/*
	Enumerate disk catalog next entry (MFS)
*/
static int mfs_image_nextenum(mac_iterator *iter, imgtool_dirent *ent)
{
	mfs_dir_entry *cur_dir_entry;
	int errorcode;


	assert(iter->format == L2I_MFS);

	ent->corrupt = 0;
	ent->eof = 0;

	errorcode = mfs_dir_read_next(&iter->u.mfs.dirref, &cur_dir_entry);
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
	mac_to_c_strncpy(ent->fname, ent->fname_len, cur_dir_entry->name);
	snprintf(ent->attr, ent->attr_len, "%s", "");
	ent->filesize = get_UINT32BE(cur_dir_entry->dataPhysicalSize)
						+ get_UINT32BE(cur_dir_entry->rsrcPhysicalSize);

	return 0;
}

/*
	Concatenate path elements in the reverse order

	dest (O): destination buffer
	dest_cur_pos (I/O): current position in destination buffer (buffer is
		filled from end to start)
	dest_max_len (I): lenght of destination buffer (use lenght minus one if you
		want to preserve a trailing NUL character)
	src (I): source C string
*/
static void concat_fname(char *dest, int *dest_cur_pos, int dest_max_len, const char *src)
{
	static const char ellipsis[] = { '.', '.', '.' };
	int src_len = strlen(src);	/* number of chars from src to insert */

	if (src_len <= *dest_cur_pos)
	{
		*dest_cur_pos -= src_len;
		memcpy(dest + *dest_cur_pos, src, src_len);
	}
	else
	{
		memcpy(dest, src + src_len - *dest_cur_pos, *dest_cur_pos);
		*dest_cur_pos = 0;
		memcpy(dest, ellipsis, (sizeof(ellipsis) <= dest_max_len)
										? sizeof(ellipsis)
										: dest_max_len);
	}
}

/*
	Enumerate disk catalog next entry (HFS)
*/
static int hfs_image_nextenum(mac_iterator *iter, imgtool_dirent *ent)
{
	void *catrec;
	hfs_catKey *catrec_key;
	union
	{
		UINT16BE dataType;
		hfs_catFolderData folder;
		hfs_catFileData file;
		hfs_catThreadData thread;
	} *catrec_data;
	UINT16 dataRecType;
	int errorcode;
	/* currently, the mac->C conversion transcodes one mac char with at most 3
	C chars */
	char buf[31*3+1];
	UINT32 parID;
	int cur_name_head;
	static const unsigned char mac_empty_str[1] = { '\0' };


	assert(iter->format == L2I_HFS);

	ent->corrupt = 0;
	ent->eof = 0;

	do
	{
		errorcode = BT_leaf_rec_enumerator_read(&iter->u.hfs.catref, &catrec);
		if (errorcode)
		{
			/* error */
			ent->corrupt = 1;
			return errorcode;
		}
		else if (!catrec)
		{
			/* EOF */
			ent->eof = 1;
			return 0;
		}

		catrec_key = catrec;
		/* data is past the key, but aligned on 16-bit boundary */
		catrec_data = (void *) ((UINT8 *)catrec + catrec_key->keyLen + (! (catrec_key->keyLen & 1)) + 1);
		dataRecType = get_UINT16BE(catrec_data->dataType);
	} while (((dataRecType != hcrt_Folder) && (dataRecType != hcrt_File))
				|| (get_UINT32BE(catrec_key->parID) == 1));
				/* condition (parID == 1) prevents us from displaying the root
				directory record */

	/* copy info */
	switch (get_UINT16BE(catrec_data->dataType))
	{
	case hcrt_Folder:
		snprintf(ent->attr, ent->attr_len, "%s", "DIR");
		ent->filesize = 0;
		break;

	case hcrt_File:
		snprintf(ent->attr, ent->attr_len, "%s", "FILE");
		ent->filesize = get_UINT32BE(catrec_data->file.dataPhysicalSize)
							+ get_UINT32BE(catrec_data->file.rsrcPhysicalSize);
		break;
	}

	/* initialize file path buffer */
	cur_name_head = ent->fname_len;
	if (cur_name_head > 0)
	{
		cur_name_head--;
		ent->fname[cur_name_head] = '\0';
	}

	/* insert folder/file name in buffer */
	mac_to_c_strncpy(buf, sizeof(buf), catrec_key->cName);
	if (ent->fname_len > 0)
		concat_fname(ent->fname, &cur_name_head, ent->fname_len-1, buf);
	/* extract parent directory ID */
	parID = get_UINT32BE(catrec_key->parID);

	/* looping while (parID != 1) will display the volume name; looping while
	(parID != 2) won't */
	while (parID != /*1*/2)
	{
		/* search catalog for folder thread */
		errorcode = hfs_cat_search(iter->l2_img, parID, mac_empty_str, &catrec);
		if (errorcode)
		{
			/* error */
			if (ent->fname_len > 0)
			{
				concat_fname(ent->fname, &cur_name_head, ent->fname_len-1, ":");
				concat_fname(ent->fname, &cur_name_head, ent->fname_len-1, "???");
			}
			memmove(ent->fname, ent->fname+cur_name_head, ent->fname_len-cur_name_head);
			ent->corrupt = 1;
			return errorcode;
		}

		/* got some record with the expected key: parse it */
		catrec_key = catrec;
		/* data is past the key, but aligned on 16-bit boundary */
		catrec_data = (void *) ((UINT8 *)catrec + catrec_key->keyLen + (! (catrec_key->keyLen & 1)) + 1);
		dataRecType = get_UINT16BE(catrec_data->dataType);

		if (dataRecType != hcrt_FolderThread)
		{
			/* error */
			if (ent->fname_len > 0)
			{
				concat_fname(ent->fname, &cur_name_head, ent->fname_len-1, ":");
				concat_fname(ent->fname, &cur_name_head, ent->fname_len-1, "???");
			}
			memmove(ent->fname, ent->fname+cur_name_head, ent->fname_len-cur_name_head);
			ent->corrupt = 1;
			return IMGTOOLERR_CORRUPTIMAGE;
		}

		/* got folder thread record: insert the folder name at the start of
		file path, then iterate */
		mac_to_c_strncpy(buf, sizeof(buf), catrec_data->thread.nodeName);
		if (ent->fname_len > 0)
		{
			concat_fname(ent->fname, &cur_name_head, ent->fname_len-1, ":");
			concat_fname(ent->fname, &cur_name_head, ent->fname_len-1, buf);
		}
		/* extract parent directory ID */
		parID = get_UINT32BE(catrec_data->thread.parID);
	}
	memmove(ent->fname, ent->fname+cur_name_head, ent->fname_len-cur_name_head);

	return 0;
}

/*
	Enumerate disk catalog next entry
*/
static int mac_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
{
	mac_iterator *iter = (mac_iterator *) enumeration;

	switch (iter->format)
	{
	case L2I_MFS:
		return mfs_image_nextenum(iter, ent);

	case L2I_HFS:
		return hfs_image_nextenum(iter, ent);
	}

	assert(1);

	return IMGTOOLERR_UNEXPECTED;
}

/*
	Free enumerator
*/
static void mac_image_closeenum(IMAGEENUM *enumeration)
{
	free(enumeration);
}

/*
	Compute free space on disk image (in allocation blocks?)
*/
static size_t mac_image_freespace(IMAGE *img)
{
	mac_l2_imgref *image = (mac_l2_imgref *) img;

	return image->freeABs;
}

/*
	Extract a file from a disk image.  The file is saved in macbinary format.
*/
static int mac_image_readfile(IMAGE *img, const char *fpath, STREAM *destf)
{
	mac_l2_imgref *image = (mac_l2_imgref *) img;
	UINT32 parID;
	mac_str255 fname;
	mac_cat_info cat_info;
	mac_fileref fileref;
	int errorcode;
	UINT32 data_len, rsrc_len;
	UINT16 commentID;
	mac_str255 comment;
	UINT8 buf[512];
	UINT32 i, run_len;

#if 0
	if (image->format == L2I_HFS)
		/* no HFS yet */
		return IMGTOOLERR_UNIMPLEMENTED;
#endif

	/* resolve path and fetch file info from directory/catalog */
	errorcode = mac_resolve_fpath(image, fpath, &parID, fname, &cat_info);
	if (errorcode)
		return errorcode;

	/* get lenght of both forks */
	data_len = cat_info.dataLogicalSize;
	rsrc_len = cat_info.rsrcLogicalSize;

	/* get comment from Desktop file */
	switch (image->format)
	{
	case L2I_MFS:
		commentID = mfs_hashString(fname);
		errorcode = get_comment(image, commentID, comment);
		break;

	case L2I_HFS:
		/* This is the way to get Finder comments in system <= 7.  Attached
		comments use another method, and Finder 8 uses yet another one. */
		commentID = get_UINT16BE(cat_info.flXFinderInfo.comment);
		if (commentID)
			errorcode = get_comment(image, commentID, comment);
		else
		{
			comment[0] = '\0';
			errorcode = 0;
		}
		break;
	}
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
		mac_strncpy(header.fname, 63, fname);
		header.ftype = cat_info.flFinderInfo.type;
		header.fcreator = cat_info.flFinderInfo.creator;
		header.flags_MSB = cat_info.flFinderInfo.flags.bytes[0];
		header.zerofill0 = 0;
		header.location = cat_info.flFinderInfo.location;
		header.fldr = cat_info.flFinderInfo.fldr;
		header.protect = cat_info.flags & 0x01;
		header.zerofill1 = 0;
		set_UINT32BE(&header.data_len, cat_info.dataLogicalSize);
		set_UINT32BE(&header.rsrc_len, cat_info.rsrcLogicalSize);
		set_UINT32BE(&header.crDate, cat_info.createDate);
		set_UINT32BE(&header.lsMod, cat_info.modifyDate);
		set_UINT16BE(&header.comment_len, comment[0]);	/* comment lenght */
		header.flags_LSB = cat_info.flFinderInfo.flags.bytes[1];
		set_UINT32BE(&header.signature, 0x6d42494e/*'mBIN'*/);
		/* Next two fields are set to 0 with MFS volumes.  IIRC, 0 normally
		means system script: I don't think MFS stores the file name script code
		anywhere on disk, so it should be a reasonable approximation. */
		header.fname_script = cat_info.flXFinderInfo.script;
		header.xflags = cat_info.flXFinderInfo.XFlags;
		memset(header.unused, 0, sizeof(header.unused));
		set_UINT32BE(&header.reserved, 0);
		set_UINT16BE(&header.sec_header_len, 0);
		header.version = 130;
		header.compat_version = 129;

		/* last step: compute CRC... */
		set_UINT16BE(&header.crc, MB_calcCRC(&header));

		set_UINT16BE(&header.reserved2, 0);

		if (stream_write(destf, &header, sizeof(header)) != sizeof(header))
			return IMGTOOLERR_WRITEERROR;
	}

	/* open file DF */
	errorcode = mac_file_open(image, parID, fname, &fileref, data_fork);
	if (errorcode)
		return errorcode;

	/* extract DF */
	for (i=0; i<data_len;)
	{
		run_len = data_len - i;
		if (run_len > 512)
			run_len = 512;
		errorcode = mac_file_read(&fileref, run_len, buf);
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
	errorcode = mac_file_open(image, parID, fname, &fileref, rsrc_fork);
	if (errorcode)
		return errorcode;

	/* extract RF */
	for (i=0; i<rsrc_len;)
	{
		run_len = rsrc_len - i;
		if (run_len > 512)
			run_len = 512;
		errorcode = mac_file_read(&fileref, run_len, buf);
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
static int mac_image_writefile(IMAGE *img, const char *fpath, STREAM *sourcef, const ResolvedOption *in_options)
{
	return IMGTOOLERR_UNIMPLEMENTED;
}

/*
	Delete a file from a mfs image.
*/
static int mac_image_deletefile(IMAGE *img, const char *fname)
{
	return IMGTOOLERR_UNIMPLEMENTED;
}

/*
	Create a blank mfs image.
*/
static int mac_image_create(const struct ImageModule *mod, STREAM *f, const ResolvedOption *in_options)
{
	return IMGTOOLERR_UNIMPLEMENTED;
}

/*
	Read one sector from a mfs image.
*/
static int mac_read_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, void *buffer, int length)
{
	(void) img; (void) head; (void) track; (void) sector; (void) offset; (void) buffer; (void) length;
	/* not yet implemented */
	return IMGTOOLERR_UNIMPLEMENTED;
}

/*
	Write one sector to a mfs image.
*/
static int mac_write_sector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, const void *buffer, int length)
{
	(void) img; (void) head; (void) track; (void) sector; (void) offset; (void) buffer; (void) length;
	/* not yet implemented */
	return IMGTOOLERR_UNIMPLEMENTED;
}
