/*********************************************************************

	mc10.h

	TRS-80 Radio Shack MicroColor Computer

*********************************************************************/

#ifndef MC10_H
#define MC10_H

extern void mc10_init_machine(void);
extern READ8_HANDLER ( mc10_bfff_r );
extern WRITE8_HANDLER ( mc10_bfff_w );
extern READ8_HANDLER ( mc10_port1_r );
extern READ8_HANDLER ( mc10_port2_r );
extern WRITE8_HANDLER ( mc10_port1_w );
extern WRITE8_HANDLER ( mc10_port2_w );

extern VIDEO_START( mc10 );
extern WRITE8_HANDLER ( mc10_ram_w );

#endif /* MC10_H */
