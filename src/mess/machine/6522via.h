/**********************************************************************

	Rockwell 6522 VIA interface and emulation

	This function emulates all the functionality of up to 8 6522
	versatile interface adapters.

    This is based on the M6821 emulation in MAME.

	Written by Mathis Rosenhauer

**********************************************************************/

#ifndef VIA_6522
#define VIA_6522


#define MAX_VIA 8

#define	VIA_PB	    0
#define	VIA_PA	    1
#define	VIA_DDRB    2
#define	VIA_DDRA    3
#define	VIA_T1CL    4
#define	VIA_T1CH    5
#define	VIA_T1LL    6
#define	VIA_T1LH    7
#define	VIA_T2CL    8
#define	VIA_T2CH    9
#define	VIA_SR     10
#define	VIA_ACR    11
#define	VIA_PCR    12
#define	VIA_IFR    13
#define	VIA_IER    14
#define	VIA_PANH   15

struct via6522_interface
{
	int (*in_a_func)(int offset);
	int (*in_b_func)(int offset);
	int (*in_ca1_func)(int offset);
	int (*in_cb1_func)(int offset);
	int (*in_ca2_func)(int offset);
	int (*in_cb2_func)(int offset);
	void (*out_a_func)(int offset, int val);
	void (*out_b_func)(int offset, int val);
	void (*out_ca2_func)(int offset, int val);
	void (*out_cb2_func)(int offset, int val);
	void (*irq_func)(int state);

    /* kludges for the Vectrex */
	void (*out_shift_func)(int val);
	void (*t2_callback)(double time);
};


void via_set_clock(int which,int clock);
void via_config(int which, const struct via6522_interface *intf);
void via_reset(void);
int via_read(int which, int offset);
void via_write(int which, int offset, int data);
void via_set_input_a(int which, int data);
void via_set_input_ca1(int which, int data);
void via_set_input_ca2(int which, int data);
void via_set_input_b(int which, int data);
void via_set_input_cb1(int which, int data);
void via_set_input_cb2(int which, int data);

/******************* Standard 8-bit CPU interfaces, D0-D7 *******************/

int via_0_r(int offset);
int via_1_r(int offset);
int via_2_r(int offset);
int via_3_r(int offset);
int via_4_r(int offset);
int via_5_r(int offset);
int via_6_r(int offset);
int via_7_r(int offset);

void via_0_w(int offset, int data);
void via_1_w(int offset, int data);
void via_2_w(int offset, int data);
void via_3_w(int offset, int data);
void via_4_w(int offset, int data);
void via_5_w(int offset, int data);
void via_6_w(int offset, int data);
void via_7_w(int offset, int data);

/******************* 8-bit A/B port interfaces *******************/

void via_0_porta_w(int offset, int data);
void via_1_porta_w(int offset, int data);
void via_2_porta_w(int offset, int data);
void via_3_porta_w(int offset, int data);
void via_4_porta_w(int offset, int data);
void via_5_porta_w(int offset, int data);
void via_6_porta_w(int offset, int data);
void via_7_porta_w(int offset, int data);

void via_0_portb_w(int offset, int data);
void via_1_portb_w(int offset, int data);
void via_2_portb_w(int offset, int data);
void via_3_portb_w(int offset, int data);
void via_4_portb_w(int offset, int data);
void via_5_portb_w(int offset, int data);
void via_6_portb_w(int offset, int data);
void via_7_portb_w(int offset, int data);

int via_0_porta_r(int offset);
int via_1_porta_r(int offset);
int via_2_porta_r(int offset);
int via_3_porta_r(int offset);
int via_4_porta_r(int offset);
int via_5_porta_r(int offset);
int via_6_porta_r(int offset);
int via_7_porta_r(int offset);

int via_0_portb_r(int offset);
int via_1_portb_r(int offset);
int via_2_portb_r(int offset);
int via_3_portb_r(int offset);
int via_4_portb_r(int offset);
int via_5_portb_r(int offset);
int via_6_portb_r(int offset);
int via_7_portb_r(int offset);

/******************* 1-bit CA1/CA2/CB1/CB2 port interfaces *******************/

void via_0_ca1_w(int offset, int data);
void via_1_ca1_w(int offset, int data);
void via_2_ca1_w(int offset, int data);
void via_3_ca1_w(int offset, int data);
void via_4_ca1_w(int offset, int data);
void via_5_ca1_w(int offset, int data);
void via_6_ca1_w(int offset, int data);
void via_7_ca1_w(int offset, int data);
void via_0_ca2_w(int offset, int data);
void via_1_ca2_w(int offset, int data);
void via_2_ca2_w(int offset, int data);
void via_3_ca2_w(int offset, int data);
void via_4_ca2_w(int offset, int data);
void via_5_ca2_w(int offset, int data);
void via_6_ca2_w(int offset, int data);
void via_7_ca2_w(int offset, int data);

void via_0_cb1_w(int offset, int data);
void via_1_cb1_w(int offset, int data);
void via_2_cb1_w(int offset, int data);
void via_3_cb1_w(int offset, int data);
void via_4_cb1_w(int offset, int data);
void via_5_cb1_w(int offset, int data);
void via_6_cb1_w(int offset, int data);
void via_7_cb1_w(int offset, int data);
void via_0_cb2_w(int offset, int data);
void via_1_cb2_w(int offset, int data);
void via_2_cb2_w(int offset, int data);
void via_3_cb2_w(int offset, int data);
void via_4_cb2_w(int offset, int data);
void via_5_cb2_w(int offset, int data);
void via_6_cb2_w(int offset, int data);
void via_7_cb2_w(int offset, int data);

int via_0_ca1_r(int offset);
int via_1_ca1_r(int offset);
int via_2_ca1_r(int offset);
int via_3_ca1_r(int offset);
int via_4_ca1_r(int offset);
int via_5_ca1_r(int offset);
int via_6_ca1_r(int offset);
int via_7_ca1_r(int offset);
int via_0_ca2_r(int offset);
int via_1_ca2_r(int offset);
int via_2_ca2_r(int offset);
int via_3_ca2_r(int offset);
int via_4_ca2_r(int offset);
int via_5_ca2_r(int offset);
int via_6_ca2_r(int offset);
int via_7_ca2_r(int offset);

int via_0_cb1_r(int offset);
int via_1_cb1_r(int offset);
int via_2_cb1_r(int offset);
int via_3_cb1_r(int offset);
int via_4_cb1_r(int offset);
int via_5_cb1_r(int offset);
int via_6_cb1_r(int offset);
int via_7_cb1_r(int offset);
int via_0_cb2_r(int offset);
int via_1_cb2_r(int offset);
int via_2_cb2_r(int offset);
int via_3_cb2_r(int offset);
int via_4_cb2_r(int offset);
int via_5_cb2_r(int offset);
int via_6_cb2_r(int offset);
int via_7_cb2_r(int offset);


#endif
