/*************************************************************************

	sndhrdw\vicdual.c

*************************************************************************/

#include "driver.h"
#include "vicdual.h"


WRITE8_HANDLER( frogs_sh_port2_w )
{
		discrete_sound_w(2, data & 0x01);			// Hop
		discrete_sound_w(1, (data & 0x02) >> 1);	// Jump
		discrete_sound_w(3, (data & 0x04) >> 2);	// Tongue
		discrete_sound_w(4, (data & 0x08) >> 3);	// Capture
		discrete_sound_w(0, (data & 0x10) >> 4);	// Fly
		discrete_sound_w(5, (data & 0x80) >> 7);	// Splash
}

/************************************************************************
 * frogs Sound System Analog emulation
 * Oct 2004, Derrick Renaud
 ************************************************************************/

/* Nodes - Inputs */
#define FROGS_FLY_EN		NODE_01
#define FROGS_JUMP_EN		NODE_03
#define FROGS_HOP_EN		NODE_04
#define FROGS_TONGUE_EN		NODE_05
#define FROGS_CAPTURE_EN	NODE_06
#define FROGS_SPLASH_EN		NODE_08
/* Nodes - Sounds */
#define FROGS_BUZZZ_SND		NODE_11
#define FROGS_BOING_SND		NODE_13
#define FROGS_HOP_SND		NODE_14
#define FROGS_ZIP_SND		NODE_15
#define FROGS_CROAK_SND		NODE_16
#define FROGS_SPLASH_SND	NODE_18
/* VRs */
#define FROGS_R93			NODE_25

const struct discrete_555_desc frogsZip555m =
{
	DISC_555_TRIGGER_IS_LOGIC | DISC_555_OUT_DC | DISC_555_OUT_CAP,
	12,		// B+ voltage of 555
	DEFAULT_555_VALUES
};

const struct discrete_555_cc_desc frogsZip555cc =
{
	DISC_555_OUT_CAP | DISC_555_OUT_AC,
	12,		// B+ voltage of 555
	DEFAULT_555_VALUES,
	12,		// B+ voltage of the Constant Current source
	0.7		// Q13 Vbe
};

const struct discrete_mixer_desc frogsMixer =
{
	DISC_MIXER_IS_OP_AMP, 2,
	{RES_K(1), RES_K(5), 0,0,0,0,0,0},
	{FROGS_R93, 0,0,0,0,0,0,0},
	{CAP_U(0.01), CAP_U(0.01), 0,0,0,0,0,0},
	0, RES_K(56), 0, CAP_U(0.1), 0, 10000
};

DISCRETE_SOUND_START(frogs_discrete_interface)
	/************************************************
	 * Input register mapping for skydiver
	 *
	 * All inputs are inverted by initial transistor.
	 ************************************************
	 *              NODE              ADDR  MASK     GAIN    OFFSET  INIT */
	DISCRETE_INPUTX(FROGS_FLY_EN,     0x00, 0x000f,  -1,     1,       0.0)
	DISCRETE_INPUTX(FROGS_JUMP_EN,    0x01, 0x000f,  -1,     1,       0.0)
	DISCRETE_INPUTX(FROGS_HOP_EN,     0x02, 0x000f,  -1,     1,       0.0)
	DISCRETE_INPUTX(FROGS_TONGUE_EN,  0x03, 0x000f,  -1,     1,       0.0)
	DISCRETE_INPUTX(FROGS_CAPTURE_EN, 0x04, 0x000f,  -1,     1,       0.0)
	DISCRETE_INPUTX(FROGS_SPLASH_EN,  0x05, 0x000f,  -1,     1,       0.0)

	DISCRETE_ADJUSTMENT(FROGS_R93, 1, RES_M(1), RES_K(10), DISC_LOGADJ, 2)

	DISCRETE_555_MSTABLE(NODE_30, 1, FROGS_TONGUE_EN, RES_K(100), CAP_U(1), &frogsZip555m)

	/* Q11 & Q12 transform the voltage from the oneshot U4, to what is
	 * needed by the 555CC circuit.  Vin to R29 must be > 1V for things
	 * to change.  <=1 then The Vout of this circuit is 12V.
	 * The Current thru R28 equals current thru R51. iR28 = iR51
	 * So when Vin>1.2, iR51 = (Vin-1)/39k.  =0 when Vin<=1
	 * So the voltage drop across R28 is vR28 = iR51 * 22k.
	 * Finally the Vout = 12 - vR28.
	 * Note this formula only works when Vin < 39/(22+39)*12V+1.
	 * Which it always is, due to the 555 clamping to 12V*2/3.
	 */
	DISCRETE_TRANSFORM5(NODE_31, 1, 12, NODE_30, 1, RES_K(22)/RES_K(39), 0, "012-4>12-*3*-")

	DISCRETE_555_CC(NODE_32, 1, NODE_31, RES_K(1), CAP_U(0.1), 0, 0, 0, &frogsZip555cc)

	DISCRETE_MIXER2(NODE_90, 1, NODE_32, 0, &frogsMixer)

	DISCRETE_OUTPUT(NODE_90, 100)

DISCRETE_SOUND_END
