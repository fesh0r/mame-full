#include "orictap.h"

/* this code based heavily on tap2wav by Fabrice Frances */

#define ORIC_SYNC_BYTE	0x016

enum
{
	ORIC_CASSETTE_SEARCHING_FOR_SYNC_BYTE,
	ORIC_CASSETTE_GOT_SYNC_BYTE,
	ORIC_CASSETTE_READ_HEADER,
	ORIC_CASSETTE_READ_FILENAME,
	ORIC_CASSETTE_WRITE_DATA
};

#define WAVEENTRY_LOW  32767
#define WAVEENTRY_HIGH   -32768
#define WAVEENTRY_NULL  0

/* to write a bit to the tape, the rom routines output either 4 periods at 1200 Hz for a 0 or 8 periods at 2400 Hz for a 1  */
/* 4800 is twice 2400hz */

/* 8 periods at 2400hz */
/* hi,lo, hi,lo, hi,lo, hi,lo */

static UINT16 wave_state = WAVEENTRY_HIGH;
static UINT16 xor_wave_state = WAVEENTRY_HIGH^WAVEENTRY_LOW;

static INT16 *oric_emit_level(INT16 *p)
{
	*(p++) = wave_state;
	wave_state = wave_state^xor_wave_state;
	return p;
}

/* 4 periods at 1200hz */ 
static INT16* oric_output_bit(INT16 *p, UINT8 b)
{
	p = oric_emit_level(p);

	if (b)
	{
		p = oric_emit_level(p);
	}
	else
	{
		p = oric_emit_level(p);
		p = oric_emit_level(p);
	}

    return p;
}

/*	each byte on cassette is stored as: 
	
	start bit		0 * 1
	data bits		8 * x (x is 0 or 1, and depends on data-bit value)
	parity bit		1 * x (x is 0 or 1, and depends on the parity of the data bits)
	stop bits		1 * 3

	if data has even parity, parity bit will be 1.
	if data has odd parity, parity bit will be 0.
*/
static INT16 *oric_output_byte(INT16 *p, UINT8 byte)
{
	int i;
	UINT8 parity;
	UINT8 data;

	/* start bit */
	p = oric_output_bit(p, 0);
	
	/* set initial parity */	
	parity = 1;
	
	/* data bits, written bit 0, bit 1...bit 7 */
	data = byte;
	for (i=0; i<8; i++)
	{
		UINT8 data_bit;

		data_bit = data & 0x01;

		parity = parity + data_bit;

		p = oric_output_bit(p, data_bit);
		data = data>>1;
	}

	/* parity */
	p = oric_output_bit(p, parity & 0x01);

	/* stop bits */
	p = oric_output_bit(p, 1);
	p = oric_output_bit(p, 1);
	p = oric_output_bit(p, 1);
	p = oric_output_bit(p, 1);

	return p;
}

static INT16 *oric_fill_pause(INT16 *p, int sample_count)
{
	int i;

	for (i=0; i<sample_count; i++)
	{
		*(p++) = WAVEENTRY_NULL;
	}

	return p;
}

static int oric_seconds_to_samples(float seconds)
{
	return (int)((float)seconds*(float)ORIC_WAV_FREQUENCY);
}

int oric_wave_size;
static int oric_cassette_state;
static int oric_data_count;
static int oric_data_length;

int oric_cassette_fill_wave(INT16 *buffer, int length, UINT8 *bytes)
{
	unsigned char header[9];
	UINT8 *data_ptr;
	INT16 *p;
	int i;
	UINT8 data;

	oric_cassette_state = ORIC_CASSETTE_SEARCHING_FOR_SYNC_BYTE;
	wave_state = WAVEENTRY_HIGH;
	p = buffer;
	data_ptr = bytes;

	/* 0.5 second pause */
	p = oric_fill_pause(p, oric_seconds_to_samples(0.5));

	while (data_ptr!=(bytes+length))
	{
		data = data_ptr[0];
		data_ptr++;

		switch (oric_cassette_state)
		{
			case ORIC_CASSETTE_SEARCHING_FOR_SYNC_BYTE:
			{
				if (data==ORIC_SYNC_BYTE)
				{
					logerror("found sync byte!\n");
					/* found first sync byte */
					oric_cassette_state = ORIC_CASSETTE_GOT_SYNC_BYTE;
				}
			}
			break;

			case ORIC_CASSETTE_GOT_SYNC_BYTE:
			{
				if (data!=ORIC_SYNC_BYTE)
				{
					logerror("found end of sync bytes!\n");
					/* found end of sync bytes */
					for (i=0; i<256; i++)
					{
						oric_output_byte(p,0x016);
					}

					if (data==0x024)
					{
						logerror("reading header!\n");
						oric_output_byte(p,data);
						oric_cassette_state = ORIC_CASSETTE_READ_HEADER;
						oric_data_count = 0;
						oric_data_length = 9;
					}
				}
			}
			break;

			case ORIC_CASSETTE_READ_HEADER:
			{
				oric_output_byte(p, header[oric_data_count]);
				oric_data_count++;

				if (oric_data_count==oric_data_length)
				{
					logerror("finished reading header!\n");
					oric_cassette_state = ORIC_CASSETTE_READ_FILENAME;
				}
			}
			break;

			case ORIC_CASSETTE_READ_FILENAME:
			{
				oric_output_byte(p, data);

				/* got end of filename? */
				if (data==0)
				{
					UINT16 end, start;
					logerror("got end of filename\n");

					oric_fill_pause(p, oric_seconds_to_samples(0.25));
					oric_cassette_state = ORIC_CASSETTE_WRITE_DATA;
					oric_data_count = 0;
					
					end = (((header[4] & 0x0ff)<<8) | (header[5] & 0x0ff));
					start = (((header[6] & 0x0ff)<<8) | (header[7] & 0x0ff));
					
					logerror("end (from header): %02x\n",end);
					logerror("start (from header): %02x\n",start);
					oric_data_length = end - start + 1;
				}
			}
			break;

			case ORIC_CASSETTE_WRITE_DATA:
			{
				oric_output_byte(p, data);
				oric_data_count++;

				if (oric_data_count==oric_data_length)
				{
					logerror("finished writing data!\n");
					oric_cassette_state = ORIC_CASSETTE_SEARCHING_FOR_SYNC_BYTE;
				}
			}
			break;

		}
	}
	
	return p - buffer;
}

