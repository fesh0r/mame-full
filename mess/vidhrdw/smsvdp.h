
#ifndef _SMSVDP_H_
#define _SMSVDP_H_

#define STATUS_VINT     (0x80)  /* Pending vertical interrupt flag */
#define STATUS_HINT     (0x40)  /* Pending horizontal interrupt flag */
#define STATUS_SPRCOL   (0x20)  /* Object collision flag */

/* Function prototypes */

int sms_vdp_start(void);
int gamegear_vdp_start(void);
READ_HANDLER ( sms_vdp_curline_r );
int SMSVDP_start (int vdp_type);
void sms_vdp_stop(void);
int sms_vdp_interrupt(void);
READ_HANDLER ( sms_vdp_data_r );
WRITE_HANDLER ( sms_vdp_data_w );
READ_HANDLER ( sms_vdp_ctrl_r );
WRITE_HANDLER ( sms_vdp_ctrl_w );
void sms_refresh_line(struct osd_bitmap *bitmap, int line);
void sms_update_palette(void);
void sms_cache_tiles(void);
void sms_vdp_refresh(struct osd_bitmap *bitmap, int full_refresh);

#endif /* _SMSVDP_H_ */

