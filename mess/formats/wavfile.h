/*********************************************************************

	wavfile.h

	Application neutral .WAV file decoding helpers

*********************************************************************/

#ifndef WAVFILE_H
#define WAVFILE_H

#include "osd_cpu.h"

/***************************************************************************

	Constants

***************************************************************************/

#define WAV_FORMAT_PCM	1

enum
{
	WAV_TAGTYPE_UNKNOWN,
	WAV_TAGTYPE_FMT,
	WAV_TAGTYPE_DATA
};

/***************************************************************************

	Type definitions

***************************************************************************/

struct wav_header
{
	char magic1[4];		/* magic bytes; "RIFF" */
	UINT32 file_size;
	char magic2[4];		/* magic bytes; "WAVE" */
};

struct wav_tag
{
	char tag_type[4];
	UINT32 tag_size;
};

struct wav_tag_fmt
{
	UINT16 format_type;
	UINT16 channels;
	UINT32 sample_rate;
	UINT32 bytes_per_second;
	UINT16 block_align;
	UINT16 bits_per_sample;
};

/***************************************************************************

	Prototypes

***************************************************************************/

/* decodes a header; returns file size on success; 0 on failure */
UINT32 wavfile_decode_header(const struct wav_header *hdr);

/* decodes a tag; returns tag type and optionally, tag size */
int wavfile_decode_tag(const struct wav_tag *tag, UINT32 *tag_size);

#endif /* WAVFILE_H */



