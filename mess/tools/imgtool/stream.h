#ifndef STREAM_H
#define STREAM_H

#include "imgterrs.h"

typedef struct _imgtool_stream STREAM;

STREAM *stream_open(const char *fname, int read_or_write);	/* similar params to mame_fopen */
STREAM *stream_open_write_stream(int filesize);
STREAM *stream_open_mem(void *buf, size_t sz);
void stream_close(STREAM *f);
size_t stream_read(STREAM *f, void *buf, size_t sz);
size_t stream_write(STREAM *f, const void *buf, size_t sz);
size_t stream_size(STREAM *f);
int stream_seek(STREAM *f, size_t pos, int where);
size_t stream_tell(STREAM *s);
/* works currently only for IMG_FILE
   clears FILE on harddisk! */
void stream_clear(STREAM *f);
/* truncate with ansi c library not easy to implement */
//void stream_truncate(STREAM *f, size_t newsize);

/* Transfers sz bytes from source to dest */
size_t stream_transfer(STREAM *dest, STREAM *source, size_t sz);
size_t stream_transfer_all(STREAM *dest, STREAM *source);

/* Fills sz bytes with b */
size_t stream_fill(STREAM *f, unsigned char b, size_t sz);

/* Returns the CRC of a file */
int stream_crc(STREAM *f, unsigned long *result);
int file_crc(const char *fname,  unsigned long *result);

/* Returns whether a stream is read only or not */
int stream_isreadonly(STREAM *f);

#endif /* STREAM_H */
