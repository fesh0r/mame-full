extern VIDEO_START( apf );

extern READ_HANDLER(apf_video_r);
extern WRITE_HANDLER(apf_video_w);

/* for .WAV */
extern int apf_cassette_init(int, mame_file *file, int effective_mode);
extern void apf_cassette_exit(int);

void	apf_update_ints(void);

extern int apfimag_floppy_init(mess_image *img, mame_file *fp, int open_mode);

