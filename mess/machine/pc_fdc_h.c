/* PC floppy interface has been extracted so it can be used in the super i/o chip */

/* TODO:
	- check how the drive select from DOR register, and the drive select
	from the fdc are related !!!! 
	- if all drives do not have a disk in them, and the fdc is reset, is a int generated?
	(if yes, indicates drives are ready without discs, if no indicates no drives are ready)
	- status register a, status register b
*/

#include "includes/pc_fdc_h.h"
#include "includes/nec765.h"

static pc_fdc fdc;

static void pc_fdc_hw_interrupt(int state);
static void pc_fdc_hw_dma_drq(int,int);

static nec765_interface pc_fdc_nec765_interface = 
{
	pc_fdc_hw_interrupt,
	pc_fdc_hw_dma_drq
};

static void pc_fdc_reset(void)
{
	/* setup reset condition */
	fdc.data_rate_register = 2;
	fdc.digital_output_register = 0;

	/* bit 7 is disk change */
	fdc.digital_input_register = 0x07f;

	nec765_reset(0);

	/* set FDC at reset */
	nec765_set_reset_state(1);
}

void pc_fdc_init(pc_fdc_hw_interface *iface)
{
	int i;
	mess_image *img;

	/* clear out globals */
	memset(&fdc, 0, sizeof(fdc));

	/* copy specified interface */
	memcpy(&fdc.fdc_interface, iface, sizeof(pc_fdc_hw_interface));

	/* setup nec765 interface */
	nec765_init(&pc_fdc_nec765_interface, NEC765A);

	pc_fdc_reset();

	for (i = 0; i < device_count(IO_FLOPPY); i++)
	{
		img = image_from_devtype_and_index(IO_FLOPPY, i);
		floppy_drive_set_geometry(img, FLOPPY_DRIVE_DS_80);
	}
}

void	pc_fdc_set_tc_state(int state)
{
	/* store state */
	fdc.tc_state = state;

	/* if dma is not enabled, tc's are not acknowledged */
	if ((fdc.digital_output_register & PC_FDC_FLAGS_DOR_DMA_ENABLED)!=0)
	{
		nec765_set_tc_state(state);
	}
}


void	pc_fdc_hw_interrupt(int state)
{
	fdc.int_state = state;

	/* if dma is not enabled, irq's are masked */
	if ((fdc.digital_output_register & PC_FDC_FLAGS_DOR_DMA_ENABLED)==0)
		return;

	/* send irq */
	if (fdc.fdc_interface.pc_fdc_interrupt!=NULL)
		fdc.fdc_interface.pc_fdc_interrupt(state);
}

int	pc_fdc_dack_r(void)
{
	int data;

	/* what is output if dack is not acknowledged? */
	data = 0x0ff;

	/* if dma is not enabled, dacks are not acknowledged */
	if ((fdc.digital_output_register & PC_FDC_FLAGS_DOR_DMA_ENABLED)!=0)
	{
		data = nec765_dack_r(0);
	}

	return data;
}

void	pc_fdc_dack_w(int data)
{
	/* if dma is not enabled, dacks are not issued */
	if ((fdc.digital_output_register & PC_FDC_FLAGS_DOR_DMA_ENABLED)!=0)
	{
		/* dma acknowledge - and send byte to fdc */
		nec765_dack_w(0,data);
	}
}

void pc_fdc_hw_dma_drq(int state, int read_)
{
	fdc.dma_state = state;
	fdc.dma_read = read_;

	/* if dma is not enabled, drqs are masked */
	if ((fdc.digital_output_register & PC_FDC_FLAGS_DOR_DMA_ENABLED)==0)
		return;

	if (fdc.fdc_interface.pc_fdc_dma_drq)
		fdc.fdc_interface.pc_fdc_dma_drq(state, read_);
}

static void pc_fdc_data_rate_w(data8_t data)
{
	if ((data & 0x080)!=0)
	{
		/* set ready state */
		nec765_set_ready_state(1);

		/* toggle reset state */
		nec765_set_reset_state(1);
 		nec765_set_reset_state(0);
	
		/* bit is self-clearing */
		data &= ~0x080;
	}

	fdc.data_rate_register = data;
}

static void pc_fdc_dor_w(data8_t data)
{
	int selected_drive;
	int floppy_count;

	logerror("FDC DOR: %02x\r\n",data);
	floppy_count = device_count(IO_FLOPPY);

	if (floppy_count > (fdc.digital_output_register & 0x03))
		floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, fdc.digital_output_register & 0x03), 1, 0);

	fdc.digital_output_register = data;

	selected_drive = data & 0x03;

	/* set floppy drive motor state */
	if (floppy_count > 0)
		floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 0),	(data>>4) & 0x0f);
	if (floppy_count > 1)
		floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 1),	(data>>5) & 0x01);
	if (floppy_count > 2)
		floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 2),	(data>>6) & 0x01);
	if (floppy_count > 3)
		floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 3),	(data>>7) & 0x01);

	if ((data>>4) & (1<<selected_drive))
	{
		if (floppy_count > selected_drive)
			floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, selected_drive), 1, 0);
	}

	/* changing the DMA enable bit, will affect the terminal count state
	from reaching the fdc - if dma is enabled this will send it through
	otherwise it will be ignored */
	pc_fdc_set_tc_state(fdc.tc_state);

	/* changing the DMA enable bit, will affect the dma drq state
	from reaching us - if dma is enabled this will send it through
	otherwise it will be ignored */
	pc_fdc_hw_dma_drq(fdc.dma_state, fdc.dma_read);

	/* changing the DMA enable bit, will affect the irq state
	from reaching us - if dma is enabled this will send it through
	otherwise it will be ignored */
	pc_fdc_hw_interrupt(fdc.int_state);

	/* reset? */
	if ((fdc.digital_output_register & PC_FDC_FLAGS_DOR_FDC_ENABLED)==0)
	{
		/* yes */

			/* pc-xt expects a interrupt to be generated
			when the fdc is reset.
			In the FDC docs, it states that a INT will
			be generated if READY input is true when the
			fdc is reset. 
			
				It also states, that outputs to drive are set to 0.
				Maybe this causes the drive motor to go on, and therefore
				the ready line is set. 

			This in return causes a int?? ---
		
		
		what is not yet clear is if this is a result of the drives ready state
		changing...
		*/
			nec765_set_ready_state(1);

		/* set FDC at reset */
		nec765_set_reset_state(1);
	}
	else
	{
		pc_fdc_set_tc_state(0);

		/* release reset on fdc */
		nec765_set_reset_state(0);
	}
}

WRITE_HANDLER ( pc_fdc_w )
{
	switch(offset) {
	case 0:	/* n/a */
	case 1:	/* n/a */
		break;
	case 2:
		pc_fdc_dor_w(data);
		break;
	case 3:
		/* tape drive select? */
		break;
	case 4:
		pc_fdc_data_rate_w(data);
		break;
	case 5:
		nec765_data_w(0, data);
		break;
	case 6: /* fdc reserved */
	case 7: /* n/a */
		break;
	}
}

READ_HANDLER ( pc_fdc_r )
{
	data8_t data = 0xff;

	switch(offset) {
	case 0: /* status register a */
	case 1: /* status register b */
		data = 0x00;
	case 2:
		data = fdc.digital_output_register;
		break;
	case 3: /* tape drive select? */
		break;
	case 4:
		data = nec765_status_r(0);
		break;
	case 5:
		data = nec765_data_r(offset);
		break;
	case 6: /* FDC reserved */
		break;
	case 7:
		data = fdc.digital_input_register;
		break;
    }
	return data;
}

