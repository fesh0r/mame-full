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
#define WAVEENTRY_HIGH  -32768
#define WAVEENTRY_NULL  0

/* to write a bit to the tape, the rom routines output either 4 periods at 1200 Hz for a 0 or 8 periods at 2400 Hz for a 1  */
/* 4800 is twice 2400hz */

/* 8 periods at 2400hz */
/* hi,lo, hi,lo, hi,lo, hi,lo */

static UINT16 wave_state = WAVEENTRY_HIGH;
static UINT16 xor_wave_state = WAVEENTRY_HIGH^WAVEENTRY_LOW;

static INT16 *oric_emit_level(INT16 *p, int count)
{	
	int i;

	for (i=0; i<count; i++)
	{
		*(p++) = wave_state;
	}
	wave_state = wave_state^xor_wave_state;
	return p;
}

/* 4 periods at 1200hz */ 
static INT16* oric_output_bit(INT16 *p, UINT8 b)
{
	p = oric_emit_level(p,1);

	if (b)
	{
		p = oric_emit_level(p,1);
	}
	else
	{
		p = oric_emit_level(p,2);
	}

    return p;
}

static int oric_get_bit_size_in_samples(UINT8 b)
{
	int count;

	count = 1;

	if (b)
	{
		count++;
	}
	else
	{
		count+=2;
	}

	return count;
}


/*	each byte on cassette is stored as: 
	
	start bit		0 * 1
	data bits		8 * x (x is 0 or 1, and depends on data-bit value)
	parity bit		1 * x (x is 0 or 1, and depends on the parity of the data bits)
	stop bits		1 * 3

	if data has even parity, parity bit will be 1.
	if data has odd parity, parity bit will be 0.
*/

/*
	256 * 0x016		->		leader
	1	* 0x024		->		sync byte

					->		header
	3	* ?			->		???
	1	* ?			->		???
	1	* x			->		end address high byte
	1	* x			->		end address low byte
	1	* x			->		start address high byte
	1	* x			->		start address low byte
	1	* ?			->		???


	
	<pause>

	data

*/

static int oric_calculate_byte_size_in_samples(UINT8 byte)
{
	int count;
	int i;
	UINT8 parity;
	UINT8 data;

	count = 0;


	/* start bit */
	count+=oric_get_bit_size_in_samples(0);

	/* set initial parity */	
	parity = 1;
	
	/* data bits, written bit 0, bit 1...bit 7 */
	data = byte;
	for (i=0; i<8; i++)
	{
		UINT8 data_bit;

		data_bit = data & 0x01;

		parity = parity + data_bit;

		count+=oric_get_bit_size_in_samples(data_bit);
		data = data>>1;
	}

	/* parity */
	count+=oric_get_bit_size_in_samples((parity & 0x01));

	/* stop bits */
	count+=oric_get_bit_size_in_samples(1);
	count+=oric_get_bit_size_in_samples(1);
	count+=oric_get_bit_size_in_samples(1);
	count+=oric_get_bit_size_in_samples(1);

	return count;
}


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

static int oric_cassette_state;
static int oric_data_count;
static int oric_data_length;

/* length is length of .tap file! */
int oric_cassette_calculate_size_in_samples(int length, UINT8 *bytes)
{
	unsigned char header[9];
	int count;

	UINT8 *data_ptr;
	int i;
	UINT8 data;

	oric_cassette_state = ORIC_CASSETTE_SEARCHING_FOR_SYNC_BYTE;
	count = 0;
	data_ptr = bytes;

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
					/* 0.25 second pause */
					count += oric_seconds_to_samples(0.25);

					logerror("found end of sync bytes!\n");
					/* found end of sync bytes */
					for (i=0; i<256; i++)
					{
						count+=oric_calculate_byte_size_in_samples(0x016);
					}

					if (data==0x024)
					{
						//logerror("reading header!\n");
						count+=oric_calculate_byte_size_in_samples(0x024);
		
						oric_cassette_state = ORIC_CASSETTE_READ_HEADER;
						oric_data_count = 0;
						oric_data_length = 9;
					}
				}
			}
			break;

			case ORIC_CASSETTE_READ_HEADER:
			{
				header[oric_data_count] = data;
				count+=oric_calculate_byte_size_in_samples(data);
		
				oric_data_count++;

				if (oric_data_count==oric_data_length)
				{
					//logerror("finished reading header!\n");
					oric_cassette_state = ORIC_CASSETTE_READ_FILENAME;
				}
			}
			break;

			case ORIC_CASSETTE_READ_FILENAME:
			{
				count+=oric_calculate_byte_size_in_samples(data);

				/* got end of filename? */
				if (data==0)
				{
					UINT16 end, start;
					logerror("got end of filename\n");

					count+=oric_seconds_to_samples(0.25);

					oric_cassette_state = ORIC_CASSETTE_WRITE_DATA;
					oric_data_count = 0;
					
					end = (((header[4] & 0x0ff)<<8) | (header[5] & 0x0ff));
					start = (((header[6] & 0x0ff)<<8) | (header[7] & 0x0ff));
//#ifdef ORIC_WAV_DEBUG		
					logerror("start (from header): %02x\n",start);
					logerror("end (from header): %02x\n",end);
//#endif
					oric_data_length = end - start + 1;
				}
			}
			break;

			case ORIC_CASSETTE_WRITE_DATA:
			{
				count+=oric_calculate_byte_size_in_samples(data);
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
	
	return count;
}

/* length is length of sample buffer to fill! */
int oric_cassette_fill_wave(INT16 *buffer, int length, UINT8 *bytes)
{
	unsigned char header[9];
	UINT8 *data_ptr;
	INT16 *p;
	int i;
	UINT8 data;

	oric_cassette_state = ORIC_CASSETTE_SEARCHING_FOR_SYNC_BYTE;
	wave_state = WAVEENTRY_LOW;
	p = buffer;
	data_ptr = bytes;

	while (p<(buffer+length))
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
					/* 0.25 second pause */
					p = oric_fill_pause(p, oric_seconds_to_samples(0.25));

					logerror("found end of sync bytes!\n");
					/* found end of sync bytes */
					for (i=0; i<256; i++)
					{
						p = oric_output_byte(p,0x016);
					}

					if (data==0x024)
					{
						//logerror("reading header!\n");
						p = oric_output_byte(p,data);
						oric_cassette_state = ORIC_CASSETTE_READ_HEADER;
						oric_data_count = 0;
						oric_data_length = 9;
					}
				}
			}
			break;

			case ORIC_CASSETTE_READ_HEADER:
			{
				header[oric_data_count] = data;
				p = oric_output_byte(p, data);
				oric_data_count++;

				if (oric_data_count==oric_data_length)
				{
					//logerror("finished reading header!\n");
					oric_cassette_state = ORIC_CASSETTE_READ_FILENAME;
				}
			}
			break;

			case ORIC_CASSETTE_READ_FILENAME:
			{
				p = oric_output_byte(p, data);

				/* got end of filename? */
				if (data==0)
				{
					UINT16 end, start;
					logerror("got end of filename\n");

					p = oric_fill_pause(p, oric_seconds_to_samples(0.25));
					oric_cassette_state = ORIC_CASSETTE_WRITE_DATA;
					oric_data_count = 0;
					
					end = (((header[4] & 0x0ff)<<8) | (header[5] & 0x0ff));
					start = (((header[6] & 0x0ff)<<8) | (header[7] & 0x0ff));
//#ifdef ORIC_WAV_DEBUG		
					logerror("start (from header): %02x\n",start);
					logerror("end (from header): %02x\n",end);
//#endif
					oric_data_length = end - start + 1;
				}
			}
			break;

			case ORIC_CASSETTE_WRITE_DATA:
			{
				p = oric_output_byte(p, data);
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
