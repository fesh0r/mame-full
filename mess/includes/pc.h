#include <stdio.h>
#include "osd_cpu.h"
#include "driver.h"
#include "cpu/i86/i86intf.h"
#include "vidhrdw/generic.h"

/* enable and set level for verbosity of the various parts of emulation */


#define VERBOSE_DBG 0       /* general debug messages */

#define VERBOSE_DMA 0		/* DMA (direct memory access) */
#define VERBOSE_PIO 0		/* PIO (keyboard controller) */

#define VERBOSE_FDC 0		/* FDC (floppy disk controller) */
#define VERBOSE_HDC 0		/* HDC (hard disk controller) */

#define VERBOSE_LPT 0		/* LPT (line printer) */
#define VERBOSE_JOY 0		/* JOY (joystick port) */
#define VERBOSE_SND 0		/* SND (sound / speaker) */

extern UINT8 pc_port[0x400];

/**************************************************************************
 * Logging
 * call the XXX_LOG with XXX_LOG("info",(errorlog,"%fmt\n",args));
 * where "info" can also be 0 to append .."%fmt",args to a line.
 **************************************************************************/
#define LOG(LEVEL,N,M,A)  \
	if(LEVEL>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }

#if VERBOSE_DBG
#define DBG_LOG(n,m,a) LOG(VERBOSE_DBG,n,m,a)
#else
#define DBG_LOG(n,m,a)
#endif

#if VERBOSE_PIO
#define PIO_LOG(n,m,a) LOG(VERBOSE_PIO,n,m,a)
#else
#define PIO_LOG(n,m,a)
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
extern int	pc_floppy_init(int id);
extern void pc_floppy_exit(int id);
extern int	pc_harddisk_init(int id);
extern void pc_harddisk_exit(int id);

typedef enum { 
	SETUP_END,
	SETUP_HEADER,
	SETUP_COMMENT,
	SETUP_MEMORY,
	SETUP_GRAPHIC0,
	SETUP_KEYB,
	SETUP_FDC0, 
	SETUP_FDC0D0, SETUP_FDC0D1, SETUP_FDC0D2, SETUP_FDC0D3,
	SETUP_HDC0, SETUP_HDC0D0, SETUP_HDC0D1,
	SETUP_RTC,
	SETUP_SER0, SETUP_SER0CHIP, SETUP_SER0DEV, 
	SETUP_SER1, SETUP_SER1CHIP, SETUP_SER1DEV, 
	SETUP_SER2, SETUP_SER2CHIP, SETUP_SER2DEV, 
	SETUP_SER3, SETUP_SER3CHIP, SETUP_SER3DEV,
	SETUP_SERIAL_MOUSE,
	SETUP_PAR0, SETUP_PAR0TYPE, SETUP_PAR0DEV, 
	SETUP_PAR1, SETUP_PAR1TYPE, SETUP_PAR1DEV, 
	SETUP_PAR2, SETUP_PAR2TYPE, SETUP_PAR2DEV,
	SETUP_GAME0, SETUP_GAME0C0, SETUP_GAME0C1,
	SETUP_MPU0, SETUP_MPU0D0,
	SETUP_FM, SETUP_FM_TYPE, SETUP_FM_PORT,
	SETUP_CMS, SETUP_CMS_TEXT,
	SETUP_PCJR_SOUND,
	SETUP_AMSTRAD_JOY, SETUP_AMSTRAD_MOUSE
} PC_ID;
typedef struct {
	PC_ID id, def, mask; 
} PC_SETUP;

extern PC_SETUP pc_setup_at[], pc_setup_t1000hx[];
void pc_init_setup(PC_SETUP *setup);


extern void init_pccga(void);
extern void init_pcmda(void);
void init_europc(void);
void init_bondwell(void);
extern void init_pc1512(void);
extern void init_pc1640(void);
extern void init_pc_vga(void);
extern void pc_mda_init_machine(void);
extern void pc_cga_init_machine(void);
extern void pc_vga_init_machine(void);

void init_pc_common(void);
void pc_cga_init(void);
void pc_mda_init(void);
void pc_vga_init(void);

extern void pc_ppi_portb_w(int chip, int data );
extern int pc_ppi_portb_r(int chip );
extern int pc_ppi_porta_r(int chip );

void pc_keyboard(void);

extern WRITE_HANDLER ( pc_EXP_w );
extern READ_HANDLER ( pc_EXP_r );

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

extern int  pc_cga_frame_interrupt(void);
extern int  pc_mda_frame_interrupt(void);
extern int  pc_vga_frame_interrupt(void);

/* from vidhrdw/pc.c */

/* from sndhrdw/pc.c */
extern int  pc_sh_init(const char *name);
#if 1	/* adjustmends for mame36b */
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

/***************************************************************************

  6845 CRT controller registers

***************************************************************************/
#define HTOTAL  0       /* horizontal total         */
#define HDISP   1       /* horizontal displayed     */
#define HSYNCP  2       /* horizontal sync position */
#define HSYNCW  3       /* horizontal sync width    */

#define VTOTAL  4       /* vertical total           */
#define VTADJ   5       /* vertical total adjust    */
#define VDISP   6       /* vertical displayed       */
#define VSYNCW  7       /* vertical sync width      */

#define INTLACE 8       /* interlaced / graphics    */
#define SCNLINE 9       /* scan lines per char row  */

#define CURTOP  10      /* cursor top row / enable  */
#define CURBOT  11      /* cursor bottom row        */

#define VIDH    12      /* video buffer start high  */
#define VIDL    13      /* video buffer start low   */

#define CURH    14      /* cursor address high      */
#define CURL    15      /* cursor address low       */

#define LPENH   16      /* light pen high           */
#define LPENL   17      /* light pen low            */

extern int pc_fill_odd_scanlines;


