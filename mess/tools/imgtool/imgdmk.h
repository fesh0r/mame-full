int imgfloppy_dmkdsk_init(const struct ImageModule *mod, const struct FloppyFormat *flopformat, STREAM *f, IMAGE **outimg);
void imgfloppy_dmkdsk_exit(IMAGE *img);
int imgfloppy_dmkdsk_readdata(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, int length, UINT8 *buf);
int imgfloppy_dmkdsk_writedata(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, int length, const UINT8 *buf);
int imgfloppy_dmkdsk_create(const struct ImageModule *mod, const struct FloppyFormat *flopformat, STREAM *f, UINT8 sides, UINT8 tracks, UINT8 filler);
void imgfloppy_dmkdsk_get_geometry(IMAGE *img, UINT8 *sides, UINT8 *tracks, UINT8 *sectors);
int imgfloppy_dmkdsk_isreadonly(IMAGE *img);
