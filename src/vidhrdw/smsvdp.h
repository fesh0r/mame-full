/***************************************************************************

  vidhrdw/smsvdp.h

***************************************************************************/

/* Public interface */

//#define OLD_VIDEO

extern unsigned char SMSVDP_palette[];
extern unsigned short SMSVDP_colortable[];

#define SMSVDP_PALETTE_SIZE		32
#define SMSVDP_COLORTABLE_SIZE	32

extern int SMSVDP_interrupt(void);
extern int SMSVDP_vram_r(int offset);
extern void SMSVDP_vram_w(int offset, int data);
extern int SMSVDP_register_r(int offset);
extern void SMSVDP_register_w(int offset, int data);
extern int SMSVDP_start(unsigned int vram_size);
extern void SMSVDP_stop(void);
extern void SMSVDP_refresh(struct osd_bitmap *bitmap);


/* Private interface */

#define MAX_STATUS_REGISTERS	2 /* Actually 1, but can we have an array of 1? */
#define MAX_CONTROL_REGISTERS	11
#define MAX_PALETTE_SIZE		64 /* 32 for the SMS, 64 for the GameGear */
#define MAX_SPRITES             64

#define MAX_DIRTY_PAT_GEN       8*(256+192)*4
#define MAX_DIRTY_COLOR         8*(256+192)*4
#define MAX_DIRTY_PAT_NAME      24*32

typedef struct
{
	unsigned char *VRAM; /* Can be up to 16K */
	int VRAM_size;	/* Actually store the size-1 here */
	int write_ptr; /* Offsets into VRAM */
	int read_ptr; /* Offsets into VRAM */
	int palette_ptr; /* Offset into palette RAM */
	int creg[MAX_CONTROL_REGISTERS]; /* 8 write-only registers */
	int sreg[MAX_STATUS_REGISTERS]; /* 1 status register */
	int temp_write; /* Holds first half of a register write, -1 when empty */
	int palette[MAX_PALETTE_SIZE];

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
} SMSVDP;

