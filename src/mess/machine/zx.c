/***************************************************************************
	zx.c

    machine driver
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

	TODO:
	Find a clean solution for putting the tape image into memory.
	Right now only loading through the IO emulation works (and takes time :)

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

#define VERBOSE 1

#if VERBOSE
#define LOG(n,x)  if( errorlog && VERBOSE >= n ) fprintf x
#else
#define LOG(n,x)
#endif

extern char zx_frame_message[128];
extern int zx_frame_time;

/* from vidhrdw/zx.c */
extern void zx_ula_bkgnd(int color);
extern int zx_ula_r(int offs, int region);
extern void zx_ula_nmi(int param);
extern void zx_ula_irq(int param);

extern void *ula_nmi;
extern void *ula_irq;
extern int ula_frame_vsync;
extern int ula_scanline_count;
extern int ula_scancode_count;

/* statics */
#define TAPE_PULSE		348
#define TAPE_DELAY		1648

static int tape_size = 0;
static void *tape_image = NULL;

static void *tape_file = NULL;
static UINT8 tape_data = 0x00;
static UINT8 tape_mask = 0x00;
static void *tape_bit_timer = NULL;
static int tape_header = 0;			   /* silence counter */
static int tape_trailer = 0;
static int tape_data_offs = 0;
static UINT8 tape_dump[16];
static char tape_name[16 + 1];
static int tape_name_offs = 0;
static int tape_name_size = 0;

void init_zx(void)
{
	UINT8 *gfx = memory_region(REGION_GFX1);
	int i;

	for (i = 0; i < 256; i++)
		gfx[i] = i;
}

static int zx_setopbase(UINT32 pc)
{
	if (pc & 0x8000)
		return zx_ula_r(pc, REGION_CPU1);
	else
	if (pc == 0x0066 && tape_size > 0)
	{
		UINT8 *ram = memory_region(REGION_CPU1);
		UINT8 border = ram[0x4028];

		memcpy(&ram[0x4009], tape_image, tape_size);
		ram[0x4028] = border;
		tape_size = 0;
		ram[0x03a7] = 0xff;
		pc = 0x03a6;
	}
	return pc;
}

static int pc8300_setopbase(UINT32 pc)
{
	if (pc & 0x8000)
		return zx_ula_r(pc, REGION_GFX2);
	else if (pc == 0x0066 && tape_size > 0)
	{
		UINT8 *ram = memory_region(REGION_CPU1);
		UINT8 border = ram[0x4028];

		memcpy(&ram[0x4009], tape_image, tape_size);
		ram[0x4028] = border;
		tape_size = 0;
		ram[0x03a7] = 0xff;
		pc = 0x03a6;
	}
	return pc;
}

static int pow3000_setopbase(UINT32 pc)
{
	if (pc & 0x8000)
		return zx_ula_r(pc, REGION_GFX2);
	return pc;
}

static void common_init_machine(void)
{
	cpu_setOPbaseoverride(0, zx_setopbase);
}

void zx80_init_machine(void)
{
	if (readinputport(0) & 0x80)
	{
		install_mem_read_handler(0, 0x4400, 0x7fff, MRA_RAM);
		install_mem_write_handler(0, 0x4400, 0x7fff, MWA_RAM);
	}
	else
	{
		install_mem_read_handler(0, 0x4400, 0x7fff, MRA_NOP);
		install_mem_write_handler(0, 0x4400, 0x7fff, MWA_NOP);
	}
	common_init_machine();
}

void zx81_init_machine(void)
{
	if (readinputport(0) & 0x80)
	{
		install_mem_read_handler(0, 0x4400, 0x7fff, MRA_RAM);
		install_mem_write_handler(0, 0x4400, 0x7fff, MWA_RAM);
	}
	else
	{
		install_mem_read_handler(0, 0x4400, 0x7fff, MRA_NOP);
		install_mem_write_handler(0, 0x4400, 0x7fff, MWA_NOP);
	}
	common_init_machine();
}

void pc8300_init_machine(void)
{
	cpu_setOPbaseoverride(0, pc8300_setopbase);
}

void pow3000_init_machine(void)
{
	cpu_setOPbaseoverride(0, pow3000_setopbase);
}

void zx_shutdown_machine(void)
{
}

int zx_cassette_init(int id)
{
	void *file;

	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);
	if (file)
	{
		tape_size = osd_fsize(file);
		tape_image = malloc(tape_size);
		if (tape_image)
		{
			if (osd_fread(file, tape_image, tape_size) != tape_size)
			{
				osd_fclose(file);
				return 1;
			}
		}
		else
		{
			tape_size = 0;
		}
		osd_fclose(file);
	}
	return 0;
}

void zx_cassette_exit(int id)
{
	if (tape_image)
	{
		free(tape_image);
		tape_image = 0;
	}
}

static void tape_bit_shift(int param)
{
	int tape_wave = param & 15;
	int tape_bits = param >> 4;

	tape_bit_timer = NULL;

	if (tape_header > 0)
	{
		tape_bit_timer = timer_set(TIME_IN_USEC(TAPE_PULSE), 0, tape_bit_shift);
		tape_header--;
		zx_frame_time = 15;
		sprintf(zx_frame_message, "Tape header %5d", tape_header);
		return;
	}

	if (tape_trailer > 0)
	{
		tape_bit_timer = timer_set(TIME_IN_USEC(TAPE_PULSE), 0, tape_bit_shift);
		tape_trailer--;
		zx_frame_time = 15;
		sprintf(zx_frame_message, "Tape trailer %5d", tape_trailer);
		return;
	}

	if (tape_wave)
	{
		tape_mask ^= 0x80;
		zx_ula_bkgnd(tape_mask ? 1 : 0);
		if (tape_wave == 1)
		{
			LOG(3, (errorlog, "TAPE wave #%d done (%02X AF:%04X BC:%04X DE:%04X HL:%04X)\n", param, tape_mask, cpu_get_reg
					(Z80_AF), cpu_get_reg(Z80_BC), cpu_get_reg(Z80_DE), cpu_get_reg(Z80_HL)));
			tape_bit_timer = timer_set(TIME_IN_USEC(TAPE_DELAY),
									   (tape_bits << 4), tape_bit_shift);
		}
		else
		{
			LOG(3, (errorlog, "TAPE wave #%d      (%02X AF:%04X BC:%04X DE:%04X HL:%04X)\n", param, tape_mask, cpu_get_reg
					(Z80_AF), cpu_get_reg(Z80_BC), cpu_get_reg(Z80_DE), cpu_get_reg(Z80_HL)));
			tape_bit_timer = timer_set(TIME_IN_USEC(tape_mask ? TAPE_PULSE : TAPE_PULSE * 6 / 7),
									   (tape_bits << 4) | (tape_wave - 1), tape_bit_shift);
		}
		return;
	}

	if (tape_bits == 0)
	{
		if (tape_name_offs < tape_name_size)
		{
			tape_data = tape_dump[tape_name_offs];
			if (errorlog)
				fprintf(errorlog, "TAPE name @$%04X: $%02X (%02X AF:%04X BC:%04X DE:%04X HL:%04X)\n", tape_name_offs, tape_data,
						tape_mask, cpu_get_reg(Z80_AF), cpu_get_reg(Z80_BC), cpu_get_reg(Z80_DE), cpu_get_reg(Z80_HL));
			tape_bits = 8;
			tape_name_offs++;
			zx_frame_time = 15;
			sprintf(zx_frame_message, "Tape name %04X:%02X", tape_name_offs, tape_data);
		}
		else if (osd_fread(tape_file, &tape_data, 1) == 1)
		{
			if (errorlog)
				fprintf(errorlog, "TAPE data @$%04X: $%02X (%02X AF:%04X BC:%04X DE:%04X HL:%04X)\n", tape_data_offs, tape_data,
						tape_mask, cpu_get_reg(Z80_AF), cpu_get_reg(Z80_BC), cpu_get_reg(Z80_DE), cpu_get_reg(Z80_HL));
			tape_bits = 8;
			tape_data_offs++;
			zx_frame_time = 15;
			sprintf(zx_frame_message, "Tape data %04X:%02X", tape_data_offs, tape_data);
		}
		else if (tape_file)
		{
			osd_fclose(tape_file);
			tape_file = NULL;
			tape_trailer = 256 * 8;
			tape_bit_timer = timer_set(TIME_IN_USEC(TAPE_PULSE), 0, tape_bit_shift);
			if (errorlog)
				fprintf(errorlog, "TAPE trailer %d\n", tape_trailer);
		}
	}

	if (tape_bits)
	{
		tape_bits--;
		if ((tape_data >> tape_bits) & 1)
		{
			if (errorlog)
				fprintf(errorlog, "TAPE get bit #%d:1 (%02X AF:%04X BC:%04X DE:%04X HL:%04X)\n", tape_bits, tape_mask,
						cpu_get_reg(Z80_AF), cpu_get_reg(Z80_BC), cpu_get_reg(Z80_DE), cpu_get_reg(Z80_HL));
			tape_wave = 9;
		}
		else
		{
			if (errorlog)
				fprintf(errorlog, "TAPE get bit #%d:0 (%02X AF:%04X BC:%04X DE:%04X HL:%04X)\n", tape_bits, tape_mask,
						cpu_get_reg(Z80_AF), cpu_get_reg(Z80_BC), cpu_get_reg(Z80_DE), cpu_get_reg(Z80_HL));
			tape_mask ^= 0x80;
			zx_ula_bkgnd(tape_mask ? 1 : 0);
			tape_wave = 4;
		}
		tape_bit_timer = timer_set(TIME_IN_USEC(TAPE_PULSE),
								   (tape_bits << 4) | (tape_wave - 1), tape_bit_shift);
	}
}

static void extract_name(void)
{
	static char zx2pc[64] =
	{
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		' ', ' ', ' ', '"', '$', '$', ':', '?',
		'(', ')', '>', '<', '=', '+', '-', '*',
		'/', ';', ',', '.', '0', '1', '2', '3',
		'4', '5', '6', '7', '8', '9', 'A', 'B',
		'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
		'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
		'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
	int a, de = cpu_get_reg(Z80_DE), i;
	UINT8 *ram = memory_region(REGION_CPU1), *name;

	tape_name[0] = '\0';
	/* find a LOAD token starting at (DE) */
	name = memchr(&ram[de], 0xef, 32);
	if (name && name[1] == 0x0b)	   /* load followed by doublequote? */
	{
		name += 2;
		for (i = 0; i < 16; i++)
		{
			a = name[i];
			tape_dump[i] = a;
			tape_name[i] = zx2pc[a & 0x3f];
			if (a & 0x80)
				break;
		}
		tape_name_size = i + 1;
		tape_name[tape_name_size] = '\0';
		tape_name_offs = 0;
		LOG(1, (errorlog, "extracted tape name '%s'\n", tape_name));
	}
	else
	{
		LOG(1, (errorlog, "no tape name found"));
	}
}

int zx_tape_get_bit(void)
{
	if (tape_file || tape_header || tape_trailer)
	{
		LOG(4, (errorlog, "      read status (%02X AF:%04X BC:%04X DE:%04X HL:%04X)\n", tape_mask, cpu_get_reg(Z80_AF),
				cpu_get_reg(Z80_BC), cpu_get_reg(Z80_DE), cpu_get_reg(Z80_HL)));
	}
	else
	{
		static int cycles_last_bit = 0, fast_read_count = 0;
		int cycles_this_bit;

		cycles_this_bit = cpu_gettotalcycles();
		/* check if there's a tight loop reading the tape input */
		if (cycles_this_bit - cycles_last_bit < 64)
		{
			fast_read_count++;
			LOG(2, (errorlog, "TAPE time between reads %d cycles %d times\n", cycles_this_bit - cycles_last_bit, fast_read_count));
			if (fast_read_count > 64)
			{
				extract_name();
				if (tape_name[0])
				{
					char *ext = tape_name + strlen(tape_name);

					strcpy(ext, ".P");
					tape_file = osd_fopen(Machine->gamedrv->name, tape_name, OSD_FILETYPE_ROM, 0);
					if (!tape_file)
					{
						strcpy(ext, ".81");
						tape_file = osd_fopen(Machine->gamedrv->name, tape_name, OSD_FILETYPE_ROM, 0);
					}
					if (!tape_file && Machine->gamedrv->clone_of)
					{
						strcpy(ext, ".P");
						tape_file = osd_fopen(Machine->gamedrv->clone_of->name, tape_name, OSD_FILETYPE_ROM, 0);
					}
					if (!tape_file && Machine->gamedrv->clone_of)
					{
						strcpy(ext, ".81");
						tape_file = osd_fopen(Machine->gamedrv->clone_of->name, tape_name, OSD_FILETYPE_ROM, 0);
					}
					if (tape_file)
					{
						tape_bit_timer = timer_set(TIME_IN_USEC(TAPE_PULSE), 0, tape_bit_shift);
						tape_header = 1024 * 8;
						tape_data_offs = 0;
						tape_mask = 0x80;
						if (errorlog)
							fprintf(errorlog, "TAPE header %d\n", tape_header);
					}
				}
			}
		}
		else
		{
			fast_read_count = 0;
		}
		cycles_last_bit = cycles_this_bit;
	}

	return tape_mask;
}

void zx_io_w(int offs, int data)
{
	LOG(2, (errorlog, "IOW %3d $%04X", cpu_getscanline(), offs));
	if ((offs & 2) == 0)
	{
		LOG(2, (errorlog, " ULA NMIs off\n"));
		if (ula_nmi)
			timer_remove(ula_nmi);
		ula_nmi = NULL;
	}
	else if ((offs & 1) == 0)
	{
		LOG(2, (errorlog, " ULA NMIs on\n"));
		ula_nmi = timer_pulse(TIME_IN_CYCLES(207, 0), 0, zx_ula_nmi);
		/* remove the IRQ */
		if (ula_irq)
		{
			timer_remove(ula_irq);
			ula_irq = NULL;
		}
	}
	else
	{
		LOG(2, (errorlog, " ULA IRQs on\n"));
		zx_ula_bkgnd(1);
		if (ula_frame_vsync == 2)
		{
			cpu_spinuntil_time(cpu_getscanlinetime(Machine->drv->screen_height - 1));
			ula_scanline_count = Machine->drv->screen_height - 1;
		}
	}
}

int zx_io_r(int offs)
{
	int data = 0xff;

	if ((offs & 1) == 0)
	{
		int extra1 = readinputport(9);
		int extra2 = readinputport(10);

		ula_scancode_count = 0;
		if ((offs & 0x0100) == 0)
		{
			data &= readinputport(1);
			/* SHIFT for extra keys */
			if (extra1 != 0xff || extra2 != 0xff)
				data &= ~0x01;
		}
		if ((offs & 0x0200) == 0)
			data &= readinputport(2);
		if ((offs & 0x0400) == 0)
			data &= readinputport(3);
		if ((offs & 0x0800) == 0)
			data &= readinputport(4) & extra1;
		if ((offs & 0x1000) == 0)
			data &= readinputport(5) & extra2;
		if ((offs & 0x2000) == 0)
			data &= readinputport(6);
		if ((offs & 0x4000) == 0)
			data &= readinputport(7);
		if ((offs & 0x8000) == 0)
			data &= readinputport(8);
		if (Machine->drv->frames_per_second > 55)
			data &= ~0x40;

		if (ula_irq)
		{
			LOG(2, (errorlog, "IOR %3d $%04X data $%02X (ULA IRQs off)\n", cpu_getscanline(), offs, data));
			zx_ula_bkgnd(0);
			timer_remove(ula_irq);
			ula_irq = NULL;
		}
		else
		{
			data &= ~zx_tape_get_bit();
			LOG(2, (errorlog, "IOR %3d $%04X data $%02X (tape)\n", cpu_getscanline(), offs, data));
		}
		if (ula_frame_vsync == 3)
		{
			ula_frame_vsync = 2;
			LOG(2, (errorlog, "vsync starts in scanline %3d\n", cpu_getscanline()));
		}
	}
	else
	{
		LOG(2, (errorlog, "IOR %3d $%04X data $%02X\n", cpu_getscanline(), offs, data));
	}
	return data;
}

int pow3000_io_r(int offs)
{
	int data = 0xff;

	if ((offs & 1) == 0)
	{
		int extra1 = readinputport(9);
		int extra2 = readinputport(10);

		ula_scancode_count = 0;
		if ((offs & 0x0100) == 0)
		{
			data &= readinputport(1) & extra1;
			/* SHIFT for extra keys */
			if (extra1 != 0xff || extra2 != 0xff)
				data &= ~0x01;
		}
		if ((offs & 0x0200) == 0)
			data &= readinputport(2) & extra2;
		if ((offs & 0x0400) == 0)
			data &= readinputport(3);
		if ((offs & 0x0800) == 0)
			data &= readinputport(4);
		if ((offs & 0x1000) == 0)
			data &= readinputport(5);
		if ((offs & 0x2000) == 0)
			data &= readinputport(6);
		if ((offs & 0x4000) == 0)
			data &= readinputport(7);
		if ((offs & 0x8000) == 0)
			data &= readinputport(8);
		if (Machine->drv->frames_per_second > 55)
			data &= ~0x40;

		if (ula_irq)
		{
			LOG(2, (errorlog, "IOR %3d $%04X data $%02X (ULA IRQs off)\n", cpu_getscanline(), offs, data));
			zx_ula_bkgnd(0);
			timer_remove(ula_irq);
			ula_irq = 0;
		}
		else
		{
			data &= ~zx_tape_get_bit();
			LOG(2, (errorlog, "IOR %3d $%04X data $%02X (tape)\n", cpu_getscanline(), offs, data));
		}
		if (ula_frame_vsync == 3)
		{
			ula_frame_vsync = 2;
			LOG(2, (errorlog, "vsync starts in scanline %3d\n", cpu_getscanline()));
		}
	}
	else
	{
		LOG(2, (errorlog, "IOR %3d $%04X data $%02X\n", cpu_getscanline(), offs, data));
	}
	return data;
}
