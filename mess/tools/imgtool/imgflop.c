#include <assert.h>
#include <string.h>
#include "imgtool.h"

int imgfloppy_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg)
{
	struct FloppyExtra *extra;
	extra = (struct FloppyExtra *) mod->extra;
	return extra->flopformat->init(mod, extra->flopformat, f, outimg);
}

void imgfloppy_exit(IMAGE *img)
{
	struct FloppyExtra *extra;
	extra = (struct FloppyExtra *) img->module->extra;
	extra->flopformat->exit(img);
}

int imgfloppy_create(const struct ImageModule *mod, STREAM *f, const ResolvedOption *createoptions)
{
	int tracks, sides;
	struct FloppyExtra *extra;

	extra = (struct FloppyExtra *) mod->extra;
	sides = createoptions[IMGFLOP_CREATEOPTION_SIDES].i;
	tracks = createoptions[IMGFLOP_CREATEOPTION_TRACKS].i;

	return extra->flopformat->create(mod, extra->flopformat, f, sides, tracks, extra->filler);
}

int imgfloppy_readsector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, char **buffer, int *size)
{
	/* Not yet implemented */
	assert(0);
	return -1;
}

int imgfloppy_writesector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, char *buffer, int size)
{
	/* Not yet implemented */
	assert(0);
	return -1;
}

/* ------------------------------------------------------------------------ */

typedef struct {
	IMAGE base;
	STREAM *f;
	UINT8 tracks;
	UINT8 sides;
	UINT8 sec_per_track;
	UINT16 sector_length;
	UINT8 first_sector_id;
	int offset;
} imgfloppy_basicdsk;

int imgfloppy_basicdsk_init(const struct ImageModule *mod, const struct FloppyFormat *flopformat, STREAM *f, IMAGE **outimg)
{
	int err;
	imgfloppy_basicdsk *img;
	const struct BasicDskExtra *extra;

	img = malloc(sizeof(imgfloppy_basicdsk));
	if (!img) {
		err = IMGTOOLERR_OUTOFMEMORY;
		goto error;
	}
	memset(img, 0, sizeof(imgfloppy_basicdsk));

	extra = (const struct BasicDskExtra *) flopformat->extra;
	assert(extra->resolvegeometry);
	err = extra->resolvegeometry(f, &img->tracks, &img->sides, &img->sec_per_track, &img->sector_length, &img->first_sector_id, &img->offset);
	if (err)
		goto error;

	img->base.module = mod;
	img->f = f;
	*outimg = &img->base;
	return 0;

error:
	if (img)
		free(img);
	*outimg = NULL;
	return err;
}

void imgfloppy_basicdsk_exit(IMAGE *img)
{
	imgfloppy_basicdsk *fimg;

	fimg = (imgfloppy_basicdsk *) img;
	stream_close(fimg->f);
	free(fimg);
}

static int imgfloppy_basicdsk_seek(imgfloppy_basicdsk *fimg, UINT8 head, UINT8 track, UINT8 sector, int offset)
{
	size_t pos;
	pos = head;
	pos *= fimg->tracks;
	pos += track;
	pos *= fimg->sec_per_track;
	pos += sector - fimg->first_sector_id;
	pos *= fimg->sector_length;
	pos += fimg->offset;
	pos += offset;
	stream_seek(fimg->f, pos, SEEK_SET);	
	return 0;
}

int imgfloppy_basicdsk_readdata(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, int length, UINT8 *buf)
{
	int err;
	imgfloppy_basicdsk *fimg;

	fimg = (imgfloppy_basicdsk *) img;
	err = imgfloppy_basicdsk_seek(fimg, head, track, sector, offset);
	if (err)
		return err;

	if (stream_read(fimg->f, buf, length) != length)
		return IMGTOOLERR_READERROR;

	return 0;
}

int imgfloppy_basicdsk_writedata(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, int length, const UINT8 *buf)
{
	int err;
	imgfloppy_basicdsk *fimg;

	fimg = (imgfloppy_basicdsk *) img;
	err = imgfloppy_basicdsk_seek(fimg, head, track, sector, offset);
	if (err)
		return err;

	if (stream_write(fimg->f, buf, length) != length)
		return IMGTOOLERR_WRITEERROR;

	return 0;
}

int imgfloppy_basicdsk_create(const struct ImageModule *mod, const struct FloppyFormat *flopformat, STREAM *f, UINT8 sides, UINT8 tracks, UINT8 filler)
{
	int err;
	size_t sz;
	const struct BasicDskExtra *extra;
	UINT8 sec_per_track;
	UINT8 first_sector_id;
	UINT16 sector_length;

	extra = (const struct BasicDskExtra *) flopformat->extra;
	sec_per_track = extra->sec_per_track;
	first_sector_id = extra->first_sector_id;
	sector_length = extra->sector_length;

	if (extra->createheader) {
		err = extra->createheader(f, sides, tracks, sec_per_track, sector_length, first_sector_id);
		if (err)
			return err;
	}

	sz = sides;
	sz *= tracks;
	sz *= sec_per_track;
	sz *= sector_length;

	if (stream_fill(f, filler, sz) != sz)
		return IMGTOOLERR_WRITEERROR;

	return 0;
}

void imgfloppy_basicdsk_get_geometry(IMAGE *img, UINT8 *sides, UINT8 *tracks, UINT8 *sectors)
{
	imgfloppy_basicdsk *fimg;

	fimg = (imgfloppy_basicdsk *) img;
	*sides = fimg->sides;
	*tracks = fimg->tracks;
	*sectors = fimg->sec_per_track;
}

int imgfloppy_basicdsk_isreadonly(IMAGE *img)
{
	imgfloppy_basicdsk *fimg;
	fimg = (imgfloppy_basicdsk *) img;
	return stream_isreadonly(fimg->f);
}

/* ------------------------------------------------------------------------ */

int imgfloppy_read(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, int length, UINT8 *buf)
{
	struct FloppyExtra *extra;
	extra = (struct FloppyExtra *) img->module->extra;
	return extra->flopformat->readdata(img, head, track, sector, offset, length, buf);
}

int imgfloppy_write(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, int length, const UINT8 *buf)
{
	struct FloppyExtra *extra;
	extra = (struct FloppyExtra *) img->module->extra;
	return extra->flopformat->writedata(img, head, track, sector, offset, length, buf);
}

void imgfloppy_get_geometry(IMAGE *img, UINT8 *sides, UINT8 *tracks, UINT8 *sectors)
{
	UINT8 dummy;
	struct FloppyExtra *extra;

	/* Adding this code takes the responsibility of checking for NULL pointers away from the floppy modules */
	if (!sides)
		sides = &dummy;
	if (!tracks)
		tracks = &dummy;
	if (!sectors)
		sectors = &dummy;

	extra = (struct FloppyExtra *) img->module->extra;
	extra->flopformat->get_geometry(img, sides, tracks, sectors);
}

int imgfloppy_isreadonly(IMAGE *img)
{
	struct FloppyExtra *extra;
	extra = (struct FloppyExtra *) img->module->extra;
	return extra->flopformat->isreadonly(img);
}

int imgfloppy_transfer_from(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, int length, STREAM *destf)
{
	int err;
	UINT8 *buffer;

	buffer = (UINT8 *) malloc(length);
	if (!buffer) {
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}

	err = imgfloppy_read(img, head, track, sector, offset, length, buffer);
	if (err)
		goto done;

	if (stream_write(destf, buffer, length) != length) {
		err = IMGTOOLERR_WRITEERROR;
		goto done;
	}

done:
	if (buffer)
		free(buffer);
	return err;
}

int imgfloppy_transfer_to(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, int length, STREAM *sourcef)
{
	int err;
	UINT8 *buffer;

	buffer = (UINT8 *) malloc(length);
	if (!buffer) {
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}

	if (stream_read(sourcef, buffer, length) != length) {
		err = IMGTOOLERR_READERROR;
		goto done;
	}

	err = imgfloppy_write(img, head, track, sector, offset, length, buffer);
	if (err)
		goto done;

done:
	if (buffer)
		free(buffer);
	return err;
}



