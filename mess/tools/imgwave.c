#include <string.h>
#include "imgtool.h"

typedef struct {
	struct ImageModule *mod;
	STREAM *f;
	int base;
	int length;
	int curpos;

	int channels;
	int resolution;
	int frequency;
} waveimage;

typedef struct {
	waveimage *wimg;
	int pos;
} waveimageenum;

static int find_wavtag(STREAM *f, int filelen, const char *tag, int *offset, UINT32 *blocklen)
{
	char buf[4];

	while(1) {
		*offset += stream_read(f, buf, sizeof(buf));
		*offset += stream_read(f, blocklen, sizeof(*blocklen));

		if (!memcmp(buf, "fmt ", 4))
			break;

		*blocklen = LITTLE_ENDIANIZE_INT32(*blocklen);
		stream_seek(f, *blocklen, SEEK_CUR);
		*offset += *blocklen;

		if (*offset >= filelen)
			return IMGTOOLERR_CORRUPTIMAGE;
	}
	return 0;
}

int imgwave_init(struct ImageModule *mod, STREAM *f, IMAGE **outimg)
{
	int err;
	waveimage *wimg;
	char buf[4];
	UINT16 format, channels, resolution;
	UINT32 filelen, blocklen, frequency;
	int offset = 0;

	offset += stream_read(f, buf, sizeof(buf));
	if (offset < 4)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (memcmp(buf, "RIFF", 4))
		return IMGTOOLERR_CORRUPTIMAGE;

	offset += stream_read(f, &filelen, sizeof(filelen));
	if (offset < 8)
		return IMGTOOLERR_CORRUPTIMAGE;
	filelen = LITTLE_ENDIANIZE_INT32(filelen);

	offset += stream_read(f, buf, sizeof(buf));
	if (offset < 12)
		return IMGTOOLERR_CORRUPTIMAGE;
	if (memcmp(buf, "WAVE", 4))
		return IMGTOOLERR_CORRUPTIMAGE;

	err = find_wavtag(f, filelen, "fmt ", &offset, &blocklen);
	if (err)
		return err;

	offset += stream_read(f, &format, sizeof(format));
	format = LITTLE_ENDIANIZE_INT16(format);

	offset += stream_read(f, &channels, sizeof(channels));
	channels = LITTLE_ENDIANIZE_INT16(channels);
	if (channels != 1 && channels != 2)
		return IMGTOOLERR_CORRUPTIMAGE;

	offset += stream_read(f, &frequency, sizeof(frequency));
	frequency = LITTLE_ENDIANIZE_INT32(frequency);

	stream_seek(f, 6, SEEK_CUR);

	offset += stream_read(f, &resolution, sizeof(resolution));
	resolution = LITTLE_ENDIANIZE_INT16(resolution);
	if ((resolution != 8) || (resolution != 16))
		return IMGTOOLERR_CORRUPTIMAGE;

	stream_seek(f, 16 - blocklen, SEEK_CUR);

	err = find_wavtag(f, filelen, "data", &offset, &blocklen);
	if (err)
		return err;

	wimg = malloc(sizeof(waveimage));
	if (!wimg)
		return IMGTOOLERR_OUTOFMEMORY;

	wimg->mod = mod;
	wimg->f = f;
	wimg->base = wimg->curpos = offset;
	wimg->length = blocklen;
	wimg->channels = channels;
	wimg->frequency = frequency;
	wimg->resolution = resolution;

	return IMGTOOLERR_UNIMPLEMENTED;
}

void imgwave_exit(IMAGE *img)
{
	waveimage *wimg = (waveimage *) img;
	stream_close(wimg->f);
	free(wimg);
}

int imgwave_seek(IMAGE *img, int pos)
{
	waveimage *wimg = (waveimage *) img;

	if (wimg->curpos != pos) {
		stream_seek(wimg->f, pos, SEEK_SET);
		wimg->curpos = pos;
	}
	return 0;
}

int imgwave_readsample(IMAGE *img, INT16 *sample)
{
	int len;
	INT16 s;
	waveimage *wimg = (waveimage *) img;

	union {
		INT8 buf8[2];
		INT16 buf16[2];
	} u;

	if (wimg->resolution == 8) {
		len = stream_read(wimg->f, u.buf8, wimg->channels);
		wimg->curpos += len;
		if (len < wimg->channels)
			return IMGTOOLERR_CORRUPTIMAGE;

		switch(wimg->channels) {
		case 1:
			s = ((INT16) u.buf8[0]) - 0x80;
			s *= 0x101;
			break;
		case 2:
			s = ((INT16) u.buf8[0]) - 0x80;
			s += ((INT16) u.buf8[1]) - 0x80;
			s *= 0x80;
			break;
		}
	}
	else {
		len = stream_read(wimg->f, u.buf16, wimg->channels * 2);
		wimg->curpos += len;
		if (len < (wimg->channels * 2))
			return IMGTOOLERR_CORRUPTIMAGE;

		switch(wimg->channels) {
		case 1:
			s = u.buf16[0];
			break;
		case 2:
			s = (u.buf16[0] / 2) + (u.buf16[1] / 2);
			break;
		}
	}
	*sample = s;
	return 0;
}

int imgwave_read(IMAGE *img, UINT8 *buf, int bufsize)
{
	int i;
	for (i = 0; i < bufsize; i++) {
		
	}
	return IMGTOOLERR_UNIMPLEMENTED;
}

int imgwave_beginenum(IMAGE *img, IMAGEENUM **outenum)
{
	waveimage *wimg = (waveimage *) img;
	waveimageenum *wenum;

	wenum = malloc(sizeof(waveimageenum));
	if (!wenum)
		return IMGTOOLERR_OUTOFMEMORY;

	wenum->wimg = wimg;
	wenum->pos = wimg->base;
	*outenum = (IMAGEENUM *) wenum;
	return 0;
}

int imgwave_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
{
	int err;
	waveimageenum *wenum;
	
	wenum = (waveimageenum *) enumeration;

	err = imgwave_seek((IMAGE *) wenum->wimg, wenum->pos);
	if (err)
		return err;

	err = ((struct WaveExtra *) wenum->wimg->mod->extra)->nextfile((IMAGE *) wenum->wimg, ent);
	if (err)
		return err;

	return 0;
}

void imgwave_closeenum(IMAGEENUM *enumeration)
{
	waveimageenum *wenum = (waveimageenum *) enumeration;
	free(wenum);
}



