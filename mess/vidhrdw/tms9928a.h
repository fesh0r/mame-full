/*
** File: tms9928a.h -- software implementation of the TMS9928A VDP.
**
** By Sean Young 1999 (sean@msxnet.org).
*/


#define TMS9928A_PALETTE_SIZE           16
#define TMS9928A_COLORTABLE_SIZE        16

/*
** The different models
*/

#define TMS99x8A	(1)
#define TMS99x8		(2)

/*
** The init, reset and shutdown functions
*/
int TMS9928A_start (int model, unsigned int vram);
void TMS9928A_reset (void);
extern PALETTE_INIT( tms9928a );

/*
** The I/O functions
*/
READ_HANDLER (TMS9928A_vram_r);
WRITE_HANDLER (TMS9928A_vram_w);
READ_HANDLER (TMS9928A_register_r);
WRITE_HANDLER (TMS9928A_register_w);

/*
** Call this function to render the screen.
*/
extern VIDEO_UPDATE( tms9928a );

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

/*
** After loading a state, call this function 
*/
void TMS9928A_post_load (void);

/*
** MachineDriver video declarations for the TMS9928A chip
*/
extern void mdrv_tms9928a(struct InternalMachineDriver *machine, int (*video_start_proc)(void));

#define MDRV_TMS9928A(video_start_proc)		mdrv_tms9928a(machine, (video_start_##video_start_proc));

