extern int apf_vh_start (void);
extern void apf_vh_stop (void);

extern READ_HANDLER(apf_video_r);
extern WRITE_HANDLER(apf_video_w);

/* for .WAV */
extern int apf_cassette_init(int);
extern void apf_cassette_exit(int);

void	apf_update_ints(void);

