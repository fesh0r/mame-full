
#ifndef _SMSVDP_H_
#define _SMSVDP_H_

/* Status register flags */

#define STATUS_VIRQ     0x80    /* Interrupt on vertical blank */
#define STATUS_HIRQ     0x40    /* Interrupt on horizontal blank */
#define STATUS_SPRCOL   0x20    /* Sprite collision */

/* RAM access flags */

#define MODE_VRAM       0x00
#define MODE_CRAM       0x01

/* Function prototypes */

int sms_vdp_start(void);
int gamegear_vdp_start(void);
int sms_vdp_curline_r (int offset);
int SMSVDP_start (int vdp_type);
void sms_vdp_stop(void);
int sms_vdp_interrupt(void);
int sms_vdp_data_r(int offset);
void sms_vdp_data_w(int offset, int data);
int sms_vdp_ctrl_r(int offset);
void sms_vdp_ctrl_w(int offset, int data);
void sms_refresh_line(struct osd_bitmap *bitmap, int line);
void sms_update_palette(void);
void sms_cache_tiles(void);
void sms_vdp_refresh(struct osd_bitmap *bitmap, int full_refresh);

#endif /* _SMSVDP_H_ */

