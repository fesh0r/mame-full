#ifndef IMAGE_H
#define IMAGE_H

#include <stdlib.h>
#include "fileio.h"
#include "utils.h"

/* not to be called by anything other than core */
int image_init(int type, int id);
void image_exit(int type, int id);

/* can be called by front ends */
int image_load(int type, int id, const char *name);
void image_unload(int type, int id);
void image_unload_all(int ispreload);

mame_file *image_fp(int type, int id);

/* memory allocators; these have the lifetime of the image */
void *image_malloc(int type, int id, size_t size) FUNCATTR_MALLOC;
char *image_strdup(int type, int id, const char *src) FUNCATTR_MALLOC;
void *image_realloc(int type, int id, void *ptr, size_t size);
void image_freeptr(int type, int id, void *ptr);

/* needs to be phased out */
mame_file *image_fopen_custom(int type, int id, int filetype, int read_or_write);

const char *image_typename_id(int type, int id);
const char *image_filename(int type, int id);
const char *image_basename(int type, int id);
const char *image_filetype(int type, int id);
const char *image_filedir(int type, int id);
int image_exists(int type, int id);
unsigned int image_length(int type, int id);
unsigned int image_crc(int type, int id);
const char *image_longname(int type, int id);
const char *image_manufacturer(int type, int id);
const char *image_year(int type, int id);
const char *image_playable(int type, int id);
const char *image_extrainfo(int type, int id);

#endif /* IMAGE_H */
