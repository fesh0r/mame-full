/***************************************************************************

	machine/pc_fdc.c

	PC-DMA glue to link with pc fdc hardware
***************************************************************************/
#include "includes/pic8259.h"
#include "includes/dma8237.h"
#include "includes/pc.h"
#include "includes/pc_fdc_h.h"

#define FDC_DMA 	2			   /* DMA channel number for the FDC */
static int pc_dma_started = 0;

static void FDC_irq(int num)
{
//	logerror("FDC irq taken\r\n");

	pic8259_0_issue_irq(num);
}

static void pc_fdc_dma_drq(int,int);
static void pc_fdc_interrupt(int);

static pc_fdc_hw_interface pc_fdc_interface=
{
	pc_fdc_interrupt,
	pc_fdc_dma_drq,
};

static void pc_fdc_interrupt(int state)
{
	if (state)
	{
//		/* bodge for code below. */
//		/* if dma was running and int was issued, shut off dma */
//		if (pc_DMA_status & (0x010<<FDC_DMA))
//		{
//			pc_DMA_status &= ~(0x010<<FDC_DMA);
//			pc_DMA_status |= 0x01 << FDC_DMA;		/* set DMA terminal count flag */
//		}
		
		/* issue IRQ */
		FDC_irq(6);
	}
}

void	pc_fdc_dma_drq(int state, int read)
{
	if (state)
	{
		if (dma8237->mask & (0x10 << FDC_DMA)) {
			FDC_LOG(1,"FDC_DMA_write",("DMA %d is masked\n", FDC_DMA));
			return;
		}

		if (!pc_dma_started)
		{
			/* DMA is blocking, for now there is no way to halt the CPU and let
			everything else continue. So this is a bodge*/
			pc_dma_started = 1;

			logerror("DMA Bytes To Transfer: %d\r\n", dma8237->chan[FDC_DMA].count+1);

			if (read)
			{
				dma8237->status |= 0x010<<FDC_DMA;

				/* running? */
				while ((dma8237->status & (0x010<<FDC_DMA))!=0)
				{
					/* dma acknowledge - and get byte from fdc */
					dma8237_read(dma8237, FDC_DMA, pc_fdc_dack_r() );
				}

				/* it says the TC is pulsed.. I have no furthur details */
				pc_fdc_set_tc_state(1);
				pc_fdc_set_tc_state(0);

				pc_dma_started = 0;
			}
			else
			{
				dma8237->status |= 0x010<<FDC_DMA;

				/* running? */
				while ((dma8237->status & (0x010<<FDC_DMA))!=0)
				{
					pc_fdc_dack_w(dma8237_write(dma8237, FDC_DMA));
				}

				pc_fdc_set_tc_state(1);
				pc_fdc_set_tc_state(0);

				pc_dma_started = 0;
			}

			logerror("DMA Bytes Remaining: %d\r\n", dma8237->chan[FDC_DMA].count+1);

		}
	}
}

void	pc_fdc_setup(void)
{
	pc_dma_started = 0;
	pc_fdc_init(&pc_fdc_interface);
}
