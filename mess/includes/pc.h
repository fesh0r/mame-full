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

struct _PC_SETUP;
extern struct _PC_SETUP pc_setup_at[], pc_setup_t1000hx[];
void pc_init_setup(struct _PC_SETUP *setup);


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

extern int  pc_cga_frame_interrupt(void);
extern int  pc_mda_frame_interrupt(void);
extern int  pc_vga_frame_interrupt(void);

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
void pc_fdc_setup(void);

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


