/***************************************************************************

  vidhrdw/sms.h

***************************************************************************/

/* Public interface */

extern unsigned char sms_palette[];
extern unsigned short sms_colortable[];

#define SMS_PALETTE_SIZE	32
#define SMS_COLORTABLE_SIZE	32
#define SCANLINES_PER_FRAME 280

int sms_vdp_vram_r(int offset);
void sms_vdp_vram_w(int offset, int data);
int sms_vdp_register_r(int offset);
void sms_vdp_register_w(int offset, int data);
int sms_vdp_start(void);
int gamegear_vdp_start(void);
void sms_vdp_stop(void);
void sms_vdp_refresh(struct osd_bitmap *bitmap, int full_refresh);
int sms_vdp_curline_r (int offset);
int sms_interrupt(void);


/* Private interface */

#define NORAM     0xFF      /* Byte to be returned from      */
                            /* non-existing pages and ports  */
#define HEIGHT 192
#define WIDTH  256

/** Following macros can be used in screen drivers ***********/
#define BigSprites  (VDP[1]&0x01) /* Zoomed sprites          */
#define Sprites16   (VDP[1]&0x02) /* Sprites 8x16/8x8        */
#define LeftSprites (VDP[0]&0x08) /* Shift sprites 8pix left */
#define LeftMask    (VDP[0]&0x20) /* Mask left-hand column   */
#define NoTopScroll (VDP[0]&0x40) /* Don't scroll top 16l.   */
#define NoRghScroll (VDP[0]&0x80) /* Don't scroll right 16l. */
#define HBlankON    (VDP[0]&0x10) /* Generate HBlank inter-s */
#define VBlankON    (VDP[1]&0x20) /* Generate VBlank inter-s */
#define ScreenON    (VDP[1]&0x40) /* Show screen             */
#define SprX(N) (SprTab+0x80)[N<<1] /* Sprite X coordinate   */
#define SprY(N) SprTab[N]           /* Sprite Y coordinate   */
#define SprP(N) (SprTab+0x81)[N<<1] /* Sprite pattern number */
#define SprA(N) (SprTab+0x40)[N<<1] /* Sprite attributes ?   */



