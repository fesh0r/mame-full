/***************************************************************************

  vidhrdw/TMS9928A.h

***************************************************************************/

/* Public interface */

extern unsigned char TMS9928A_palette[];
extern unsigned short TMS9928A_colortable[];

#define TMS9928A_PALETTE_SIZE		16
#define TMS9928A_COLORTABLE_SIZE	32

extern int TMS9928A_interrupt(void);
extern int TMS9928A_vram_r(void);
extern void TMS9928A_vram_w(int data);
extern int TMS9928A_register_r(void);
extern void TMS9928A_register_w(int data);
extern int TMS9928A_start(unsigned int vram_size);
extern void TMS9928A_stop(void);
extern void TMS9928A_refresh(struct osd_bitmap *bitmap);


/* Private interface */

#define MAX_STATUS_REGISTERS	2 /* Actually 1, but can we have an array of 1? */
#define MAX_CONTROL_REGISTERS	8

#define MAX_DIRTY_PAT_GEN       8*256*3
#define MAX_DIRTY_COLOR         8*256*3
#define MAX_DIRTY_PAT_NAME      24*40

typedef struct
{
	unsigned char *VRAM; /* Can be up to 16K */
	int VRAM_size;	/* Actually store the size-1 here */
	int write_ptr; /* Offsets into VRAM */
	int read_ptr; /* Offsets into VRAM */
	int creg[MAX_CONTROL_REGISTERS]; /* 8 write-only registers */
	int sreg[MAX_STATUS_REGISTERS]; /* 1 status register */
	int temp_write; /* Holds first half of a register write, -1 when empty */

	/* internal variables for emulation optimization */
	int i_screenMode;
	int i_backColor;
	int i_textColor;

	int i_patNameTabStart;
	int i_colorTabStart;
	int i_patGenTabStart;
	int i_spriteAttrTabStart;
	int i_spritePatGenTabStart;

	int i_patNameTabEnd;
	int i_colorTabEnd;
	int i_patGenTabEnd;
	int i_spriteAttrTabEnd;
	int i_spritePatGenTabEnd;

    char i_anyDirtyPatGen;
    char i_anyDirtyColor;
    char i_anyDirtyPatName;
    char *i_dirtyPatGen;
    char *i_dirtyColor;
    char *i_dirtyPatName;

    struct osd_bitmap *i_tmpbitmap;
} TMS9928A;

