void *imghd_open(STREAM *stream);
void imghd_close(void *disk);
UINT32 imghd_read(void *disk, UINT32 lbasector, UINT32 numsectors, void *buffer);
UINT32 imghd_write(void *disk, UINT32 lbasector, UINT32 numsectors, const void *buffer);
const struct hard_disk_header *imghd_get_header(void *disk);
