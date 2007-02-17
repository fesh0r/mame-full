/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Motorola 6854 emulation.

  The MC6854 chip is an Advanced Data-Link Controller (ADLC).
  It provides a high-level network interface that can tramsimit frames with
  arbitrary data and address length, and is compatible with the following 
  standards:
  - ADCCP (Advanced Data Communication Control Procedure)
  - HDLC  (High-Level Data-Link Control)
  - SDLC  (Synchronous Data-Link Control)
  It is designed to be interfaced with a M6800-family CPU.

  It is used in the "Nano-network" extension of the Thomson computers to 
  link up to 32 computers at 500 Kbps.
  Many networks involving one PC server and several MO5 or TO7/70 computers
  were build in French schools in the 1980's to teach computer science.

  TODO:
  - CRC
  - DMA mode
  - loop mode
  - status prioritization
  - NRZI vs. NRZ coding
  - FD output

**********************************************************************/


#include "driver.h"
#include "timer.h"
#include "state.h"
#include "mc6854.h"

/******************* parameters ******************/

#define VERBOSE 0


#define MAX_FRAME_LENGTH 65536
/* arbitrary value, you may need to enlarge it if you get truncated frames */

#define FIFO_SIZE 3 
/* hardcoded size of the 6854 FIFO (this is a hardware limit) */

#define FLAG 0x7e
/* flag value, as defined by HDLC protocol: 01111110 */

#define BIT_LENGTH TIME_IN_HZ( 500000 )


/******************* internal chip data structure ******************/

static struct {

  /* interface */
  const mc6854_interface* iface;

  /* registers */
  UINT8 cr1, cr2, cr3, cr4; /* control registers */
  UINT8 sr1, sr2;           /* status registers */

  UINT8 cts, dcd;

  /* transmit state */
  UINT8  tstate;
  UINT16 tfifo[FIFO_SIZE];  /* X x 8-bit FIFO + full & last marker bits */
  UINT8  tones;             /* counter for zero-insertion */
  mame_timer *ttimer;       /* when to ask for more data */

  /* receive state */
  UINT8  rstate;
  UINT32 rreg;              /* shift register */
  UINT8  rones;             /* count '1 bits */
  UINT8  rsize;             /* bits in the shift register */
  UINT16 rfifo[FIFO_SIZE];  /* X x 8-bit FIFO + full & addr marker bits */

  /* frame-based interface*/
  UINT8  frame[MAX_FRAME_LENGTH];
  UINT32 flen, fpos;

} * mc6854;

/* meaning of tstate / rtate:
   0 = idle / waiting for frame flag
   1 = flag sync
   2 = 8-bit address field(s)
 3-4 = 8-bit  control field(s)
   5 = 8-bit logical control field(s)
   6 = variable-length data field(s)
*/

/******************* utilitiy function and macros ********************/

#if VERBOSE
#define LOG(x)  logerror x
#else
#define LOG(x)
#endif

#define AC ( mc6854->cr1 & 1 )
#define FCTDRA ( mc6854->cr2 & 8 )
/* extra register select bits */

#define RRESET ( mc6854->cr1 & 0x40 )
#define TRESET ( mc6854->cr1 & 0x80 )
/* transmit / reset condition */

#define RIE ( mc6854->cr1 & 2 )
#define TIE ( mc6854->cr1 & 4 )
/* interrupt enable */

#define DISCONTINUE ( mc6854->cr1 & 0x20 )
/* discontinue received frame */

#define PSE ( mc6854->cr2 & 1 )
/* prioritize status bits (TODO) */

#define TWOBYTES ( mc6854->cr2 & 2 )
/* two-bytes mode */

#define FMIDLE ( mc6854->cr2 & 4 )
/* flag time fill (vs. mark idle) */

#define TLAST ( mc6854->cr2 & 0x10 )
/* transmit last byte of frame */

#define RTS ( mc6854->cr2 & 0x80 )
/* request-to-send */

#define LCF ( mc6854->cr3 & 1 )
/* logical control field select */

#define CEX ( mc6854->cr3 & 2 )
/* control field is 16 bits instead of 8 */

#define AEX ( mc6854->cr3 & 4 )
/* extended address mode (vs normal 8-bit address mode) */

#define IDL0 ( mc6854->cr3 & 8 )
/* idle condition begins with a '0' instead of a '1" */

#define FDSE ( mc6854->cr3 & 0x10 )
/* enable the flag detect status in SR1 */

#define LOOP ( mc6854->cr3 & 0x20 )
/* loop mode */

#define TST ( mc6854->cr3 & 0x40 )
/* test mode (or go active on poll) */

#define DTR ( mc6854->cr3 & 0x80 )
/* data-transmit-ready (or loop on-line control) */

#define TWOINTER ( mc6854->cr4 & 1 )
/* both an openning and a closing inter-frame are sent */

static const int word_length[4] = { 5, 6, 7, 8 };
#define TWL word_length[ ( mc6854->cr4 >> 1 ) & 3 ]
#define RWL word_length[ ( mc6854->cr4 >> 3 ) & 3 ]
/* transmit / receive word length */

#define ABT ( mc6854->cr4 & 0x20 )
/* aborts */

#define ABTEX ( mc6854->cr4 & 0x40 )
/* abort generates 16 '1' bits instead of 8 */

#define NRZ ( mc6854->cr4 & 0x80 )
/* zero complement / non-zero complement data format */


/* status register 1 */
#define RDA  0x01  /* receiver data available */
#define S2RQ 0x02  /* status register #2 read request */
#define FD   0x04  /* flag detect */
#define CTS  0x10  /* clear-to-send */
#define TU   0x20  /* transmitter underrun */
#define TDRA 0x40  /* transmitter data register available */
#define IRQ  0x80  /* interrupt request */

/* status register 2 */
#define AP    0x01  /* address present */
#define FV    0x02  /* frame valid */
#define RIDLE 0x04  /* receiver idle */
#define RABT  0x08  /* receiver abort */
#define ERR   0x10  /* invalid frame error */
#define DCD   0x20  /* data carrier detect (ignored) */
#define OVRN  0x40  /* receiver overrun */
#define RDA2  0x80  /* copy of RDA */



/*********************** transmit ***********************/

/* MC6854 fills bit queue */
static void mc6854_send_bits( UINT32 data, int len, int zi )
{
  double expire;
  int i;
  if ( zi ) {
    /* zero-insertion mode */
    UINT32 d = 0;
    int l = 0;
    for ( i = 0; i < len; i++, data >>= 1, l++ )
      if ( data & 1 ) {
	d |= 1 << l;
	mc6854->tones++;
	if ( mc6854->tones == 5 ) {
	  /* insert a '0' after 5 consecutive '1" */
	  mc6854->tones = 0;
	  l++;	  
	}
      }
      else mc6854->tones = 0;
    data = d;
    len = l;
  }
  else mc6854->tones = 0;

  /* send bits */
  if ( mc6854->iface->out_tx ) {
    for ( i = 0; i < len; i++, data >>= 1 )
      mc6854->iface->out_tx( data & 1 );
  }

  /* schedule when to ask the MC6854 for more bits */
  expire = timer_timeleft( mc6854->ttimer );
  if ( expire >= TIME_IN_SEC( 1 ) ) expire = 0;
  timer_reset( mc6854->ttimer, expire + len * BIT_LENGTH );
}

/* CPU push -> tfifo[0] -> ... -> tfifo[FIFO_SIZE-1] -> pop */
static void mc6854_tfifo_push( UINT8 data )
{
  int i;

  if ( TRESET ) return;

  /* push towards the rightmost free entry */
  for ( i = FIFO_SIZE - 1; i >= 0; i-- )
    if ( ! ( mc6854->tfifo[ i ] & 0x100 ) ) break;
  if ( i >= 0 ) mc6854->tfifo[ i ] = data | 0x100;
  else logerror( "%f mc6854_tfifo_push: FIFO overrun\n", timer_get_time() );

  /* start frame, if needed */
  if ( ! mc6854->tstate ) {
    LOG(( "%f mc6854_tfifo_push: start frame\n", timer_get_time() ));
    mc6854->tstate = 2;
    mc6854_send_bits( FLAG, 8, 0 );
  }
}

/* CPU asks for normal frame termination */
static void mc6854_tfifo_terminate( void )
{
  /* mark most recently pushed byte as the last one of the frame */
  int i;
  for ( i = 0; i < FIFO_SIZE; i++ )
    if ( mc6854->tfifo[ i ] & 0x100 ) {
      mc6854->tfifo[ i ] |= 0x200;
      break;
    }
}

/* call-back to refill the bit-stream from the FIFO */
static void mc6854_tfifo_cb( int dummy )
{
  int i, data = mc6854->tfifo[ FIFO_SIZE - 1 ];

  if ( ! mc6854->tstate ) return;

  /* shift FIFO to the right */
  for ( i = FIFO_SIZE - 1; i > 0; i-- )
    mc6854->tfifo[ i ] = mc6854->tfifo[ i - 1 ];
  mc6854->tfifo[ 0 ] = 0;

  if ( data & 0x100 ) {
    /* got data */

    int blen = 8;

    switch ( mc6854->tstate ) {

    case 2: /* 8-bit address field */
      if ( ( data & 1 ) || ( ! AEX ) ) mc6854->tstate = 3;
      LOG(( "%f mc6854_tfifo_cb: address field $%02X\n", 
	    timer_get_time(), data & 0xff ));
      break;

    case 3: /* 8-bit control field */
      if ( CEX )  mc6854->tstate = 4; 
      else if ( LCF ) mc6854->tstate = 5; else mc6854->tstate = 6;
      LOG(( "%f mc6854_tfifo_cb: control field $%02X\n", 
	    timer_get_time(), data & 0xff ));
      break;
      
    case 4: /* 8-bit extended control field (optional) */
      if ( LCF ) mc6854->tstate = 5; else mc6854->tstate = 6;
      LOG(( "%f mc6854_tfifo_cb: control field $%02X\n", 
	    timer_get_time(), data & 0xff ));
      break;
      
    case 5: /* 8-bit logical control (optional) */
      if ( ! ( data & 0x80 ) ) mc6854->tstate = 6;
      LOG(( "%f mc6854_tfifo_cb: logical control field $%02X\n",
	    timer_get_time(), data & 0xff ));
      break;
      
    case 6: /* variable-length data */
      blen = TWL; 
      LOG(( "%f mc6854_tfifo_cb: data field $%02X, %i bits\n", 
	    timer_get_time(), data & 0xff, blen ));
     break;
    }

    if ( mc6854->flen < MAX_FRAME_LENGTH )
      mc6854->frame[ mc6854->flen++ ] = data;
    else logerror( "mc6854_tfifo_cb: truncated frame, max=%i\n",
		   MAX_FRAME_LENGTH );

    mc6854_send_bits( data, blen, 1 );
  }
  else {
    /* data underrun => abort */
    logerror( "%f mc6854_tfifo_cb: FIFO underrun\n", timer_get_time() );
    mc6854->sr1 |= TU;
    mc6854->tstate = 0;
    mc6854_send_bits( 0xffff, ABTEX ? 16 : 8, 0 );
    mc6854->flen = 0;
  }

  /* close frame, if needed */
  if ( data & 0x200 ) {
    int len = mc6854->flen;

    LOG(( "%f mc6854_tfifo_cb: end frame\n", timer_get_time() ));
    mc6854_send_bits( 0xdeadbeef, 16, 1 );  /* send check-sum: TODO */
    mc6854_send_bits( FLAG, 8, 0 );         /* send closing flag */
    
    if ( mc6854->tfifo[ FIFO_SIZE - 1 ] & 0x100 ) {
      /* re-open frame asap */
      LOG(( "%f mc6854_tfifo_cb: start frame\n", timer_get_time() ));
      if ( TWOINTER ) mc6854_send_bits( FLAG, 8, 0 );
    }
    else mc6854->tstate = 0;

    mc6854->flen = 0;
    if ( mc6854->iface->out_frame )
      mc6854->iface->out_frame( mc6854->frame, len );
  }
}

static void mc6854_tfifo_clear( void )
{
  memset( mc6854->tfifo, 0, sizeof( mc6854->tfifo ) );
  mc6854->tstate = 0;
  mc6854->flen = 0;
  timer_reset( mc6854->ttimer, TIME_NEVER );
}



/*********************** receive ***********************/

/* MC6854 pushes a field in the FIFO */
static void mc6854_rfifo_push( UINT8 d )
{
  int i, blen = 8;
  unsigned data = d;

  switch ( mc6854->rstate ) {

  case 0:
  case 1:
  case 2: /* 8-bit address field */
    if ( ( data & 1 ) || ( ! AEX ) ) mc6854->rstate = 3;
    else mc6854->rstate = 2;
    LOG(( "%f mc6854_rfifo_push: address field $%02X\n", 
	  timer_get_time(), data ));
    data |= 0x400; /* address marker */
    break;

  case 3: /* 8-bit control field */
    if ( CEX ) mc6854->rstate = 4; 
    else if ( LCF ) mc6854->rstate = 5; else mc6854->rstate = 6;
    LOG(( "%f mc6854_rfifo_push: control field $%02X\n", 
	  timer_get_time(), data ));
    break;
    
  case 4: /* 8-bit extended control field (optional) */
    if ( LCF ) mc6854->rstate = 5; else mc6854->rstate = 6;
    LOG(( "%f mc6854_rfifo_push: control field $%02X\n", 
	  timer_get_time(), data ));
    break;
    
  case 5: /* 8-bit logical control (optional) */
    if ( ! ( data & 0x80 ) ) mc6854->rstate = 6;
    LOG(( "%f mc6854_rfifo_push: logical control field $%02X\n",
	  timer_get_time(), data ));
    break;
    
  case 6: /* variable-length data */
    blen = RWL; 
    data >>= 8 - blen;
    LOG(( "%f mc6854_rfifo_push: data field $%02X, %i bits\n", 
	  timer_get_time(), data, blen ));
    break;
  }

  /* no further FIFO fill until FV is cleared! */
  if ( mc6854->sr2 & FV ) {
    LOG(( "%f mc6854_rfifo_push: field not pushed\n", timer_get_time() ));
    return;
  }

  data |= 0x100; /* entry full marker */
  
  /* push towards the rightmost free entry */
  for ( i = FIFO_SIZE - 1; i >= 0; i-- )
    if ( ! ( mc6854->rfifo[ i ] & 0x100 ) ) break;
  if ( i >= 0 ) mc6854->rfifo[ i ] = data | 0x100;
  else {
    /* FIFO full */
    mc6854->sr2 |= OVRN;
    mc6854->rfifo[ 0 ] = data;
    logerror( "%f mc6854_rfifo_push: FIFO overrun\n", timer_get_time() );
  }

  mc6854->rsize -= blen;
}

static void mc6854_rfifo_terminate( void )
{
  /* mark most recently pushed byte as the last one of the frame */
  int i;
  for ( i = 0; i < FIFO_SIZE; i++ )
    if ( mc6854->rfifo[ i ] & 0x100 ) {
      mc6854->tfifo[ i ] |= 0x200;
      break;
    }

  mc6854->flen = 0;
  mc6854->rstate = 1;
}

/* CPU pops the FIFO */
static UINT8 mc6854_rfifo_pop( void )
{
  int i, data = mc6854->rfifo[ FIFO_SIZE - 1 ];

  /* shift FIFO to the right */
  for ( i = FIFO_SIZE - 1; i > 0; i -- )
    mc6854->rfifo[ i ] = mc6854->rfifo[ i - 1 ];
  mc6854->rfifo[ 0 ] = 0;

  if ( mc6854->rfifo[ FIFO_SIZE - 1 ] & 0x200 ) {
    /* last byte in frame */
    mc6854->sr2 |= FV; /* TODO: check CRC & set ERR instead of FV if error*/
  }

  /* auto-refill in frame mode */
  if ( mc6854->flen > 0 ) {
    mc6854_rfifo_push( mc6854->frame[ mc6854->fpos++ ] );
    if ( mc6854->fpos == mc6854->flen ) mc6854_rfifo_terminate();
  }

  return data;
}


/* MC6854 makes fileds from bits */
void mc6854_set_rx( int bit )
{
  int fieldlen = ( mc6854->rstate < 6 ) ? 8 : RWL;

  if ( RRESET || DCD ) return;

  if ( bit ) {
    mc6854->rones++;
    mc6854->rreg = (mc6854->rreg >> 1) | 0x80000000;
    if ( mc6854->rones >= 8 ) {
      /* abort */
      mc6854->rstate = 0;
      mc6854->rsize = 0; 
      if ( mc6854->rstate > 1 ) { 
	/* only in-frame abort are  */
	mc6854->sr2 |= RABT;
	LOG(( "%f mc6854_receive_bit: abort\n", timer_get_time() ));
      }
    }
    else {
      mc6854->rsize++;
      if ( mc6854->rstate && mc6854->rsize >= fieldlen + 24 )
	mc6854_rfifo_push( mc6854->rreg );
    }
  }
  else if ( mc6854->rones == 5 ) {
    /* discards '0' inserted after 5 '1' */
    mc6854->rones = 0;
    return;
  }
  else if ( mc6854->rones == 6 ) {
    /* flag */
    if ( FDSE ) mc6854->sr1 |= FD;
    if ( mc6854->rstate > 1 ) {
      /* end of frame */
      mc6854->rreg >>= 1;
      mc6854->rsize++;
      if ( mc6854->rsize >= fieldlen + 24 ) /* last field */
	mc6854_rfifo_push( mc6854->rreg );
      mc6854_rfifo_terminate();
      LOG(( "%f mc6854_receive_bit: end of frame\n", timer_get_time() ));
    }
    mc6854->rones = 0;
    mc6854->rstate = 1;
    mc6854->rsize = 0;
  } else {
    mc6854->rones = 0;
    mc6854->rreg >>= 1;
    mc6854->rsize++;
    if ( mc6854->rstate && mc6854->rsize >= fieldlen + 24 )
      mc6854_rfifo_push( mc6854->rreg );
  }
}

static void mc6854_rfifo_clear( void )
{
  memset( mc6854->rfifo, 0, sizeof( mc6854->rfifo ) );
  mc6854->rstate = 0;
  mc6854->rreg = 0;
  mc6854->rsize = 0;
  mc6854->rones = 0;
  mc6854->flen = 0;
}


int mc6854_send_frame( UINT8* data, int len )
{
  if ( mc6854->rstate > 1 || mc6854->tstate > 1 || RTS )return -1; /* busy */
  if ( len > MAX_FRAME_LENGTH ) {
    logerror( "mc6854_send_frame: truncated frame, size=%i, max=%i\n",
	      len, MAX_FRAME_LENGTH );
    len = MAX_FRAME_LENGTH;
  }
  else if ( len < 2 ) {
    logerror( "mc6854_send_frame: frame too short, size=%i, min=2\n", len );
    len = 2;
  }
  memcpy( mc6854->frame, data, len );
  if ( FDSE ) mc6854->sr1 |= FD;
  mc6854->flen = len;
  mc6854->fpos = 0;
  mc6854_rfifo_push( mc6854->frame[ mc6854->fpos++ ] );
  mc6854_rfifo_push( mc6854->frame[ mc6854->fpos++ ] );
  if ( mc6854->fpos == mc6854->flen ) mc6854_rfifo_terminate();
  return 0;
}


/************************** CPU interface ****************************/

void mc6854_set_cts( int data )
{
  if ( ! mc6854->cts && data ) mc6854->sr1 |= CTS;
  mc6854->cts = data;

  if ( mc6854->cts ) mc6854->sr1 |= CTS;
  else mc6854->sr1 &= ~CTS;
}

void mc6854_set_dcd( int data )
{
  if ( ! mc6854->dcd && data ) {
    mc6854->sr2 |= DCD;
    /* partial reset */
    mc6854->rstate = 0;
    mc6854->rreg = 0;
    mc6854->rsize = 0;
    mc6854->rones = 0;
  }
  mc6854->dcd = data;
}

static void mc6854_update_sr2( void )
{
  /* update RDA */
  mc6854->sr2 |= RDA2;
  if ( ! (mc6854->rfifo[ FIFO_SIZE - 1 ] & 0x100) ) mc6854->sr2 &= ~RDA2; 
  else if ( TWOBYTES && ! (mc6854->tfifo[ FIFO_SIZE - 2 ] & 0x100) ) 
    mc6854->sr2 &= ~RDA2;
  
  /* update AP */
  if ( mc6854->rfifo[ FIFO_SIZE - 1 ] & 0x400 ) mc6854->sr2 |= AP; 
  else mc6854->sr2 &= ~AP;
}

static void mc6854_update_sr1( void )
{
  mc6854_update_sr2();

  /* update S2RQ */
  if ( mc6854->sr2 & 0x7f ) mc6854->sr1 |= S2RQ; else mc6854->sr1 &= ~S2RQ; 
  
  /* update TRDA (always prioritized by CTS) */
  if ( TRESET || ( mc6854->sr1 & CTS ) ) mc6854->sr1 &= ~TDRA;
  else {
    mc6854->sr1 |= TDRA;
    if ( mc6854->tfifo[ 0 ] & 0x100 ) mc6854->sr1 &= ~TDRA; 
    else if ( TWOBYTES && (mc6854->tfifo[ 1 ] & 0x100) ) mc6854->sr1 &= ~TDRA;
  }

  /* update RDA */
  if ( mc6854->sr2 & RDA2 ) mc6854->sr1 |= RDA; else mc6854->sr1 &= ~RDA; 
  
  /* update IRQ */
  mc6854->sr1 &= ~IRQ;
  if ( RIE && (mc6854->sr1 & (TU | TDRA) ) ) mc6854->sr1 |= IRQ;
  if ( TIE ) {
    if ( mc6854->sr1 & (S2RQ | RDA | CTS) )  mc6854->sr1 |= IRQ;
    if ( mc6854->sr2 & (ERR | FV | DCD | OVRN | RABT | RIDLE | AP) ) 
      mc6854->sr1 |= IRQ;
  }
}

READ8_HANDLER ( mc6854_r )
{
  switch ( offset ) {

  case 0: /* status register 1 */
    mc6854_update_sr1();
    LOG(( "%f $%04x mc6854_r: get SR1=$%02X (rda=%i,s2rq=%i,fd=%i,cts=%i,tu=%i,tdra=%i,irq=%i)\n", 
	  timer_get_time(), activecpu_get_previouspc(), mc6854->sr1,
	  ( mc6854->sr1 & RDA) ? 1 : 0, ( mc6854->sr1 & S2RQ) ? 1 : 0, 
	  ( mc6854->sr1 & FD ) ? 1 : 0, ( mc6854->sr1 & CTS ) ? 1 : 0,
	  ( mc6854->sr1 & TU ) ? 1 : 0, ( mc6854->sr1 & TDRA) ? 1 : 0,
	  ( mc6854->sr1 & IRQ) ? 1 : 0 ));
    return mc6854->sr1;

  case 1: /* status register 2 */
    mc6854_update_sr2();
    LOG(( "%f $%04x mc6854_r: get SR2=$%02X (ap=%i,fv=%i,ridle=%i,rabt=%i,err=%i,dcd=%i,ovrn=%i,rda2=%i)\n", 
	  timer_get_time(), activecpu_get_previouspc(), mc6854->sr2,
	  ( mc6854->sr2 & AP   ) ? 1 : 0, ( mc6854->sr2 & FV  ) ? 1 : 0, 
	  ( mc6854->sr2 & RIDLE) ? 1 : 0, ( mc6854->sr2 & RABT) ? 1 : 0, 
	  ( mc6854->sr2 & ERR  ) ? 1 : 0, ( mc6854->sr2 & DCD ) ? 1 : 0, 
	  ( mc6854->sr2 & OVRN ) ? 1 : 0, ( mc6854->sr2 & RDA2) ? 1 : 0 ));
    return mc6854->sr2;

  case 2: /* receiver data register */
  case 3:
    {
      UINT8 data = mc6854_rfifo_pop();
      LOG(( "%f $%04x mc6854_r: get data $%02X\n", 
	    timer_get_time(), activecpu_get_previouspc(), data ));
      return data;
    }

  default:
   logerror( "$%04x mc6854 invalid read offset %i\n",
             activecpu_get_previouspc(), offset );
  }
  return 0;
}

WRITE8_HANDLER ( mc6854_w )
{
  switch ( offset ) {

  case 0: /* control register 1 */
    mc6854->cr1 = data;
    LOG(( "%f $%04x mc6854_w: set CR1=$%02X (ac=%i,irq=%c%c,%sreset=%c%c)\n", 
	  timer_get_time(), activecpu_get_previouspc(), mc6854->cr1,
	  AC ? 1 : 0, 
	  RIE ? 'r' : '-', TIE ? 't' : '-',
	  DISCONTINUE ? "discontinue," : "",
	  RRESET ? 'r' : '-', TRESET ? 't' : '-'
	  ));
    if ( mc6854->cr1 & 0xc ) 
      logerror( "$%04x mc6854 DMA not handled (CR1=$%02X)\n", 
		activecpu_get_previouspc(), mc6854->cr1 );
    if ( DISCONTINUE ) {
      /* abort receive FIFO but keeps shift register & synchro */
      mc6854->rstate = 0;
      memset( mc6854->rfifo, 0, sizeof( mc6854->rfifo ) );
    }
    if ( RRESET ) {
      /* abort FIFO & synchro */
      mc6854_rfifo_clear();
      mc6854->sr1 &= ~FD;
      mc6854->sr2 &= ~(AP | FV | RIDLE | RABT | ERR | OVRN | DCD);
      if ( mc6854->dcd ) mc6854->sr2 |= DCD;
    }
    if ( TRESET ) {
      mc6854_tfifo_clear();
      mc6854->sr1 &= ~(TU | TDRA | CTS);
      if ( mc6854->cts ) mc6854->sr1 |= CTS;
    }
    break;

  case 1:
    if ( AC ) {
      /* control register 3 */
      mc6854->cr3 = data;
      LOG(( "%f $%04x mc6854_w: set CR3=$%02X (lcf=%i,aex=%i,idl=%i,fdse=%i,loop=%i,tst=%i,dtr=%i)\n", 
	    timer_get_time(), activecpu_get_previouspc(), mc6854->cr3,
	    LCF ? (CEX ? 16 : 8) : 0,  AEX ? 1 : 0,
	    IDL0 ? 0 : 1, FDSE ? 1 : 0, LOOP ? 1 : 0,
	    TST ? 1 : 0, DTR ? 1 : 0
	    ));
      if ( LOOP )
	logerror( "$%04x mc6854 loop mode not handled (CR3=$%02X)\n",
		  activecpu_get_previouspc(), mc6854->cr3 );
      if ( TST )
	logerror( "$%04x mc6854 test mode not handled (CR3=$%02X)\n",
		  activecpu_get_previouspc(), mc6854->cr3 );

      if ( mc6854->iface->out_dtr )
	mc6854->iface->out_dtr( DTR ? 1 : 0 );

    }
    else {
      /* control register 2 */
      mc6854->cr2 = data;
      LOG(( "%f $%04x mc6854_w: set CR2=$%02X (pse=%i,bytes=%i,fmidle=%i,%s,tlast=%i,clr=%c%c,rts=%i)\n", 
	    timer_get_time(), activecpu_get_previouspc(), mc6854->cr2,
	    PSE ? 1 : 0,  TWOBYTES ? 2 : 1,  FMIDLE ? 1 : 0, 
	    FCTDRA ? "fc" : "tdra", TLAST ? 1 : 0,
	    data & 0x20 ? 'r' : '-',  data & 0x40 ? 't' : '-',
	    RTS ? 1 : 0 ));
      if ( PSE )
  	logerror( "$%04x mc6854 status prioritization not handled (CR2=$%02X)\n",
		  activecpu_get_previouspc(), mc6854->cr2 );
      if ( TLAST ) mc6854_tfifo_terminate();
      if ( data & 0x20 ) {
	/* clear receiver status */
	mc6854->sr1 &= ~FD;
	mc6854->sr2 &= ~(AP | FV | RIDLE | RABT | ERR | OVRN | DCD);
	if ( mc6854->dcd ) mc6854->sr2 |= DCD;
     }
      if ( data & 0x40 ) {
	/* clear transmitter status */
	mc6854->sr1 &= ~(TU | TDRA | CTS);
	if ( mc6854->cts ) mc6854->sr1 |= CTS;
      }

      if ( mc6854->iface->out_rts ) mc6854->iface->out_rts( RTS ? 1 : 0 );
    }
    break;

  case 2: /* transmitter data: continue data */
    LOG(( "%f $%04x mc6854_w: push data=$%02X\n", 
	  timer_get_time(), activecpu_get_previouspc(), data ));
    mc6854_tfifo_push( data );
    break;

  case 3:
    if ( AC ) {
      /* control register 4 */
      mc6854->cr4 = data;
      LOG(( "%f $%04x mc6854_w: set CR4=$%02X (interframe=%i,tlen=%i,rlen=%i,%s%s)\n", 
	    timer_get_time(), activecpu_get_previouspc(), mc6854->cr4,
	    TWOINTER ? 2 : 1, 
	    TWL, RWL, 
	    ABT ? ( ABTEX ? "abort-ext," : "abort,") : "",
	    NRZ ? "nrz" : "nrzi" ));
      if ( ABT ) { 
	mc6854->tstate = 0;
	mc6854_send_bits( 0xffff, ABTEX ? 16 : 8, 0 );
	mc6854->flen = 0;
      }
    }
    else {
      /* transmitter data: last data */
      LOG(( "%f $%04x mc6854_w: push last-data=$%02X\n", 
	    timer_get_time(), activecpu_get_previouspc(), data ));
      mc6854_tfifo_push( data );
      mc6854_tfifo_terminate();
    }
    break;
 
  default:
    logerror( "$%04x mc6854 invalid write offset %i (data=$%02X)\n",
              activecpu_get_previouspc(), offset, data );
  }
}


/************************ reset *****************************/

void mc6854_reset ( void )
{
  LOG (( "mc6854 reset\n" ));
  mc6854->cr1 = 0xc0; /* reset condition */
  mc6854->cr2 = 0;
  mc6854->cr3 = 0;
  mc6854->cr4 = 0;
  mc6854->sr1 = 0;
  mc6854->sr2 = 0;
  mc6854->cts = 0;
  mc6854->dcd = 0;
  mc6854_tfifo_clear();
  mc6854_rfifo_clear();
}


/************************** configuration ****************************/

void mc6854_config ( const mc6854_interface* iface )
{
  assert( iface );
  mc6854 = auto_malloc( sizeof( * mc6854 ) );
  assert( mc6854 );
  mc6854->iface = iface;
  mc6854->ttimer = mame_timer_alloc( mc6854_tfifo_cb );
  state_save_register_item( "mc6854", 0, mc6854->cr1 );
  state_save_register_item( "mc6854", 0, mc6854->cr2 );
  state_save_register_item( "mc6854", 0, mc6854->cr3 );
  state_save_register_item( "mc6854", 0, mc6854->cr4 );
  state_save_register_item( "mc6854", 0, mc6854->sr1 );
  state_save_register_item( "mc6854", 0, mc6854->sr2 );
  state_save_register_item( "mc6854", 0, mc6854->cts );
  state_save_register_item( "mc6854", 0, mc6854->dcd );
  state_save_register_item( "mc6854", 0, mc6854->tstate );
  state_save_register_item_array( "mc6854", 0, mc6854->tfifo );
  state_save_register_item( "mc6854", 0, mc6854->tones );
  state_save_register_item( "mc6854", 0, mc6854->rstate );
  state_save_register_item( "mc6854", 0, mc6854->rreg );
  state_save_register_item( "mc6854", 0, mc6854->rones );
  state_save_register_item( "mc6854", 0, mc6854->rsize );
  state_save_register_item_array( "mc6854", 0, mc6854->rfifo );
  state_save_register_item_array( "mc6854", 0, mc6854->frame );
  state_save_register_item( "mc6854", 0, mc6854->flen );
  state_save_register_item( "mc6854", 0, mc6854->fpos );
  mc6854_reset();
}
