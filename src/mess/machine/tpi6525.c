/***************************************************************************
    mos tri port interface 6525
	mos triple interface adapter 6523

    peter.trauner@jk.uni-linz.ac.at

	used in commodore b series
	used in commodore c1551 floppy disk drive
***************************************************************************/

/*
 mos tpi 6525
 40 pin package
 3 8 bit ports (pa, pb, pc)
 8 registers to pc
 0 port a 0 in low
 1 port a data direction register 1 output
 2 port b
 3 port b ddr
 4 port c
  handshaking, interrupt mode
  0 interrupt 0 input, 1 interrupt enabled
   interrupt set on falling edge
  1 interrupt 1 input
  2 interrupt 2 input
  3 interrupt 3 input
  4 interrupt 4 input
  5 irq output
  6 ca handshake line (read handshake answer on I3 prefered)
  7 cb handshake line (write handshake clear on I4 prefered)
 5 port c ddr

 6 cr configuration register
  0 mc
    0 port c normal input output mode like port a and b
    1 port c used for handshaking and interrupt input
  1 1 interrupt priorized
  2 i3 configure edge
    1 interrupt set on positive edge
  3 i4 configure edge
  5,4 ca handshake
   00 on read
      rising edge of i3 sets ca high
      read a data from computers sets ca low
   01 pulse output
      1 microsecond low after read a operation
   10 manual output low
   11 manual output high
  7,6 cb handshake
   00 handshake on write
      write b data from computer sets cb 0
      rising edge of i4 sets cb high
   01 pulse output
      1 microsecond low after write b operation
   10 manual output low
   11 manual output high
 7 air active interrupt register
   0 I0 occured
   1 I1 occured
   2 I2 occured
   3 I3 occured
   4 I4 occured
   read clears interrupt

 non priorized interrupts
  interrupt is set when occured
  read clears all interrupts

 priorized interrupts
  I4>I3>I2>I1>I0
  highest interrupt can be found in air register
  read clears highest interrupt
*/
#include <ctype.h>
#include "driver.h"

#define VERBOSE_DBG 1
#include "cbm.h"

#include "tpi6525.h"

TPI6525 tpi6525[4]={
	{ 0 },
	{ 1 },
	{ 2 },
	{ 3 }
};

#define INTERRUPT_MODE (this->cr&1)
#define PRIORIZED_INTERRUPTS (this->cr&2)
#define INTERRUPT3_RISING_EDGE (this->cr&4)
#define INTERRUPT4_RISING_EDGE (this->cr&8)
#define CA_MANUAL_OUT (this->cr&0x20)
#define CA_MANUAL_LEVEL ((this->cr&0x10)?1:0)
#define CB_MANUAL_OUT (this->cr&0x80)
#define CB_MANUAL_LEVEL ((this->cr&0x40)?1:0)

static void tpi6525_reset(TPI6525 *this)
{
	this->cr=0;this->air=0;
	this->a.ddr=0;this->a.port=0;
	this->b.ddr=0;this->b.port=0;
	this->c.ddr=0;this->c.port=0;
	this->a.in=0xff;
	this->b.in=0xff;
	this->c.in=0xff;
	this->interrupt.level=
		this->irq_level[0]=
		this->irq_level[1]=
		this->irq_level[2]=
		this->irq_level[3]=
		this->irq_level[4]=0;
}

static void tpi6525_set_interrupt(TPI6525 *this)
{
	if (!this->interrupt.level && (this->air!=0)) {
		this->interrupt.level=1;
		DBG_LOG (3, "tpi6525",(errorlog, "%d set interrupt\n",this->number));
		if (this->interrupt.output!=NULL)
			this->interrupt.output(this->interrupt.level);
	}
}

static void tpi6525_clear_interrupt(TPI6525 *this)
{
	if (this->interrupt.level && (this->air==0)) {
		this->interrupt.level=0;
		DBG_LOG (3, "tpi6525",(errorlog, "%d clear interrupt\n",this->number));
		if (this->interrupt.output!=NULL)
			this->interrupt.output(this->interrupt.level);
	}
}

static void tpi6525_irq0_level(TPI6525 *this, int level)
{
	if (INTERRUPT_MODE && (level!=this->irq_level[0]) ) {
		this->irq_level[0]=level;
		if ((level==0)&&!(this->air&1)&&(this->c.ddr&1)) {
			this->air|=1;
			tpi6525_set_interrupt(this);
		}
	}
}

static void tpi6525_irq1_level(TPI6525 *this, int level)
{
	if (INTERRUPT_MODE && (level!=this->irq_level[1]) ) {
		this->irq_level[1]=level;
		if ((level==0)&&!(this->air&2)&&(this->c.ddr&2)) {
			this->air|=2;
			tpi6525_set_interrupt(this);
		}
	}
}

static void tpi6525_irq2_level(TPI6525 *this, int level)
{
	if (INTERRUPT_MODE && (level!=this->irq_level[2]) ) {
		this->irq_level[2]=level;
		if ((level==0)&&!(this->air&4)&&(this->c.ddr&4)) {
			this->air|=4;
			tpi6525_set_interrupt(this);
		}
	}
}

static void tpi6525_irq3_level(TPI6525 *this, int level)
{
	if (INTERRUPT_MODE && (level!=this->irq_level[3]) ) {
		this->irq_level[3]=level;
		if ( ((INTERRUPT3_RISING_EDGE&&(level==1))
			  ||(!INTERRUPT3_RISING_EDGE&&(level==0)))
			 &&!(this->air&8)&&(this->c.ddr&8)) {
			this->air|=8;
			tpi6525_set_interrupt(this);
		}
	}
}

static void tpi6525_irq4_level(TPI6525 *this, int level)
{
	if (INTERRUPT_MODE &&(level!=this->irq_level[4]) ) {
		this->irq_level[4]=level;
		if ( ((INTERRUPT4_RISING_EDGE&&(level==1))
			  ||(!INTERRUPT4_RISING_EDGE&&(level==0)))
			  &&!(this->air&0x10)&&(this->c.ddr&0x10)) {
			this->air|=0x10;
			tpi6525_set_interrupt(this);
		}
	}
}

static int tpi6525_port_a_r(TPI6525 *this, int offset)
{
	int data=this->a.in;

	if (this->a.read) data=this->a.read();
	data=(data&~this->a.ddr)|(this->a.ddr&this->a.port);

	return data;
}

static void tpi6525_port_a_w(TPI6525 *this, int offset, int data)
{
	this->a.in=data;
}

static int tpi6525_port_b_r(TPI6525 *this, int offset)
{
	int data=this->b.in;

	if (this->b.read) data=this->b.read();
	data=(data&~this->b.ddr)|(this->b.ddr&this->b.port);

	return data;
}

static void tpi6525_port_b_w(TPI6525 *this, int offset, int data)
{
	this->b.in=data;
}

static int tpi6525_port_c_r(TPI6525 *this, int offset)
{
	int data=this->c.in;

	if (this->c.read) data&=this->c.read();
	data=(data&~this->c.ddr)|(this->c.ddr&this->c.port);

	return data;
}

static void tpi6525_port_c_w(TPI6525 *this, int offset, int data)
{
	this->c.in=data;
}

static int tpi6525_port_r(TPI6525 *this, int offset)
{
	int data=0xff;
	switch (offset&7) {
	case 0:
		data=this->a.in;
		if (this->a.read) data&=this->a.read();
		data=(data&~this->a.ddr)|(this->a.ddr&this->a.port);
		break;
	case 1:
		data=this->b.in;
		if (this->b.read) data&=this->b.read();
		data=(data&~this->b.ddr)|(this->b.ddr&this->b.port);
		break;
	case 2:
		if (INTERRUPT_MODE) {
			data=0;
			if (this->irq_level[0]) data|=1;
			if (this->irq_level[1]) data|=2;
			if (this->irq_level[2]) data|=4;
			if (this->irq_level[3]) data|=8;
			if (this->irq_level[4]) data|=0x10;
			if (!this->interrupt.level) data|=0x20;
			if (this->ca.level) data|=0x40;
			if (this->cb.level) data|=0x80;
		} else {
			data=this->c.in;
			if (this->c.read) data&=this->c.read();
			data=(data&~this->c.ddr)|(this->c.ddr&this->c.port);
		}
		DBG_LOG (2, "tpi6525",
				 (errorlog, "%d read %.2x %.2x\n",this->number, offset,data));
		break;
	case 3:
		data=this->a.ddr;
		break;
	case 4:
		data=this->b.ddr;
		break;
	case 5:
		data=this->c.ddr;
		break;
	case 6: /* cr */
		data=this->cr;
		break;
	case 7: /* air */
		if (PRIORIZED_INTERRUPTS) {
			if (this->air&0x10) {
				data=0x10;
				this->air&=~0x10;
			} else if (this->air&8) {
				data=8;
				this->air&=~8;
			} else if (this->air&4) {
				data=4;
				this->air&=~4;
			} else if (this->air&2) {
				data=2;
				this->air&=~2;
			} else if (this->air&1) {
				data=1;
				this->air&=~1;
			}
		} else {
			data=this->air;
			this->air=0;
		}
		tpi6525_clear_interrupt(this);
		break;
	}
	DBG_LOG (3, "tpi6525",
			 (errorlog, "%d read %.2x %.2x\n",this->number, offset,data));
	return data;
}

static void tpi6525_port_w(TPI6525 *this, int offset, int data)
{
	DBG_LOG (2, "tpi6525",
			 (errorlog, "%d write %.2x %.2x\n",this->number, offset,data));

	switch (offset&7) {
	case 0:
		this->a.port=data;
		if (this->a.output) {
			this->a.output(this->a.port&this->a.ddr);
		}
		break;
	case 1:
		this->b.port=data;
		if (this->b.output) {
			this->b.output(this->b.port&this->b.ddr);
		}
		break;
	case 2:
		this->c.port=data;
		if (!INTERRUPT_MODE&&this->c.output) {
			this->c.output(this->c.port&this->c.ddr);
		}
		break;
	case 3:
		this->a.ddr=data;
		if (this->a.output) {
			this->a.output(this->a.port&this->a.ddr);
		}
		break;
	case 4:
		this->b.ddr=data;
		if (this->b.output) {
			this->b.output(this->b.port&this->b.ddr);
		}
		break;
	case 5:
		this->c.ddr=data;
		if (!INTERRUPT_MODE&&this->c.output) {
			this->c.output(this->c.port&this->c.ddr);
		}
		break;
	case 6:
		this->cr=data;
		if (INTERRUPT_MODE) {
			if (CA_MANUAL_OUT) {
				if (this->ca.level!=CA_MANUAL_LEVEL) {
					this->ca.level=CA_MANUAL_LEVEL;
					if (this->ca.output) this->ca.output(this->ca.level);
				}
			}
			if (CB_MANUAL_OUT) {
				if (this->cb.level!=CB_MANUAL_LEVEL) {
					this->cb.level=CB_MANUAL_LEVEL;
					if (this->cb.output) this->cb.output(this->cb.level);
				}
			}
		}
		break;
	case 7:
		//this->air=data;
		break;
	}
}


void tpi6525_0_reset(void)
{
	tpi6525_reset(tpi6525);
}

void tpi6525_1_reset(void)
{
	tpi6525_reset(tpi6525+1);
}

void tpi6525_2_reset(void)
{
	tpi6525_reset(tpi6525+2);
}

void tpi6525_3_reset(void)
{
	tpi6525_reset(tpi6525+3);
}

void tpi6525_0_irq0_level(int level)
{
	tpi6525_irq0_level(tpi6525, level);
}

void tpi6525_0_irq1_level(int level)
{
	tpi6525_irq1_level(tpi6525, level);
}

void tpi6525_0_irq2_level(int level)
{
	tpi6525_irq2_level(tpi6525, level);
}

void tpi6525_0_irq3_level(int level)
{
	tpi6525_irq3_level(tpi6525, level);
}

void tpi6525_0_irq4_level(int level)
{
	tpi6525_irq4_level(tpi6525, level);
}

void tpi6525_1_irq0_level(int level)
{
	tpi6525_irq0_level(tpi6525+1, level);
}

void tpi6525_1_irq1_level(int level)
{
	tpi6525_irq1_level(tpi6525+1, level);
}

void tpi6525_1_irq2_level(int level)
{
	tpi6525_irq2_level(tpi6525+1, level);
}

void tpi6525_1_irq3_level(int level)
{
	tpi6525_irq3_level(tpi6525+1, level);
}

void tpi6525_1_irq4_level(int level)
{
	tpi6525_irq4_level(tpi6525+1, level);
}

READ_HANDLER ( tpi6525_0_port_r )
{
	return tpi6525_port_r(tpi6525, offset);
}

READ_HANDLER ( tpi6525_1_port_r )
{
	return tpi6525_port_r(tpi6525+1, offset);
}

READ_HANDLER ( tpi6525_2_port_r )
{
	return tpi6525_port_r(tpi6525+2, offset);
}

READ_HANDLER ( tpi6525_3_port_r )
{
	return tpi6525_port_r(tpi6525+3, offset);
}

WRITE_HANDLER ( tpi6525_0_port_w )
{
	tpi6525_port_w(tpi6525, offset, data);
}

WRITE_HANDLER ( tpi6525_1_port_w )
{
	tpi6525_port_w(tpi6525+1, offset, data);
}

WRITE_HANDLER ( tpi6525_2_port_w )
{
	tpi6525_port_w(tpi6525+2, offset, data);
}

WRITE_HANDLER ( tpi6525_3_port_w )
{
	tpi6525_port_w(tpi6525+3, offset, data);
}

READ_HANDLER ( tpi6525_0_port_a_r )
{
	return tpi6525_port_a_r(tpi6525, offset);
}

READ_HANDLER ( tpi6525_1_port_a_r )
{
	return tpi6525_port_a_r(tpi6525+1, offset);
}

READ_HANDLER ( tpi6525_2_port_a_r )
{
	return tpi6525_port_a_r(tpi6525+2, offset);
}

READ_HANDLER ( tpi6525_3_port_a_r )
{
	return tpi6525_port_a_r(tpi6525+3, offset);
}

WRITE_HANDLER ( tpi6525_0_port_a_w )
{
	tpi6525_port_a_w(tpi6525, offset, data);
}

WRITE_HANDLER ( tpi6525_1_port_a_w )
{
	tpi6525_port_a_w(tpi6525+1, offset, data);
}

WRITE_HANDLER ( tpi6525_2_port_a_w )
{
	tpi6525_port_a_w(tpi6525+2, offset, data);
}

WRITE_HANDLER ( tpi6525_3_port_a_w )
{
	tpi6525_port_a_w(tpi6525+3, offset, data);
}

READ_HANDLER ( tpi6525_0_port_b_r )
{
	return tpi6525_port_b_r(tpi6525, offset);
}

READ_HANDLER ( tpi6525_1_port_b_r )
{
	return tpi6525_port_b_r(tpi6525+1, offset);
}

READ_HANDLER ( tpi6525_2_port_b_r )
{
	return tpi6525_port_b_r(tpi6525+2, offset);
}

READ_HANDLER ( tpi6525_3_port_b_r )
{
	return tpi6525_port_b_r(tpi6525+3, offset);
}

WRITE_HANDLER ( tpi6525_0_port_b_w )
{
	tpi6525_port_b_w(tpi6525, offset, data);
}

WRITE_HANDLER ( tpi6525_1_port_b_w )
{
	tpi6525_port_b_w(tpi6525+1, offset, data);
}

WRITE_HANDLER ( tpi6525_2_port_b_w )
{
	tpi6525_port_b_w(tpi6525+2, offset, data);
}

WRITE_HANDLER ( tpi6525_3_port_b_w )
{
	tpi6525_port_b_w(tpi6525+3, offset, data);
}

READ_HANDLER ( tpi6525_0_port_c_r )
{
	return tpi6525_port_c_r(tpi6525, offset);
}

READ_HANDLER ( tpi6525_1_port_c_r )
{
	return tpi6525_port_c_r(tpi6525+1, offset);
}

READ_HANDLER ( tpi6525_2_port_c_r )
{
	return tpi6525_port_c_r(tpi6525+2, offset);
}

READ_HANDLER ( tpi6525_3_port_c_r )
{
	return tpi6525_port_c_r(tpi6525+3, offset);
}

WRITE_HANDLER ( tpi6525_0_port_c_w )
{
	tpi6525_port_c_w(tpi6525, offset, data);
}

WRITE_HANDLER ( tpi6525_1_port_c_w )
{
	tpi6525_port_c_w(tpi6525+1, offset, data);
}

WRITE_HANDLER ( tpi6525_2_port_c_w )
{
	tpi6525_port_c_w(tpi6525+2, offset, data);
}

WRITE_HANDLER ( tpi6525_3_port_c_w )
{
	tpi6525_port_c_w(tpi6525+3, offset, data);
}

