#include <stdio.h>
#include "osd_cpu.h"
#include "driver.h"
#include "cpu/i86/i86intrf.h"
#include "vidhrdw/generic.h"

/* enable and set level for verbosity of the various parts of emulation */


#define VERBOSE_DBG 1       /* general debug messages */

#define VERBOSE_DMA 0		/* DMA (direct memory access) */
#define VERBOSE_PIO 0		/* PIO (keyboard controller) */
#define VERBOSE_PIT 0		/* PIT (programmable interrupt timer) */
#define VERBOSE_PIC 0		/* PIC (programmable interrupt controller) */

#define VERBOSE_MDA 0		/* MDA (Monochrome Display Adapter) */
#define VERBOSE_CGA 0		/* CGA (Color Graphics Adapter) */
#define VERBOSE_T1T 0		/* T1T (Tandy 100 Graphics Adapter) */

#define VERBOSE_FDC 0		/* FDC (floppy disk controller) */
#define VERBOSE_HDC 0		/* HDC (hard disk controller) */

#define VERBOSE_LPT 0		/* LPT (line printer) */
#define VERBOSE_COM 0		/* COM (communication / serial) */
#define VERBOSE_JOY 0		/* JOY (joystick port) */
#define VERBOSE_SND 0		/* SND (sound / speaker) */

extern UINT8 pc_port[0x400];

/**************************************************************************
 * Logging
 * call the XXX_LOG with XXX_LOG("info",(errorlog,"%fmt\n",args));
 * where "info" can also be 0 to append .."%fmt",args to a line.
 **************************************************************************/
#define LOG(LEVEL,N,M,A)  \
	if( errorlog && (LEVEL>=N) ){ if( M )fprintf( errorlog,"%11.6f: %-24s",timer_get_time(),(char*)M ); fprintf##A; }

#if VERBOSE_DBG
#define DBG_LOG(n,m,a) LOG(VERBOSE_DBG,n,m,a)
#else
#define DBG_LOG(n,m,a)
#endif

#if VERBOSE_DMA
#define DMA_LOG(n,m,a) LOG(VERBOSE_DMA,n,m,a)
#else
#define DMA_LOG(n,m,a)
#endif

#if VERBOSE_PIO
#define PIO_LOG(n,m,a) LOG(VERBOSE_PIO,n,m,a)
#else
#define PIO_LOG(n,m,a)
#endif

#if VERBOSE_PIC
#define PIC_LOG(n,m,a) LOG(VERBOSE_PIC,n,m,a)
#else
#define PIC_LOG(n,m,a)
#endif

#if VERBOSE_PIT
#define PIT_LOG(n,m,a) LOG(VERBOSE_PIT,n,m,a)
#else
#define PIT_LOG(n,m,a)
#endif

#if VERBOSE_MDA
#define MDA_LOG(n,m,a) LOG(VERBOSE_MDA,n,m,a)
#else
#define MDA_LOG(n,m,a)
#endif

#if VERBOSE_CGA
#define CGA_LOG(n,m,a) LOG(VERBOSE_CGA,n,m,a)
#else
#define CGA_LOG(n,m,a)
#endif

#if VERBOSE_T1T
#define T1T_LOG(n,m,a) LOG(VERBOSE_T1T,n,m,a)
#else
#define T1T_LOG(n,m,a)
#endif

#if VERBOSE_FDC
#define FDC_LOG(n,m,a) LOG(VERBOSE_FDC,n,m,a)
#else
#define FDC_LOG(n,m,a)
#endif

#if VERBOSE_HDC
#define HDC_LOG(n,m,a) LOG(VERBOSE_HDC,n,m,a)
#else
#define HDC_LOG(n,m,a)
#endif

#if VERBOSE_LPT
#define LPT_LOG(n,m,a) LOG(VERBOSE_LPT,n,m,a)
#else
#define LPT_LOG(n,m,a)
#endif

#if VERBOSE_COM
#define COM_LOG(n,m,a) LOG(VERBOSE_COM,n,m,a)
#else
#define COM_LOG(n,m,a)
#endif

#if VERBOSE_JOY
#define JOY_LOG(n,m,a) LOG(VERBOSE_JOY,n,m,a)
#else
#define JOY_LOG(n,m,a)
#endif

#if VERBOSE_SND
#define SND_LOG(n,m,a) LOG(VERBOSE_SND,n,m,a)
#else
#define SND_LOG(n,m,a)
#endif

/* from machine/pc.c */
extern int	pc_framecnt;
extern int	pc_floppy_init(int id);
extern void pc_floppy_exit(int id);
extern int	pc_harddisk_init(int id);
extern void pc_harddisk_exit(int id);

extern void init_pc(void);
extern void pc_mda_init_machine(void);
extern void pc_cga_init_machine(void);
extern void pc_t1t_init_machine(void);
extern void pc_shutdown_machine(void);

extern WRITE_HANDLER ( pc_DMA_w );
extern READ_HANDLER ( pc_DMA_r );
extern WRITE_HANDLER ( pc_DMA_page_w );
extern READ_HANDLER ( pc_DMA_page_r );

extern WRITE_HANDLER ( pc_PIC_w );
extern READ_HANDLER ( pc_PIC_r );

extern WRITE_HANDLER ( pc_PIT_w );
extern READ_HANDLER ( pc_PIT_r );

extern WRITE_HANDLER ( pc_PIO_w );
extern READ_HANDLER ( pc_PIO_r );

extern WRITE_HANDLER ( pc_EXP_w );
extern READ_HANDLER ( pc_EXP_r );

extern WRITE_HANDLER ( pc_LPT1_w );
extern READ_HANDLER ( pc_LPT1_r );
extern WRITE_HANDLER ( pc_LPT2_w );
extern READ_HANDLER ( pc_LPT2_r );
extern WRITE_HANDLER ( pc_LPT3_w );
extern READ_HANDLER ( pc_LPT3_r );

extern WRITE_HANDLER ( pc_COM1_w );
extern READ_HANDLER ( pc_COM1_r );
extern WRITE_HANDLER ( pc_COM2_w );
extern READ_HANDLER ( pc_COM2_r );
extern WRITE_HANDLER ( pc_COM3_w );
extern READ_HANDLER ( pc_COM3_r );
extern WRITE_HANDLER ( pc_COM4_w );
extern READ_HANDLER ( pc_COM4_r );

extern WRITE_HANDLER ( pc_JOY_w );
extern READ_HANDLER ( pc_JOY_r );

extern WRITE_HANDLER ( pc_FDC_w );
extern READ_HANDLER ( pc_FDC_r );

extern WRITE_HANDLER ( pc_HDC1_w );
extern READ_HANDLER (	pc_HDC1_r );
extern WRITE_HANDLER ( pc_HDC2_w );
extern READ_HANDLER ( pc_HDC2_r );

extern WRITE_HANDLER ( pc_MDA_w );
extern READ_HANDLER ( pc_MDA_r );

extern WRITE_HANDLER ( pc_CGA_w );
extern READ_HANDLER ( pc_CGA_r );

extern WRITE_HANDLER ( pc_t1t_p37x_w );
extern READ_HANDLER ( pc_t1t_p37x_r );
extern WRITE_HANDLER ( pc_T1T_w );
extern READ_HANDLER (	pc_T1T_r );

extern int  pc_frame_interrupt(void);

extern UINT8 pc_DMA_msb;
extern UINT8 pc_DMA_temp;
extern int pc_DMA_address[4];
extern int pc_DMA_count[4];
extern int pc_DMA_page[4];
extern UINT8 pc_DMA_transfer_mode[4];
extern INT8 pc_DMA_direction[4];
extern UINT8 pc_DMA_operation[4];
extern UINT8 pc_DMA_status;
extern UINT8 pc_DMA_mask;
extern UINT8 pc_DMA_command;
extern UINT8 pc_DMA_DACK_hi;
extern UINT8 pc_DMA_DREQ_hi;
extern UINT8 pc_DMA_write_extended;
extern UINT8 pc_DMA_rotate_priority;
extern UINT8 pc_DMA_compressed_timing;
extern UINT8 pc_DMA_enable_controller;
extern UINT8 pc_DMA_channel;

extern void pc_PIC_issue_irq(int irq);

/* from vidhrdw/pc.c */
extern int  pc_mda_vh_start(void);
extern void pc_mda_vh_stop(void);
extern void pc_mda_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
extern WRITE_HANDLER ( pc_mda_videoram_w );
extern void pc_mda_blink_textcolors(int on);
extern void pc_mda_index_w(int data);
extern int	pc_mda_index_r(void);
extern void pc_mda_port_w(int data);
extern int	pc_mda_port_r(void);
extern void pc_mda_mode_control_w(int data);
extern int	pc_mda_mode_control_r(void);
extern void pc_mda_color_select_w(int data);
extern int	pc_mda_color_select_r(void);
extern void pc_mda_feature_control_w(int data);
extern int	pc_mda_status_r(void);
extern void pc_mda_lightpen_strobe_w(int data);
extern void pc_hgc_config_w(int data);
extern int	pc_hgc_config_r(void);

extern int	pc_cga_vh_start(void);
extern void pc_cga_vh_stop(void);
extern WRITE_HANDLER ( pc_cga_videoram_w );
extern void pc_cga_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

extern void pc_cga_blink_textcolors(int on);
extern void pc_cga_index_w(int data);
extern int	pc_cga_index_r(void);
extern void pc_cga_port_w(int data);
extern int	pc_cga_port_r(void);
extern void pc_cga_mode_control_w(int data);
extern int	pc_cga_mode_control_r(void);
extern void pc_cga_color_select_w(int data);
extern int	pc_cga_color_select_r(void);
extern void pc_cga_feature_control_w(int data);
extern int	pc_cga_status_r(void);
extern void pc_cga_lightpen_strobe_w(int data);

extern int	pc_t1t_vh_start(void);
extern void pc_t1t_vh_stop(void);
extern void pc_t1t_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
extern WRITE_HANDLER ( pc_t1t_videoram_w );
extern READ_HANDLER ( pc_t1t_videoram_r );

extern void pc_t1t_blink_textcolors(int on);
extern void pc_t1t_index_w(int data);
extern int	pc_t1t_index_r(void);
extern void pc_t1t_port_w(int data);
extern int	pc_t1t_port_r(void);
extern void pc_t1t_mode_control_w(int data);
extern int	pc_t1t_mode_control_r(void);
extern void pc_t1t_color_select_w(int data);
extern int	pc_t1t_color_select_r(void);
extern void pc_t1t_vga_index_w(int data);
extern int	pc_t1t_status_r(void);
extern void pc_t1t_lightpen_strobe_w(int data);
extern void pc_t1t_vga_data_w(int data);
extern int	pc_t1t_vga_data_r(void);
extern void pc_t1t_bank_w(int data);
extern int	pc_t1t_bank_r(void);

/* from sndhrdw/pc.c */
extern int  pc_sh_init(const char *name);
#if 1 // adjustmends for mame36b
extern int  pc_sh_custom_start(const struct MachineSound *driver);
extern void pc_sh_custom_update(void);
#endif
extern void pc_sh_stop(void);
extern void pc_sh_update(int param, INT16 *buff, int length);
extern void pc_sh_speaker(int mode);

/* from machine/pc_fdc.c */
extern void pc_fdc_command_w(int data);
extern void pc_fdc_data_rate_w(int data);
extern void pc_fdc_DOR_w(int data);

extern int	pc_fdc_data_r(void);
extern int	pc_fdc_status_r(void);
extern int	pc_fdc_DIR_r(void);

extern void *pc_fdc_file[2];
extern UINT8 pc_fdc_spt[2];
extern UINT8 pc_fdc_heads[2];
extern UINT8 pc_fdc_scl[2];

#if 0
/* from machine/pc_ide.c */
extern void pc_ide_data_w(int data);
extern void pc_ide_write_precomp_w(int data);
extern void pc_ide_sector_count_w(int data);
extern void pc_ide_sector_number_w(int data);
extern void pc_ide_cylinder_number_l_w(int data);
extern void pc_ide_cylinder_number_h_w(int data);
extern void pc_ide_drive_head_w(int data);
extern void pc_ide_command_w(int data);

extern int	pc_ide_data_r(void);
extern int	pc_ide_error_r(void);
extern int	pc_ide_sector_count_r(void);
extern int	pc_ide_sector_number_r(void);
extern int	pc_ide_cylinder_number_l_r(void);
extern int	pc_ide_cylinder_number_h_r(void);
extern int	pc_ide_drive_head_r(void);
extern int	pc_ide_status_r(void);
#endif

/* from machine/pc_hdc.c */
extern void *pc_hdc_file[4];

extern void pc_hdc_data_w(int n, int data);
extern void pc_hdc_reset_w(int n, int data);
extern void pc_hdc_select_w(int n, int data);
extern void pc_hdc_control_w(int n, int data);

extern int	pc_hdc_data_r(int n);
extern int	pc_hdc_status_r(int n);
extern int	pc_hdc_dipswitch_r(int n);


