#include <assert.h>
#include <string.h>

#include "utils.h"
#include "formats/wavfile.h"

UINT32 wavfile_decode_header(const struct wav_header *hdr)
{
	UINT32 file_size;

	assert(sizeof(struct wav_header) == 12);

	if (memcmp(hdr->magic1, "RIFF", 4))
		return 0;
	if (memcmp(hdr->magic2, "WAVE", 4))
		return 0;
	
	file_size = LITTLE_ENDIANIZE_INT32(hdr->file_size);
	return file_size;
}

int wavfile_decode_tag(const struct wav_tag *tag, UINT32 *tag_size)
{
	int tag_type;

	if (tag_size)
		*tag_size = LITTLE_ENDIANIZE_INT32(tag->tag_size);

	if (!memcmp(tag->tag_type, "fmt ", 4))
		tag_type = WAV_TAGTYPE_FMT;
	else if (!memcmp(tag->tag_type, "data", 4))
		tag_type = WAV_TAGTYPE_DATA;
	else
		tag_type = WAV_TAGTYPE_UNKNOWN;

	return tag_type;
}

