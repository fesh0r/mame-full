#include "formats/vt_cas.h"

#if 0
#define LO	-20000
#define HI	+20000

#define BITSAMPLES	18
#define BYTESAMPLES 8*BITSAMPLES
#define SILENCE 8000

static INT16 bit0[BITSAMPLES] =
{
	/* short cycle, long cycles */
	HI,HI,HI,LO,LO,LO,
	HI,HI,HI,HI,HI,HI,
	LO,LO,LO,LO,LO,LO
};

static INT16 bit1[BITSAMPLES] =
{
	/* three short cycle */
	HI,HI,HI,LO,LO,LO,
	HI,HI,HI,LO,LO,LO,
	HI,HI,HI,LO,LO,LO
};



#define LO	-32768
#define HI	+32767

#define BITSAMPLES	6
#define BYTESAMPLES 8*BITSAMPLES
#define SILENCE 8000

static INT16 *fill_wave_byte(INT16 *buffer, int byte)
{
	int i;

    for( i = 7; i >= 0; i-- )
	{
		*buffer++ = HI; 	/* initial cycle */
		*buffer++ = LO;
		if( (byte >> i) & 1 )
		{
			*buffer++ = HI; /* two more cycles */
            *buffer++ = LO;
			*buffer++ = HI;
            *buffer++ = LO;
        }
		else
		{
			*buffer++ = HI; /* one slow cycle */
			*buffer++ = HI;
			*buffer++ = LO;
			*buffer++ = LO;
        }
	}
    return buffer;
}

static int fill_wave(INT16 *buffer, int length, UINT8 *code)
{
	static int nullbyte;

    if( code == CODE_HEADER )
    {
		int i;

        if( length < SILENCE )
			return -1;
		for( i = 0; i < SILENCE; i++ )
			*buffer++ = 0;

		nullbyte = 0;

        return SILENCE;
    }

	if( code == CODE_TRAILER )
    {
		int i;

        /* silence at the end */
		for( i = 0; i < length; i++ )
			*buffer++ = 0;
        return length;
    }

    if( length < BYTESAMPLES )
		return -1;

	buffer = fill_wave_byte(buffer, *code);

    if( !nullbyte && *code == 0 )
	{
		int i;
		for( i = 0; i < 2*BITSAMPLES; i++ )
			*buffer++ = LO;
        nullbyte = 1;
		return BYTESAMPLES + 2 * BITSAMPLES;
	}

    return BYTESAMPLES;
}

#endif

/*struct CassetteFormat vt_cas_format =
{
	"vt\0",
	vt_cas_identify,
	vt_cas_load,
	NULL
};*/



CASSETTE_FORMATLIST_START(vt_cassette_formats)
	/* TODO - Readd support for VT Cassette files files */
	/*	CASSETTE_FORMAT(vt_cas_format) */
CASSETTE_FORMATLIST_END
