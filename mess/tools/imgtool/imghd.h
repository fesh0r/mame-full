int imghd_create_base_v1_v2(STREAM *stream, UINT32 version, UINT32 blocksize, UINT32 cylinders, UINT32 heads, UINT32 sectors, UINT32 seclen);
void *imghd_open(STREAM *stream);
void imghd_close(void *disk);
UINT32 imghd_read(void *disk, UINT32 lbasector, UINT32 numsectors, void *buffer);
UINT32 imghd_write(void *disk, UINT32 lbasector, UINT32 numsectors, const void *buffer);
const struct hard_disk_info *imghd_get_header(struct hard_disk_file *disk);

