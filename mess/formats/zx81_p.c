/* .p tape images */
/*
ZX81 Cassette File Structure
  x seconds    your voice, saying "filename" (optional)
  x seconds    video noise
  5 seconds    silence
  1-127 bytes  filename (bit7 set in last char)
  LEN bytes    data, loaded to address 4009h, LEN=(4014h)-4009h.
  1 pulse      video retrace signal (only if display was enabled)
  x seconds    silence / video noise
The data field contains the system area, the basic program, the video memory,
and VARS area.

ZX80 Cassette File Structure
  x seconds    your voice, saying "filename" (optional)
  x seconds    video noise
  5 seconds    silence
  LEN bytes    data, loaded to address 4000h, LEN=(400Ah)-4000h.
  x seconds    silence / video noise
ZX80 files do not have filenames, and video memory is not included in the file.

File End
For both ZX80 and ZX81 the fileend is calculated as shown above. In either
case, the last byte of a (clean) file should be 80h (ie. the last byte of the
VARS area), not followed by any further signals except eventually video noise.

Bits and Bytes
Each byte consists of 8 bits (MSB first) without any start and stop bits,
directly followed by the next byte. A "0" bit consists of four high pulses, a
"1" bit of nine pulses, either one followed by a silence period.
  0:  /\/\/\/\________
  1:  /\/\/\/\/\/\/\/\/\________
Each pulse is split into a 150us High period, and 150us Low period. The
duration of the silence between each bit is 1300us. The baud rate is thus 400
bps (for a "0" filled area) downto 250 bps (for a "1" filled area). Average
medium transfer rate is approx. 307 bps (38 bytes/sec) for files that contain
50% of "0" and "1" bits each.
*/

#include "zx81_p.h"

#define WAVEENTRY_LOW  -32768
#define WAVEENTRY_HIGH  32767

#define ZX81_WAV_FREQUENCY	44100
#define ZX81_TIMER_FREQUENCY	5500

#define ZX81_PULSE_LENGTH	(ZX81_WAV_FREQUENCY/ZX81_TIMER_FREQUENCY)
#define ZX81_PAUSE_LENGTH	((ZX81_WAV_FREQUENCY/ZX81_TIMER_FREQUENCY)*4)

#define ZX81_LOW_BIT_LENGTH	(ZX81_PULSE_LENGTH*4+ZX81_PAUSE_LENGTH)
#define ZX81_HIGH_BIT_LENGTH	(ZX81_PULSE_LENGTH*9+ZX81_PAUSE_LENGTH)

/*
#define ZX81_PILOT_BITS	(zx81_TIMER_FREQUENCY*3)
#define ZX81_PAUSE_BITS	(zx81_TIMER_FREQUENCY/2)
*/

static INT16 *zx81_emit_level(INT16 *p, int count, int level)
{	
	int i;

	for (i=0; i<count; i++)	*(p++) = level;

	return p;
}

static INT16* zx81_emit_pulse(INT16 *p)
{
	p = zx81_emit_level (p, ZX81_PULSE_LENGTH/2, WAVEENTRY_HIGH);
	p = zx81_emit_level (p, ZX81_PULSE_LENGTH/2, WAVEENTRY_LOW);

	return p;
}

static INT16* zx81_emit_pause(INT16 *p)
{
	p = zx81_emit_level (p, ZX81_PAUSE_LENGTH, WAVEENTRY_LOW);

	return p;
}

static INT16* zx81_output_bit(INT16 *p, UINT8 bit)
{
	int i;

	if (bit)
		for (i=0; i<9; i++)
			p = zx81_emit_pulse (p);
	else
		for (i=0; i<4; i++)
			p = zx81_emit_pulse (p);

	p = zx81_emit_pause(p);

    	return p;
}

static INT16* zx81_output_byte(INT16 *p, UINT8 byte)
{
	int i;

	for (i=0; i<8; i++)
		p = zx81_output_bit(p,(byte>>(7-i)) & 0x01);

	return p;
}

static int zx81_cassette_calculate_size_in_samples(const UINT8 *bytes, int length)
{
	unsigned int number_of_0 = 0;
	unsigned int number_of_1 = 0;

	return number_of_0*ZX81_LOW_BIT_LENGTH + number_of_1*ZX81_HIGH_BIT_LENGTH;
}

static int zx81_cassette_fill_wave(INT16 *buffer, int length, UINT8 *bytes)
{
	int i;
	INT16 * p = buffer;

	return p - buffer;
}

static struct CassetteLegacyWaveFiller zx81_legacy_fill_wave =
{
	zx81_cassette_fill_wave,			/* fill_wave */
	-1,											/* chunk_size */
	0,											/* chunk_samples */
	zx81_cassette_calculate_size_in_samples,	/* chunk_sample_calc */
	ZX81_WAV_FREQUENCY,										/* sample_frequency */
	0,											/* header_samples */
	0											/* trailer_samples */
};

static casserr_t zx81_p_identify(cassette_image *cassette, struct CassetteOptions *opts)
{
	return cassette_legacy_identify(cassette, opts, &zx81_legacy_fill_wave);
}

static casserr_t zx81_p_load(cassette_image *cassette)
{
	return cassette_legacy_construct(cassette, &zx81_legacy_fill_wave);
}

struct CassetteFormat zx81_p_image_format =
{
	"pmd\0",
	zx81_p_identify,
	zx81_p_load,
	NULL
};

CASSETTE_FORMATLIST_START(zx81_p_format)
	CASSETTE_FORMAT(zx81_p_image_format)
CASSETTE_FORMATLIST_END
