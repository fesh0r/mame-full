/******************************************************************************
 *
 *	kaypro.h
 *
 *	KAYPRO terminal emulation for CP/M
 *
 *	from Juergen Buchmueller's VT52 emulation, July 1998
 *      Benjamin C. W. Sittler, July 1998
 *
 ******************************************************************************/

#define KAYPRO_FONT_W 	8
#define KAYPRO_FONT_H 	16
#define KAYPRO_SCREEN_W	80
#define KAYPRO_SCREEN_H   25

extern int	kaypro_vh_start(void);
extern void kaypro_vh_stop(void);
extern void kaypro_vh_screenrefresh(struct osd_bitmap * bitmap, int full_refresh);

extern int	kaypro_const_r(int offset);
extern void kaypro_const_w(int offset, int data);
extern int	kaypro_conin_r(int offset);
extern void kaypro_conin_w(int offset, int data);
extern int	kaypro_conout_r(int offset);
extern void kaypro_conout_w(int offset, int data);

