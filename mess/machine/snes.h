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
			and the others....
  		The original SNEeSe team members (other than myself ;-)) -
			Charles Bilyue - Your continued work on SNEeSe is fantastic!
			Santeri Saarimaa - Who'd have thought I'd come back to emulation ;-)

	***************************************************************************

***************************************************************************/

//#define EMULATE_SPC700												// If commented out the core will use skipper emulation

int snes_load_rom (int id);
void snes_exit_rom (int id);
void snes_init_machine(void);
void snes_shutdown_machine(void);

int snes_vh_start(void);
void snes_vh_stop(void);
void snes_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);

void RenderSNESScreenLine(struct mame_bitmap *,int);

extern unsigned short palIndx[256+1];			// +1 for fixed colour backdrop
extern unsigned short fixedColour;					// Fixed color encoded into here

extern unsigned char port21xx[256];
extern unsigned char wport21xx[2][256];

extern unsigned char port42xx[256];
extern unsigned char port43xx[256];

#ifdef EMULATE_SPC700
extern unsigned char SPCIN[4],SPCOUT[4];		// Used as crossover ports between spc and cpu
extern unsigned char spcPort[19];
extern int spc700TimerExtra;					// Used to hold whether to increment T0 & T1
void spcTimerTick(int);
#endif

extern unsigned char OAMADDRESS_L,OAMADDRESS_H;

extern unsigned char *SNES_VRAM;				// Video memory
extern unsigned char *SNES_CRAM;				// Colour memory
extern unsigned char *SNES_ORAM;				// Object memory (sprites)
extern unsigned char *SNES_WRAM;				// Work memory
extern unsigned char *SNES_SRAM;				// Save Ram / Extra Ram



