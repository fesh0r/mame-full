/*
	mt3200.c: emulation of the mt3200 tape controller, used by TILINE-based TI990 systems
	(TI990/10, /12, /12LR, /10A, Business system 300 and 600)


	Short description (946250-9701F, page 3-5):
	"Streaming cache, serial-access, 9-track magnetic tape transport.  Standard recording
	density of 1600 or 3200 bpi (phase-encoded)."

	Long description: see 2234398-9701.


	This device should be mostly compatible with mt1600, with the difference that mt1600
	supports low-density NRZI-encoded tapes (800bpi) and does not support high-density
	3200bpi tapes.
*/

#include "mt3200.h"

typedef struct tape_unit_t
{
	void *fd;		/* file descriptor */
	int id;			/* MESS id */
	unsigned int bot : 1;	/* TRUE if we are at the beginning of tape */
	unsigned int eot : 1;	/* TRUE if we are at the end of tape */
	unsigned int wp : 1;	/* TRUE if tape is write-protected */
} tape_unit_t;

typedef struct mt3200_t
{
	UINT16 w[8];

	tape_unit_t t[2];
} mt3200_t;

enum
{
	w0_offline			= 0x8000,
	w0_BOT				= 0x4000,
	w0_EOR				= 0x2000,
	w0_EOF				= 0x1000,
	w0_EOT				= 0x0800,
	w0_write_ring		= 0x0400,
	w0_tape_rewinding	= 0x0200,
	w0_command_timeout	= 0x0100,

	w0_rewind_status	= 0x00f0,
	w0_rewind_mask		= 0x000f,

	w6_unit0_sel		= 0x8000,
	w6_unit1_sel		= 0x4000,
	w6_command			= 0x0f00,

	w7_idle				= 0x8000,
	w7_complete			= 0x4000,
	w7_error			= 0x2000,
	w7_int_enable		= 0x1000,
	w7_PE_format		= 0x0200,
	w7_abnormal_completion	= 0x0100,
	w7_interface_parity_err	= 0x0080,
	w7_err_correction_enabled	= 0x0040,
	w7_hard_error			= 0x0020,
	w7_tiline_parity_err	= 0x0010,
	w7_tiline_timing_err	= 0x0008,
	w7_tiline_timeout_err	= 0x0004,
	/*w7_format_error		= 0x0002,*/
	w7_tape_error		= 0x0001
};

static mt3200_t mt3200;


/*
	Image encoding:


	2 bytes: record len - little-endian
	2 bytes: always 0s (length MSBs?)
	len bytes: data
	2 bytes: record len - little-endian
	2 bytes: always 0s (length MSBs?)

	4 0s: EOF mark?
*/


int mt3200_tape_init(int id)
{
	tape_unit_t *t;

	if ((id < 0) || (id > 1))
		return INIT_FAIL;

	t = &mt3200.t[id];
	memset(t, 0, sizeof(*t));
	t->id = id;

	if (!device_filename(IO_CASSETTE,id))
		return INIT_PASS;

	t->fd = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_RW);
	if (!t->fd)
	{
		t->fd = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_READ);
		if (!t->fd)
			goto error;
		t->wp = 1;
	}
	t->bot = 1;

	return INIT_PASS;

error:
	if (t->fd)
		osd_fclose(t->fd);
	return INIT_FAIL;
}

void mt3200_tape_exit(int id)
{
	tape_unit_t *t;

	if ((id < 0) || (id > 1))
		return;

	t = &mt3200.t[id];

	if (t->fd)
	{
		osd_fclose(t->fd);
		t->fd = NULL;
		t->wp = 0;
		t->bot = 0;
		t->eot = 0;
	}
}

void mt3200_reset(void)
{
	memset(mt3200.w, 0, sizeof(mt3200.w));
	mt3200.w[7] = w7_idle | w7_PE_format;
}

static tape_unit_t *cur_tape_unit(void)
{
	if (mt3200.w[6] & w6_unit0_sel)
		return &mt3200.t[0];
	else if (mt3200.w[6] & w6_unit1_sel)
		return &mt3200.t[1];
	else
		return NULL;
}

static void read_record(void)
{
	UINT8 buffer[256];
	int reclen;

	int dma_address;
	int char_count;
	int read_offset;

	int rec_count;
	int chunk_len;
	int bytes_to_read;
	int bytes_read;
	int i;


	tape_unit_t *t = cur_tape_unit();

	if (!t)
	{
		/* No idea what to report... */
		mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
		return;
	}
	else if (! t->fd)
	{	/* offline */
		mt3200.w[0] |= w0_offline;
		mt3200.w[7] |= w7_idle | w7_error | w7_tape_error;
		return;
	}
#if 0
	else if (0)
	{	/* rewind in progress */
		mt3200.w[0] |= /*...*/;
		mt3200.w[7] |= w7_idle | w7_error | w7_tape_error;
		return;
	}
#endif

	t->bot = 0;

	dma_address = ((((int) mt3200.w[6]) << 16) | mt3200.w[5]) & 0x1ffffe;
	char_count = mt3200.w[4];
	read_offset = mt3200.w[3];

	bytes_read = osd_fread(t->fd, buffer, 4);
	if (bytes_read != 4)
	{
		if (bytes_read == 0)
		{	/* legitimate EOF */
			mt3200.w[0] |= w0_EOT;	/* or should it be w0_command_timeout? */
			mt3200.w[7] |= w7_idle | w7_error | w7_tape_error;
			goto update_registers;
		}
		else
		{	/* illegitimate EOF */
			/* No idea what to report... */
			mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
			goto update_registers;
		}
	}
	reclen = (((int) buffer[1]) << 8) | buffer[0];
	if (buffer[2] || buffer[3])
	{	/* no idea what these bytes mean */
		logerror("Tape format looks gooofy\n");
		/* eject tape to avoid catastrophes */
		device_filename_change(IO_CASSETTE, t->id, NULL);
		mt3200.w[0] |= w0_offline;
		mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
		goto update_registers;
	}

#if 1
	if (reclen > 0x8000)
	{	/* eject tape (correct?) */
		device_filename_change(IO_CASSETTE, t->id, NULL);
		mt3200.w[0] |= w0_offline;
		mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
		goto update_registers;
	}
#endif

	if (reclen == 0)
	{	/* EOF mark (mere guess) */
		mt3200.w[0] |= w0_EOF;
		mt3200.w[7] |= w7_idle | w7_error | w7_tape_error;
		goto update_registers;
	}

	rec_count = reclen;

	/* skip up to read_offset bytes */
	chunk_len = (read_offset > rec_count) ? rec_count : read_offset;

	if (osd_fseek(t->fd, chunk_len, SEEK_CUR))
	{	/* eject tape */
		device_filename_change(IO_CASSETTE, t->id, NULL);
		mt3200.w[0] |= w0_offline;
		mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
		goto update_registers;
	}

	rec_count -= chunk_len;
	read_offset -= chunk_len;
	if (read_offset)
	{
		mt3200.w[0] |= w0_EOR;
		mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
		goto skip_trailer;
	}

	/* read up to char_count bytes */
	chunk_len = (char_count > rec_count) ? rec_count : char_count;

	for (; chunk_len>0; )
	{
		bytes_to_read = (chunk_len < sizeof(buffer)) ? chunk_len : sizeof(buffer);
		bytes_read = osd_fread(t->fd, buffer, bytes_to_read);

		if (bytes_read & 1)
		{
			buffer[bytes_read] = 0xff;
		}

		/* DMA */
		for (i=0; i<bytes_read; i+=2)
		{
			cpu_writemem24bew_word(dma_address, (((int) buffer[i]) << 8) | buffer[i+1]);
			dma_address = (dma_address + 2) & 0x1ffffe;
		}

		rec_count -= bytes_read;
		char_count -= bytes_read;
		chunk_len -= bytes_read;

		if (bytes_read != bytes_to_read)
		{	/* eject tape */
			device_filename_change(IO_CASSETTE, t->id, NULL);
			mt3200.w[0] |= w0_offline;
			mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
			goto update_registers;
		}
	}

	if (char_count)
	{
		mt3200.w[0] |= w0_EOR;
		mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
		goto skip_trailer;
	}
	if (rec_count)
	{	/* skip end of record */
		if (osd_fseek(t->fd, rec_count, SEEK_CUR))
		{	/* eject tape */
			device_filename_change(IO_CASSETTE, t->id, NULL);
			mt3200.w[0] |= w0_offline;
			mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
			goto update_registers;
		}
	}

skip_trailer:
	if (osd_fread(t->fd, buffer, 4) != 4)
	{	/* eject tape */
		device_filename_change(IO_CASSETTE, t->id, NULL);
		mt3200.w[0] |= w0_offline;
		mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
		goto update_registers;
	}

	if (reclen != ((((int) buffer[1]) << 8) | buffer[0]))
	{	/* eject tape */
		device_filename_change(IO_CASSETTE, t->id, NULL);
		mt3200.w[0] |= w0_offline;
		mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
		goto update_registers;
	}
	if (buffer[2] || buffer[3])
	{	/* no idea what these bytes mean */
		logerror("Tape format looks gooofy\n");
		/* eject tape to avoid catastrophes */
		device_filename_change(IO_CASSETTE, t->id, NULL);
		mt3200.w[0] |= w0_offline;
		mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
		goto update_registers;
	}

	if (! (mt3200.w[7] & w7_error))
		mt3200.w[7] |= w7_idle | w7_complete;

update_registers:
	mt3200.w[3] = read_offset;
	mt3200.w[4] = char_count;
	mt3200.w[5] = (dma_address >> 1) & 0xffff;
	mt3200.w[6] = (mt3200.w[6] & 0xffe0) | ((dma_address >> 17) & 0xf);
}

static void skip_record(void)
{

}

static void read_transport_status(void)
{
	const tape_unit_t *t = cur_tape_unit();

	if (!t)
	{
		/* No idea what to report... */
		mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
	}
	else if (! t->fd)
	{	/* offline */
		mt3200.w[0] |= w0_offline;
		mt3200.w[7] |= w7_idle | w7_error | w7_tape_error;
	}
#if 0
	else if (0)
	{	/* rewind in progress */
		mt3200.w[0] |= /*...*/;
		mt3200.w[7] |= w7_idle | w7_error | w7_tape_error;
	}
#endif
	else
	{	/* no particular error condition */
		if (t->bot)
			mt3200.w[0] |= w0_BOT;
		if (t->eot)
			mt3200.w[0] |= w0_EOT;
		if (t->wp)
			mt3200.w[0] |= w0_write_ring;
		mt3200.w[7] |= w7_idle | w7_complete;
	}
}

static void execute_command(void)
{
	switch ((mt3200.w[6] & w6_command) >> 8)
	{
	case 0b0000:
	case 0b1100:
	case 0b1110:
		/* NOP */
		logerror("NOP\n");
		mt3200.w[7] |= w7_idle | w7_complete;
		break;
	case 0b0001:
		/* buffer sync: means nothing under emulation */
		logerror("buffer sync\n");
		mt3200.w[7] |= w7_idle | w7_complete;
		break;
	case 0b0010:
		/* write EOF - not emulated */
		logerror("write EOF\n");
		/* ... */
		mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
		break;
	case 0b0011:
		/* record skip reverse - not emulated */
		logerror("record skip reverse\n");
		/* ... */
		mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
		break;
	case 0b0100:
		/* read binary forward */
		logerror("read binary forward\n");
		read_record();
		//mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;	/* WIP */
		break;
	case 0b0101:
		/* record skip forward - not emulated */
		logerror("record skip forward\n");
		/* ... */
		mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
		break;
	case 0b0110:
		/* write binary forward - not emulated */
		logerror("write binary forward\n");
		/* ... */
		mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
		break;
	case 0b0111:
		/* erase - not emulated */
		logerror("erase\n");
		/* ... */
		mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
		break;
	case 0b1000:
	case 0b1001:
		/* read transport status */
		logerror("read transport status\n");
		read_transport_status();
		break;
	case 0b1010:
		/* rewind - not emulated */
		logerror("rewind\n");
		/* ... */
		mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
		break;
	case 0b1011:
		/* rewind and offline - not emulated */
		logerror("rewind and offline\n");
		/* ... */
		mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
		break;
	case 0b1111:
		/* extended control and status - not emulated */
		logerror("extended control and status\n");
		/* ... */
		mt3200.w[7] |= w7_idle | w7_error | w7_hard_error;
		break;
	}
}

READ16_HANDLER(mt3200_r)
{
	if ((offset >= 0) && (offset < 8))
		return mt3200.w[offset];
	else
		return 0;
}

WRITE16_HANDLER(mt3200_w)
{
	if ((offset >= 0) && (offset < 8))
	{
		UINT16 old_data = mt3200.w[offset];

		if (offset == 7)
			data |= w7_PE_format;	/* "This bit is always set for the MT3200" */

		mt3200.w[offset] = data;	/* I bet a few bits cannot be modified like that */

		if ((offset == 7) && (old_data & w7_idle) && ! (data & w7_idle))
		{
			execute_command();
		}
	}
}
