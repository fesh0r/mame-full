/***************************************************************************

  $Id: pc8801.h,v 1.4 2001/11/16 05:02:20 npwoods Exp $

***************************************************************************/

WRITE_HANDLER(pc8801_write_interrupt_level);
WRITE_HANDLER(pc8801_write_interrupt_mask);
int pc8801_interrupt(void);
READ_HANDLER(pc88sr_inport_30);
READ_HANDLER(pc88sr_inport_31);
READ_HANDLER(pc88sr_inport_32);
WRITE_HANDLER(pc88sr_outport_30);
WRITE_HANDLER(pc88sr_outport_31);
WRITE_HANDLER(pc88sr_outport_32);
WRITE_HANDLER(pc88sr_outport_40);
READ_HANDLER(pc88sr_inport_40);
READ_HANDLER(pc8801_inport_70);
WRITE_HANDLER(pc8801_outport_70);
WRITE_HANDLER(pc8801_outport_78);
READ_HANDLER(pc88sr_inport_71);
WRITE_HANDLER(pc88sr_outport_71);
void pc88sr_ch_reset_l (void);
void pc88sr_ch_reset_h (void);
void pc8801_update_bank(void);
extern unsigned char *pc8801_mainRAM;
extern int pc88sr_is_highspeed;
int pc8801_floppy_id (int id);
int pc8801_floppy_init (int id);
READ_HANDLER(pc8801fd_nec765_tc);
void pc88sr_sound_interupt(int irq);
WRITE_HANDLER(pc8801_write_kanji1);
READ_HANDLER(pc8801_read_kanji1);
WRITE_HANDLER(pc8801_write_kanji2);
READ_HANDLER(pc8801_read_kanji2);
WRITE_HANDLER(pc8801_calender);
READ_HANDLER(pc8801_read_extmem);
WRITE_HANDLER(pc8801_write_extmem);

void pc8801_video_init (int hireso);
int is_pc8801_vram_select(void);
WRITE_HANDLER(pc8801_vramsel);
READ_HANDLER(pc8801_vramtest);
extern unsigned char *pc88sr_textRAM;
int pc8801_vh_start(void);
void pc8801_vh_exit(void);
void pc8801_vh_refresh(struct mame_bitmap *bitmap,int full_refresh);
void pc8801_init_palette (unsigned char *sys_palette,
			  unsigned short *sys_colortable,
			  const unsigned char *color_prom);
extern int pc8801_is_24KHz;
WRITE_HANDLER(pc8801_crtc_write);
READ_HANDLER(pc8801_crtc_read);
WRITE_HANDLER(pc8801_dmac_write);
READ_HANDLER(pc8801_dmac_read);
WRITE_HANDLER(pc88sr_disp_30);
WRITE_HANDLER(pc88sr_disp_31);
WRITE_HANDLER(pc88sr_disp_32);
WRITE_HANDLER(pc88sr_ALU);
WRITE_HANDLER(pc8801_palette_out);
