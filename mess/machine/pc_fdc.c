/***************************************************************************

	machine/pc_fdc.c

	PC-DMA glue to link with pc fdc hardware
***************************************************************************/

#include "includes/pic8259.h"
#include "includes/pc.h"
#include "includes/pc_fdc_h.h"

#include "machine/8237dma.h"

#define VERBOSE_FDC 0		/* FDC (floppy disk controller) */
#if VERBOSE_FDC
#define FDC_LOG(N,M,A)
	if(VERBOSE_FDC>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define FDC_LOG(n,m,a)
#endif

#define FDC_DMA 	2			   /* DMA channel number for the FDC */

static void FDC_irq(int num)
{
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
		/* issue IRQ */
		FDC_irq(6);
	}
}



void pc_fdc_dma_drq(int state, int read_)
{
	dma8237_drq_write(0, FDC_DMA, state);
}



void pc_fdc_setup(void)
{
	pc_fdc_init(&pc_fdc_interface);
}
