/******************************************************************************
	Atari 400/800

    Machine driver

	Juergen Buchmueller, June 1998
******************************************************************************/

#include <ctype.h>
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "mess/machine/atari.h"
#include "mess/vidhrdw/atari.h"

#define VERBOSE_POKEY	0
#define VERBOSE_SERIAL	1
#define VERBOSE_TIMERS	0
#define VERBOSE_CHKSUM	0

#define LOG(x) if (errorlog) fprintf x

POKEY   pokey;
PIA 	pia;

unsigned char *ROM;

typedef struct {
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
}	DRIVE;

DRIVE	drv[4] = {{0}, };

void a800_serial_command(void);
void a800_serial_write(void);
void a800_serial_read(void);

int  a800_cart_loaded = 0;
int  a800_cart_16k = 0;
int  a800_cart_banked = 0;
int  a800xl_mode = 0;
void a800xl_mmu(UINT8 old_mmu, UINT8 new_mmu);

void gtia_reset(void);
void pokey_reset(void);
void atari_pia_reset(void);
void antic_reset(void);

static void make_chksum(UINT8 * chksum, UINT8 data);
static void clr_serout(int expect_data);
static void clr_serin(int ser_delay);
static void add_serin(UINT8 data, int with_checksum);

void a800_setbank(int n)
{
	switch (n)
    {
        case 1:
			cpu_setbankhandler_r(2, MRA_BANK1);
			cpu_setbankhandler_w(2, MWA_ROM);
			cpu_setbank(1, memory_region(REGION_CPU1)+0x10000);
			break;
		case 2:
			cpu_setbankhandler_r(2, MRA_BANK1);
			cpu_setbankhandler_w(2, MWA_ROM);
			cpu_setbank(1, memory_region(REGION_CPU1)+0x12000);
            break;
		default:
			cpu_setbankhandler_r(2, MRA_RAM);
			cpu_setbankhandler_w(2, MWA_RAM);
			cpu_setbank(1, memory_region(REGION_CPU1)+0x0a000);
            break;
    }
}

void a800_init_machine(void)
{
	int i;

	a800xl_mode = 0;

    gtia_reset();
	pokey_reset();
	atari_pia_reset();
	antic_reset();

    if( a800_cart_loaded )
		a800_setbank( 1 );
	for( i = 0; i < 4; i ++ )
		drv[i].image = NULL;
}

int a800_load_rom(void)
{
	static char filename[200];
	void *file;
	int size;
	ROM = memory_region(REGION_CPU1);

	/* load an optional monitor.rom */
	sprintf(filename, "monitor.rom");
	file = osd_fopen(Machine->gamedrv->name, "monitor.rom", OSD_FILETYPE_IMAGE_R, 0);
	if (file)
	{
		LOG((errorlog,"%s loading optional image '%s' to C000-CFFF\n", Machine->gamedrv->name, filename));
		size = osd_fread(file, ROM + 0xc000, 0x1000);
		osd_fclose(file);
	}
	else
	{
		LOG((errorlog,"%s optional image '%s' not found\n", Machine->gamedrv->name, filename));
	}

    /* load an optional (dual) cartidge (e.g. basic.rom) */
	if( strlen(rom_name[0]) )
	{
		strcpy(filename, rom_name[0]);
		file = osd_fopen(Machine->gamedrv->name, filename, OSD_FILETYPE_IMAGE_R, 0);
		if( file )
		{
			size = osd_fread(file, ROM + 0x10000, 0x2000);
            a800_cart_loaded = size / 0x2000;
			size = osd_fread(file, ROM + 0x12000, 0x2000);
			a800_cart_16k = size / 0x2000;
			osd_fclose(file);
			LOG((errorlog,"%s loaded left cartridge '%s' size %s\n",
				Machine->gamedrv->name,
				filename,
				(a800_cart_16k) ? "16K":"8K"));
			if( strlen(rom_name[1]) )
			{
				strcpy(filename, rom_name[1]);
				file = osd_fopen(Machine->gamedrv->name, filename, OSD_FILETYPE_IMAGE_R, 0);
				if( file )
				{
					size = osd_fread(file, ROM + 0x12000, 0x2000);
					a800_cart_16k = (size == 0x2000);
					osd_fclose(file);
					LOG((errorlog,"%s loaded right cartridge '%s' size 16K\n",
						Machine->gamedrv->name,
						filename));
				}
				else
				{
					LOG((errorlog,"%s cartridge '%s' not found\n", Machine->gamedrv->name, filename));
				}
			}
			if( a800_cart_loaded )
				a800_setbank( 1 );
        }
		else
		{
			LOG((errorlog,"%s cartridge '%s' not found\n", Machine->gamedrv->name, filename));
		}
	}
	return 0;
}

int a800_id_rom(const char *name, const char *gamename)
{
	return 1;
}

/**************************************************************
 *
 * Atari 800-XL
 *
 **************************************************************/

void a800xl_init_machine(void)
{
	int i;

	a800xl_mode = 1;

    gtia_reset();
	pokey_reset();
	atari_pia_reset();
	antic_reset();

	for( i = 0; i < 4; i ++ )
		drv[i].image = NULL;
}


int a800xl_load_rom(void)
{
    static char filename[200];
    void *file;
    unsigned size;

    memcpy(ROM + 0x0c000, ROM + 0x10000, 0x01000);
	memcpy(ROM + 0x05000, ROM + 0x11000, 0x00800);
	memcpy(ROM + 0x0d800, ROM + 0x11800, 0x02800);

	sprintf(filename, "basic.rom");
	file = osd_fopen(Machine->gamedrv->name, filename, OSD_FILETYPE_ROM, 0);
	if( file )
    {
		size = osd_fread(file, ROM + 0x14000, 0x2000);
        osd_fclose(file);
		if( size < 0x2000 )
		{
			LOG((errorlog,"%s image '%s' load failed (less than 8K)\n", Machine->gamedrv->name, filename));
			return 2;
		}
	}

    /* load an optional (dual) cartidge (e.g. basic.rom) */
	if( strlen(rom_name[0]) )
    {
        strcpy(filename, rom_name[0]);
        file = osd_fopen(Machine->gamedrv->name, filename, OSD_FILETYPE_IMAGE_R, 0);
        if( file )
        {
			size = osd_fread(file, ROM + 0x14000, 0x2000);
            a800_cart_loaded = size / 0x2000;
			size = osd_fread(file, ROM + 0x16000, 0x2000);
			a800_cart_16k = size / 0x2000;
            osd_fclose(file);
            LOG((errorlog,"%s loaded cartridge '%s' size %s\n",
                Machine->gamedrv->name,
                filename,
				(a800_cart_16k) ? "16K":"8K"));
        }
        else
        {
            LOG((errorlog,"%s cartridge '%s' not found\n", Machine->gamedrv->name, filename));
        }
    }

    return 0;
}

int a800xl_id_rom(const char *name, const char *gamename)
{
	return 1;
}

/**************************************************************
 *
 * Atari 5200 console
 *
 **************************************************************/

void a5200_init_machine(void)
{
	gtia_reset();
	pokey_reset();
	antic_reset();
}

int a5200_load_rom(void)
{
	static char filename[200];
	void *file;
	int size;
	ROM = memory_region(REGION_CPU1);

	/* load an optional (dual) cartidge */
	if( strlen(rom_name[0]) )
	{
		strcpy(filename, rom_name[0]);
		file = osd_fopen(Machine->gamedrv->name, filename, OSD_FILETYPE_IMAGE_R, 0);
		if (file)
		{
			size = osd_fread(file, ROM + 0x4000, 0x8000);
            osd_fclose(file);
			/* move it into upper memory */
			if (size < 0x8000)
			{
				memcpy(ROM + 0x8000, ROM + 0x6000, 0x1000);
				memcpy(ROM + 0xa000, ROM + 0x6000, 0x1000);
				memcpy(ROM + 0x9000, ROM + 0x7000, 0x1000);
				memcpy(ROM + 0xb000, ROM + 0x7000, 0x1000);
				memcpy(ROM + 0x6000, ROM + 0x4000, 0x1000);
				memcpy(ROM + 0x7000, ROM + 0x5000, 0x1000);
			}
			LOG((errorlog,"%s loaded cartridge '%s' size %dK\n",
				Machine->gamedrv->name, filename, size/1024));
        }
		else
		{
			LOG((errorlog,"%s %s not found\n", Machine->gamedrv->name, filename));
		}
	}
	return 0;
}

int a5200_id_rom(const char *name, const char *gamename)
{
	return 1;
}

void pokey_reset(void)
{
	pokey1_w(15,0);
}

#define FORMAT_XFD	0
#define FORMAT_ATR  1
#define FORMAT_DSK  2

/*****************************************************************************
 * This is the structure I used in my own Atari 800 emulator some years ago
 * Though it's a bit overloaded, I used it as it is the maximum of all
 * supported formats:
 * XFD no header at all
 * ATR 16 bytes header
 * DSK this struct
 * It is used to determine the format of a XFD image by it's size only
 *****************************************************************************/

typedef struct {
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
}   dsk_format;

/* combined with the size the image should have */
typedef struct  {
    int size;
    dsk_format dsk;
}   xfd_format;

/* here's a table of known xfd formats */
static  xfd_format xfd_formats[] = {
    {35 * 18 * 1 * 128,
        {0,35,1,0,18,0,0,0,128,255,0,0,0,13,"xfd 35 SS SD"}},
    {35 * 26 * 1 * 128,
        {1,35,1,0,26,0,4,0,128,255,0,0,0,13,"xfd 35 SS MD"}},
    {(35 * 18 * 1 - 3) * 256 + 3 * 128,
        {2,35,1,0,18,0,4,1,  0,255,0,0,0,13,"xfd 35 SS DD"}},
    {40 * 18 * 1 * 128,
        {0,40,1,0,18,0,0,0,128,255,0,0,0,13,"xfd 40 SS SD"}},
    {40 * 26 * 1 * 128,
        {1,40,1,0,26,0,4,0,128,255,0,0,0,13,"xfd 40 SS MD"}},
    {(40 * 18 * 1 - 3) * 256 + 3 * 128,
        {2,40,1,0,18,0,4,1,  0,255,0,0,0,13,"xfd 40 SS DD"}},
    {40 * 18 * 2 * 128,
        {0,40,1,0,18,1,0,0,128,255,0,0,0,13,"xfd 40 DS SD"}},
    {40 * 26 * 2 * 128,
        {1,40,1,0,26,1,4,0,128,255,0,0,0,13,"xfd 40 DS MD"}},
    {(40 * 18 * 2 - 3) * 256 + 3 * 128,
        {2,40,1,0,18,1,4,1,  0,255,0,0,0,13,"xfd 40 DS DD"}},
    {77 * 18 * 1 * 128,
        {0,77,1,0,18,0,0,0,128,255,0,0,0,13,"xfd 77 SS SD"}},
    {77 * 26 * 1 * 128,
        {1,77,1,0,26,0,4,0,128,255,0,0,0,13,"xfd 77 SS MD"}},
    {(77 * 18 * 1 - 3) * 256 + 3 * 128,
        {2,77,1,0,18,0,4,1,  0,255,0,0,0,13,"xfd 77 SS DD"}},
    {77 * 18 * 2 * 128,
        {0,77,1,0,18,1,0,0,128,255,0,0,0,13,"xfd 77 DS SD"}},
    {77 * 26 * 2 * 128,
        {1,77,1,0,26,1,4,0,128,255,0,0,0,13,"xfd 77 DS MD"}},
    {(77 * 18 * 2 - 3) * 256 + 3 * 128,
        {2,77,1,0,18,1,4,1,  0,255,0,0,0,13,"xfd 77 DS DD"}},
    {80 * 18 * 2 * 128,
        {0,80,1,0,18,1,0,0,128,255,0,0,0,13,"xfd 80 DS SD"}},
    {80 * 26 * 2 * 128,
        {1,80,1,0,26,1,4,0,128,255,0,0,0,13,"xfd 80 DS MD"}},
    {(80 * 18 * 2 - 3) * 256 + 3 * 128,
        {2,80,1,0,18,1,4,1,  0,255,0,0,0,13,"xfd 80 DS DD"}},
    {0, {0,}}
};


/*****************************************************************************
 *
 * Open a floppy image for drive 'drive' if it is not yet openend
 * and a name was given. Determine the image geometry depending on the
 * type of image and store the results into the global drv[] structure
 *
 *****************************************************************************/
static  void open_floppy(int drive)
{
#define MAXSIZE 2880 * 256 + 80
	char * ext;
	UINT8 * image;
	void * file;
	int size, i;

	if( !drv[drive].image )
    {
        if (!strlen(floppy_name[drive]))
            return;
        ext = strrchr(floppy_name[drive], '.');
        /* no extension: assume XFD format (no header) */
        if (!ext)
        {
			drv[drive].type = FORMAT_XFD;
			drv[drive].header_skip = 0;
        }
        else
        /* XFD extension */
		if( toupper(ext[1])=='X' && toupper(ext[2])=='F' && toupper(ext[3])=='D' )
        {
			drv[drive].type = FORMAT_XFD;
			drv[drive].header_skip = 0;
        }
        else
        /* ATR extension */
		if( toupper(ext[1])=='A' && toupper(ext[2])=='T' && toupper(ext[3])=='R' )
        {
			drv[drive].type = FORMAT_ATR;
			drv[drive].header_skip = 16;
        }
        else
        /* DSK extension */
		if( toupper(ext[1])=='D' && toupper(ext[2])=='S' && toupper(ext[3])=='K' )
        {
			drv[drive].type = FORMAT_DSK;
			drv[drive].header_skip = sizeof(dsk_format);
        }
        else
        {
            return;
        }

        image = malloc(MAXSIZE);
        if (!image)
            return;
        /* try to open the image read/write */
		drv[drive].mode = 1;
		file = osd_fopen(Machine->gamedrv->name, floppy_name[drive], OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_RW);
        if (!file)
        {
            /* if this fails, try to open it read only */
			drv[drive].mode = 0;
            file = osd_fopen(Machine->gamedrv->name, floppy_name[drive], OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
        }
		/* still failed, so create a new image */
        if (!file)
        {
			/* if this fails, try to open it read only */
			drv[drive].mode = 1;
			file = osd_fopen(Machine->gamedrv->name, floppy_name[drive], OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_WRITE);
			if( file )
			{
				int sector;
				char buff[256];
				memset(buff, 0, 256);
				/* default to 720 sectors */
				for( sector = 0; sector < 720; sector++ )
					osd_fwrite(file, buff, 256);
				osd_fseek(file, 0, SEEK_SET);
			}
        }
        size = osd_fread(file, image, MAXSIZE);
        osd_fclose(file);
		if( size <= 0 )
        {
            free(image);
            return;
        }
        /* re allocate the buffer; we don't want to be too lazy ;) */
        image = realloc(image, size);
		drv[drive].image = image;

		if( drv[drive].type == FORMAT_ATR )
		{
			if( image[0] != 0x96 || image[1] != 0x02 )
			{
				drv[drive].type = FORMAT_XFD;
				drv[drive].header_skip = 0;
			}
		}

        switch (drv[drive].type)
        {
            /* XFD or unknown format: find a matching size from the table */
            case FORMAT_XFD:
				for( i = 0; xfd_formats[i].size; i++ )
                {
					if( size == xfd_formats[i].size )
                    {
						drv[drive].density = xfd_formats[i].dsk.density;
						drv[drive].tracks = xfd_formats[i].dsk.tracks;
						drv[drive].spt = xfd_formats[i].dsk.spt;
						drv[drive].heads = (xfd_formats[i].dsk.doublesided) ? 2 : 1;
						drv[drive].bseclen = 128;
						drv[drive].seclen = 256 * xfd_formats[i].dsk.seclen_hi + xfd_formats[i].dsk.seclen_lo;
						drv[drive].sectors = drv[drive].tracks * drv[drive].heads * drv[drive].spt;
                        break;
                    }
                }
                break;
            /* ATR format: find a size including the 16 bytes header */
            case FORMAT_ATR:
				{
					int s;

                    drv[drive].bseclen = 128;
					/* get sectors from ATR header */
					s = (size - 16) / 128;
					/* 3 + odd number of sectors ? */
					if ( image[4] == 128 || (s % 18) == 0 || (s % 26) == 0 || ((s - 3) % 1) != 0 )
					{
						drv[drive].sectors = s;
						drv[drive].seclen = 128;
                        /* sector size 128 or count not evenly dividable by 26 ? */
						if( drv[drive].seclen == 128 || (s % 26) != 0 )
						{
							/* yup! single density */
							drv[drive].density = 0;
							drv[drive].spt = 18;
							drv[drive].heads = 1;
							drv[drive].tracks = s / 18;
							if( s % 18 != 0 )
								drv[drive].tracks += 1;
							if( drv[drive].tracks % 2 == 0 && drv[drive].tracks > 80 )
							{
								drv[drive].heads = 2;
								drv[drive].tracks /= 2;
							}
						}
						else
						{
							/* yes: medium density */
							drv[drive].density = 0;
							drv[drive].spt = 26;
							drv[drive].heads = 1;
							drv[drive].tracks = s / 26;
							if( s % 26 != 0 )
								drv[drive].tracks += 1;
							if( drv[drive].tracks % 2 == 0 && drv[drive].tracks > 80 )
							{
								drv[drive].heads = 2;
								drv[drive].tracks /= 2;
							}
						}
					}
					else
					{
						/* it's double density */
						s = (s - 3) / 2 + 3;
						drv[drive].sectors = s;
						drv[drive].density = 2;
						drv[drive].seclen = 256;
						drv[drive].spt = 18;
						drv[drive].heads = 1;
						drv[drive].tracks = s / 18;
						if( s % 18 != 0 )
							drv[drive].tracks += 1;
						if( drv[drive].tracks % 2 == 0 && drv[drive].tracks > 80 )
						{
							drv[drive].heads = 2;
							drv[drive].tracks /= 2;
						}
					}
				}
                break;
            /* DSK format: it's all in the header */
            case FORMAT_DSK:
                {
					dsk_format *dsk = (dsk_format *) image;

                    drv[drive].tracks = dsk->tracks;
					drv[drive].spt = dsk->spt;
					drv[drive].heads = (dsk->doublesided) ? 2 : 1;
					drv[drive].seclen = 256 * dsk->seclen_hi + dsk->seclen_lo;
					drv[drive].bseclen = drv[drive].seclen;
					drv[drive].sectors = drv[drive].tracks * drv[drive].heads * drv[drive].spt;
                }
                break;
        }
		LOG((errorlog,"atari opened image #%d '%s', %d sectors (%d %s%s) %d bytes/sector\n",
                drive+1,
                floppy_name[drive],
				drv[drive].sectors,
				drv[drive].tracks,
				(drv[drive].heads == 1) ? "SS" : "DS",
				(drv[drive].density == 0) ? "SD" : (drv[drive].density == 1) ? "MD" : "DD",
				drv[drive].seclen));
    }
}

void a800_close_floppy (void)
{
	int i;

    for (i = 0; i < 4; i ++)
    {
		if (drv[i].image)
        {
			free(drv[i].image);
			drv[i].image = NULL;
        }
    }
}

static  void make_chksum(UINT8 * chksum, UINT8 data)
{
UINT8 new;
    new = *chksum + data;
    if (new < *chksum)
        new++;
#if VERBOSE_CHKSUM
	LOG((errorlog,"atari chksum old $%02x + data $%02x -> new $%02x\n", *chksum, data, new));
#endif
    *chksum = new;
}

static  void clr_serout(int expect_data)
{
    pokey.serout_chksum = 0;
    pokey.serout_offs = 0;
    pokey.serout_count = expect_data + 1;
}

static  void add_serout(int expect_data)
{
    pokey.serout_chksum = 0;
    pokey.serout_count = expect_data + 1;
}

static	void clr_serin(int ser_delay)
{
    pokey.serin_chksum = 0;
    pokey.serin_offs = 0;
    pokey.serin_count = 0;
	pokey1_serin_ready(ser_delay * 40);
}

static  void add_serin(UINT8 data, int with_checksum)
{
    pokey.serin_buff[pokey.serin_count++] = data;
    if (with_checksum)
        make_chksum(&pokey.serin_chksum, data);
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

void a800_serial_command(void)
{
	int  i, drive, sector, offset;

	if( !pokey.serout_offs )
    {
#if VERBOSE_SERIAL
	LOG((errorlog,"atari serout command offset = 0\n"));
#endif
        return;
    }
    clr_serin(10);
#if VERBOSE_SERIAL
	LOG((errorlog,"atari serout command %d: %02X %02X %02X %02X %02X : %02X ",
        pokey.serout_offs,
        pokey.serout_buff[0], pokey.serout_buff[1], pokey.serout_buff[2],
		pokey.serout_buff[3], pokey.serout_buff[4], pokey.serout_chksum));
#endif
    if (pokey.serout_chksum == 0)
    {
#if VERBOSE_SERIAL
		LOG((errorlog,"OK\n"));
#endif
        drive = pokey.serout_buff[0] - '1';   /* drive # */
        open_floppy(drive);
        /* sector # */
        if (drive < 0 || drive > 3)             /* ignore unknown drives */
        {
			LOG((errorlog,"atari unsupported drive #%d\n", drive+1));
            return;
        }

        /* extract sector number from the command buffer */
        sector = pokey.serout_buff[2] + 256 * pokey.serout_buff[3];

        switch (pokey.serout_buff[1]) /* command ? */
        {
            case 'S':   /* status */
#if VERBOSE_SERIAL
				LOG((errorlog,"atari status\n"));
#endif
                add_serin('A',0);
                add_serin('C',0);
				if (!drv[drive].mode) /* read only mode ? */
                {
					if (drv[drive].spt == 26)
                        add_serin(0x80,1);  /* MD: 0x80 */
                    else
					if (drv[drive].seclen == 128)
                        add_serin(0x00,1);  /* SD: 0x00 */
                    else
                        add_serin(0x20,1);  /* DD: 0x20 */
                }
                else
                {
					if (drv[drive].spt == 26)
                        add_serin(0x84,1);  /* MD: 0x84 */
                    else
					if (drv[drive].seclen == 128)
                        add_serin(0x04,1);  /* SD: 0x04 */
                    else
                        add_serin(0x24,1);  /* DD: 0x24 */
                }
				if (drv[drive].image)
                    add_serin(0xff,1);  /* door closed: 0xff */
                else
                    add_serin(0x7f,1);  /* door open: 0x7f */
                add_serin(0xe0,1);  /* dunno */
                add_serin(0x00,1);  /* dunno */
                add_serin(pokey.serin_chksum,0);
                break;

            case 'R':   /* read sector */
#if VERBOSE_SERIAL
				LOG((errorlog,"atari read sector #%d\n", sector));
#endif
				if( sector < 1 || sector > drv[drive].sectors )
                {
#if VERBOSE_SERIAL
					LOG((errorlog,"atari bad sector #\n"));
#endif
                    add_serin('E',0);
                    break;
                }
                add_serin('A',0);   /* acknowledge */
                add_serin('C',0);   /* completed */
                if (sector < 4)     /* sector 1 .. 3 might be different length */
                {
					offset = (sector - 1) * drv[drive].bseclen + drv[drive].header_skip;
                    for (i = 0; i < 128; i++)
						add_serin(drv[drive].image[offset++],1);
                }
                else
                {
					offset = (sector - 1) * drv[drive].seclen + drv[drive].header_skip;
					for (i = 0; i < drv[drive].seclen; i++)
						add_serin(drv[drive].image[offset++],1);
                }
                add_serin(pokey.serin_chksum,0);
                break;

            case 'W':   /* write sector with verify */
#if VERBOSE_SERIAL
				LOG((errorlog,"atari write sector #%d\n", sector));
#endif
                add_serin('A',0);
                if (sector < 4)     /* sector 1 .. 3 might be different length */
					add_serout(drv[drive].bseclen);
                else
					add_serout(drv[drive].seclen);
                break;

            case 'P':   /* put sector (no verify) */
#if VERBOSE_SERIAL
				LOG((errorlog,"atari put sector #%d\n", sector));
#endif
                add_serin('A',0);
                if (sector < 4)     /* sector 1 .. 3 might be different length */
					add_serout(drv[drive].bseclen);
                else
					add_serout(drv[drive].seclen);
                break;

            case '!':   /* SD format */
#if VERBOSE_SERIAL
				LOG((errorlog,"atari format SD drive #%d\n", drive+1));
#endif
                add_serin('A',0);   /* acknowledge */
                add_serin('C',0);   /* completed */
                for (i = 0; i < 128; i++)
                    add_serin(0,1);
                add_serin(pokey.serin_chksum,0);
                break;

            case '"':   /* DD format */
#if VERBOSE_SERIAL
				LOG((errorlog,"atari format DD drive #%d\n", drive+1));
#endif
                add_serin('A',0);   /* acknowledge */
                add_serin('C',0);   /* completed */
                for (i = 0; i < 256; i++)
                    add_serin(0,1);
                add_serin(pokey.serin_chksum,0);
                break;

            default:
#if VERBOSE_SERIAL
				LOG((errorlog,"atari unknown command #%c\n", pokey.serout_buff[1]));
#endif
                add_serin('N',0);   /* negative acknowledge */
        }
    }
    else
    {
#if VERBOSE_SERIAL
		LOG((errorlog,"BAD\n"));
#endif
        add_serin('E',0);
    }
#if VERBOSE_SERIAL
	LOG((errorlog,"atari %d bytes to read\n", pokey.serin_count));
#endif
}

void a800_serial_write(void)
{
int i, drive, sector, offset;
#if VERBOSE_SERIAL
	LOG((errorlog,"atari serout %d bytes written : %02X ",
		pokey.serout_offs, pokey.serout_chksum));
#endif
    clr_serin(80);
    if (pokey.serout_chksum == 0)
    {
#if VERBOSE_SERIAL
		LOG((errorlog,"OK\n"));
#endif
        add_serin('C',0);
        /* write the sector */
        drive = pokey.serout_buff[0] - '1';   /* drive # */
        /* not write protected and image available ? */
		if (drv[drive].mode && drv[drive].image)
        {
            /* extract sector number from the command buffer */
            sector = pokey.serout_buff[2] + 256 * pokey.serout_buff[3];
            if (sector < 4)     /* sector 1 .. 3 might be different length */
            {
				offset = (sector - 1) * drv[drive].bseclen + drv[drive].header_skip;
#if VERBOSE_SERIAL
				LOG((errorlog,"atari storing 128 byte sector %d at offset 0x%08X",
					sector, offset ));
#endif
                for (i = 0; i < 128; i++)
					drv[drive].image[offset++] = pokey.serout_buff[5+i];
            }
            else
            {
				offset = (sector - 1) * drv[drive].seclen + drv[drive].header_skip;
#if VERBOSE_SERIAL
				LOG((errorlog,"atari storing %d byte sector %d at offset 0x%08X",
					drv[drive].seclen, sector, offset ));
#endif
				for (i = 0; i < drv[drive].seclen; i++)
					drv[drive].image[offset++] = pokey.serout_buff[5+i];
            }
            add_serin('C',0);
        }
        else
        {
            add_serin('E',0);
        }
    }
    else
    {
#if VERBOSE_SERIAL
		LOG((errorlog,"BAD\n"));
#endif
        add_serin('E',0);
    }
}

int atari_serin_r(int offset)
{
	int data = 0x00;
	int ser_delay = 0;

	if (pokey.serin_count)
	{
		data = pokey.serin_buff[pokey.serin_offs];
		ser_delay = 2 * 40;
		if (pokey.serin_offs < 3)
		{
			ser_delay = 4 * 40;
			if (pokey.serin_offs < 2)
				ser_delay = 200 * 40;
		}
		pokey.serin_offs++;
		if (--pokey.serin_count == 0)
			pokey.serin_offs = 0;
		else
			pokey1_serin_ready(ser_delay);
    }
#if VERBOSE_SERIAL
	LOG((errorlog,"atari serin[$%04x] -> $%02x; delay %d\n", pokey.serin_offs, data, ser_delay));
#endif
    return data;
}

void    atari_serout_w(int offset, int data)
{
	if (pokey.serout_count)
	{
		/* store data */
		pokey.serout_buff[pokey.serout_offs] = data;
#if VERBOSE_SERIAL
		LOG((errorlog,"atari serout[$%04x] <- $%02x; count %d\n", pokey.serout_offs, data, pokey.serout_count));
#endif
		pokey.serout_offs++;
		if (--pokey.serout_count == 0)
		{
			/* exclusive or written checksum with calculated */
			pokey.serout_chksum ^= data;
			/* if the attention line is high, this should be data */
			if (pia.rw.pbctl & 0x08)
                a800_serial_write();
		}
		else
		{
			make_chksum(&pokey.serout_chksum, data);
		}
	}
}

void    atari_interrupt_cb(int mask)
{
	if (errorlog)
	{
#if VERBOSE_POKEY
		if (mask & 0x80)
			fprintf(errorlog, "atari interrupt_cb BREAK\n");
		if (mask & 0x40)
			fprintf(errorlog, "atari interrupt_cb KBCOD\n");
#endif
#if VERBOSE_SERIAL
		if (mask & 0x20)
			fprintf(errorlog, "atari interrupt_cb SERIN\n");
		if (mask & 0x10)
			fprintf(errorlog, "atari interrupt_cb SEROR\n");
		if (mask & 0x08)
			fprintf(errorlog, "atari interrupt_cb SEROC\n");
#endif
#if VERBOSE_TIMERS
		if (mask & 0x04)
			fprintf(errorlog, "atari interrupt_cb TIMR4\n");
		if (mask & 0x02)
			fprintf(errorlog, "atari interrupt_cb TIMR2\n");
		if (mask & 0x01)
			fprintf(errorlog, "atari interrupt_cb TIMR1\n");
#endif
	}
	cpu_cause_interrupt(0, M6502_INT_IRQ);
}

/**************************************************************
 *
 * Read PIA hardware registers
 *
 **************************************************************/

int MRA_PIA(int offset)
{
    switch (offset & 3)
    {
        case 0: /* PIA port A */
            pia.r.painp = input_port_1_r(0);
            return (pia.w.paout & pia.h.pamsk) | (pia.r.painp & ~pia.h.pamsk);
        case 1: /* PIA port B */
            pia.r.pbinp = input_port_2_r(0);
            return (pia.w.pbout & pia.h.pbmsk) | (pia.r.pbinp & ~pia.h.pbmsk);
        case 2: /* PIA port A control */
            return pia.rw.pactl;
        case 3: /* PIA port B control */
            return pia.rw.pbctl;
    }
    return 0xff;
}

/**************************************************************
 *
 * Write PIA hardware registers
 *
 **************************************************************/

void MWA_PIA(int offset, int data)
{
    switch (offset & 3)
    {
		case 0: /* PIA port A */
			if (pia.rw.pactl & 0x04)
				pia.w.paout = data;   /* output */
            else
				pia.h.pamsk = data;   /* mask register */
            break;
		case 1: /* PIA port B */
			if( pia.rw.pbctl & 0x04 )
            {
				if( a800xl_mode )
					a800xl_mmu(pia.w.pbout, data);
				pia.w.pbout = data;   /* output */
            }
            else
            {
				pia.h.pbmsk = data;   /* 400/800 mode mask register */
            }
            break;
		case 2: /* PIA port A control */
			pia.rw.pactl = data;
            break;
		case 3: /* PIA port B control */
			if( (pia.rw.pbctl ^ data) & 0x08 )	/* serial attention change ? */
			{
				if( pia.rw.pbctl & 0x08 )		/* serial attention before ? */
				{
					clr_serout(4);				/* expect 4 command bytes + checksum */
				}
				else
				{
					pokey.serin_delay = 0;
					a800_serial_command();
				}
			}
			pia.rw.pbctl = data;
            break;
    }
}

/**************************************************************
 *
 * Reset hardware
 *
 **************************************************************/
void	atari_pia_reset(void)
{
    /* reset the PIA */
	MWA_PIA(3,0);
	MWA_PIA(2,0);
	MWA_PIA(1,0);
	MWA_PIA(0,0);
}

void a800xl_mmu(UINT8 old_mmu, UINT8 new_mmu)
{
	UINT8 changes = new_mmu ^ old_mmu;

	if( changes == 0 )
		return;

	LOG((errorlog,"%s MMU old:%02x new:%02x\n", Machine->gamedrv->name, old_mmu, new_mmu));

    /* check if memory C000-FFFF changed */
    if( changes & 0x01 )
	{
		if( new_mmu & 0x01 )
		{
			LOG((errorlog,"%s MMU BIOS ROM\n", Machine->gamedrv->name));
			cpu_setbankhandler_r(3, MRA_BANK3);
			cpu_setbankhandler_w(3, MWA_ROM);
            cpu_setbank(3, memory_region(REGION_CPU1)+0x10000);  /* 0x1000 bytes */
			cpu_setbankhandler_r(4, MRA_BANK4);
			cpu_setbankhandler_w(4, MWA_ROM);
            cpu_setbank(4, memory_region(REGION_CPU1)+0x11800);  /* 0x2800 bytes */
        }
		else
		{
			LOG((errorlog,"%s MMU BIOS RAM\n", Machine->gamedrv->name));
            cpu_setbankhandler_r(3, MRA_RAM);
			cpu_setbankhandler_w(3, MWA_RAM);
            cpu_setbank(3, memory_region(REGION_CPU1)+0x0c000);  /* 0x1000 bytes */
			cpu_setbankhandler_r(4, MRA_RAM);
			cpu_setbankhandler_w(4, MWA_RAM);
            cpu_setbank(4, memory_region(REGION_CPU1)+0x0d800);  /* 0x2800 bytes */
        }
	}
	/* check if BASIC changed */
    if( changes & 0x02 )
	{
		if( new_mmu & 0x02 )
        {
			LOG((errorlog,"%s MMU BASIC RAM\n", Machine->gamedrv->name));
            cpu_setbankhandler_r(2, MRA_RAM);
            cpu_setbankhandler_w(2, MWA_RAM);
            cpu_setbank(2, memory_region(REGION_CPU1)+0x0a000);  /* 0x2000 bytes */
        }
		else
		{
			LOG((errorlog,"%s MMU BASIC ROM\n", Machine->gamedrv->name));
            cpu_setbankhandler_r(2, MRA_BANK2);
            cpu_setbankhandler_w(2, MWA_ROM);
            cpu_setbank(2, memory_region(REGION_CPU1)+0x14000);  /* 0x2000 bytes */
		}
    }
	/* check if self-test ROM changed */
	if( changes & 0x80 )
	{
		if( new_mmu & 0x80 )
        {
			LOG((errorlog,"%s MMU SELFTEST RAM\n", Machine->gamedrv->name));
            cpu_setbankhandler_r(1, MRA_RAM);
            cpu_setbankhandler_w(1, MWA_RAM);
            cpu_setbank(1, memory_region(REGION_CPU1)+0x05000);  /* 0x0800 bytes */
        }
		else
		{
			LOG((errorlog,"%s MMU SELFTEST ROM\n", Machine->gamedrv->name));
            cpu_setbankhandler_r(1, MRA_BANK1);
            cpu_setbankhandler_w(1, MWA_ROM);
            cpu_setbank(1, memory_region(REGION_CPU1)+0x11000);  /* 0x0800 bytes */
        }
    }
}

/**************************************************************
 *
 * Keyboard
 *
 **************************************************************/

#define AKEY_L			0x00
#define AKEY_J          0x01
#define AKEY_SEMICOLON  0x02
#define AKEY_BREAK		0x03	/* this not really a scancode */
#define AKEY_K          0x05
#define AKEY_PLUS       0x06
#define AKEY_ASTERISK   0x07
#define AKEY_O          0x08
#define AKEY_NONE       0x09
#define AKEY_P          0x0a
#define AKEY_U          0x0b
#define AKEY_ENTER      0x0c
#define AKEY_I          0x0d
#define AKEY_MINUS      0x0e
#define AKEY_EQUALS     0x0f
#define AKEY_V          0x10
#define AKEY_C          0x12
#define AKEY_B          0x15
#define AKEY_X          0x16
#define AKEY_Z          0x17
#define AKEY_4          0x18
#define AKEY_3          0x1a
#define AKEY_6          0x1b
#define AKEY_ESC        0x1c
#define AKEY_5          0x1d
#define AKEY_2          0x1e
#define AKEY_1          0x1f
#define AKEY_COMMA      0x20
#define AKEY_SPACE      0x21
#define AKEY_STOP       0x22
#define AKEY_N          0x23
#define AKEY_M          0x25
#define AKEY_SLASH      0x26
#define AKEY_ATARI      0x27
#define AKEY_R          0x28
#define AKEY_E          0x2a
#define AKEY_Y          0x2b
#define AKEY_TAB        0x2c
#define AKEY_T          0x2d
#define AKEY_W          0x2e
#define AKEY_Q          0x2f
#define AKEY_9          0x30
#define AKEY_0          0x32
#define AKEY_7          0x33
#define AKEY_BSP        0x34
#define AKEY_8          0x35
#define AKEY_LESSER     0x36
#define AKEY_GREATER    0x37
#define AKEY_F          0x38
#define AKEY_H          0x39
#define AKEY_D          0x3a
#define AKEY_CAPS       0x3c
#define AKEY_G          0x3d
#define AKEY_S          0x3e
#define AKEY_A          0x3f
#define ASHF_L          0x40
#define ASHF_J          0x41
#define ASHF_COLON      0x42
#define ASHF_BREAK		0x43	/* this not really a scancode */
#define ASHF_K          0x45
#define ASHF_BACKSLASH  0x46
#define ASHF_TILDE      0x47
#define ASHF_O          0x48
#define ASHF_SHIFT      0x49
#define ASHF_P          0x4a
#define ASHF_U          0x4b
#define ASHF_ENTER      0x4c
#define ASHF_I          0x4d
#define ASHF_UNDERSCORE 0x4e
#define ASHF_BAR        0x4f
#define ASHF_V          0x50
#define ASHF_C          0x52
#define ASHF_B          0x55
#define ASHF_X          0x56
#define ASHF_Z          0x57
#define ASHF_DOLLAR     0x58
#define ASHF_HASH       0x5a
#define ASHF_AMPERSAND  0x5b
#define ASHF_ESC        0x5c
#define ASHF_PERCENT    0x5d
#define ASHF_DQUOTE     0x5e
#define ASHF_EXCLAM     0x5f
#define ASHF_LBRACE     0x60
#define ASHF_SPACE      0x61
#define ASHF_RBRACE     0x62
#define ASHF_N          0x63
#define ASHF_M          0x65
#define ASHF_QUESTION   0x66
#define ASHF_ATARI      0x67
#define ASHF_R          0x68
#define ASHF_E          0x6a
#define ASHF_Y          0x6b
#define ASHF_TAB        0x6c
#define ASHF_T          0x6d
#define ASHF_W          0x6e
#define ASHF_Q          0x6f
#define ASHF_LPAREN     0x70
#define ASHF_RPAREN     0x72
#define ASHF_QUOTE      0x73
#define ASHF_BSP        0x74
#define ASHF_AT         0x75
#define ASHF_CLEAR      0x76
#define ASHF_INSERT     0x77
#define ASHF_F          0x78
#define ASHF_H          0x79
#define ASHF_D          0x7a
#define ASHF_CAPS       0x7c
#define ASHF_G          0x7d
#define ASHF_S          0x7e
#define ASHF_A          0x7f
#define ACTL_L          0x80
#define ACTL_J          0x81
#define ACTL_SEMICOLON  0x82
#define ACTL_BREAK		0x83	/* this not really a scancode */
#define ACTL_K          0x85
#define ACTL_PLUS       0x86
#define ACTL_ASTERISK   0x87
#define ACTL_O          0x88
#define ACTL_CONTROL    0x89
#define ACTL_P          0x8a
#define ACTL_U          0x8b
#define ACTL_ENTER      0x8c
#define ACTL_I          0x8d
#define ACTL_MINUS      0x8e
#define ACTL_EQUALS     0x8f
#define ACTL_V          0x90
#define ACTL_C          0x92
#define ACTL_B          0x95
#define ACTL_X          0x96
#define ACTL_Z          0x97
#define ACTL_4          0x98
#define ACTL_3          0x9a
#define ACTL_6          0x9b
#define ACTL_ESC        0x9c
#define ACTL_5          0x9d
#define ACTL_2          0x9e
#define ACTL_1          0x9f
#define ACTL_COMMA      0xa0
#define ACTL_SPACE      0xa1
#define ACTL_STOP       0xa2
#define ACTL_N          0xa3
#define ACTL_M          0xa5
#define ACTL_SLASH      0xa6
#define ACTL_ATARI      0xa7
#define ACTL_R          0xa8
#define ACTL_E          0xaa
#define ACTL_Y          0xab
#define ACTL_TAB        0xac
#define ACTL_T          0xad
#define ACTL_W          0xae
#define ACTL_Q          0xaf
#define ACTL_9          0xb0
#define ACTL_0          0xb2
#define ACTL_7          0xb3
#define ACTL_BSP        0xb4
#define ACTL_8          0xb5
#define ACTL_LESSER     0xb6
#define ACTL_GREATER    0xb7
#define ACTL_F          0xb8
#define ACTL_H          0xb9
#define ACTL_D          0xba
#define ACTL_CAPS       0xbc
#define ACTL_G          0xbd
#define ACTL_S          0xbe
#define ACTL_A          0xbf
#define ACSH_L          0xc0
#define ACSH_J          0xc1
#define ACSH_COLON      0xc2
#define ACSH_BREAK		0x84	/* this not really a scancode */
#define ACSH_K          0xc5
#define ACSH_BACKSLASH  0xc6
#define ACSH_TILDE      0xc7
#define ACSH_O          0xc8
#define ACSH_CTRLSHIFT  0xc9
#define ACSH_P          0xca
#define ACSH_U          0xcb
#define ACSH_ENTER      0xcc
#define ACSH_I          0xcd
#define ACSH_UNDERSCORE 0xce
#define ACSH_BAR        0xcf
#define ACSH_V          0xd0
#define ACSH_C          0xd2
#define ACSH_B          0xd5
#define ACSH_X          0xd6
#define ACSH_Z          0xd7
#define ACSH_DOLLAR     0xd8
#define ACSH_HASH       0xda
#define ACSH_AMPERSAND  0xdb
#define ACSH_ESC        0xdc
#define ACSH_PERCENT    0xdd
#define ACSH_DQUOTE     0xde
#define ACSH_EXCLAM     0xdf
#define ACSH_LBRACE     0xe0
#define ACSH_SPACE      0xe1
#define ACSH_RBRACE     0xe2
#define ACSH_N          0xe3
#define ACSH_M          0xe5
#define ACSH_QUESTION   0xe6
#define ACSH_ATARI      0xe7
#define ACSH_R          0xe8
#define ACSH_E          0xea
#define ACSH_Y          0xeb
#define ACSH_TAB        0xec
#define ACSH_T          0xed
#define ACSH_W          0xee
#define ACSH_Q          0xef
#define ACSH_LPAREN     0xf0
#define ACSH_RPAREN     0xf2
#define ACSH_QUOTE      0xf3
#define ACSH_BSP        0xf4
#define ACSH_AT         0xf5
#define ACSH_CLEAR      0xf6
#define ACSH_INSERT     0xf7
#define ACSH_F          0xf8
#define ACSH_H          0xf9
#define ACSH_D          0xfa
#define ACSH_CAPS       0xfc
#define ACSH_G          0xfd
#define ACSH_S          0xfe
#define ACSH_A          0xff

typedef struct {
	int osd;
	int joystick;
	int atari[4];
}	key_trans;

static  key_trans key_trans_de[] = {
{0, 				0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_ESC,		0, {AKEY_ESC		,ASHF_ESC		 ,ACTL_ESC		  ,ACTL_ESC 	   }},
{KEYCODE_1, 		0, {AKEY_1			,ASHF_EXCLAM	 ,ACTL_1		  ,ACTL_1		   }},
{KEYCODE_2, 		0, {AKEY_2			,ASHF_DQUOTE	 ,ACTL_2		  ,ACTL_2		   }},
{KEYCODE_3, 		0, {AKEY_3			,ASHF_AT		 ,ACTL_3		  ,ACTL_3		   }},
{KEYCODE_4, 		0, {AKEY_4			,ASHF_DOLLAR	 ,ACTL_4		  ,ACTL_4		   }},
{KEYCODE_5, 		0, {AKEY_5			,ASHF_PERCENT	 ,ACTL_5		  ,ACTL_5		   }},
{KEYCODE_6, 		0, {AKEY_6			,ASHF_AMPERSAND  ,ACTL_6		  ,ACTL_6		   }},
{KEYCODE_7, 		0, {AKEY_7			,AKEY_SLASH 	 ,ACTL_7		  ,ACTL_7		   }},
{KEYCODE_8, 		0, {AKEY_8			,ASHF_LPAREN	 ,ACTL_8		  ,ACTL_8		   }},
{KEYCODE_9, 		0, {AKEY_9			,ASHF_RPAREN	 ,ACTL_9		  ,ACTL_9		   }},
{KEYCODE_0, 		0, {AKEY_0			,AKEY_EQUALS	 ,ACTL_0		  ,ACTL_0		   }},
{KEYCODE_MINUS, 	0, {AKEY_GREATER	,ASHF_QUESTION	 ,ACTL_SLASH	  ,ACTL_SLASH	   }},
{KEYCODE_EQUALS,	0, {ASHF_QUOTE		,ASHF_QUOTE 	 ,ACSH_QUOTE	  ,ACSH_QUOTE	   }},
{KEYCODE_BACKSPACE, 0, {AKEY_BSP		,ASHF_BSP		 ,ACTL_BSP		  ,ACTL_BSP 	   }},
{KEYCODE_TAB,		0, {AKEY_TAB		,ASHF_TAB		 ,ACTL_TAB		  ,ACTL_TAB 	   }},
{KEYCODE_Q, 		0, {AKEY_Q			,ASHF_Q 		 ,ACTL_Q		  ,ACTL_Q		   }},
{KEYCODE_W, 		0, {AKEY_W			,ASHF_W 		 ,ACTL_W		  ,ACTL_W		   }},
{KEYCODE_E, 		0, {AKEY_E			,ASHF_E 		 ,ACTL_E		  ,ACTL_E		   }},
{KEYCODE_R, 		0, {AKEY_R			,ASHF_R 		 ,ACTL_R		  ,ACTL_R		   }},
{KEYCODE_T, 		0, {AKEY_T			,ASHF_T 		 ,ACTL_T		  ,ACTL_T		   }},
{KEYCODE_Y, 		0, {AKEY_Z			,ASHF_Z 		 ,ACTL_Z		  ,ACTL_Z		   }},
{KEYCODE_U, 		0, {AKEY_U			,ASHF_U 		 ,ACTL_U		  ,ACTL_U		   }},
{KEYCODE_I, 		0, {AKEY_I			,ASHF_I 		 ,ACTL_I		  ,ACTL_I		   }},
{KEYCODE_O, 		0, {AKEY_O			,ASHF_O 		 ,ACTL_O		  ,ACTL_O		   }},
{KEYCODE_P, 		0, {AKEY_P			,ASHF_P 		 ,ACTL_P		  ,ACTL_P		   }},
{KEYCODE_OPENBRACE, 0, {ASHF_RBRACE 	,ASHF_CLEAR 	 ,ACSH_RBRACE	  ,ACSH_RBRACE	   }},
{KEYCODE_CLOSEBRACE,0, {AKEY_PLUS		,AKEY_ASTERISK	 ,ACTL_PLUS 	  ,ACTL_PLUS	   }},
{KEYCODE_ENTER, 	0, {AKEY_ENTER		,ASHF_ENTER 	 ,ACTL_ENTER	  ,ACTL_ENTER	   }},
{KEYCODE_LCONTROL,	1, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_A, 		0, {AKEY_A			,ASHF_A 		 ,ACTL_A		  ,ACTL_A		   }},
{KEYCODE_S, 		0, {AKEY_S			,ASHF_S 		 ,ACTL_S		  ,ACTL_S		   }},
{KEYCODE_D, 		0, {AKEY_D			,ASHF_D 		 ,ACTL_D		  ,ACTL_D		   }},
{KEYCODE_F, 		0, {AKEY_F			,ASHF_F 		 ,ACTL_F		  ,ACTL_F		   }},
{KEYCODE_G, 		0, {AKEY_G			,ASHF_G 		 ,ACTL_G		  ,ACTL_G		   }},
{KEYCODE_H, 		0, {AKEY_H			,ASHF_H 		 ,ACTL_H		  ,ACTL_H		   }},
{KEYCODE_J, 		0, {AKEY_J			,ASHF_J 		 ,ACTL_J		  ,ACTL_J		   }},
{KEYCODE_K, 		0, {AKEY_K			,ASHF_K 		 ,ACTL_K		  ,ACTL_K		   }},
{KEYCODE_L, 		0, {AKEY_L			,ASHF_L 		 ,ACTL_L		  ,ACTL_L		   }},
{KEYCODE_COLON, 	0, {ASHF_BACKSLASH	,ASHF_BAR		 ,ACSH_BACKSLASH  ,ACSH_BACKSLASH  }},
{KEYCODE_QUOTE, 	0, {ASHF_LBRACE 	,ASHF_LBRACE	 ,ACSH_LBRACE	  ,ACSH_LBRACE	   }},
{KEYCODE_TILDE, 	0, {ASHF_TILDE		,ASHF_TILDE 	 ,ACSH_TILDE	  ,ACSH_TILDE	   }},
{KEYCODE_LSHIFT,	0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{0, 				0, {ASHF_HASH		,ASHF_QUOTE 	 ,ACSH_HASH 	  ,ACSH_HASH	   }},
{KEYCODE_Z, 		0, {AKEY_Y			,ASHF_Y 		 ,ACTL_Y		  ,ACTL_Y		   }},
{KEYCODE_X, 		0, {AKEY_X			,ASHF_X 		 ,ACTL_X		  ,ACTL_X		   }},
{KEYCODE_C, 		0, {AKEY_C			,ASHF_C 		 ,ACTL_C		  ,ACTL_C		   }},
{KEYCODE_V, 		0, {AKEY_V			,ASHF_V 		 ,ACTL_V		  ,ACTL_V		   }},
{KEYCODE_B, 		0, {AKEY_B			,ASHF_B 		 ,ACTL_B		  ,ACTL_B		   }},
{KEYCODE_N, 		0, {AKEY_N			,ASHF_N 		 ,ACTL_N		  ,ACTL_N		   }},
{KEYCODE_M, 		0, {AKEY_M			,ASHF_M 		 ,ACTL_M		  ,ACTL_M		   }},
{KEYCODE_COMMA, 	0, {AKEY_COMMA		,AKEY_SEMICOLON  ,ACTL_COMMA	  ,ACTL_COMMA	   }},
{KEYCODE_STOP,		0, {AKEY_STOP		,ASHF_COLON 	 ,ACTL_STOP 	  ,ACTL_STOP	   }},
{KEYCODE_SLASH, 	0, {AKEY_MINUS		,ASHF_UNDERSCORE ,ACTL_MINUS	  ,ACTL_MINUS	   }},
{KEYCODE_RSHIFT,	0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_ASTERISK,	0, {AKEY_LESSER 	,AKEY_GREATER	 ,ACTL_LESSER	  ,ACTL_GREATER    }},
{KEYCODE_LALT,		0, {AKEY_ATARI		,ASHF_ATARI 	 ,ACTL_ATARI	  ,ACTL_ATARI	   }},
{KEYCODE_SPACE, 	0, {AKEY_SPACE		,ASHF_SPACE 	 ,ACTL_SPACE	  ,ACTL_SPACE	   }},
{KEYCODE_CAPSLOCK,	0, {AKEY_CAPS		,ASHF_CAPS		 ,ACTL_CAPS 	  ,ACTL_CAPS	   }},
{KEYCODE_F1,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_F2,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_F3,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_F4,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_F5,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_F6,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_F7,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_F8,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_F9,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_F10,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_NUMLOCK,	0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_SCRLOCK,	0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_HOME,		0, {ASHF_CLEAR		,ASHF_CLEAR 	 ,ASHF_CLEAR	  ,ASHF_CLEAR	   }},
{KEYCODE_UP,		1, {ACTL_MINUS		,ACTL_MINUS 	 ,ACTL_MINUS	  ,ACTL_MINUS	   }},
{KEYCODE_PGUP,		0, {AKEY_BREAK		,ASHF_BREAK 	 ,ACTL_BREAK	  ,ACSH_BREAK	   }},
{KEYCODE_MINUS_PAD, 0, {AKEY_MINUS		,AKEY_MINUS 	 ,AKEY_MINUS	  ,AKEY_MINUS	   }},
{KEYCODE_LEFT,		1, {ACTL_PLUS		,ACTL_PLUS		 ,ACTL_PLUS 	  ,ACTL_PLUS	   }},
{KEYCODE_5_PAD, 	0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_RIGHT, 	1, {ACTL_ASTERISK	,ACTL_ASTERISK	 ,ACTL_ASTERISK   ,ACTL_ASTERISK   }},
{KEYCODE_PLUS_PAD,	0, {AKEY_PLUS		,AKEY_PLUS		 ,AKEY_PLUS 	  ,AKEY_PLUS	   }},
{KEYCODE_END,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_DOWN,		1, {ACTL_EQUALS 	,ACTL_EQUALS	 ,ACTL_EQUALS	  ,ACTL_EQUALS	   }},
{KEYCODE_PGDN,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_INSERT,	0, {ASHF_INSERT 	,ASHF_INSERT	 ,ASHF_INSERT	  ,ASHF_INSERT	   }},
{KEYCODE_DEL,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_RCONTROL,	0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_RALT,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_BACKSLASH2,0, {AKEY_LESSER 	,AKEY_GREATER	 ,AKEY_LESSER	  ,AKEY_GREATER    }},
{0xff}
};

static	key_trans key_trans_us[] = {
{0, 				0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_ESC,		0, {AKEY_ESC		,ASHF_ESC		 ,ACTL_ESC		  ,ACSH_ESC 	   }},
{KEYCODE_1, 		0, {AKEY_1			,ASHF_EXCLAM	 ,ACTL_1		  ,ACSH_EXCLAM	   }},
{KEYCODE_2, 		0, {AKEY_2			,ASHF_DQUOTE	 ,ACTL_2		  ,ACSH_DQUOTE	   }},
{KEYCODE_3, 		0, {AKEY_3			,ASHF_HASH		 ,ACTL_3		  ,ACSH_HASH	   }},
{KEYCODE_4, 		0, {AKEY_4			,ASHF_DOLLAR	 ,ACTL_4		  ,ACSH_DOLLAR	   }},
{KEYCODE_5, 		0, {AKEY_5			,ASHF_PERCENT	 ,ACTL_5		  ,ACSH_PERCENT    }},
{KEYCODE_6, 		0, {AKEY_6			,ASHF_TILDE 	 ,ACTL_6		  ,ACSH_AMPERSAND  }},
{KEYCODE_7, 		0, {AKEY_7			,ASHF_AMPERSAND  ,ACTL_7		  ,ACSH_N		   }},
{KEYCODE_8, 		0, {AKEY_8			,AKEY_ASTERISK	 ,ACTL_8		  ,ACSH_M		   }},
{KEYCODE_9, 		0, {AKEY_9			,ASHF_LPAREN	 ,ACTL_9		  ,ACSH_LBRACE	   }},
{KEYCODE_0, 		0, {AKEY_0			,ASHF_RPAREN	 ,ACTL_0		  ,ACSH_RBRACE	   }},
{KEYCODE_MINUS, 	0, {AKEY_MINUS		,ASHF_UNDERSCORE ,ACTL_MINUS	  ,ACSH_UNDERSCORE }},
{KEYCODE_EQUALS,	0, {AKEY_EQUALS 	,AKEY_PLUS		 ,ACTL_EQUALS	  ,ACTL_PLUS	   }},
{KEYCODE_BACKSPACE, 0, {AKEY_BSP		,ASHF_BSP		 ,ACTL_BSP		  ,ACSH_BSP 	   }},
{KEYCODE_TAB,		0, {AKEY_TAB		,ASHF_TAB		 ,ACTL_TAB		  ,ACSH_TAB 	   }},
{KEYCODE_Q, 		0, {AKEY_Q			,ASHF_Q 		 ,ACTL_Q		  ,ACSH_Q		   }},
{KEYCODE_W, 		0, {AKEY_W			,ASHF_W 		 ,ACTL_W		  ,ACSH_W		   }},
{KEYCODE_E, 		0, {AKEY_E			,ASHF_E 		 ,ACTL_E		  ,ACSH_E		   }},
{KEYCODE_R, 		0, {AKEY_R			,ASHF_R 		 ,ACTL_R		  ,ACSH_R		   }},
{KEYCODE_T, 		0, {AKEY_T			,ASHF_T 		 ,ACTL_T		  ,ACSH_T		   }},
{KEYCODE_Y, 		0, {AKEY_Y			,ASHF_Y 		 ,ACTL_Y		  ,ACSH_Y		   }},
{KEYCODE_U, 		0, {AKEY_U			,ASHF_U 		 ,ACTL_U		  ,ACTL_U		   }},
{KEYCODE_I, 		0, {AKEY_I			,ASHF_I 		 ,ACTL_I		  ,ACTL_I		   }},
{KEYCODE_O, 		0, {AKEY_O			,ASHF_O 		 ,ACTL_O		  ,ACTL_O		   }},
{KEYCODE_P, 		0, {AKEY_P			,ASHF_P 		 ,ACTL_P		  ,ACTL_P		   }},
{KEYCODE_OPENBRACE, 0, {ASHF_LBRACE 	,ACTL_COMMA 	 ,ACTL_COMMA	  ,ACSH_LBRACE	   }},
{KEYCODE_CLOSEBRACE,0, {ASHF_RBRACE 	,ACTL_STOP		 ,ACTL_STOP 	  ,ACSH_RBRACE	   }},
{KEYCODE_ENTER, 	0, {AKEY_ENTER		,ASHF_ENTER 	 ,ACTL_ENTER	  ,ACSH_ENTER	   }},
{KEYCODE_LCONTROL,	1, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_A, 		0, {AKEY_A			,ASHF_A 		 ,ACTL_A		  ,ACSH_A		   }},
{KEYCODE_S, 		0, {AKEY_S			,ASHF_S 		 ,ACTL_S		  ,ACSH_S		   }},
{KEYCODE_D, 		0, {AKEY_D			,ASHF_D 		 ,ACTL_D		  ,ACSH_D		   }},
{KEYCODE_F, 		0, {AKEY_F			,ASHF_F 		 ,ACTL_F		  ,ACSH_F		   }},
{KEYCODE_G, 		0, {AKEY_G			,ASHF_G 		 ,ACTL_G		  ,ACSH_G		   }},
{KEYCODE_H, 		0, {AKEY_H			,ASHF_H 		 ,ACTL_H		  ,ACSH_H		   }},
{KEYCODE_J, 		0, {AKEY_J			,ASHF_J 		 ,ACTL_J		  ,ACSH_J		   }},
{KEYCODE_K, 		0, {AKEY_K			,ASHF_K 		 ,ACTL_K		  ,ACSH_K		   }},
{KEYCODE_L, 		0, {AKEY_L			,ASHF_L 		 ,ACTL_L		  ,ACSH_L		   }},
{KEYCODE_COLON, 	0, {AKEY_SEMICOLON	,ASHF_COLON 	 ,ACTL_SEMICOLON  ,ACSH_COLON	   }},
{KEYCODE_QUOTE, 	0, {ASHF_QUOTE		,ACSH_QUOTE 	 ,ASHF_DQUOTE	  ,ACSH_DQUOTE	   }},
{KEYCODE_TILDE, 	0, {ASHF_QUOTE		,ACSH_QUOTE 	 ,ACTL_ASTERISK   ,ACSH_TILDE	   }},
{KEYCODE_LSHIFT,	0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_BACKSLASH, 0, {ASHF_BACKSLASH	,ASHF_BAR		 ,ACSH_BACKSLASH  ,ACSH_BAR 	   }},
{KEYCODE_Z, 		0, {AKEY_Z			,ASHF_Z 		 ,ACTL_Z		  ,ACSH_Z		   }},
{KEYCODE_X, 		0, {AKEY_X			,ASHF_X 		 ,ACTL_X		  ,ACTL_X		   }},
{KEYCODE_C, 		0, {AKEY_C			,ASHF_C 		 ,ACTL_C		  ,ACTL_C		   }},
{KEYCODE_V, 		0, {AKEY_V			,ASHF_V 		 ,ACTL_V		  ,ACTL_V		   }},
{KEYCODE_B, 		0, {AKEY_B			,ASHF_B 		 ,ACTL_B		  ,ACTL_B		   }},
{KEYCODE_N, 		0, {AKEY_N			,ASHF_N 		 ,ACTL_N		  ,ACTL_N		   }},
{KEYCODE_M, 		0, {AKEY_M			,ASHF_M 		 ,ACTL_M		  ,ACTL_M		   }},
{KEYCODE_COMMA, 	0, {AKEY_COMMA		,AKEY_LESSER	 ,ACTL_COMMA	  ,ACTL_LESSER	   }},
{KEYCODE_STOP,		0, {AKEY_STOP		,AKEY_GREATER	 ,ACTL_STOP 	  ,ACTL_GREATER    }},
{KEYCODE_SLASH, 	0, {AKEY_SLASH		,ASHF_QUESTION	 ,ACTL_SLASH	  ,ACSH_QUESTION   }},
{KEYCODE_RSHIFT,	0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_ASTERISK,	0, {ASHF_BACKSLASH	,ASHF_BAR		 ,ACSH_BACKSLASH  ,ACSH_BAR 	   }},
{KEYCODE_LALT,		0, {AKEY_ATARI		,ASHF_ATARI 	 ,ACTL_ATARI	  ,ACSH_ATARI	   }},
{KEYCODE_SPACE, 	0, {AKEY_SPACE		,ASHF_SPACE 	 ,ACTL_SPACE	  ,ACSH_SPACE	   }},
{KEYCODE_CAPSLOCK,	0, {AKEY_CAPS		,ASHF_CAPS		 ,ACTL_CAPS 	  ,ACSH_CAPS	   }},
{KEYCODE_F1,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_F2,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_F3,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_F4,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_F5,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_F6,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_F7,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_F8,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_F9,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_F10,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_NUMLOCK,	0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_SCRLOCK,	0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_HOME,		0, {ASHF_CLEAR		,ASHF_CLEAR 	 ,ACSH_CLEAR	  ,ACSH_CLEAR	   }},
{KEYCODE_UP,		1, {ACTL_MINUS		,ACTL_MINUS 	 ,ACTL_MINUS	  ,ACTL_MINUS	   }},
{KEYCODE_PGUP,		0, {AKEY_BREAK		,ASHF_BREAK 	 ,ACTL_BREAK	  ,ACSH_BREAK	   }},
{KEYCODE_MINUS_PAD, 0, {AKEY_MINUS		,AKEY_MINUS 	 ,AKEY_MINUS	  ,AKEY_MINUS	   }},
{KEYCODE_LEFT,		1, {ACTL_PLUS		,ACTL_PLUS		 ,ACTL_PLUS 	  ,ACTL_PLUS	   }},
{KEYCODE_5_PAD, 	0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_RIGHT, 	1, {ACTL_ASTERISK	,ACTL_ASTERISK	 ,ACTL_ASTERISK   ,ACTL_ASTERISK   }},
{KEYCODE_PLUS_PAD,	0, {AKEY_PLUS		,AKEY_PLUS		 ,AKEY_PLUS 	  ,AKEY_PLUS	   }},
{KEYCODE_END,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_DOWN,		1, {ACTL_EQUALS 	,ACTL_EQUALS	 ,ACTL_EQUALS	  ,ACTL_EQUALS	   }},
{KEYCODE_PGDN,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_INSERT,	0, {ASHF_INSERT 	,ASHF_INSERT	 ,ASHF_INSERT	  ,ASHF_INSERT	   }},
{KEYCODE_DEL,		0, {AKEY_BSP		,AKEY_BSP		 ,AKEY_BSP		  ,AKEY_BSP 	   }},
{KEYCODE_RCONTROL,	0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_RALT,		0, {AKEY_NONE		,AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   }},
{KEYCODE_BACKSLASH2,0, {ASHF_BACKSLASH	,ASHF_BAR		 ,ACSH_BACKSLASH  ,ACSH_BAR 	   }},
{0xff}
};


static	key_trans * kbd = key_trans_us;

void a800_handle_keyboard(void)
{
	int i, modifiers, shift_control, atari_code;
	static int atari_last = 0;

	if( setup_active() || onscrd_active() )
		return;

	if (keyboard_pressed(KEYCODE_LCONTROL) && keyboard_pressed(KEYCODE_LALT))
    {
		if (keyboard_pressed(KEYCODE_F1))	 /* ctrl+alt+f1 ? */
			kbd = key_trans_us;
		if (keyboard_pressed(KEYCODE_F2))	 /* ctrl+alt+f2 ? */
			kbd = key_trans_de;
	}
    modifiers = 0;
	shift_control = 0;
    /* with shift ? */
	if (keyboard_pressed(KEYCODE_LSHIFT) || keyboard_pressed(KEYCODE_RSHIFT))
        shift_control |= 1;
    /* with control ? */
	if (keyboard_pressed(KEYCODE_LCONTROL) || keyboard_pressed(KEYCODE_RCONTROL))
        shift_control |= 2;

    for (i = 0; kbd[i].osd != 0xff; i++)
	{
		if (keyboard_pressed(modifiers | kbd[i].osd))
		{
			/* joystick key and joystick mode enabled ? */
            if (kbd[i].joystick && (input_port_0_r(0) & 0x80))
				continue;
            atari_code = kbd[i].atari[shift_control];
			if (atari_code != AKEY_NONE)
			{
				if (atari_code == atari_last)
					return;
				atari_last = atari_code;
				if ((atari_code & 0x3f) == AKEY_BREAK)
				{
					pokey1_break_w(atari_code & 0x40);
                    return;
                }
				pokey1_kbcode_w(atari_code, 1);
				return;
            }
        }
	}
    /* remove key pressed status bit from skstat */
	pokey1_kbcode_w(AKEY_NONE, 0);
	atari_last = AKEY_NONE;
}


















/*										KBCODE							*/
/*										Key 	bits	Keypad code 	*/
/*										------------------- 			*/
#define VKEY_NONE		0x00	/*		none	0000	$FF 			*/
#define VKEY_HASH		0x01	/*		#		0001	$0B 			*/
#define VKEY_0			0x02	/*		0		0010	$00 			*/
#define VKEY_ASTERISK	0x03	/*		*		0011	$0A 			*/
#define VKEY_RESET		0x04	/*		Reset	0100	$0E 			*/
#define VKEY_9			0x05	/*		9		0101	$09 			*/
#define VKEY_8			0x06	/*		8		0110	$08 			*/
#define VKEY_7			0x07	/*		7		0111	$07 			*/
#define VKEY_PAUSE		0x08	/*		Pause	1000	$0D 			*/
#define VKEY_6			0x09	/*		6		1001	$06 			*/
#define VKEY_5			0x0a	/*		5		1010	$05 			*/
#define VKEY_4			0x0b	/*		4		1011	$04 			*/
#define VKEY_START		0x0c	/*		Start	1100	$0C 			*/
#define VKEY_3			0x0d	/*		3		1101	$03 			*/
#define VKEY_2			0x0e	/*		2		1110	$02 			*/
#define VKEY_1			0x0f	/*		1		1111	$01 			*/

#define VKEY_BREAK		0x10

static  key_trans pad_trans_us[] = {
{0, 				0, {VKEY_NONE,		VKEY_NONE,			VKEY_NONE,			VKEY_NONE		   }},
{KEYCODE_1_PAD, 	0, {VKEY_1, 		VKEY_1|0x40,		VKEY_1|0x80,		VKEY_1|0x80 	   }},
{KEYCODE_2_PAD, 	0, {VKEY_2, 		VKEY_2|0x40,		VKEY_2|0x80,		VKEY_2|0x80 	   }},
{KEYCODE_3_PAD, 	0, {VKEY_3, 		VKEY_3|0x40,		VKEY_3|0x80,		VKEY_3|0x80 	   }},
{KEYCODE_4_PAD, 	0, {VKEY_4, 		VKEY_4|0x40,		VKEY_4|0x80,		VKEY_4|0x80 	   }},
{KEYCODE_5_PAD, 	0, {VKEY_5, 		VKEY_5|0x40,		VKEY_5|0x80,		VKEY_5|0x80 	   }},
{KEYCODE_6_PAD, 	0, {VKEY_6, 		VKEY_6|0x40,		VKEY_6|0x80,		VKEY_6|0x80 	   }},
{KEYCODE_7_PAD, 	0, {VKEY_7, 		VKEY_7|0x40,		VKEY_7|0x80,		VKEY_7|0x80 	   }},
{KEYCODE_8_PAD, 	0, {VKEY_8, 		VKEY_8|0x40,		VKEY_8|0x80,		VKEY_8|0x80 	   }},
{KEYCODE_9_PAD, 	0, {VKEY_9, 		VKEY_9|0x40,		VKEY_9|0x80,		VKEY_9|0x80 	   }},
{KEYCODE_0_PAD, 	0, {VKEY_0, 		VKEY_0|0x40,		VKEY_0|0x80,		VKEY_0|0x80 	   }},
{KEYCODE_F1,		0, {VKEY_START, 	VKEY_START|0x40,	VKEY_START|0x80,	VKEY_START|0x80    }},
{KEYCODE_F2,		0, {VKEY_PAUSE, 	VKEY_PAUSE|0x40,	VKEY_PAUSE|0x80,	VKEY_PAUSE|0x80    }},
{KEYCODE_F3,		0, {VKEY_RESET, 	VKEY_RESET|0x40,	VKEY_RESET|0x80,	VKEY_RESET|0x80    }},
{KEYCODE_F5,		0, {VKEY_HASH,		VKEY_HASH|0x40, 	VKEY_HASH|0x80, 	VKEY_HASH|0x80	   }},
{KEYCODE_F6,		0, {VKEY_ASTERISK,	VKEY_ASTERISK|0x40, VKEY_ASTERISK|0x80, VKEY_ASTERISK|0x80 }},
{KEYCODE_F7,		0, {VKEY_BREAK, 	VKEY_BREAK|0x40,	VKEY_BREAK|0x80,	VKEY_BREAK|0x80    }},
{0xff}
};

static	key_trans *pad = pad_trans_us;

/* absolutely no clue what to do here :((( */
void a5200_handle_keypads(void)
{
	int i, /*modifiers,*/ shift_control, atari_code;
	static int atari_last = 0;

	shift_control = 0;
    /* with shift ? */
	if (keyboard_pressed(KEYCODE_LSHIFT) || keyboard_pressed(KEYCODE_RSHIFT))
        shift_control |= 1;
    /* with control ? */
	if (keyboard_pressed(KEYCODE_LCONTROL) || keyboard_pressed(KEYCODE_RCONTROL))
        shift_control |= 2;

	for (i = 0; pad[i].osd != 0xff; i++)
	{
		if( keyboard_pressed(pad[i].osd) )
		{
			/* joystick key and joystick mode enabled ? */
			if( pad[i].joystick && (input_port_0_r(0) & 0x80) )
				continue;
			atari_code = pad[i].atari[shift_control];
			if (atari_code != VKEY_NONE)
			{
				if (atari_code == atari_last)
					return;
				atari_last = atari_code;
				if( (atari_code & 0x3f) == VKEY_BREAK )
                {
                    pokey1_break_w(atari_code & 0x40);
                    return;
                }
				pokey1_kbcode_w(atari_code | 0x30, 1);
				return;
            }
        }
	}
    /* remove key pressed status bit from skstat */
	pokey1_kbcode_w(VKEY_NONE, 0);
	atari_last = VKEY_NONE;
}

