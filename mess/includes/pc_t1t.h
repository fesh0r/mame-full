#ifndef PC_T1T_H
#define PC_T1T_H

extern unsigned short pcjr_colortable[256*2+16*2+2*4+1*16];
extern struct GfxDecodeInfo t1000hx_gfxdecodeinfo[];
extern struct GfxDecodeInfo t1000sx_gfxdecodeinfo[];

PALETTE_INIT( pcjr );

VIDEO_START( pc_t1t );
VIDEO_UPDATE( pc_t1t );

void pc_t1t_timer(void);
WRITE_HANDLER ( pc_t1t_videoram_w );
READ_HANDLER ( pc_t1t_videoram_r );
WRITE_HANDLER ( pc_T1T_w );
READ_HANDLER (	pc_T1T_r );

void pc_t1t_reset(void);

#endif /* PC_T1T_H */
