#include "driver.h"
#include "cpuintrf.h"
#include "machine/z80fmly.h"
#include "cpu/z80/z80.h"
#include "includes/kc.h"


static void kc85_4_update_0x0c000(void);
static void kc85_4_update_0x0e000(void);
static void kc85_4_update_0x08000(void);
//static void kc85_4_update_0x04000(void);

unsigned char *kc85_ram;

static int kc85_4_pio_data[4];

/* port 0x084/0x085:

bit 7: RAF3
bit 6: RAF2
bit 5: RAF1
bit 4: RAF0
bit 3: FPIX. high resolution
bit 2: BLA1 .access screen
bit 1: BLA0 .pixel/color
bit 0: BILD .display screen 0 or 1
*/

/* port 0x086/0x087:

bit 7: ROCC
bit 6: ROF1
bit 5: ROF0
bit 4-2 are not connected
bit 1: WRITE PROTECT RAM 4
bit 0: ACCESS RAM 4
*/

/* PIO PORT A: port 0x088:

bit 7: ROM C (BASIC)
bit 6: Tape Motor on
bit 5: LED
bit 4: K OUT
bit 3: WRITE PROTECT RAM 0
bit 2: IRM
bit 1: ACCESS RAM 0
bit 0: CAOS ROM E
*/

/* PIO PORT B: port 0x089:
bit 7: BLINK
bit 6: WRITE PROTECT RAM 8
bit 5: ACCESS RAM 8
bit 4: TONE 4
bit 3: TONE 3
bit 2: TONE 2
bit 1: TONE 1
bit 0: TRUCK */


/* load image */
int kc_load(char *filename, int addr)
{
	void *file;

	file = osd_fopen(Machine->gamedrv->name, filename, OSD_FILETYPE_IMAGE_R,OSD_FOPEN_READ);

	if (file)
	{
		int datasize;
		unsigned char *data;

		/* get file size */
		datasize = osd_fsize(file);

		if (datasize!=0)
		{
			/* malloc memory for this data */
			data = malloc(datasize);

			if (data!=NULL)
			{
				int i;

				/* read whole file */
				osd_fread(file, data, datasize);

				for (i=0; i<datasize; i++)
				{
					cpu_writemem16(addr+i, data[i]);
				}

				/* close file */
				osd_fclose(file);

				free(data);

				logerror("File loaded!\n");

				/* ok! */
				return 1;
			}
			osd_fclose(file);

		}
	}

	return 0;
}

/**********************/
/* KEYBOARD EMULATION */
/**********************/

/* The basic transmit proceedure is working, keys are received */
/* Todo: Key-repeat, and allowing the same key to be pressed twice! */

//#define KC_KEYBOARD_DEBUG

/*

  E-mail from Torsten Paul about the keyboard:

	Torsten.Paul@gmx.de

"> I hope that you will be able to help me so I can add keyboard
> emulation to it.
>
Oh yes the keyboard caused me a lot of trouble too. I still don't
understand it completely.
A first dirty but working keyboard support is quite easy because
of the good and modular system rom. Most programs use the supplied
routines to read keyboard input so patching the right memory address
with the keycode that is originally supplied by the interrupt
routines for keyboard input will work. So if you first want to
have a simple start to get other things to work you can try this.

    * write keycode (ascii) to address IX+0dh (0x1fd)
    * set bit 0 at address IX+08h (0x1f8) - simply writing 01h
      worked for me but some other bits are used too
      this bit is reset if the key is read by the system

> From the schematics I see that the keyboard is linked to the CTC and PIO,
> but I don't understand it fully.
> Please can you tell me more about how the keyboard is scanned?
>
The full emulation is quite tricky and I still have some programs that
behave odd. (A good hint for a correct keyboard emulation is Digger ;-).

Ok, now the technical things:

The keyboard of the KC is driven by a remote control circuit that is
originally designed for infrared remote control. This one was named
U807 and I learned there should be a similar chip called SAB 3021
available but I never found the specs on the web. The SAB 3021 was
produced by Valvo which doesn't exist anymore (bought by Phillips
if I remember correctly). If you have more luck finding the specs
I'm still interested.
There also was a complementary chip for the recieving side but that
was not used in the KC unfortunately. They choosed to measure the
pulses sent by the U807 via PIO and CTC.

Anyway here is what I know about the protocol:

The U807 sends impulses with equal length. The information is
given by the time between two impulses. Short time means bit is 1
long time mean bit is 0. The impulses are modulated by a 62.5 kHz
Signal but that's not interesting for the emulator.
The timing comes from a base clock of 4 MHz that is divided
multiple times:

4 MHz / 64 - 62.5 kHz -> used for filtered amplification
62.5 kHz / 64 - 976 Hz -> 1.024 ms
976 / 7 - 140 Hz -> 7.2 ms
976 / 5 - 195 Hz -> 5.1 ms

short - 5.12 ms - 1 bit
long - 7.168 ms - 0 bit

       +-+     +-+       +-+     +-+
       | |     | |       | |     | |
    +--+ +-----+ +-------+ +-----+ +--....

         |     | |---0---| |--1--|
            ^
            |
        Startbit = shift key

The keyboard can have 64 keys with an additional key that is used by
the KC as shift key. This additional key is directly connected to the
chip and changes the startbit. All other keys are arranged in a matrix.

The chip sends full words of 7 bits (including the startbit) at least
twice for each keypress with a spacing of 14 * 1.024 ms. If the key
is still pressed the double word is repeated after 19 * 1.024 ms.

The impulses trigger the pio interrupt line for channel B that
triggers the time measurement by the CTC channel 3." */


/* 

	The CTC timer 3 count is initialised at 0x08f and counts down.
  
	The pulses are connected into PIO /BSTB and generate a interrupt
	on a positive edge. A pulse will not get through if BRDY from PIO is
	not true!
	
	Then the interrupt occurs, the CTC count is read. The time between
	pulses is therefore measured by the CTC.

	The time is checked and depending on the value the KC detects
	a 1 or 0 bit sent from the keyboard.

	Summary From code below:

	0x065<=count<0x078	-> 0 bit				"short pulse"
	0x042<=count<0x065	-> 1 bit				"long pulse"
	count<0x014			-> ignore
	count>=0x078		-> ignore
	0x014<=count<0x042	-> signal end of code	"very long pulse"
	
	codes are sent bit 0, bit 1, bit 2...bit 7. bit 0 is the state
	of the shift key.

	Torsten's e-mail indicates "short pulse" for 1 bit, and "long
	pulse" for 0 bit, but I found this to be incorrect. However,
	the timings are correct.


	Keyboard reading procedure extracted from KC85/4 system rom:

0345  f5        push    af
0346  db8f      in      a,(#8f)			; get CTC timer 3 count
0348  f5        push    af
0349  3ea7      ld      a,#a7			; channel 3, enable int, select counter mode, control word
										; write, software reset, time constant follows	
034b  d38f      out     (#8f),a
034d  3e8f      ld      a,#8f			; time constant
034f  d38f      out     (#8f),a
0351  f1        pop     af

0352  fb        ei      
0353  b7        or      a				; count is over
0354  284d      jr      z,#03a3         ; 

	;; check count is in range
0356  fe14      cp      #14
0358  3849      jr      c,#03a3         ; 
035a  fe78      cp      #78
035c  3045      jr      nc,#03a3        ; 

	;; at this point, time must be between #14 and #77 to be valid

	;; if >=#65, then carry=0, and a 0 bit has been detected

035e  fe65      cp      #65
0360  303d      jr      nc,#039f        ; (61)

	;; if <#65, then a 1 bit has been detected. carry is set with the addition below.
	;; a carry will be generated if the count is >#42

	;; therefore for a 1 bit to be generated, then #42<=time<#65
	;; must be true.

0362  c6be      add     a,#be
0364  3839      jr      c,#039f         ; (57)

	;; this code appears to take the transmitted scan-code
	;; and converts it into a useable code by the os???
0366  e5        push    hl
0367  d5        push    de
	;; convert hardware scan-code into os code
0368  dd7e0c    ld      a,(ix+#0c)
036b  1f        rra     
036c  ee01      xor     #01
036e  dd6e0e    ld      l,(ix+#0e)
0371  dd660f    ld      h,(ix+#0f)
0374  1600      ld      d,#00
0376  5f        ld      e,a
0377  19        add     hl,de
0378  7e        ld      a,(hl)
0379  d1        pop     de
037a  e1        pop     hl

	;; shift lock pressed?
037b  ddcb087e  bit     7,(ix+#08)
037f  200a      jr      nz,#038b        
	;; no.

	;; alpha char?
0381  fe40      cp      #40
0383  3806      jr      c,#038b         
0385  fe80      cp      #80
0387  3002      jr      nc,#038b        
	;; yes, it is a alpha char
	;; force to lower case
0389  ee20      xor     #20

038b  ddbe0d    cp      (ix+#0d)		;; same as stored?
038e  201d      jr      nz,#03ad		
	
	 ;; yes - must be held for a certain time before it can repeat?
0390  f5        push    af
0391  3ae0b7    ld      a,(#b7e0)
0394  ddbe0a    cp      (ix+#0a)
0397  3811      jr      c,#03aa        
0399  f1        pop     af

039a  dd340a    inc     (ix+#0a)		;; incremenent repeat count?
039d  1804      jr      #03a3 

	;; update scan-code received so far

039f  ddcb0c1e  rr      (ix+#0c)		; shift in 0 or 1 bit depending on what has been selected

03a3  db89      in      a,(#89)			; used to clear brdy
03a5  d389      out     (#89),a
03a7  f1        pop     af
03a8  ed4d      reti    

03aa  f1        pop     af
03ab  1808      jr      #03b5           

;; clear count
03ad  dd360a00  ld      (ix+#0a),#00

;; shift lock?
03b1  fe16      cp      #16
03b3  2809      jr      z,#03be         

;; store char
03b5  dd770d    ld      (ix+#0d),a
03b8  ddcb08c6  set     0,(ix+#08)
03bc  18e5      jr      #03a3           

;; toggle shift lock on/off
03be  dd7e08    ld      a,(ix+#08)
03c1  ee80      xor     #80
03c3  dd7708    ld      (ix+#08),a
;; shift/lock was last key pressed
03c6  3e16      ld      a,#16
03c8  18eb      jr      #03b5           

03ca  b7        or      a
03cb  ddcb0846  bit     0,(ix+#08)
03cf  c8        ret     z
03d0  dd7e0d    ld      a,(ix+#0d)
03d3  37        scf     
03d4  c9        ret     
03d5  cdcae3    call    #e3ca
03d8  d0        ret     nc
03d9  ddcb0886  res     0,(ix+#08)
03dd  c9        ret     
03de  cdcae3    call    #e3ca
03e1  d0        ret     nc
03e2  fe03      cp      #03
03e4  37        scf     
03e5  c8        ret     z
03e6  a7        and     a
03e7  c9        ret     
*/

/* number of keycodes that can be stored in queue */
#define KC_KEYCODE_QUEUE_LENGTH 256

/* no transmit is in progress, keyboard is idle ready to transmit a key */
#define KC_KEYBOARD_TRANSMIT_IDLE	0x0001
/* keyboard is transmitting a key-code */
#define KC_KEYBOARD_TRANSMIT_ACTIVE	0x0002

#define KC_KEYBOARD_NUM_LINES	9

typedef struct kc_keyboard
{
	/* list of stored keys */
	unsigned char keycodes[KC_KEYCODE_QUEUE_LENGTH];
	/* index of start of list */
	int head;
	/* index of end of list */
	int tail;

	/* transmitting state */
	int transmit_state;

	/* number of pulses remaining to be transmitted */
	int	transmit_pulse_count_remaining;
	/* count of pulses transmitted so far */
	int transmit_pulse_count;

	/* pulses to transmit */
	unsigned char transmit_buffer[32];

	/* transmit timer */
	void	*transmit_timer;

	/* update timer */
	void	*update_timer;
} kc_keyboard;

/* previous state of keyboard - currently used to detect if a key has been pressed/released  since last scan */
static int kc_previous_keyboard[KC_KEYBOARD_NUM_LINES-1];

/* 
	The kc keyboard is seperate from the kc base unit.

	The keyboard emulation uses 2 timers:

	update_timer is used to add scan-codes to the keycode list.
	transmit_timer is used to transmit the scan-code to the kc.
*/

static kc_keyboard keyboard_data;
static void kc_keyboard_attempt_transmit(void);
static void kc_keyboard_update(int dummy);

/* this is called at a regular interval */
static void kc_keyboard_transmit_timer_callback(int dummy)
{
	if (keyboard_data.transmit_pulse_count_remaining!=0)
	{
		int pulse_state;
		/* byte containing pulse state */
		int pulse_byte_count = keyboard_data.transmit_pulse_count>>3;
		/* bit within byte containing pulse state */
		int pulse_bit_count = 7-(keyboard_data.transmit_pulse_count & 0x07);

		/* get current pulse state */
		pulse_state = (keyboard_data.transmit_buffer[pulse_byte_count]>>pulse_bit_count) & 0x01;

#ifdef KC_KEYBOARD_DEBUG
		logerror("kc keyboard sending pulse: %02x\n",pulse_state);
#endif

		/* set pulse */
		z80pio_bstb_w(0,pulse_state);

		/* update counts */
		keyboard_data.transmit_pulse_count_remaining--;
		keyboard_data.transmit_pulse_count++;

	}
	else
	{
		keyboard_data.transmit_state = KC_KEYBOARD_TRANSMIT_IDLE;
	}
}

/* add a pulse */
static void kc_keyboard_add_pulse_to_transmit_buffer(int pulse_state)
{
	int pulse_byte_count = keyboard_data.transmit_pulse_count_remaining>>3;
	int pulse_bit_count = 7-(keyboard_data.transmit_pulse_count_remaining & 0x07);

	if (pulse_state)
	{
		keyboard_data.transmit_buffer[pulse_byte_count] |= (1<<pulse_bit_count);
	}
	else
	{
		keyboard_data.transmit_buffer[pulse_byte_count] &= ~(1<<pulse_bit_count);
	}

	keyboard_data.transmit_pulse_count_remaining++;
}


/* initialise keyboard queue */
static void kc_keyboard_init(void)
{
	int i;

	/* head and tail of list is at beginning */
	keyboard_data.head = (keyboard_data.tail = 0);

	/* 50hz is just a arbitrary value - used to put scan-codes into the queue for transmitting */
	keyboard_data.update_timer = timer_pulse(TIME_IN_HZ(50), 0, kc_keyboard_update);

	/* timer to transmit pulses to kc base unit */
	keyboard_data.transmit_timer = timer_pulse(TIME_IN_MSEC(1.024), 0, kc_keyboard_transmit_timer_callback);

	/* kc keyboard is not transmitting */
	keyboard_data.transmit_state = KC_KEYBOARD_TRANSMIT_IDLE;

	/* setup transmit parameters */
	keyboard_data.transmit_pulse_count_remaining = 0;
	keyboard_data.transmit_pulse_count = 0;
	
	/* set initial state */
	z80pio_bstb_w(0,0);


	for (i=0; i<KC_KEYBOARD_NUM_LINES; i++)
	{
		/* read input port */
		kc_previous_keyboard[i] = readinputport(i);
	}
}


/* add a key to the queue */
static void kc_keyboard_queue_scancode(int scan_code)
{
	/* add a key */
	keyboard_data.keycodes[keyboard_data.tail] = scan_code;
	/* update next insert position */
	keyboard_data.tail = (keyboard_data.tail + 1) % KC_KEYCODE_QUEUE_LENGTH;
}


/* exit keyboard */
static void kc_keyboard_exit(void)
{
	/* turn off update timer */
	if (keyboard_data.update_timer!=NULL)
	{
		timer_remove(keyboard_data.update_timer);
		keyboard_data.update_timer = NULL;
	}

	/* turn off transmit timer */
	if (keyboard_data.transmit_timer!=NULL)
	{
		timer_remove(keyboard_data.transmit_timer);
		keyboard_data.transmit_timer = NULL;
	}
}

/* fill transmit buffer with pulse for 0 or 1 bit */
static void kc_keyboard_add_bit(int bit)
{
	if (bit)
	{
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
	}
	else
	{
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
		kc_keyboard_add_pulse_to_transmit_buffer(0);
	}

	/* "end of bit" pulse -> end of time for bit */
	kc_keyboard_add_pulse_to_transmit_buffer(1);
}


static void kc_keyboard_begin_transmit(int scan_code)
{
	int i;
	int scan;

	keyboard_data.transmit_pulse_count_remaining = 0;
	keyboard_data.transmit_pulse_count = 0;

	/* initial pulse -> start of code */
	kc_keyboard_add_pulse_to_transmit_buffer(1);

	scan = scan_code;

	/* state of shift key */
	kc_keyboard_add_bit(((readinputport(8) & 0x01)^0x01));
	
	for (i=0; i<6; i++)
	{
		/* each bit in turn */
		kc_keyboard_add_bit(scan & 0x01);
		scan = scan>>1;
	}

	/* signal end of scan-code */
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);
	kc_keyboard_add_pulse_to_transmit_buffer(0);

	kc_keyboard_add_pulse_to_transmit_buffer(1);

	/* back to original state */
	kc_keyboard_add_pulse_to_transmit_buffer(0);

}

/* attempt to transmit a new keycode to the base unit */
static void kc_keyboard_attempt_transmit(void)
{
	/* is the keyboard transmit is idle */
	if (keyboard_data.transmit_state == KC_KEYBOARD_TRANSMIT_IDLE)
	{
		/* keycode available? */
		if (keyboard_data.head!=keyboard_data.tail)
		{
			int code;

			keyboard_data.transmit_state = KC_KEYBOARD_TRANSMIT_ACTIVE;

			/* get code */
			code = keyboard_data.keycodes[keyboard_data.head];
			/* update start of list */
			keyboard_data.head = (keyboard_data.head + 1) % KC_KEYCODE_QUEUE_LENGTH;

			/* setup transmit buffer with scan-code */
			kc_keyboard_begin_transmit(code);
		}
	}
}


/* update keyboard */
static void	kc_keyboard_update(int dummy)
{
	int i;

	/* scan all lines (excluding shift) */
	for (i=0; i<KC_KEYBOARD_NUM_LINES-1; i++)
	{
		int b;
		int keyboard_line_data;
		int changed_keys;
		int mask = 0x001;

		/* read input port */
		keyboard_line_data = readinputport(i);
		/* identify keys that have changed */
		changed_keys = keyboard_line_data ^ kc_previous_keyboard[i];
		/* store input port for next time */
		kc_previous_keyboard[i] = keyboard_line_data;

		/* scan through each bit */
		for (b=0; b<8; b++)
		{
			/* key changed? */
			if (changed_keys & mask)
			{
				/* yes */

				/* new state is pressed? */
				if ((keyboard_line_data & mask)!=0)
				{
					/* pressed */
					int code;

					/* generate fake code */
					code = (i<<3) | b;
#ifdef KC_KEYBOARD_DEBUG
					logerror("code: %02x\n",code);
#endif
					kc_keyboard_queue_scancode(code);
				}
			}

			mask = mask<<1;
		}
	}

	kc_keyboard_attempt_transmit();
}

/*********************************************************************/


static int kc85_84_data;
static int kc85_86_data;

//static int opbase_reset_done = 0;

OPBASE_HANDLER( kc85_opbaseoverride )
{
        //if (!opbase_reset_done)
        //{
        //    opbase_reset_done = 1;

            memory_set_opbase_handler(0,0);

            cpu_set_reg(Z80_PC, 0x0f000);

	kc_load("bd_0400.prg", 0x0400);
    kc_load("bd_4000.scr", 0x4000);


            return (cpu_get_pc() & 0x0ffff);

        //}
        //
        //return PC;
}


READ_HANDLER ( kc85_4_pio_data_r )
{
	return z80pio_d_r(0,offset);
}

READ_HANDLER ( kc85_4_pio_control_r )
{
	return z80pio_c_r(0,offset);
}


WRITE_HANDLER ( kc85_4_pio_data_w )
{

   {
   int PC = cpu_get_pc();

        logerror( "PIO W: PC: %04x O %02x D %02x\n",PC, offset, data);
   }

   kc85_4_pio_data[offset] = data;

   z80pio_d_w(0, offset, data);


   if (offset==0)
   {
           kc85_4_update_0x0c000();
           kc85_4_update_0x0e000();
   }

   //if (offset==1)
   //{
        kc85_4_update_0x08000();
   //}
}

WRITE_HANDLER ( kc85_4_pio_control_w )
{
   z80pio_c_w(0, offset, data);
}


READ_HANDLER ( kc85_4_ctc_r )
{
	unsigned char data;

	data = z80ctc_0_r(offset);
#ifdef KC_KEYBOARD_DEBUG
	logerror("ctc data:%02x\n",data);
#endif
	return data;
}

WRITE_HANDLER ( kc85_4_ctc_w )
{
        logerror("CTC W: %02x\n",data);

        z80ctc_0_w(offset,data);
}

WRITE_HANDLER ( kc85_4_84_w )
{
        logerror("0x084 W: %02x\n",data);

        kc85_84_data = data;

		kc85_4_video_ram_select_bank(data & 0x01);

        kc85_4_update_0x08000();
}

READ_HANDLER ( kc85_4_84_r )
{
	return kc85_84_data;
}

/* port 0x086:

bit 7: CAOS ROM C
bit 6:
bit 5:
bit 4:
bit 3:
bit 2:
bit 1: write protect ram 4
bit 0: ram 4
*/

static void kc85_4_update_0x08000(void)
{
        unsigned char *ram_page;

        if (kc85_4_pio_data[0] & 4)
        {
                /* IRM enabled - has priority over RAM8 enabled */
                logerror("IRM enabled\n");


                {

                        if (kc85_84_data & 0x04)
                        {
                                logerror("access screen 1\n");
                        }
                        else
                        {
                                logerror("access screen 0\n");
                        }

                        if (kc85_84_data & 0x02)
                        {
                                logerror("access colour\n");
                        }
                        else
                        {
                                logerror("access pixel\n");
                        }
               }


				ram_page = kc85_4_get_video_ram_base((kc85_84_data & 0x04), (kc85_84_data & 0x02));

               cpu_setbank(3, ram_page);


               memory_set_bankhandler_r(3, 0, MRA_BANK3);
               memory_set_bankhandler_w(3, 0, MWA_BANK3);


        }
        else
        if (kc85_4_pio_data[1] & 0x020)
        {
                /* RAM8 ACCESS */
                logerror("RAM8 enabled\n");

                ram_page = kc85_ram + 0x08000;

                cpu_setbank(3, ram_page);


                memory_set_bankhandler_r(3, 0, MRA_BANK3);

                /* write protect RAM8 ? */
                if (kc85_4_pio_data[1] & 0x040)
                {
                        memory_set_bankhandler_w(3, 0, MWA_NOP);
                }
                else
                {
                        memory_set_bankhandler_w(3, 0, MWA_BANK3);
                }

        }
        else
        {
                memory_set_bankhandler_r(3, 0, MRA_NOP);
                memory_set_bankhandler_w(3, 0, MWA_NOP);
        }
}

WRITE_HANDLER ( kc85_4_86_w )
{
        logerror("0x086 W: %02x\n",data);

	kc85_86_data = data;

        kc85_4_update_0x0c000();
}

READ_HANDLER ( kc85_4_86_r )
{
	return kc85_86_data;
}

static void kc85_4_pio_interrupt(int state)
{
        cpu_cause_interrupt(0, Z80_VECTOR(0, state));

}

static void kc85_4_ctc_interrupt(int state)
{
        cpu_cause_interrupt(0, Z80_VECTOR(0, state));
}

static void	kc_pio_ardy_callback(int state)
{
	if (state)
	{
		logerror("PIO A Ready\n");
	}

}

static void kc_pio_brdy_callback(int state)
{
	if (state)
	{
		logerror("PIO A Ready\n");
	}
}


static z80pio_interface kc85_4_pio_intf =
{
	1,					/* number of PIOs to emulate */
	{ kc85_4_pio_interrupt },	/* callback when change interrupt status */
	{ kc_pio_ardy_callback },	/* portA ready active callback */
	{ kc_pio_brdy_callback }	/* portB ready active callback */
};

static WRITE_HANDLER ( kc85_4_zc2_callback )
{
//z80ctc_trg_w(0, 3, 0,keyboard_data);
//z80ctc_trg_w(0, 3, 1,keyboard_data);
//z80ctc_trg_w(0, 3, 2,keyboard_data);
//z80ctc_trg_w(0, 3, 3,keyboard_data);

//keyboard_data^=0x0ff;
}


static z80ctc_interface	kc85_4_ctc_intf =
{
	1,
	{1379310.344828},
	{0},
    {kc85_4_ctc_interrupt},
	{0},
	{0},
    {kc85_4_zc2_callback}
};


/* update memory address 0x0c000-0x0e000 */
static void kc85_4_update_0x0c000(void)
{
        if (kc85_86_data & 0x080)
	{
		/* CAOS rom takes priority */

                logerror("CAOS rom 0x0c000\n");

                cpu_setbank(1,memory_region(REGION_CPU1) + 0x012000);
     //           cpu_setbankhandler_r(1,MRA_BANK1);
	}
	else
	if (kc85_4_pio_data[0] & 0x080)
	{
		/* BASIC takes next priority */
                logerror("BASIC rom 0x0c000\n");

                cpu_setbank(1, memory_region(REGION_CPU1) + 0x010000);
       //         cpu_setbankhandler_r(1, MRA_BANK1);
	}
	else
	{
                logerror("No roms 0x0c000\n");

      //          cpu_setbankhandler_r(1, MRA_NOP);
	}
}

/* update memory address 0x0e000-0x0ffff */
static void kc85_4_update_0x0e000(void)
{
	if (kc85_4_pio_data[0] & 0x01)
	{
		/* enable CAOS rom in memory range 0x0e000-0x0ffff */

                logerror("CAOS rom 0x0e000\n");

		/* read will access the rom */
                cpu_setbank(2,memory_region(REGION_CPU1) + 0x014000);
       //         cpu_setbankhandler_r(2, MRA_BANK2);
	}
	else
	{
                logerror("no rom 0x0e000\n");

		/* enable empty space memory range 0x0e000-0x0ffff */
       //         cpu_setbankhandler_r(2, MRA_NOP);
	}
}


void    kc85_4_reset(void)
{
	z80ctc_reset(0);
	z80pio_reset(0);

	/* enable CAOS rom in range 0x0e000-0x0ffff */
	kc85_4_pio_data[0] = 1;
	kc85_4_update_0x0e000();

	/* this is temporary. Normally when a Z80 is reset, it will
	execute address 0. It appears the KC85/4 pages in the rom
	at address 0x0000-0x01000 which has a single jump in it,
	can't see yet where it disables it later!!!! so for now
	here will be a override */
	memory_set_opbase_handler(0, kc85_opbaseoverride);
}

void kc85_4_init_machine(void)
{
	z80pio_init(&kc85_4_pio_intf);
	z80ctc_init(&kc85_4_ctc_intf);

	memory_set_bankhandler_r(1, 0, MRA_BANK1);
	memory_set_bankhandler_r(2, 0, MRA_BANK2);
	memory_set_bankhandler_r(3, 0, MRA_BANK3);
	memory_set_bankhandler_w(3, 0, MWA_BANK3);

	kc85_ram = malloc(64*1024);

	kc85_4_reset();

	kc_keyboard_init();
}

void kc85_4_shutdown_machine(void)
{
	if (kc85_ram)
	{
		free(kc85_ram);
		kc85_ram = NULL;
	}

	kc_keyboard_exit();
}

void kc85_3_init_machine(void)
{
	z80pio_init(&kc85_4_pio_intf);
	z80ctc_init(&kc85_4_ctc_intf);

	memory_set_bankhandler_r(1, 0, MRA_BANK1);
	memory_set_bankhandler_r(2, 0, MRA_BANK2);
	memory_set_bankhandler_r(3, 0, MRA_BANK3);
	memory_set_bankhandler_w(3, 0, MWA_BANK3);

	kc85_ram = malloc(64*1024);

	kc85_4_reset();

	kc_keyboard_init();
}

void kc85_3_shutdown_machine(void)
{
	if (kc85_ram)
	{
		free(kc85_ram);
		kc85_ram = NULL;
	}

	kc_keyboard_exit();
}


