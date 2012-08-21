/***************************************************************************

    Atari 400/800

    Floppy Disk Controller code

    Juergen Buchmueller, June 1998

***************************************************************************/

#include <ctype.h>

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "includes/atari.h"
#include "ataridev.h"
#include "sound/pokey.h"
#include "machine/6821pia.h"
#include "imagedev/flopdrv.h"
#include "formats/atari_dsk.h"

#define VERBOSE_SERIAL	0
#define VERBOSE_CHKSUM	0

typedef struct _atari_drive atari_drive;
struct _atari_drive
{
	UINT8 *image;		/* malloc'd image */
	int type;			/* type of image (XFD, ATR, DSK) */
	int mode;			/* 0 read only, != 0 read/write */
	int density;		/* 0 SD, 1 MD, 2 DD */
	int header_skip;	/* number of bytes in format header */
	int tracks; 		/* number of tracks (35,40,77,80) */
	int heads;			/* number of heads (1,2) */
	int spt;			/* sectors per track (18,26) */
	int seclen; 		/* sector length (128,256) */
	int bseclen;		/* boot sector length (sectors 1..3) */
	int sectors;		/* total sectors, ie. tracks x heads x spt */
};

typedef struct _atari_fdc_t atari_fdc_t;
struct _atari_fdc_t
{
	int  serout_count;
	int  serout_offs;
	UINT8 serout_buff[512];
	UINT8 serout_chksum;
	int  serout_delay;

	int  serin_count;
	int  serin_offs;
	UINT8 serin_buff[512];
	UINT8 serin_chksum;
	int  serin_delay;

	atari_drive drv[4];
};

/*************************************
 *
 *  Disk stuff
 *
 *************************************/

#define FORMAT_XFD	0
#define FORMAT_ATR	1
#define FORMAT_DSK	2

/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/
INLINE atari_fdc_t *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == ATARI_FDC);

	return (atari_fdc_t *)downcast<legacy_device_base *>(device)->token();
}

/*****************************************************************************
 * This is the structure I used in my own Atari 800 emulator some years ago
 * Though it's a bit overloaded, I used it as it is the maximum of all
 * supported formats:
 * XFD no header at all
 * ATR 16 bytes header
 * DSK this struct
 * It is used to determine the format of a XFD image by it's size only
 *****************************************************************************/

typedef struct _dsk_format dsk_format;
struct _dsk_format
{
	UINT8 density;
	UINT8 tracks;
	UINT8 door;
	UINT8 sta1;
	UINT8 spt;
	UINT8 doublesided;
	UINT8 highdensity;
	UINT8 seclen_hi;
	UINT8 seclen_lo;
	UINT8 status;
	UINT8 sta2;
	UINT8 sta3;
	UINT8 sta4;
	UINT8 cr;
	UINT8 info[65+1];
};

/* combined with the size the image should have */
typedef struct _xfd_format xfd_format;
struct _xfd_format
{
	int size;
	dsk_format dsk;
};

/* here's a table of known xfd formats */
static const xfd_format xfd_formats[] =
{
	{35 * 18 * 1 * 128, 				{0,35,1,0,18,0,0,0,128,255,0,0,0,13,"35 SS/SD"}},
	{35 * 26 * 1 * 128, 				{1,35,1,0,26,0,4,0,128,255,0,0,0,13,"35 SS/MD"}},
	{(35 * 18 * 1 - 3) * 256 + 3 * 128, {2,35,1,0,18,0,4,1,  0,255,0,0,0,13,"35 SS/DD"}},
	{40 * 18 * 1 * 128, 				{0,40,1,0,18,0,0,0,128,255,0,0,0,13,"40 SS/SD"}},
	{40 * 26 * 1 * 128, 				{1,40,1,0,26,0,4,0,128,255,0,0,0,13,"40 SS/MD"}},
	{(40 * 18 * 1 - 3) * 256 + 3 * 128, {2,40,1,0,18,0,4,1,  0,255,0,0,0,13,"40 SS/DD"}},
	{40 * 18 * 2 * 128, 				{0,40,1,0,18,1,0,0,128,255,0,0,0,13,"40 DS/SD"}},
	{40 * 26 * 2 * 128, 				{1,40,1,0,26,1,4,0,128,255,0,0,0,13,"40 DS/MD"}},
	{(40 * 18 * 2 - 3) * 256 + 3 * 128, {2,40,1,0,18,1,4,1,  0,255,0,0,0,13,"40 DS/DD"}},
	{77 * 18 * 1 * 128, 				{0,77,1,0,18,0,0,0,128,255,0,0,0,13,"77 SS/SD"}},
	{77 * 26 * 1 * 128, 				{1,77,1,0,26,0,4,0,128,255,0,0,0,13,"77 SS/MD"}},
	{(77 * 18 * 1 - 3) * 256 + 3 * 128, {2,77,1,0,18,0,4,1,  0,255,0,0,0,13,"77 SS/DD"}},
	{77 * 18 * 2 * 128, 				{0,77,1,0,18,1,0,0,128,255,0,0,0,13,"77 DS/SD"}},
	{77 * 26 * 2 * 128, 				{1,77,1,0,26,1,4,0,128,255,0,0,0,13,"77 DS/MD"}},
	{(77 * 18 * 2 - 3) * 256 + 3 * 128, {2,77,1,0,18,1,4,1,  0,255,0,0,0,13,"77 DS/DD"}},
	{80 * 18 * 2 * 128, 				{0,80,1,0,18,1,0,0,128,255,0,0,0,13,"80 DS/SD"}},
	{80 * 26 * 2 * 128, 				{1,80,1,0,26,1,4,0,128,255,0,0,0,13,"80 DS/MD"}},
	{(80 * 18 * 2 - 3) * 256 + 3 * 128, {2,80,1,0,18,1,4,1,  0,255,0,0,0,13,"80 DS/DD"}},
	{0, {0,}}
};

/*****************************************************************************
 *
 * Open a floppy image for drive 'drive' if it is not yet openend
 * and a name was given. Determine the image geometry depending on the
 * type of image and store the results into the global drv[] structure
 *
 *****************************************************************************/

#define MAXSIZE 5760 * 256 + 80
static void atari_load_proc(device_image_interface &image)
{
	atari_fdc_t *fdc = get_safe_token(image.device().owner());
	int id = floppy_get_drive(image);
	int size, i;
	const char *ext;

	fdc->drv[id].image = auto_alloc_array(image.device().machine(),UINT8,MAXSIZE);
	if (!fdc->drv[id].image)
		return;

	/* tell whether the image is writable */
	fdc->drv[id].mode = !image.is_readonly();
	/* set up image if it has been created */
	if (image.has_been_created())
	{
		int sector;
		char buff[256];
		memset(buff, 0, sizeof(buff));
		/* default to 720 sectors */
		for( sector = 0; sector < 720; sector++ )
			image.fwrite(buff, 256);
		image.fseek(0, SEEK_SET);
	}

	size = image.fread(fdc->drv[id].image, MAXSIZE);
	if( size <= 0 )
	{
		fdc->drv[id].image = NULL;
		return;
	}
	/* re allocate the buffer; we don't want to be too lazy ;) */
    //fdc->drv[id].image = (UINT8*)image.image_realloc(fdc->drv[id].image, size);

	ext = image.filetype();
    /* no extension: assume XFD format (no header) */
    if (!ext)
    {
        fdc->drv[id].type = FORMAT_XFD;
        fdc->drv[id].header_skip = 0;
    }
    else
    /* XFD extension */
    if( toupper(ext[0])=='X' && toupper(ext[1])=='F' && toupper(ext[2])=='D' )
    {
        fdc->drv[id].type = FORMAT_XFD;
        fdc->drv[id].header_skip = 0;
    }
    else
    /* ATR extension */
    if( toupper(ext[0])=='A' && toupper(ext[1])=='T' && toupper(ext[2])=='R' )
    {
        fdc->drv[id].type = FORMAT_ATR;
        fdc->drv[id].header_skip = 16;
    }
    else
    /* DSK extension */
    if( toupper(ext[0])=='D' && toupper(ext[1])=='S' && toupper(ext[2])=='K' )
    {
        fdc->drv[id].type = FORMAT_DSK;
        fdc->drv[id].header_skip = sizeof(dsk_format);
    }
    else
    {
		fdc->drv[id].type = FORMAT_XFD;
        fdc->drv[id].header_skip = 0;
    }

	if( fdc->drv[id].type == FORMAT_ATR &&
		(fdc->drv[id].image[0] != 0x96 || fdc->drv[id].image[1] != 0x02) )
	{
		fdc->drv[id].type = FORMAT_XFD;
		fdc->drv[id].header_skip = 0;
	}

	switch (fdc->drv[id].type)
	{
	/* XFD or unknown format: find a matching size from the table */
	case FORMAT_XFD:
		for( i = 0; xfd_formats[i].size; i++ )
		{
			if( size == xfd_formats[i].size )
			{
				fdc->drv[id].density = xfd_formats[i].dsk.density;
				fdc->drv[id].tracks = xfd_formats[i].dsk.tracks;
				fdc->drv[id].spt = xfd_formats[i].dsk.spt;
				fdc->drv[id].heads = (xfd_formats[i].dsk.doublesided) ? 2 : 1;
				fdc->drv[id].bseclen = 128;
				fdc->drv[id].seclen = 256 * xfd_formats[i].dsk.seclen_hi + xfd_formats[i].dsk.seclen_lo;
				fdc->drv[id].sectors = fdc->drv[id].tracks * fdc->drv[id].heads * fdc->drv[id].spt;
				break;
			}
		}
		break;
	/* ATR format: find a size including the 16 bytes header */
	case FORMAT_ATR:
		{
			int s;

			fdc->drv[id].bseclen = 128;
			/* get sectors from ATR header */
			s = (size - 16) / 128;
			/* 3 + odd number of sectors ? */
			if ( fdc->drv[id].image[4] == 128 || (s % 18) == 0 || (s % 26) == 0 || ((s - 3) % 1) != 0 )
			{
				fdc->drv[id].sectors = s;
				fdc->drv[id].seclen = 128;
				/* sector size 128 or count not evenly dividable by 26 ? */
				if( fdc->drv[id].seclen == 128 || (s % 26) != 0 )
				{
					/* yup! single density */
					fdc->drv[id].density = 0;
					fdc->drv[id].spt = 18;
					fdc->drv[id].heads = 1;
					fdc->drv[id].tracks = s / 18;
					if( s % 18 != 0 )
						fdc->drv[id].tracks += 1;
					if( fdc->drv[id].tracks % 2 == 0 && fdc->drv[id].tracks > 80 )
					{
						fdc->drv[id].heads = 2;
						fdc->drv[id].tracks /= 2;
					}
				}
				else
				{
					/* yes: medium density */
					fdc->drv[id].density = 0;
					fdc->drv[id].spt = 26;
					fdc->drv[id].heads = 1;
					fdc->drv[id].tracks = s / 26;
					if( s % 26 != 0 )
						fdc->drv[id].tracks += 1;
					if( fdc->drv[id].tracks % 2 == 0 && fdc->drv[id].tracks > 80 )
					{
						fdc->drv[id].heads = 2;
						fdc->drv[id].tracks /= 2;
					}
				}
			}
			else
			{
				/* it's double density */
				s = (s - 3) / 2 + 3;
				fdc->drv[id].sectors = s;
				fdc->drv[id].density = 2;
				fdc->drv[id].seclen = 256;
				fdc->drv[id].spt = 18;
				fdc->drv[id].heads = 1;
				fdc->drv[id].tracks = s / 18;
				if( s % 18 != 0 )
					fdc->drv[id].tracks += 1;
				if( fdc->drv[id].tracks % 2 == 0 && fdc->drv[id].tracks > 80 )
				{
					fdc->drv[id].heads = 2;
					fdc->drv[id].tracks /= 2;
				}
			}
		}
		break;
	/* DSK format: it's all in the header */
	case FORMAT_DSK:
		{
			dsk_format *dsk = (dsk_format *) fdc->drv[id].image;

			fdc->drv[id].tracks = dsk->tracks;
			fdc->drv[id].spt = dsk->spt;
			fdc->drv[id].heads = (dsk->doublesided) ? 2 : 1;
			fdc->drv[id].seclen = 256 * dsk->seclen_hi + dsk->seclen_lo;
			fdc->drv[id].bseclen = fdc->drv[id].seclen;
			fdc->drv[id].sectors = fdc->drv[id].tracks * fdc->drv[id].heads * fdc->drv[id].spt;
		}
		break;
	}
	logerror("atari opened floppy '%s', %d sectors (%d %s%s) %d bytes/sector\n",
			image.filename(),
			fdc->drv[id].sectors,
			fdc->drv[id].tracks,
			(fdc->drv[id].heads == 1) ? "SS" : "DS",
			(fdc->drv[id].density == 0) ? "SD" : (fdc->drv[id].density == 1) ? "MD" : "DD",
			fdc->drv[id].seclen);
	return;
}



/*****************************************************************************
 *
 * This is a description of the data flow between Atari (A) and the
 * Floppy (F) for the supported commands.
 *
 * A->F     DEV  CMD  AUX1 AUX2 CKS
 *          '1'  'S'  00   00                 get status
 * F->A     ACK  CPL  04   FF   E0   00   CKS
 *                     ^    ^
 *                     |    |
 *                     |    bit 7 : door closed
 *                     |
 *                     bit7  : MD 128 bytes/sector, 26 sectors/track
 *                     bit5  : DD 256 bytes/sector, 18 sectors/track
 *                     else  : SD 128 bytes/sector, 18 sectors/track
 *
 * A->F     DEV  CMD  AUX1 AUX2 CKS
 *          '1'  'R'  SECL SECH               read sector
 * F->A     ACK                               command acknowledge
 *               ***                          now read the sector
 * F->A              CPL                      complete: sector read
 * F->A                  128/256 byte CKS
 *
 * A->F     DEV  CMD  AUX1 AUX2 CKS
 *          '1'  'W'  SECL SECH               write with verify
 * F->A     ACK                               command acknowledge
 * A->F          128/256 data CKS
 * F->A                            CPL        complete: CKS okay
 *          execute writing the sector
 * F->A                                 CPL   complete: sector written
 *
 * A->F     DEV  CMD  AUX1 AUX2 CKS
 *          '1'  'P'  SECL SECH               put sector
 * F->A     ACK                               command acknowledge
 * A->F          128/256 data CKS
 * F->A                            CPL        complete: CKS okay
 *          execute writing the sector
 * F->A                                 CPL   complete: sector written
 *
 * A->F     DEV  CMD  AUX1 AUX2 CKS
 *           '1' '!'  xx   xx                 single density format
 * F->A     ACK                               command acknowledge
 *          execute formatting
 * F->A               CPL                     complete: format
 * F->A                    128/256 byte CKS   bad sector table
 *
 *
 * A->F     DEV  CMD  AUX1 AUX2 CKS
 *          '1'  '"'  xx   xx                 double density format
 * F->A     ACK                               command acknowledge
 *          execute formatting
 * F->A               CPL                     complete: format
 * F->A                    128/256 byte CKS   bad sector table
 *
 *****************************************************************************/
static void make_chksum(UINT8 * chksum, UINT8 data)
{
	UINT8 newone;
	newone= *chksum + data;
	if (newone < *chksum)
		newone++;

	if (VERBOSE_CHKSUM)
		logerror("atari chksum old $%02x + data $%02x -> new $%02x\n", *chksum, data, newone);

	*chksum = newone;
}

static void clr_serout(device_t *device,int expect_data)
{
	atari_fdc_t *fdc = get_safe_token(device);

	fdc->serout_chksum = 0;
	fdc->serout_offs = 0;
	fdc->serout_count = expect_data + 1;
}

static void add_serout(device_t *device,int expect_data)
{
	atari_fdc_t *fdc = get_safe_token(device);

	fdc->serout_chksum = 0;
	fdc->serout_count = expect_data + 1;
}

static void clr_serin(device_t *device, int ser_delay)
{
	atari_fdc_t *fdc = get_safe_token(device);
	pokey_device *pokey = device->machine().device<pokey_device>("pokey");
	fdc->serin_chksum = 0;
	fdc->serin_offs = 0;
	fdc->serin_count = 0;
	pokey->serin_ready(ser_delay * 40);
}

static void add_serin(device_t *device,UINT8 data, int with_checksum)
{
	atari_fdc_t *fdc = get_safe_token(device);
	fdc->serin_buff[fdc->serin_count++] = data;
	if (with_checksum)
		make_chksum(&fdc->serin_chksum, data);
}

static void ATTR_PRINTF(2,3) atari_set_frame_message(device_t *device,const char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	vsprintf(atari_frame_message, fmt, arg);
	atari_frame_counter = 30; /* FIXME */

	va_end(arg);
}

static void a800_serial_command(device_t *device)
{
	int i, drive, sector, offset;
	atari_fdc_t *fdc = get_safe_token(device);

	if( !fdc->serout_offs )
	{
		if (VERBOSE_SERIAL)
			logerror("atari serout command offset = 0\n");
		return;
	}
	clr_serin(device, 10);

	if (VERBOSE_SERIAL)
	{
		logerror("atari serout command %d: %02X %02X %02X %02X %02X : %02X ",
			fdc->serout_offs,
			fdc->serout_buff[0], fdc->serout_buff[1], fdc->serout_buff[2],
			fdc->serout_buff[3], fdc->serout_buff[4], fdc->serout_chksum);
	}

	if (fdc->serout_chksum == 0)
	{
		if (VERBOSE_SERIAL)
			logerror("OK\n");

		drive = fdc->serout_buff[0] - '1';   /* drive # */
		/* sector # */
		if (drive < 0 || drive > 3) 			/* ignore unknown drives */
		{
			logerror("atari unsupported drive #%d\n", drive+1);
			atari_set_frame_message(device, "DRIVE #%d not supported", drive+1);
			return;
		}

		/* extract sector number from the command buffer */
		sector = fdc->serout_buff[2] + 256 * fdc->serout_buff[3];

		switch (fdc->serout_buff[1]) /* command ? */
		{
			case 'S':   /* status */
				atari_set_frame_message(device, "DRIVE #%d STATUS", drive+1);

				if (VERBOSE_SERIAL)
					logerror("atari status\n");

				add_serin(device,'A',0);
				add_serin(device,'C',0);
				if (!fdc->drv[drive].mode) /* read only mode ? */
				{
					if (fdc->drv[drive].spt == 26)
						add_serin(device,0x80,1);	/* MD: 0x80 */
					else
					if (fdc->drv[drive].seclen == 128)
						add_serin(device,0x00,1);	/* SD: 0x00 */
					else
						add_serin(device,0x20,1);	/* DD: 0x20 */
				}
				else
				{
					if (fdc->drv[drive].spt == 26)
						add_serin(device,0x84,1);	/* MD: 0x84 */
					else
					if (fdc->drv[drive].seclen == 128)
						add_serin(device,0x04,1);	/* SD: 0x04 */
					else
						add_serin(device,0x24,1);	/* DD: 0x24 */
				}
				if (fdc->drv[drive].image)
					add_serin(device,0xff,1);	/* door closed: 0xff */
				else
					add_serin(device,0x7f,1);	/* door open: 0x7f */
				add_serin(device,0xe0,1);	/* dunno */
				add_serin(device,0x00,1);	/* dunno */
				add_serin(device,fdc->serin_chksum,0);
				break;

			case 'R':   /* read sector */
				if (VERBOSE_SERIAL)
					logerror("atari read sector #%d\n", sector);

				if( sector < 1 || sector > fdc->drv[drive].sectors )
				{
					atari_set_frame_message(device, "DRIVE #%d READ SECTOR #%3d - ERR", drive+1, sector);

					if (VERBOSE_SERIAL)
						logerror("atari bad sector #\n");

					add_serin(device,'E',0);
					break;
				}
				add_serin(device,'A',0);   /* acknowledge */
				add_serin(device,'C',0);   /* completed */
				if (sector < 4) 	/* sector 1 .. 3 might be different length */
				{
					atari_set_frame_message(device, "DRIVE #%d READ SECTOR #%3d - SD", drive+1, sector);
					offset = (sector - 1) * fdc->drv[drive].bseclen + fdc->drv[drive].header_skip;
					for (i = 0; i < 128; i++)
						add_serin(device,fdc->drv[drive].image[offset++],1);
				}
				else
				{
					atari_set_frame_message(device, "DRIVE #%d READ SECTOR #%3d - %cD", drive+1, sector, (fdc->drv[drive].seclen == 128) ? 'S' : 'D');
                    offset = (sector - 1) * fdc->drv[drive].seclen + fdc->drv[drive].header_skip;
					for (i = 0; i < fdc->drv[drive].seclen; i++)
						add_serin(device,fdc->drv[drive].image[offset++],1);
				}
				add_serin(device,fdc->serin_chksum,0);
				break;

			case 'W':   /* write sector with verify */
				if (VERBOSE_SERIAL)
					logerror("atari write sector #%d\n", sector);

				add_serin(device,'A',0);
				if (sector < 4) 	/* sector 1 .. 3 might be different length */
				{
					add_serout(device,fdc->drv[drive].bseclen);
					atari_set_frame_message(device, "DRIVE #%d WRITE SECTOR #%3d - SD", drive+1, sector);
                }
				else
				{
					add_serout(device,fdc->drv[drive].seclen);
					atari_set_frame_message(device, "DRIVE #%d WRITE SECTOR #%3d - %cD", drive+1, sector, (fdc->drv[drive].seclen == 128) ? 'S' : 'D');
                }
				break;

			case 'P':   /* put sector (no verify) */
				if (VERBOSE_SERIAL)
					logerror("atari put sector #%d\n", sector);

				add_serin(device,'A',0);
				if (sector < 4) 	/* sector 1 .. 3 might be different length */
				{
					add_serout(device,fdc->drv[drive].bseclen);
					atari_set_frame_message(device, "DRIVE #%d PUT SECTOR #%3d - SD", drive+1, sector);
                }
				else
				{
					add_serout(device,fdc->drv[drive].seclen);
					atari_set_frame_message(device, "DRIVE #%d PUT SECTOR #%3d - %cD", drive+1, sector, (fdc->drv[drive].seclen == 128) ? 'S' : 'D');
                }
				break;

			case '!':   /* SD format */
				if (VERBOSE_SERIAL)
					logerror("atari format SD drive #%d\n", drive+1);

				atari_set_frame_message(device, "DRIVE #%d FORMAT SD", drive+1);
                add_serin(device,'A',0);   /* acknowledge */
				add_serin(device,'C',0);   /* completed */
				for (i = 0; i < 128; i++)
					add_serin(device,0,1);
				add_serin(device,fdc->serin_chksum,0);
				break;

			case '"':   /* DD format */
				if (VERBOSE_SERIAL)
					logerror("atari format DD drive #%d\n", drive+1);

				atari_set_frame_message(device, "DRIVE #%d FORMAT DD", drive+1);
                add_serin(device,'A',0);   /* acknowledge */
				add_serin(device,'C',0);   /* completed */
				for (i = 0; i < 256; i++)
					add_serin(device,0,1);
				add_serin(device,fdc->serin_chksum,0);
				break;

			default:
				if (VERBOSE_SERIAL)
					logerror("atari unknown command #%c\n", fdc->serout_buff[1]);

				atari_set_frame_message(device, "DRIVE #%d UNKNOWN CMD '%c'", drive+1, fdc->serout_buff[1]);
                add_serin(device,'N',0);   /* negative acknowledge */
		}
	}
	else
	{
		atari_set_frame_message(device, "serial cmd chksum error");
		if (VERBOSE_SERIAL)
			logerror("BAD\n");

		add_serin(device,'E',0);
	}
	if (VERBOSE_SERIAL)
		logerror("atari %d bytes to read\n", fdc->serin_count);
}

static void a800_serial_write(device_t *device)
{
	int i, drive, sector, offset;
	atari_fdc_t *fdc = get_safe_token(device);

	if (VERBOSE_SERIAL)
	{
		logerror("atari serout %d bytes written : %02X ",
			fdc->serout_offs, fdc->serout_chksum);
	}

	clr_serin(device, 80);
	if (fdc->serout_chksum == 0)
	{
		if (VERBOSE_SERIAL)
			logerror("OK\n");

		add_serin(device,'C',0);
		/* write the sector */
		drive = fdc->serout_buff[0] - '1';   /* drive # */
		/* not write protected and image available ? */
		if (fdc->drv[drive].mode && fdc->drv[drive].image)
		{
			/* extract sector number from the command buffer */
			sector = fdc->serout_buff[2] + 256 * fdc->serout_buff[3];
			if (sector < 4) 	/* sector 1 .. 3 might be different length */
			{
				offset = (sector - 1) * fdc->drv[drive].bseclen + fdc->drv[drive].header_skip;

				if (VERBOSE_SERIAL)
					logerror("atari storing 128 byte sector %d at offset 0x%08X", sector, offset );

				for (i = 0; i < 128; i++)
					fdc->drv[drive].image[offset++] = fdc->serout_buff[5+i];
				atari_set_frame_message(device, "DRIVE #%d WROTE SECTOR #%3d - SD", drive+1, sector);
            }
			else
			{
				offset = (sector - 1) * fdc->drv[drive].seclen + fdc->drv[drive].header_skip;

				if (VERBOSE_SERIAL)
					logerror("atari storing %d byte sector %d at offset 0x%08X", fdc->drv[drive].seclen, sector, offset );

				for (i = 0; i < fdc->drv[drive].seclen; i++)
					fdc->drv[drive].image[offset++] = fdc->serout_buff[5+i];
				atari_set_frame_message(device, "DRIVE #%d WROTE SECTOR #%3d - %cD", drive+1, sector, (fdc->drv[drive].seclen == 128) ? 'S' : 'D');
            }
			add_serin(device,'C',0);
		}
		else
		{
			add_serin(device,'E',0);
		}
	}
	else
	{
		if (VERBOSE_SERIAL)
			logerror("BAD\n");

		add_serin(device,'E',0);
	}
}

READ8_DEVICE_HANDLER ( atari_serin_r )
{
	int data = 0x00;
	int ser_delay = 0;
	atari_fdc_t *fdc = get_safe_token(device);

	if (fdc->serin_count)
	{
		pokey_device *pokey = device->machine().device<pokey_device>("pokey");

		data = fdc->serin_buff[fdc->serin_offs];
		ser_delay = 2 * 40;
		if (fdc->serin_offs < 3)
		{
			ser_delay = 4 * 40;
			if (fdc->serin_offs < 2)
				ser_delay = 200 * 40;
		}
		fdc->serin_offs++;
		if (--fdc->serin_count == 0)
			fdc->serin_offs = 0;
		else
			pokey->serin_ready(ser_delay);
	}

	if (VERBOSE_SERIAL)
		logerror("atari serin[$%04x] -> $%02x; delay %d\n", fdc->serin_offs, data, ser_delay);

	return data;
}

WRITE8_DEVICE_HANDLER ( atari_serout_w )
{
	pia6821_device *pia = device->machine().device<pia6821_device>( "pia" );
	atari_fdc_t *fdc = get_safe_token(device);

	/* ignore serial commands if no floppy image is specified */
	if( !fdc->drv[0].image )
		return;
	if (fdc->serout_count)
	{
		/* store data */
		fdc->serout_buff[fdc->serout_offs] = data;

		if (VERBOSE_SERIAL)
			logerror("atari serout[$%04x] <- $%02x; count %d\n", fdc->serout_offs, data, fdc->serout_count);

		fdc->serout_offs++;
		if (--fdc->serout_count == 0)
		{
			/* exclusive or written checksum with calculated */
			fdc->serout_chksum ^= data;
			/* if the attention line is high, this should be data */
			if (pia->irq_b_state())
				a800_serial_write(device);
		}
		else
		{
			make_chksum(&fdc->serout_chksum, data);
		}
	}
}

WRITE_LINE_DEVICE_HANDLER(atarifdc_pia_cb2_w)
{
	atari_fdc_t *fdc = get_safe_token(device);
	if (!state)
	{
		clr_serout(device,4); /* expect 4 command bytes + checksum */
	}
	else
	{
		fdc->serin_delay = 0;
		a800_serial_command(device);
	}
}

static const floppy_interface atari_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	LEGACY_FLOPPY_OPTIONS_NAME(atari_only),
	NULL,
	NULL
};

static MACHINE_CONFIG_FRAGMENT( atari_fdc )
	MCFG_LEGACY_FLOPPY_4_DRIVES_ADD(atari_floppy_interface)
MACHINE_CONFIG_END

device_t *atari_floppy_get_device_child(device_t *device,int drive)
{
	switch(drive) {
		case 0 : return device->subdevice(FLOPPY_0);
		case 1 : return device->subdevice(FLOPPY_1);
		case 2 : return device->subdevice(FLOPPY_2);
		case 3 : return device->subdevice(FLOPPY_3);
	}
	return NULL;
}

static DEVICE_START(atari_fdc)
{
	int id;
	for(id=0;id<4;id++)
	{
		floppy_install_load_proc(atari_floppy_get_device_child(device, id), atari_load_proc);
	}
}

static DEVICE_RESET(atari_fdc)
{
}

DEVICE_GET_INFO( atari_fdc )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;												break;
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(atari_fdc_t);								break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_CONFIG_NAME(atari_fdc);		break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(atari_fdc);					break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(atari_fdc);					break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Atari FDC");								break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Atari FDC");								break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}

DEFINE_LEGACY_DEVICE(ATARI_FDC, atari_fdc);
