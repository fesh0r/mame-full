#include "driver.h"
#include "mess/includes/pclpt.h"

#define LOG(LEVEL,N,M,A)  \
if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; 


//#define VERBOSE_LPT

#ifdef VERBOSE_LPT
#define LPT_LOG(n,m,a) LOG(VERBOSE_LPT,n,m,a)
#else
#define LPT_LOG(n,m,a)
#endif


/*************************************************************************
 *
 *		LPT
 *		line printer
 *
 *************************************************************************/
static UINT8 LPT_data[3] = {0, };
static UINT8 LPT_status[3] = {0, };
static UINT8 LPT_control[3] = {0, };

static void pc_LPT_w(int n, int offset, int data)
{
	if ( !(input_port_2_r(0) & (0x08>>n)) ) return;
	switch( offset )
	{
		case 0:
			LPT_data[n] = data;
			LPT_LOG(1,"LPT_data_w",("LPT%d $%02x\n", n, data));
			break;
		case 1:
			LPT_LOG(1,"LPT_status_w",("LPT%d $%02x\n", n, data));
			break;
		case 2:
			LPT_control[n] = data;
			LPT_LOG(1,"LPT_control_w",("%d $%02x\n", n, data));
			break;
    }
}

WRITE_HANDLER ( pc_LPT1_w ) { pc_LPT_w(0,offset,data); }
WRITE_HANDLER ( pc_LPT2_w ) { pc_LPT_w(1,offset,data); }
WRITE_HANDLER ( pc_LPT3_w ) { pc_LPT_w(2,offset,data); }

static int pc_LPT_r(int n, int offset)
{
    int data = 0xff;
	if ( !(input_port_2_r(0) & (0x08>>n)) ) return data;
	switch( offset )
	{
		case 0:
			data = LPT_data[n];
			LPT_LOG(1,"LPT_data_r",("LPT%d $%02x\n", n, data));
			break;
		case 1:
			/* set status 'out of paper', '*no* error', 'IRQ has *not* occured' */
			LPT_status[n] = 0x09c;	//0x2c;
			
			LPT_status[n] &= ~(1<<4);
			if (LPT_control[n] & (1<<3))
			{
				LPT_status[n] |= (1<<4);
			}

			data = LPT_status[n];
			LPT_LOG(1,"LPT_status_r",("%d $%02x\n", n, data));
			break;
		case 2:
			LPT_control[n] = 0x0c;
			data = LPT_control[n];
			LPT_LOG(1,"LPT_control_r",("%d $%02x\n", n, data));
			break;
    }
	return data;
}
READ_HANDLER ( pc_LPT1_r) { return pc_LPT_r(0, offset); }
READ_HANDLER ( pc_LPT2_r) { return pc_LPT_r(1, offset); }
READ_HANDLER ( pc_LPT3_r) { return pc_LPT_r(2, offset); }
