/**********************************************************************
	Metal Oxid Semiconductor / Commodore Business Machines
        Complex Interface Adapter 6526

	based on 6522via emulation
**********************************************************************/
#include "driver.h"
#include "assert.h"

#define VERBOSE_DBG 1      /* general debug messages */
#include "cbm.h"

#include "cia6526.h"

/* todin pin 50 or 60 hertz frequency */
#define TODIN_50HZ(v) ((v)->cra&0x80) /* else 60 Hz */

#define TIMER1_B6(v) ((v)->cra&2)
#define TIMER1_B6_TOGGLE(v) ((v)->cra&4) /* else single pulse */
#define TIMER1_ONESHOT(v) ((v)->cra&8) /* else continuous */
#define TIMER1_STOP(v) (!((v)->cra&1))
#define TIMER1_RELOAD(v) ((v)->cra&0x10)
#define TIMER1_COUNT_CNT(v) (v->cra&0x20) /* else clock 2 input */

#define TIMER2_ONESHOT(v) ((v)->crb&8) /* else continuous */
#define TIMER2_STOP(v) (!((v)->crb&1))
#define TIMER2_RELOAD(v) ((v)->crb&0x10)
#define TIMER2_COUNT_CLOCK(v) ((v->crb&0x60)==0)
#define TIMER2_COUNT_CNT(v) ((v->crb&0x60)==0x20)
#define TIMER2_COUNT_TIMER1(v) ((v->crb&0x60)==0x40)
#define TIMER2_COUNT_TIMER1_CNT(v) ((v->crb&0x60)==0x60)

#define TOD_ALARM(v) ((v)->crb&0x80) /* else write to tod clock */
#define BCD_INC(v) (((v)&0xf)==9?(v)+=0x10-9:(v)++)

struct cia6526
{
  const struct cia6526_interface *intf;
  int todin50hz; /* else 60 hz */
  
  UINT8 in_a;
  UINT8 out_a;
  UINT8 ddr_a;
  
  UINT8 in_b;
  UINT8 out_b;
  UINT8 ddr_b;
  
  UINT16 t1c;
  UINT32 t1l;
  void *timer1;
  int timer1_state;

  UINT16 t2c;
  UINT32 t2l;
  void *timer2;
  int timer2_state;

  UINT8 tod10ths, todsec, todmin, todhour;
  UINT8 alarm10ths, alarmsec, alarmmin, alarmhour;

  UINT8 latch10ths, latchsec, latchmin, latchhour;
  int todlatched;

  int todstopped;
  void *todtimer;

  int cnt, /* input */
    flag, /* input */
    sp; /* input or output */

  UINT8 cra, crb;

  UINT8 ier;
  UINT8 ifr;
};

static struct cia6526 cia[MAX_CIA]={{0}};

static void cia_timer1_timeout (int which);
static void cia_timer2_timeout (int which);

/******************* configuration *******************/

void cia6526_config(int which, const struct cia6526_interface *intf, 
		    int todin50hz)
{
  if (which >= MAX_CIA) return;
  memset(cia+which,0,sizeof(cia[which]));
  cia[which].intf=intf;
  cia[which].todin50hz=todin50hz;
}


/******************* reset *******************/

void cia6526_reset(void)
{
  int i;
  assert(((int)cia[0].intf&3)==0);
  
  /* zap each structure, preserving the interface and swizzle */
  for (i = 0; i < MAX_CIA; i++)
    {
      const struct cia6526_interface *intf = cia[i].intf;
      int todin50hz=cia[i].todin50hz;
      if (cia[i].timer1) timer_remove(cia[i].timer1);
      if (cia[i].timer2) timer_remove(cia[i].timer2);
      if (cia[i].todtimer) timer_remove(cia[i].todtimer);
      memset(&cia[i], 0, sizeof(cia[i]));
      cia[i].intf=intf;
      cia[i].todin50hz=todin50hz;
      cia[i].t1l=0xffff;
      cia[i].t2l=0xffff;
    }
  assert(((int)cia[0].intf&3)==0);
}

/******************* external interrupt check *******************/

static void cia_set_interrupt (int which, int data)
{
  struct cia6526 *v=cia+which;
  v->ifr|=data;
  if (v->ier&data) {
    if (!(v->ifr&0x80)) { 
      DBG_LOG(3,"cia set interrupt",(errorlog,"%d %.2x\n",which,data));
      if (v->intf->irq_func) v->intf->irq_func(1);
      v->ifr|=0x80;
    }
  }
}

static void cia_clear_interrupt(int which, int data)
{
  struct cia6526 *v=cia+which;

  v->ifr&=~data;
  if ((v->ifr&0x9f)==0x80) {
    v->ifr&=~0x80;
    if (v->intf->irq_func) v->intf->irq_func(0);
  }
}

/******************* Timer timeouts *************************/
static void cia_tod_timeout(int which)
{
  struct cia6526 *v=cia+which;

  v->tod10ths++;
  if (v->tod10ths>9) {
    v->tod10ths=0;
    BCD_INC(v->todsec);
    if (v->todsec>0x59) {
      v->todsec=0;
      BCD_INC(v->todmin);
      if (v->todmin>0x59) {
	v->todmin=0;
	if (v->todhour==0x91) v->todhour=0;
	else if (v->todhour==0x89) v->todhour=0x90;
	else if (v->todhour==0x11) v->todhour=0x80;
	else if (v->todhour==0x09) v->todhour=0x10;
	else v->todhour++;
      }
    }
  }
  if ( (v->todhour==v->alarmhour)
       &&(v->todmin==v->alarmmin)
       &&(v->todsec==v->alarmsec)
       &&(v->tod10ths==v->alarm10ths)) cia_set_interrupt(which, 4);
  if (TODIN_50HZ(v)) {
    if (v->todin50hz) timer_reset(v->todtimer, 0.1);
    else timer_reset(v->todtimer, 5.0/60);
  } else {
    if (v->todin50hz) timer_reset(v->todtimer, 6.0/50);
    else timer_reset(v->todtimer, 0.1);
  }
}

static void cia_timer1_state(int which)
{
  struct cia6526 *v = cia + which;

  DBG_LOG(1,"timer1 state",(errorlog,"%d\n",v->timer1_state));
  switch (v->timer1_state) {
  case 0: /* timer stopped */
    if (TIMER1_RELOAD(v)) {
      v->cra&=~0x10;
      v->t1c=v->t1l;
    }
    if (!TIMER1_STOP(v)) {
      if (TIMER1_COUNT_CNT(v)) {
	v->timer1_state=2;
      } else {
	v->timer1_state=1;
	v->timer1=timer_set(TIME_IN_CYCLES(v->t1c, 0), which, cia_timer1_timeout);
      }
    }
    break;
  case 1: /* counting clock input */
    if (TIMER1_RELOAD(v)) {
      v->cra&=~0x10;
      v->t1c=v->t1l;
      if (!TIMER1_STOP(v))
	timer_reset(v->timer1,TIME_IN_CYCLES(v->t1c, 0));
    }
    if (TIMER1_STOP(v)) {
      v->timer1_state=0;
      timer_remove(v->timer1);
      v->timer1=0;
    } else if (TIMER1_COUNT_CNT(v)) {
      v->timer1_state=2;
      timer_remove(v->timer1);
      v->timer1=0;
    }
    break;
  case 2: /* counting cnt input */
    if (TIMER1_RELOAD(v)) {
      v->cra&=~0x10;
      v->t1c=v->t1l;
    }
    if (TIMER1_STOP(v)) {
      v->timer1_state=0;
    } else if (!TIMER1_COUNT_CNT(v)) {
      v->timer1=timer_set(TIME_IN_CYCLES(v->t1c, 0), which, cia_timer1_timeout);
      v->timer1_state=1;
    }
    break;
  }
  DBG_LOG(1,"timer1 state",(errorlog,"%d\n",v->timer1_state));
}

static void cia_timer2_state(int which)
{
  struct cia6526 *v = cia + which;

  switch (v->timer2_state) {
  case 0: /* timer stopped */
    if (TIMER2_RELOAD(v)) {
      v->crb&=~0x10;
      v->t2c=v->t2l;
    }
    if (!TIMER2_STOP(v)) {
      if (TIMER2_COUNT_CLOCK(v)) {
	v->timer2_state=1;
	v->timer2=timer_set(TIME_IN_CYCLES(v->t2c, 0), which, cia_timer2_timeout);
      } else {
	v->timer2_state=2;
      }
    }
    break;
  case 1: /* counting clock input */
    if (TIMER2_RELOAD(v)) {
      v->crb&=~0x10;
      v->t2c=v->t2l;
      timer_reset(v->timer2,TIME_IN_CYCLES(v->t2c, 0));
    }
    if (TIMER2_STOP(v)) {
      v->timer2_state=0;
      timer_remove(v->timer2);
      v->timer2=0;
    } else if (!TIMER2_COUNT_CLOCK(v)) {
      v->timer2_state=2;
      timer_remove(v->timer2);
      v->timer2=0;
    }
    break;
  case 2: /* counting cnt, timer1  input */
    if (v->t2c==0) {
      cia_set_interrupt(which, 2);
      v->crb|=0x10;
    }
    if (TIMER2_RELOAD(v)) {
      v->crb&=~0x10;
      v->t2c=v->t2l;
    }
    if (TIMER2_STOP(v)) {
      v->timer2_state=0;
    } else if (TIMER2_COUNT_CLOCK(v)) {
      v->timer2=timer_set(TIME_IN_CYCLES(v->t2c, 0), which, cia_timer2_timeout);
      v->timer2_state=1;
    }
    break;
  }
}

static void cia_timer1_timeout (int which)
{
  struct cia6526 *v = cia + which;

  v->t1c=v->t1l;

  if (TIMER1_ONESHOT(v)) {
    v->cra&=~1;
    v->timer1_state=0;
  } else {
    timer_reset(v->timer1,TIME_IN_CYCLES(v->t1c, 0));
  }
  cia_set_interrupt(which, 1);
  /*  cia_timer1_state(which); */

  if (TIMER2_COUNT_TIMER1(v)||((TIMER2_COUNT_TIMER1_CNT(v))&&(v->cnt)) ) {
    v->t2c--;
    cia_timer2_state(which);
  }
}

static void cia_timer2_timeout (int which)
{
  struct cia6526 *v = cia + which;

  v->t2c=v->t2l;

  if (TIMER2_ONESHOT(v)) {
    v->crb&=~1;
    v->timer2_state=0;
  } else {
    timer_reset(v->timer2,TIME_IN_CYCLES(v->t2c, 0));
  }

  cia_set_interrupt(which, 2);
  /*  cia_timer2_state(which); */
}


/******************* CPU interface for VIA read *******************/

int cia6526_read(int which, int offset)
{
  struct cia6526 *v = cia + which;
  int val = 0;
  
  offset &=0xf;
  switch (offset) {
  case 0:
    if (v->intf->in_a_func) v->in_a=v->intf->in_a_func(which);
    val=((v->out_a&v->ddr_a)|(0xff&~v->ddr_a))&v->in_a;
    break;
  case 1:
    if (v->intf->in_b_func) v->in_b=v->intf->in_b_func(which);
    val=((v->out_b&v->ddr_b)|(0xff&~v->ddr_b))&v->in_b;
    break;
  case 2: val=v->ddr_a; break;
  case 3: val=v->ddr_b; break;
  case 8: 
    if (v->todlatched) val=v->latch10ths; 
    else val=v->tod10ths;
    v->todlatched=0;
    break;
  case 9: 
    if (v->todlatched) val=v->latchsec; 
    else val=v->todsec;
    break;
  case 0xa: 
    if (v->todlatched) val=v->latchmin; 
    else val=v->todmin;
    break;
  case 0xb: 
    v->latch10ths=v->tod10ths;
    v->latchsec=v->todsec;
    v->latchmin=v->todmin;
    val=v->latchhour=v->todhour;
    v->todlatched=1;
    break;
  case 0xd:
    val=v->ifr&~0x60;
    cia_clear_interrupt(which, 0x1f);
    break;
  case 4:
    if (v->timer1)
      val = TIME_TO_CYCLES(0, timer_timeleft(v->timer1))&0xff;
    else val=v->t1c&0xff;
    DBG_LOG(3,"cia timer 1 lo",(errorlog,"%d %.2x\n",which,val));
    break;
  case 5:
    if (v->timer1)
      val = TIME_TO_CYCLES(0, timer_timeleft(v->timer1)) >> 8;
    else val=v->t1c>>8;
    DBG_LOG(3,"cia timer 1 hi",(errorlog,"%d %.2x\n",which,val));
    break;
  case 6:
    if (v->timer2)
      val = TIME_TO_CYCLES(0, timer_timeleft(v->timer2))&0xff;
    else val=v->t2c&0xff;
    DBG_LOG(3,"cia timer 2 lo",(errorlog,"%d %.2x\n",which,val));
    break;
  case 7:
    if (v->timer2)
      val = TIME_TO_CYCLES(0, timer_timeleft(v->timer2)) >> 8;
    else val=v->t2c>>8;
    DBG_LOG(3,"cia timer 2 hi",(errorlog,"%d %.2x\n",which,val));
    break; 
  case 0xe: val=v->cra;break;
  case 0xf: val=v->crb;break;
  }
  DBG_LOG(2,"cia read",(errorlog,"%d %.2x:%.2x\n",which,offset,val));
  return val;
}


/******************* CPU interface for VIA write *******************/

void cia6526_write(int which, int offset, int data)
{
  struct cia6526 *v = cia + which;

  DBG_LOG(2,"cia write",(errorlog,"%d %.2x:%.2x\n",which,offset,data));
  offset&=0xf;

  switch (offset) {
  case 0:
    v->out_a=data;
    if (v->intf->out_a_func)
      v->intf->out_a_func(which, v->out_a & v->ddr_a);
    break;
  case 1:
    v->out_b=data;
    if (v->intf->out_b_func)
      v->intf->out_b_func(which, v->out_b & v->ddr_b);
    break;
  case 2: 
    v->ddr_a = data;
    if (v->intf->out_a_func)
      v->intf->out_a_func(which, v->out_a & v->ddr_a);
    break;
  case 3: 
    v->ddr_b = data;
    if (v->intf->out_b_func)
      v->intf->out_b_func(which, v->out_b & v->ddr_b);
    break;
  case 8: 
    if (TOD_ALARM(v)) v->alarm10ths=data;
    else {
      v->tod10ths=data;
      if (v->todstopped) {
        if (TODIN_50HZ(v)) {
	   if (v->todin50hz) 
	     v->todtimer=timer_set(0.1, which, cia_tod_timeout);
	   else
	     v->todtimer=timer_set(5.0/60, which, cia_tod_timeout);
	} else {
	  if (v->todin50hz)
	    v->todtimer=timer_set(60/5.0, which, cia_tod_timeout);
	  else
	    v->todtimer=timer_set(0.1, which, cia_tod_timeout);
	}
      }
      v->todstopped=0;
    } 
    break;
  case 9: 
    if (TOD_ALARM(v)) v->alarmsec=data;
    else v->todsec=data; 
    break;
  case 0xa: 
    if (TOD_ALARM(v)) v->alarmmin=data;
    else v->todmin=data; 
    break;
  case 0xb:
    if (TOD_ALARM(v)) v->alarmhour=data;
    else {
      if (v->todtimer) timer_remove(v->todtimer);v->todtimer=0;
      v->todstopped=1;
      v->todhour=data;
    }
    break;
  case 0xd:
    DBG_LOG(1,"cia interrupt enable",(errorlog,"%d %.2x\n",which,data));
    if (data&0x80) {
      v->ier|=data;
      cia_set_interrupt(which,data&0x1f);
    } else {
      v->ier&=~data;
      cia_clear_interrupt(which,data&0x1f);
    }
    break;
  case 4:
    v->t1l=(v->t1l&~0xff)|data;
    if (v->t1l==0) v->t1l=0x10000; /*avoid hanging in timer_schedule*/
    DBG_LOG(3,"cia timer 1 lo write",(errorlog,"%d %.2x\n",which,data));
    break;
  case 5:
    v->t1l=(v->t1l&0xff)|(data<<8);
    if (v->t1l==0) v->t1l=0x10000; /*avoid hanging in timer_schedule*/    
    if (TIMER1_STOP(v)) v->t1c=v->t1l;
    DBG_LOG(3,"cia timer 1 hi write",(errorlog,"%d %.2x\n",which,data));
    break;
  case 6:
    v->t2l=(v->t2l&~0xff)|data;
    if (v->t2l==0) v->t2l=0x10000; /*avoid hanging in timer_schedule*/
    DBG_LOG(3,"cia timer 2 lo write",(errorlog,"%d %.2x\n",which,data));
    break;
  case 7:
    v->t2l=(v->t2l&0xff)|(data<<8);
    if (v->t2l==0) v->t2l=0x10000; /*avoid hanging in timer_schedule*/
    if (TIMER2_STOP(v)) v->t2c=v->t2l;
    DBG_LOG(3,"cia timer 2 hi write",(errorlog,"%d %.2x\n",which,data));
    break;
  case 0xe:
    DBG_LOG(3,"cia write cra",(errorlog,"%d %.2x\n",which,data));  
    v->cra=data;
    cia_timer1_state(which);
    break;
  case 0xf:
    DBG_LOG(3,"cia write crb",(errorlog,"%d %.2x\n",which,data));
    v->crb=data;
    cia_timer2_state(which);
    break;
  }
}

void cia_set_input_a(int which, int data)
{
  struct cia6526 *v = cia + which;
  v->in_a = data;
}

void cia_set_input_b(int which, int data)
{
  struct cia6526 *v = cia + which;
  v->in_b = data;
}

void cia6526_set_input_flag(int which, int data)
{
  struct cia6526 *v = cia + which;
  if (v->flag&&!data) cia_set_interrupt(which,0x10);
  v->flag=data;
}

int cia6526_0_port_r(int offset) { return cia6526_read(0, offset); }
int cia6526_1_port_r(int offset) { return cia6526_read(1, offset); }
int cia6526_2_port_r(int offset) { return cia6526_read(2, offset); }
int cia6526_3_port_r(int offset) { return cia6526_read(3, offset); }
int cia6526_4_port_r(int offset) { return cia6526_read(4, offset); }
int cia6526_5_port_r(int offset) { return cia6526_read(5, offset); }
int cia6526_6_port_r(int offset) { return cia6526_read(6, offset); }
int cia6526_7_port_r(int offset) { return cia6526_read(7, offset); }

void cia6526_0_port_w(int offset, int data) 
{ cia6526_write(0, offset, data); }
void cia6526_1_port_w(int offset, int data) 
{ cia6526_write(1, offset, data); }
void cia6526_2_port_w(int offset, int data) 
{ cia6526_write(2, offset, data); }
void cia6526_3_port_w(int offset, int data) 
{ cia6526_write(3, offset, data); }
void cia6526_4_port_w(int offset, int data) 
{ cia6526_write(4, offset, data); }
void cia6526_5_port_w(int offset, int data) 
{ cia6526_write(5, offset, data); }
void cia6526_6_port_w(int offset, int data) 
{ cia6526_write(6, offset, data); }
void cia6526_7_port_w(int offset, int data) 
{ cia6526_write(7, offset, data); }

/******************* 8-bit A/B port interfaces *******************/

void cia6526_0_porta_w(int offset, int data) { cia_set_input_a(0, data); }
void cia6526_1_porta_w(int offset, int data) { cia_set_input_a(1, data); }
void cia6526_2_porta_w(int offset, int data) { cia_set_input_a(2, data); }
void cia6526_3_porta_w(int offset, int data) { cia_set_input_a(3, data); }
void cia6526_4_porta_w(int offset, int data) { cia_set_input_a(4, data); }
void cia6526_5_porta_w(int offset, int data) { cia_set_input_a(5, data); }
void cia6526_6_porta_w(int offset, int data) { cia_set_input_a(6, data); }
void cia6526_7_porta_w(int offset, int data) { cia_set_input_a(7, data); }

void cia6526_0_portb_w(int offset, int data) { cia_set_input_b(0, data); }
void cia6526_1_portb_w(int offset, int data) { cia_set_input_b(1, data); }
void cia6526_2_portb_w(int offset, int data) { cia_set_input_b(2, data); }
void cia6526_3_portb_w(int offset, int data) { cia_set_input_b(3, data); }
void cia6526_4_portb_w(int offset, int data) { cia_set_input_b(4, data); }
void cia6526_5_portb_w(int offset, int data) { cia_set_input_b(5, data); }
void cia6526_6_portb_w(int offset, int data) { cia_set_input_b(6, data); }
void cia6526_7_portb_w(int offset, int data) { cia_set_input_b(7, data); }

int cia6526_0_porta_r(int offset) { return cia[0].in_a; }
int cia6526_1_porta_r(int offset) { return cia[1].in_a; }
int cia6526_2_porta_r(int offset) { return cia[2].in_a; }
int cia6526_3_porta_r(int offset) { return cia[3].in_a; }
int cia6526_4_porta_r(int offset) { return cia[4].in_a; }
int cia6526_5_porta_r(int offset) { return cia[5].in_a; }
int cia6526_6_porta_r(int offset) { return cia[6].in_a; }
int cia6526_7_porta_r(int offset) { return cia[7].in_a; }
int cia6526_0_portb_r(int offset) { return cia[0].in_b; }
int cia6526_1_portb_r(int offset) { return cia[1].in_b; }
int cia6526_2_portb_r(int offset) { return cia[2].in_b; }
int cia6526_3_portb_r(int offset) { return cia[3].in_b; }
int cia6526_4_portb_r(int offset) { return cia[4].in_b; }
int cia6526_5_portb_r(int offset) { return cia[5].in_b; }
int cia6526_6_portb_r(int offset) { return cia[6].in_b; }
int cia6526_7_portb_r(int offset) { return cia[7].in_b; }

void cia6526_0_set_input_flag(int data) { cia6526_set_input_flag(0, data); }
void cia6526_1_set_input_flag(int data) { cia6526_set_input_flag(1, data); }
void cia6526_2_set_input_flag(int data) { cia6526_set_input_flag(2, data); }
void cia6526_3_set_input_flag(int data) { cia6526_set_input_flag(3, data); }
void cia6526_4_set_input_flag(int data) { cia6526_set_input_flag(4, data); }
void cia6526_5_set_input_flag(int data) { cia6526_set_input_flag(5, data); }
void cia6526_6_set_input_flag(int data) { cia6526_set_input_flag(6, data); }
void cia6526_7_set_input_flag(int data) { cia6526_set_input_flag(7, data); }

void cia6526_status(char *text, int size)
{
  text[0]=0;
#if VERBOSE_DBG
# if 0
  snprintf(text,size,"cia ier:%.2x ifr:%.2x %d", cia[1].ier, cia[1].ifr, 
	   cia[1].flag);
# endif
# if 0
  snprintf(text,size,"cia 1 %.2x %.2x %.2x %.2x", 
	   cia[1].tod10ths, cia[1].todsec, 
	   cia[1].todmin, cia[1].todhour);

# endif
#endif
}
