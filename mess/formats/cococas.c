#include "cococas.h"

#ifndef LOG_WAVE
#define LOG_WAVE 0
#endif

#define WAVEENTRY_LOW  32767
#define WAVEENTRY_HIGH   -32768
#define WAVEENTRY_NULL  0
#define WAVESAMPLES_BYTE    8*4

static INT16* fill_wave_byte(INT16 *p, UINT8 b)
{
    int i;
    /* Each byte in a .CAS file is read bit by bit, starting at bit 0, and
     * ending with bit 7.  High bits are decoded into {l,h} (a 2400hz pulse)
     * and low bits are decoded into {l,l,h,h} (a 1200hz pulse)
     */
    for (i = 0; i < 8; i++) {
        *(p++) = WAVEENTRY_LOW;
        if (((b >> i) & 0x01) == 0) {
            *(p++) = WAVEENTRY_LOW;
            *(p++) = WAVEENTRY_HIGH;
        }
        *(p++) = WAVEENTRY_HIGH;
    }
    return p;
}

static int get_cas_block(const UINT8 *bytes, int length, UINT8 *block, int *blocklen)
{
	int i, j, state, phase=0;
	UINT8 newb, b;
	UINT8 block_length=0, block_checksum=0;

	b = 0x00;
	state = 0;

	for (i = 0; i < length; i++) {
		newb = bytes[i];
		for (j = 0; j < 8; j++) {
			b >>= 1;
			if (newb & 1)
				b |= 0x80;
			newb >>= 1;

			if (state == 0) {
				/* Searching for a block */
				if (b == 0x3c) {
					/* Found one! */
					phase = j;
					state = 1;
				}
			}
			else if (j == phase) {
				switch(state) {
				case 1:
					/* Found file type */
					block_checksum = b;
					state++;
					break;

				case 2:
					/* Found file size */
					block_length = b;
					block_checksum += b;
					state++;
					*blocklen = ((int) block_length) + 3;
					break;

				case 3:
					/* Data byte */
					if (block_length) {
						block_length--;
						block_checksum += b;
					}
					else {
						/* End of block! check the checksum */
						if (b != block_checksum) {
							/* Checksum error */
							return 0;
						}
						/* We got a block! Return new position */
						*block = b;
						return i + 1;
					}
					break;

				}
				*(block++) = b;
			}
		}
	}
	/* Couldn't find a block */
	return 0;
}

int coco_wave_size;

int coco_cassette_fill_wave(INT16 *buffer, int length, UINT8 *bytes)
{
	int i, usedbytes, blocklen, position;
	UINT8 last_blocktype;
    INT16 *p;
	UINT8 block[258];	/* 255 bytes per block + 3 (type, length, checksum) */

    p = buffer;

    if (bytes == CODE_HEADER) {
        for (i = 0; i < COCO_WAVESAMPLES_HEADER; i++)
            *(p++) = WAVEENTRY_NULL;
    }
    else if (bytes == CODE_TRAILER) {
        /* Fill in one magic byte; then the empty trailer */
        for (i = fill_wave_byte(p, 0x55) - p; i < COCO_WAVESAMPLES_TRAILER; i++)
            *(p++) = WAVEENTRY_NULL;
    }
    else {
		/* This is the actual code that processes the CAS data.  CAS files are
		 * a problem because they are a legacy of previous CoCo emulators, and
		 * most of these emulators patch the system as a shortcut.  In doing
		 * so, they make the CoCo more tolerant of short headers and lack of
		 * delays between blocks.  This legacy is reflected in most CAS files
		 * in use, and thus presents a problem for a pure hardware emulation
		 * like MESS.
		 *
		 * One alternative is to preprocess the data on the CAS, file by file,
		 * but this proves to be problematic because in the process, legitimate
		 * data that is unrecognized by the preprocessor may get dropped.
		 *
		 * The approach taken here is a hybrid approach - it retrieves the data
		 * block by block until an error occurs; be it the end of the CAS or a
		 * corrupt (?!) block.  When "legitimate" blocks are done processing,
		 * the remainder of the data is added to the waveform in a traditional
		 * manner.  The result has proven to work quite well.
		 *
		 * One slight issue; strict bounds checking is not done and as such,
		 * this code could theoretically overflow.  However, I made sure that
		 * double the amount of required memory was set aside so such overflows
		 * may be very rare in practice
		 */

		position = 0;
		last_blocktype = 0;

		while((usedbytes = get_cas_block(&bytes[position], coco_wave_size, block, &blocklen)) > 0) {
			#if LOG_WAVE
				logerror("COCO found block; position=0x%06x type=%i len=%i\n", position, (int) block[0], blocklen);
			#endif

			/* was the last block a filename block? */
			if ((last_blocktype == 0) || (last_blocktype == 0xff)) {
				#if LOG_WAVE
					logerror("COCO filling silence %d\n", WAVESAMPLES_HEADER);
				#endif

				/* silence */
				for (i = 0; i < COCO_WAVESAMPLES_HEADER; i++)
					*(p++) = WAVEENTRY_NULL;
				/* sync data */
				for (i = 0; i < 128; i++)
					p = fill_wave_byte(p, 0x55);
			}
			/* Now fill in the magic bytes */
			p = fill_wave_byte(p, 0x55);
			p = fill_wave_byte(p, 0x3c);

			/* Write the block */
			for (i = 0; i < blocklen; i++)
				p = fill_wave_byte(p, block[i]);

			/* Now fill in the last magic byte */
			p = fill_wave_byte(p, 0x55);

			/* Move the pointers ahead etc */
			position += usedbytes;
			coco_wave_size -= usedbytes;
			last_blocktype = block[0];
		}

		/* We havn't been able to decipher any further blocks; so we are going
		 * to output the rest of the CAS verbatim
		 */
		#if LOG_WAVE
			logerror("COCO leaving %i extraneous bytes; position=%i\n", coco_wave_size, position);
		#endif

		for (i = 0; i < coco_wave_size ; i++)
			p = fill_wave_byte(p, bytes[position + i]);
    }
    return p - buffer;
}

