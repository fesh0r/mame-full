/*
	Handlers for macintosh images (MFS and HFS formats).

	Raphael Nabet, 2003

	TODO:
	* add support for tag data
	* add support for HFS write
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
	MFS (Macintosh File System): File system used by the early Macintosh.  This
		File system does not support folders (you may create folders on a MFS
		disk, but such folders are not implemented on File System level but in
		the Desktop file, and they are just a hint of how programs should list
		files, i.e. you can't have two files with the same name on a volume
		even if they are in two different folders), and it is not adequate for
		large volumes.
	HFS (Hierarchical File System): File system introduced with the HD20
		harddisk, the Macintosh Plus ROMs, and system 3.2 (IIRC).  Contrary to
		MFS, it supports hierarchical folders.  Also, it is suitable for larger
		volumes.
	HFS+ (HFS Plus): New file system introduced with MacOS 8.1.  It has a lot
		in common with HFS, but it supports more allocation blocks (up to 4
		billions IIRC), and many extra features, including longer file names
		(up to 255 UTF-16 Unicode chars).


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
#include <time.h>
#include <limits.h>
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
	mac_stricmp()

	Compare two macintosh strings in a manner compatible with the macintosh HFS
	file system.

	This functions emulates the way HFS (and MFS???) sorts string: this is
	equivalent to the RelString macintosh toolbox call with the caseSensitive
	parameter as false and the diacSensitive parameter as true.

	s1 (I): the string to compare
	s2 (I): the comparison string

	Return a zero if s1 and s2 are equal, a negative value if s1 is less than
	s2, and a positive value if s1 is greater than s2.

Known issues:
	Using this function makes sense with the MacRoman encoding, as it means the
	computer will regard "DeskTop File", "Desktop File", "Desktop file", etc,
	as the same file.  (UNIX users will probably regard this as an heresy, but
	you must consider that, unlike UNIX, the Macintosh was not designed for
	droids, but error-prone human beings that may forget about case.)

	(Also, letters with diatrical signs follow the corresponding letters
	without diacritical signs in the HFS catalog file.  This does not matter,
	though, since the Finder will not display files in the HFS catalog order,
	but it will instead sort files according to whatever order is currently
	selected in the View menu.)

	However, with other text encodings, the behavior will be completely absurd.
	For instance, with the Greek encoding, it will think that the degree symbol
	is the same letter (with different case) as the upper-case Psi, so that if
	a program asks for a file called "90°" on a greek HFS volume, and there is
	a file called "90[Psi]" on this volume, file "90[Psi]" will be opened.
	Results will probably be even weirder with 2-byte script like Japanese or
	Chinese.  Of course, we are not going to fix this issue, since doing so
	would break the compatibility with the original Macintosh OS.
*/

static int mac_stricmp(const UINT8 *s1, const UINT8 *s2)
{
	static const unsigned char mac_char_sort_table[256] =
	{
	/*	\x00	\x01	\x02	\x03	\x04	\x05	\x06	\x07 */
		0x00,	0x01,	0x02,	0x03,	0x04,	0x05,	0x06,	0x07,
	/*	\x08	\x09	\x0a	\x0b	\x0c	\x0d	\x0e	\x0f */
		0x08,	0x09,	0x0a,	0x0b,	0x0c,	0x0d,	0x0e,	0x0f,
	/*	\x10	\x11	\x12	\x13	\x14	\x15	\x16	\x17 */
		0x10,	0x11,	0x12,	0x13,	0x14,	0x15,	0x16,	0x17,
	/*	\x18	\x19	\x1a	\x1b	\x1c	\x1d	\x1e	\x1f */
		0x18,	0x19,	0x1a,	0x1b,	0x1c,	0x1d,	0x1e,	0x1f,
	/*	\x20	\x21	\x22	\x23	\x24	\x25	\x26	\x27 */
		0x20,	0x21,	0x22,	0x27,	0x28,	0x29,	0x2a,	0x2b,
	/*	\x28	\x29	\x2a	\x2b	\x2c	\x2d	\x2e	\x2f */
		0x2e,	0x2f,	0x30,	0x31,	0x32,	0x33,	0x34,	0x35,
	/*	\x30	\x31	\x32	\x33	\x34	\x35	\x36	\x37 */
		0x36,	0x37,	0x38,	0x39,	0x3a,	0x3b,	0x3c,	0x3d,
	/*	\x38	\x39	\x3a	\x3b	\x3c	\x3d	\x3e	\x3f */
		0x3e,	0x3f,	0x40,	0x41,	0x42,	0x43,	0x44,	0x45,
	/*	\x40	\x41	\x42	\x43	\x44	\x45	\x46	\x47 */
		0x46,	0x47,	0x51,	0x52,	0x54,	0x55,	0x5a,	0x5b,
	/*	\x48	\x49	\x4a	\x4b	\x4c	\x4d	\x4e	\x4f */
		0x5c,	0x5d,	0x62,	0x63,	0x64,	0x65,	0x66,	0x68,
	/*	\x50	\x51	\x52	\x53	\x54	\x55	\x56	\x57 */
		0x71,	0x72,	0x73,	0x74,	0x76,	0x77,	0x7c,	0x7d,
	/*	\x58	\x59	\x5a	\x5b	\x5c	\x5d	\x5e	\x5f */
		0x7e,	0x7f,	0x81,	0x82,	0x83,	0x84,	0x85,	0x86,
	/*	\x60	\x61	\x62	\x63	\x64	\x65	\x66	\x67 */
		0x4d,	0x47,	0x51,	0x52,	0x54,	0x55,	0x5a,	0x5b,
	/*	\x68	\x69	\x6a	\x6b	\x6c	\x6d	\x6e	\x6f */
		0x5c,	0x5d,	0x62,	0x63,	0x64,	0x65,	0x66,	0x68,
	/*	\x70	\x71	\x72	\x73	\x74	\x75	\x76	\x77 */
		0x71,	0x72,	0x73,	0x74,	0x76,	0x77,	0x7c,	0x7d,
	/*	\x78	\x79	\x7a	\x7b	\x7c	\x7d	\x7e	\x7f */
		0x7e,	0x7f,	0x81,	0x87,	0x88,	0x89,	0x8a,	0x8b,
	/*	\x80	\x81	\x82	\x83	\x84	\x85	\x86	\x87 */
		0x49,	0x4b,	0x53,	0x56,	0x67,	0x69,	0x78,	0x4e,
	/*	\x88	\x89	\x8a	\x8b	\x8c	\x8d	\x8e	\x8f */
		0x48,	0x4f,	0x49,	0x4a,	0x4b,	0x53,	0x56,	0x57,
	/*	\x90	\x91	\x92	\x93	\x94	\x95	\x96	\x97 */
		0x58,	0x59,	0x5e,	0x5f,	0x60,	0x61,	0x67,	0x6d,
	/*	\x98	\x99	\x9a	\x9b	\x9c	\x9d	\x9e	\x9f */
		0x6e,	0x6f,	0x69,	0x6a,	0x79,	0x7a,	0x7b,	0x78,
	/*	\xa0	\xa1	\xa2	\xa3	\xa4	\xa5	\xa6	\xa7 */
		0x8c,	0x8d,	0x8e,	0x8f,	0x90,	0x91,	0x92,	0x75,
	/*	\xa8	\xa9	\xaa	\xab	\xac	\xad	\xae	\xaf */
		0x93,	0x94,	0x95,	0x96,	0x97,	0x98,	0x4c,	0x6b,
	/*	\xb0	\xb1	\xb2	\xb3	\xb4	\xb5	\xb6	\xb7 */
		0x99,	0x9a,	0x9b,	0x9c,	0x9d,	0x9e,	0x9f,	0xa0,
	/*	\xb8	\xb9	\xba	\xbb	\xbc	\xbd	\xbe	\xbf */
		0xa1,	0xa2,	0xa3,	0x50,	0x70,	0xa4,	0x4c,	0x6b,
	/*	\xc0	\xc1	\xc2	\xc3	\xc4	\xc5	\xc6	\xc7 */
		0xa5,	0xa6,	0xa7,	0xa8,	0xa9,	0xaa,	0xab,	0x25,
	/*	\xc8	\xc9	\xca	\xcb	\xcc	\xcd	\xce	\xcf */
		0x26,	0xac,	0x20,	0x48,	0x4a,	0x6a,	0x6c,	0x6c,
	/*	\xd0	\xd1	\xd2	\xd3	\xd4	\xd5	\xd6	\xd7 */
		0xad,	0xae,	0x23,	0x24,	0x2c,	0x2d,	0xaf,	0xb0,
	/*	\xd8	\xd9	\xda	\xdb	\xdc	\xdd	\xde	\xdf */
		0x80,	0xb1,	0xb2,	0xb3,	0xb4,	0xb5,	0xb6,	0xb7,
	/*	\xe0	\xe1	\xe2	\xe3	\xe4	\xe5	\xe6	\xe7 */
		0xb8,	0xb9,	0xba,	0xbb,	0xbc,	0xbd,	0xbe,	0xbf,
	/*	\xe8	\xe9	\xea	\xeb	\xec	\xed	\xee	\xef */
		0xc0,	0xc1,	0xc2,	0xc3,	0xc4,	0xc5,	0xc6,	0xc7,
	/*	\xf0	\xf1	\xf2	\xf3	\xf4	\xf5	\xf6	\xf7 */
		0xc8,	0xc9,	0xca,	0xcb,	0xcc,	0xcd,	0xce,	0xcf,
	/*	\xf8	\xf9	\xfa	\xfb	\xfc	\xfd	\xfe	\xff */
		0xd0,	0xd1,	0xd2,	0xd3,	0xd4,	0xd5,	0xd6,	0xd7
	};

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
	mac_GetDateTime

	Returns macintosh clock, i.e. number of seconds elapsed since January 1st
	1904
*/
static UINT32 mac_GetDateTime(void)
{
	/* All these functions should be ANSI */
	time_t cur_time = time(NULL);
	struct tm base_expanded_time =
	{
		0,	/* seconds */
		0,	/* minutes */
		0,	/* hours */
		1,	/* day of the month */
		0,	/* months */
		4,	/* years after 1900 */
		0,	/* days after sunday - ignored */
		0,	/* days after january the 1st - ignored */
		-1	/* DST flag - we tell it is indeterminate cause it might be on in the southern hemisphere */
	};
	time_t base_time = mktime(&base_expanded_time);
	double diff_time = difftime(cur_time, base_time);

	/* we cast to UINT64 because the conversion from UINT64 to UINT32 will
	cause wrap-around, whereas direct conversion from double to UINT32 may
	cause overflow */
	return (UINT32) (UINT64) diff_time;
}

#if 0
#pragma mark -
#pragma mark MACBINARY SUPPORT
#endif

/*
	MacBinary format - encode data and ressource forks into a single file

	AppleSingle is much more flexible than MacBinary IMHO, but MacBinary is
	supported by many more utilities.

	There are 3 versions of MacBinary: MacBinary ("v1"), MacBinary II ("v2")
	and MacBinary III ("v3").  In addition, the comment field encoding was
	proposed as an extension to v1 well before v2 was introduced, so I will
	call MacBinary with the comment extension "v1.1".

	All three formats are backward- and forward-compatible with each other.
*/
/*
	MacBinary Header

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

	This piece of code was ripped from the MacBinary II source (and converted
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
	Low-level disk routines: the disk is implemented as a succession of
	512-byte blocks addressed by block index.
*/

/*
	header of diskcopy 4.2 images
*/
typedef struct diskcopy_header_t
{
	UINT8    diskName[64];	/* name of the disk */
	UINT32BE dataSize;		/* total size of data for all sectors (512*number_of_sectors) */
	UINT32BE tagSize;		/* total size of tag data for all sectors
								(12*number_of_sectors for GCR 3.5" floppies,
								20*number_of_sectors for HD20, 0 otherwise) */
	UINT32BE dataChecksum;	/* CRC32 checksum of all sector data */
	UINT32BE tagChecksum;	/* CRC32 checksum of all tag data */
	UINT8    diskFormat;	/* 0 = 400K, 1 = 800K, 2 = 720K, 3 = 1440K  (other values reserved) */
	UINT8    formatByte;	/* should be $00 Apple II, $01 Lisa, $02 Mac MFS ??? */
							/* $12 = 400K, $22 = >400K Macintosh (DiskCopy uses this value for
							   all Apple II disks not 800K in size, and even for some of those),
							   $24 = 800K Apple II disk */
	UINT16BE private;		/* always $0100 (otherwise, the file may be in a different format. */
} diskcopy_header_t;

/*
	disk image reference
*/
typedef struct mac_l1_imgref
{
	STREAM *f;					/* imgtool file reference */
	UINT32 num_blocks;			/* total number of 512-byte blocks */
	UINT32 tagbytesperblock;	/* number of tag bytes per block */
	UINT32 tag_offset;			/* file offset to tag data */
	enum { bare, apple_diskcopy } format;	/* disk image format */
	/*union
	{
		struct
		{
		} bare;
		struct
		{
		} apple_diskcopy;
	} u;*/
} mac_l1_imgref;

/*
	floppy_image_open()

	Open a macintosh disk image

	f (I): imgtool reference of file to open
	image (O): level-1 image reference

	Return imgtool error code
*/
static int floppy_image_open(STREAM *f, mac_l1_imgref *image)
{
	diskcopy_header_t header;

	image->f = f;

	image->format = bare;	/* default */

	/* read image header */
	if (stream_read(image->f, & header, sizeof(header)) == sizeof(header))
	{
		UINT32 dataSize = get_UINT32BE(header.dataSize);
		UINT32 tagSize = get_UINT32BE(header.tagSize);

		/* various checks to make sure it is diskcopy: */
		if ((header.diskName[0] <= 63)
				&& (stream_size(image->f) == (dataSize + tagSize + 84))
				&& (get_UINT16BE(header.private) == 0x0100))
		{
			image->format = apple_diskcopy;

			if (dataSize % 512)
				return IMGTOOLERR_CORRUPTIMAGE;
			image->num_blocks = dataSize/512;
			if (tagSize % image->num_blocks)
			{
				image->tagbytesperblock = 0;
				image->tag_offset = 0;
			}
			else
			{
				image->tagbytesperblock = tagSize / image->num_blocks;
				image->tag_offset = dataSize + 84;
			}
		}
	}

	if (image->format == bare)
	{
		long image_len = stream_size(image->f);

		if (image_len % 512)
			return IMGTOOLERR_CORRUPTIMAGE;
		image->num_blocks = image_len/512;
		image->tagbytesperblock = 0;
		image->tag_offset = 0;
	}

	return 0;
}

/*
	image_read_block()

	Read one 512-byte block of data from a macintosh disk image

	image (I/O): level-1 image reference
	block (I): address of block to read
	dest (O): buffer where block data should be stored

	Return imgtool error code
*/
static int image_read_block(mac_l1_imgref *image, UINT32 block, void *dest)
{
	if (block > image->num_blocks)
		return IMGTOOLERR_UNEXPECTED;

	if (stream_seek(image->f, (image->format == apple_diskcopy) ? block*512 + 84 : block*512, SEEK_SET))
		return IMGTOOLERR_READERROR;

	if (stream_read(image->f, dest, 512) != 512)
		return IMGTOOLERR_READERROR;

	return 0;
}

/*
	image_write_block()

	Read one 512-byte block of data from a macintosh disk image

	image (I/O): level-1 image reference
	block (I): address of block to write
	src (I): buffer with the block data

	Return imgtool error code
*/
static int image_write_block(mac_l1_imgref *image, UINT32 block, const void *src)
{
	if (block > image->num_blocks)
		return IMGTOOLERR_UNEXPECTED;

	if (stream_seek(image->f, (image->format == apple_diskcopy) ? block*512 + 84 : block*512, SEEK_SET))
		return IMGTOOLERR_WRITEERROR;

	if (stream_write(image->f, src, 512) != 512)
		return IMGTOOLERR_WRITEERROR;

	return 0;
}

/*
	image_get_tag_len()

	Get lenght of tag data (12 for GCR floppies, 20 for HD20, 0 otherwise)

	image (I/O): level-1 image reference

	Return tag lenght
*/
INLINE int image_get_tag_len(mac_l1_imgref *image)
{
	return image->tagbytesperblock;
}

/*
	image_read_tag()

	Read a 12- or 20-byte tag record associated with a disk block

	image (I/O): level-1 image reference
	block (I): address of block to read
	dest (O): buffer where tag data should be stored

	Return imgtool error code
*/
static int image_read_tag(mac_l1_imgref *image, UINT32 block, void *dest)
{
	if (block > image->num_blocks)
		return IMGTOOLERR_UNEXPECTED;

	/* check that there is tag data in the image */
	if (image->tagbytesperblock == 0)
		return IMGTOOLERR_UNEXPECTED;

	/* read tag data from disk image */
	if (stream_seek(image->f, image->tag_offset + block*image->tagbytesperblock, SEEK_SET))
		return IMGTOOLERR_READERROR;

	if (stream_read(image->f, dest, image->tagbytesperblock) != image->tagbytesperblock)
		return IMGTOOLERR_READERROR;

	return 0;
}

/*
	image_write_tag()

	Write a 12- or 20-byte tag record associated with a disk block

	image (I/O): level-1 image reference
	block (I): address of block to write
	src (I): buffer with the tag data

	Return imgtool error code
*/
static int image_write_tag(mac_l1_imgref *image, UINT32 block, const void *src)
{
	if (block > image->num_blocks)
		return IMGTOOLERR_UNEXPECTED;

	/* check that there is tag data in the image */
	if (image->tagbytesperblock == 0)
		return IMGTOOLERR_UNEXPECTED;

	/* read tag data from disk image */
	if (stream_seek(image->f, image->tag_offset + block*image->tagbytesperblock, SEEK_SET))
		return IMGTOOLERR_WRITEERROR;

	if (stream_write(image->f, src, image->tagbytesperblock) != image->tagbytesperblock)
		return IMGTOOLERR_WRITEERROR;

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

typedef enum { data_fork = 0x00, rsrc_fork = 0xff } mac_forkID;

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

	unsigned char ABlink_dirty[13];	/* dirty flag for each disk block in the ABlink array */
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
	hfs_extent_3 extents;			/* first 3 file extents */

	UINT32 parID;					/* CNID of parent directory (undefined for extent & catalog files) */
	mac_str31 fname;				/* file name (undefined for extent & catalog files) */
} hfs_fileref;

/*
	MFS/HFS open file ref
*/
typedef struct mac_fileref
{
	mac_l2_imgref *l2_img;			/* image pointer */

	UINT32 fileID;					/* file ID (a.k.a. CNID in HFS/HFS+) */

	mac_forkID forkType;			/* 0x00 for data, 0xff for resource */

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
	IMAGE base;

	mac_l1_imgref l1_img;

	UINT16 numABs;
	UINT16 blocksperAB;

	UINT16 freeABs;

	UINT32 nxtCNID;		/* nxtFNum in MFS, nxtCNID in HFS */

	mac_format format;
	union
	{
		mfs_l2_imgref mfs;
		hfs_l2_imgref hfs;
	} u;
};

/*
	MFS Master Directory Block
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
							the last in any file, the entry is 1; if an AB
							belongs to a file and is not the last one, the
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
	HFS Master Directory Block
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

	UINT32BE xtFlSize;		/* size (in bytes) of extents overflow file */
	hfs_extent_3 xtExtRec;	/* extent record for extents overflow file */
	UINT32BE ctFlSize;		/* size (in bytes) of catalog file */
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

/*
	Tag record for GCR floppies (12 bytes)

	And, no, I don't know the format of the 20-byte tag record of the HD20
*/
typedef struct floppy_tag_record
{
	UINT32BE fileID;			/* a.k.a. CNID */
								/* a value of 1 seems to be the default for non-AB blocks, but this is not consistent */
	UINT8 ftype;				/* bit 1 = 1 if resource fork */
								/* bit 0 = 1 if block is allocated??? */
								/* bit 7 seems to be used, but don't know what it means */
								/* a value of $FF seems to be the default for non-AB blocks, but this is not consistent */
	UINT8 fattr;				/* bit 0 = 1 if locked(?) */
								/* a value of $FF seems to be the default for non-AB blocks, but this is not consistent */
	UINT16BE fblock;			/* relative file block number (enough for any volume up to 32 MBytes in size) */
	UINT32BE wrCnt;				/* MFS: date and time of last write */
								/* HFS: seems related to the wrCnt field in the mdb, i.e.
									each time a volume is written to, the current value of
									wrCnt is written in the tag field, then it is incremented */
								/* (DV17 says "disk block number", but it cannot be true) */
} floppy_tag_record;

static int mfs_image_open(mac_l2_imgref *l2_img, img_open_buf *buf);
static int hfs_image_open(mac_l2_imgref *l2_img, img_open_buf *buf);
static void hfs_image_close(mac_l2_imgref *l2_img);
static int mfs_file_get_nth_block_address(mac_fileref *fileref, UINT32 block_num, UINT32 *block_address);
static int hfs_file_get_nth_block_address(mac_fileref *fileref, UINT32 block_num, UINT32 *block_address);
static int mfs_resolve_fpath(mac_l2_imgref *l2_img, const char *fpath, mac_str255 fname, mac_cat_info *cat_info, int create_it);
static int hfs_resolve_fpath(mac_l2_imgref *l2_img, const char *fpath, UINT32 *parID, mac_str255 fname, mac_cat_info *cat_info);
static int mfs_file_open(mac_l2_imgref *l2_img, const mac_str255 fname, mac_forkID fork, mac_fileref *fileref);
static int hfs_file_open(mac_l2_imgref *l2_img, UINT32 parID, const mac_str255 fname, mac_forkID fork, mac_fileref *fileref);
static int mfs_file_setABeof(mac_fileref *fileref, UINT32 newABeof);
static int mfs_dir_update(mac_fileref *fileref);

/*
	mac_image_open

	Open a macintosh image.  Image must already be open on level 1.

	l2_img (I/O): level-2 image reference

	Return imgtool error code
*/
static int mac_image_open(mac_l2_imgref *l2_img)
{
	img_open_buf buf;
	int errorcode;

	/* read MDB */
	errorcode = image_read_block(&l2_img->l1_img, 2, &buf.raw);
	if (errorcode)
		return errorcode;

	/* guess format from value in sigWord */
	switch ((buf.raw[0] << 8) | buf.raw[1])
	{
	case 0xd2d7:
		l2_img->format = L2I_MFS;
		return mfs_image_open(l2_img, &buf);

	case 0x4244:
		l2_img->format = L2I_HFS;
		return hfs_image_open(l2_img, &buf);

	case 0x484b:
		/* HFS+ is not supported */
		/* Note that we should normally not see this with HFS+ volumes, because
		HFS+ volumes are usually embedded in a HFS volume... */
		return IMGTOOLERR_UNIMPLEMENTED;
	}

	/* default case: sigWord has no legal value */
	return IMGTOOLERR_CORRUPTIMAGE;
}

/*
	mac_image_close

	Close a macintosh image.

	l2_img (I/O): level-2 image reference
*/
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

/*
	mac_resolve_fpath

	Resolve a file path, and translate it to a parID + fname pair that enables
	to do an efficient file search on a HFS volume (and an inefficient one on
	MFS, but it is not an issue as MFS volumes typically have a few dozens
	files, vs. possibly thousands with HFS volumes).

	l2_img (I/O): level-2 image reference
	fpath (I): file path (C string)
	parID (O): set to the CNID of the parent directory if the volume is in HFS
		format (reserved for MFS volumes)
	fname (O): set to the actual name of the file, with capitalization matching
		the one on the volume rather than the one in the fpath parameter (Mac
		string)
	cat_info (O): catalog info for this file extracted from the catalog file
		(may be NULL)

	Return imgtool error code
*/
static int mac_resolve_fpath(mac_l2_imgref *l2_img, const char *fpath, UINT32 *parID, mac_str255 fname, mac_cat_info *cat_info, int create_it)
{
	switch (l2_img->format)
	{
	case L2I_MFS:
		*parID = 0;
		return mfs_resolve_fpath(l2_img, fpath, fname, cat_info, create_it);

	case L2I_HFS:
		return hfs_resolve_fpath(l2_img, fpath, parID, fname, cat_info);
	}

	return IMGTOOLERR_UNEXPECTED;
}

/*
	mac_file_open

	Open a file located on a macintosh image

	l2_img (I/O): level-2 image reference
	parID (I): CNID of the parent directory if the volume is in HFS format
		(reserved for MFS volumes)
	fname (I): name of the file (Mac string)
	mac_forkID (I): tells which fork should be opened
	fileref (O): mac open file reference

	Return imgtool error code
*/
static int mac_file_open(mac_l2_imgref *l2_img, UINT32 parID, const mac_str255 fname, mac_forkID fork, mac_fileref *fileref)
{
	switch (l2_img->format)
	{
	case L2I_MFS:
		return mfs_file_open(l2_img, fname, fork, fileref);

	case L2I_HFS:
		return hfs_file_open(l2_img, parID, fname, fork, fileref);
	}

	return IMGTOOLERR_UNEXPECTED;
}

/*
	mac_file_read

	Read data from an open mac file, starting at current position in file

	l2_img (I/O): level-2 image reference
	len (I): number of bytes to read
	dest (O): destination buffer

	Return imgtool error code
*/
static int mac_file_read(mac_fileref *fileref, UINT32 len, void *dest)
{
	UINT32 block;
	int errorcode = 0;
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
	mac_file_write

	Write data to an open mac file, starting at current position in file

	l2_img (I/O): level-2 image reference
	len (I): number of bytes to read
	dest (O): destination buffer

	Return imgtool error code
*/
static int mac_file_write(mac_fileref *fileref, UINT32 len, const void *src)
{
	UINT32 block;
	int errorcode = 0;
	int run_len;

	if ((fileref->crPs + len) > fileref->eof)
		/* EOF */
		return IMGTOOLERR_UNEXPECTED;

	while (len > 0)
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
		if (fileref->reload_buf)
		{
			errorcode = image_read_block(&fileref->l2_img->l1_img, block, fileref->block_buffer);
			if (errorcode)
				return errorcode;
			fileref->reload_buf = FALSE;
		}
		run_len = 512 - (fileref->crPs % 512);
		if (run_len > len)
			run_len = len;

		memcpy(fileref->block_buffer+(fileref->crPs % 512), src, run_len);
		errorcode = image_write_block(&fileref->l2_img->l1_img, block, fileref->block_buffer);
		if (errorcode)
			return errorcode;
		len -= run_len;
		src = (const UINT8 *)src + run_len;
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

/*
	mac_file_seteof

	Set file EOF
*/
static int mac_file_seteof(mac_fileref *fileref, UINT32 newEof)
{
	UINT32 newABEof;
	int errorcode = 0;

	newABEof = (newEof + fileref->l2_img->blocksperAB * 512 - 1) / (fileref->l2_img->blocksperAB * 512);

	/*if (fileref->pLen % (fileref->l2_img->blocksperAB * 512))
		return IMGTOOLERR_CORRUPTIMAGE;*/

	if (newEof < fileref->eof)
		fileref->eof = newEof;

	switch (fileref->l2_img->format)
	{
	case L2I_MFS:
		errorcode = mfs_file_setABeof(fileref, newABEof);
		break;

	case L2I_HFS:
		errorcode = IMGTOOLERR_UNIMPLEMENTED;
		break;
	}
	if (errorcode)
		return errorcode;

	fileref->eof = newEof;

	errorcode = mfs_dir_update(fileref);
	if (errorcode)
		return errorcode;

	/* update current pos if beyond new EOF */
	/*if (fileref->crPs > newEof)
	{
		if ((fileref->crPs / 512) != (newEof / 512))
			fileref->reload_buf = TRUE;

		fileref->crPs = newEof;
	}*/

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

	Note that the directory does not seem to be sorted: the order in which
	files appear does not match file names, and it does not always match file
	IDs.
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

	In typical Apple manner, this ressource is not documented.  However, I have
	managed to reverse engineer some parts of it.
*/
typedef struct mfs_FOBJ
{
	UINT8 unknown0[2];		/* $00: $0004 for disk, $0008 for folder??? */
	mac_point location;		/* $02: location in parent window */
	UINT8 unknown1[4];		/* $06: ??? */
	UINT8 view;				/* $0A: manner in which folders are displayed??? */
	UINT8 unknown2;			/* $0B: ??? */
	UINT16BE par_fldr;		/* $0C: parent folder ID */
	UINT8 unknown3[10];		/* $0E: ??? */
	UINT16BE unknown4;		/* $18: ??? */
	UINT32BE createDate;	/* $1A: date and time of creation */
	UINT32BE modifyDate;	/* $1E: date and time of last modification */
	UINT16BE unknown5;		/* $22: put-away folder ID?????? */
	UINT8 unknown6[8];		/* $24: ??? */
	mac_rect bounds;		/* $2C: window bounds */
	mac_point scroll;		/* $34: current scroll offset??? */
	union
	{	/* I think there are two versions of the structure */
		struct
		{
			UINT16BE item_count;	/* number of items (folders and files) in
										this folder */
			UINT32BE item_descs[1];	/* this variable-lenght array has
										item_count entries - meaning of entry is unknown */
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

	Open a MFS image.  Image must already be open on level 1.  This function
	should not be called directly: call mac_image_open() instead.

	l2_img (I/O): level-2 image reference (l1_img and format fields must be
		initialized)
	img_open_buf (I): buffer with the MDB block

	Return imgtool error code
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

	l2_img->nxtCNID = get_UINT32BE(buf->mfs_mdb.nxtFNum);

	l2_img->freeABs = get_UINT16BE(buf->mfs_mdb.freeABs);

	mac_strcpy(l2_img->u.mfs.volname, buf->mfs_mdb.VN);

	if (l2_img->numABs > 4094)
		return IMGTOOLERR_CORRUPTIMAGE;

	/* extract link array */
	{
		int byte_len = l2_img->numABs + ((l2_img->numABs + 1) >> 1);
		int cur_byte;
		int cur_block;
		int block_len = sizeof(buf->mfs_mdb.ABlink);

		/* clear dirty flags */
		for (cur_block=0; cur_block<13; cur_block++)
			l2_img->u.mfs.ABlink_dirty[cur_block] = 0;

		/* append the chunk after MDB to link array */
		cur_byte = 0;
		if (block_len > (byte_len - cur_byte))
			block_len = byte_len - cur_byte;
		memcpy(l2_img->u.mfs.ABlink+cur_byte, buf->mfs_mdb.ABlink, block_len);
		cur_byte += block_len;
		cur_block = 2;
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
	mfs_update_mdb

	Update MDB on disk

	l2_img (I/O): level-2 image reference

	Return imgtool error code
*/
static int mfs_update_mdb(mac_l2_imgref *l2_img)
{
	int errorcode;
	union
	{
		mfs_mdb mfs_mdb;
		UINT8 raw[512];
	} buf;

	assert(l2_img->format == L2I_MFS);

	/* read MDB */
	errorcode = image_read_block(&l2_img->l1_img, 2, &buf.mfs_mdb);
	if (errorcode)
		return errorcode;

	set_UINT16BE(&buf.mfs_mdb.nmFls, l2_img->u.mfs.dir_num_files);
#if 0	/* these fields are never changed */
	set_UINT16BE(&buf.mfs_mdb.dirSt, l2_img->u.mfs.dir_start);
	set_UINT16BE(&buf.mfs_mdb.blLn, l2_img->u.mfs.dir_blk_len);

	set_UINT16BE(&buf.mfs_mdb.nmAlBlks, l2_img->numABs);
	set_UINT32BE(&buf.mfs_mdb.alBlkSiz, l2_img->blocksperAB*512);
	set_UINT16BE(&buf.mfs_mdb.alBlSt, l2_img->u.mfs.ABStart);
#endif

	set_UINT32BE(&buf.mfs_mdb.nxtFNum, l2_img->nxtCNID);

	set_UINT16BE(&buf.mfs_mdb.freeABs, l2_img->freeABs);

#if 0	/* these fields are never changed */
	mac_strcpy(buf.mfs_mdb.VN, l2_img->u.mfs.volname);
#endif

	/* save link array */
	{
		int byte_len = l2_img->numABs + ((l2_img->numABs + 1) >> 1);
		int cur_byte = 0;
		int cur_block = 2;
		int block_len = sizeof(buf.mfs_mdb.ABlink);

		/* update the chunk of link array after the MDB */
		if (block_len > (byte_len - cur_byte))
			block_len = byte_len - cur_byte;
		memcpy(buf.mfs_mdb.ABlink, l2_img->u.mfs.ABlink+cur_byte, block_len);
		cur_byte += block_len;

		if (block_len < sizeof(buf.mfs_mdb.ABlink))
			memset(buf.mfs_mdb.ABlink+block_len, 0, sizeof(buf.mfs_mdb.ABlink)-block_len);

		l2_img->u.mfs.ABlink_dirty[0] = 0;

		/* write back modified MDB+link */
		errorcode = image_write_block(&l2_img->l1_img, 2, &buf.mfs_mdb);
		if (errorcode)
			return errorcode;

		while (cur_byte < byte_len)
		{
			/* advance to next block */
			cur_block++;
			block_len = 512;

			/* extract the current chunk of link array */
			if (block_len > (byte_len - cur_byte))
				block_len = byte_len - cur_byte;

			if (l2_img->u.mfs.ABlink_dirty[cur_block-2])
			{
				memcpy(buf.raw, l2_img->u.mfs.ABlink+cur_byte, block_len);
				if (block_len < 512)
					memset(buf.raw+block_len, 0, 512-block_len);
				/* write back link array */
				errorcode = image_write_block(&l2_img->l1_img, cur_block, buf.raw);
				if (errorcode)
					return errorcode;
				l2_img->u.mfs.ABlink_dirty[cur_block-2] = 0;
			}

			cur_byte += block_len;
		}
	}

	return 0;
}

/*
	mfs_dir_open

	Open the directory file

	l2_img (I/O): level-2 image reference
	dirref (O): open directory file reference

	Return imgtool error code
*/
static int mfs_dir_open(mac_l2_imgref *l2_img, mfs_dirref *dirref)
{
	int errorcode;


	assert(l2_img->format == L2I_MFS);

	dirref->l2_img = l2_img;
	dirref->index = 0;

	dirref->cur_block = 0;
	dirref->cur_offset = 0;
	errorcode = image_read_block(&l2_img->l1_img, l2_img->u.mfs.dir_start + dirref->cur_block, dirref->block_buffer);
	if (errorcode)
		return errorcode;

	return 0;
}

/*
	mfs_dir_read

	Read one entry of directory file

	dirref (I/O): open directory file reference
	dir_entry (O): set to point to the entry read: set to NULL if EOF or error

	Return imgtool error code
*/
static int mfs_dir_read(mfs_dirref *dirref, mfs_dir_entry **dir_entry)
{
	mfs_dir_entry *cur_dir_entry;
	size_t cur_dir_entry_len;
	int errorcode;


	if (dir_entry)
		*dir_entry = NULL;

	if (dirref->index == dirref->l2_img->u.mfs.dir_num_files)
		/* EOF */
		return 0;

	/* get cat entry pointer */
	cur_dir_entry = (mfs_dir_entry *) (dirref->block_buffer + dirref->cur_offset);
	while ((dirref->cur_offset == 512) || ! (cur_dir_entry->flags & 0x80))
	{
		dirref->cur_block++;
		dirref->cur_offset = 0;
		if (dirref->cur_block > dirref->l2_img->u.mfs.dir_blk_len)
			/* aargh! */
			return IMGTOOLERR_CORRUPTIMAGE;
		errorcode = image_read_block(&dirref->l2_img->l1_img, dirref->l2_img->u.mfs.dir_start + dirref->cur_block, dirref->block_buffer);
		if (errorcode)
			return errorcode;
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
	mfs_dir_insert

	Add an entry in the directory file

	Return imgtool error code
*/
static int mfs_dir_insert(mac_l2_imgref *l2_img, mfs_dirref *dirref, const UINT8 *new_fname, mfs_dir_entry **dir_entry)
{
	size_t new_dir_entry_len;
	mfs_dir_entry *cur_dir_entry;
	size_t cur_dir_entry_len;
	UINT32 cur_date;
	int errorcode;

	dirref->l2_img = l2_img;
	dirref->index = 0;

	new_dir_entry_len = offsetof(mfs_dir_entry, name) + new_fname[0] + 1;

	for (dirref->cur_block = 0; dirref->cur_block < dirref->l2_img->u.mfs.dir_blk_len; dirref->cur_block++)
	{
		/* read current block */
		errorcode = image_read_block(&dirref->l2_img->l1_img, dirref->l2_img->u.mfs.dir_start + dirref->cur_block, dirref->block_buffer);
		if (errorcode)
			return errorcode;

		/* get free chunk in this block */
		dirref->cur_offset = 0;
		cur_dir_entry = (mfs_dir_entry *) (dirref->block_buffer + dirref->cur_offset);
		while ((dirref->cur_offset < 512) && (cur_dir_entry->flags & 0x80))
		{	/* skip cur_dir_entry */
			cur_dir_entry_len = offsetof(mfs_dir_entry, name) + cur_dir_entry->name[0] + 1;
			/* update offset in block */
			dirref->cur_offset += cur_dir_entry_len;
			/* align to word boundary */
			dirref->cur_offset = (dirref->cur_offset + 1) & ~1;
			/* update entry pointer */
			cur_dir_entry = (mfs_dir_entry *) (dirref->block_buffer + dirref->cur_offset);
			/* update file index (useless, but can't harm) */
			dirref->index++;
		}

		if (new_dir_entry_len <= (/*512*/510 - dirref->cur_offset))
		{
			/*memcpy(cur_dir_entry, new_dir_entry, new_dir_entry_len);*/
			cur_dir_entry->flags = 0x80;
			cur_dir_entry->flVersNum = 0x00;
			memset(&cur_dir_entry->flFinderInfo, 0, sizeof(cur_dir_entry->flFinderInfo));
			set_UINT32BE(&cur_dir_entry->fileID, dirref->l2_img->nxtCNID++);
			set_UINT16BE(&cur_dir_entry->dataStartBlock, 1);
			set_UINT32BE(&cur_dir_entry->dataLogicalSize, 0);
			set_UINT32BE(&cur_dir_entry->dataPhysicalSize, 0);
			set_UINT16BE(&cur_dir_entry->rsrcStartBlock, 1);
			set_UINT32BE(&cur_dir_entry->rsrcLogicalSize, 0);
			set_UINT32BE(&cur_dir_entry->rsrcPhysicalSize, 0);
			cur_date = mac_GetDateTime();
			set_UINT32BE(&cur_dir_entry->createDate, cur_date);
			set_UINT32BE(&cur_dir_entry->modifyDate, cur_date);
			mac_strcpy(cur_dir_entry->name, new_fname);

			/* update offset in block */
			dirref->cur_offset += new_dir_entry_len;
			/* align to word boundary */
			dirref->cur_offset = (dirref->cur_offset + 1) & ~1;
			if (dirref->cur_offset < 512)
				/* mark remaining space as free record */
				dirref->block_buffer[dirref->cur_offset] = 0;
			/* write back directory */
			errorcode = image_write_block(&dirref->l2_img->l1_img, dirref->l2_img->u.mfs.dir_start + dirref->cur_block, dirref->block_buffer);
			if (errorcode)
				return errorcode;
			/* update file count */
			dirref->l2_img->u.mfs.dir_num_files++;

			/* update MDB (nxtCNID & dir_num_files fields) */
			errorcode = mfs_update_mdb(dirref->l2_img);
			if (errorcode)
				return errorcode;

			if (dir_entry)
				*dir_entry = cur_dir_entry;
			return 0;
		}
	}

	return IMGTOOLERR_NOSPACE;
}

/*
	mfs_dir_update

	Update one entry of directory file

	fileref (I/O): open file reference

	Return imgtool error code
*/
static int mfs_dir_update(mac_fileref *fileref)
{
	UINT16 cur_block;
	UINT16 cur_offset;
	UINT8 block_buffer[512];
	mfs_dir_entry *cur_dir_entry;
	size_t cur_dir_entry_len;
	int errorcode;

	for (cur_block = 0; cur_block < fileref->l2_img->u.mfs.dir_blk_len; cur_block++)
	{
		/* read current block */
		errorcode = image_read_block(&fileref->l2_img->l1_img, fileref->l2_img->u.mfs.dir_start + cur_block, block_buffer);
		if (errorcode)
			return errorcode;

		/* get free chunk in this block */
		cur_offset = 0;
		cur_dir_entry = (mfs_dir_entry *) (block_buffer + cur_offset);
		while ((cur_offset < 512) && (cur_dir_entry->flags & 0x80))
		{
			if (get_UINT32BE(cur_dir_entry->fileID) == fileref->fileID)
			{	/* found it: update directory entry */
				switch (fileref->forkType)
				{
				case data_fork:
					set_UINT16BE(&cur_dir_entry->dataStartBlock, fileref->u.mfs.stBlk);
					set_UINT32BE(&cur_dir_entry->dataLogicalSize, fileref->eof);
					set_UINT32BE(&cur_dir_entry->dataPhysicalSize, fileref->pLen);
					break;

				case rsrc_fork:
					set_UINT16BE(&cur_dir_entry->rsrcStartBlock, fileref->u.mfs.stBlk);
					set_UINT32BE(&cur_dir_entry->rsrcLogicalSize, fileref->eof);
					set_UINT32BE(&cur_dir_entry->rsrcPhysicalSize, fileref->pLen);
					break;
				}
				/* write back directory */
				errorcode = image_write_block(&fileref->l2_img->l1_img, fileref->l2_img->u.mfs.dir_start + cur_block, block_buffer);
				if (errorcode)
					return errorcode;

				return 0;
			}
			/* skip cur_dir_entry */
			cur_dir_entry_len = offsetof(mfs_dir_entry, name) + cur_dir_entry->name[0] + 1;
			/* update offset in block */
			cur_offset += cur_dir_entry_len;
			/* align to word boundary */
			cur_offset = (cur_offset + 1) & ~1;
			/* update entry pointer */
			cur_dir_entry = (mfs_dir_entry *) (block_buffer + cur_offset);
		}
	}

	return IMGTOOLERR_UNEXPECTED;
}


/*
	mfs_find_dir_entry

	Find a file in an MFS directory

	dirref (I/O): open directory file reference
	fname (I): file name (Mac string)
	dir_entry (O): set to point to the entry read: set to NULL if EOF or error

	Return imgtool error code
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
		errorcode = mfs_dir_read(dirref, &cur_dir_entry);
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
	mfs_resolve_fpath

	Resolve a file path for MFS volumes.  This function should not be called
	directly: call mac_resolve_fpath instead.

	l2_img (I/O): level-2 image reference
	fpath (I): file path (C string)
	fname (O): set to the actual name of the file, with capitalization matching
		the one on the volume rather than the one in the fpath parameter (Mac
		string)
	cat_info (I/O): on output, catalog info for this file extracted from the
		catalog file (may be NULL)
		If create_it is TRUE, created info will first be set according to the
		data from cat_info
	create_it (I): TRUE if entry should be created if not found

	Return imgtool error code
*/
static int mfs_resolve_fpath(mac_l2_imgref *l2_img, const char *fpath, mac_str255 fname, mac_cat_info *cat_info, int create_it)
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
	if ((errorcode == IMGTOOLERR_FILENOTFOUND) && create_it)
		errorcode = mfs_dir_insert(l2_img, &dirref, fname, &dir_entry);
	if (errorcode)
		return errorcode;

	mac_strcpy(fname, dir_entry->name);

	if (create_it && cat_info)
	{
		dir_entry->flFinderInfo = cat_info->flFinderInfo;
		dir_entry->flags = (dir_entry->flags & 0x80) | (cat_info->flags & 0x7f);
		set_UINT32BE(&dir_entry->createDate, cat_info->createDate);
		set_UINT32BE(&dir_entry->modifyDate, cat_info->modifyDate);

		/* write current directory block */
		errorcode = image_write_block(&l2_img->l1_img, l2_img->u.mfs.dir_start + dirref.cur_block, dirref.block_buffer);
		if (errorcode)
			return errorcode;
	}

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
static int mfs_file_open_internal(mac_l2_imgref *l2_img, const mfs_dir_entry *dir_entry, mac_forkID fork, mac_fileref *fileref)
{
	assert(l2_img->format == L2I_MFS);

	fileref->l2_img = l2_img;

	fileref->fileID = get_UINT32BE(dir_entry->fileID);
	fileref->forkType = fork;

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

/*
	mfs_file_open

	Open a file located on a MFS volume.  This function should not be called
	directly: call mac_file_open instead.

	l2_img (I/O): level-2 image reference
	fname (I): name of the file (Mac string)
	mac_forkID (I): tells which fork should be opened
	fileref (O): mac open file reference

	Return imgtool error code
*/
static int mfs_file_open(mac_l2_imgref *l2_img, const mac_str255 fname, mac_forkID fork, mac_fileref *fileref)
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
	return mfs_file_open_internal(l2_img, dir_entry, fork, fileref);
}

/*
	mfs_get_ABlink

	Read one entry of the Allocation Bitmap link array, on an MFS volume.

	l2_img (I/O): level-2 image reference
	AB_address (I): index in the array, which is an AB address

	Returns the 12-bit value read in array.
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
	mfs_set_ABlink

	Set one entry of the Allocation Bitmap link array, on an MFS volume.

	l2_img (I/O): level-2 image reference
	AB_address (I): index in the array, which is an AB address
	data (I): 12-bit value to write in array
*/
static void mfs_set_ABlink(mac_l2_imgref *l2_img, UINT16 AB_address, UINT16 data)
{
	int base;

	assert(l2_img->format == L2I_MFS);

	base = (AB_address >> 1) * 3;

	if (! (AB_address & 1))
	{
		l2_img->u.mfs.ABlink[base] = (data >> 4) & 0xff;
		l2_img->u.mfs.ABlink[base+1] = (l2_img->u.mfs.ABlink[base+1] & 0x0f) | ((data << 4) & 0xf0);

		l2_img->u.mfs.ABlink_dirty[(base+64)/512] = 1;
		l2_img->u.mfs.ABlink_dirty[(base+1+64)/512] = 1;
	}
	else
	{
		l2_img->u.mfs.ABlink[base+1] = (l2_img->u.mfs.ABlink[base+1] & 0xf0) | ((data >> 8) & 0x0f);
		l2_img->u.mfs.ABlink[base+2] = data & 0xff;

		l2_img->u.mfs.ABlink_dirty[(base+1+64)/512] = 1;
		l2_img->u.mfs.ABlink_dirty[(base+2+64)/512] = 1;
	}
}

/*
	mfs_file_get_nth_block_address

	Get the disk block address of a given block in an open file on a MFS image.
	Called by macintosh file code.

	fileref (I/O): mac open file reference
	block_num (I): file block index
	block_address (O): disk block address for the file block

	Return imgtool error code
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
	mfs_file_allocABs

	Allocate a chunk of ABs
*/
static int mfs_file_allocABs(mac_fileref *fileref, UINT16 lastAB, UINT32 allocABs)
{
	int numABs = fileref->l2_img->numABs;
	int free_ABs;
	int i;
	int extentBaseAB, extentABlen;
	int firstBestExtentBaseAB = 0, firstBestExtentABlen;
	int secondBestExtentBaseAB = 0, secondBestExtentABlen;

	/* return if done */
	if (! allocABs)
		return 0;

	/* compute free space */
	free_ABs = 0;
	for (i=0; i<numABs; i++)
	{
		if (mfs_get_ABlink(fileref->l2_img, i) == 0)
			free_ABs++;
	}

	/* check we have enough free space */
	if (free_ABs < allocABs)
		return IMGTOOLERR_NOSPACE;

	if (fileref->u.mfs.stBlk != 1)
	{	/* try to extend last file extent */
		/* append free ABs after last AB */
		for (i=lastAB+1; (mfs_get_ABlink(fileref->l2_img, i) == 0) && (allocABs > 0) && (i < numABs); i++)
		{
			mfs_set_ABlink(fileref->l2_img, lastAB, i+2);
			lastAB = i;
			allocABs--;
			free_ABs--;
		}
		/* return if done */
		if (! allocABs)
		{
			mfs_set_ABlink(fileref->l2_img, lastAB, 1);
			fileref->l2_img->freeABs = free_ABs;
			return 0;	/* done */
		}
	}

	while (allocABs)
	{
		/* find smallest data block at least nb_alloc_physrecs in lenght, and largest data block less than nb_alloc_physrecs in lenght */
		firstBestExtentABlen = INT_MAX;
		secondBestExtentABlen = 0;
		for (i=0; i<numABs; i++)
		{
			if (mfs_get_ABlink(fileref->l2_img, i) == 0)
			{	/* found one free block */
				/* compute its lenght */
				extentBaseAB = i;
				extentABlen = 0;
				while ((i<numABs) && (mfs_get_ABlink(fileref->l2_img, i) == 0))
				{
					extentABlen++;
					i++;
				}
				/* compare to previous best and second-best blocks */
				if ((extentABlen < firstBestExtentABlen) && (extentABlen >= allocABs))
				{
					firstBestExtentBaseAB = extentBaseAB;
					firstBestExtentABlen = extentABlen;
					if (extentABlen == allocABs)
						/* no need to search further */
						break;
				}
				else if ((extentABlen > secondBestExtentABlen) && (extentABlen < allocABs))
				{
					secondBestExtentBaseAB = extentBaseAB;
					secondBestExtentABlen = extentABlen;
				}
			}
		}

		if (firstBestExtentABlen != INT_MAX)
		{	/* found one contiguous block which can hold it all */
			extentABlen = allocABs;
			for (i=0; i<allocABs; i++)
			{
				if (fileref->u.mfs.stBlk != 1)
					mfs_set_ABlink(fileref->l2_img, lastAB, firstBestExtentBaseAB+i+2);
				else
					fileref->u.mfs.stBlk = firstBestExtentBaseAB+i+2;
				lastAB = firstBestExtentBaseAB+i;
			}
			free_ABs -= allocABs;
			allocABs = 0;
			mfs_set_ABlink(fileref->l2_img, lastAB, 1);
			fileref->l2_img->freeABs = free_ABs;
			/*return 0;*/	/* done */
		}
		else if (secondBestExtentABlen != 0)
		{	/* jeez, we need to fragment it.  We use the largest smaller block to limit fragmentation. */
			for (i=0; i<secondBestExtentABlen; i++)
			{
				if (fileref->u.mfs.stBlk != 1)
					mfs_set_ABlink(fileref->l2_img, lastAB, secondBestExtentBaseAB+i+2);
				else
					fileref->u.mfs.stBlk = secondBestExtentBaseAB+i+2;
				lastAB = secondBestExtentBaseAB+i;
			}
			free_ABs -= secondBestExtentABlen;
			allocABs -= secondBestExtentABlen;
		}
		else
		{
			mfs_set_ABlink(fileref->l2_img, lastAB, 1);
			return IMGTOOLERR_NOSPACE;	/* This should never happen, as we pre-check that there is enough free space */
		}
	}

	return 0;
}

/*
	mfs_file_setABeof

	Set physical file EOF in ABs
*/
static int mfs_file_setABeof(mac_fileref *fileref, UINT32 newABeof)
{
	UINT16 AB_address = 0;
	UINT16 AB_link;
	int i;
	int errorcode = 0;
	int MDB_dirty = 0;


	assert(fileref->l2_img->format == L2I_MFS);

	/* run through link chain until we reach the old or the new EOF */
	AB_link = fileref->u.mfs.stBlk;
	if ((AB_link == 0) || (AB_link >= fileref->l2_img->numABs+2))
		/* 0 -> ??? */
		return IMGTOOLERR_CORRUPTIMAGE;
	for (i=0; (i<newABeof) && (AB_link!=1); i++)
	{
		AB_address = AB_link - 2;
		AB_link = mfs_get_ABlink(fileref->l2_img, AB_address);
		if ((AB_link == 0) || (AB_link >= fileref->l2_img->numABs+2))
			/* 0 -> empty block: there is no way an empty block could make it
			into the link chain!!! */
			return IMGTOOLERR_CORRUPTIMAGE;
	}

	if (i == newABeof)
	{	/* new EOF is shorter than old one */
		/* mark new eof */
		if (i==0)
			fileref->u.mfs.stBlk = 1;
		else
		{
			mfs_set_ABlink(fileref->l2_img, AB_address, 1);
			MDB_dirty = 1;
		}

		/* free all remaining blocks */
		while (AB_link != 1)
		{
			AB_address = AB_link - 2;
			AB_link = mfs_get_ABlink(fileref->l2_img, AB_address);
			if ((AB_link == 0) || (AB_link >= fileref->l2_img->numABs+2))
			{	/* 0 -> empty block: there is no way an empty block could make
				it into the link chain!!! */
				if (MDB_dirty)
				{	/* update MDB (freeABs field) and ABLink array */
					errorcode = mfs_update_mdb(fileref->l2_img);
					if (errorcode)
						return errorcode;
				}
				return IMGTOOLERR_CORRUPTIMAGE;
			}
			mfs_set_ABlink(fileref->l2_img, AB_address, 0);
			fileref->l2_img->freeABs++;
			MDB_dirty = 1;
		}
	}
	else
	{	/* new EOF is larger than old one */
		errorcode = mfs_file_allocABs(fileref, AB_address, newABeof - i);
		if (errorcode)
			return errorcode;
		MDB_dirty = 1;
	}

	if (MDB_dirty)
	{	/* update MDB (freeABs field) and ABLink array */
		errorcode = mfs_update_mdb(fileref->l2_img);
		if (errorcode)
			return errorcode;
	}

	fileref->pLen = newABeof * (fileref->l2_img->blocksperAB * 512);

	return 0;
}

/*
	mfs_hashString

	Hash string to get the resource ID of comment (FCMT ressource type).

	Ripped from Apple technote TB06 (converted from 68k ASM to C)

	string (I): string to hash

	Returns hash value
*/
static int mfs_hashString(const mac_str255 string)
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
	UINT16BE recordType;		/* record type */
	UINT32BE reserved[2];		/* reserved - set to zero */
	UINT32BE parID;				/* parent ID for this catalog node */
	mac_str31 nodeName;			/* name of this catalog node */
} hfs_catThreadData;

/*
	union for all types at once
*/
typedef union hfs_catData
{
	UINT16BE dataType;
	hfs_catFolderData folder;
	hfs_catFileData file;
	hfs_catThreadData thread;
} hfs_catData;

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

	This is similar to the MFS catalog flag field, but the "thread exists" flag
	(0x02) is specific to HFS/HFS+, whereas the "Record in use" flag (0x80) is
	only used by MFS.
*/
enum
{
	cfrf_fileLocked		= 0x01,		/* file is locked and cannot be written to */
	cfrf_threadExists	= 0x02		/* a file thread record exists for this file */
};

/*
	BT functions used by HFS functions
*/
typedef struct BT_leaf_rec_enumerator
{
	mac_BTref *BTref;
	UINT32 cur_node;
	int cur_rec;
} BT_leaf_rec_enumerator;

static int BT_open(mac_BTref *BTref, int (*key_compare_func)(const void *key1, const void *key2));
static void BT_close(mac_BTref *BTref);
static int BT_search_leaf_rec(mac_BTref *BTref, void *search_key,
								UINT32 *node_ID, int *record_ID,
								void **record_ptr, int *record_len,
								int search_exact_match, int *match_found);
static int BT_get_keyed_record_data(mac_BTref *BTref, void *rec_ptr, int rec_len, void **data_ptr, int *data_len);
static int BT_leaf_rec_enumerator_open(mac_BTref *BTref, BT_leaf_rec_enumerator *enumerator);
static int BT_leaf_rec_enumerator_read(BT_leaf_rec_enumerator *enumerator, void **record_ptr, int *rec_len);

typedef struct hfs_cat_enumerator
{
	mac_l2_imgref *l2_img;
	BT_leaf_rec_enumerator BT_enumerator;
} hfs_cat_enumerator;

/*
	hfs_open_extents_file

	Open the file extents B-tree file
*/
static int hfs_open_extents_file(mac_l2_imgref *l2_img, const hfs_mdb *mdb, mac_fileref *fileref)
{
	assert(l2_img->format == L2I_HFS);

	fileref->l2_img = l2_img;

	fileref->fileID = 3;
	fileref->forkType = 0x00;

	fileref->eof = fileref->pLen = get_UINT32BE(mdb->xtFlSize);
	memcpy(fileref->u.hfs.extents, mdb->xtExtRec, sizeof(hfs_extent_3));

	fileref->crPs = 0;

	fileref->reload_buf = TRUE;

	return 0;
}

/*
	hfs_open_cat_file

	Open the disk catalog B-tree file
*/
static int hfs_open_cat_file(mac_l2_imgref *l2_img, const hfs_mdb *mdb, mac_fileref *fileref)
{
	assert(l2_img->format == L2I_HFS);

	fileref->l2_img = l2_img;

	fileref->fileID = 4;
	fileref->forkType = 0x00;

	fileref->eof = fileref->pLen = get_UINT32BE(mdb->ctFlSize);
	memcpy(fileref->u.hfs.extents, mdb->ctExtRec, sizeof(hfs_extent_3));

	fileref->crPs = 0;

	fileref->reload_buf = TRUE;

	return 0;
}

/*
	hfs_extentKey_compare

	key compare function for file extents B-tree
*/
static int hfs_extentKey_compare(const void *p1, const void *p2)
{
	const hfs_extentKey *key1 = p1;
	const hfs_extentKey *key2 = p2;

	/* let's keep it simple for now */
	return memcmp(key1, key2, sizeof(hfs_extentKey));
}

/*
	hfs_catKey_compare

	key compare function for disk catalog B-tree
*/
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

	l2_img->nxtCNID = get_UINT32BE(buf->hfs_mdb.nxtCNID);

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

/*
	hfs_image_close

	Close a HFS image.
*/
static void hfs_image_close(mac_l2_imgref *l2_img)
{
	assert(l2_img->format == L2I_HFS);

	BT_close(&l2_img->u.hfs.extents_BT);
	BT_close(&l2_img->u.hfs.cat_BT);
}

/*
	hfs_get_cat_record_data

	extract data from a catalog B-tree record
*/
static int hfs_get_cat_record_data(mac_l2_imgref *l2_img, void *rec_raw, int rec_len, hfs_catKey **rec_key, hfs_catData **rec_data)
{
	hfs_catKey *lrec_key;
	void *rec_data_raw;
	hfs_catData *lrec_data;
	int rec_data_len;
	int min_data_size;
	int errorcode;


	assert(l2_img->format == L2I_HFS);

	lrec_key = rec_raw;
	/* check that key is long enough to hold it all */
	if ((lrec_key->keyLen+1) < (offsetof(hfs_catKey, cName) + lrec_key->cName[0] + 1))
		return IMGTOOLERR_CORRUPTIMAGE;

	/* get pointer to record data */
	errorcode = BT_get_keyed_record_data(&l2_img->u.hfs.cat_BT, rec_raw, rec_len, &rec_data_raw, &rec_data_len);
	if (errorcode)
		return errorcode;
	lrec_data = rec_data_raw;

	/* extract record type */
	if (rec_data_len < 2)
		return IMGTOOLERR_CORRUPTIMAGE;

	/* check that the record is large enough */
	switch (get_UINT16BE(lrec_data->dataType))
	{
	case hcrt_Folder:
		min_data_size = sizeof(hfs_catFolderData);
		break;

	case hcrt_File:
		min_data_size = sizeof(hfs_catFileData);
		break;

	case hcrt_FolderThread:
	case hcrt_FileThread:
		min_data_size = sizeof(hfs_catThreadData);
		break;

	default:
		min_data_size = 0;
		break;
	}

	if (rec_data_len < min_data_size)
		return IMGTOOLERR_CORRUPTIMAGE;

	if (rec_key)
		*rec_key = lrec_key;
	if (rec_data)
		*rec_data = lrec_data;

	return 0;
}

/*
	hfs_cat_open

	Open an enumerator on the disk catalog
*/
static int hfs_cat_open(mac_l2_imgref *l2_img, hfs_cat_enumerator *enumerator)
{
	assert(l2_img->format == L2I_HFS);

	enumerator->l2_img = l2_img;

	return BT_leaf_rec_enumerator_open(&l2_img->u.hfs.cat_BT, &enumerator->BT_enumerator);
}

/*
	hfs_cat_read

	Enumerate the disk catalog
*/
static int hfs_cat_read(hfs_cat_enumerator *enumerator, hfs_catKey **rec_key, hfs_catData **rec_data)
{
	void *rec;
	int rec_len;
	int errorcode;


	*rec_key = NULL;
	*rec_data = NULL;

	/* read next record */
	errorcode = BT_leaf_rec_enumerator_read(&enumerator->BT_enumerator, &rec, &rec_len);
	if (errorcode)
		return errorcode;

	/* check EOList condition */
	if (rec == NULL)
		return 0;

	/* extract record data */
	errorcode = hfs_get_cat_record_data(enumerator->l2_img, rec, rec_len, rec_key, rec_data);
	if (errorcode)
		return errorcode;

	return 0;
}

/*
	hfs_cat_search

	Search the catalog for a given file
*/
static int hfs_cat_search(mac_l2_imgref *l2_img, UINT32 parID, const mac_str31 cName, hfs_catKey **rec_key, hfs_catData **rec_data)
{
	hfs_catKey search_key;
	void *rec;
	int rec_len;
	int errorcode;

	assert(l2_img->format == L2I_HFS);

	if (cName[0] > 31)
		return IMGTOOLERR_UNEXPECTED;

	/* generate search key */
	search_key.keyLen = search_key.resrv1 = 0;	/* these fields do not matter
												to the compare function, so we
												don't fill them */
	set_UINT32BE(&search_key.parID, parID);
	mac_strcpy(search_key.cName, cName);

	/* search key */
	errorcode = BT_search_leaf_rec(&l2_img->u.hfs.cat_BT, &search_key, NULL, NULL, &rec, &rec_len, TRUE, NULL);
	if (errorcode)
		return errorcode;

	/* extract record data */
	errorcode = hfs_get_cat_record_data(l2_img, rec, rec_len, rec_key, rec_data);
	if (errorcode)
		return errorcode;

	return 0;
}

/*
	hfs_resolve_fpath

	Resolve a file path
*/
static int hfs_resolve_fpath(mac_l2_imgref *l2_img, const char *fpath, UINT32 *parID, mac_str255 fname, mac_cat_info *cat_info)
{
	const char *element_start, *element_end;
	int element_len;
	mac_str255 mac_element_name;
	UINT32 lparID = 2;
	int level;
	int errorcode;
	hfs_catKey *catrec_key;
	hfs_catData *catrec_data;
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

		errorcode = hfs_cat_search(l2_img, lparID, mac_element_name, &catrec_key, &catrec_data);
		if (errorcode)
			return errorcode;

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

static int hfs_file_open_internal(mac_l2_imgref *l2_img, const hfs_catFileData *file_rec, mac_forkID fork, mac_fileref *fileref)
{
	assert(l2_img->format == L2I_HFS);

	fileref->l2_img = l2_img;

	fileref->fileID = get_UINT32BE(file_rec->fileID);
	fileref->forkType = fork;

	switch (fork)
	{
	case data_fork:
		fileref->eof = get_UINT32BE(file_rec->dataLogicalSize);
		fileref->pLen = get_UINT32BE(file_rec->dataPhysicalSize);
		memcpy(fileref->u.hfs.extents, file_rec->dataExtents, sizeof(hfs_extent_3));
		break;

	case rsrc_fork:
		fileref->eof = get_UINT32BE(file_rec->rsrcLogicalSize);
		fileref->pLen = get_UINT32BE(file_rec->rsrcPhysicalSize);
		memcpy(fileref->u.hfs.extents, file_rec->rsrcExtents, sizeof(hfs_extent_3));
		break;
	}

	fileref->crPs = 0;

	fileref->reload_buf = TRUE;

	return 0;
}

/*
	hfs_file_open

	Open a file located on an HFS image
*/
static int hfs_file_open(mac_l2_imgref *l2_img, UINT32 parID, const mac_str255 fname, mac_forkID fork, mac_fileref *fileref)
{
	hfs_catKey *catrec_key;
	hfs_catData *catrec_data;
	UINT16 dataRecType;
	int errorcode;

	/* lookup file in catalog */
	errorcode = hfs_cat_search(l2_img, parID, fname, &catrec_key, &catrec_data);
	if (errorcode)
		return errorcode;

	dataRecType = get_UINT16BE(catrec_data->dataType);

	/* file expected */
	if (dataRecType != hcrt_File)
		return IMGTOOLERR_BADFILENAME;

	fileref->u.hfs.parID = get_UINT32BE(catrec_key->parID);
	mac_strcpy(fileref->u.hfs.fname, catrec_key->cName);

	/* open it */
	return hfs_file_open_internal(l2_img, &catrec_data->file, fork, fileref);
}

/*
	hfs_file_get_nth_block_address

	Get the disk block address of a given block in an open file on a MFS image.
	Called by macintosh file code.

	fileref (I/O): mac open file reference
	block_num (I): file block index
	block_address (O): disk block address for the file block

	Return imgtool error code
*/
static int hfs_file_get_nth_block_address(mac_fileref *fileref, UINT32 block_num, UINT32 *block_address)
{
	UINT32 AB_num;
	UINT32 cur_AB;
	UINT32 i;
	void *cur_extents_raw;
	hfs_extent *cur_extents;
	int cur_extents_len;
	void *extents_BT_rec;
	int extents_BT_rec_len;
	int errorcode;
	UINT16 AB_address;

	assert(fileref->l2_img->format == L2I_HFS);

	AB_num = block_num / fileref->l2_img->blocksperAB;
	cur_AB = 0;
	cur_extents = fileref->u.hfs.extents;

	/* first look in catalog tree extents */
	for (i=0; i<3; i++)
	{
		if (AB_num < cur_AB+get_UINT16BE(cur_extents[i].numABlks))
			break;
		cur_AB += get_UINT16BE(cur_extents[i].numABlks);
	}
	if (i == 3)
	{
		/* extent not found: read extents record from extents BT */
		hfs_extentKey search_key;
		hfs_extentKey *found_key;

		search_key.keyLength = keyLength_hfs_extentKey;
		search_key.forkType = fileref->forkType;
		set_UINT32BE(&search_key.fileID, fileref->fileID);
		set_UINT16BE(&search_key.startBlock, AB_num);

		/* search for the record with the largest key lower than or equal to
		search_key.  The keys are constructed in such a way that, if a record
		includes AB_num, it is that one. */
		errorcode = BT_search_leaf_rec(&fileref->l2_img->u.hfs.extents_BT, &search_key,
										NULL, NULL, &extents_BT_rec, &extents_BT_rec_len,
										FALSE, NULL);
		if (errorcode)
			return errorcode;

		if (extents_BT_rec == NULL)
			return IMGTOOLERR_CORRUPTIMAGE;

		found_key = extents_BT_rec;
		/* check that this record concerns the correct file */
		if ((found_key->forkType != fileref->forkType)
			|| (get_UINT32BE(found_key->fileID) != fileref->fileID))
			return IMGTOOLERR_CORRUPTIMAGE;

		/* extract start AB */
		cur_AB = get_UINT16BE(found_key->startBlock);
		/* get extents pointer */
		errorcode = BT_get_keyed_record_data(&fileref->l2_img->u.hfs.extents_BT, extents_BT_rec, extents_BT_rec_len, &cur_extents_raw, &cur_extents_len);
		if (errorcode)
			return errorcode;
		if (cur_extents_len < 3*sizeof(hfs_extent))
			return IMGTOOLERR_CORRUPTIMAGE;
		cur_extents = cur_extents_raw;

		/* pick correct extent in record */
		for (i=0; i<3; i++)
		{
			if (AB_num < cur_AB+get_UINT16BE(cur_extents[i].numABlks))
				break;
			cur_AB += get_UINT16BE(cur_extents[i].numABlks);
		}
		if (i == 3)
			/* extent not found */
			return IMGTOOLERR_CORRUPTIMAGE;
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
	B-tree (Balanced search tree) files are used by the HFS and HFS+ file
	systems: the Extents and Catalog files are both B-Tree.

	Note that these B-trees are B+-trees: data is only on the leaf level, and
	nodes located on the same level are also linked sequentially, which allows
	fast sequenctial access to the catalog file.

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
	records with keys and data).  The first node is always a header node.
	Other nodes can be of any of the 3 other type, or they can be free.
*/

/*
	BTNodeHeader

	Header of a node record
*/
typedef struct BTNodeHeader
{
	UINT32BE fLink;			/* (index of) next node at this level */
	UINT32BE bLink;			/* (index of) previous node at this level */
	UINT8    kind;			/* kind of node (leaf, index, header, map) */
	UINT8    height;		/* zero for header, map; 1 for leaf, 2 through
								treeDepth for index (child is one LESS than
								parent, whatever IM says) */
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

static int BT_check(mac_BTref *BTref);

/*
	BT_open

	Open a file as a B-tree.  The file must be already open as a macintosh
	file.

	BTref (I/O): B-tree file handle to open (BTref->fileref must have been
		open previously)
	key_compare_func (I): function that compares two keys

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

	if ((node_header.kind != btnk_headerNode) || (get_UINT16BE(node_header.numRecords) < 3)
			|| (node_header.height != 0))
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

#if 1
	/* optional: check integrity of B-tree */
	errorcode = BT_check(BTref);
	if (errorcode)
		return errorcode;
#endif

	return 0;
}

/*
	BT_close

	Close a B-tree
*/
static void BT_close(mac_BTref *BTref)
{
	free(BTref->node_buf);
}

/*
	BT_read_node

	Read a node from a B-tree
*/
static int BT_read_node(mac_BTref *BTref, UINT32 node_ID, int expected_kind, int expected_depth, void *dest)
{
	int errorcode;

	/* seek to node */
	errorcode = mac_file_seek(&BTref->fileref, node_ID*BTref->nodeSize);
	if (errorcode)
		return errorcode;

	/* read it */
	errorcode = mac_file_read(&BTref->fileref, BTref->nodeSize, dest);
	if (errorcode)
		return errorcode;

	/* check node kind and depth */
	if ((((BTNodeHeader *) dest)->kind != expected_kind)
			|| (((BTNodeHeader *) dest)->height != expected_depth))
		return IMGTOOLERR_CORRUPTIMAGE;

	return 0;
}

/*
	Extract raw record
*/
static int BT_node_get_record(mac_BTref *BTref, void *node_buf, unsigned recnum, void **rec_ptr, int *rec_len)
{
	UINT16 node_numRecords = get_UINT16BE(((BTNodeHeader *) node_buf)->numRecords);
	UINT16 offset;
	UINT16 next_offset;

	if (recnum >= node_numRecords)
		return IMGTOOLERR_UNEXPECTED;

	offset = get_UINT16BE(((UINT16BE *)((UINT8 *) node_buf + BTref->nodeSize))[-recnum-1]);
	next_offset = get_UINT16BE(((UINT16BE *)((UINT8 *) node_buf + BTref->nodeSize))[-recnum-2]);

	if ((offset < sizeof(BTNodeHeader)) || (offset > BTref->nodeSize-2*node_numRecords)
			|| (next_offset < sizeof(BTNodeHeader)) || (next_offset > BTref->nodeSize-2*node_numRecords)
			|| (offset & 1) || (next_offset & 1)
			|| (offset > next_offset))
		return IMGTOOLERR_CORRUPTIMAGE;

	*rec_ptr = (UINT8 *)node_buf + offset;
	*rec_len = next_offset - offset;

	return 0;
}

/*
	Extract keyed record

	Equivalent to BT_node_get_record, only we do extra checks
*/
static int BT_node_get_keyed_record(mac_BTref *BTref, void *node_buf, unsigned recnum, void **rec_ptr, int *rec_len)
{
	int errorcode;
	void *lrec_ptr;
	int lrec_len;
	int key_len;

	/* extract record */
	errorcode = BT_node_get_record(BTref, node_buf, recnum, &lrec_ptr, &lrec_len);
	if (errorcode)
		return errorcode;

	/* read key len */
	key_len = (BTref->attributes & btha_bigKeysMask)
				? get_UINT16BE(* (UINT16BE *)lrec_ptr)
				: (* (UINT8 *)lrec_ptr);

	/* check that key fits in record */
	if ((key_len + ((BTref->attributes & btha_bigKeysMask) ? 2 : 1)) > lrec_len)
		/* hurk! */
		return IMGTOOLERR_CORRUPTIMAGE;

	if (rec_ptr)
		*rec_ptr = lrec_ptr;
	if (rec_len)
		*rec_len = lrec_len;

	return 0;
}

/*
	extract data from a keyed record
*/
static int BT_get_keyed_record_data(mac_BTref *BTref, void *rec_ptr, int rec_len, void **data_ptr, int *data_len)
{
	int lkey_len;
	int data_offset;

	/* read key len */
	lkey_len = (BTref->attributes & btha_bigKeysMask)
				? get_UINT16BE(* (UINT16BE *)rec_ptr)
				: (* (UINT8 *)rec_ptr);

	/* compute offset to data record */
	data_offset = lkey_len + ((BTref->attributes & btha_bigKeysMask) ? 2 : 1);
	if (data_offset > rec_len)
		/* hurk! */
		return IMGTOOLERR_CORRUPTIMAGE;
	/* fix alignment */
	if (data_offset & 1)
		data_offset++;

	if (data_ptr)
		*data_ptr = (UINT8 *)rec_ptr + data_offset;
	if (data_len)
		*data_len = (rec_len > data_offset) ? rec_len-data_offset : 0;

	return 0;
}

/*
	BT_check

	Check integrity of a complete B-tree
*/
static int BT_check(mac_BTref *BTref)
{
	UINT16 node_numRecords;
	BTHeaderRecord *header_rec;
	UINT8 *bitmap;
	struct
	{
		void *buf;
		UINT32 node_num;
		UINT32 cur_rec;
		UINT32 num_recs;
	} *data_nodes;
	int i;
	UINT32 cur_node, prev_node;
	void *rec1, *rec2;
	int rec1_len, rec2_len;
	void *rec1_data;
	int rec1_data_len;
	UINT32 totalNodes, lastLeafNode;
	UINT32 freeNodes;
	int compare_result;
	UINT32 map_count, map_len;
	UINT32 run_len;
	UINT32 run_bit_len;
	UINT32 actualFreeNodes;
	int errorcode;


	/* read header node */
	errorcode = BT_read_node(BTref, 0, btnk_headerNode, 0, BTref->node_buf);
	if (errorcode)
		return errorcode;

	/* check we have enough records */
	node_numRecords = get_UINT16BE(((BTNodeHeader *) BTref->node_buf)->numRecords);
	if (node_numRecords < 3)
		return IMGTOOLERR_CORRUPTIMAGE;

	/* get header record */
	errorcode = BT_node_get_record(BTref, BTref->node_buf, 0, &rec1, &rec1_len);
	if (errorcode)
		return errorcode;
	header_rec = (BTHeaderRecord *)rec1;

	/* check lenght of header record */
	if (rec1_len < sizeof(BTHeaderRecord))
		return IMGTOOLERR_CORRUPTIMAGE;

	totalNodes = get_UINT32BE(header_rec->totalNodes);
	if (totalNodes == 0)
		/* at least one header node */
		return IMGTOOLERR_CORRUPTIMAGE;
	lastLeafNode = get_UINT32BE(header_rec->lastLeafNode);
	freeNodes = get_UINT32BE(header_rec->freeNodes);

	/* check file lenght */
	if ((BTref->nodeSize * totalNodes) != BTref->fileref.pLen)
		return IMGTOOLERR_CORRUPTIMAGE;

	/* initialize for the function postlog ("bail:" tag) */
	errorcode = 0;
	bitmap = NULL;
	data_nodes = NULL;

	/* alloc buffer for reconstructed bitmap */
	map_len = (totalNodes + 7) / 8;
	bitmap = malloc(map_len);
	if (! bitmap)
		return IMGTOOLERR_OUTOFMEMORY;
	memset(bitmap, 0, map_len);

	/* check map node chain */
	cur_node = 0;	/* node 0 is the header node... */
	bitmap[0] |= 0x80;
	/* check back linking */
	if (get_UINT32BE(((BTNodeHeader *) BTref->node_buf)->bLink))
	{
		errorcode = IMGTOOLERR_CORRUPTIMAGE;
		goto bail;
	}
	/* get pointer to next node */
	cur_node = get_UINT32BE(((BTNodeHeader *) BTref->node_buf)->fLink);
	while (cur_node != 0)
	{
		/* save node address */
		prev_node = cur_node;
		/* check that node has not been used for another purpose */
		/* this check is unecessary because the current consistency checks that
		forward and back linking match and that node height is correct are
		enough to detect such errors */
		/*if (bitmap[cur_node >> 3] & (0x80 >> (cur_node & 7)))
		{
			errorcode = IMGTOOLERR_CORRUPTIMAGE;
			goto bail;
		}*/
		/* add node in bitmap */
		bitmap[cur_node >> 3] |= (0x80 >> (cur_node & 7));
		/* read map node */
		errorcode = BT_read_node(BTref, cur_node, btnk_mapNode, 0, BTref->node_buf);
		if (errorcode)
			goto bail;
		/* check back linking */
		if (get_UINT32BE(((BTNodeHeader *) BTref->node_buf)->bLink) != prev_node)
		{
			errorcode = IMGTOOLERR_CORRUPTIMAGE;
			goto bail;
		}
		/* get pointer to next node */
		cur_node = get_UINT32BE(((BTNodeHeader *) BTref->node_buf)->fLink);
	}

	/* check B-tree data nodes (i.e. index and leaf nodes) */
	if (BTref->treeDepth == 0)
	{
		/* B-tree is empty */
		if (BTref->rootNode || BTref->firstLeafNode || lastLeafNode)
		{
			errorcode = IMGTOOLERR_OUTOFMEMORY;
			goto bail;
		}
	}
	else
	{
		/* alloc array of buffers for catalog data nodes */
		data_nodes = malloc(sizeof(data_nodes[0]) * BTref->treeDepth);
		if (! data_nodes)
		{
			errorcode = IMGTOOLERR_OUTOFMEMORY;
			goto bail;
		}
		for (i=0; i<BTref->treeDepth; i++)
			data_nodes[i].buf = NULL;	/* required for function postlog to work should next loop fail */
		for (i=0; i<BTref->treeDepth; i++)
		{
			data_nodes[i].buf = malloc(BTref->nodeSize);
			if (!data_nodes[i].buf)
			{
				errorcode = IMGTOOLERR_OUTOFMEMORY;
				goto bail;
			}
		}

		/* read first data nodes */
		cur_node = BTref->rootNode;
		for (i=BTref->treeDepth-1; i>=0; i--)
		{
			/* check node index */
			if (cur_node >= totalNodes)
			{
				errorcode = IMGTOOLERR_CORRUPTIMAGE;
				goto bail;
			}
			/* check that node has not been used for another purpose */
			/* this check is unecessary because the current consistency checks
			that forward and back linking match and that node height is correct
			are enough to detect such errors */
			/*if (bitmap[cur_node >> 3] & (0x80 >> (cur_node & 7)))
			{
				errorcode = IMGTOOLERR_CORRUPTIMAGE;
				goto bail;
			}*/
			/* add node in bitmap */
			bitmap[cur_node >> 3] |= (0x80 >> (cur_node & 7));
			/* read node */
			errorcode = BT_read_node(BTref, cur_node, i ? btnk_indexNode : btnk_leafNode, i+1, data_nodes[i].buf);
			if (errorcode)
				goto bail;
			/* check that it is the first node at this level */
			if (get_UINT32BE(((BTNodeHeader *) data_nodes[i].buf)->bLink))
			{
				errorcode = IMGTOOLERR_CORRUPTIMAGE;
				goto bail;
			}
			/* fill other fields */
			data_nodes[i].node_num = cur_node;
			data_nodes[i].cur_rec = 0;
			data_nodes[i].num_recs = get_UINT16BE(((BTNodeHeader *) data_nodes[i].buf)->numRecords);
			/* check that there is at least one record */
			if (data_nodes[i].num_recs == 0)
			{
				errorcode = IMGTOOLERR_CORRUPTIMAGE;
				goto bail;
			}

			/* iterate to next level if applicable */
			if (i != 0)
			{
				/* extract first record */
				errorcode = BT_node_get_keyed_record(BTref, data_nodes[i].buf, 0, &rec1, &rec1_len);
				if (errorcode)
					goto bail;

				/* extract record data ptr */
				errorcode = BT_get_keyed_record_data(BTref, rec1, rec1_len, &rec1_data, &rec1_data_len);
				if (errorcode)
					goto bail;
				if (rec1_data_len < sizeof(UINT32BE))
				{
					errorcode = IMGTOOLERR_CORRUPTIMAGE;
					goto bail;
				}

				/* iterate to next level */
				cur_node = get_UINT32BE(* (UINT32BE *)rec1_data);
			}
		}

		/* check that a) the root node has no successor, and b) that we have really
		read the first leaf node */
		if (get_UINT32BE(((BTNodeHeader *) data_nodes[BTref->treeDepth-1].buf)->fLink)
				|| (cur_node != BTref->firstLeafNode))
		{
			errorcode = IMGTOOLERR_CORRUPTIMAGE;
			goto bail;
		}

		/* check that keys are ordered correctly */
		while (1)
		{
			/* iterate through parent nodes */
			i = 0;
			while ((i<BTref->treeDepth) && ((data_nodes[i].cur_rec == 0) || (data_nodes[i].cur_rec == data_nodes[i].num_recs)))
			{
				/* read next node if necessary */
				if (data_nodes[i].cur_rec == data_nodes[i].num_recs)
				{
					/* get link to next node */
					cur_node = get_UINT32BE(((BTNodeHeader *) data_nodes[i].buf)->fLink);
					if (cur_node == 0)
					{
						if (i == 0)
							/* normal End of List */
							goto end_of_list;
						else
						{
							/* error */
							errorcode = IMGTOOLERR_CORRUPTIMAGE;
							goto bail;
						}
					}
					/* add node in bitmap */
					bitmap[cur_node >> 3] |= (0x80 >> (cur_node & 7));
					/* read node */
					errorcode = BT_read_node(BTref, cur_node, i ? btnk_indexNode : btnk_leafNode, i+1, data_nodes[i].buf);
					if (errorcode)
						goto bail;
					/* check that backward linking match forward linking */
					if (get_UINT32BE(((BTNodeHeader *) data_nodes[i].buf)->bLink) != data_nodes[i].node_num)
					{
						errorcode = IMGTOOLERR_CORRUPTIMAGE;
						goto bail;
					}
					/* fill other fields */
					data_nodes[i].node_num = cur_node;
					data_nodes[i].cur_rec = 0;
					data_nodes[i].num_recs = get_UINT16BE(((BTNodeHeader *) data_nodes[i].buf)->numRecords);
					/* check that there is at least one record */
					if (data_nodes[i].num_recs == 0)
					{
						errorcode = IMGTOOLERR_CORRUPTIMAGE;
						goto bail;
					}
					/* next test is not necessary because we have checked that
					the root node has no successor */
					/*if (i < BTref->treeDepth-1)
					{*/
						data_nodes[i+1].cur_rec++;
					/*}
					else
					{
						errorcode = IMGTOOLERR_CORRUPTIMAGE;
						goto bail;
					}*/
				}
				i++;
			}

			if (i<BTref->treeDepth)
			{
				/* extract current record */
				errorcode = BT_node_get_keyed_record(BTref, data_nodes[i].buf, data_nodes[i].cur_rec, &rec1, &rec1_len);
				if (errorcode)
					goto bail;

				/* extract previous record */
				errorcode = BT_node_get_keyed_record(BTref, data_nodes[i].buf, data_nodes[i].cur_rec-1, &rec2, &rec2_len);
				if (errorcode)
					goto bail;

				compare_result = (*BTref->key_compare_func)(rec1, rec2);
				if (compare_result <= 0)
				{
					errorcode = IMGTOOLERR_CORRUPTIMAGE;
					goto bail;
				}

				i--;
			}
			else
			{
				i--;
				if (i>0)
				{	/* extract first record of root if it is an index node */
					errorcode = BT_node_get_keyed_record(BTref, data_nodes[i].buf, data_nodes[i].cur_rec, &rec1, &rec1_len);
					if (errorcode)
						goto bail;
				}
				i--;
			}

			while (i>=0)
			{
				/* extract first record of current level */
				errorcode = BT_node_get_keyed_record(BTref, data_nodes[i].buf, data_nodes[i].cur_rec, &rec2, &rec2_len);
				if (errorcode)
					goto bail;

				/* compare key with key of current record of upper level */
				compare_result = (*BTref->key_compare_func)(rec1, rec2);
				if (compare_result != 0)
				{
					errorcode = IMGTOOLERR_CORRUPTIMAGE;
					goto bail;
				}

				/* extract record data ptr */
				errorcode = BT_get_keyed_record_data(BTref, rec1, rec1_len, &rec1_data, &rec1_data_len);
				if (errorcode)
					goto bail;
				if (rec1_data_len < sizeof(UINT32BE))
				{
					errorcode = IMGTOOLERR_CORRUPTIMAGE;
					goto bail;
				}
				cur_node = get_UINT32BE(* (UINT32BE *)rec1_data);

				/* compare node index with data of current record of upper
				level */
				if (cur_node != data_nodes[i].node_num)
				{
					errorcode = IMGTOOLERR_CORRUPTIMAGE;
					goto bail;
				}

				/* iterate to next level */
				rec1 = rec2;
				rec1_len = rec2_len;
				i--;
			}

			/* next leaf record */
			data_nodes[0].cur_rec++;
		}

end_of_list:
		/* check that we are at the end of list for each index level */
		for (i=1; i<BTref->treeDepth; i++)
		{
			if ((data_nodes[i].cur_rec != (data_nodes[i].num_recs-1))
					|| get_UINT32BE(((BTNodeHeader *) data_nodes[i].buf)->fLink))
			{
				errorcode = IMGTOOLERR_CORRUPTIMAGE;
				goto bail;
			}
		}
		/* check that the last leaf node is what it is expected to be */
		if (data_nodes[0].node_num != lastLeafNode)
		{
			errorcode = IMGTOOLERR_CORRUPTIMAGE;
			goto bail;
		}
	}

	/* re-read header node */
	errorcode = BT_read_node(BTref, 0, btnk_headerNode, 0, BTref->node_buf);
	if (errorcode)
		goto bail;

	/* get header bitmap record */
	errorcode = BT_node_get_record(BTref, BTref->node_buf, 2, &rec1, &rec1_len);
	if (errorcode)
		goto bail;

	/* check bitmap, iterating map nodes */
	map_count = 0;
	actualFreeNodes = 0;
	while (map_count < map_len)
	{
		/* compute compare len */
		run_len = rec1_len;
		if (run_len > (map_len-map_count))
			run_len = map_len-map_count;
		/* check that all used nodes are marked as such in the B-tree bitmap */
		for (i=0; i<run_len; i++)
			if (bitmap[map_count+i] & ~((UINT8 *)rec1)[i])
			{
				errorcode = IMGTOOLERR_CORRUPTIMAGE;
				goto bail;
			}
		/* count free nodes */
		run_bit_len = rec1_len*8;
		if (run_bit_len > (totalNodes-map_count*8))
			run_bit_len = totalNodes-map_count*8;
		for (i=0; i<run_bit_len; i++)
			if (! (((UINT8 *)rec1)[i>>3] & (0x80 >> (i & 7))))
				actualFreeNodes++;
		map_count += run_len;
		/* read next map node if required */
		if (map_count < map_len)
		{
			/* get pointer to next node */
			cur_node = get_UINT32BE(((BTNodeHeader *) BTref->node_buf)->fLink);
			if (cur_node == 0)
			{
				errorcode = IMGTOOLERR_CORRUPTIMAGE;
				goto bail;
			}
			/* read map node */
			errorcode = BT_read_node(BTref, cur_node, btnk_mapNode, 0, BTref->node_buf);
			if (errorcode)
				goto bail;
			/* get map record */
			errorcode = BT_node_get_record(BTref, BTref->node_buf, 0, &rec1, &rec1_len);
			if (errorcode)
				goto bail;
			header_rec = (BTHeaderRecord *)rec1;
		}
	}

	/* check free node count */
	if (freeNodes != actualFreeNodes)
		return IMGTOOLERR_CORRUPTIMAGE;

bail:
	/* free buffers */
	if (data_nodes)
	{
		for (i=0; i<BTref->treeDepth; i++)
			if (data_nodes[i].buf)
				free(data_nodes[i].buf);
		free(data_nodes);
	}
	if (bitmap)
		free(bitmap);

	return errorcode;
}


static int BT_search_leaf_rec(mac_BTref *BTref, void *search_key,
								UINT32 *node_ID, int *record_ID,
								void **record_ptr, int *record_len,
								int search_exact_match, int *match_found)
{
	int errorcode;
	int i;
	UINT32 cur_node;
	void *cur_rec;
	int cur_rec_len;
	void *last_rec;
	int last_rec_len = 0;
	void *rec_data;
	int rec_data_len;
	int depth;
	UINT16 node_numRecords;
	int compare_result = 0;

	/* start with root node */
	if ((BTref->rootNode == 0) || (BTref->treeDepth == 0))
		/* tree is empty */
		return ((BTref->rootNode == 0) == (BTref->treeDepth == 0))
					? IMGTOOLERR_FILENOTFOUND
					: IMGTOOLERR_CORRUPTIMAGE;

	cur_node = BTref->rootNode;
	depth = BTref->treeDepth;

	while (1)
	{
		/* read current node */
		errorcode = BT_read_node(BTref, cur_node, (depth > 1) ? btnk_indexNode : btnk_leafNode, depth, BTref->node_buf);
		if (errorcode)
			return errorcode;

		/* search for key */
		node_numRecords = get_UINT16BE(((BTNodeHeader *) BTref->node_buf)->numRecords);
		last_rec = cur_rec = NULL;
		for (i=0; i<node_numRecords; i++)
		{
			errorcode = BT_node_get_keyed_record(BTref, BTref->node_buf, i, &cur_rec, &cur_rec_len);
			if (errorcode)
				return errorcode;

			compare_result = (*BTref->key_compare_func)(cur_rec, search_key);
			if (compare_result > 0)
				break;
			last_rec = cur_rec;
			last_rec_len = cur_rec_len;
			if (compare_result == 0)
				break;
		}

		if (! last_rec)
		{	/* all keys are greater than the search key: the search key is
			nowhere in the tree */
			if (search_exact_match)
				return IMGTOOLERR_FILENOTFOUND;

			if (match_found)
				*match_found = FALSE;

			if (node_ID)
				*node_ID = 0;

			if (record_ID)
				*record_ID = -1;

			if (record_ptr)
				*record_ptr = NULL;

			return 0;
		}

		if (((BTNodeHeader *) BTref->node_buf)->kind == btnk_leafNode)
			/* leaf node -> end of search */
			break;

		/* extract record data ptr */
		errorcode = BT_get_keyed_record_data(BTref, last_rec, last_rec_len, &rec_data, &rec_data_len);
		if (errorcode)
			return errorcode;
		if (rec_data_len < sizeof(UINT32BE))
			return IMGTOOLERR_CORRUPTIMAGE;

		/* iterate to next level */
		cur_node = get_UINT32BE(* (UINT32BE *)rec_data);
		depth--;
	}

	if (compare_result != 0)
		/* key not found */
		if (search_exact_match)
			return IMGTOOLERR_FILENOTFOUND;

	if (match_found)
		*match_found = (compare_result == 0);

	if (node_ID)
		*node_ID = cur_node;

	if (record_ID)
		*record_ID = i;

	if (record_ptr)
		*record_ptr = last_rec;

	if (record_len)
		*record_len = last_rec_len;

	return 0;
}

static int BT_leaf_rec_enumerator_open(mac_BTref *BTref, BT_leaf_rec_enumerator *enumerator)
{
	enumerator->BTref = BTref;
	enumerator->cur_node = BTref->firstLeafNode;
	enumerator->cur_rec = 0;

	return 0;
}

static int BT_leaf_rec_enumerator_read(BT_leaf_rec_enumerator *enumerator, void **record_ptr, int *rec_len)
{
	UINT16 node_numRecords;
	int errorcode;


	*record_ptr = NULL;

	/* check EOList condition */
	if (enumerator->cur_node == 0)
		return 0;

	/* read current node */
	errorcode = BT_read_node(enumerator->BTref, enumerator->cur_node, btnk_leafNode, 1, enumerator->BTref->node_buf);
	if (errorcode)
		return errorcode;
	node_numRecords = get_UINT16BE(((BTNodeHeader *) enumerator->BTref->node_buf)->numRecords);

	/* skip nodes until we find a record */
	while ((enumerator->cur_rec >= node_numRecords) && (enumerator->cur_node != 0))
	{
		enumerator->cur_node = get_UINT32BE(((BTNodeHeader *) enumerator->BTref->node_buf)->fLink);
		enumerator->cur_rec = 0;

		/* read node */
		errorcode = BT_read_node(enumerator->BTref, enumerator->cur_node, btnk_leafNode, 1, enumerator->BTref->node_buf);
		if (errorcode)
			return errorcode;
		node_numRecords = get_UINT16BE(((BTNodeHeader *) enumerator->BTref->node_buf)->numRecords);
	}

	/* get current record */
	errorcode = BT_node_get_keyed_record(enumerator->BTref, enumerator->BTref->node_buf, enumerator->cur_rec, record_ptr, rec_len);
	if (errorcode)
		return errorcode;

	/* iterate to next record */
	enumerator->cur_rec++;
	return 0;
}

/*
	Empty node alloc algorithm:

	* see if there is any free node in B-tree bitmap
	* optionally, try to compact the B-tree if file is full
	* if file is still full, extend EOF and try again
	* mark new block as used and return its index


	Empty node delete algorithm:

	* remove node from link list
	* mark node as free in the B-tree bitmap
	* optionally, if more than N% of the B-tree is free, compact the B-tree and
		free some disk space
	* Count nodes on this level; if there is only one left, delete parent index
		node and mark the relaining node as root; if it was the last leaf node,
		update header node with an empty B-tree; in either case, decrement tree
		depth


	Record shifting algorithm:

	For a given node and its first non-empty successor node:

	* compute how much free room there is in the node
	* see if the first record of the first non-empty successor can fit
	* if so, move it (i.e. delete the first record of the later node, and add a
		copy of it to the end of the former)


	Node merging algorithm

	* Consider node and its predecessor.  If there is room, shift all records
		from later to former, then delete empty later node, and delete later
		record from parent index node.


	Node splitting algorithm (non-first)

	* Consider node and its predecessor.  Create new middle node and split
		records in 3 even sets.  Update record for last node and insert record
		for middle node in parent index node.


	Node splitting algorithm (first node)

	* Create new successor node, and split records in 2 1/3 and 2/3 sets.
		Insert record for later node in parent index node.


	Record delete algorithm:

	* remove record from node
	* if record was first in node, test if node is now empty
		* if node is not empty, substitute key of deleted record with key of
			new head record in index tree
		* if node is empty, delete key of deleted record in index tree, then
			delete empty node
	* optionally, look the predecessor node.  Merge the two nodes if possible.


	Record insert algorithm:

	* if there room, just insert new record in node; if new record is in first
		position, update record in parent index node
	* else consider predecessor: see if we can make enough room by shifting
		records.  If so, do shift records, insert new record, update record in
		parent index node
	* else split the nodes and insert record
*/
/*
	Possible additions:

	Node compaction algorithm:

	This algorithm can be executed with a specific start point and max number
	of nodes, or with all nodes on a level.

	* see how many nodes we can save by shifting records left
	* if we will save at least one node, do shift as many records as possible
		(try to leave free space split homogeneously???)
*/

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

	resfileref (I/O): ressource file handle to open (resfileref->fileref must
		have been open previously)

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
	All macintosh volume have a desktop database.

	The desktop database includes file comments, copy of BNDL and FREF
	resources that describes supported file types for each application on the
	volume, copies of icons for each file type registered by each application
	on the volume, etc.  With MFS volumes, this file also contains the list of
	folders.


	There have been two implementations of the desktop database:

	* The original desktop file.  The database is stored in the resource fork
		of a (usually invisible) file called "Desktop" (case may change
		according to the system version), located at the root of the volume.
		The desktop file is used by System 6 and earlier for all volumes
		(unless Appleshare 2 is installed and the volume is shared IIRC), and
		by System 7 and later for volumes smaller than 2MBytes (to keep floppy
		disks fully compatible with earlier versions of system).  This file is
		incompletely documented by Apple technote TB06.

	* The desktop database.  The database is stored in the resource fork is
		stored in the data fork of two (usually invisible) files called
		"Desktop DF" and "Desktop DF".  The desktop database is used for
		volumes shared by Appleshare 2, and for most volumes under System 7 and
		later. The format of these file is not documented AFAIK.


	The reasons for the introduction of the desktop database were:
	* the macintosh ressource manager cannot share ressource files, which was
		a problem for Appleshare
	* the macintosh ressource manager is pretty limited ()
*/

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
	errorcode = mac_file_open(l2_img, 2, desktop_fname, rsrc_fork, &resfileref.fileref);
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
	mac_image_writefile,			/* write file */
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
	image->base.module = mod;

	errorcode = floppy_image_open(f, &image->l1_img);
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
			hfs_cat_enumerator catref;		/* catalog file enumerator */
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
		errorcode = hfs_cat_open(image, &iter->u.hfs.catref);
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

	errorcode = mfs_dir_read(&iter->u.mfs.dirref, &cur_dir_entry);
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
	hfs_catKey *catrec_key;
	hfs_catData *catrec_data;
	UINT16 dataRecType;
	int errorcode;
	/* currently, the mac->C conversion transcodes one mac char with at most 3
	C chars */
	char buf[31*3+1];
	UINT32 parID;
	int cur_name_head;
	static const UINT8 mac_empty_str[1] = { '\0' };


	assert(iter->format == L2I_HFS);

	ent->corrupt = 0;
	ent->eof = 0;

	do
	{
		errorcode = hfs_cat_read(&iter->u.hfs.catref, &catrec_key, &catrec_data);
		if (errorcode)
		{
			/* error */
			ent->corrupt = 1;
			return errorcode;
		}
		else if (!catrec_key)
		{
			/* EOF */
			ent->eof = 1;
			return 0;
		}
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
		errorcode = hfs_cat_search(iter->l2_img, parID, mac_empty_str, &catrec_key, &catrec_data);
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

	/* resolve path and fetch file info from directory/catalog */
	errorcode = mac_resolve_fpath(image, fpath, &parID, fname, &cat_info, FALSE);
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
	errorcode = mac_file_open(image, parID, fname, data_fork, &fileref);
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
	errorcode = mac_file_open(image, parID, fname, rsrc_fork, &fileref);
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
	Add a file to a disk image.  The file must be in macbinary format.
*/
static int mac_image_writefile(IMAGE *img, const char *fpath, STREAM *sourcef, const ResolvedOption *in_options)
{
	mac_l2_imgref *image = (mac_l2_imgref *) img;
	UINT32 parID;
	mac_str255 fname;
	mac_cat_info cat_info;
	mac_fileref fileref;
	UINT32 data_len, rsrc_len;
	UINT16 sec_header_len;
	/*UINT16 commentID;
	mac_str255 comment;*/
	UINT8 buf[512];
	UINT32 i, run_len;
	int errorcode;

	if (image->format == L2I_HFS)
		return IMGTOOLERR_UNIMPLEMENTED;

	/* read MB header */
	{
		MBHeader header;
		int version = -1;

		if (stream_read(sourcef, &header, sizeof(header)) != sizeof(header))
			return IMGTOOLERR_READERROR;

		if (get_UINT32BE(header.signature) == 0x6d42494e/*'mBIN'*/)
		{	/* MacBinary III */
			version = 2;

#if 0
			/* I don't know what is going on, but Drop MacBinary III uses a
			different formula for CRC than MacBinary II, so I am forced to
			disable CRC checking */
			if (get_UINT16BE(header.crc) != MB_calcCRC(&header))
				return IMGTOOLERR_CORRUPTIMAGE;
#endif
		}
		else if ((header.version_old == 0) || (header.zerofill0 == 0))
		{
			if (get_UINT16BE(header.crc) == MB_calcCRC(&header))
			{	/* MacBinary II */
				version = 1;
			}
			else if (header.zerofill1 == 0)
			{	/* MacBinary I */
				version = 0;

				set_UINT32BE(&header.reserved, 0);
				set_UINT16BE(&header.sec_header_len, 0);
				header.version = 129;
				header.compat_version = 129;
				set_UINT16BE(&header.crc, 0);
				set_UINT16BE(&header.reserved2, 0);
			}
			/*&header.signature*/
		}

		if (version == -1)
			return IMGTOOLERR_CORRUPTIMAGE;

		if (header.compat_version > 130)
			return IMGTOOLERR_UNIMPLEMENTED;

		if (header.fname[0] > 63)
			return IMGTOOLERR_CORRUPTIMAGE;

		/*if (header.version_old != 0)
			return IMGTOOLERR_UNIMPLEMENTED;*/
		/*mac_strcpy(fname, header.fname);*/
		cat_info.flFinderInfo.type = header.ftype;
		cat_info.flFinderInfo.creator = header.fcreator;
		cat_info.flFinderInfo.flags.bytes[0] = header.flags_MSB;
		cat_info.flFinderInfo.location = header.location;
		cat_info.flFinderInfo.fldr = header.fldr;
		cat_info.flags = header.protect & 0x01;
		data_len = get_UINT32BE(header.data_len);
		rsrc_len = get_UINT32BE(header.rsrc_len);
		cat_info.createDate = get_UINT32BE(header.crDate);
		cat_info.modifyDate = get_UINT32BE(header.lsMod);
		/*comment[0] = get_UINT16BE(header.comment_len);*/	/* comment lenght */
		cat_info.flFinderInfo.flags.bytes[1] = header.flags_LSB;
		/* Next two fields are set to 0 with MFS volumes.  IIRC, 0 normally
		means system script: I don't think MFS stores the file name script code
		anywhere on disk, so it should be a reasonable approximation. */
		cat_info.flXFinderInfo.script = header.fname_script;
		cat_info.flXFinderInfo.XFlags = header.xflags;
		sec_header_len = get_UINT16BE(header.sec_header_len);
	}

	/* skip seconday header */
	if (sec_header_len)
	{
		errorcode = stream_seek(sourcef, (sec_header_len + 127) & ~127, SEEK_CUR);
		if (errorcode)
			return errorcode;
	}

	/* create file */
	/* clear inited flag and file location in window */
	set_UINT16BE(&cat_info.flFinderInfo.flags, get_UINT16BE(cat_info.flFinderInfo.flags) & ~fif_hasBeenInited);
	set_UINT16BE(&cat_info.flFinderInfo.location.v, 0);
	set_UINT16BE(&cat_info.flFinderInfo.location.h, 0);

	/* resolve path and create file */
	errorcode = mac_resolve_fpath(image, fpath, &parID, fname, &cat_info, TRUE);
	if (errorcode)
		return errorcode;

	/* open file DF */
	errorcode = mac_file_open(image, parID, fname, data_fork, &fileref);
	if (errorcode)
		return errorcode;

	errorcode = mac_file_seteof(&fileref, data_len);
	if (errorcode)
		return errorcode;

	/* extract DF */
	for (i=0; i<data_len;)
	{
		run_len = data_len - i;
		if (run_len > 512)
			run_len = 512;
		if (stream_read(sourcef, buf, run_len) != run_len)
			return IMGTOOLERR_READERROR;
		errorcode = mac_file_write(&fileref, run_len, buf);
		if (errorcode)
			return errorcode;
		i += run_len;
	}
	/* pad to multiple of 128 */
	if (data_len % 128)
	{
		run_len = 128 - (data_len % 128);
		errorcode = stream_seek(sourcef, run_len, SEEK_CUR);
		if (errorcode)
			return errorcode;
	}

	/* open file RF */
	errorcode = mac_file_open(image, parID, fname, rsrc_fork, &fileref);
	if (errorcode)
		return errorcode;

	errorcode = mac_file_seteof(&fileref, rsrc_len);
	if (errorcode)
		return errorcode;

	/* extract RF */
	for (i=0; i<rsrc_len;)
	{
		run_len = rsrc_len - i;
		if (run_len > 512)
			run_len = 512;
		if (stream_read(sourcef, buf, run_len) != run_len)
			return IMGTOOLERR_READERROR;
		errorcode = mac_file_write(&fileref, run_len, buf);
		if (errorcode)
			return errorcode;
		i += run_len;
	}
	/* pad to multiple of 128 */
	if (rsrc_len % 128)
	{
		run_len = 128 - (rsrc_len % 128);
		errorcode = stream_seek(sourcef, run_len, SEEK_CUR);
		if (errorcode)
			return errorcode;
	}
	/* TODO: set comments */
	/* ... */

	return 0;
}

/*
	Delete a file from a disk image.
*/
static int mac_image_deletefile(IMAGE *img, const char *fname)
{
	return IMGTOOLERR_UNIMPLEMENTED;
}

/*
	Create a blank disk image.
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
