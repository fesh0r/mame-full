/***************************************************************************
	vtech2.c

    machine driver
	Juergen Buchmueller <pullmoll@t-online.de> MESS driver, Jan 2000
	Davide Moretti <dave@rimini.com> ROM dump and hardware description

    TODO:
		Add loading images from WAV files.
		Printer and RS232 support.
		Check if the FDC is really the same as in the
		Laser 210/310 (aka VZ200/300) series.

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define VERBOSE 1

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)	/* x */
#endif

/* from mame.c */
extern int bitmap_dirty;

/* from mess/vidhrdw/vtech2.c */
extern char laser_frame_message[64+1];
extern int laser_frame_time;

/* public */
int laser_latch = -1;

/* static */
static UINT8 *mem = NULL;
static int laser_bank_mask = 0x0000;	/* up to 16 4K banks supported */
static int laser_bank[4] = {-1,-1,-1,-1};
static int laser_video_bank = 0;

static UINT8 *cassette_image = NULL;

#define TRKSIZE_VZ	0x9a0	/* arbitrary (actually from analyzing format) */
#define TRKSIZE_FM	3172	/* size of a standard FM mode track */

static const char *floppy_name[2] = {NULL, NULL};
static void *laser_fdc_file[2] = {NULL, NULL};
static UINT8 laser_track_x2[2] = {80, 80};
static UINT8 laser_fdc_wrprot[2] = {0x80, 0x80};
static UINT8 laser_fdc_status = 0;
static UINT8 laser_fdc_data[TRKSIZE_FM];
static int laser_data;
static int laser_fdc_edge = 0;
static int laser_fdc_bits = 8;
static int laser_drive = -1;
static int laser_fdc_start = 0;
static int laser_fdc_write = 0;
static int laser_fdc_offs = 0;
static int laser_fdc_latch = 0;

/* write to banked memory (handle memory mapped i/o and videoram) */
static void mwa_bank(int bank, int offs, int data);

/* wrappers for bank #1 to #4 */
static void mwa_bank1(int offs, int data) { mwa_bank(0,offs,data); }
static void mwa_bank2(int offs, int data) { mwa_bank(1,offs,data); }
static void mwa_bank3(int offs, int data) { mwa_bank(2,offs,data); }
static void mwa_bank4(int offs, int data) { mwa_bank(3,offs,data); }

/* read from banked memory (handle memory mapped i/o) */
static int mra_bank(int bank, int offs);

/* wrappers for bank #1 to #4 */
static int mra_bank1(int offs) { return mra_bank(0,offs); }
static int mra_bank2(int offs) { return mra_bank(1,offs); }
static int mra_bank3(int offs) { return mra_bank(2,offs); }
static int mra_bank4(int offs) { return mra_bank(3,offs); }

/* read banked memory (handle memory mapped i/o) */
static int (*mra_bank_soft[4])(int) =
{
    mra_bank1,  /* mapped in 0000-3fff */
    mra_bank2,  /* mapped in 4000-7fff */
    mra_bank3,  /* mapped in 8000-bfff */
    mra_bank4   /* mapped in c000-ffff */
};

/* write banked memory (handle memory mapped i/o and videoram) */
static void (*mwa_bank_soft[4])(int,int) =
{
    mwa_bank1,  /* mapped in 0000-3fff */
    mwa_bank2,  /* mapped in 4000-7fff */
    mwa_bank3,  /* mapped in 8000-bfff */
    mwa_bank4   /* mapped in c000-ffff */
};

/* read banked memory (plain ROM/RAM) */
static int (*mra_bank_hard[4])(int) =
{
    MRA_BANK1,  /* mapped in 0000-3fff */
    MRA_BANK2,  /* mapped in 4000-7fff */
    MRA_BANK3,  /* mapped in 8000-bfff */
    MRA_BANK4   /* mapped in c000-ffff */
};

/* write banked memory (plain ROM/RAM) */
static void (*mwa_bank_hard[4])(int,int) =
{
    MWA_BANK1,  /* mapped in 0000-3fff */
    MWA_BANK2,  /* mapped in 4000-7fff */
    MWA_BANK3,  /* mapped in 8000-bfff */
    MWA_BANK4   /* mapped in c000-ffff */
};

void init_laser(void)
{
    UINT8 *gfx = memory_region(REGION_GFX2);
    int i;
    for (i = 0; i < 256; i++)
        gfx[i] = i;
}

void laser350_init_machine(void)
{
    mem = memory_region(REGION_CPU1);
	/* banks 0 to 3 only, optional ROM extension */
	laser_bank_mask = 0xf00f;
    laser_video_bank = 3;
	videoram = mem + laser_video_bank * 0x04000;
	LOG((errorlog,"laser350 init machine: bank mask $%04X, video %d [$%05X]\n", laser_bank_mask, laser_video_bank, laser_video_bank * 0x04000));
}

void laser500_init_machine(void)
{
    mem = memory_region(REGION_CPU1);
	/* banks 0 to 2, and 4-7 only , optional ROM extension */
	laser_bank_mask = 0xf0f7;
    laser_video_bank = 7;
    videoram = mem + laser_video_bank * 0x04000;
	LOG((errorlog,"laser500 init machine: bank mask $%04X, video %d [$%05X]\n", laser_bank_mask, laser_video_bank, laser_video_bank * 0x04000));
}

void laser700_init_machine(void)
{
    mem = memory_region(REGION_CPU1);
	/* all banks except #3 */
	laser_bank_mask = 0xfff7;
    laser_video_bank = 7;
    videoram = mem + laser_video_bank * 0x04000;
	LOG((errorlog,"laser700 init machine: bank mask $%04X, video %d [$%05X]\n", laser_bank_mask, laser_video_bank, laser_video_bank * 0x04000));
}

void laser_shutdown_machine(void)
{
    int i;
    for( i = 0; i < 2; i++ )
    {
        if( laser_fdc_file[i] )
            osd_fclose(laser_fdc_file[i]);
        laser_fdc_file[i] = NULL;
    }
}

static void mwa_empty(int offs, int data)
{
	/* empty */
}

static int mra_empty(int offs)
{
	return 0xff;
}

void laser_bank_select_w(int offs, int data)
{
    static const char *bank_name[16] = {
        "ROM lo","ROM hi","MM I/O","Video RAM lo",
        "RAM #0","RAM #1","RAM #2","RAM #3",
        "RAM #4","RAM #5","RAM #6","RAM #7/Video RAM hi",
        "ext ROM #0","ext ROM #1","ext ROM #2","ext ROM #3"};

    data &= 15;

	if( data != laser_bank[offs] )
    {
        laser_bank[offs] = data;
		LOG((errorlog,"select bank #%d $%02X [$%05X] %s\n", offs+1, data, 0x4000 * (data & 15), bank_name[data]));

        /* memory mapped I/O bank selected? */
		if (data == 2)
		{
			cpu_setbankhandler_r(1+offs,mra_bank_soft[offs]);
			cpu_setbankhandler_w(1+offs,mwa_bank_soft[offs]);
		}
		else
		{
			cpu_setbank(offs+1, &mem[0x4000*laser_bank[offs]]);
			if( laser_bank_mask & (1 << data) )
			{
				cpu_setbankhandler_r(1+offs,mra_bank_hard[offs]);
				/* video RAM bank selected? */
				if( data == laser_video_bank )
				{
					LOG((errorlog,"select bank #%d VIDEO!\n", offs+1));
                    cpu_setbankhandler_w(1+offs,mwa_bank_soft[offs]);
				}
				else
				{
					cpu_setbankhandler_w(1+offs,mwa_bank_hard[offs]);
				}
			}
			else
			{
				LOG((errorlog,"select bank #%d MASKED!\n", offs+1));
				cpu_setbankhandler_r(1+offs,mra_empty);
				cpu_setbankhandler_w(1+offs,mwa_empty);
			}
		}
    }
}

/*************************************************
 * memory mapped I/O read
 * bit  function
 * 7	not assigned
 * 6	column 6
 * 5	column 5
 * 4	column 4
 * 3	column 3
 * 2	column 2
 * 1	column 1
 * 0	column 0
 ************************************************/
static int mra_bank(int bank, int offs)
{
    int data = 0xff;

    /* Laser 500/700 only: keyboard rows A through D */
	if( (offs & 0x00ff) == 0x00ff )
	{
		static int row_a,row_b,row_c,row_d;
		if( (offs & 0x0300) == 0x0000 ) /* keyboard row A */
		{
			if( readinputport( 8) != row_a )
			{
				row_a = readinputport(8);
				data &= row_a;
			}
		}
		if( (offs & 0x0300) == 0x0100 ) /* keyboard row B */
		{
			if( readinputport( 9) != row_b )
			{
				row_b = readinputport( 9);
				data &= row_b;
			}
		}
		if( (offs & 0x0300) == 0x0200 ) /* keyboard row C */
		{
			if( readinputport(10) != row_c )
			{
				row_c = readinputport(10);
				data &= row_c;
			}
		}
		if( (offs & 0x0300) == 0x0300 ) /* keyboard row D */
		{
			if( readinputport(11) != row_d )
			{
				row_d = readinputport(11);
				data &= row_d;
			}
		}
	}
	else
	{
		/* All Lasers keyboard rows 0 through 7 */
        if( !(offs & 0x01) )
			data &= readinputport( 0);
		if( !(offs & 0x02) )
			data &= readinputport( 1);
		if( !(offs & 0x04) )
			data &= readinputport( 2);
		if( !(offs & 0x08) )
			data &= readinputport( 3);
		if( !(offs & 0x10) )
			data &= readinputport( 4);
		if( !(offs & 0x20) )
			data &= readinputport( 5);
		if( !(offs & 0x40) )
			data &= readinputport( 6);
		if( !(offs & 0x80) )
			data &= readinputport( 7);
	}
	/* what's bit 7 good for? tape input maybe? */
//  data &= ~0x80;

//	LOG((errorlog,"bank #%d keyboard_r [$%04X] $%02X\n", bank, offs, data));

    return data;
}

/*************************************************
 * memory mapped I/O write
 * bit  function
 * 7-6  not assigned
 * 5    speaker B ???
 * 4    ???
 * 3    mode: 1 graphics, 0 text
 * 2    cassette out (MSB)
 * 1    cassette out (LSB)
 * 0    speaker A
 ************************************************/
static void mwa_bank(int bank, int offs, int data)
{
	offs += 0x4000 * laser_bank[bank];
    switch (laser_bank[bank])
    {
    case  0:    /* ROM lower 16K */
    case  1:    /* ROM upper 16K */
		LOG((errorlog,"bank #%d write to ROM [$%05X] $%02X\n", bank+1, offs, data));
        break;
    case  2:    /* memory mapped output */
        if (data != laser_latch)
        {
            /* Toggle between graphics and text modes? */
            if ((data ^ laser_latch) & 0x08)
                bitmap_dirty = 1;
            laser_latch = data;
            LOG((errorlog,"bank #%d write to I/O [$%05X] $%02X\n", bank+1, offs, data));
            DAC_data_w(0, (data & 1) ? 255 : 0);
        }
        break;
    case 12:    /* ext. ROM #1 */
    case 13:    /* ext. ROM #2 */
    case 14:    /* ext. ROM #3 */
    case 15:    /* ext. ROM #4 */
		LOG((errorlog,"bank #%d write to ROM [$%05X] $%02X\n", bank+1, offs, data));
        break;
    default:    /* RAM */
        if( laser_bank[bank] == laser_video_bank && mem[offs] != data )
		{
			LOG((errorlog,"bank #%d write to videoram [$%05X] $%02X\n", bank+1, offs, data));
            dirtybuffer[offs&0x3fff] = 1;
		}
        mem[offs] = data;
        break;
    }
}

int laser_rom_id(const char *name, const char *gamename)
{
	/* Hmmm.. if I only had a single ROM to identify it ;) */
	return 1;
}

int laser_rom_init(int id, const char *name)
{
	int size = 0;
    void *file;

	file = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
    if( file )
    {
		size = osd_fread(file, &mem[0x30000], 0x10000);
        osd_fclose(file);
		laser_bank_mask &= ~0xf000;
		if( size > 0 )
			laser_bank_mask |= 0x1000;
		if( size > 0x4000 )
			laser_bank_mask |= 0x2000;
		if( size > 0x8000 )
			laser_bank_mask |= 0x4000;
		if( size > 0xc000 )
			laser_bank_mask |= 0x8000;
    }
    if( size > 0 )
        return 0;
    return 1;
}

void laser_rom_exit(int id)
{
	laser_bank_mask &= ~0xf000;
	/* wipe out the memory contents to be 100% sure */
	memset(&mem[0x30000], 0xff, 0x10000);
}

int laser_cassette_id(const char *name, const char *gamename)
{
	const char magic[] = "  \000\000";
    char buff[4];
    void *file;

    file = osd_fopen(gamename, name, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
    if( file )
    {
        osd_fread(file, buff, sizeof(buff));
		if( memcmp(buff, magic, sizeof(buff)) == 0 )
        {
			LOG((errorlog, "laser_cassette_id: magic %02X%02X%02X%02X found\n", magic[0],magic[1],magic[2],magic[3]));
            return 1;
        }
    }
    return 0;
}

int laser_cassette_init(int id, const char *name)
{
	const char magic[] = "  \000\000";
    char buff[4];
    void *file;

	file = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
    if( file )
    {
        osd_fread(file, buff, sizeof(buff));
		if( memcmp(buff, magic, sizeof(buff)) == 0 )
        {
			int size = osd_fsize(file);
			LOG((errorlog, "laser_cassette_id: magic %02X%02X%02X%02X found\n", magic[0],magic[1],magic[2],magic[3]));
			cassette_image = (UINT8 *) malloc(size);
			if( cassette_image )
			{
				osd_fseek(file, 0, SEEK_SET);
				osd_fread(file, cassette_image, size);
				osd_fclose(file);
				return 0;
			}
        }
    }
	return 1;
}

void laser_cassette_exit(int id)
{
	if( cassette_image )
		free(cassette_image);
	cassette_image = NULL;
}

int laser_floppy_id(const char *name, const char *gamename)
{
	void *file;
	UINT8 buff[32];

    file = osd_fopen(gamename, name, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
    if( file )
    {
        osd_fread(file, buff, sizeof(buff));
		osd_fclose(file);
		if( memcmp(buff, "\x80\x80\x80\x80\x80\x80\x00\xfe\0xe7\0x18\0xc3\x00\x00\x00\x80\x80", 16) == 0 )
			return 1;
	}
	return 0;
}

int laser_floppy_init(int id, const char *name)
{
    floppy_name[id] = name;
    return 0;
}

void laser_floppy_exit(int id)
{
    if( laser_fdc_file[id] )
        osd_fclose(laser_fdc_file[id]);
    laser_fdc_file[id] = NULL;
    floppy_name[id] = NULL;
}

static void laser_get_track(void)
{
    sprintf(laser_frame_message, "#%d get track %02d", laser_drive, laser_track_x2[laser_drive]/2);
    laser_frame_time = 30;
    /* drive selected or and image file ok? */
    if( laser_drive >= 0 && laser_fdc_file[laser_drive] != NULL )
    {
        int size, offs;
        size = TRKSIZE_VZ;
        offs = TRKSIZE_VZ * laser_track_x2[laser_drive]/2;
        osd_fseek(laser_fdc_file[laser_drive], offs, SEEK_SET);
        size = osd_fread(laser_fdc_file[laser_drive], laser_fdc_data, size);
        LOG((errorlog,"get track @$%05x $%04x bytes\n", offs, size));
    }
    laser_fdc_offs = 0;
    laser_fdc_write = 0;
}

static void laser_put_track(void)
{
    /* drive selected and image file ok? */
    if( laser_drive >= 0 && laser_fdc_file[laser_drive] != NULL )
    {
        int size, offs;
        offs = TRKSIZE_VZ * laser_track_x2[laser_drive]/2;
        osd_fseek(laser_fdc_file[laser_drive], offs + laser_fdc_start, SEEK_SET);
        size = osd_fwrite(laser_fdc_file[laser_drive], &laser_fdc_data[laser_fdc_start], laser_fdc_write);
        LOG((errorlog,"put track @$%05X+$%X $%04X/$%04X bytes\n", offs, laser_fdc_start, size, laser_fdc_write));
    }
}

#define PHI0(n) (((n)>>0)&1)
#define PHI1(n) (((n)>>1)&1)
#define PHI2(n) (((n)>>2)&1)
#define PHI3(n) (((n)>>3)&1)

int laser_fdc_r(int offset)
{
    int data = 0xff;
    switch( offset )
    {
    case 1: /* data (read-only) */
        if( laser_fdc_bits > 0 )
        {
            if( laser_fdc_status & 0x80 )
                laser_fdc_bits--;
            data = (laser_data >> laser_fdc_bits) & 0xff;
#if 0
            LOG((errorlog,"laser_fdc_r bits %d%d%d%d%d%d%d%d\n",
                (data>>7)&1,(data>>6)&1,(data>>5)&1,(data>>4)&1,
                (data>>3)&1,(data>>2)&1,(data>>1)&1,(data>>0)&1 ));
#endif
        }
        if( laser_fdc_bits == 0 )
        {
            laser_data = laser_fdc_data[laser_fdc_offs];
            LOG((errorlog,"laser_fdc_r %d : data ($%04X) $%02X\n", offset, laser_fdc_offs, laser_data));
            if( laser_fdc_status & 0x80 )
            {
                laser_fdc_bits = 8;
                laser_fdc_offs = ++laser_fdc_offs % TRKSIZE_FM;
            }
            laser_fdc_status &= ~0x80;
        }
        break;
    case 2: /* polling (read-only) */
        /* fake */
        if( laser_drive >= 0 )
            laser_fdc_status |= 0x80;
        data = laser_fdc_status;
        break;
    case 3: /* write protect status (read-only) */
        if( laser_drive >= 0 )
            data = laser_fdc_wrprot[laser_drive];
        LOG((errorlog,"laser_fdc_r %d : write_protect $%02X\n", offset, data));
        break;
    }
    return data;
}

void laser_fdc_w(int offset, int data)
{
    int drive;

    switch( offset )
    {
    case 0: /* latch (write-only) */
        drive = (data & 0x10) ? 0 : (data & 0x80) ? 1 : -1;
        if( drive != laser_drive )
        {
            laser_drive = drive;
            if( laser_drive >= 0 )
                laser_get_track();
        }
        if( laser_drive >= 0 )
        {
            if( (PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI1(laser_fdc_latch)) ||
                (PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI2(laser_fdc_latch)) ||
                (PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI3(laser_fdc_latch)) ||
                (PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI0(laser_fdc_latch)) )
            {
                if( laser_track_x2[laser_drive] > 0 )
                    laser_track_x2[laser_drive]--;
                LOG((errorlog,"laser_fdc_w(%d) $%02X drive %d: stepout track #%2d.%d\n", offset, data, laser_drive, laser_track_x2[laser_drive]/2,5*(laser_track_x2[laser_drive]&1)));
                if( (laser_track_x2[laser_drive] & 1) == 0 )
                    laser_get_track();
            }
            else
            if( (PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI3(laser_fdc_latch)) ||
                (PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI0(laser_fdc_latch)) ||
                (PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI1(laser_fdc_latch)) ||
                (PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI2(laser_fdc_latch)) )
            {
                if( laser_track_x2[laser_drive] < 2*40 )
                    laser_track_x2[laser_drive]++;
                LOG((errorlog,"laser_fdc_w(%d) $%02X drive %d: stepin track #%2d.%d\n", offset, data, laser_drive, laser_track_x2[laser_drive]/2,5*(laser_track_x2[laser_drive]&1)));
                if( (laser_track_x2[laser_drive] & 1) == 0 )
                    laser_get_track();
            }
            if( (data & 0x40) == 0 )
            {
                laser_data <<= 1;
                if( (laser_fdc_latch ^ data) & 0x20 )
                    laser_data |= 1;
                if( (laser_fdc_edge ^= 1) == 0 )
                {
                    if( --laser_fdc_bits == 0 )
                    {
                        UINT8 value = 0;
                        laser_data &= 0xffff;
                        if( laser_data & 0x4000 ) value |= 0x80;
                        if( laser_data & 0x1000 ) value |= 0x40;
                        if( laser_data & 0x0400 ) value |= 0x20;
                        if( laser_data & 0x0100 ) value |= 0x10;
                        if( laser_data & 0x0040 ) value |= 0x08;
                        if( laser_data & 0x0010 ) value |= 0x04;
                        if( laser_data & 0x0004 ) value |= 0x02;
                        if( laser_data & 0x0001 ) value |= 0x01;
                        LOG((errorlog,"laser_fdc_w(%d) data($%04X) $%02X <- $%02X ($%04X)\n", offset, laser_fdc_offs, laser_fdc_data[laser_fdc_offs], value, laser_data));
                        laser_fdc_data[laser_fdc_offs] = value;
                        laser_fdc_offs = ++laser_fdc_offs % TRKSIZE_FM;
                        laser_fdc_write++;
                        laser_fdc_bits = 8;
                    }
                }
            }
            /* change of write signal? */
            if( (laser_fdc_latch ^ data) & 0x40 )
            {
                /* falling edge? */
                if ( laser_fdc_latch & 0x40 )
                {
                    sprintf(laser_frame_message, "#%d put track %02d", laser_drive, laser_track_x2[laser_drive]/2);
                    laser_frame_time = 30;
                    laser_fdc_start = laser_fdc_offs;
					laser_fdc_edge = 0;
                }
                else
                {
                    /* data written to track before? */
                    if( laser_fdc_write )
                        laser_put_track();
                }
                laser_fdc_bits = 8;
                laser_fdc_write = 0;
            }
        }
        laser_fdc_latch = data;
        break;
    }
}


