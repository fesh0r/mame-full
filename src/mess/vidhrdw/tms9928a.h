/*
** File: tms9928a.h -- software implementation of the TMS9928A VDP.
**
** By Sean Young 1999 (sean@msxnet.org).
*/

extern unsigned char TMS9928A_palette[];
extern unsigned short TMS9928A_colortable[];
/* initialise palette function */
void tms9928A_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

#define TMS9928A_PALETTE_SIZE           16
#define TMS9928A_COLORTABLE_SIZE        32


typedef struct {
    // TMS9928A internal settings
    UINT8 ReadAhead,Regs[8],StatusReg,oldStatusReg;
    int Addr,FirstByte,INT,BackColour,Change,mode;
    int colour,pattern,nametbl,spriteattribute,spritepattern;
    int colourmask,patternmask;
    void (*INTCallback)(int);
    // memory
    UINT8 *vMem, *dBackMem;
    struct osd_bitmap *tmpbmp;
    int vramsize;
    // emulation settings
    int LimitSprites; // max 4 sprites on a row, like original TMS9918A
    // dirty tables
    char anyDirtyColour, anyDirtyName, anyDirtyPattern;
    char *DirtyColour, *DirtyName, *DirtyPattern;
} TMS9928A;


/*
** The init, reset and shutdown functions
*/
int TMS9928A_start (unsigned int vramsize);
void TMS9928A_reset (void);
void TMS9928A_stop (void);

/*
** The I/O functions
*/
UINT8 TMS9928A_vram_r (void);
void TMS9928A_vram_w (UINT8 val);
UINT8 TMS9928A_register_r (void);
void TMS9928A_register_w (UINT8 val);

/*
** Call this function to render the screen.
*/
void TMS9928A_refresh (struct osd_bitmap *, int full_refresh);

/*
** This next function must be called 50 or 60 times per second,
** to generate the necessary interrupts
*/
int TMS9928A_interrupt (void);

/*
** The parameter is a function pointer. This function is called whenever
** the state of the INT output of the TMS9918A changes.
*/
void TMS9928A_int_callback (void (*callback)(int));

/*
** Set display of illegal sprites on or off
*/
void TMS9928A_set_spriteslimit (int);

