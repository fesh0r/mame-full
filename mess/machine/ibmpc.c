#include "driver.h"
#include "gregoria.h"
#include "includes/pc.h"
#include "includes/ibmpc.h"

#define VERBOSE_JOY 0		/* JOY (joystick port) */

#define VERBOSE_PIO 0		/* PIO (keyboard controller) */

#define VERBOSE_DBG 0       /* general debug messages */

#if VERBOSE_JOY
#define LOG(LEVEL,N,M,A)  \
#define JOY_LOG(N,M,A) \
	if(VERBOSE_JOY>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define JOY_LOG(n,m,a)
#endif

#if VERBOSE_DBG
#define DBG_LOG(N,M,A) \
	if(VERBOSE_DBG>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define DBG_LOG(n,m,a)
#endif

#if VERBOSE_PIO
#define PIO_LOG(N,M,N) \
	if(VERBOSE_PIO>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define PIO_LOG(n,m,a)
#endif


/*************************************************************************
 *
 *		PIO
 *		parallel input output
 *
 *************************************************************************/
static int pc_ppi_porta_r(int chip );
static int pc_ppi_portb_r(int chip );
static int pc_ppi_portc_r(int chip);
static void pc_ppi_porta_w(int chip, int data );
static void pc_ppi_portb_w( int chip, int data );
static void pc_ppi_portc_w(int chip, int data );


/* PC-XT has a 8255 which is connected to keyboard and other
status information */
ppi8255_interface pc_ppi8255_interface =
{
	1,
	pc_ppi_porta_r,
	pc_ppi_portb_r,
	pc_ppi_portc_r,
	pc_ppi_porta_w,
	pc_ppi_portb_w,
	pc_ppi_portc_w
};


static struct {
	int portc_switch_high;
	int speaker;
} pc_ppi={ 0 };

int pc_ppi_porta_r(int chip )
{
	int data;

	/* KB port A */
	data = pc_keyb_read();
    PIO_LOG(1,"PIO_A_r",("$%02x\n", data));
    return data;
}

int pc_ppi_portb_r(int chip )
{
	int data;

	data = 0xff;
	PIO_LOG(1,"PIO_B_r",("$%02x\n", data));
	return data;
}

int pc_ppi_portc_r( int chip )
{
	int data=0xff;

	data&=~0x80; // no parity error
	data&=~0x40; // no error on expansion board
	/* KB port C: equipment flags */
//	if (pc_port[0x61] & 0x08)
	if (pc_ppi.portc_switch_high)
	{
		/* read hi nibble of S2 */
		data = (data&0xf0)|((input_port_1_r(0) >> 4) & 0x0f);
		PIO_LOG(1,"PIO_C_r (hi)",("$%02x\n", data));
	}
	else
	{
		/* read lo nibble of S2 */
		data = (data&0xf0)|(input_port_1_r(0) & 0x0f);
		PIO_LOG(1,"PIO_C_r (lo)",("$%02x\n", data));
	}

	return data;
}

void pc_ppi_porta_w(int chip, int data )
{
	/* KB controller port A */
	PIO_LOG(1,"PIO_A_w",("$%02x\n", data));
}

void pc_ppi_portb_w(int chip, int data )
{
	/* KB controller port B */
	PIO_LOG(1,"PIO_B_w",("$%02x\n", data));
	pc_ppi.portc_switch_high=data&0x8;
	pc_sh_speaker(data&3);
	pc_keyb_set_clock(data&0x40);
}

void pc_ppi_portc_w(int chip, int data )
{
	/* KB controller port C */
	PIO_LOG(1,"PIO_C_w",("$%02x\n", data));
}

/*************************************************************************
 *
 *		JOY
 *		joystick port
 *
 *************************************************************************/

static double JOY_time = 0.0;

WRITE_HANDLER ( pc_JOY_w )
{
	JOY_time = timer_get_time();
}

#if 0
#define JOY_VALUE_TO_TIME(v) (24.2e-6+11e-9*(100000.0/256)*v)
READ_HANDLER ( pc_JOY_r )
{
	int data, delta;
	double new_time = timer_get_time();

	data=input_port_15_r(0)^0xf0;
#if 0
    /* timer overflow? */
	if (new_time - JOY_time > 0.01)
	{
		//data &= ~0x0f;
		JOY_LOG(2,"JOY_r",("$%02x, time > 0.01s\n", data));
	}
	else
#endif
	{
		delta=new_time-JOY_time;
		if ( delta>JOY_VALUE_TO_TIME(input_port_16_r(0)) ) data &= ~0x01;
		if ( delta>JOY_VALUE_TO_TIME(input_port_17_r(0)) ) data &= ~0x02;
		if ( delta>JOY_VALUE_TO_TIME(input_port_18_r(0)) ) data &= ~0x04;
		if ( delta>JOY_VALUE_TO_TIME(input_port_19_r(0)) ) data &= ~0x08;
		JOY_LOG(1,"JOY_r",("$%02x: X:%d, Y:%d, time %8.5f, delta %d\n", data, input_port_16_r(0), input_port_17_r(0), new_time - JOY_time, delta));
	}

	return data;
}
#else
READ_HANDLER ( pc_JOY_r )
{
	int data, delta;
	double new_time = timer_get_time();

	data=input_port_15_r(0)^0xf0;
    /* timer overflow? */
	if (new_time - JOY_time > 0.01)
	{
		//data &= ~0x0f;
		JOY_LOG(2,"JOY_r",("$%02x, time > 0.01s\n", data));
	}
	else
	{
		delta = (int)( 256 * 1000 * (new_time - JOY_time) );
		if (input_port_16_r(0) < delta) data &= ~0x01;
		if (input_port_17_r(0) < delta) data &= ~0x02;
		if (input_port_18_r(0) < delta) data &= ~0x04;
		if (input_port_19_r(0) < delta) data &= ~0x08;
		JOY_LOG(1,"JOY_r",("$%02x: X:%d, Y:%d, time %8.5f, delta %d\n", data, input
						   _port_16_r(0), input_port_17_r(0), new_time - JOY_time, delta));
	}

	return data;
}
#endif


// damned old checkit doesn't test at standard adresses
// will do more when I have a program supporting it
static struct {
	int data[0x18];
	void *timer;
} pc_rtc;

static void pc_rtc_timer(int param)
{
	int year;
	if (++pc_rtc.data[2]>=60) {
		pc_rtc.data[2]=0;
		if (++pc_rtc.data[3]>=60) {
			pc_rtc.data[3]=0;
			if (++pc_rtc.data[4]>=24) {
				pc_rtc.data[4]=0;
				pc_rtc.data[5]=(pc_rtc.data[5]%7)+1;
				year=pc_rtc.data[9]+2000;
				if (++pc_rtc.data[6]>=gregorian_days_in_month(pc_rtc.data[7], year)) {
					pc_rtc.data[6]=1;
					if (++pc_rtc.data[7]>12) {
						pc_rtc.data[7]=1;
						pc_rtc.data[9]=(pc_rtc.data[9]+1)%100;
					}
				}
			}
		}
	}
}

void pc_rtc_init(void)
{
	memset(&pc_rtc,0,sizeof(pc_rtc));
	pc_rtc.timer=timer_pulse(1.0,0,pc_rtc_timer);
}

READ_HANDLER( pc_rtc_r )
{
	int data;
	switch (offset) {
	default:
		data=pc_rtc.data[offset];
	}
	logerror( "rtc read %.2x %.2x\n", offset, data);
	return data;
}

WRITE_HANDLER( pc_rtc_w )
{
	logerror( "rtc write %.2x %.2x\n", offset, data);
	switch(offset) {
	default:
		pc_rtc.data[offset]=data;
	}
}

/*************************************************************************
 *
 *		EXP
 *		expansion port
 *
 *************************************************************************/

// I even don't know what it is!
static struct {
	/*
	  reg 0 ram behaviour if in
	  reg 3 write 1 to enable it
	  reg 4 ram behaviour ???
	  reg 5,6 (5 hi, 6 lowbyte) ???
	*/
	/* selftest in ibmpc, ibmxt */
	UINT8 reg[8];
} pc_expansion={ { 0,0,0,0,0,0,1 } };

WRITE_HANDLER ( pc_EXP_w )
{
	DBG_LOG(1,"EXP_unit_w",("%.2x $%02x\n", offset, data));
	switch (offset) {
	case 4:
		pc_expansion.reg[4]=pc_expansion.reg[5]=pc_expansion.reg[6]=data;
		break;
	default:
		pc_expansion.reg[offset] = data;
	}
}

READ_HANDLER ( pc_EXP_r )
{
    int data;
	UINT16 a;

	switch (offset) {
	case 6: 
		data = pc_expansion.reg[offset];
		a=(pc_expansion.reg[5]<<8)|pc_expansion.reg[6];
		a<<=1;
		pc_expansion.reg[5]=a>>8;
		pc_expansion.reg[6]=a&0xff;
		break;
	default:
		data = pc_expansion.reg[offset];
	}
    DBG_LOG(1,"EXP_unit_r",("%.2x $%02x\n", offset, data));
	return data;
}

