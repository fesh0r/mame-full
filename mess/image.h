#ifndef IMAGE_H
#define IMAGE_H

#include <stdlib.h>

extern int images_is_running;

extern int image_load(int type, int id, const char *name);
extern void image_unload(int type, int id);
extern void image_unload_all(void);

extern void *image_fp(int type, int id);

extern void *image_malloc(int type, int id, size_t size);
extern char *image_strdup(int type, int id, const char *src);

/* needs to be phased out */
extern void *image_fopen_custom(int type, int id, int filetype, int read_or_write);

extern const char *image_typename_id(int type, int id);
extern const char *image_filename(int type, int id);
extern const char *image_basename(int type, int id);
extern const char *image_filetype(int type, int id);
extern const char *image_filedir(int type, int id);
extern int image_exists(int type, int id);
extern unsigned int image_length(int type, int id);
extern unsigned int image_crc(int type, int id);
extern const char *image_longname(int type, int id);
extern const char *image_manufacturer(int type, int id);
extern const char *image_year(int type, int id);
extern const char *image_playable(int type, int id);
extern const char *image_extrainfo(int type, int id);

#endif /* IMAGE_H */
