/***************************************************************************

 Super Nintendo Entertainment System Driver - Written By Lee Hammerton aKa Savoury SnaX

 Acknowledgements

	I gratefully acknowledge the work of Karl Stenerud for his work on the processor
  cores used in this emulation and of course I hope you'll continue to work with me
  to improve this project.

	I would like to acknowledge the support of all those who helped me during SNEeSe and
  in doing so have helped get this project off the ground. There are many, many people
  to thank and so little space that I am keeping this as brief as I can :

		All snes technical document authors!
		All snes emulator authors!
			ZSnes
			Snes9x
			Snemul
			Nlksnes
			Esnes
			and the others....
		The original SNEeSe team members (other than myself ;-)) - 
			Charles Bilyue - Your continued work on SNEeSe is fantastic!
			Santeri Saarimaa - Who'd have thought I'd come back to emulation ;-)

	***************************************************************************

***************************************************************************/

#include "driver.h"
#include "../machine/snes.h"

#ifdef EMULATE_SPC700											// If this is not defined there really is no point in compiling this module

extern struct CustomSound_interface snesSoundInterface;

int spc700Interrupt(void);
void spcTimerTick(int);
READ_HANDLER ( spc_io_r );
WRITE_HANDLER ( spc_io_w );

extern const struct Memory_ReadAddress spc_readmem[];
extern const struct Memory_WriteAddress spc_writemem[];
#endif


