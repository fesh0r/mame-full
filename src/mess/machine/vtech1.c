/***************************************************************************
	vtech1.c

	Video Technology Models (series 1)
	Laser 110 monochrome
	Laser 210
		Laser 200 (same hardware?)
		aka VZ 200 (Australia)
		aka Salora Fellow (Finland)
		aka Texet8000 (UK)
	Laser 310
		aka VZ 300 (Australia)

    machine driver
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

	Thanks go to:
	- Guy Thomason
	- Jason Oakley
	- Bushy Maunder
	- and anybody else on the vzemu list :)
	- Davide Moretti for the detailed description of the colors.

    TODO:
		Add loading .vz images from WAV files.
		Printer and RS232 support.

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)	/* x */
#endif

extern char vtech1_frame_message[64+1];
extern int vtech1_frame_time;

int vtech1_latch = -1;

static const char *cassette_name = NULL;

#define TRKSIZE_VZ	0x9a0	/* arbitrary (actually from analyzing format) */
#define TRKSIZE_FM	3172	/* size of a standard FM mode track */

static void *vtech1_fdc_file[2] = {NULL, NULL};
static UINT8 vtech1_track_x2[2] = {80, 80};
static UINT8 vtech1_fdc_wrprot[2] = {0x80, 0x80};
static UINT8 vtech1_fdc_status = 0;
static UINT8 vtech1_fdc_data[TRKSIZE_FM];
static int vtech1_data;
static int vtech1_fdc_edge = 0;
static int vtech1_fdc_bits = 8;
static int vtech1_drive = -1;
static int vtech1_fdc_start = 0;
static int vtech1_fdc_write = 0;
static int vtech1_fdc_offs = 0;
static int vtech1_fdc_latch = 0;

void init_vtech1(void)
{
	int i;
    UINT8 *gfx = memory_region(REGION_GFX1);

	/* create 256 bit patterns */
    for( i = 0; i < 256; i++ )
        gfx[0x0c00+i] = i;
}

static int opbaseoverride(int PC)
{
	/* Did we hit the first timer interrupt? */
	if( PC == 0x0038 )
	{
		UINT8 *RAM = memory_region(REGION_CPU1);
		const char magic_basic[] = "VZF0";
		const char magic_mcode[] = "  \000\000";
		char name[17+1] = "";
		UINT16 start=0, end=0, addr, size;
		UINT8 type = 0;
		char buff[4];
		void *file;

		if( cassette_name && cassette_name[0] )
		{
			file = osd_fopen(Machine->gamedrv->name, cassette_name, OSD_FILETYPE_IMAGE_RW, 0);
			if( file )
			{

				osd_fread(file, buff, sizeof(buff));
				if( memcmp(buff, magic_basic, sizeof(buff)) == 0 ||
					memcmp(buff, magic_mcode, sizeof(buff)) == 0 )
				{
					osd_fread(file, &name, 17);
					osd_fread(file, &type, 1);
					osd_fread_lsbfirst(file, &addr, 2);
					name[17] = '\0';
					LOG((errorlog, "cassette load: %s ($%02X) addr $%04X\n", name, type, addr));
					start = addr;
					size = osd_fread(file, &RAM[addr], 0xffff - addr);
					osd_fclose(file);
					addr += size;
					if( type == 0xf0 ) /* Basic image? */
						end = addr - 2;
					else
					if( type == 0xf1 ) /* mcode image? */
						end = addr;
				}
				else
				{
                    LOG((errorlog, "cassette load: magic not found\n"));
				}
			}
			if( type == 0xf0 )
			{
				RAM[0x78a4] = start & 0xff;
				RAM[0x78a5] = start >> 8;
				RAM[0x78f9] = end & 0xff;
				RAM[0x78fa] = end >> 8;
				RAM[0x78fb] = end & 0xff;
				RAM[0x78fc] = end >> 8;
				RAM[0x78fd] = end & 0xff;
				RAM[0x78fe] = end >> 8;
			}
			else
			if( type == 0xf1 )
			{
				/* set USR() address */
				RAM[0x788e] = start & 0xff;
				RAM[0x788f] = start >> 8;
			}
		}
		cpu_setOPbaseoverride(0, NULL);
	}
	return PC;
}

static void common_init_machine(void)
{
	/* install DOS ROM ? */
    if( readinputport(0) & 0x40 )
    {
		void *dos;
		UINT8 *ROM = memory_region(REGION_CPU1);

		memset(vtech1_fdc_data, 0, TRKSIZE_FM);

        dos = osd_fopen(Machine->gamedrv->name, "vzdos.rom", OSD_FILETYPE_IMAGE_R, OSD_FOPEN_READ);
		if( dos )
        {
			osd_fread(dos, &ROM[0x4000], 0x2000);
			osd_fclose(dos);
        }
    }

    /* setup opbaseoverride handler to detect when to load a cassette image */
	cpu_setOPbaseoverride(0, opbaseoverride);
}

void laser110_init_machine(void)
{
    if( readinputport(0) & 0x80 )
    {
		install_mem_read_handler(0, 0x8000, 0xbfff, MRA_RAM);
		install_mem_write_handler(0, 0x8000, 0xbfff, MWA_RAM);
    }
    else
    {
		install_mem_read_handler(0, 0x8000, 0xbfff, MRA_NOP);
		install_mem_write_handler(0, 0x8000, 0xbfff, MWA_NOP);
    }
	common_init_machine();
}

void laser210_init_machine(void)
{
    if( readinputport(0) & 0x80 )
    {
        install_mem_read_handler(0, 0x9000, 0xcfff, MRA_RAM);
        install_mem_write_handler(0, 0x9000, 0xcfff, MWA_RAM);
    }
    else
    {
        install_mem_read_handler(0, 0x9000, 0xcfff, MRA_NOP);
        install_mem_write_handler(0, 0x9000, 0xcfff, MWA_NOP);
    }
	common_init_machine();
}

void laser310_init_machine(void)
{
    if( readinputport(0) & 0x80 )
    {
        install_mem_read_handler(0, 0xb800, 0xf7ff, MRA_RAM);
        install_mem_write_handler(0, 0xb800, 0xf7ff, MWA_RAM);
    }
    else
    {
        install_mem_read_handler(0, 0xb800, 0xf7ff, MRA_NOP);
        install_mem_write_handler(0, 0xb800, 0xf7ff, MWA_NOP);
    }
	common_init_machine();
}

void vtech1_shutdown_machine(void)
{
	int i;
	for( i = 0; i < 2; i++ )
	{
		if( vtech1_fdc_file[i] )
			osd_fclose(vtech1_fdc_file[i]);
		vtech1_fdc_file[i] = NULL;
	}
}

int vtech1_cassette_id(const char *name, const char *gamename)
{
    const char magic_basic[] = "VZF0";
    const char magic_mcode[] = "  \000\000";
    char buff[4];
    void *file;

    file = osd_fopen(gamename, name, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
    if( file )
    {
        osd_fread(file, buff, sizeof(buff));
        if( memcmp(buff, magic_basic, sizeof(buff)) == 0 )
        {
			LOG((errorlog, "vtech1_cassette_id: BASIC magic '%s' found\n", magic_basic));
            return 0;
        }
        if( memcmp(buff, magic_mcode, sizeof(buff)) == 0 )
        {
			LOG((errorlog, "vtech1_cassette_id: MCODE magic '%s' found\n", magic_mcode));
            return 0;
        }
    }
    return 1;
}

int vtech1_cassette_init(int id, const char *name)
{
    cassette_name = name;
    return 0;
}

void vtech1_cassette_exit(int id)
{
	cassette_name = NULL;
}

int vtech1_floppy_id(const char *name, const char *gamename)
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

int vtech1_floppy_init(int id, const char *name)
{
	if( name && name[0] )
	{
		/* first try to open existing image RW */
		vtech1_fdc_wrprot[id] = 0x00;
		vtech1_fdc_file[id] = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_RW);
		/* failed? */
		if( !vtech1_fdc_file[id] )
		{
			/* try to open existing image RO */
			vtech1_fdc_wrprot[id] = 0x80;
			vtech1_fdc_file[id] = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
		}
		/* failed? */
		if( !vtech1_fdc_file[id] )
		{
			/* create new image RW */
			vtech1_fdc_wrprot[id] = 0x00;
			vtech1_fdc_file[id] = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_RW_CREATE);
		}
	}
	if( vtech1_fdc_file[id] )
		return 0;
	/* failed permanently */
    return 1;
}

void vtech1_floppy_exit(int id)
{
	if( vtech1_fdc_file[id] )
		osd_fclose(vtech1_fdc_file[id]);
	vtech1_fdc_file[id] = NULL;
}

static void vtech1_get_track(void)
{
	sprintf(vtech1_frame_message, "#%d get track %02d", vtech1_drive, vtech1_track_x2[vtech1_drive]/2);
	vtech1_frame_time = 30;
    /* drive selected or and image file ok? */
	if( vtech1_drive >= 0 && vtech1_fdc_file[vtech1_drive] != NULL )
	{
		int size, offs;
		size = TRKSIZE_VZ;
		offs = TRKSIZE_VZ * vtech1_track_x2[vtech1_drive]/2;
		osd_fseek(vtech1_fdc_file[vtech1_drive], offs, SEEK_SET);
		size = osd_fread(vtech1_fdc_file[vtech1_drive], vtech1_fdc_data, size);
		LOG((errorlog,"get track @$%05x $%04x bytes\n", offs, size));
    }
	vtech1_fdc_offs = 0;
	vtech1_fdc_write = 0;
}

static void vtech1_put_track(void)
{
    /* drive selected and image file ok? */
	if( vtech1_drive >= 0 && vtech1_fdc_file[vtech1_drive] != NULL )
	{
		int size, offs;
		offs = TRKSIZE_VZ * vtech1_track_x2[vtech1_drive]/2;
		osd_fseek(vtech1_fdc_file[vtech1_drive], offs + vtech1_fdc_start, SEEK_SET);
		size = osd_fwrite(vtech1_fdc_file[vtech1_drive], &vtech1_fdc_data[vtech1_fdc_start], vtech1_fdc_write);
		LOG((errorlog,"put track @$%05X+$%X $%04X/$%04X bytes\n", offs, vtech1_fdc_start, size, vtech1_fdc_write));
    }
}

#define PHI0(n) (((n)>>0)&1)
#define PHI1(n) (((n)>>1)&1)
#define PHI2(n) (((n)>>2)&1)
#define PHI3(n) (((n)>>3)&1)

int vtech1_fdc_r(int offset)
{
    int data = 0xff;
    switch( offset )
    {
    case 1: /* data (read-only) */
		if( vtech1_fdc_bits > 0 )
        {
			if( vtech1_fdc_status & 0x80 )
				vtech1_fdc_bits--;
			data = (vtech1_data >> vtech1_fdc_bits) & 0xff;
#if 0
			LOG((errorlog,"vtech1_fdc_r bits %d%d%d%d%d%d%d%d\n",
                (data>>7)&1,(data>>6)&1,(data>>5)&1,(data>>4)&1,
                (data>>3)&1,(data>>2)&1,(data>>1)&1,(data>>0)&1 ));
#endif
        }
		if( vtech1_fdc_bits == 0 )
        {
			vtech1_data = vtech1_fdc_data[vtech1_fdc_offs];
			LOG((errorlog,"vtech1_fdc_r %d : data ($%04X) $%02X\n", offset, vtech1_fdc_offs, vtech1_data));
			if( vtech1_fdc_status & 0x80 )
            {
				vtech1_fdc_bits = 8;
				vtech1_fdc_offs = ++vtech1_fdc_offs % TRKSIZE_FM;
            }
			vtech1_fdc_status &= ~0x80;
        }
        break;
    case 2: /* polling (read-only) */
        /* fake */
		if( vtech1_drive >= 0 )
			vtech1_fdc_status |= 0x80;
		data = vtech1_fdc_status;
        break;
    case 3: /* write protect status (read-only) */
		if( vtech1_drive >= 0 )
			data = vtech1_fdc_wrprot[vtech1_drive];
		LOG((errorlog,"vtech1_fdc_r %d : write_protect $%02X\n", offset, data));
        break;
    }
    return data;
}

void vtech1_fdc_w(int offset, int data)
{
	int drive;

    switch( offset )
	{
	case 0: /* latch (write-only) */
		drive = (data & 0x10) ? 0 : (data & 0x80) ? 1 : -1;
		if( drive != vtech1_drive )
		{
			vtech1_drive = drive;
			if( vtech1_drive >= 0 )
				vtech1_get_track();
        }
		if( vtech1_drive >= 0 )
        {
			if( (PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI1(vtech1_fdc_latch)) ||
				(PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI2(vtech1_fdc_latch)) ||
				(PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI3(vtech1_fdc_latch)) ||
				(PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI0(vtech1_fdc_latch)) )
            {
				if( vtech1_track_x2[vtech1_drive] > 0 )
					vtech1_track_x2[vtech1_drive]--;
				LOG((errorlog,"vtech1_fdc_w(%d) $%02X drive %d: stepout track #%2d.%d\n", offset, data, vtech1_drive, vtech1_track_x2[vtech1_drive]/2,5*(vtech1_track_x2[vtech1_drive]&1)));
				if( (vtech1_track_x2[vtech1_drive] & 1) == 0 )
					vtech1_get_track();
            }
            else
			if( (PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI3(vtech1_fdc_latch)) ||
				(PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI0(vtech1_fdc_latch)) ||
				(PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI1(vtech1_fdc_latch)) ||
				(PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI2(vtech1_fdc_latch)) )
            {
				if( vtech1_track_x2[vtech1_drive] < 2*40 )
					vtech1_track_x2[vtech1_drive]++;
				LOG((errorlog,"vtech1_fdc_w(%d) $%02X drive %d: stepin track #%2d.%d\n", offset, data, vtech1_drive, vtech1_track_x2[vtech1_drive]/2,5*(vtech1_track_x2[vtech1_drive]&1)));
				if( (vtech1_track_x2[vtech1_drive] & 1) == 0 )
					vtech1_get_track();
            }
            if( (data & 0x40) == 0 )
			{
				vtech1_data <<= 1;
				if( (vtech1_fdc_latch ^ data) & 0x20 )
					vtech1_data |= 1;
				if( (vtech1_fdc_edge ^= 1) == 0 )
                {
					if( --vtech1_fdc_bits == 0 )
					{
						UINT8 value = 0;
						vtech1_data &= 0xffff;
						if( vtech1_data & 0x4000 ) value |= 0x80;
						if( vtech1_data & 0x1000 ) value |= 0x40;
						if( vtech1_data & 0x0400 ) value |= 0x20;
						if( vtech1_data & 0x0100 ) value |= 0x10;
						if( vtech1_data & 0x0040 ) value |= 0x08;
						if( vtech1_data & 0x0010 ) value |= 0x04;
						if( vtech1_data & 0x0004 ) value |= 0x02;
						if( vtech1_data & 0x0001 ) value |= 0x01;
						LOG((errorlog,"vtech1_fdc_w(%d) data($%04X) $%02X <- $%02X ($%04X)\n", offset, vtech1_fdc_offs, vtech1_fdc_data[vtech1_fdc_offs], value, vtech1_data));
						vtech1_fdc_data[vtech1_fdc_offs] = value;
						vtech1_fdc_offs = ++vtech1_fdc_offs % TRKSIZE_FM;
						vtech1_fdc_write++;
						vtech1_fdc_bits = 8;
					}
                }
            }
			/* change of write signal? */
			if( (vtech1_fdc_latch ^ data) & 0x40 )
            {
                /* falling edge? */
				if ( vtech1_fdc_latch & 0x40 )
                {
					sprintf(vtech1_frame_message, "#%d put track %02d", vtech1_drive, vtech1_track_x2[vtech1_drive]/2);
					vtech1_frame_time = 30;
					vtech1_fdc_start = vtech1_fdc_offs;
					vtech1_fdc_edge = 0;
                }
                else
                {
                    /* data written to track before? */
					if( vtech1_fdc_write )
						vtech1_put_track();
                }
				vtech1_fdc_bits = 8;
				vtech1_fdc_write = 0;
            }
        }
		vtech1_fdc_latch = data;
		break;
    }
}

int vtech1_joystick_r(int offset)
{
    int data = 0xff;

	if( !(offset & 1) )
		data &= readinputport(10);
	if( !(offset & 2) )
		data &= readinputport(11);
	if( !(offset & 4) )
		data &= readinputport(12);
	if( !(offset & 8) )
		data &= readinputport(13);

    return data;
}

#define KEY_INV 0x80
#define KEY_RUB 0x40
#define KEY_LFT 0x20
#define KEY_DN  0x10
#define KEY_RGT 0x08
#define KEY_BSP 0x04
#define KEY_UP  0x02
#define KEY_INS 0x01

int vtech1_keyboard_r(int offset)
{
    int data = 0xff;

    if( !(offset & 0x01) )
    {
        data &= readinputport(1);
    }
    if( !(offset & 0x02) )
    {
        data &= readinputport(2);
		/* extra keys pressed? */
		if( readinputport(9) != 0xff )
			data &= ~0x04;
    }
    if( !(offset & 0x04) )
    {
        data &= readinputport(3);
    }
    if( !(offset & 0x08) )
    {
        data &= readinputport(4);
    }
    if( !(offset & 0x10) )
    {
		int extra = readinputport(9);
        data &= readinputport(5);
		/* easy cursor keys */
		data &= extra | ~(KEY_UP|KEY_DN|KEY_LFT|KEY_RGT);
		/* backspace does cursor left too */
		if( !(extra & KEY_BSP) )
			data &= ~KEY_LFT;
    }
    if( !(offset & 0x20) )
    {
        data &= readinputport(6);
    }
    if( !(offset & 0x40) )
    {
        data &= readinputport(7);
    }
    if( !(offset & 0x80) )
    {
		int extra = readinputport(9);
        data &= readinputport(8);
		if( !(extra & KEY_INV) )
			data &= ~0x04;
		if( !(extra & KEY_RUB) )
			data &= ~0x10;
		if( !(extra & KEY_INS) )
			data &= ~0x02;
    }

    if( cpu_getscanline() >= 16*12 )
        data &= ~0x80;

    /* cassette input would be bit 5 (0x40) */

    return data;
}

/*************************************************
 * bit  function
 * 7-6  not assigned
 * 5    speaker B
 * 4    VDC background 0 green, 1 orange
 * 3    VDC display mode 0 text, 1 graphics
 * 2    cassette out (MSB)
 * 1    cassette out (LSB)
 * 0    speaker A
 ************************************************/
void vtech1_latch_w(int offset, int data)
{
    int dac = 0;

	LOG((errorlog, "vtech1_latch_w $%02X\n", data));
    /* dirty all if the mode or the background color are toggled */
	if( (vtech1_latch ^ data) & 0x18 )
	{
		extern int bitmap_dirty;
        bitmap_dirty = 1;
		if( (vtech1_latch ^ data) & 0x10 )
			LOG((errorlog, "vtech1_latch_w: change background %d", (data>>4)&1));
		if( (vtech1_latch ^ data) & 0x08 )
			LOG((errorlog, "vtech1_latch_w: change mode to %s", (data&0x08)?"gfx":"text"));
    }
	vtech1_latch = data;

	/* cassette output bits */
	dac = (vtech1_latch & 0x06) * 8;

	/* speaker B push */
	if( vtech1_latch & 0x20 )
        dac += 48;
	/* speaker B pull */
	if( vtech1_latch & 0x01 )
        dac -= 48;

    DAC_signed_data_w(0, dac);
}


