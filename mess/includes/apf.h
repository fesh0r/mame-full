extern VIDEO_START( apf );

extern  READ8_HANDLER(apf_video_r);
extern WRITE8_HANDLER(apf_video_w);

void apf_update_ints(void);

DEVICE_LOAD( apfimag_floppy );

