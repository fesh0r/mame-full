/***************************************************************************

	stream.h

	Code for implementing Imgtool streams

***************************************************************************/

#ifndef STREAM_H
#define STREAM_H

#include "imgterrs.h"

typedef struct _imgtool_stream imgtool_stream;

imgtool_stream *stream_open(const char *fname, int read_or_write);	/* similar params to mame_fopen */
imgtool_stream *stream_open_write_stream(int filesize);
imgtool_stream *stream_open_mem(void *buf, size_t sz);
void stream_close(imgtool_stream *f);
size_t stream_read(imgtool_stream *f, void *buf, size_t sz);
size_t stream_write(imgtool_stream *f, const void *buf, size_t sz);
UINT64 stream_size(imgtool_stream *f);
int stream_seek(imgtool_stream *f, size_t pos, int where);
size_t stream_tell(imgtool_stream *s);
void stream_clear(imgtool_stream *f);
void *stream_getptr(imgtool_stream *f);

/* Transfers sz bytes from source to dest */
size_t stream_transfer(imgtool_stream *dest, imgtool_stream *source, size_t sz);
size_t stream_transfer_all(imgtool_stream *dest, imgtool_stream *source);

/* Fills sz bytes with b */
size_t stream_fill(imgtool_stream *f, unsigned char b, size_t sz);

/* Returns the CRC of a file */
int stream_crc(imgtool_stream *f, unsigned long *result);
int file_crc(const char *fname,  unsigned long *result);

/* Returns whether a stream is read only or not */
int stream_isreadonly(imgtool_stream *f);

#endif /* STREAM_H */
