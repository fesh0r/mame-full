/***************************************************************************

Taito Z System [twin 68K with optional Z80]
-------------------------------------------

David Graves

(this is based on the F2 driver by Bryan McPhail, Brad Oliver, Andrew Prime.
Acknowledgments and thanks to Richard Bush and the Raine team, whose open
source was very helpful in many areas particularly the sprites.)

				*****

The Taito Z system has a number of similarities with the Taito F2 system, as
they use some of the same custom Taito components (e.g. the TC0100SCN).

TaitoZ supports 5 separate layers of graphics - one 64x64 tiled scrolling
background plane of 8x8 tiles, a similar foreground plane, another optional
plane used for drawing a road (e.g. Chasehq), a sprite plane [with varying
properties], and a text plane with character definitions held in ram.

The sprites are typically 16x8 tiles aggregated through a spritemap rom
into bigger sizes. Spacegun has 64x64 sprites, but some of the games use
different [128x128] or even multiple sizes. Some of the games aggregate
16x16 tiles to create their sprites. The size of the sprite ram area varies
from 0x400 to 0x4000 bytes, suggesting there is no standard obj chip in
all TaitoZ games.

The Z system has twin 68K CPUs which communicate via $4000 bytes shared ram.
[DG: seems true for all of them so far].

The first 68000 handles screen, palette and sprites, and sometimes other
jobs [e.g. inputs].

The second 68000 typically handles either:
(i)  inputs, sound (through a YM2610) and dips.
(ii) the "road" which features in almost all TaitoZ games [except Spacegun!].

Most Z system games have a Z80 as well, which takes over sound duties.
Commands are written to it by the first 68000 in identical fashion to the
F2 games.


The memory map for the Taito Z games is broadly similar but never
identical. An example without the Z80 follows.

Memory map for Space Gun
------------------------

(1) 68000A
    ======
0x000000 - 0x07ffff : ROM (not all used)
0x30c000 - 0x30ffff : 16K of RAM
0x310000 - 0x313fff : 16K shared RAM
0x314000 - 0x31ffff : 48k of RAM
0x500000 - 0x5005ff : sprite RAM
0x900000 - 0x90ffff : TC0100SCN tilemap generator (\vidhrdw\taitoic.c has more detail)
{
0x900000 - 0x903fff : 64x64 background layer
0x904000 - 0x905fff : 64x64 text layer
0x906000 - 0x907fff : 256 character generator RAM
0x908000 - 0x90bfff : 64x64 foreground layer
0x90c000 - 0x90ffff : unused/row scroll
}
0x920000 - 0x92000f : TC0100SCN control registers
{
0x920000 - 0x920005 : x scroll for 3 layers
0x920006 - 0x92000b : y scroll for 3 layers
0x92000c - 0x92000f : other TC0100SCN control regs
}
0xb00000 - 0b00007 : TC0XXXX palette generator, 4096(?) total colors

(2) 68000B
    ======
0x000000 - 0x07ffff : ROM (not all used)
0x20c000 - 0x20ffff : 16K of RAM
0x210000 - 0x213fff : 16K shared RAM
0x214000 - 0x21ffff : 48k of RAM
0x800000 - 0x80000f : input ports, dipswitches, eerom
0xc00000 - 0xc0000f : communication with YM2610
0xc20000 - 0xc20003 : YM2610 pan (acc. to Raine)
0xe00000 - 0xe00001 : unknown
0xf00002 - 0xf00007 : lightgun hardware



TODO Lists
==========

Palette generators should have sensible names!

Are TopSpeed/Full Throttle and World Grand Prix also Taito System Z games ??
1st two "Air system"?

DIPs


Chasehq
-------

No road or sound. Set dip to steering not locked so you can control car.

[CPUA $4e6 calls sub to test common ram, used to say "Common ram
error". Same sub used to test all blocks of CPUA ram; so what went
wrong was CPUB altering common ram while CPUA testing it.

CPUB every int4 (vblank, $474) puts counter in $1080a6 which corrupts what
CPUA expected to find there. Cpub should be halted during this test.
Now implemented, works fine.]


Chasehq2
--------

Goes through attract. No road.

Sprites are not being written into spriteram!? (The code in \vidhrdw would
display them if they were.)

Sound scarcely works, a snatch here and there.


Continental Circus
------------------

Sound error reported in test mode. No sound or road.

Acceleration/brake inputs are each 8-level. Not sure how to
map this conveniently.


Night Striker
-------------

No road or sound. Unmapped control stick.


Battle Shark
------------

No road. Rotten sound. Unmapped control stick.

[Formerly "SUB CPU ERROR" in service mode. $6b30-48 is where CPUA
loops after showing test mode pattern. Subs $4xx and $7xxx off
this, one of them eventually gets to $817a-a2 sub which after
a while waiting for $112dca to change jumps to $bfc4 which says
"sub cpu error".

$112dca looks like a frame counter, inc'ed at $8000 by CPUB.
It only starts inc'ing this when it has done initial tests and its
interrupts start (enabled at $938; instant interrupt 4 happens
which shortly gets to the code at $8000).

CPUB enables its interrupts slightly too late (1/10s?), CPUA has
already given up on it. Solved by upping cpu interleave 1->100.]

[Non-service mode CPUB enable/disable documentation:
$bd4c-5c cpua waits for byte of shared ram $112dc1 to be non-zero.
$bd5e if it takes too long, cpua decides cpub has "SUB CPU ERROR".

CPUB in fact *does* set this byte of shared ram: but with no CPUB
halt, it does so before CPUA has even started testing shared ram.
CPUA then overwrites the CPUB byte in its testing phase so won't
find it later when it needs to.

Answer is that CPUA is keeping CPUB halted [not just killing its
interrupts?] until the point where it wants CPUB to write the
$112dc1 byte. This is done by poke to $600000. 0 halts
CPUB, 3 re-enables it.]


Aqua Jack
---------

Hangs briefly fairly often: getting stuck in a loop perhaps?
Maybe bad int timing and causation is responsible.

CPUB $108a-c2 reads 0x20 bytes from unmapped area, not sure
what it's doing. Perhaps this machine had some optional
exotic input device...

No road.


Spacegun
--------

Monsters (and various sprites) are missing.

Sprite vs layer priorities not working, see in \vidhrdw.

YM2610 is complaining a *lot* about its mode in the log.
"Slave mode changed while expecting to transmit".

Light gun interrupts timing won't match real machine.

Eerom read and writes not implemented.

[Speed is poor on P3-500. Idle time skip?]


Spacegun code documentation
---------------------------

CPUA
====
$c0c end of main init stuff

$c3e waits for RAM loc. to change. Shared ram that CPU B
has to write to.

$c98 - main initial mem tests now over.

	$562e sub. does ROM checksum
	$18b18 sub. commands read of 0x25 words from eerom by poking 0x24 in
	$310004 and 0 in $310002. DG: prg rom appears to have been hacked!?

	CPUA should surely wait until $310002 is -1 to indicate eerom read is done:
		$18b46 BNE $18b4a   should probably be BEQ $18b4a
	In theory what should happen is:
	CPUB obeys, CPUA checksums words from $310006 - $31004f and
	sets $310db2/3 [-$724e, A5] accordingly. [These are the eerom contents
	dumped by CPUB into shared ram.]

	Currently CPUA does checksum *before* eerom is read, so it's useless !

$23cc - game has passed 2 rom tests and now checks $310db2/3 (shared ram).
        Unless this eerom checksum is -1, jumps --> $57f4.

$57f4 - prints EE ROM ERROR, with the accompanying text message:
    (310db2/3)
	  0   = No Mounted Device ?
	-$25  = New Device ?
	other = Device Death ?

Then jumps us to test mode...

$1bbe - enters test mode ($1c74 draws grid), a while after jumps
      --> $1b500 --> $1b5a6  --> $558 --> $8f6 --> $1ca8/6

$1ca6 loops until 2p select is pressed to advance to next test

$1cb6 calls $17da6 sub. which does all gun calibration stuff
	[$1a words get displayed on screen : relevant eerom contents]

	In this subroutine we find raw light gun coordinates are
	stored in shared ram:
	$310d94	VRP1X
	$310d96	VRP1Y
	$310d98	VRP2X
	$310d9a	VRP2Y

	At $184c4 we have decided to reinitialise the eerom with the new
	values. $18ace sub checksums $31008c-$3100d3 and stores (-1-result)
	in $3100d4/5. This ensures when eerom is read on bootup it will
	correctly checksum to -1.

	CPUA commands the write of 0x25 words to eerom by poking 0x24 into
	$31008a, 0 in $310088. [CPUB sees this -it's shared ram- and obeys.]

	CPUB sets $310088 to -1 to say "done" and $310086 to -1 if the
	write failed. CPUA reports whether write succeeded.

	CPUA then commands a read of 0x24 words of eerom by poking 0x23
	in $310004, 0 in $310002. CPUB sets $310002 to -1 to say "done"
	and $310000 to -1 if the read failed. CPUA reports whether read
	succeeded.

	[$18cc4 "EEROM_WRITE_ERR" then "EEROM_WRITE_OK"
	 $18ce4 "EEROM_READ_ERR" then "EEROM_READ_OK"]

$1ce0 - $1cf2 infinite loop for final page of test mode.
	$1d56 sub. prints out standard text for this page
	$291e sub. prints all button on/off states
	$22fe sub. prints all dips
	$225a sub. lets us choose and play a sound

$1b72 called to place sound in CPUA sound stack

	$317362 - $317381: sound bytes (fill from start)
	$317382/3: sound stack pointer (to next free space)

$1b26 called regularly to take sounds out of CPU A stack and dump them
in shared ram where CPUB can deal with them (it is called from $dd4
thence from $876 which is part of the trap $9 routine).

	$310dbe : set to 1		(-$7242, A5)
	$310dc0 : sound byte		(-$7240, A5)

Interrupt: one with valid vector 
================================
4]  $5f4	probably VBL?


CPUB
====

$982 routine pokes and peeks $c0000d
$9a6 loops waiting for interrupt...

$fd6 - does stuff to $210000-05 and sets pointers to $210006 i.e. start
	of eerom area (shared ram). Eerom read area has last word at
	$210082(or 84 ??). (It seems to contain in total $7e bytes of data:
	log shows 64 words read out of eerom on init.)

	[Eerom write area starts at $210086.]

$b5e trap / interrupt routine start?

$b76-$b80 deals with sound byte in shared ram $210dbe/c0
	$1322 sub. does sound

Interrupts: two with valid vectors
==================================
[Sequence: each frame int4 followed by 4 occurences of int5]

4] $87a   VBL?

Sub $a4c zeros $210d9c, the lookup for which of 4 lightgun coordinates
we await. Lookup values refer to:

00	VRP1X
01	VRP1Y
02	VRP2X
03	VRP2Y

	Pokes $f00000=$0000 to tell lightgun hardware to get 1st coord
	VRP1X.

	Then calls sub $fd6 which does the eerom reading and writing *if*
	the appropriate command words in shared ram are set.

	Reading
	-------
	$1160 gets to loop:
	$1176-$11b0 calls $c20 sub to read words from eerom and stores them
		in the eerom area (shared ram).
			$c20 sets up with writes to $800006. Then bit 7 of
			$800006 is checked to ensure it is clear. If so: 4 more
			writes to $800006, then it is read again to get a
			bit from the EEROM from bit 7 :- until word is read.
EEROM Read Pattern
------------------
46 writes to $800006
Read from bit 7 of $800006 to make sure it is 0
16 repeats of	{
4 writes to $800006
1 read from bit 7 of $800006, eerom data
			}
18 writes to $800006

This pattern is repeated 64 times.

Bit usage in write to $800006 [4 lsbs always zero?]
x.......	appears to be "taken" from read of $800006, not set by the program
.a......	may be set by the prog
..b.....	may be set by the prog
...c....	may be set by the prog

Seen a/b/c combis are 7,5,3,1 and (in some of the 18 post-read writes) 2.


	Writing
	-------
	$1xxx	(near the reading bit above) calls:
		sub $dde which prepares the EEROM (?) calling...
			sub $c9e
		sub $d16 which is called to write a word to the EEROM. This
		happens multiple times until all the words have been written.
		After each word is written in, bit 7 of $800006 is checked to
		ensure it is clear, and then to see if it becomes set.
		Some 0xa00 tries are made, but if either check fails then a
		flag is set to show eerom write process has failed. 


5] $bb2   LIGHT GUN?

Lookup word at $210d9c.

Pulls byte out of ($f00001+lookup*2). Pokes as word into ($21e2b4+
lookup*2):

$21e2b4	VRP1X
$21e2b6	VRP1Y
$21e2b8	VRP2X
$21e2ba	VRP2Y

Increases lookup by 1. Pokes ($f00000+lookup*2)=lookup to tell
lightgun hardware to get next coord.

When lookup=4 all coordinates have been received, and they are
copied into shared ram (see under CPUA for addresses).


Text in Spacegun roms	B.Troha
---------------------
Work RAM (Listed twice)      Common RAM      Ticket     Ticket Payout
Ticket Error     Main Video     Mask CHR Display      SCR CHR Display
Style Display (1 & 2)      Screen Block Display

Test mode: V-ROM M-ROM V-Work RAM V-Prog ROM M-Prog ROM

Title Type:
0: Space Gun	1: Annihilator	2: Creatures Shock
Mfer type:
0: Japan	1: Japan	2: Tamco	3: Tamco-Other	4: Licensed-Other
5: Romstar-Japan	6: Romstar-Tamco	7: Phoenix

***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"

int taitoz_vh_start (void);
int spacegun_vh_start (void);
void taitoz_vh_stop (void);

void chasehq_vh_screenrefresh (struct osd_bitmap *bitmap,int full_refresh);
void contcirc_vh_screenrefresh (struct osd_bitmap *bitmap,int full_refresh);
void bshark_vh_screenrefresh (struct osd_bitmap *bitmap,int full_refresh);
void aquajack_vh_screenrefresh (struct osd_bitmap *bitmap,int full_refresh);
void spacegun_vh_screenrefresh (struct osd_bitmap *bitmap,int full_refresh);

READ_HANDLER ( taitoz_spriteram_r );
WRITE_HANDLER( taitoz_spriteram_w );

/* Palette generators: one has the R and B guns swapped */
READ_HANDLER ( TC0XXXX_word_r );
WRITE_HANDLER( TC0XXXX_word_w );
READ_HANDLER ( TC0YYYY_word_r );
WRITE_HANDLER( TC0YYYY_word_w );

static int ioc220_port=0;
static int old_cpua_ctrl = 0xff;	// must be reset to this each session!

// This was used in the F2 driver for hiscore save or something, marking
// the main ram area. I don't have a use for it yet.
//static unsigned char *taitoz_ram;

static unsigned char *sharedram;

static READ_HANDLER( sharedram_r )
{
	return READ_WORD(&sharedram[offset]);
}

static WRITE_HANDLER( sharedram_w )
{
	COMBINE_WORD_MEM(&sharedram[offset],data);
}

static WRITE_HANDLER( cpua_ctrl_w )
{
	/* bit 0 enables cpu B */

	if ((data &0x1)!=(old_cpua_ctrl &0x1))	// perhaps unnecessary but it's often written with same value
		cpu_set_reset_line(1,(data &0x1) ? CLEAR_LINE : ASSERT_LINE);

	/* is there an irq enable ??? */
//	irqAen = data & 0x02;

	old_cpua_ctrl = data;

	logerror("CPU #0 PC %06x: write %04x to cpu control\n",cpu_get_pc(),data);
}


/***********************************************************
				INTERRUPTS
***********************************************************/

/* 68000 A */

static int taitoz_interrupt(void)
{
	return 4;
}

//void taitoz_interrupt4(int x)
//{
//	cpu_cause_interrupt(0,4);
//}

void taitoz_interrupt6(int x)
{
	cpu_cause_interrupt(0,6);
}

/* 68000 B */

static int taitoz_cpub_interrupt(void)
{
	return 4;
}

//void taitoz_cpub_interrupt4(int x)
//{
//	cpu_cause_interrupt(1,4);
//}

void taitoz_cpub_interrupt5(int x)
{
	cpu_cause_interrupt(1,5);
}


/***** Routines for particular games *****/

static int chasehq_interrupt(void)
{
	return 4;
}

static int chq_cpub_interrupt(void)
{
	return 4;
}

static int chasehq2_interrupt(void)	// Raine halves frequency of int6
{
	timer_set(TIME_IN_CYCLES(200000-500,0),0, taitoz_interrupt6);
	return 4;
}

static int chq2_cpub_interrupt(void)
{
	return 4;
}

// Contcirc: both 68Ks give int6 an individual address, the others all
// point to the same location. I don't know if there should be an int4.

static int contcirc_interrupt(void)
{
//	timer_set(TIME_IN_CYCLES(200000-500,0),0, taitoz_interrupt4);
	return 6;
}

static int contcirc_cpub_interrupt(void)
{
//	timer_set(TIME_IN_CYCLES(200000-500,0),0, taitoz_cpub_interrupt4);
	return 6;
}

static int nightstr_interrupt(void)
{
	timer_set(TIME_IN_CYCLES(200000-500,0),0, taitoz_interrupt6);
	return 4;
}

static int nightstr_cpub_interrupt(void)
{
	return 4;
}

static int bshark_interrupt(void)
{
	timer_set(TIME_IN_CYCLES(200000-500,0),0, taitoz_interrupt6);
	return 4;
}

static int bshark_cpub_interrupt(void)
{
	return 4;
}

static int aquajack_interrupt(void)
{
	return 4;
}

static int aquajack_cpub_interrupt(void)	// int4 goes straight to RTE
{
	return 4;
}

static int spacegun_interrupt(void)
{
	return 4;
}

static int spacegun_cpub_interrupt(void)
{
	return 4;
}


/**********************************************************
			GAME INPUTS
**********************************************************/

// IOC port inputs aren't tidily implemented yet. In lookup table
// 255 is used to represent a port in the IOC we don't read.

static UINT8 chasehq_port_lookup[16] =
{ 8, 9, 0, 1, 255, 255, 255, 255, 2, 3, 4, 5, 6, 7, 255, 255 };

static READ_HANDLER( chasehq_ioc_r )
{
	switch (offset)
	{
		case 0x00:
			{
				/* convert port number to index for MAME's readinputport() */
				int i = chasehq_port_lookup[ioc220_port & 0xf];

				if (i==255)	/* port is not mapped to one of our input ports */
				{
					if (ioc220_port!=4)	// log fills too much with this one ????
logerror("CPU #0 PC %06x: warning - read unmapped ioc220 port %02x\n",cpu_get_pc(),ioc220_port);
					return 0;
				}
				else 
				{
#if 0
	logerror("CPU #0 PC %06x: - read ioc220 port %02x: MAME equivalent %02x\n",cpu_get_pc(),ioc220_port,i);
#endif
					return readinputport(i);
				}
			}

		case 0x02:
			return ioc220_port;
	}

	return 0x00;	// keep compiler happy
}

static WRITE_HANDLER( chasehq_ioc_w )
{
	switch (offset)
	{
		case 0x00:
			// write to ioc [coin lockout etc.?]
			break;

		case 0x02:
			ioc220_port = data &0xff;	// mask seems ok
	}
}


static UINT8 contcirc_port_lookup[16] =
{ 4, 5, 0, 1, 255, 255, 255, 255, 2, 3, 255, 255, 255, 255, 255, 255 };

static READ_HANDLER( contcirc_ioc_r )
{
	switch (offset)
	{
		case 0x00:
			{
				/* convert port number to index for MAME's readinputport() */
				int i = contcirc_port_lookup[ioc220_port & 0xf];

				if (i==255)	/* port is not mapped to one of our input ports */
				{
					if (ioc220_port!=4)	// log fills too much with this one
logerror("CPU #1 PC %06x: warning - read unmapped ioc220 port %02x\n",cpu_get_pc(),ioc220_port);
					return 0;
				}
				else 
				{
#if 0
	logerror("CPU #1 PC %06x: - read ioc220 port %02x: MAME equivalent %02x\n",cpu_get_pc(),ioc220_port,i);
#endif
					return readinputport(i);
				}
			}

		case 0x02:
			return ioc220_port;
	}

	return 0x00;	// keep compiler happy
}

static WRITE_HANDLER( contcirc_ioc_w )
{
	switch (offset)
	{
		case 0x00:
			// write to ioc [coin lockout etc.?]
			break;

		case 0x02:
			ioc220_port = data &0xff;	// mask seems ok
	}
}


static READ_HANDLER( taitoz_input_r )
{
	switch (offset)
	{
		case 0x00:
			return readinputport(3);	/* DSW A */

		case 0x02:
			return readinputport(4);	/* DSW B */

		case 0x04:
			return readinputport(0);	/* IN0 */

		case 0x06:
			return readinputport(1);	/* IN1 */
 
		case 0x0e:
			return readinputport(2);	/* IN2 */
	}

logerror("CPU #0 PC %06x: warning - read unmapped input offset %06x\n",cpu_get_pc(),offset);

	return 0xff;
}


static READ_HANDLER( chasehq2_input_r )
{
	switch (offset)
	{
		case 0x00:
			return readinputport(4);	/* DSW A */

		case 0x02:
			return readinputport(5);	/* DSW B */

		case 0x04:
			return readinputport(0);	/* IN0 */

		case 0x06:
			return readinputport(1);	/* IN1 */
 
		case 0x18:
			return readinputport(2);	/* IN2 */

 		case 0x1a:
			return 0x00; //readinputport(3);	/* IN3 */
	}

logerror("CPU #0 PC %06x: warning - read unmapped input offset %06x\n",cpu_get_pc(),offset);

	return 0xff;
}

static READ_HANDLER( bshark_input_r )
{
	switch (offset)
	{
		case 0x00:
			return readinputport(2);	/* DSW A */

		case 0x02:
			return readinputport(3);	/* DSW B */

		case 0x04:
			return readinputport(0);	/* IN0 */
 
		case 0x0e:
			return readinputport(1);	/* IN1 */
	}

logerror("CPU #0 PC %06x: warning - read unmapped input offset %06x\n",cpu_get_pc(),offset);

	return 0xff;
}


static READ_HANDLER( spacegun_input_r )
{
	switch (offset)
	{
		case 0x00:
			return readinputport(3);	/* DSW A */

		case 0x02:
			return readinputport(4);	/* DSW B */

		case 0x04:
			return readinputport(0);	/* IN0 */

/* msbit used to read eerom, not implemented */
/* 4msbs used to write and set up eerom, not implemented */
		case 0x06:
			return 0x00;
//			return rand() &0xff;
//			return readinputport(1);	/* IN1 */

		case 0x0e:
			return readinputport(2); /* IN2 */
	}

logerror("CPU #1 PC %06x: warning - read unmapped input offset %06x\n",cpu_get_pc(),offset);

	return 0xff;
}

static READ_HANDLER( taitoz_lightgun_r )
{
	switch (offset)
	{
		case 0x00:
			return readinputport(5);	/* P1X */

		case 0x02:
			return readinputport(6);	/* P1Y */

		case 0x04:
			return readinputport(7);	/* P2X */

		case 0x06:
			return readinputport(8);	/* P2Y */
	}

logerror("CPU #1 lightgun_r offset %06x: warning - read unmapped memory address %06x\n",cpu_get_pc(),offset);

	return 0x0;
}

static WRITE_HANDLER( taitoz_lightgun_w )
{
	/* Each write invites a new lightgun interrupt as soon as the
	   hardware has got the next coordinate ready. We set a token
	   delay of 10000 cycles; our "lightgun" coords are always ready
	   but we don't want CPUB to have an int5 before int4 is over (?).

	   Four lightgun interrupts happen before the collected coords
	   are moved to shared ram where CPUA can use them. */

	timer_set(TIME_IN_CYCLES(10000,0),0, taitoz_cpub_interrupt5);
}


/*****************************************
			SOUND
*****************************************/

static WRITE_HANDLER( bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU3);
	int banknum = (data - 1) & 7;

#ifdef MAME_DEBUG
	if (banknum>3) logerror("CPU#3 (Z80) switch to ROM bank %06x: should only happen if Z80 prg rom is 128K!\n",banknum);
#endif
	cpu_setbank (10, &RAM [0x10000 + (banknum * 0x4000)]);
}

WRITE_HANDLER( taitoz_sound_w )
{
	if (offset == 0)
		taito_soundsys_sound_port_w (0, data & 0xff);
	else if (offset == 2)
		taito_soundsys_sound_comm_w (0, data & 0xff);
#ifdef MAME_DEBUG
	if (data & 0xff00)
	{
		char buf[80];

		sprintf(buf,"taitoz_sound_w to high byte: %04x",data);
		usrintf_showmessage(buf);
	}
#endif
}

READ_HANDLER( taitoz_sound_r )
{
	if (offset == 2)
		return ((taito_soundsys_sound_comm_r (0) & 0xff));
	else return 0;
}

WRITE_HANDLER( taitoz_msb_sound_w )
{
	if (offset == 0)
		taito_soundsys_sound_port_w (0,(data >> 8) & 0xff);
	else if (offset == 2)
		taito_soundsys_sound_comm_w (0,(data >> 8) & 0xff);
#ifdef MAME_DEBUG
	if (data & 0xff)
	{
		char buf[80];

		sprintf(buf,"taitoz_msb_sound_w to low byte: %04x",data);
		usrintf_showmessage(buf);
	}
#endif
}

READ_HANDLER( taitoz_msb_sound_r )
{
	if (offset == 2)
		return ((taito_soundsys_sound_comm_r (0) & 0xff) << 8);
	else return 0;
}


/***********************************************************
			 MEMORY STRUCTURES

The ROM ranges are all 7ffff, quite a lot need changing
Bank10 is reserved for the Z80
***********************************************************/


static struct MemoryReadAddress chasehq_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x100000, 0x107fff, MRA_BANK1 },	/* main CPUA ram */
	{ 0x108000, 0x10bfff, sharedram_r },
	{ 0x10c000, 0x10ffff, MRA_BANK2 },	/* extra CPUA ram */
	{ 0x400000, 0x400003, chasehq_ioc_r },
	{ 0x820000, 0x820003, taitoz_msb_sound_r },
	{ 0xa00000, 0xa00007, TC0YYYY_word_r },	/* palette */
	{ 0xc00000, 0xc0ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0xc20000, 0xc2000f, TC0100SCN_ctrl_word_0_r },
	{ 0xd00000, 0xd007ff, taitoz_spriteram_r },
	{ -1 }
};

static struct MemoryWriteAddress chasehq_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100000, 0x107fff, MWA_BANK1 },
	{ 0x108000, 0x10bfff, sharedram_w, &sharedram },
	{ 0x10c000, 0x10ffff, MWA_BANK2 },
	{ 0x400000, 0x400003, chasehq_ioc_w },
	{ 0x800000, 0x800001, cpua_ctrl_w },
	{ 0x820000, 0x820003, taitoz_msb_sound_w },
	{ 0xa00000, 0xa00007, TC0YYYY_word_w },	/* palette */
	{ 0xc00000, 0xc0ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0xc20000, 0xc2000f, TC0100SCN_ctrl_word_0_w },
	{ 0xd00000, 0xd007ff, taitoz_spriteram_w, &spriteram, &spriteram_size },
	{ -1 }
};

static struct MemoryReadAddress chq_cpub_readmem[] =
{
	{ 0x000000, 0x01ffff, MRA_ROM },
	{ 0x100000, 0x103fff, MRA_BANK6 },
	{ 0x108000, 0x10bfff, sharedram_r },
	{ 0x800000, 0x801fff, MRA_BANK8 },	// road?
	{ -1 }
};

static struct MemoryWriteAddress chq_cpub_writemem[] =
{
	{ 0x000000, 0x01ffff, MWA_ROM },
	{ 0x100000, 0x103fff, MWA_BANK6 },
	{ 0x108000, 0x10bfff, sharedram_w, &sharedram },
	{ 0x800000, 0x801fff, MWA_BANK8 },	// road?
	{ -1 }
};

static struct MemoryReadAddress chasehq2_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x100000, 0x107fff, MRA_BANK1 },	/* main CPUA ram */
	{ 0x108000, 0x10bfff, sharedram_r },	/* extent ?? */
	{ 0x10c000, 0x10ffff, MRA_BANK2 },	/* extra CPUA ram */
	{ 0x200000, 0x20001f, chasehq2_input_r },
	{ 0x420000, 0x420003, taitoz_sound_r },
	{ 0x800000, 0x801fff, paletteram_word_r },
	{ 0xa00000, 0xa0ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0xa20000, 0xa2000f, TC0100SCN_ctrl_word_0_r },
	{ 0xc00000, 0xc03fff, taitoz_spriteram_r },	// Raine says only 0x1000 actually used
	{ -1 }
};

static struct MemoryWriteAddress chasehq2_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100000, 0x107fff, MWA_BANK1 },
	{ 0x108000, 0x10bfff, sharedram_w, &sharedram },
	{ 0x10c000, 0x10ffff, MWA_BANK2 },
	{ 0x420000, 0x420003, taitoz_sound_w },
	{ 0x800000, 0x801fff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0xa00000, 0xa0ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0xa20000, 0xa2000f, TC0100SCN_ctrl_word_0_w },
	{ 0xc00000, 0xc03fff, taitoz_spriteram_w, &spriteram, &spriteram_size },
	{ -1 }
};

static struct MemoryReadAddress chq2_cpub_readmem[] =
{
	{ 0x000000, 0x01ffff, MRA_ROM },
	{ 0x200000, 0x207fff, MRA_BANK6 },
	{ 0x208000, 0x20bfff, sharedram_r },
	{ 0xa00000, 0xa01fff, MRA_BANK8 },	// road?
	{ -1 }
};

static struct MemoryWriteAddress chq2_cpub_writemem[] =
{
	{ 0x000000, 0x01ffff, MWA_ROM },
	{ 0x200000, 0x207fff, MWA_BANK6 },
	{ 0x208000, 0x20bfff, sharedram_w, &sharedram },
	{ 0xa00000, 0xa01fff, MWA_BANK8 },	// road?
	{ -1 }
};


static struct MemoryReadAddress contcirc_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x080000, 0x083fff, MRA_BANK1 },	/* main CPUA ram */
	{ 0x084000, 0x087fff, sharedram_r },
	{ 0x100000, 0x100007, TC0XXXX_word_r },	/* palette */
	{ 0x200000, 0x20ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0x220000, 0x22000f, TC0100SCN_ctrl_word_0_r },
	{ 0x300000, 0x301fff, MRA_BANK3 },	/* "root ram", road? */
	{ 0x400000, 0x4006ff, taitoz_spriteram_r },
	{ -1 }
};

static struct MemoryWriteAddress contcirc_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x080000, 0x083fff, MWA_BANK1 },
	{ 0x084000, 0x087fff, sharedram_w, &sharedram },
	{ 0x100000, 0x100007, TC0XXXX_word_w },	/* palette */
	{ 0x200000, 0x20ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0x220000, 0x22000f, TC0100SCN_ctrl_word_0_w },
	{ 0x300000, 0x301fff, MWA_BANK3 },	/* "root ram", road? */
	{ 0x400000, 0x4006ff, taitoz_spriteram_w, &spriteram, &spriteram_size },
	{ -1 }
};

static struct MemoryReadAddress contcirc_cpub_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x080000, 0x083fff, MRA_BANK6 },
	{ 0x084000, 0x087fff, sharedram_r },
	{ 0x100000, 0x100003, contcirc_ioc_r },
	{ 0x200000, 0x200003, taitoz_msb_sound_r },
	{ -1 }
};

static struct MemoryWriteAddress contcirc_cpub_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x080000, 0x083fff, MWA_BANK6 },
	{ 0x084000, 0x087fff, sharedram_w, &sharedram },
	{ 0x100000, 0x100003, contcirc_ioc_w },
	{ 0x200000, 0x200003, taitoz_msb_sound_w },
	{ -1 }
};


static struct MemoryReadAddress nightstr_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x100000, 0x10ffff, MRA_BANK1 },	/* main CPUA ram */
	{ 0x110000, 0x113fff, sharedram_r },
	{ 0x400000, 0x40000f, bshark_input_r },
	{ 0x820000, 0x820003, taitoz_msb_sound_r },
	{ 0xa00000, 0xa00007, TC0YYYY_word_r },	/* palette */
	{ 0xc00000, 0xc0ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0xc20000, 0xc2000f, TC0100SCN_ctrl_word_0_r },
	{ 0xd00000, 0xd007ff, taitoz_spriteram_r },
	{ -1 }
};

static struct MemoryWriteAddress nightstr_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100000, 0x10ffff, MWA_BANK1 },
	{ 0x110000, 0x113fff, sharedram_w, &sharedram },
	{ 0x400000, 0x400001, MWA_NOP },	/* watchdog ?? */
	{ 0x800000, 0x800001, cpua_ctrl_w },
	{ 0x820000, 0x820003, taitoz_msb_sound_w },
	{ 0xa00000, 0xa00007, TC0YYYY_word_w },	/* palette */
	{ 0xc00000, 0xc0ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0xc20000, 0xc2000f, TC0100SCN_ctrl_word_0_w },
	{ 0xd00000, 0xd007ff, taitoz_spriteram_w, &spriteram, &spriteram_size },
	{ -1 }
};

static struct MemoryReadAddress nightstr_cpub_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x100000, 0x103fff, MRA_BANK6 },
	{ 0x104000, 0x107fff, sharedram_r },
	{ 0x800000, 0x801fff, MRA_BANK8 },	/* "root ram", road? */
	{ -1 }
};

static struct MemoryWriteAddress nightstr_cpub_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x100000, 0x103fff, MWA_BANK6 },
	{ 0x104000, 0x107fff, sharedram_w, &sharedram },
	{ 0x800000, 0x801fff, MWA_BANK8 },	/* "root ram", road? */
	{ -1 }
};


static struct MemoryReadAddress bshark_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x100000, 0x10ffff, MRA_BANK1 },	/* main CPUA ram */
	{ 0x110000, 0x113fff, sharedram_r },
	{ 0x400000, 0x40000f, bshark_input_r },
//	{ 0x800002, 0x800003, taitoz_unknown_r },	// ???
	{ 0xa00000, 0xa01fff, paletteram_word_r },	/* palette */
	{ 0xc00000, 0xc00fff, taitoz_spriteram_r },
	{ 0xd00000, 0xd0ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0xd20000, 0xd2000f, TC0100SCN_ctrl_word_0_r },
	{ -1 }
};

static struct MemoryWriteAddress bshark_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100000, 0x10ffff, MWA_BANK1 },
	{ 0x110000, 0x113fff, sharedram_w, &sharedram },
	{ 0x400000, 0x400001, MWA_NOP },	/* watchdog ?? */
	{ 0x600000, 0x600001, cpua_ctrl_w },
//	{ 0x800002, 0x800003, MWA_NOP },	/* unknown */
//	{ 0x800006, 0x800007, MWA_NOP },	/* unknown */
	{ 0xa00000, 0xa01fff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },	// used up to $a0117f
	{ 0xc00000, 0xc00fff, taitoz_spriteram_w, &spriteram, &spriteram_size },
	{ 0xd00000, 0xd0ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0xd20000, 0xd2000f, TC0100SCN_ctrl_word_0_w },
	{ -1 }
};

static struct MemoryReadAddress bshark_cpub_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x108000, 0x10bfff, MRA_BANK6 },
	{ 0x110000, 0x113fff, sharedram_r },
//	{ 0x40000a, 0x40000b, taitoz_unknown_r },	// ???
	{ 0x600000, 0x600001, YM2610_status_port_0_A_r },
	{ 0x600002, 0x600003, YM2610_read_port_0_r },
	{ 0x600004, 0x600005, YM2610_status_port_0_B_r },
	{ 0x60000c, 0x60000d, MRA_NOP },
	{ 0x60000e, 0x60000f, taito_soundsys_a001_r },
	{ 0x800000, 0x801fff, MRA_BANK8 },	/* "root ram", road? */
	{ -1 }
};

static struct MemoryWriteAddress bshark_cpub_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x108000, 0x10bfff, MWA_BANK6 },
	{ 0x110000, 0x113fff, sharedram_w },
//	{ 0x400000, 0x400007, MWA_NOP },   // pan ???
	{ 0x600000, 0x600001, YM2610_control_port_0_A_w },
	{ 0x600002, 0x600003, YM2610_data_port_0_A_w },
	{ 0x600004, 0x600005, YM2610_control_port_0_B_w },
	{ 0x600006, 0x600007, YM2610_data_port_0_B_w },
	{ 0x60000c, 0x60000d, taito_soundsys_a000_w },
	{ 0x60000e, 0x60000f, taito_soundsys_a001_w },
	{ 0x800000, 0x801fff, MWA_BANK8 },	/* "root ram", road? */
	{ -1 }
};


static struct MemoryReadAddress aquajack_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x100000, 0x103fff, MRA_BANK1 },	/* main CPUA ram */
	{ 0x104000, 0x107fff, sharedram_r },
	{ 0x300000, 0x300007, TC0YYYY_word_r },	/* palette */
	{ 0x800000, 0x801fff, MRA_BANK8 },	// road ?
	{ 0xa00000, 0xa0ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0xa20000, 0xa2000f, TC0100SCN_ctrl_word_0_r },
	{ 0xc40000, 0xc403ff, taitoz_spriteram_r },
	{ -1 }
};

static struct MemoryWriteAddress aquajack_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x100000, 0x103fff, MWA_BANK1 },
	{ 0x104000, 0x107fff, sharedram_w, &sharedram },
	{ 0x300000, 0x300007, TC0YYYY_word_w },	/* palette */
	{ 0x800000, 0x801fff, MWA_BANK8 },	// road ?
	{ 0xa00000, 0xa0ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0xa20000, 0xa2000f, TC0100SCN_ctrl_word_0_w },
	{ 0xc40000, 0xc403ff, taitoz_spriteram_w, &spriteram, &spriteram_size },
	{ -1 }
};

static struct MemoryReadAddress aquajack_cpub_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x100000, 0x103fff, MRA_BANK6 },
	{ 0x104000, 0x107fff, sharedram_r },
	{ 0x200000, 0x20000f, taitoz_input_r },
	{ 0x300000, 0x300003, taitoz_sound_r },
//	{ 0x800800, 0x80083f, taitoz_unknown_r }, // There are a LOT of unknown reads here...
//	{ 0x900000, 0x900007, taitoz_unknown_r },
	{ -1 }
};

static struct MemoryWriteAddress aquajack_cpub_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x100000, 0x103fff, MWA_BANK6 },
	{ 0x104000, 0x107fff, sharedram_w, &sharedram },
	{ 0x300000, 0x300003, taitoz_sound_w },
//	{ 0x800800, 0x800801, taitoz_unknown_w },
//	{ 0x900000, 0x900007, taitoz_unknown_w },
	{ -1 }
};


// DG: I don't think Raine is correct in making full 64K shared...

static struct MemoryReadAddress spacegun_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x30c000, 0x30ffff, MRA_BANK1 },	/* extra CPUA ram */
	{ 0x310000, 0x313fff, sharedram_r },	/* extent ?? */
	{ 0x314000, 0x31ffff, MRA_BANK2 },	/* main CPUA ram */
	{ 0x500000, 0x5005ff, taitoz_spriteram_r },
	{ 0x900000, 0x90ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0x920000, 0x92000f, TC0100SCN_ctrl_word_0_r },
	{ 0xb00000, 0xb00007, TC0XXXX_word_r },	/* palette */
	{ -1 }
};

static struct MemoryWriteAddress spacegun_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x30c000, 0x30ffff, MWA_BANK1 },
	{ 0x310000, 0x313fff, sharedram_w, &sharedram },
	{ 0x314000, 0x31ffff, MWA_BANK2 },
	{ 0x500000, 0x5005ff, taitoz_spriteram_w, &spriteram, &spriteram_size },
	{ 0x900000, 0x90ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0x920000, 0x92000f, TC0100SCN_ctrl_word_0_w },
	{ 0xb00000, 0xb00007, TC0XXXX_word_w },	/* palette */
	{ -1 }
};

static struct MemoryReadAddress spacegun_cpub_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x20c000, 0x20ffff, MRA_BANK6 },
	{ 0x210000, 0x213fff, sharedram_r },
	{ 0x214000, 0x21ffff, MRA_BANK7 },
	{ 0x800000, 0x80000f, spacegun_input_r },   // includes eerom reads
	{ 0xc00000, 0xc00001, YM2610_status_port_0_A_r },
	{ 0xc00002, 0xc00003, YM2610_read_port_0_r },
	{ 0xc00004, 0xc00005, YM2610_status_port_0_B_r },
	{ 0xc0000c, 0xc0000d, MRA_NOP },
	{ 0xc0000e, 0xc0000f, taito_soundsys_a001_r },
	{ 0xf00000, 0xf00007, taitoz_lightgun_r },
	{ -1 }
};

static struct MemoryWriteAddress spacegun_cpub_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x20c000, 0x20ffff, MWA_BANK6 },
	{ 0x210000, 0x213fff, sharedram_w },
	{ 0x214000, 0x21ffff, MWA_BANK7 },
	{ 0x800000, 0x800001, MWA_NOP },   /* diverse writes */
//	{ 0x800006, 0x800009, MWA_NOP },   /* diverse writes here, includes eerom */
	{ 0xc00000, 0xc00001, YM2610_control_port_0_A_w },
	{ 0xc00002, 0xc00003, YM2610_data_port_0_A_w },
	{ 0xc00004, 0xc00005, YM2610_control_port_0_B_w },
	{ 0xc00006, 0xc00007, YM2610_data_port_0_B_w },
	{ 0xc0000c, 0xc0000d, taito_soundsys_a000_w },
	{ 0xc0000e, 0xc0000f, taito_soundsys_a001_w },
//	{ 0xc20000, 0xc20003, YM2610_???? },	/* Pan (acc. to Raine) */
//	{ 0xe00000, 0xe00001, MWA_NOP },	/* unknown */
	{ 0xf00000, 0xf00007, taitoz_lightgun_w },
	{ -1 }
};


/***************************************************************************/

static struct MemoryReadAddress z80_sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK10 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, taito_soundsys_a001_r },
	{ 0xea00, 0xea00, MRA_NOP },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress z80_sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, taito_soundsys_a000_w },
	{ 0xe201, 0xe201, taito_soundsys_a001_w },
	{ 0xe400, 0xe403, MWA_NOP }, /* pan */
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP }, /* ? */
	{ 0xf200, 0xf200, bankswitch_w },
	{ -1 }  /* end of table */
};


/***********************************************************
			 INPUT PORTS, DIPs
***********************************************************/

INPUT_PORTS_START( chasehq )	// IN2-5 perhaps used on cockpit setup?
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )	// brake
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )	// turbo
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )	// gear
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )	// accel
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2, ??? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3, ??? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN4, ??? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN5, ??? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN6, steering (high byte?) */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER1, 50, 10, 0, 0 )

	PORT_START      /* IN7, steering (low byte?) */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER2, 50, 10, 0, 0 )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x03, "Upright / Steering Lock" )	//DG: these need swapping?
	PORT_DIPSETTING(    0x02, "Upright / No Steering Lock" )
	PORT_DIPSETTING(    0x01, "Full Throttle Convert, Cockpit" )
	PORT_DIPSETTING(    0x00, "Full Throttle Convert, Deluxe" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Timer Setting" )
	PORT_DIPSETTING(    0x0c, "60 Seconds" )
	PORT_DIPSETTING(    0x08, "70 Seconds" )
	PORT_DIPSETTING(    0x04, "85 Seconds" )
	PORT_DIPSETTING(    0x00, "55 Seconds" )
	PORT_DIPNAME( 0x10, 0x10, "Turbos Stocked" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Damage Cleared at Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( chasehq2 )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2, wheel ??? */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER1, 50, 10, 0, 0 )

	PORT_START      /* IN3, gear ??? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, "Cockpit" )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Timer Setting" )
	PORT_DIPSETTING(    0x0c, "60 Seconds" )
	PORT_DIPSETTING(    0x08, "70 Seconds" )
	PORT_DIPSETTING(    0x04, "85 Seconds" )
	PORT_DIPSETTING(    0x00, "55 Seconds" )
	PORT_DIPNAME( 0x10, 0x10, "Turbos Stocked" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x20, 0x20, "Respond to Controls" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Damage Cleared at Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Siren Volume" )
	PORT_DIPSETTING(    0x80, "Normal" )
	PORT_DIPSETTING(    0x00, "Low" )
INPUT_PORTS_END

INPUT_PORTS_START( contcirc )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )	// 3 for accel [7 levels]
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_BUTTON3 | IPF_PLAYER1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON7 | IPF_PLAYER1 )	// gear shift lo/hi
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )	// 3 for brake [7 levels]
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )

	PORT_START      /* IN2, "handle" [i.e. wheel] */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER1, 50, 10, 0, 0 )

	PORT_START      /* IN3, ??? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( nightstr )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	// these 4 may relate to the 1st test screen
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( bshark )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1, not sure of b2-6 purpose */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )	// "Fire"
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( aquajack )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )	// may not exist?
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )	// may not exist?

	PORT_START      /* IN2, what is it ??? */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER1, 50, 10, 0, 0 )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, "Cockpit" )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )	/* Actually "Price to Continue" */
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )	/* Actually "Same as current Coin A" */
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B, needs numbers in reverse order like DSWA ?? */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "50k" )
	PORT_DIPSETTING(    0x08, "80k" )
	PORT_DIPSETTING(    0x04, "100k" )
	PORT_DIPSETTING(    0x00, "30k" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) ) /* Dips 7 & 8 shown as "Do Not Touch" in manual */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( spacegun )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )  /* ?? not shown in service mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2)

	PORT_START      /* IN1, used to read eerom */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER1, 20, 10, 0, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_Y | IPF_PLAYER1, 20, 10, 0, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER2, 20, 10, 0, 0xff)

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_AD_STICK_Y | IPF_PLAYER2, 20, 10, 0, 0xff)
INPUT_PORTS_END


/***********************************************************
				GFX DECODING

Preliminary: perhaps some games have multiple chunk sizes??
OBJ rom load order is wrong, which is causing peculiar
colours on sprites...

Raine gives these details on obj layouts:
- Chase HQ
- Night Striker

00000-3FFFF = Object 128x128 [16x16] [19900/80:0332] gfx bank#1
40000-5FFFF = Object  64x128 [16x16] [0CC80/40:0332] gfx bank#2
60000-7FFFF = Object  32x128 [16x16] [06640/20:0332] gfx bank#2

- Top Speed
- Full Throttle
- Continental Circus

00000-7FFFF = Object 128x128 [16x8] [xxxxx/100:xxxx] gfx bank#1

- Aqua Jack
- Chase HQ 2
- Battle Shark
- Space Gun
- Operation Thunderbolt

00000-7FFFF = Object 64x64 [16x8] [xxxxx/40:xxxx] gfx bank#1
***********************************************************/

static struct GfxLayout tile16x8_layout =
{
	16,8,	// 16*8 sprites
	RGN_FRAC(1,1),
	4,	// 4 bits per pixel
	{ 0, 8, 16, 24 },
	{ 32, 33, 34, 35, 36, 37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8	// every sprite takes 64 consecutive bytes
};

static struct GfxLayout tile16x16_layout =
{
	16,16,	// 16*16 sprites
	RGN_FRAC(1,1),
	4,	// 4 bits per pixel
	{ 0, 8, 16, 24 },
	{ 32, 33, 34, 35, 36, 37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*64, 1*64,  2*64,  3*64,  4*64,  5*64,  6*64,  7*64,
	  8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	64*16	// every sprite takes 128 consecutive bytes
};

/*******************************************************
(Previously we decoded Spacegun sprites at full size,
 but now we aggregate them on the fly.)

static struct GfxLayout spacegun_tilelayout =
{
	64,64,	// 64*64 sprites
	RGN_FRAC(1,1),
	4,	// 4 bits per pixel
	{ 0, 8, 16, 24 },
			{32,  33,  34,  35,  36,  37,  38,  39,
			  0,   1,   2,   3,   4,   5,   6,   7,
			 96,  97,  98,  99, 100, 101, 102, 103,
			 64,  65,  66,  67,  68,  69,  70,  71,
			160, 161, 162, 163, 164, 165, 166, 167,
			128, 129, 130, 131, 132, 133, 134, 135,
			224, 225, 226, 227, 228, 229, 230, 231,
			192, 193, 194, 195, 196, 197, 198, 199 },
			{0*256,  1*256,  2*256,  3*256,  4*256,  5*256,  6*256,  7*256,
			 8*256,  9*256, 10*256, 11*256, 12*256, 13*256, 14*256, 15*256,
			16*256, 17*256, 18*256, 19*256, 20*256, 21*256, 22*256, 23*256,
			24*256, 25*256, 26*256, 27*256, 28*256, 29*256, 30*256, 31*256,
			32*256, 33*256, 34*256, 35*256, 36*256, 37*256, 38*256, 39*256,
			40*256, 41*256, 42*256, 43*256, 44*256, 45*256, 46*256, 47*256,
			48*256, 49*256, 50*256, 51*256, 52*256, 53*256, 54*256, 55*256,
			56*256, 57*256, 58*256, 59*256, 60*256, 61*256, 62*256, 63*256 },
	256*64  	// every sprite takes 2048 consecutive bytes
};
*******************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo spacegun_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &tile16x8_layout,  0, 256 },	/* sprite parts */
	{ REGION_GFX1, 0, &charlayout,  0, 256 },	/* sprites & playfield */
	{ -1 } /* end of array */
};

// taitoic TC0100SCN routines expect scr stuff to be in second gfx
// slot, so 2nd batch of obj must go third:

static struct GfxDecodeInfo chasehq_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0, &tile16x16_layout,  0, 256 },	/* sprite parts */
	{ REGION_GFX1, 0x0, &charlayout,  0, 256 },	/* sprites & playfield */
	{ REGION_GFX4, 0x0, &tile16x16_layout,  0, 256 },	/* sprites parts */
	// Missing Road!
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo chasehq2_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0, &tile16x8_layout,  0, 256 },	/* sprite parts */
	{ REGION_GFX1, 0x0, &charlayout,  0, 256 },	/* sprites & playfield */
	// Missing Road!
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo contcirc_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0, &tile16x8_layout,  0, 256 },	/* sprite parts */
	{ REGION_GFX1, 0x0, &charlayout,  0, 256 },	/* sprites & playfield */
	// Missing Road!
	{ -1 } /* end of array */
};


/**************************************************************
			     YM2610 (SOUND)

The first interface is for game boards with twin 68000 and Z80.

Interface B is for games which lack a Z80 (Spacegun, Bshark).
**************************************************************/

/* handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(2,0,irq ? ASSERT_LINE : CLEAR_LINE);
}


/* handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irqhandlerb(int irq)
{
	// DG: this is probably wrong? 2nd 68000 has 2 ints, both write to ym2610
	// causing errors in log.

	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	1,	/* 1 chip */
	16000000/2,	/* 8 MHz ?? */
	{ 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND2 },	/* Delta-T */
	{ REGION_SOUND1 },	/* ADPCM */
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) }
};

static struct YM2610interface ym2610_interfaceb =
{
	1,	/* 1 chip */
	16000000/2,	/* 8 MHz ?? */
	{ 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandlerb },
	{ REGION_SOUND2 },	/* Delta-T */
	{ REGION_SOUND1 },	/* ADPCM */
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) }
};


/***********************************************************
			     MACHINE DRIVERS

Bshark needed the high cpu interleaving to run test mode.
Chasehq2 and Aquajack have got it to rule it out as a cause
of the problems they currently have.
***********************************************************/

static struct MachineDriver machine_driver_chasehq =
{
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			chasehq_readmem,chasehq_writemem,0,0,
			chasehq_interrupt, 1
		},
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			chq_cpub_readmem,chq_cpub_writemem,0,0,
			chq_cpub_interrupt, 1
		},
		{																			\
			CPU_Z80 | CPU_AUDIO_CPU,												\
			16000000/4,	/* 4 MHz ??? */													\
			z80_sound_readmem, z80_sound_writemem,0,0,										\
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */				\
		}																			\
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* CPU slices */
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 32*8-1 },

	chasehq_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitoz_vh_start,
	taitoz_vh_stop,
	chasehq_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_chasehq2 =
{
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			chasehq2_readmem,chasehq2_writemem,0,0,
			chasehq2_interrupt, 1
		},
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			chq2_cpub_readmem,chq2_cpub_writemem,0,0,
			chq2_cpub_interrupt, 1
		},
		{																			\
			CPU_Z80 | CPU_AUDIO_CPU,												\
			16000000/4,	/* 4 MHz ??? */													\
			z80_sound_readmem, z80_sound_writemem,0,0,										\
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */				\
		}																			\
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	100,	/* CPU slices */
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 32*8-1 },

	chasehq2_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitoz_vh_start,
	taitoz_vh_stop,
	bshark_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_contcirc =
{
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			contcirc_readmem,contcirc_writemem,0,0,
			contcirc_interrupt, 1
		},
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			contcirc_cpub_readmem,contcirc_cpub_writemem,0,0,
			contcirc_cpub_interrupt, 1
		},
		{																			\
			CPU_Z80 | CPU_AUDIO_CPU,												\
			16000000/4,	/* 4 MHz ??? */													\
			z80_sound_readmem, z80_sound_writemem,0,0,										\
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */				\
		}																			\
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* CPU slices */
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 32*8-1 },

	contcirc_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitoz_vh_start,
	taitoz_vh_stop,
	contcirc_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_nightstr =
{
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			nightstr_readmem,nightstr_writemem,0,0,
			nightstr_interrupt, 1
		},
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			nightstr_cpub_readmem,nightstr_cpub_writemem,0,0,
			nightstr_cpub_interrupt, 1
		},
		{																			\
			CPU_Z80 | CPU_AUDIO_CPU,												\
			16000000/4,	/* 4 MHz ??? */													\
			z80_sound_readmem, z80_sound_writemem,0,0,										\
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */				\
		}																			\
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* CPU slices */
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 32*8-1 },

	chasehq_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitoz_vh_start,
	taitoz_vh_stop,
	chasehq_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};

static struct MachineDriver machine_driver_bshark =
{
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			bshark_readmem,bshark_writemem,0,0,
			bshark_interrupt, 1
		},
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			bshark_cpub_readmem,bshark_cpub_writemem,0,0,
			bshark_cpub_interrupt, 1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	100,	/* CPU slices */
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 32*8-1 },

	chasehq2_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitoz_vh_start,
	taitoz_vh_stop,
	bshark_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interfaceb
		}
	}
};

static struct MachineDriver machine_driver_aquajack =
{
	{
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			aquajack_readmem,aquajack_writemem,0,0,
			aquajack_interrupt, 1
		},
		{
			CPU_M68000,
			12000000,	/* 12 MHz ??? */
			aquajack_cpub_readmem,aquajack_cpub_writemem,0,0,
			aquajack_cpub_interrupt, 1
		},
		{																			\
			CPU_Z80 | CPU_AUDIO_CPU,												\
			16000000/4,	/* 4 MHz ??? */													\
			z80_sound_readmem, z80_sound_writemem,0,0,										\
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */				\
		}																			\
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	100,	/* CPU slices */
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 32*8-1 },

	chasehq2_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	taitoz_vh_start,
	taitoz_vh_stop,
	aquajack_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};


static struct MachineDriver machine_driver_spacegun =
{
	{
		{
			CPU_M68000,
			16000000,	/* 16 MHz ??? Why should 68000 be faster than other TaitoZ */
			spacegun_readmem,spacegun_writemem,0,0,
			spacegun_interrupt, 1
		},
		{
			CPU_M68000,
			16000000,	/* 16 MHz ??? */
			spacegun_cpub_readmem,spacegun_cpub_writemem,0,0,
			spacegun_cpub_interrupt, 1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* CPU slices */
	0,

	/* video hardware */
	40*8, 32*8, { 0*8, 40*8-1, 2*8, 32*8-1 },

	spacegun_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	spacegun_vh_start,
	taitoz_vh_stop,
	spacegun_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interfaceb
		}
	}
};


/***************************************************************************
					DRIVERS

It is likely that some of the adpcm/delta-t roms are wrongly ordered.
***************************************************************************/

ROM_START( chasehq )
	ROM_REGION( 0x80000, REGION_CPU1 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD_EVEN( "b52-130.rom", 0x00000, 0x20000, 0x4e7beb46 )
	ROM_LOAD_ODD ( "b52-136.rom", 0x00000, 0x20000, 0x2f414df0 )
	ROM_LOAD_EVEN( "b52-131.rom", 0x40000, 0x20000, 0xaa945d83 )
	ROM_LOAD_ODD ( "b52-129.rom", 0x40000, 0x20000, 0x0eaebc08 )

	ROM_REGION( 0x80000, REGION_CPU2 )	/* 128K for 68000 code (CPU B) */
	ROM_LOAD_EVEN( "b52-132.rom", 0x00000, 0x10000, 0xa2f54789 )
	ROM_LOAD_ODD ( "b52-133.rom", 0x00000, 0x10000, 0x12232f95 )

	ROM_REGION( 0x1c000, REGION_CPU3 )	/* Z80 sound cpu */
	ROM_LOAD( "b52-137.rom",   0x00000, 0x04000, 0x37abb74a )
	ROM_CONTINUE(            0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "b52-m29.rom", 0x00000, 0x80000, 0x8366d27c )	/* SCR */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)
	ROM_LOAD_QUAD( "b52-m35.rom", 0x000000, 0x080000, 0x78eeec0d )	/* OBJ A: each rom has 1 bitplane, forming 16x16 tiles */
	ROM_LOAD_QUAD( "b52-m34.rom", 0x000000, 0x080000, 0x7d8dce36 )
	ROM_LOAD_QUAD( "b52-m37.rom", 0x000000, 0x080000, 0xf02e47b9 )	/* These are grouped according to a sprite map */
	ROM_LOAD_QUAD( "b52-m36.rom", 0x000000, 0x080000, 0x61e89e91 )	/* rom to create the big sprites the game uses. */

	ROM_REGION( 0x80000, REGION_GFX3 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "b52-m28.rom", 0x00000, 0x80000, 0x963bc82b )	/* ROD, road lines */

	ROM_REGION( 0x200000, REGION_GFX4 | REGIONFLAG_DISPOSE)
	ROM_LOAD_QUAD( "b52-m31.rom", 0x000000, 0x080000, 0xf1998e20 )	/* OBJ B: each rom has 1 bitplane, forming 16x16 tiles */
	ROM_LOAD_QUAD( "b52-m30.rom", 0x000000, 0x080000, 0x1b8cc647 )
	ROM_LOAD_QUAD( "b52-m33.rom", 0x000000, 0x080000, 0xe6f4b8c4 )	/* These are grouped according to a sprite map */
	ROM_LOAD_QUAD( "b52-m32.rom", 0x000000, 0x080000, 0x8620780c )	/* rom to create the big sprites the game uses. */

	ROM_REGION( 0x80000, REGION_USER1)
	ROM_LOAD( "b52-m38.fix", 0x00000, 0x80000, 0x5b5bf7f6 )	/* STY, index used to create big sprites on the fly */

	ROM_REGION( 0x180000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "b52-m113.rom", 0x000000, 0x080000, 0x2c6a3a05 )
	ROM_LOAD( "b52-m114.rom", 0x080000, 0x080000, 0x3a73d6b1 )
	ROM_LOAD( "b52-m115.rom", 0x100000, 0x080000, 0x4e117e93 )

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* delta-t samples */
	ROM_LOAD( "b52-m116.rom", 0x00000, 0x80000, 0xad46983c )

	ROM_REGION( 0x10000, REGION_USER2)
	ROM_LOAD( "b52-50.rom", 0x00000, 0x10000, 0xc189781c )	/* unused roms */
	ROM_LOAD( "b52-51.rom", 0x00000, 0x10000, 0x30cc1f79 )
ROM_END

ROM_START( chasehqj )
	ROM_REGION( 0x80000, REGION_CPU1 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD_EVEN( "b52-140",     0x00000, 0x20000, 0xc1298a4b )
	ROM_LOAD_ODD ( "b52-139",     0x00000, 0x20000, 0x997f732e )
	ROM_LOAD_EVEN( "b52-131.rom", 0x40000, 0x20000, 0xaa945d83 )
	ROM_LOAD_ODD ( "b52-129.rom", 0x40000, 0x20000, 0x0eaebc08 )

	ROM_REGION( 0x80000, REGION_CPU2 )	/* 128K for 68000 code (CPU B) */
	ROM_LOAD_EVEN( "b52-132.rom", 0x00000, 0x10000, 0xa2f54789 )
	ROM_LOAD_ODD ( "b52-133.rom", 0x00000, 0x10000, 0x12232f95 )

	ROM_REGION( 0x1c000, REGION_CPU3 )	/* Z80 sound cpu */
	ROM_LOAD( "b52-134",    0x00000, 0x04000, 0x91faac7f )
	ROM_CONTINUE(           0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "b52-m29.rom", 0x00000, 0x80000, 0x8366d27c )	/* SCR */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)
	ROM_LOAD_QUAD( "b52-m35.rom", 0x000000, 0x080000, 0x78eeec0d )	/* OBJ A: each rom has 1 bitplane, forming 16x16 tiles */
	ROM_LOAD_QUAD( "b52-m34.rom", 0x000000, 0x080000, 0x7d8dce36 )
	ROM_LOAD_QUAD( "b52-m37.rom", 0x000000, 0x080000, 0xf02e47b9 )	/* These are grouped according to a sprite map */
	ROM_LOAD_QUAD( "b52-m36.rom", 0x000000, 0x080000, 0x61e89e91 )	/* rom to create the big sprites the game uses. */

	ROM_REGION( 0x80000, REGION_GFX3 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "b52-m28.rom", 0x00000, 0x80000, 0x963bc82b )	/* ROD, road lines */

	ROM_REGION( 0x200000, REGION_GFX4 | REGIONFLAG_DISPOSE)
	ROM_LOAD_QUAD( "b52-m31.rom", 0x000000, 0x080000, 0xf1998e20 )	/* OBJ B: each rom has 1 bitplane, forming 16x16 tiles */
	ROM_LOAD_QUAD( "b52-m30.rom", 0x000000, 0x080000, 0x1b8cc647 )
	ROM_LOAD_QUAD( "b52-m33.rom", 0x000000, 0x080000, 0xe6f4b8c4 )	/* These are grouped according to a sprite map */
	ROM_LOAD_QUAD( "b52-m32.rom", 0x000000, 0x080000, 0x8620780c )	/* rom to create the big sprites the game uses. */

	ROM_REGION( 0x80000, REGION_USER1)
	ROM_LOAD( "b52-m38.fix", 0x00000, 0x80000, 0x5b5bf7f6 )	/* STY, index used to create big sprites on the fly */

	ROM_REGION( 0x180000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "b52-39", 0x000000, 0x80000, 0xac9cbbd3 )
	ROM_LOAD( "b52-40", 0x080000, 0x80000, 0xf0551055 )
	ROM_LOAD( "b52-41", 0x100000, 0x80000, 0x8204880c )

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* delta-t samples */
	ROM_LOAD( "b52-42", 0x00000, 0x80000, 0x6e617df1 )

	ROM_REGION( 0x10000, REGION_USER2)
	ROM_LOAD( "b52-50.rom", 0x00000, 0x10000, 0xc189781c )	/* unused roms */
	ROM_LOAD( "b52-51.rom", 0x00000, 0x10000, 0x30cc1f79 )
ROM_END

ROM_START( chasehq2 )
	ROM_REGION( 0x80000, REGION_CPU1 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD_EVEN( "c09-37.rom", 0x00000, 0x20000, 0x0fecea17 )
	ROM_LOAD_ODD ( "c09-40.rom", 0x00000, 0x20000, 0xe46ebd9b )
	ROM_LOAD_EVEN( "c09-38.rom", 0x40000, 0x20000, 0xf4404f87 )
	ROM_LOAD_ODD ( "c09-41.rom", 0x40000, 0x20000, 0xde87bcb9 )

	ROM_REGION( 0x80000, REGION_CPU2 )	/* 128K for 68000 code (CPU B) */
	ROM_LOAD_EVEN( "c09-33.rom", 0x00000, 0x10000, 0xcf4e6c5b )
	ROM_LOAD_ODD ( "c09-32.rom", 0x00000, 0x10000, 0xa4713719 )

	ROM_REGION( 0x2c000, REGION_CPU3 )	/* Z80 sound cpu */
	ROM_LOAD( "c09-34.rom",   0x00000, 0x04000, 0xa21b3151 )
	ROM_CONTINUE(            0x10000, 0x1c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "c09-05.rom", 0x00000, 0x80000, 0x890b38f0 )	/* SCR */

// wrong obj load order (probably)
	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)
	ROM_LOAD_QUAD( "c09-02.rom", 0x000000, 0x080000, 0xa83a0389 )
	ROM_LOAD_QUAD( "c09-01.rom", 0x000000, 0x080000, 0x64bfea10 )	/* OBJ: each rom has 1 bitplane, forming 16x8 tiles */
	ROM_LOAD_QUAD( "c09-04.rom", 0x000000, 0x080000, 0x2cbb3c9b )
	ROM_LOAD_QUAD( "c09-03.rom", 0x000000, 0x080000, 0xa31d0e80 )

	ROM_REGION( 0x80000, REGION_GFX3 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "c09-07.rom", 0x00000, 0x80000, 0x963bc82b )	/* ROD, road lines */

	ROM_REGION( 0x80000, REGION_USER1)
	ROM_LOAD( "c09-06.rom", 0x00000, 0x80000, 0x12df6d7b )	/* STY, index used to create 64x64 sprites on the fly */

	ROM_REGION( 0x180000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "c09-12.rom", 0x000000, 0x080000, 0x56c99fa5 )
	ROM_LOAD( "c09-13.rom", 0x080000, 0x080000, 0xd57c41d3 )
	ROM_LOAD( "c09-14.rom", 0x100000, 0x080000, 0xad78bf46 )

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* delta-t samples */
	ROM_LOAD( "c09-15.rom", 0x00000, 0x80000, 0xe63b9095 )

	ROM_REGION( 0x10000, REGION_USER2)
	ROM_LOAD( "c09-16.rom", 0x00000, 0x10000, 0x7245a6f6 )	/* unused roms */
	ROM_LOAD( "c09-23.rom", 0x00000, 0x00100, 0xfbf81f30 )
ROM_END

ROM_START( chashq2u )
	ROM_REGION( 0x80000, REGION_CPU1 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD_EVEN( "c09-43.37",  0x00000, 0x20000, 0x20a9343e )
	ROM_LOAD_ODD ( "c09-44.40",  0x00000, 0x20000, 0x7524338a )
	ROM_LOAD_EVEN( "c09-41.38",  0x40000, 0x20000, 0x83477f11 )
	ROM_LOAD_ODD ( "c09-41.rom", 0x40000, 0x20000, 0xde87bcb9 )

	ROM_REGION( 0x80000, REGION_CPU2 )	/* 128K for 68000 code (CPU B) */
	ROM_LOAD_EVEN( "c09-33.rom", 0x00000, 0x10000, 0xcf4e6c5b )
	ROM_LOAD_ODD ( "c09-32.rom", 0x00000, 0x10000, 0xa4713719 )

	ROM_REGION( 0x2c000, REGION_CPU3 )	/* Z80 sound cpu */
	ROM_LOAD( "c09-34.rom",   0x00000, 0x04000, 0xa21b3151 )
	ROM_CONTINUE(             0x10000, 0x1c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "c09-05.rom", 0x00000, 0x80000, 0x890b38f0 )	/* SCR */

// wrong obj load order (probably)
	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)
	ROM_LOAD_QUAD( "c09-03.rom", 0x000000, 0x080000, 0xa31d0e80 )
	ROM_LOAD_QUAD( "c09-04.rom", 0x000000, 0x080000, 0x2cbb3c9b )
	ROM_LOAD_QUAD( "c09-01.rom", 0x000000, 0x080000, 0x64bfea10 )	/* OBJ: each rom has 1 bitplane, forming 16x8 tiles */
	ROM_LOAD_QUAD( "c09-02.rom", 0x000000, 0x080000, 0xa83a0389 )

	ROM_REGION( 0x80000, REGION_GFX3 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "c09-07.rom", 0x00000, 0x80000, 0x963bc82b )	/* ROD, road lines */

	ROM_REGION( 0x80000, REGION_USER1)
	ROM_LOAD( "c09-06.rom", 0x00000, 0x80000, 0x12df6d7b )	/* STY, index used to create 64x64 sprites on the fly */

	ROM_REGION( 0x180000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "c09-12.rom", 0x000000, 0x080000, 0x56c99fa5 )
	ROM_LOAD( "c09-13.rom", 0x080000, 0x080000, 0xd57c41d3 )
	ROM_LOAD( "c09-14.rom", 0x100000, 0x080000, 0xad78bf46 )

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* delta-t samples */
	ROM_LOAD( "c09-15.rom", 0x00000, 0x80000, 0xe63b9095 )

	ROM_REGION( 0x10000, REGION_USER2)
	ROM_LOAD( "c09-16.rom", 0x00000, 0x10000, 0x7245a6f6 )	/* unused roms */
	ROM_LOAD( "c09-23.rom", 0x00000, 0x00100, 0xfbf81f30 )
ROM_END

ROM_START( contcirc )	// The "alt" set has better naming conventions on its roms
	ROM_REGION( 0x80000, REGION_CPU1 )	/* 256K for 68000 code (CPU A) */
	ROM_LOAD_EVEN( "cc_2_25.bin", 0x00000, 0x20000, 0xf5c92e42 )
	ROM_LOAD_ODD ( "cc_2_26.bin", 0x00000, 0x20000, 0x1345ebe6 )

	ROM_REGION( 0x80000, REGION_CPU2 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD_EVEN( "cc_2_35.bin", 0x00000, 0x20000, 0x16522f2d )
	ROM_LOAD_ODD ( "cc_2_36.bin", 0x00000, 0x20000, 0xa1732ea5 )

	ROM_REGION( 0x1c000, REGION_CPU3 )	/* Z80 sound cpu */
	ROM_LOAD( "cc_1_11.bin",   0x00000, 0x04000, 0xd8746234 )
	ROM_CONTINUE(              0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "cc_2_57.bin", 0x00000, 0x80000, 0xf6fb3ba2 )	/* SCR */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)
	ROM_LOAD_QUAD( "cc_3_06.bin", 0x000000, 0x080000, 0xbddf9eea )	/* OBJ: each rom has 1 bitplane, forming 16x8 tiles */
	ROM_LOAD_QUAD( "cc_3_07.bin", 0x000000, 0x080000, 0x2cb40599 )
	ROM_LOAD_QUAD( "cc_3_01.bin", 0x000000, 0x080000, 0x4f6c36d9 )
	ROM_LOAD_QUAD( "cc_3_02.bin", 0x000000, 0x080000, 0x8df866a2 )

	ROM_REGION( 0x80000, REGION_GFX3 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "cc_2_03.bin", 0x00000, 0x80000, 0xf11f2be8 )	/* ROD, road lines */

	ROM_REGION( 0x80000, REGION_USER1)
	ROM_LOAD( "cc_3_64.bin", 0x00000, 0x80000, 0x151e1f52 )	/* STY, index used to create 128x128 sprites on the fly */

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "cc_1_18.bin", 0x000000, 0x080000, 0x1e6724b5 )
	ROM_LOAD( "cc_1_17.bin", 0x080000, 0x080000, 0xe9ce03ab )

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* delta-t samples */
	ROM_LOAD( "cc_1_19.bin", 0x00000, 0x80000, 0xcaa1c4c8 )

	ROM_REGION( 0x10000, REGION_USER2)
	ROM_LOAD( "cc_3_97.bin", 0x00000, 0x10000, 0xdccb0c7f )	/* unused rom */
ROM_END

ROM_START( contcrcu )
	ROM_REGION( 0x80000, REGION_CPU1 )	/* 256K for 68000 code (CPU A) */
	ROM_LOAD_EVEN( "ic25", 0x00000, 0x20000, 0xf5c92e42 )
	ROM_LOAD_ODD ( "ic26", 0x00000, 0x20000, 0xe7c1d1fa )

	ROM_REGION( 0x80000, REGION_CPU2 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD_EVEN( "ic35", 0x00000, 0x20000, 0x16522f2d )
	ROM_LOAD_ODD ( "ic36", 0x00000, 0x20000, 0xd6741e33 )

	ROM_REGION( 0x1c000, REGION_CPU3 )	/* Z80 sound cpu */
	ROM_LOAD( "b33_30",   0x00000, 0x04000, 0xd8746234 )
	ROM_CONTINUE(              0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "b33_02", 0x00000, 0x80000, 0xf6fb3ba2 )	/* SCR */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)
	ROM_LOAD_QUAD( "b33_05", 0x000000, 0x080000, 0xbddf9eea )	/* OBJ: each rom has 1 bitplane, forming 16x8 tiles */
	ROM_LOAD_QUAD( "b33_06", 0x000000, 0x080000, 0x2cb40599 )
	ROM_LOAD_QUAD( "b33_03", 0x000000, 0x080000, 0x4f6c36d9 )
	ROM_LOAD_QUAD( "b33_04", 0x000000, 0x080000, 0x8df866a2 )

	ROM_REGION( 0x80000, REGION_GFX3 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "b33_01", 0x00000, 0x80000, 0xf11f2be8 )	/* ROD, road lines */

	ROM_REGION( 0x80000, REGION_USER1)
	ROM_LOAD( "b33_07", 0x00000, 0x80000, 0x151e1f52 )	/* STY, index used to create 128x128 sprites on the fly */

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "b33_09", 0x000000, 0x080000, 0x1e6724b5 )
	ROM_LOAD( "b33_10", 0x080000, 0x080000, 0xe9ce03ab )

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* delta-t samples */
	ROM_LOAD( "b33_08", 0x00000, 0x80000, 0xcaa1c4c8 )

	ROM_REGION( 0x10000, REGION_USER2)
	ROM_LOAD( "b14-30", 0x00000, 0x10000, 0xdccb0c7f )	/* unused roms */
	ROM_LOAD( "b14-31", 0x00000, 0x02000, 0x5c6b013d )
ROM_END

ROM_START( nightstr )
	ROM_REGION( 0x80000, REGION_CPU1 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD_EVEN( "b91-45.bin", 0x00000, 0x20000, 0x7ad63421 )
	ROM_LOAD_ODD ( "b91-44.bin", 0x00000, 0x20000, 0x4bc30adf )
	ROM_LOAD_EVEN( "b91-43.bin", 0x40000, 0x20000, 0x3e6f727a )
	ROM_LOAD_ODD ( "b91-46.bin", 0x40000, 0x20000, 0xe870be95 )

	ROM_REGION( 0x80000, REGION_CPU2 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD_EVEN( "b91-39.bin", 0x00000, 0x20000, 0x725b23ae )
	ROM_LOAD_ODD ( "b91-40.bin", 0x00000, 0x20000, 0x81fb364d )

	ROM_REGION( 0x2c000, REGION_CPU3 )	/* Z80 sound cpu */
	ROM_LOAD( "b91-41.bin",   0x00000, 0x04000, 0x2694bb42 )
	ROM_CONTINUE(            0x10000, 0x1c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "b91-11.bin", 0x00000, 0x80000, 0xfff8ce31 )	/* SCR */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)
	ROM_LOAD_QUAD( "b91-03.bin", 0x000000, 0x080000, 0xcd5fed39 )	/* OBJ A: each rom has 1 bitplane, forming 16x16 tiles */
	ROM_LOAD_QUAD( "b91-04.bin", 0x000000, 0x080000, 0x8ca1970d )
	ROM_LOAD_QUAD( "b91-01.bin", 0x000000, 0x080000, 0x3731d94f )
	ROM_LOAD_QUAD( "b91-02.bin", 0x000000, 0x080000, 0x457c64b8 )

	ROM_REGION( 0x80000, REGION_GFX3 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "b91-10.bin", 0x00000, 0x80000, 0x1d8f05b4 )	/* ROD, road lines */

	ROM_REGION( 0x200000, REGION_GFX4 | REGIONFLAG_DISPOSE)
	ROM_LOAD_QUAD( "b91-07.bin", 0x000000, 0x080000, 0x4d8ec6cf )
	ROM_LOAD_QUAD( "b91-08.bin", 0x000000, 0x080000, 0x66f35c34 )	/* OBJ B: each rom has 1 bitplane, forming 16x16 tiles */
	ROM_LOAD_QUAD( "b91-05.bin", 0x000000, 0x080000, 0x5e72ac90 )
	ROM_LOAD_QUAD( "b91-06.bin", 0x000000, 0x080000, 0xa34dc839 )

	ROM_REGION( 0x80000, REGION_USER1)
	ROM_LOAD( "b91-09.bin", 0x00000, 0x80000, 0x5f247ca2 )	/* STY, index used to create big sprites on the fly */

	ROM_REGION( 0x100000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "b91-12.bin", 0x000000, 0x080000, 0xda77c7af )
	ROM_LOAD( "b91-13.bin", 0x080000, 0x080000, 0x8c7bf0f5 )

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* delta-t samples */
	ROM_LOAD( "b91-14.bin", 0x00000, 0x80000, 0x6bc314d3 )

	ROM_REGION( 0x10000, REGION_USER2)
	ROM_LOAD( "b91-29.bin", 0x00000, 0x2000,  0xad685be8 )	/* unused roms */
	ROM_LOAD( "b91-30.bin", 0x00000, 0x10000, 0x30cc1f79 )
	ROM_LOAD( "b91-31.bin", 0x00000, 0x10000, 0xc189781c )
	ROM_LOAD( "b91-26.bin", 0x00000, 0x400,   0x77682a4f )
	ROM_LOAD( "b91-27.bin", 0x00000, 0x400,   0xa3f8490d )
	ROM_LOAD( "b91-28.bin", 0x00000, 0x400,   0xfa2f840e )
	ROM_LOAD( "b91-32.bin", 0x00000, 0x100,   0xfbf81f30 )
	ROM_LOAD( "b91-33.bin", 0x00000, 0x100,   0x89719d17 )
ROM_END

ROM_START( bshark )
	ROM_REGION( 0x80000, REGION_CPU1 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD_EVEN( "bshark71.bin", 0x00000, 0x20000, 0xdf1fa629 )
	ROM_LOAD_ODD ( "bshark69.bin", 0x00000, 0x20000, 0xa54c137a )
	ROM_LOAD_EVEN( "bshark70.bin", 0x40000, 0x20000, 0xd77d81e2 )
	ROM_LOAD_ODD ( "bshark67.bin", 0x40000, 0x20000, 0x39307c74 )

	ROM_REGION( 0x80000, REGION_CPU2 )	/* 512K for 68000 code (CPU B) */
	ROM_LOAD_EVEN( "bshark74.bin", 0x00000, 0x20000, 0x6869fa99 )
	ROM_LOAD_ODD ( "bshark72.bin", 0x00000, 0x20000, 0xc09c0f91 )
	ROM_LOAD_EVEN( "bshark75.bin", 0x40000, 0x20000, 0x6ba65542 )
	ROM_LOAD_ODD ( "bshark73.bin", 0x40000, 0x20000, 0xf2fe62b5 )

	ROM_REGION( 0x80000, REGION_GFX1 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "bshark05.bin", 0x00000, 0x80000, 0x596b83da )	/* SCR */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)
	ROM_LOAD_QUAD( "bshark03.bin", 0x000000, 0x080000, 0xa18eab78 )	/* OBJ: each rom has 1 bitplane, forming 16x8 tiles */
	ROM_LOAD_QUAD( "bshark04.bin", 0x000000, 0x080000, 0x2446b0da )
	ROM_LOAD_QUAD( "bshark01.bin", 0x000000, 0x080000, 0x3ebe8c63 )
	ROM_LOAD_QUAD( "bshark02.bin", 0x000000, 0x080000, 0x8488ba10 )

	ROM_REGION( 0x80000, REGION_GFX3 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "bshark07.bin", 0x00000, 0x80000, 0xedb07808 )	/* ROD, road lines */

	ROM_REGION( 0x80000, REGION_USER1)
	ROM_LOAD( "bshark06.bin", 0x00000, 0x80000, 0xd200b6eb )	/* STY, index used to create 64x64 sprites on the fly */

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* ADPCM samples ? */
	ROM_LOAD( "bshark09.bin", 0x00000, 0x80000, 0xc4cbc907 )

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* delta-t samples ? rom too short? */
	ROM_LOAD( "bshark08.bin", 0x00000, 0x20000, 0x1c79e69c )

	ROM_REGION( 0x10000, REGION_USER2)
	ROM_LOAD( "bshark18.bin", 0x00000, 0x10000, 0x7245a6f6 )	/* unused roms */
	ROM_LOAD( "bshark19.bin", 0x00000, 0x00100, 0x2ee9c404 )
ROM_END

ROM_START( aquajack )
	ROM_REGION( 0x80000, REGION_CPU1 )	/* 256K for 68000 code (CPU A) */
	ROM_LOAD_EVEN( "b77-22.rom", 0x00000, 0x20000, 0x67400dde )
	ROM_LOAD_ODD ( "34.17",      0x00000, 0x20000, 0xcd4d0969 )

	ROM_REGION( 0x80000, REGION_CPU2 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD_EVEN( "b77-24.rom", 0x00000, 0x20000, 0x95e643ed )
	ROM_LOAD_ODD ( "b77-23.rom", 0x00000, 0x20000, 0x395a7d1c )

	ROM_REGION( 0x1c000, REGION_CPU3 )	/* sound cpu */
	ROM_LOAD( "b77-20.rom",   0x00000, 0x04000, 0x84ba54b7 )
	ROM_CONTINUE(            0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "b77-05.rom", 0x00000, 0x80000, 0x7238f0ff )	/* SCR */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)
	ROM_LOAD_QUAD( "b77-03.rom", 0x000000, 0x80000, 0x9a3030a7 )	/* OBJ: each rom has 1 bitplane, forming 16x8 tiles */
	ROM_LOAD_QUAD( "b77-04.rom", 0x000000, 0x80000, 0xbed0be6c )
	ROM_LOAD_QUAD( "b77-01.rom", 0x000000, 0x80000, 0xcdab000d )
	ROM_LOAD_QUAD( "b77-02.rom", 0x000000, 0x80000, 0xdaea0d2e )

	ROM_REGION( 0x80000, REGION_GFX3 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "b77-07.rom", 0x000000, 0x80000, 0x7db1fc5e )	/* ROD, road lines */

	ROM_REGION( 0x80000, REGION_USER1)
	ROM_LOAD( "b77-06.rom", 0x00000, 0x80000, 0xce2aed00 )	/* STY, index used to create 64x64 sprites on the fly */

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "b77-09.rom", 0x00000, 0x80000, 0x948e5ad9 )

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* delta-t samples */
	ROM_LOAD( "b77-08.rom", 0x00000, 0x80000, 0x119b9485 )

//	(no unused roms in my set)
ROM_END

ROM_START( aquajckj )
	ROM_REGION( 0x80000, REGION_CPU1 )	/* 256K for 68000 code (CPU A) */
	ROM_LOAD_EVEN( "b77-22.rom", 0x00000, 0x20000, 0x67400dde )
	ROM_LOAD_ODD ( "b77-21.rom", 0x00000, 0x20000, 0x23436845 )

	ROM_REGION( 0x80000, REGION_CPU2 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD_EVEN( "b77-24.rom", 0x00000, 0x20000, 0x95e643ed )
	ROM_LOAD_ODD ( "b77-23.rom", 0x00000, 0x20000, 0x395a7d1c )

	ROM_REGION( 0x1c000, REGION_CPU3 )	/* sound cpu */
	ROM_LOAD( "b77-20.rom",   0x00000, 0x04000, 0x84ba54b7 )
	ROM_CONTINUE(            0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION( 0x80000, REGION_GFX1 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "b77-05.rom", 0x00000, 0x80000, 0x7238f0ff )	/* SCR */

	ROM_REGION( 0x200000, REGION_GFX2 | REGIONFLAG_DISPOSE)
	ROM_LOAD_QUAD( "b77-03.rom", 0x000000, 0x80000, 0x9a3030a7 )	/* OBJ: each rom has 1 bitplane, forming 16x8 tiles */
	ROM_LOAD_QUAD( "b77-04.rom", 0x000000, 0x80000, 0xbed0be6c )
	ROM_LOAD_QUAD( "b77-01.rom", 0x000000, 0x80000, 0xcdab000d )
	ROM_LOAD_QUAD( "b77-02.rom", 0x000000, 0x80000, 0xdaea0d2e )

	ROM_REGION( 0x80000, REGION_GFX3 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "b77-07.rom", 0x000000, 0x80000, 0x7db1fc5e )	/* ROD, road lines */

	ROM_REGION( 0x80000, REGION_USER1)
	ROM_LOAD( "b77-06.rom", 0x00000, 0x80000, 0xce2aed00 )	/* STY, index used to create 64x64 sprites on the fly */

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "b77-09.rom", 0x00000, 0x80000, 0x948e5ad9 )

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* delta-t samples */
	ROM_LOAD( "b77-08.rom", 0x00000, 0x80000, 0x119b9485 )

//	(no unused roms in my set)
ROM_END

ROM_START( spacegun )
	ROM_REGION( 0x80000, REGION_CPU1 )	/* 512K for 68000 code (CPU A) */
	ROM_LOAD_EVEN( "c57-18", 0x00000, 0x20000, 0x19d7d52e )
	ROM_LOAD_ODD ( "c57-20", 0x00000, 0x20000, 0x2e58253f )
	ROM_LOAD_EVEN( "c57-17", 0x40000, 0x20000, 0xe197edb8 )
	ROM_LOAD_ODD ( "c57-22", 0x40000, 0x20000, 0x5855fde3 )

	ROM_REGION( 0x80000, REGION_CPU2 )	/* 256K for 68000 code (CPU B) */
	ROM_LOAD_EVEN( "c57-15", 0x00000, 0x20000, 0xb36eb8f1 )
	ROM_LOAD_ODD ( "c57-16", 0x00000, 0x20000, 0xbfb5d1e7 )

	ROM_REGION( 0x80000, REGION_GFX1 | REGIONFLAG_DISPOSE)
	ROM_LOAD( "c57-06", 0x00000, 0x80000, 0x4ebadd5b )		/* SCR */

	ROM_REGION( 0x400000, REGION_GFX2 | REGIONFLAG_DISPOSE)
	ROM_LOAD_QUAD( "c57-02", 0x000000, 0x100000, 0x21ee4633 )	/* OBJ: each rom has 1 bitplane, forming 16x8 tiles */
	ROM_LOAD_QUAD( "c57-01", 0x000000, 0x100000, 0xf901b04e )
	ROM_LOAD_QUAD( "c57-04", 0x000000, 0x100000, 0xa9787090 )
	ROM_LOAD_QUAD( "c57-03", 0x000000, 0x100000, 0xfafca86f )

	ROM_REGION( 0x80000, REGION_USER1)
	ROM_LOAD( "c57-05", 0x00000, 0x80000, 0x6a70eb2e )	/* STY, index used to create 64x64 sprites on the fly */

	ROM_REGION( 0x80000, REGION_SOUND1 )	/* ADPCM samples */
	ROM_LOAD( "c57-07", 0x00000, 0x80000, 0xad653dc1 )

	ROM_REGION( 0x80000, REGION_SOUND2 )	/* delta-t samples */
	ROM_LOAD( "c57-08", 0x00000, 0x80000, 0x22593550 )

//	ROM_LOAD( "c57-09", 0x00000, 0xc00, 0x22593550 )	/* unknown roms, lengths checksums incorrect */
//	ROM_LOAD( "c57-10", 0x00000, 0xc00, 0x22593550 )
//	ROM_LOAD( "c57-11", 0x00000, 0xc00, 0x22593550 )
//	ROM_LOAD( "c57-12", 0x00000, 0xc00, 0x22593550 )
//	ROM_LOAD( "c57-13", 0x00000, 0xc00, 0x22593550 )
//	ROM_LOAD( "c57-14", 0x00000, 0xc00, 0x22593550 )
ROM_END

void init_taitoz(void)
{
	taito_soundsys_setz80_soundcpu( 2 );
}

void init_bshark(void)
{
	taito_soundsys_setz80_soundcpu( 1 );	// 68000 not yet supported in sndhrdw\taitosnd.c
}

READ_HANDLER( eerom_checksum_r )	/* kludge to pass Spacegun eerom test */
{
	return 0xffff;
}

void init_spacegun(void)
{
	install_mem_read_handler (0, 0x310db2, 0x310db3, eerom_checksum_r);
	init_bshark();
}


/* Busted Games */

GAMEX( 1988, chasehq,  0,        chasehq,  chasehq,  taitoz,   ROT0,               "Taito Corporation Japan", "Chase HQ (World)", GAME_NOT_WORKING )
GAMEX( 1988, chasehqj, chasehq,  chasehq,  chasehq,  taitoz,   ROT0,               "Taito Corporation", "Chase HQ (Japan)", GAME_NOT_WORKING )
GAMEX( 1989, chasehq2, 0,        chasehq2, chasehq2, taitoz,   ROT0,               "Taito Corporation Japan", "Chase HQ 2: SCI (World)", GAME_NOT_WORKING )
GAMEX( 1989, chashq2u, chasehq2, chasehq2, chasehq2, taitoz,   ROT0,               "Taito Corporation", "Chase HQ 2: SCI (US)", GAME_NOT_WORKING )
GAMEX( 1989, contcirc, 0,        contcirc, contcirc, taitoz,   ROT0,               "Taito Corporation Japan", "Continental Circus (World)", GAME_NOT_WORKING )
GAMEX( 1987, contcrcu, contcirc, contcirc, contcirc, taitoz,   ROT0,               "Taito America Corporation", "Continental Circus (US)", GAME_NOT_WORKING )
GAMEX( 1989, nightstr, 0,        nightstr, nightstr, taitoz,   ROT0,               "Taito America Corporation", "Night Striker (US)", GAME_NOT_WORKING )
GAMEX( 1989, bshark,   0,        bshark,   bshark,   bshark,   ORIENTATION_FLIP_X, "Taito America Corporation", "Battle Shark (US)", GAME_NOT_WORKING )
GAMEX( 1990, aquajack, 0,        aquajack, aquajack, taitoz,   ROT0,               "Taito Corporation Japan", "Aqua Jack (World)", GAME_NOT_WORKING )
GAMEX( 1990, aquajckj, aquajack, aquajack, aquajack, taitoz,   ROT0,               "Taito Corporation", "Aqua Jack (Japan)", GAME_NOT_WORKING )
GAMEX( 1990, spacegun, 0,        spacegun, spacegun, spacegun, ORIENTATION_FLIP_X, "Taito Corporation Japan", "Space Gun (World)", GAME_NOT_WORKING )


