/*
	990_hd.c: emulation of a generic ti990 hard disk controller, for use with TILINE-based
	TI990 systems (TI990/10, /12, /12LR, /10A, Business system 300 and 600).

	This core will emulate the common feature set found in every disk controller.
	Most controllers support additional features, but are still compatible with
	the basic feature set.  I have a little documentation documentation on two specific
	disk controllers (MT3200 and WD800/WD800A), but I have not tried to emulate
	controller-specific features.


	Long description: see 2234398-9701 and 2306140-9701.


	Raphael Nabet 2002
*/

#include "990_hd.h"

typedef struct hd_unit_t
{
	void *fd;		/* file descriptor */
	int id;			/* MESS id */
	unsigned int wp : 1;	/* TRUE if tape is write-protected */
} hd_unit_t;

typedef struct hdc_t
{
	UINT16 w[8];

	hd_unit_t d[4];
} hdc_t;

enum
{
	w0_offline			= 0x8000/*,
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
/*	w7_tape_error		= 0x0001*/
};

static hdc_t hdc;


/*
	Image encoding:


	2 bytes: record len - little-endian
	2 bytes: always 0s (length MSBs?)
	len bytes: data
	2 bytes: record len - little-endian
	2 bytes: always 0s (length MSBs?)

	4 0s: EOF mark?
*/


int ti990_hd_init(int id)
{
	hd_unit_t *d;

	if ((id < 0) || (id > 3))
		return INIT_FAIL;

	d = &hdc.d[id];
	memset(d, 0, sizeof(*d));
	d->id = id;

	if (!device_filename(IO_CASSETTE,id))
		return INIT_PASS;

	d->fd = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_RW);
	if (!d->fd)
	{
		d->fd = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_READ);
		if (!d->fd)
			goto error;
		d->wp = 1;
	}

	return INIT_PASS;

error:
	if (d->fd)
		osd_fclose(d->fd);
	return INIT_FAIL;
}

void ti990_hd_exit(int id)
{
	hd_unit_t *d;

	if ((id < 0) || (id > 3))
		return;

	d = &hdc.d[id];

	if (d->fd)
	{
		osd_fclose(d->fd);
		d->fd = NULL;
		d->wp = 0;
	}
}

/*
	Init the hdc core
*/
void ti990_hdc_init(void)
{
	memset(hdc.w, 0, sizeof(hdc.w));
	/*wd900.w[7] = w7_idle | w7_PE_format;*/
}

/*
	Parse command code and execute the command.
*/
#if 0
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
#endif

/*
	Read one register in TPCS space
*/
READ16_HANDLER(ti990_hdc_r)
{
	if ((offset >= 0) && (offset < 8))
		return hdc.w[offset];
	else
		return 0;
}

/*
	Write one register in TPCS space.  Execute command if w7_idle is cleared.
*/
WRITE16_HANDLER(ti990_hdc_w)
{
	if ((offset >= 0) && (offset < 8))
	{
		UINT16 old_data = hdc.w[offset];

		hdc.w[offset] = data;	/* I bet a few bits cannot be modified like that */

		/*if ((offset == 7) && (old_data & w7_idle) && ! (data & w7_idle))
		{
			execute_command();
		}*/
	}
}
