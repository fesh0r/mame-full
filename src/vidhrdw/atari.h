/******************************************************************************
	Atari 400/800

	ANTIC video controller
	GTIA  graphics television interface adapter

	Juergen Buchmueller, June 1998
******************************************************************************/

#define ACCURATE_ANTIC_READMEM	0
#define SHOW_EVERY_SCREEN_BIT	0
#define OPTIMIZE				1
#define OPTIMIZE_VISIBLE		0

/*************************************************************************
 * Some calculations concerning the Atari 800 NTSC video signal timings:
 * ----------------------------------------------------------------------
 * An Atari 800 has 228 'color clocks' per scanline, but only 176 of
 * them are visible due to the limited overscan range of normal TV sets.
 * That is 44 characters with 4 color clocks (= 8 pixels) each.
 *
 * Drawing the entire screen takes 16,684 microseconds.
 * This is equal to 59.9376 Hz; very close to 60 Hz (what we use here).
 * A single scanline takes 64 microseconds (15.625 kHz) to go through
 * the 228 color clocks. The 6502 CPU clock is derived from the color
 * clock divided by two, so there are 114 CPU clock cycles per scanline.
 * During 9 clock cycles per scanline the CPU is halted by ANTIC to
 * to do DMA RAM refresh. The horizontal blank has a duration of
 * 14 microseconds. This is close to 25 cpu cycles per scanline.
 *
 *	   A detailed graph will hopefully make the timings more clear:
 *	   (Right now it's not even clear to myself - but I'm working on it ;)
 *
 *    #0                                                             #113
 *	   |			  entire scanline takes 64 microseconds 			|
 *	   |<------------------------ 114clks ----------------------------->|
 *	   |		   9 clks lost due to ANTICs DMA RAM refresh			|
 *	   |		  [1]  [2]	 [3]  [4]  [5]	[6]  [7]  [8]  [9]			|
 *	   |																|
 *	   |			  BIOS DLI srvc 									|
 *	   |			#22 		 #33									|
 *	   |			  |<- 11clks ->|									|
 *	   |		   DLI NMI		   |									|
 *	   |<-- 22clks -->| 		   |									|
 *	   |		#15   | 		   |					  #103			|
 *	-->|  HBLANK  |   | 		   |	  DLI phase one 	 |	HBLANK	|-->
 *	   |<-15clks->|   | 		   |<------ 21-62 clks ----->|<-10clks->|
 *	   +----------+>--+------------+-------------------------+----------+
 *	   |		  |<----------- max. 80 clks --------------->|
 *	   |		  | 	  ANTIC fetching video data 		 | DLI phase two
 *	   |		  | 		   [ 0 - 48 clks ]				 |	max 24 clks
 *	   |		  |-7-> 									 |
 *	   |		DLI NMI 									 |
 *	   |													 |
 *	   ANTIC scanline DMA									 WSYNC reset
 *	   [0 - 8 cycles lost]									 CPU continues
 *	   1 clk per command
 *	   2 clks if LMS is set
 *	   1 clk for missile graphics
 *	   4 clks for player graphics
 *
 * The first 8 scanlines are invisible, because they occur during
 * vertical blank. Another 4 scanlines are invisible due to overscan.
 * Scanline 12 to 244 are the maximum visible scanlines.
 * VBLANK starts in scanline 248 and lasts until line 260 and after the
 * electron beam returned to line 0 for another 8 scan lines.
 *
 * The need to do 261 scanlines was derived from tests with a Jumpman
 * boot disc and it's attract mode, which seems to use the VBL timing
 * in relation to the POKEY timers to coordinate music and animation.
 *************************************************************************/

/* total number of cpu cycles per scanline (incl. hblank) */
#define CYCLES_PER_LINE 114
/* number of cycles for horizontal blank period (14 of 64 microseconds) */
#define CYCLES_HBLANK	25
/* number of cycles lost for ANTICs RAM refresh using DMA */
#define CYCLES_REFRESH  9
/* where does the actual line drawing start */
#define CYCLES_HSTART	15
/* number of cycles until the CPU recognizes a DLI */
#define CYCLES_DLI_NMI  7
/* where does the HSYNC position of a scanline start */
#define CYCLES_HSYNC	(104-CYCLES_HSTART)

/* vblank ends in this scanline */
#define VBL_END         8
/* video display begins in this scanline */
#define VDATA_START     12
/* video display ends in this scanline */
#define VDATA_END       244
/* vblank starts in this scanline */
#define VBL_START       248
/* total number of lines per frame (incl. vblank) */
#define TOTAL_LINES     261

#define FRAMES			60
/* The real CPU clock */
#define CPU_EXACT		1789790
/* calculate our CPU clock */
#define CPU_APPROX		FRAMES * TOTAL_LINES * CYCLES_PER_LINE
/* caclulate vertical blank duration; this also catches up minor */
/* deviations from the real CYCLES_PER_LINE and TOTAL_LINES values */
#define VBL_DURATION	(VBL_START - VBL_END) * CYCLES_PER_LINE

#define HWIDTH          48      /* total characters per line                */
#define HCHARS			44		/* visible characters per line				*/
#define VHEIGHT 		32
#define VCHARS			(VDATA_END-VDATA_START+7)/8
#define BUF_OFFS0		(HWIDTH-HCHARS)/2

#define PMOFFSET		32		/* # of pixels to adjust p/m hpos */

#define VPAGE			0xf000	/* 4K page mask for video data src */
#define VOFFS			0x0fff	/* 4K offset mask for video data src */
#define DPAGE			0xfc00	/* 1K page mask for display list */
#define DOFFS			0x03ff	/* 1K offset mask for display list */

#define DLI_NMI         0x80    /* 10000000b bit mask for display list interrupt */
#define VBL_NMI 		0x40	/* 01000000b bit mask for vertical blank interrupt */

#define ANTIC_DLI		0x80	/* 10000000b cmds with display list intr	*/
#define ANTIC_LMS		0x40	/* 01000000b cmds with load memory scan 	*/
#define ANTIC_VSCR		0x20	/* 00100000b cmds with vertical scroll		*/
#define ANTIC_HSCR		0x10	/* 00010000b cmds with horizontal scroll	*/
#define ANTIC_MODE		0x0f	/* 00001111b cmd mode mask					*/

#define DMA_ANTIC		0x20	/* 00100000b ANTIC DMA enable				*/
#define DMA_PM_DBLLINE	0x10	/* 00010000b double line player/missile 	*/
#define DMA_PLAYER		0x08	/* 00001000b player DMA enable				*/
#define DMA_MISSILE 	0x04	/* 00000100b missile DMA enable 			*/

#define OFS_MIS_SINGLE	3*256	/* offset missiles single line DMA			*/
#define OFS_PL0_SINGLE	4*256	/* offset player 0 single line DMA			*/
#define OFS_PL1_SINGLE	5*256	/* offset player 1 single line DMA			*/
#define OFS_PL2_SINGLE	6*256	/* offset player 2 single line DMA			*/
#define OFS_PL3_SINGLE	7*256	/* offset player 3 single line DMA			*/

#define OFS_MIS_DOUBLE  3*128   /* offset missiles double line DMA          */
#define OFS_PL0_DOUBLE	4*128	/* offset player 0 double line DMA			*/
#define OFS_PL1_DOUBLE	5*128	/* offset player 1 double line DMA			*/
#define OFS_PL2_DOUBLE	6*128	/* offset player 2 double line DMA			*/
#define OFS_PL3_DOUBLE	7*128	/* offset player 3 double line DMA			*/

#define PFD 	0x00	/* 00000000b playfield default color */

#define PBK 	0x00	/* 00000000b playfield background */
#define PF0 	0x01	/* 00000001b playfield color #0   */
#define PF1 	0x02	/* 00000010b playfield color #1   */
#define PF2 	0x04	/* 00000100b playfield color #2   */
#define PF3 	0x08	/* 00001000b playfield color #3   */
#define PL0 	0x11	/* 00010001b player #0			  */
#define PL1 	0x12	/* 00010010b player #1			  */
#define PL2 	0x14	/* 00010100b player #2			  */
#define PL3 	0x18	/* 00011000b player #3			  */
#define MI0 	0x21	/* 00100001b missile #0 		  */
#define MI1 	0x22	/* 00100010b missile #1 		  */
#define MI2 	0x24	/* 00100100b missile #2 		  */
#define MI3 	0x28	/* 00101000b missile #3 		  */
#define T00 	0x40	/* 01000000b text mode pixels 00  */
#define P000	0x48	/* 01001000b player #0 pixels 00  */
#define P100	0x4a	/* 01001010b player #1 pixels 00  */
#define P200	0x4c	/* 01001100b player #2 pixels 00  */
#define P300	0x4e	/* 01001110b player #3 pixels 00  */
#define P400	0x4f	/* 01001111b missiles  pixels 00  */
#define T01 	0x50	/* 01010000b text mode pixels 01  */
#define P001	0x58	/* 01011000b player #0 pixels 01  */
#define P101	0x5a	/* 01011010b player #1 pixels 01  */
#define P201	0x5c	/* 01011100b player #2 pixels 01  */
#define P301	0x5e	/* 01011110b player #3 pixels 01  */
#define P401	0x5f	/* 01011111b missiles  pixels 01  */
#define T10 	0x60	/* 01100000b text mode pixels 10  */
#define P010	0x68	/* 01101000b player #0 pixels 10  */
#define P110	0x6a	/* 01101010b player #1 pixels 10  */
#define P210	0x6c	/* 01101100b player #2 pixels 10  */
#define P310	0x6e	/* 01101110b player #3 pixels 10  */
#define P410	0x6f	/* 01101111b missiles  pixels 10  */
#define T11 	0x70	/* 01110000b text mode pixels 11  */
#define P011	0x78	/* 01111000b player #0 pixels 11  */
#define P111	0x7a	/* 01111010b player #1 pixels 11  */
#define P211	0x7c	/* 01111100b player #2 pixels 11  */
#define P311	0x7e	/* 01111110b player #3 pixels 11  */
#define P411	0x7f	/* 01111111b missiles  pixels 11  */
#define G00 	0x80	/* 10000000b hires gfx pixels 00  */
#define G01 	0x90	/* 10010000b hires gfx pixels 01  */
#define G10 	0xa0	/* 10100000b hires gfx pixels 10  */
#define G11 	0xb0	/* 10110000b hires gfx pixels 11  */
#define GT1 	0xc0	/* 11000000b gtia mode 1		  */
#define GT2 	0xd0	/* 11010000b gtia mode 2		  */
#define GT3 	0xe0	/* 11100000b gtia mode 3		  */
#define ILL 	0xfe	/* 11111110b illegal priority	  */
#define EOR 	0xff	/* 11111111b EOR mode color 	  */

#define LUM 	0x0f	/* 00001111b luminance bits 	  */
#define HUE 	0xf0	/* 11110000b hue bits			  */

#define TRIGGER_VBLANK	64715
#define TRIGGER_STEAL	64716
#define TRIGGER_HSYNC	64717

#define GTIA_MISSILE	0x01
#define GTIA_PLAYER 	0x02
#define GTIA_TRIGGER	0x04

#ifdef  LSB_FIRST

/* make a dword from four bytes */
#define MKD(b0,b1,b2,b3) ((b0)|((b1)<<8)|((b2)<<16)|((b3)<<24))

#define BYTE0	0x000000ff
#define BYTE1   0x0000ff00
#define BYTE2   0x00ff0000
#define BYTE3   0xff000000

#else

/* make a dword from four bytes */
#define MKD(b0,b1,b2,b3) (((b0)<<24)|((b1)<<16)|((b2)<<8)|(b3))

#define BYTE0	0xff000000
#define BYTE1   0x00ff0000
#define BYTE2   0x0000ff00
#define BYTE3   0x000000ff

#endif

/* make a qword from eight bytes */
#define MKQ(b0,b1,b2,b3,b4,b5,b6,b7) MKD(b0,b1,b2,b3), MKD(b4,b5,b6,b7)

/* make two dwords from 4 bytes by doubling them */
#define MKDD(b0,b1,b2,b3) MKD(b0,b0,b1,b1), MKD(b2,b2,b3,b3)

/* make a four dwords from 4 bytes by quadrupling them */
#define MKQD(b0,b1,b2,b3) MKD(b0,b0,b0,b0), MKD(b1,b1,b1,b1), MKD(b2,b2,b2,b2), MKD(b3,b3,b3,b3)

/*****************************************************************************
 * If your memcpy does not expand too well if you use it with constant
 * size_t field, you might want to define these macros in another way.
 * NOTE: dst is not necessarily dword aligned (because of horz scrolling)!
 *****************************************************************************/
#define COPY4(dst,src)    memcpy(dst,src,4); dst+=4
#define COPY8(dst,src)    memcpy(dst,src,8); dst+=8
#define COPY16(dst,src)   memcpy(dst,src,16); dst+=16

typedef struct {
	byte	m0pf;			/* d000 missile 0 playfield collisions */
	byte	m1pf;			/* d001 missile 1 playfield collisions */
	byte	m2pf;			/* d002 missile 2 playfield collisions */
	byte	m3pf;			/* d003 missile 3 playfield collisions */
	byte	p0pf;			/* d004 player 0 playfield collisions */
	byte	p1pf;			/* d005 player 1 playfield collisions */
	byte	p2pf;			/* d006 player 2 playfield collisions */
	byte	p3pf;			/* d007 player 3 playfield collisions */
	byte	m0pl;			/* d008 missile 0 player collisions */
	byte	m1pl;			/* d009 missile 1 player collisions */
	byte	m2pl;			/* d00a missile 2 player collisions */
	byte	m3pl;			/* d00b missile 3 player collisions */
	byte	p0pl;			/* d00c player 0 player collisions */
	byte	p1pl;			/* d00d player 1 player collisions */
	byte	p2pl;			/* d00e player 2 player collisions */
	byte	p3pl;			/* d00f player 3 player collisions */
	byte	but0;			/* d010 button stick 0 */
	byte	but1;			/* d011 button stick 1 */
	byte	but2;			/* d012 button stick 2 */
	byte	but3;			/* d013 button stick 3 */
	byte	gtia14; 		/* d014 nothing */
	byte	gtia15; 		/* d015 nothing */
	byte	gtia16; 		/* d016 nothing */
	byte	gtia17; 		/* d017 nothing */
	byte	gtia18; 		/* d018 nothing */
	byte	gtia19; 		/* d019 nothing */
	byte	gtia1a; 		/* d01a nothing */
	byte	gtia1b; 		/* d01b nothing */
	byte	gtia1c; 		/* d01c nothing */
	byte	gtia1d; 		/* d01d nothing */
	byte	gtia1e; 		/* d01e nothing */
	byte	cons;			/* d01f console keys */
}   GTIA_R; /* reading registers */

typedef struct {
	byte	hposp0; 		/* d000 player 0 horz position */
	byte	hposp1; 		/* d001 player 1 horz position */
	byte	hposp2; 		/* d002 player 2 horz position */
	byte	hposp3; 		/* d003 player 3 horz position */
	byte	hposm0; 		/* d004 missile 0 horz position */
	byte	hposm1; 		/* d005 missile 1 horz position */
	byte	hposm2; 		/* d006 missile 2 horz position */
	byte	hposm3; 		/* d007 missile 3 horz position */
	byte	sizep0; 		/* d008 size player 0 */
	byte	sizep1; 		/* d009 size player 1 */
	byte	sizep2; 		/* d00a size player 2 */
	byte	sizep3; 		/* d00b size player 3 */
	byte	grafp0; 		/* d00d graphics data for player 0 */
	byte	grafp1; 		/* d00e graphics data for player 1 */
	byte	grafp2; 		/* d00f graphics data for player 2 */
	byte	grafp3; 		/* d010 graphics data for player 3 */
	byte	colpm0; 		/* d012 color for player/missile 0 */
	byte	colpm1; 		/* d013 color for player/missile 1 */
	byte	colpm2; 		/* d014 color for player/missile 2 */
	byte	colpm3; 		/* d015 color for player/missile 3 */
	byte	colpf0; 		/* d016 playfield color 0 */
	byte	colpf1; 		/* d017 playfield color 1 */
	byte	colpf2; 		/* d018 playfield color 2 */
	byte	colpf3; 		/* d019 playfield color 3 */
	byte	sizem;			/* d00c size missiles */
	byte	grafm;			/* d011 graphics data for missiles */
	byte	colbk;			/* d01a background playfield */
	byte	prior;			/* d01b priority select */
	byte	vdelay; 		/* d01c delay until vertical retrace */
	byte	gractl; 		/* d01d graphics control */
	byte	hitclr; 		/* d01e clear collisions */
	byte	cons;			/* d01f write console (speaker) */
}	GTIA_W;  /* writing registers */

typedef struct GTIA_H {
	int  hitclr_frames; 	/* frames gone since last hitclr */
	byte hposp0;			/* optimized horz position player 0 */
	byte hposp1;			/* optimized horz position player 1 */
	byte hposp2;			/* optimized horz position player 2 */
	byte hposp3;			/* optimized horz position player 3 */
	byte sizep0;			/* optimized size player 0 */
	byte sizep1;			/* optimized size player 1 */
	byte sizep2;			/* optimized size player 2 */
	byte sizep3;			/* optimized size player 3 */
	byte grafp0;			/* optimized graphics data player 0 */
	byte grafp1;			/* optimized graphics data player 1 */
	byte grafp2;			/* optimized graphics data player 2 */
	byte grafp3;			/* optimized graphics data player 3 */
	byte hposm0;			/* optimized horz position missile 0 */
	byte hposm1;			/* optimized horz position missile 1 */
	byte hposm2;			/* optimized horz position missile 2 */
	byte hposm3;			/* optimized horz position missile 3 */
	byte grafm0;			/* optimized graphics data missile 0 */
	byte grafm1;			/* optimized graphics data missile 1 */
	byte grafm2;			/* optimized graphics data missile 2 */
	byte grafm3;			/* optimized graphics data missile 3 */
    byte sizem;             /* optimized size missiles */
	byte usedp; 			/* mask for used player colors */
	byte usedm0;			/* mask for used missile 0 color */
	byte usedm1;			/* mask for used missile 1 color */
	byte usedm2;			/* mask for used missile 2 color */
	byte usedm3;			/* mask for used missile 3 color */
	byte vdelay_m0; 		/* vertical delay for missile 0 */
	byte vdelay_m1; 		/* vertical delay for missile 1 */
	byte vdelay_m2; 		/* vertical delay for missile 2 */
	byte vdelay_m3; 		/* vertical delay for missile 3 */
	byte vdelay_p0; 		/* vertical delay for player 0 */
	byte vdelay_p1; 		/* vertical delay for player 1 */
	byte vdelay_p2; 		/* vertical delay for player 2 */
	byte vdelay_p3; 		/* vertical delay for player 3 */
}	GTIA_H;

typedef struct {
	GTIA_R	r;				/* read registers */
	GTIA_W	w;				/* write registers */
	GTIA_H	h;
}   GTIA;

extern	GTIA	gtia;

typedef struct {
	byte	antic00;		/* 00 nothing */
	byte	antic01;		/* 01 nothing */
	byte	antic02;		/* 02 nothing */
	byte	antic03;		/* 03 nothing */
	byte	antic04;		/* 04 nothing */
	byte	antic05;		/* 05 nothing */
	byte	antic06;		/* 06 nothing */
	byte	antic07;		/* 07 nothing */
	byte	antic08;		/* 08 nothing */
	byte	antic09;		/* 09 nothing */
	byte	antic0a;		/* 0a nothing */
	byte	vcount; 		/* 0b vertical (scanline) counter */
	byte	penh;			/* 0c light pen horizontal pos */
	byte	penv;			/* 0d light pen vertical pos */
	byte	antic0e;		/* 0e nothing */
	byte	nmist;			/* 0f NMI status */
}	ANTIC_R;  /* read registers */

typedef struct {
	byte	dmactl; 		/* 00 write DMA control */
	byte	chactl; 		/* 01 write character control */
	byte	dlistl; 		/* 02 display list low */
	byte	dlisth; 		/* 03 display list high */
	byte	hscrol; 		/* 04 horz scroll */
	byte	vscrol; 		/* 05 vert scroll */
	byte	pmbasl; 		/* 06 player/missile base addr low */
	byte	pmbash; 		/* 07 player/missile base addr high */
	byte	chbasl; 		/* 08 character generator base addr low */
	byte	chbash; 		/* 09 character generator base addr high */
	byte	wsync;			/* 0a wait for hsync */
	byte	antic0b;		/* 0b nothing */
	byte	antic0c;		/* 0c nothing */
	byte	antic0d;		/* 0d nothing */
	byte	nmien;			/* 0e NMI enable */
	byte	nmires; 		/* 0f NMI reset */
}	ANTIC_W;  /* write registers */

typedef struct {
	int 	cmd;				/* antic command for this scanline */
#if OPTIMIZE
    int     dirty;              /* line is dirty flag */
	ANTIC_W antic_w;
	GTIA_W	gtia_w;
	GTIA_H	gtia_h;
	GTIA_R	gtia_r;
#endif
	word	data[48];			/* graphics data buffer (text through chargen) */
}   VIDEO;

typedef struct {

	ANTIC_R r;					/* read registers */
	ANTIC_W w;					/* write registers */

	int 	cmd;				/* currently executed display list command */
	int 	modelines;			/* number of lines for current ANTIC mode */
	int 	pfwidth;			/* playfield width */
	int 	steal_cycles;		/* steal how many cpu cycles for this line ? */
	int 	vscrol_old; 		/* old vscrol value */
	int 	hscrol_old; 		/* old hscrol value */
	int 	dpage;				/* display list address page */
	int 	doffs;				/* display list offset into page */
	int 	vpage;				/* video data source page */
	int 	voffs;				/* video data offset into page */
    int     pmbase_s;           /* p/m graphics single line source base */
    int     pmbase_d;           /* p/m graphics double line source base */
	int 	chbase; 			/* character mode source base */
	int 	chand;				/* character and mask (chactl) */
	int 	chxor;				/* character xor mask (chactl) */
    int     scanline;
    byte    cclock[256];        /* color clock buffer filled by ANTIC */
	byte	pmbits[256+32]; 	/* player missile buffer filled by GTIA */
	word	color_lookup[256];	/* color lookup table */
	byte   *prio_table[64]; 	/* player/missile priority tables */
	VIDEO  *video[TOTAL_LINES]; /* video buffer */
	dword  *cclk_expand;		/* shared buffer for the following */
	dword  *pf_21;				/* 1cclk 2 color txt 2,3 */
	dword  *pf_x10b;			/* 1cclk 4 color txt 4,5, gfx D,E */
	dword  *pf_3210b2;			/* 1cclk 5 color txt 6,7, gfx 9,B,C */
	dword  *pf_210b4;			/* 4cclk 4 color gfx 8 */
	dword  *pf_210b2;			/* 2cclk 4 color gfx A */
	dword  *pf_1b;				/* 1cclk hires gfx F */
	dword  *pf_gtia1;			/* 1cclk gtia mode 1 */
	dword  *pf_gtia2;			/* 1cclk gtia mode 2 */
	dword  *pf_gtia3;			/* 1cclk gtia mode 3 */
	byte   *used_colors;		/* shared buffer for the following */
	byte   *uc_21;				/* used colors for txt (2,3) */
	byte   *uc_x10b;			/* used colors for txt 4,5, gfx D,E */
	byte   *uc_3210b2;			/* used colors for txt 6,7, gfx 9,B,C */
	byte   *uc_210b4;			/* used colors for gfx 8 */
	byte   *uc_210b2;			/* used colors for gfx A */
	byte   *uc_1b;				/* used colors for gfx F */
	byte   *uc_g1;				/* used colors for gfx GTIA 1 */
	byte   *uc_g2;				/* used colors for gfx GTIA 2 */
	byte   *uc_g3;				/* used colors for gfx GTIA 3 */
}   ANTIC;

extern	ANTIC antic;

int 	atari_vh_init(void);
int 	atari_vh_start(void);
void	atari_vh_stop(void);
void	atari_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

int     atari_interrupt(void);

#if ACCURATE_ANTIC_READMEM
#define RDANTIC()	cpu_readmem16(antic.dpage+antic.doffs)
#define RDVIDEO(o)	cpu_readmem16(antic.vpage+((antic.voffs+(o))&VOFFS))
#define RDCHGEN(o)	cpu_readmem16(antic.chbase+(o))
#define RDPMGFXS(o) cpu_readmem16(antic.pmbase_s+(o)+(antic.scanline>>1))
#define RDPMGFXD(o) cpu_readmem16(antic.pmbase_d+(o)+antic.scanline)
#else
#define RDANTIC()	Machine->memory_region[0][antic.dpage+antic.doffs]
#define RDVIDEO(o)	Machine->memory_region[0][antic.vpage+((antic.voffs+(o))&VOFFS)]
#define RDCHGEN(o)	Machine->memory_region[0][antic.chbase+(o)]
#define RDPMGFXS(o) Machine->memory_region[0][antic.pmbase_s+(o)+(antic.scanline>>1)]
#define RDPMGFXD(o) Machine->memory_region[0][antic.pmbase_d+(o)+antic.scanline]
#endif

#if OPTIMIZE
/*****************************************************************************
 * check for relevant changes in ANTIC or GTIA values
 *****************************************************************************/

#define USED_PM_COLORS \
	(gtia.h.usedp | gtia.h.usedm0 | gtia.h.usedm1 | gtia.h.usedm2 | gtia.h.usedm3)

#define COMPARE_RELEVANT(used_colors) \
   (video->cmd != antic.cmd || \
	video->antic_w.dmactl != antic.w.dmactl || \
	video->antic_w.chactl != antic.w.chactl || \
	((antic.cmd & ANTIC_HSCR) && video->antic_w.hscrol != antic.w.hscrol) || \
	((antic.cmd & ANTIC_HSCR) && video->antic_w.vscrol != antic.w.vscrol) || \
	*(dword*)&video->gtia_h.hposp0 != *(dword*)&gtia.h.hposp0 || \
	*(dword*)&video->gtia_h.sizep0 != *(dword*)&gtia.h.sizep0 || \
    *(dword*)&video->gtia_h.grafp0 != *(dword*)&gtia.h.grafp0 || \
	*(dword*)&video->gtia_h.hposm0 != *(dword*)&gtia.h.hposm0 || \
    *(dword*)&video->gtia_h.grafm0 != *(dword*)&gtia.h.grafm0 || \
	((used_colors & 0x01) && video->gtia_w.colpf0 != gtia.w.colpf0) || \
	((used_colors & 0x02) && video->gtia_w.colpf1 != gtia.w.colpf1) || \
	((used_colors & 0x04) && video->gtia_w.colpf2 != gtia.w.colpf2) || \
	((used_colors & 0x08) && video->gtia_w.colpf3 != gtia.w.colpf3) || \
	((used_colors & 0x10) && video->gtia_w.colpm0 != gtia.w.colpm0) || \
    ((used_colors & 0x20) && video->gtia_w.colpm1 != gtia.w.colpm1) || \
    ((used_colors & 0x40) && video->gtia_w.colpm2 != gtia.w.colpm2) || \
    ((used_colors & 0x80) && video->gtia_w.colpm3 != gtia.w.colpm3) || \
    video->gtia_h.sizem != gtia.h.sizem || \
	video->gtia_w.colbk != gtia.w.colbk || \
	video->gtia_w.prior != gtia.w.prior)

/*****************************************************************************
 * copy relevant ANTIC and GTIA values to scan line buffer
 *****************************************************************************/
#define UPDATE_RELEVANT \
	video->dirty = 1; \
	video->cmd = antic.cmd; \
	video->antic_w.dmactl = antic.w.dmactl; \
	video->antic_w.chactl = antic.w.chactl; \
	video->antic_w.hscrol = antic.w.hscrol; \
	video->antic_w.vscrol = antic.w.vscrol; \
	*(dword*)&video->gtia_h.hposp0 = *(dword*)&gtia.h.hposp0; \
	*(dword*)&video->gtia_h.sizep0 = *(dword*)&gtia.h.sizep0; \
	*(dword*)&video->gtia_h.grafp0 = *(dword*)&gtia.h.grafp0; \
    *(dword*)&video->gtia_h.hposm0 = *(dword*)&gtia.h.hposm0; \
	*(dword*)&video->gtia_h.grafm0 = *(dword*)&gtia.h.grafm0; \
    *(dword*)&video->gtia_w.colpm0 = *(dword*)&gtia.w.colpm0; \
	*(dword*)&video->gtia_w.colpf0 = *(dword*)&gtia.w.colpf0; \
    video->gtia_h.sizem = gtia.h.sizem; \
	video->gtia_w.colbk = gtia.w.colbk; \
	video->gtia_w.prior = gtia.w.prior

#define PREPARE()												\
byte *dst = &antic.cclock[PMOFFSET];							\
byte used_colors = USED_PM_COLORS;								\
	if (video->dirty || COMPARE_RELEVANT(used_colors))			\
	{															\
		UPDATE_RELEVANT

#define PREPARE_TXT2(width) 									\
byte *dst = &antic.cclock[PMOFFSET];							\
word changes = 0;												\
byte used_colors = USED_PM_COLORS;								\
int i;															\
    for (i = 0; i < width; i++)                                 \
	{															\
	word ch = RDVIDEO(i) << 3;									\
		if (ch & 0x400) 										\
		{														\
			ch &= 0x3f8;										\
			ch = RDCHGEN(ch + antic.w.chbasl);					\
			ch = (ch ^ antic.chxor) & antic.chand;				\
		}														\
		else													\
			ch = RDCHGEN(ch + antic.w.chbasl);					\
        used_colors |= antic.uc_21[ch];                         \
		changes |= ch ^ video->data[i]; 						\
		video->data[i] = ch;									\
	}															\
	if (video->dirty || changes || COMPARE_RELEVANT(used_colors)) \
	{															\
		UPDATE_RELEVANT

#define PREPARE_TXT3(width) 									\
byte *dst = &antic.cclock[PMOFFSET];							\
word changes = 0;												\
byte used_colors = USED_PM_COLORS;								\
int i;															\
    for (i = 0; i < width; i++)                                 \
	{															\
	word ch = RDVIDEO(i) << 3;									\
		if (ch & 0x400) 										\
		{														\
			ch &= 0x3f8;										\
			if ((ch & 0x300) == 0x300)							\
			{													\
				if (antic.w.chbasl < 2) /* first lines empty */ \
					ch = 0x00;									\
				else /* lines 2..7 are standard, 8&9 are 0&1 */ \
					ch = RDCHGEN(ch + (antic.w.chbasl & 7));	\
			}													\
			else												\
			{													\
				if (antic.w.chbasl > 7) /* last lines empty */	\
					ch = 0x00;									\
				else /* lines 0..7 are standard */				\
					ch = RDCHGEN(ch + antic.w.chbasl);			\
			}													\
            ch = (ch ^ antic.chxor) & antic.chand;              \
        }                                                       \
		else													\
		{														\
			if ((ch & 0x300) == 0x300)							\
			{													\
				if (antic.w.chbasl < 2) /* first lines empty */ \
					ch = 0x00;									\
				else /* lines 2..7 are standard, 8&9 are 0&1 */ \
					ch = RDCHGEN(ch + (antic.w.chbasl & 7));	\
			}													\
			else												\
			{													\
				if (antic.w.chbasl > 7) /* last lines empty */	\
					ch = 0x00;									\
				else /* lines 0..7 are standard */				\
					ch = RDCHGEN(ch + antic.w.chbasl);			\
            }                                                   \
		}														\
        used_colors |= antic.uc_21[ch];                         \
		changes |= ch ^ video->data[i]; 						\
		video->data[i] = ch;									\
	}															\
	if (video->dirty || changes || COMPARE_RELEVANT(used_colors)) \
	{															\
		UPDATE_RELEVANT

#define PREPARE_TXT45(width,shift)                              \
byte *dst = &antic.cclock[PMOFFSET];							\
word changes = 0;												\
byte used_colors = USED_PM_COLORS;								\
int i;															\
    for (i = 0; i < width; i++)                                 \
	{															\
	word ch = RDVIDEO(i) << 3;									\
		ch = ((ch>>2)&0x100)|RDCHGEN((ch&0x3f8)+(antic.w.chbasl>>shift)); \
		changes |= ch ^ video->data[i]; 						\
		used_colors |= antic.uc_x10b[ch];						\
		video->data[i] = ch;									\
	}															\
    if (video->dirty || changes || COMPARE_RELEVANT(used_colors)) \
	{															\
		UPDATE_RELEVANT

#define PREPARE_TXT67(width,shift)								\
byte *dst = &antic.cclock[PMOFFSET];							\
word changes = 0;												\
byte used_colors = USED_PM_COLORS;								\
int i;															\
    for (i = 0; i < width; i++)                                 \
	{															\
	word ch = RDVIDEO(i) << 3;									\
		ch = (ch&0x600)|(RDCHGEN((ch&0x1f8)+(antic.w.chbasl>>shift))<<1); \
		changes |= ch ^ video->data[i]; 						\
		used_colors |= antic.uc_3210b2[ch]; 					\
		video->data[i] = ch;									\
	}															\
	if (video->dirty || changes || COMPARE_RELEVANT(used_colors)) \
	{															\
		UPDATE_RELEVANT

#define PREPARE_GFX8(width) 									\
byte *dst = &antic.cclock[PMOFFSET];							\
word changes = 0;												\
byte used_colors = USED_PM_COLORS;								\
int i;															\
    for (i = 0; i < width; i++)                                 \
	{															\
	word ch = RDVIDEO(i) << 2;									\
		used_colors |= antic.uc_210b4[ch];						\
		changes |= ch ^ video->data[i]; 						\
		video->data[i] = ch;									\
	}															\
	if (video->dirty || changes || COMPARE_RELEVANT(used_colors)) \
	{															\
		UPDATE_RELEVANT

#define PREPARE_GFX9BC(width)									\
byte *dst = &antic.cclock[PMOFFSET];							\
word changes = 0;												\
byte used_colors = USED_PM_COLORS;								\
int i;															\
	for (i = 0; i < width; i++) 								\
	{															\
	word ch = RDVIDEO(i) << 1;									\
		used_colors |= antic.uc_210b2[ch];						\
		changes |= ch ^ video->data[i]; 						\
		video->data[i] = ch;									\
	}															\
	if (video->dirty || changes || COMPARE_RELEVANT(used_colors)) \
	{															\
        UPDATE_RELEVANT

#define PREPARE_GFXA(width) 									\
byte *dst = &antic.cclock[PMOFFSET];							\
word changes = 0;												\
byte used_colors = USED_PM_COLORS;								\
int i;															\
	for (i = 0; i < width; i++) 								\
	{															\
	word ch = RDVIDEO(i) << 1;									\
		used_colors |= antic.uc_210b2[ch];						\
		changes |= ch ^ video->data[i]; 						\
		video->data[i] = ch;									\
	}															\
	if (video->dirty || changes || COMPARE_RELEVANT(used_colors)) \
	{															\
		UPDATE_RELEVANT

#define PREPARE_GFXDE(width)									\
byte *dst = &antic.cclock[PMOFFSET];							\
word changes = 0;												\
byte used_colors = USED_PM_COLORS;								\
int i;															\
	for (i = 0; i < width; i++) 								\
	{															\
	word ch = RDVIDEO(i);										\
		used_colors |= antic.uc_x10b[ch];						\
		changes |= ch ^ video->data[i]; 						\
		video->data[i] = ch;									\
	}															\
	if (video->dirty || changes || COMPARE_RELEVANT(used_colors)) \
	{															\
		UPDATE_RELEVANT

#define PREPARE_GFXF(width) 									\
byte *dst = &antic.cclock[PMOFFSET];							\
word changes = 0;												\
byte used_colors = USED_PM_COLORS;								\
int i;															\
	for (i = 0; i < width; i++) 								\
	{															\
	word ch = RDVIDEO(i);										\
		used_colors |= antic.uc_1b[ch]; 						\
		changes |= ch ^ video->data[i]; 						\
		video->data[i] = ch;									\
	}															\
	if (video->dirty || changes || COMPARE_RELEVANT(used_colors)) \
	{															\
		UPDATE_RELEVANT

#define PREPARE_GFXG1(width)									\
byte *dst = &antic.cclock[PMOFFSET];							\
word changes = 0;												\
byte used_colors = USED_PM_COLORS;								\
int i;															\
	for (i = 0; i < width; i++) 								\
	{															\
	word ch = RDVIDEO(i);										\
		used_colors |= antic.uc_g1[ch]; 						\
        changes |= ch ^ video->data[i];                         \
		video->data[i] = ch;									\
	}															\
	if (video->dirty || changes || COMPARE_RELEVANT(used_colors)) \
	{															\
		UPDATE_RELEVANT

#define PREPARE_GFXG2(width)									\
byte *dst = &antic.cclock[PMOFFSET];							\
word changes = 0;												\
byte used_colors = USED_PM_COLORS;								\
int i;															\
	for (i = 0; i < width; i++) 								\
	{															\
	word ch = RDVIDEO(i);										\
		used_colors |= antic.uc_g2[ch]; 						\
        changes |= ch ^ video->data[i];                         \
		video->data[i] = ch;									\
	}															\
	if (video->dirty || changes || COMPARE_RELEVANT(used_colors)) \
	{															\
		UPDATE_RELEVANT

#define PREPARE_GFXG3(width)									\
byte *dst = &antic.cclock[PMOFFSET];							\
word changes = 0;												\
byte used_colors = USED_PM_COLORS;								\
int i;															\
	for (i = 0; i < width; i++) 								\
	{															\
	word ch = RDVIDEO(i);										\
		used_colors |= antic.uc_g3[ch]; 						\
        changes |= ch ^ video->data[i];                         \
		video->data[i] = ch;									\
	}															\
	if (video->dirty || changes || COMPARE_RELEVANT(used_colors)) \
	{															\
		UPDATE_RELEVANT

/******************************************************************
 * common end of a single antic/gtia mode emulation function
 ******************************************************************/
#define POST()													\
	}															\
	--antic.modelines

#define POST_GFX(width,oddlines)								\
	}															\
	if (!(antic.modelines & oddlines))							\
        antic.steal_cycles += width;                            \
    if (!--antic.modelines)                                     \
		antic.voffs = (antic.voffs + width) & VOFFS

#define POST_TXT(width,oddlines)								\
	}															\
	if (!(antic.modelines & oddlines))							\
		antic.steal_cycles += width;							\
	if (!--antic.modelines) 									\
		antic.voffs = (antic.voffs + width) & VOFFS;			\
	else														\
		antic.w.chbasl += 1 - ((antic.w.chactl >> 1) & 2)

#else   /* !OPTIMIZE */

#define PREPARE()												\
byte *dst = &antic.cclock[PMOFFSET]

#define PREPARE_TXT2(width) 									\
byte *dst = &antic.cclock[PMOFFSET];							\
int i;															\
    for (i = 0; i < width; i++)                                 \
	{															\
	word ch = RDVIDEO(i) << 3;									\
	word xor = (ch & 0x400) ? antic.chxor : 0;					\
	word and = (ch & 0x400) ? antic.chand : 0xff;				\
		ch &= 0x3f8;											\
		ch = RDCHGEN(ch + antic.w.chbasl);						\
		ch = (ch ^ xor) & and;									\
		video->data[i] = ch;									\
	}

#define PREPARE_TXT3(width) 									\
byte *dst = &antic.cclock[PMOFFSET];							\
int i;															\
    for (i = 0; i < width; i++)                                 \
	{															\
	word ch = RDVIDEO(i) << 3;									\
	word xor = (ch & 0x400) ? antic.chxor : 0;					\
	word and = (ch & 0x400) ? antic.chand : 0xff;				\
		ch &= 0x3f8;											\
		if ((ch & 0x300) == 0x300)								\
		{														\
			if (antic.w.chbasl < 2) /* first two lines empty */ \
				ch = 0x00;										\
			else /* lines 2..7 are standard, 8&9 are 0&1 */ 	\
				ch = RDCHGEN(ch + (antic.w.chbasl & 7));		\
        }                                                       \
		else													\
		{														\
			if (antic.w.chbasl > 7) /* last two lines empty */	\
				ch = 0x00;										\
			else /* lines 0..7 are standard */					\
				ch = RDCHGEN(ch + antic.w.chbasl);				\
		}														\
		ch = (ch ^ xor) & and;									\
		video->data[i] = ch;									\
	}

#define PREPARE_TXT45(width,shift)								\
byte *dst = &antic.cclock[PMOFFSET];							\
int i;															\
    for (i = 0; i < width; i++)                                 \
	{															\
	word ch = RDVIDEO(i) << 3;									\
		ch = ((ch>>2)&0x100)|RDCHGEN((ch&0x3f8)+(antic.w.chbasl>>shift)); \
		video->data[i] = ch;									\
	}


#define PREPARE_TXT67(width,shift)								\
byte *dst = &antic.cclock[PMOFFSET];							\
int i;															\
    for (i = 0; i < width; i++)                                 \
	{															\
	word ch = RDVIDEO(i) << 3;									\
		ch = (ch&0x600)|(RDCHGEN((ch&0x1f8)+(antic.w.chbasl>>shift))<<1); \
		video->data[i] = ch;									\
	}

#define PREPARE_GFX8(width)                                     \
byte *dst = &antic.cclock[PMOFFSET];							\
int i;															\
	for (i = 0; i < width; i++) 								\
		video->data[i] = RDVIDEO(i) << 2

#define PREPARE_GFX9BC(width)									\
byte *dst = &antic.cclock[PMOFFSET];							\
int i;															\
	for (i = 0; i < width; i++) 								\
		video->data[i] = RDVIDEO(i) << 1

#define PREPARE_GFXA(width) 									\
byte *dst = &antic.cclock[PMOFFSET];							\
int i;															\
	for (i = 0; i < width; i++) 								\
		video->data[i] = RDVIDEO(i) << 1

#define PREPARE_GFXDE(width)									\
byte *dst = &antic.cclock[PMOFFSET];							\
int i;															\
	for (i = 0; i < width; i++) 								\
		video->data[i] = RDVIDEO(i)

#define PREPARE_GFXF(width) 									\
byte *dst = &antic.cclock[PMOFFSET];							\
int i;															\
	for (i = 0; i < width; i++) 								\
		video->data[i] = RDVIDEO(i)

#define PREPARE_GFXG1(width)									\
byte *dst = &antic.cclock[PMOFFSET];							\
int i;															\
	for (i = 0; i < width; i++) 								\
		video->data[i] = RDVIDEO(i)

#define PREPARE_GFXG2(width)									\
byte *dst = &antic.cclock[PMOFFSET];							\
int i;															\
	for (i = 0; i < width; i++) 								\
		video->data[i] = RDVIDEO(i)

#define PREPARE_GFXG3(width)									\
byte *dst = &antic.cclock[PMOFFSET];							\
int i;															\
	for (i = 0; i < width; i++) 								\
		video->data[i] = RDVIDEO(i)

/******************************************************************
 * common end of a single antic/gtia mode emulation function
 ******************************************************************/
#define POST()													\
	--antic.modelines

#define POST_GFX(width,oddlines)								\
	if (!(antic.modelines & oddlines))							\
        antic.steal_cycles += width;                            \
    if (!--antic.modelines)                                     \
		antic.voffs = (antic.voffs + width) & VOFFS

#define POST_TXT(width,oddlines)								\
	if (!(antic.modelines & oddlines))							\
		antic.steal_cycles += width;							\
	if (!--antic.modelines) 									\
		antic.voffs = (antic.voffs + width) & VOFFS;			\
	else														\
		antic.w.chbasl += 1 - ((antic.w.chactl >> 1) & 2)

#endif

/* erase a number of color clocks to background color PBK */
#define ERASE(w) memset(dst, PBK, w); dst += w

#define REP08(FUNC) 						\
	ERASE(8*4); 							\
	FUNC( 0); FUNC( 1); FUNC( 2); FUNC( 3); \
	FUNC( 4); FUNC( 5); FUNC( 6); FUNC( 7); \
	ERASE(8*4)

#define REP10(FUNC) 						\
	ERASE(4*4); 							\
	FUNC( 0); FUNC( 1); FUNC( 2); FUNC( 3); \
	FUNC( 4); FUNC( 5); FUNC( 6); FUNC( 7); \
	FUNC( 8); FUNC( 9); 					\
	ERASE(4*4)

#define REP12(FUNC) 						\
	FUNC( 0); FUNC( 1); FUNC( 2); FUNC( 3); \
	FUNC( 4); FUNC( 5); FUNC( 6); FUNC( 7); \
	FUNC( 8); FUNC( 9); FUNC(10); FUNC(11)	

#define REP16(FUNC) 						\
	ERASE(8*4); 							\
	FUNC( 0); FUNC( 1); FUNC( 2); FUNC( 3); \
	FUNC( 4); FUNC( 5); FUNC( 6); FUNC( 7); \
	FUNC( 8); FUNC( 9); FUNC(10); FUNC(11); \
	FUNC(12); FUNC(13); FUNC(14); FUNC(15); \
	ERASE(8*4)

#define REP20(FUNC) 						\
	ERASE(4*4); 							\
	FUNC( 0); FUNC( 1); FUNC( 2); FUNC( 3); \
	FUNC( 4); FUNC( 5); FUNC( 6); FUNC( 7); \
	FUNC( 8); FUNC( 9); FUNC(10); FUNC(11); \
	FUNC(12); FUNC(13); FUNC(14); FUNC(15); \
	FUNC(16); FUNC(17); FUNC(18); FUNC(19); \
	ERASE(4*4)

#define REP24(FUNC) 						\
	FUNC( 0); FUNC( 1); FUNC( 2); FUNC( 3); \
	FUNC( 4); FUNC( 5); FUNC( 6); FUNC( 7); \
	FUNC( 8); FUNC( 9); FUNC(10); FUNC(11); \
	FUNC(12); FUNC(13); FUNC(14); FUNC(15); \
	FUNC(16); FUNC(17); FUNC(18); FUNC(19); \
	FUNC(20); FUNC(21); FUNC(22); FUNC(23)

#define REP32(FUNC) 						\
	ERASE(8*4); 							\
	FUNC( 0); FUNC( 1); FUNC( 2); FUNC( 3); \
	FUNC( 4); FUNC( 5); FUNC( 6); FUNC( 7); \
	FUNC( 8); FUNC( 9); FUNC(10); FUNC(11); \
	FUNC(12); FUNC(13); FUNC(14); FUNC(15); \
	FUNC(16); FUNC(17); FUNC(18); FUNC(19); \
	FUNC(20); FUNC(21); FUNC(22); FUNC(23); \
	FUNC(24); FUNC(25); FUNC(26); FUNC(27); \
	FUNC(28); FUNC(29); FUNC(30); FUNC(31); \
	ERASE(8*4)

#define REP40(FUNC) 						\
	ERASE(4*4); 							\
	FUNC( 0); FUNC( 1); FUNC( 2); FUNC( 3); \
	FUNC( 4); FUNC( 5); FUNC( 6); FUNC( 7); \
	FUNC( 8); FUNC( 9); FUNC(10); FUNC(11); \
	FUNC(12); FUNC(13); FUNC(14); FUNC(15); \
	FUNC(16); FUNC(17); FUNC(18); FUNC(19); \
	FUNC(20); FUNC(21); FUNC(22); FUNC(23); \
	FUNC(24); FUNC(25); FUNC(26); FUNC(27); \
	FUNC(28); FUNC(29); FUNC(30); FUNC(31); \
	FUNC(32); FUNC(33); FUNC(34); FUNC(35); \
	FUNC(36); FUNC(37); FUNC(38); FUNC(39); \
	ERASE(4*4)

#define REP48(FUNC) 						\
	FUNC( 0); FUNC( 1); FUNC( 2); FUNC( 3); \
	FUNC( 4); FUNC( 5); FUNC( 6); FUNC( 7); \
	FUNC( 8); FUNC( 9); FUNC(10); FUNC(11); \
	FUNC(12); FUNC(13); FUNC(14); FUNC(15); \
	FUNC(16); FUNC(17); FUNC(18); FUNC(19); \
	FUNC(20); FUNC(21); FUNC(22); FUNC(23); \
	FUNC(24); FUNC(25); FUNC(26); FUNC(27); \
	FUNC(28); FUNC(29); FUNC(30); FUNC(31); \
	FUNC(32); FUNC(33); FUNC(34); FUNC(35); \
	FUNC(36); FUNC(37); FUNC(38); FUNC(39); \
	FUNC(40); FUNC(41); FUNC(42); FUNC(43); \
	FUNC(44); FUNC(45); FUNC(46); FUNC(47)

typedef void (*renderer_function)(VIDEO *video);

void	antic_mode_0_xx(VIDEO *video);
void	antic_mode_1_xx(VIDEO *video);
void	antic_mode_2_32(VIDEO *video);
void	antic_mode_2_40(VIDEO *video);
void	antic_mode_2_48(VIDEO *video);
void	antic_mode_3_32(VIDEO *video);
void	antic_mode_3_40(VIDEO *video);
void	antic_mode_3_48(VIDEO *video);
void	antic_mode_4_32(VIDEO *video);
void	antic_mode_4_40(VIDEO *video);
void	antic_mode_4_48(VIDEO *video);
void	antic_mode_5_32(VIDEO *video);
void	antic_mode_5_40(VIDEO *video);
void	antic_mode_5_48(VIDEO *video);
void	antic_mode_6_32(VIDEO *video);
void	antic_mode_6_40(VIDEO *video);
void	antic_mode_6_48(VIDEO *video);
void	antic_mode_7_32(VIDEO *video);
void	antic_mode_7_40(VIDEO *video);
void	antic_mode_7_48(VIDEO *video);
void	antic_mode_8_32(VIDEO *video);
void	antic_mode_8_40(VIDEO *video);
void	antic_mode_8_48(VIDEO *video);
void	antic_mode_9_32(VIDEO *video);
void	antic_mode_9_40(VIDEO *video);
void	antic_mode_9_48(VIDEO *video);
void	antic_mode_a_32(VIDEO *video);
void	antic_mode_a_40(VIDEO *video);
void	antic_mode_a_48(VIDEO *video);
void	antic_mode_b_32(VIDEO *video);
void	antic_mode_b_40(VIDEO *video);
void	antic_mode_b_48(VIDEO *video);
void	antic_mode_c_32(VIDEO *video);
void	antic_mode_c_40(VIDEO *video);
void	antic_mode_c_48(VIDEO *video);
void	antic_mode_d_32(VIDEO *video);
void	antic_mode_d_40(VIDEO *video);
void	antic_mode_d_48(VIDEO *video);
void	antic_mode_e_32(VIDEO *video);
void	antic_mode_e_40(VIDEO *video);
void	antic_mode_e_48(VIDEO *video);
void	antic_mode_f_32(VIDEO *video);
void	antic_mode_f_40(VIDEO *video);
void	antic_mode_f_48(VIDEO *video);

void	gtia_mode_1_32(VIDEO *video);
void	gtia_mode_1_40(VIDEO *video);
void	gtia_mode_1_48(VIDEO *video);
void	gtia_mode_2_32(VIDEO *video);
void	gtia_mode_2_40(VIDEO *video);
void	gtia_mode_2_48(VIDEO *video);
void	gtia_mode_3_32(VIDEO *video);
void	gtia_mode_3_40(VIDEO *video);
void	gtia_mode_3_48(VIDEO *video);
void	gtia_render(VIDEO *video);


