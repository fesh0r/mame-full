/****************************************************************************

	imghd.h

	Bridge between Imgtool and CHD hard disk images

****************************************************************************/

#ifndef IMGHD_H
#define IMGHD_H

#include "harddisk.h"

/* created a new hard disk */
imgtoolerr_t imghd_create_base_v1_v2(imgtool_stream *stream, UINT32 version, UINT32 blocksize, UINT32 cylinders, UINT32 heads, UINT32 sectors, UINT32 seclen);

/* opens a hard disk given an Imgtool stream */
imgtoolerr_t imghd_open(imgtool_stream *stream, struct hard_disk_file **hard_disk);

/* closed a hard disk */
void imghd_close(struct hard_disk_file *disk);

/* reads data from a hard disk */
UINT32 imghd_read(struct hard_disk_file *disk, UINT32 lbasector, UINT32 numsectors, void *buffer);

/* writes data to a hard disk */
UINT32 imghd_write(struct hard_disk_file *disk, UINT32 lbasector, UINT32 numsectors, const void *buffer);

/* gets the header from a hard disk */
const struct hard_disk_info *imghd_get_header(struct hard_disk_file *disk);

#endif /* IMGHD_H */
