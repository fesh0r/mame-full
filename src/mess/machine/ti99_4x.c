/*
  Machine code for TI-99/4A.
  Raphael Nabet, 1999.
  Some code derived from Ed Swartz's V9T9.

  References :
  * The TI-99/4A Tech Pages <http://www.nouspikel.com/ti99/titech.htm>.  Great site.

Emulated :
  * All TI99 basic console hardware, except tape input, and a few tricks in TMS9901 emulation.
  * Cartidge with ROM (either non-paged or paged) and GROM (no GRAM or extra GPL ports).
  * Speech Synthesizer, with standard speech ROM (no speech ROM expansion).
  * Disk emulation (incomplete, but seems to work OK).

  Compatibility looks quite good.

New (9911) :
  * updated for 16 bit handlers.
  * added some timing (OK, it's a trick...).
New (991202) :
  * fixed the CRU base for disk
New (991206) :
  * "Juergenified" the timer code
New (991210) :
  * "Juergenified" code to use region accessor
*/

#include "driver.h"
#include "wd179x.h"
#include "mess/vidhrdw/tms9928a.h"
#include <math.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

unsigned char *ROM;

extern WD179X *wd[];
extern int tms9900_ICount;


static void tms9901_set_int2(int state);
static void tms9901_init(void);
static void tms9901_cleanup(void);

/*================================================================
  General purpose code :
  initialization, cart loading, etc.
================================================================*/

static unsigned char *cartidge_pages[2] = {NULL, NULL};
static int cartidge_paged;
static int current_page_number;
static unsigned char *current_page_ptr;

/*
  Load ROM.  All files are in raw binary format.
  1st ROM : GROM (up to 40kb)
  2nd ROM : CPU ROM (8kb)
  3rd ROM : CPU ROM, 2nd page (8kb)
*/
int ti99_load_rom (void)
{
  FILE *cartfile;
  ROM = memory_region(REGION_CPU1);

  cartidge_paged = FALSE;

  if (cartidge_pages[0])
  {
    free(cartidge_pages[0]);
    cartidge_pages[0] = NULL;
  }

  if (cartidge_pages[1])
  {
    free(cartidge_pages[1]);
    cartidge_pages[1] = NULL;
  }

  current_page_ptr = ROM + 0x6000;

  if (strlen(rom_name[0]))
  {
    cartfile = osd_fopen (Machine->gamedrv->name, rom_name[0], OSD_FILETYPE_IMAGE_R, 0);
    if (cartfile == NULL)
    {
      if (errorlog)
        fprintf(errorlog,"TI99 - Unable to locate cartridge: %s\n",rom_name[0]);
      return 1;
    }

    osd_fread (cartfile, memory_region(REGION_GFX1) + 0x6000, 0xA000);
    osd_fclose (cartfile);
  }

  if (strlen(rom_name[1]))
  {
    cartfile = osd_fopen (Machine->gamedrv->name, rom_name[1], OSD_FILETYPE_IMAGE_R, 0);
    if (cartfile == NULL)
    {
      if (errorlog)
        fprintf(errorlog,"TI99 - Unable to locate cartridge: %s\n",rom_name[1]);
      return 1;
    }

    osd_fread_msbfirst (cartfile, ROM + 0x6000, 0x2000);
    osd_fclose (cartfile);
  }

  if (strlen(rom_name[2]))
  {
    cartidge_paged = TRUE;
    cartidge_pages[0] = malloc(0x2000);
    cartidge_pages[1] = malloc(0x2000);
    current_page_number = 0;
    memcpy(cartidge_pages[0], ROM + 0x6000, 0x2000); /* save first page */
    current_page_ptr = cartidge_pages[current_page_number];

    cartfile = osd_fopen (Machine->gamedrv->name, rom_name[2], OSD_FILETYPE_IMAGE_R, 0);
    if (cartfile == NULL)
    {
      if (errorlog)
        fprintf(errorlog,"TI99 - Unable to locate cartridge: %s\n",rom_name[2]);
      return 1;
    }

    osd_fread_msbfirst (cartfile, cartidge_pages[1], 0x2000);
    osd_fclose (cartfile);
  }

  return 0;
}

/*
  ti99_init_machine() ; launched AFTER ti99_load_rom...
*/
void ti99_init_machine(void)
{
  int i;

  /* callback for the TMS9901 to be notified of changes to the TMS9928A INT* line
     (connected to the TMS9901 INT2* line) */
  TMS9928A_int_callback(tms9901_set_int2);

  wd179x_init(1); /* initialize the floppy disk controller */
  /* we set the thing to single density by hand */
  for (i=0; i<2; i++)
  {
    wd179x_set_geometry(i, 40, 1, 9, 256, 0, 0);
    wd[i]->density = DEN_FM_LO;
  }

  tms9901_init();
}

void ti99_stop_machine(void)
{
  if (cartidge_pages[0])
  {
    free(cartidge_pages[0]);
    cartidge_pages[0] = NULL;
  }

  if (cartidge_pages[1])
  {
    free(cartidge_pages[1]);
    cartidge_pages[1] = NULL;
  }

   wd179x_stop_drive();
}


/*
  ROM identification : currently does nothing.

  Actually, TI ROM header starts with a >AA.  Unfortunately, header-less ROMs do exist.
*/
int ti99_id_rom (const char *name, const char *gamename)
{
  return 1;
}

/*
  video initialization.
*/
int ti99_vh_start(void)
{
  return TMS9928A_start(0x4000);  /* 16 kb of video RAM */
}

/*
  VBL interrupt  (mmm... actually, it happens when the beam enters the lower border, so it is not
  a genuine VBI, but who cares ?)
*/
int ti99_vblank_interrupt(void)
{
  TMS9928A_interrupt();

  return ignore_interrupt();
}



/*================================================================
  Memory handlers.

  TODO :
  * GRAM support
  * GPL paging support
  * DSR support
  * minimem support

================================================================*/

/*
  Same as MRA_NOP, but with an additionnal delay.
*/
int ti99_rw_null8bits(int offset)
{
  tms9900_ICount -= 4;

  return(0);
}

void ti99_ww_null8bits(int offset, int data)
{
  tms9900_ICount -= 4;
}

/*
  Memory extension : same as MRA_RAM, but with an additionnal delay.
*/
/* low 8 kb : 0x2000-0x3fff */
int ti99_rw_xramlow(int offset)
{
  tms9900_ICount -= 4;

  return READ_WORD(& ROM[0x2000 + offset]);
}

void ti99_ww_xramlow(int offset, int data)
{
  tms9900_ICount -= 4;

  WRITE_WORD(& ROM[0x2000 + offset], data | (READ_WORD(& ROM[0x2000 + offset]) & (data >> 16)));
}

/* high 24 kb : 0xa000-0xffff */
int ti99_rw_xramhigh(int offset)
{
  tms9900_ICount -= 4;

  return READ_WORD(& ROM[0xA000 + offset]);
}

void ti99_ww_xramhigh(int offset, int data)
{
  tms9900_ICount -= 4;

  WRITE_WORD(& ROM[0xA000 + offset], data | (READ_WORD(& ROM[0xA000 + offset]) & (data >> 16)));
}

/*
  Cartidge read : same as MRA_ROM, but with an additionnal delay.
*/
int ti99_rw_cartmem(int offset)
{
  tms9900_ICount -= 4;

  return READ_WORD(current_page_ptr + offset);
}

/*
  this handler handles ROM switching in cartidges
*/
void ti99_ww_cartmem(int offset, int data)
{
  tms9900_ICount -= 4;

  if (cartidge_paged)
  { /* if cartidge is paged */
    int new_page = (offset >> 1) & 1; /* new page number */

    if (current_page_number != new_page)
    { /* if page number changed */
      current_page_number = new_page;

      current_page_ptr = cartidge_pages[current_page_number];
    }
  }
}

/*----------------------------------------------------------------
   Scratch RAM PAD.
   0x8000-8300 are the same as 0x8300-8400. (only 256 bytes installed)
----------------------------------------------------------------*/

/*
  PAD read
*/
int ti99_rw_scratchpad(int offset)
{
  return READ_WORD(& ROM[0x8300 | offset]);
}

/*
  PAD write
*/
void ti99_ww_scratchpad(int offset, int data)
{
  if (! (data & 0xff000000))
    ROM[0x8300 | offset] = (data >> 8) & 0xff;
  if (! (data & 0x00ff0000))
    ROM[0x8300 | offset] = data & 0xff;
}

/*----------------------------------------------------------------
   memory-mapped registers handlers.
----------------------------------------------------------------*/

/*
  About memory-mapped registers.

  These are registers to communicate with several peripherals.
  These registers are all 8 bit wide, and are located at even adresses between
  0x8400 and 0x9FFE.
  The registers are identified by (addr & 1C00), and, for VDP and GPL access, by (addr & 2).
  These registers are either read-only or write-only.  (Actually, (addr & 4000) can
  be regarded as some sort of a R/W line.)

  Memory mapped registers list:
  - write sound chip. (8400-87FE)
  - read VDP memory. (8800-8BFE), (addr&2) == 0
  - read VDP status. (8800-8BFE), (addr&2) == 2
  - write VDP memory. (8C00-8FFE), (addr&2) == 0
  - write VDP address and VDP register. (8C00-8FFE), (addr&2) == 2
  - read speech synthesis chip. (9000-93FE)
  - write speech synthesis chip. (9400-97FE)
  - read GPL memory. (9800-9BFE), (addr&2) == 0 (1)
  - read GPL adress. (9800-9BFE), (addr&2) == 2 (1)
  - write GPL memory. (9C00-9FFE), (addr&2) == 0 (1)
  - write GPL adress. (9C00-9FFE), (addr&2) == 2 (1)

(1) on some hardware, (addr & 0x3C) provides a GPL page number.
*/

/*
  TMS9919 sound chip write
*/
void ti99_ww_wsnd(int offset, int data)
{
  tms9900_ICount -= 4;

  SN76496_0_w(offset, (data >> 8) & 0xff);
}

/*
  TMS9918A VDP read
*/
int ti99_rw_rvdp(int offset)
{
  tms9900_ICount -= 4;

  if (offset & 2)
  { /* read VDP status */
    return(TMS9928A_register_r() << 8);
  }
  else
  { /* read VDP RAM */
    return(TMS9928A_vram_r() << 8);
  }
}

/*
  TMS9918A vdp write
*/
void ti99_ww_wvdp(int offset, int data)
{
  tms9900_ICount -= 4;

  if (offset & 2)
  { /* write VDP adress */
    TMS9928A_register_w((data >> 8) & 0xff);
  }
  else
  { /* write VDP data */
    TMS9928A_vram_w((data >> 8) & 0xff);
  }
}

/*
  TMS5200 speech chip read
*/
int ti99_rw_rspeech(int offset)
{
  tms9900_ICount -= 4;  /* much more, actually */

  return tms5220_status_r(offset) << 8;
}

/*
  TMS5200 speech chip write
*/
void ti99_ww_wspeech(int offset, int data)
{
  tms9900_ICount -= 4;  /* much more, actually */

  tms5220_data_w(offset, (data >> 8) & 0xff);
}

/* current position in the gpl memory space */
static int gpl_addr = 0;

/*
  GPL read
*/
int ti99_rw_rgpl(int offset)
{
  tms9900_ICount -= 4;

  /*int page = (offset & 0x3C) >> 2;*/  /* GROM/GRAM can be paged */

  if (offset & 2)
  { /* read GPL adress */
    int value;
    /* increment gpl_addr (GPL wraps in 8k segments!) : */
    value = ((gpl_addr + 1) & 0x1FFF) | (gpl_addr & 0xE000);
    gpl_addr = (value & 0xFF) << 8; /* gpl_addr is shifted left by 8 bits */
    return value & 0xFF00;          /* and we retreive the MSB */
    /* to get full GPL adress, we make two byte reads! */
  }
  else
  { /* read GPL data */
    int value = memory_region(REGION_GFX1)[gpl_addr /*+ ((long) page << 16)*/];  /* retreive byte */
    /* increment gpl_addr (GPL wraps in 8k segments!) : */
    gpl_addr = ((gpl_addr + 1) & 0x1FFF) | (gpl_addr & 0xE000);
    return(value << 8);
  }
}

/*
  GPL write
*/
void ti99_ww_wgpl(int offset, int data)
{
  tms9900_ICount -= 4;

  data = (data >> 8) & 0xff;

  /*int page = (offset & 0x3C) >> 2;*/  /* GROM/GRAM can be paged */

  if (offset & 2)
  { /* write GPL adress */
    gpl_addr = ((gpl_addr & 0xFF) << 8) | (data & 0xFF);
  }
  else
  { /* write GPL byte */
#if 0
    /* Disabled because we don't know whether we have RAM or ROM. */
    /* GRAM are quite uncommon, anyway. */
    if ((gpl_addr >= gram_start) && (gpl_addr <= gram_end))
      gplseg[gpl_addr /*+ ((long) page << 16)*/] = value;
#endif
    /* increment gpl_addr (GPL wraps in 8k segments!) : */
    gpl_addr = ((gpl_addr + 1) & 0x1FFF) | (gpl_addr & 0xE000);
  }
}

/*================================================================
  TMS9901 emulation.

  TMS9901 handles interrupts, provides several I/O pins, and a timer (a.k.a. clock : it is
  merely a register which decrements regularily and can generate an interrupt when it reaches
  0).

  It communicates with the TMS9900 with a CRU interface, and with the rest of the world with
  a number of parallel I/O pins.

  TODO :
  * complete TMS9901 emulation (e.g. other interrupts pins, pin mirroring...)
  * make emulation more general (most TMS9900-based systems used a TMS9901, so it could be
    interesting for other drivers)
  * support for tape input, possibly improved tape output.

  KNOWN PROBLEMS :
  * in a real TI99/4A, TMS9901 is mirorred in the whole first half of CRU address space
    (i.e. CRU bit 0 = CRU bit 32 = bit 64 = ... = bit 504 ).  This will be emulated later...
  * a read or write to bits 16-31 causes TMS9901 to quit timer mode.  The problem is :
    on a TI99/4A, any memory access causes a dummy CRU read.  Therefore, TMS9901 can quit
    timer mode even thought the program did not explicitely ask... and THIS is impossible
    to emulate efficiently (we'd have to check each memory operation).
================================================================*/

/*
  TMS9901 interrupt handling on a TI99/4x.

  Three interrupts are used by the system (INT1, INT2, and timer), out of 15/16 possible
  interrupt.  Keyboard pins can be used as interrupt pins, too, but this is not emulated
  (it's a trick, anyway, and I don't know of any program which uses it).

  When an interrupt line is set (and the corresponding bit in the interrupt mask is set),
  a level 1 interrupt is requested from the TMS9900.  This interrupt request lasts as long as
  the interrupt pin and the revelant bit in the interrupt mask are set.

  TIMER interrupts are kind of an exception, since they are not associated with an external
  interrupt pin, and I guess it takes a write to the 9901 CRU bit 3 ("SBO 3") to clear
  the pending interrupt (or am I wrong once again ?).

nota :
  All interrupt routines notify (by software) the TMS9901 of interrupt recognition ("SBO n").
  However, AFAIK, this has strictly no consequence in the TMS9901, and interrupt routines
  would work fine without this, (except probably TIMER interrupt).  All this is quite weird.
  Maybe the interrupt recognition notification is needed on TMS9985, or any other weird
  variant of TMS9900 (does any TI990/10 owner wants to test this :-) ?).
*/

/* Masks for the three interrupts used on the TI99. */
#define M_INT1 1    /* external interrupt (used by RS232 controller, for instance) */
#define M_INT2 2    /* VDP interrupt */
#define M_INT3 4    /* timer interrupt */

static int int_state = 0;     /* state of the int1-int15 lines */
static int timer_int_pending = 0; /* timer int pending (overrides int3 pin if timer enabled) */
static int enabled_ints = 0;  /* interrupt enable mask */

static int int_pending = 0; /* status of int* pin (connected to TMS9900) */

/*
  TMS9901 mode bit.

  0 = I/O mode (allow interrupts ???),
  1 = Timer mode (we're programming the clock).
*/
static int mode9901 = 0;

/*
  clock registers

  frequency : CPUCLK/64 = 46875Hz
*/

/* MESS timer, used to emulate the decrementer register */
static void *tms9901_timer = NULL;

/* clock interval, loaded in decrementer when it reaches 0.  0 means decrementer off */
static int clockinvl = 0;

/* when we go to timer mode, the decrementer is copied there to allow to read it reliably */
static int latchedtimer;

/* set to true when clockinvl has been written to, which means the decrementer must be reloaded
when we quit timer mode (??) */
static int clockinvlchanged;

static void tms9901_field_interrupts(void);
static void reset_tms9901_timer(void);

static void tms9901_init(void)
{
  int_state = 0;
  timer_int_pending = 0;
  enabled_ints = 0;

  tms9901_field_interrupts();

  mode9901 = 0;

  clockinvl = 0;
  reset_tms9901_timer();
}


static void tms9901_cleanup(void)
{
  if (tms9901_timer)
  {
    timer_remove(tms9901_timer);
    tms9901_timer = NULL;
  }
}

/*
  should be called after any change to int_state or enabled_ints.
*/
static void tms9901_field_interrupts(void)
{
  int current_ints;

  /* int_state : state of lines int1-int15 */
  /* enabled_ints : enabled interrupts */
  current_ints = int_state;
  if (clockinvl != 0)
  { /* if timer is enabled, INT3 pin is overriden by timer */
    if (timer_int_pending)
      current_ints |= M_INT3;
    else
      current_ints &= ~ M_INT3;
  }

  current_ints &= enabled_ints;

  if (current_ints)
  {
    int level;

    /* find which interrupt tripped us :
       we simply look for the first bit set to 1 in current_ints... */
    /*level = 1;
    while (! (current_ints & 1))
    {
      current_ints >>= 1;  // try next bit
      level++;
    }*/

    /* On TI99, TMS9900 IC0-3 lines are not connected to TMS9901,
    but hard-wired to force level 1 interrupts */
    level = 1;

    int_pending = TRUE;

    cpu_0_irq_line_vector_w(0, level);
    cpu_set_irq_line(0, 0, ASSERT_LINE);  /* interrupt it, baby */
  }
  else
  {
    int_pending = FALSE;

    cpu_set_irq_line(0, 0, CLEAR_LINE);
  }
}

/*
  callback function which is called when the state of INT2* change

  state == 0 : INT2* is inactive (high)
  state != 0 : INT2* is active (low)
*/
static void tms9901_set_int2(int state)
{
  if (state)
    int_state |= M_INT2;    /* VDP interrupt requested */
  else
    int_state &= ~ M_INT2;

  tms9901_field_interrupts();
}

/*
  this call-back is called by MESS timer system when the decrementer reaches 0.
*/
static void decrementer_callback(int ignored)
{
  timer_int_pending = TRUE; /* decrementer interrupt requested */

  tms9901_field_interrupts();
}

/*
  load the content of clockinvl into the decrementer
*/
static void reset_tms9901_timer(void)
{
  if (tms9901_timer)
  {
    timer_remove(tms9901_timer);
    tms9901_timer = NULL;
  }

  /* clock interval == 0 -> no timer */
  if (clockinvl)
  {
    tms9901_timer = timer_pulse(clockinvl / 46875., 0, decrementer_callback);
  }
}

/*================================================================
  CRU handlers.

  The first handlers are TMS9901 related

  BTW, although TMS9900 is generally big-endian, it is little endian as far as CRU is
  concerned. (ie bit 0 is the least significant)
================================================================*/

/*----------------------------------------------------------------
  TMS9901 CRU interface.
----------------------------------------------------------------*/

/* keyboard interface */
static int KeyCol = 0;
static int AlphaLockLine = 0;

/*
  Read first 8 bits of TI99's 9901.

  signification :
  bit 0 : mode9901
  if (mode9901 == 0)
   bit 1 : INT1 status
   bit 2 : INT2 status
   bit 3-7 : keyboard status bits 0 to 4
  else
   bit 1-7 : current timer value, bits 0 to 6
*/
int ti99_R9901_0(int offset)
{

  if (mode9901)
  { /* timer mode */
    return ((latchedtimer & 0x7F) << 1) | 0x01;
  }
  else
  { /* I/O mode */
    int answer = 0;

    if (! (int_state & M_INT1))
      answer |= 0x02;

    if (! (int_state & M_INT2))
      answer |= 0x04;

    answer |= (readinputport(KeyCol) << 3) & 0xF8;

    if (! AlphaLockLine)
      answer &= ~ (input_port_8_r(0) << 3);

    return(answer);
  }
}


/*
  Read last 8 bits of TI99's 9901.

  if (mode9901 == 0)
   bit 0-2 : keyboard status bits 5 to 7
   bit 3-7 : weird, not emulated
  else
   bit 0-6 : current timer value, bits 7 to 14
   bit 7 : value of the INTREQ* (interrupt request to TMS9900) pin.
*/
int ti99_R9901_1(int offset)
{
  int answer;

  if (mode9901)
  { /* timer mode */
    answer = (latchedtimer & 0x3F80) >> 7;
    if (! int_pending)
      answer |= 0x80;
  }
  else
  { /* I/O mode */
    answer = (readinputport(KeyCol) >> 5) & 0x07;

  }
  return answer;
}

/*
  write to mode bit
*/
void ti99_W9901_0(int offset, int data)
{
  if (mode9901 != (data & 1))
  {
    mode9901 = (data & 1);

    if (mode9901)
    {
      if (tms9901_timer)
        latchedtimer = ceil(timer_timeleft(tms9901_timer) * 46875.);
      else
        latchedtimer = 0; /* timer inactive... */
      clockinvlchanged = FALSE;
    }
    else
    {
      if (clockinvlchanged)
        reset_tms9901_timer();
    }
  }
}

/*
  write one bit to 9901 (bits 1-14)

  mode9901==0 ?  Disable/Enable an interrupt
              :  Bit in clock interval
*/
void ti99_W9901_S(int offset, int data)
{
  /* offset is the index of the modified bit of register (-> interrupt number -1) */
  int mask = 1 << offset; /* corresponding mask */

  if (mode9901)
  { /* modify clock interval */
    if (data & 1)
      clockinvl |= mask;      /* set bit */
    else
      clockinvl &= ~ mask;    /* unset bit */
    clockinvlchanged = TRUE;
  }
  else
  { /* modify interrupt mask */
    if (data & 1)
      enabled_ints |= mask;     /* set bit */
    else
      enabled_ints &= ~ mask;   /* unset bit */

    if (offset == 2)
      timer_int_pending = FALSE;  /* SBO 3 clears pending timer interrupt (??) */

    tms9901_field_interrupts();   /* changed interrupt state */
  }
}

/*
  write bit 15 of TMS9901
*/
void ti99_W9901_F(int offset, int data)
{
  if (mode9901)
  { /* clock mode */
    if (! (data & 1))
    { /* TMS9901 soft reset : not emulated*/


    }
  }
  else
  { /* modify interrupt mask */
    if (data & 1)
      enabled_ints |= 0x4000;     /* set bit */
    else
      enabled_ints &= ~ 0x4000;   /* unset bit */

    tms9901_field_interrupts();   /* changed interrupt state */
  }
}


/*
  Read cru bits 16-23
*/
int ti99_R9901_2(int offset)
{
  int answer;

  if (mode9901)
  { /* exit timer mode */
    mode9901 = 0;

    if (clockinvlchanged)
      reset_tms9901_timer();
  }

  answer = KeyCol << 2;

  if (! AlphaLockLine)
    answer |= 0x20;

  return answer;
}

/*
  Read cru bits 24-31
*/
int ti99_R9901_3(int offset)
{
  /*only important bit : bit 26 : tape input */

  if (mode9901)
  { /* exit timer mode */
    mode9901 = 0;

    if (clockinvlchanged)
      reset_tms9901_timer();
  }

  /*return 8;*/
  return 0;
}

/*
  WRITE column number bit 2
*/
void ti99_KeyC2(int offset, int data)
{
  if (mode9901)
  { /* exit timer mode */
    mode9901 = 0;

    if (clockinvlchanged)
      reset_tms9901_timer();
  }

  if (data & 1)
    KeyCol |= 1;
  else
    KeyCol &= (~ 1);
}

/*
  WRITE column number bit 1
*/
void ti99_KeyC1(int offset, int data)
{
  if (mode9901)
  { /* exit timer mode */
    mode9901 = 0;

    if (clockinvlchanged)
      reset_tms9901_timer();
  }

  if (data & 1)
    KeyCol |= 2;
  else
    KeyCol &= (~ 2);
}

/*
  WRITE column number bit 0
*/
void ti99_KeyC0(int offset, int data)
{
  if (mode9901)
  { /* exit timer mode */
    mode9901 = 0;

    if (clockinvlchanged)
      reset_tms9901_timer();
  }

  if (data & 1)
    KeyCol |= 4;
  else
    KeyCol &= (~ 4);
}

/*
  WRITE alpha lock line
*/
void ti99_AlphaW(int offset, int data)
{
  if (mode9901)
  { /* exit timer mode */
    mode9901 = 0;

    if (clockinvlchanged)
      reset_tms9901_timer();
  }

  AlphaLockLine = (data &1);
}

/*
  command CS1 tape unit motor - not emulated
*/
void ti99_CS1_motor(int offset, int data)
{
  if (mode9901)
  { /* exit timer mode */
    mode9901 = 0;

    if (clockinvlchanged)
      reset_tms9901_timer();
  }


}

/*
  command CS2 tape unit motor - not emulated
*/
void ti99_CS2_motor(int offset, int data)
{
  if (mode9901)
  { /* exit timer mode */
    mode9901 = 0;

    if (clockinvlchanged)
      reset_tms9901_timer();
  }


}

/*
  audio gate

  connected to the AUDIO IN pin of TMS9919

  set to 1 before using tape (in order not to burn the TMS9901 ??)

  I am not sure about polarity.
*/
void ti99_audio_gate(int offset, int data)
{
  if (mode9901)
  { /* exit timer mode */
    mode9901 = 0;

    if (clockinvlchanged)
      reset_tms9901_timer();
  }

  if (data & 1)
    DAC_data_w(1, 0xFF);
  else
    DAC_data_w(1, 0);
}

/*
  tape output
  I am not sure about polarity.
*/
void ti99_CS_output(int offset, int data)
{
  if (mode9901)
  { /* exit timer mode */
    mode9901 = 0;

    if (clockinvlchanged)
      reset_tms9901_timer();
  }

  if (data & 1)
    DAC_data_w(0, 0xFF);
  else
    DAC_data_w(0, 0);
}


/*----------------------------------------------------------------
  disk DSR handlers
----------------------------------------------------------------*/

/* TRUE when the disk DSR is active */
int diskromon = 0;

/*
  read a byte in disk DSR.
*/
int ti99_rw_disk(int offset)
{
  tms9900_ICount -= 4;

  if (! diskromon)
    return 0;

  switch (offset)
  {
  case 0x1FF0:  /* Status register */
    return (wd179x_status_r(offset) ^ 0xFF) << 8;
    break;
  case 0x1FF2:  /* Track register */
    return (wd179x_track_r(offset) ^ 0xFF) << 8;
    break;
  case 0x1FF4:  /* Sector register */
    return (wd179x_sector_r(offset) ^ 0xFF) << 8;
    break;
  case 0x1FF6:  /* Data register */
    return (wd179x_data_r(offset) ^ 0xFF) << 8;
    break;
  default:
    return READ_WORD(& ROM[0x4000 + offset]);
    break;
  }
}

/*
  write a byte in disk DSR.
*/
void ti99_ww_disk(int offset, int data)
{
  tms9900_ICount -= 4;

  if (! diskromon)
    return;

  data = ((data >> 8) & 0xFF) ^ 0xFF; /* inverted data bus */

  switch (offset)
  {
  case 0x1FF8:  /* Command register */
    wd179x_command_w(offset, data);
    break;
  case 0x1FFA:  /* Track register */
    wd179x_track_w(offset, data);
    break;
  case 0x1FFC:  /* Sector register */
    wd179x_sector_w(offset, data);
    break;
  case 0x1FFE:  /* Data register */
    wd179x_data_w(offset, data);
    break;
  }
}

/*----------------------------------------------------------------
  disk CRU interface.
----------------------------------------------------------------*/

/*
  Read disk CRU interface

  bit 0 : HLD pin
  bit 1-3 : drive n selected (setting the bit means it is absent ???)
  bit 4 : 0: motor strobe on
  bit 5 : always 0
  bit 6 : always 1
  bit 7 : selected side
*/
int ti99_DSKget(int offset)
{
  return(0x40);
}

/*
  WRITE to DISK DSR ROM bit (bit 0)
*/
void ti99_DSKROM(int offset, int data)
{
  if (data & 1)
  {
    diskromon = 1;
  }
  else
  {
    diskromon = 0;
  }
}



static int DSKhold;

static int DRQ_IRQ_status;

#define fdc_IRQ 1
#define fdc_DRQ 2

#define HALT   ASSERT_LINE
#define RESUME CLEAR_LINE

static void handle_hold(void)
{
  if (DSKhold && (! DRQ_IRQ_status))
	cpu_set_halt_line(1, ASSERT_LINE);
  else
	cpu_set_halt_line(1, CLEAR_LINE);
}

static void ti99_fdc_callback(int event)
{
  switch (event)
  {
  case WD179X_IRQ_CLR:
    DRQ_IRQ_status &= ~fdc_IRQ;
    break;
  case WD179X_IRQ_SET:
    DRQ_IRQ_status |= fdc_IRQ;
    break;
  case WD179X_DRQ_CLR:
    DRQ_IRQ_status &= ~fdc_DRQ;
    break;
  case WD179X_DRQ_SET:
    DRQ_IRQ_status |= fdc_DRQ;
    break;
  }

  handle_hold();
}

/*
  Set disk ready/hold (bit 2)
  0 : ignore IRQ and DRQ
  1 : TMS9900 is stopped until IRQ or DRQ are set (OR the motor stops rotating - rotates for
      4.23s after write to revelant CRU bit, this is not emulated and could cause the TI99
      to lock...)
*/
void ti99_DSKhold(int offset, int data)
{
  DSKhold = data & 1;

  handle_hold();
}


/*
  Load disk heads (HLT pin) (bit 3)
*/
void ti99_DSKheads(int offset, int data)
{
}




static int DSKnum = -1;
static int DSKside = 0;

/*
  Select drive X (bits 4-6)
*/
void ti99_DSKsel(int offset, int data)
{
  int drive = offset; /* drive # (0-2) */

  if (data & 1)
  {
    if (drive != DSKnum)  /* turn on drive... already on ? */
    {
      DSKnum = drive;
      wd179x_select_drive(DSKnum, DSKside, ti99_fdc_callback, floppy_name[DSKnum]);
    }
  }
  else
  {
    if (drive == DSKnum)  /* geez... who cares? */
    {
      DSKnum = -1;   /* no drive selected */
    }
  }
}

/*
  Select side of disk (bit 7)
*/
void ti99_DSKside(int offset, int data)
{
  DSKside = data & 1;

  wd179x_select_drive(DSKnum, DSKside, ti99_fdc_callback, floppy_name[DSKnum]);
}

