#ifndef ATARI_H
#define ATARI_H

#define NEW_POKEY	1

typedef struct {
	int  serout_count;
	int  serout_offs;
	byte serout_buff[512];
	byte serout_chksum;
	int  serout_delay;

	int  serin_count;
	int  serin_offs;
	byte serin_buff[512];
	byte serin_chksum;
	int  serin_delay;
}   POKEY;
extern	POKEY	pokey;

typedef struct {
	struct {
		byte painp; 	/* pia port A input*/
		byte pbinp; 	/* pia port B input */
	}	r;	/* read registers */
	struct {
		byte paout; 	/* pia port A output */
		byte pbout; 	/* pia port B output */
	}	w;	/* write registers */
	struct {
		byte pactl; 	/* pia port A control */
		byte pbctl; 	/* pia port B control */
	}	rw; /* read/write registers */
	struct {
		byte pamsk; 	/* pia port A mask */
		byte pbmsk; 	/* pia port B mask */
	}	h;	/* helper variables */
}	PIA;
extern	PIA 	pia;

void	atari800_init_machine(void);
void	atari800_close_floppy(void);
int     atari800_load_rom(void);
int     atari800_id_rom(const char *name, const char *gamename);

void	vcs5200_init_machine(void);
int 	vcs5200_load_rom(void);
int 	vcs5200_id_rom(const char *name, const char *gamename);

int     MRA_GTIA(int offset);
int 	MRA_PIA(int offset);
int     MRA_ANTIC(int offset);

void    MWA_GTIA(int offset, int data);
void	MWA_PIA(int offset, int data);
void    MWA_ANTIC(int offset, int data);

int     atari_serin_r(int offset);
void	atari_serout_w(int offset, int data);
void	atari_interrupt_cb(int mask);

void    atari800_handle_keyboard(void);
void	vcs5200_handle_keypads(void);

#endif
