#include "includes/cococart.h"
#include "includes/wd179x.h"
#include "includes/basicdsk.h"
#include "includes/dragon.h"

static const struct cartridge_callback *cartcallbacks;

/***************************************************************************
  Floppy disk controller
 ***************************************************************************
 * The CoCo and Dragon both use the Western Digital 1793 floppy disk
 * controller.  The wd1793's variables are mapped to $ff48-$ff4b on the CoCo
 * and on $ff40-$ff43 on the Dragon.  In addition, there is another register
 * called DSKREG that controls the interface with the wd1793.  DSKREG is
 * detailed below:.  But they appear to be
 *
 * References:
 *		CoCo:	Disk Basic Unravelled
 *		Dragon:	Inferences from the PC-Dragon source code
 *              DragonDos Controller, Disk and File Formats by Graham E Kinns
 *
 * TODO
 *		- Make the wd179x DRQ and IRQ work the way they did in reality.  The
 *		  problem is that the wd179x code doesn't implement any timing, and as
 *		  such behaves differently then the real thing.
 *
 * ---------------------------------------------------------------------------
 * DSKREG - the control register
 * CoCo ($ff40)                            Dragon ($ff48)
 *
 * Bit                                     Bit
 *	7 halt enable flag                      7 not used
 *	6 drive select #3                       6 not used
 *	5 density flag (0=single, 1=double)     5 NMI enable flag
 *	4 write precompensation                 4 write precompensation
 *	3 drive motor activation                3 single density enable
 *	2 drive select #2                       2 drive motor activation
 *	1 drive select #1                       1 drive select high bit
 *	0 drive select #0                       0 drive select low bit
 * ---------------------------------------------------------------------------
 */

static int haltenable;
static int nmienable;
static int dskreg;
static void coco_fdc_callback(int event);
static void dragon_fdc_callback(int event);
static int ff4b_count;

enum {
	HW_COCO,
	HW_DRAGON
};

static void coco_fdc_init(const struct cartridge_callback *callbacks)
{
    wd179x_init(coco_fdc_callback);
	dskreg = -1;
	ff4b_count = 0x100;
	nmienable = 1;
	cartcallbacks = callbacks;
}

static void dragon_fdc_init(const struct cartridge_callback *callbacks)
{
    wd179x_init(dragon_fdc_callback);
	dskreg = -1;
	ff4b_count = 0x100;
	nmienable = 1;
	cartcallbacks = callbacks;
}

static void raise_nmi(int dummy)
{
#if LOG_FLOPPY
	logerror("raise_nmi(): Raising NMI from floppy controller\n");
#endif
	cpu_set_nmi_line(0, ASSERT_LINE);
}

static void coco_fdc_callback(int event)
{
	/* In all honesty, I believe that I should be able to tie the WD179X IRQ
	 * directly to the 6809 NMI input.  But it seems that if I do that, the NMI
	 * occurs when the last byte of a read is made without any delay.  This
	 * means that we drop the last byte of every sector read or written.  Thus,
	 * we will delay the NMI
	 */
	switch(event) {
	case WD179X_IRQ_CLR:
		cpu_set_nmi_line(0, CLEAR_LINE);
		break;
	case WD179X_IRQ_SET:
#if LOG_FLOPPY
		logerror("coco_fdc_callback(): Called with WD179X_IRQ_SET; but not raising NMI because of hack (ff4b_count=$%04x)\n", ff4b_count);
#endif
		/* timer_set(COCO_CPU_SPEED * 11 / timer_get_overclock(0), 0, raise_nmi); */
		break;
	case WD179X_DRQ_CLR:
		cpu_set_halt_line(0, CLEAR_LINE);
		break;
	case WD179X_DRQ_SET:
		/* I should be able to specify haltenable instead of zero, but several
		 * programs don't appear to work
		 */
		cpu_set_halt_line(0, 0 /*haltenable*/ ? ASSERT_LINE : CLEAR_LINE);
		break;
	}
}

static void dragon_fdc_callback(int event)
{
	/* In all honesty, I believe that I should be able to tie the WD179X IRQ
	 * directly to the 6809 NMI input.  But it seems that if I do that, the NMI
	 * occurs when the last byte of a read is made without any delay.  This
	 * means that we drop the last byte of every sector read or written.  Thus,
	 * we will delay the NMI
	 */
	switch(event) {
	case WD179X_IRQ_CLR:
		cpu_set_nmi_line(0, CLEAR_LINE);
		break;
	case WD179X_IRQ_SET:
#if LOG_FLOPPY
		logerror("dragon_fdc_callback(): Called with WD179X_IRQ_SET; but not raising NMI because of hack (ff4b_count=$%04x)\n", ff4b_count);
#endif
		if (nmienable)
			timer_set(COCO_CPU_SPEED * 11 / timer_get_overclock(0), 0, raise_nmi);
		break;
	case WD179X_DRQ_CLR:
		cartcallbacks->setcartline(CARTLINE_CLEAR);
		break;
	case WD179X_DRQ_SET:
		cartcallbacks->setcartline(CARTLINE_ASSERTED);
		break;
	}
}

int dragon_floppy_init(int id)
{
	void *file;
	int tracks;
	int heads;

	if (basicdsk_floppy_init(id)==INIT_PASS) {
		file = image_fopen(IO_FLOPPY, id, OSD_FILETYPE_IMAGE_R, OSD_FOPEN_READ);
		if (!file)
			return INIT_FAIL;

		tracks = osd_fsize(file) / (18*256);
		heads = (tracks > 80) ? 2 : 1;
		tracks /= heads;

		basicdsk_set_geometry(id, tracks, heads, 18, 256, 1);

		osd_fclose(file);
	}
	return INIT_PASS;
}


static void set_dskreg(int data, int hardware)
{
	UINT8 drive = 0;
	UINT8 head = 0;
	int motor_mask = 0;
	int haltenable_mask = 0;

#if LOG_FLOPPY
	logerror("set_dskreg(): data=$%02x\n", data);
#endif

	switch(hardware) {
	case HW_COCO:
		if ((dskreg & 0x1cf) == (data & 0xcf))
			return;

		/* An email from John Kowalski informed me that if the last drive is
		 * selected, and one of the other drive bits is selected, then the
		 * second side is selected.  If multiple bits are selected in other
		 * situations, then both drives are selected, and any read signals get
		 * yucky.
		 */
		motor_mask = 0x08;
		haltenable_mask = 0x80;

		if (data & 0x04)
			drive = 2;
		else if (data & 0x02)
			drive = 1;
		else if (data & 0x01)
			drive = 0;
		else if (data & 0x40)
			drive = 3;
		else
			motor_mask = 0;

		head = ((data & 0x40) && (drive != 3)) ? 1 : 0;
		break;

	case HW_DRAGON:
		if ((dskreg & 0x127) == (data & 0x27))
			return;
		drive = data & 0x03;
		motor_mask = 0x04;
		haltenable_mask = 0x00;
		nmienable = data & 0x20;
		break;
	}

	haltenable = data & haltenable_mask;
	dskreg = data;

	if (data & motor_mask) {
		wd179x_set_drive(drive);
		wd179x_set_side(head);
	}
}

static int dc_floppy_r(int offset)
{
	int result = 0;

	switch(offset & 0xef) {
	case 8:
		result = wd179x_status_r(0);
		break;
	case 9:
		result = wd179x_track_r(0);
		break;
	case 10:
		result = wd179x_sector_r(0);
		break;
	case 11:
		if (ff4b_count-- == 0)
			raise_nmi(0);
		result = wd179x_data_r(0);
		break;
	}
	return result;
}

static void dc_floppy_w(int offset, int data, int hardware)
{
	switch(offset & 0xef) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		set_dskreg(data, hardware);
		break;
	case 8:
		wd179x_command_w(0, data);
		ff4b_count = 0x100;
		break;
	case 9:
		wd179x_track_w(0, data);
		break;
	case 10:
		wd179x_sector_w(0, data);
		break;
	case 11:
		if (ff4b_count-- == 0)
			raise_nmi(0);
		else
			wd179x_data_w(0, data);
		break;
	};
}

/* ---------------------------------------------------- */

READ_HANDLER(coco_floppy_r)
{
	return dc_floppy_r(offset);
}

WRITE_HANDLER(coco_floppy_w)
{
	dc_floppy_w(offset, data, HW_COCO);
}

READ_HANDLER(dragon_floppy_r)
{
	return dc_floppy_r(offset ^ 8);
}

WRITE_HANDLER(dragon_floppy_w)
{
	dc_floppy_w(offset ^ 8, data, HW_DRAGON);
}

/* ---------------------------------------------------- */

const struct cartridge_slot cartridge_fdc_coco =
{
	coco_fdc_init,
	wd179x_exit,
	coco_floppy_r,
	coco_floppy_w,
	NULL
};

const struct cartridge_slot cartridge_fdc_dragon =
{
	dragon_fdc_init,
	wd179x_exit,
	dragon_floppy_r,
	dragon_floppy_w,
	NULL
};

/* ---------------------------------------------------- */

static void cartidge_standard_init(const struct cartridge_callback *callbacks)
{
	cartcallbacks = callbacks;
	cartcallbacks->setcartline(CARTLINE_Q);
}

static WRITE_HANDLER(cartridge_twobanks_io_w)
{
	cartcallbacks->setbank(data & 1);
}

const struct cartridge_slot cartridge_standard =
{
	cartidge_standard_init,
	NULL,
	NULL,
	NULL,
	NULL
};

const struct cartridge_slot cartridge_twobanks =
{
	cartidge_standard_init,
	NULL,
	NULL,
	cartridge_twobanks_io_w,
	NULL
};

/***************************************************************************
  Other hardware to do
****************************************************************************

  IDE - There is an IDE ehancement for the CoCo 3.  Here are some docs on the
  interface (info courtesy: LCB):

		The IDE interface (which is jumpered to be at either at $ff5x or $ff7x
		is mapped thusly:

		$FFx0 - 1st 8 bits of 16 bit IDE DATA register
		$FFx1 - Error (read) / Features (write) register
		$FFx2 - Sector count register
		$FFx3 - Sector # register
		$FFx4 - Low byte of cylinder #
		$FFx5 - Hi byte of cylinder #
		$FFx6 - Device/head register
		$FFx7 - Status (read) / Command (write) register
		$FFx8 - Latch containing the 2nd 8 bits of the 16 bit IDE DATA reg.

		All of these directly map to the IDE standard equivalents. No drivers
		use	the IRQ's... they all use Polled I/O.

  ORCH-90 STEREO SOUND PACK - This is simply 2 8 bit DACs, with the left and
  right channels at $ff7a & $ff7b respectively (info courtesy: LCB).

***************************************************************************/
