#ifndef DGN_BETA
#define DGN_BETA

#define DGNBETA_CPU_SPEED_HZ		2000000	/* 2MHz */
#define DGNBETA_FRAMES_PER_SECOND	50

#define RamSize				256	/* 256K by default */
#define RamPageSize			4096	/* ram pages are 4096 bytes */

#define MaxTasks			16	/* Tasks 0..15 */
#define MaxPage				16	/* 16 4K pages */

DRIVER_INIT( dgnbeta );
MACHINE_INIT( dgnbeta );
MACHINE_STOP( dgnbeta );

// Page IO at FE00
READ8_HANDLER( dgn_beta_page_r );
WRITE8_HANDLER( dgn_beta_page_w );

// Ram banking handlers.
void dgnbeta_ram_b0_w(offs_t offset, UINT8 data);
void dgnbeta_ram_b1_w(offs_t offset, UINT8 data);
void dgnbeta_ram_b2_w(offs_t offset, UINT8 data);
void dgnbeta_ram_b3_w(offs_t offset, UINT8 data);
void dgnbeta_ram_b4_w(offs_t offset, UINT8 data);
void dgnbeta_ram_b5_w(offs_t offset, UINT8 data);
void dgnbeta_ram_b6_w(offs_t offset, UINT8 data);
void dgnbeta_ram_b7_w(offs_t offset, UINT8 data);
void dgnbeta_ram_b8_w(offs_t offset, UINT8 data);
void dgnbeta_ram_b9_w(offs_t offset, UINT8 data);
void dgnbeta_ram_bA_w(offs_t offset, UINT8 data);
void dgnbeta_ram_bB_w(offs_t offset, UINT8 data);
void dgnbeta_ram_bC_w(offs_t offset, UINT8 data);
void dgnbeta_ram_bD_w(offs_t offset, UINT8 data);
void dgnbeta_ram_bE_w(offs_t offset, UINT8 data);
void dgnbeta_ram_bF_w(offs_t offset, UINT8 data);
void dgnbeta_ram_bG_w(offs_t offset, UINT8 data);

/* mc6845 video display generator */
void init_video(void);
extern VIDEO_UPDATE( dgnbeta );

/*  WD2797 FDC */
READ8_HANDLER(dgnbeta_wd2797_r);
WRITE8_HANDLER(dgnbeta_wd2797_w);

extern int dgnbeta_font;
//extern static UINT8 PageRegs[16][16];	

#define iosize	(0xfc80-0xfc00)
extern UINT8 *ioarea;

#endif
