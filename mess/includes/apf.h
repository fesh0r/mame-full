extern VIDEO_START( apf );

extern READ_HANDLER(apf_video_r);
extern WRITE_HANDLER(apf_video_w);

/* for .WAV */
DEVICE_LOAD( apf_cassette );

void apf_update_ints(void);

DEVICE_LOAD( apfimag_floppy );

