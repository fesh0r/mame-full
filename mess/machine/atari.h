#ifndef ATARI_H
#define ATARI_H

typedef struct {
	int  serout_count;
	int  serout_offs;
	UINT8 serout_buff[512];
	UINT8 serout_chksum;
	int  serout_delay;

	int  serin_count;
	int  serin_offs;
	UINT8 serin_buff[512];
	UINT8 serin_chksum;
	int  serin_delay;
}	ATARI_FDC;
extern ATARI_FDC atari_fdc;

typedef struct {
	struct {
		UINT8 painp; 	/* pia port A input*/
		UINT8 pbinp; 	/* pia port B input */
	}	r;	/* read registers */
	struct {
		UINT8 paout; 	/* pia port A output */
		UINT8 pbout; 	/* pia port B output */
	}	w;	/* write registers */
	struct {
		UINT8 pactl; 	/* pia port A control */
		UINT8 pbctl; 	/* pia port B control */
	}	rw; /* read/write registers */
	struct {
		UINT8 pamsk; 	/* pia port A mask */
		UINT8 pbmsk; 	/* pia port B mask */
	}	h;	/* helper variables */
}	ATARI_PIA;
extern ATARI_PIA atari_pia;

extern void a400_init_machine(void);
extern void a800_init_machine(void);

extern int a800_floppy_init(int id);
extern void a800_floppy_exit(int id);

extern int a800_id_rom(int id);
extern int a800_rom_init(int id);
extern void a800_rom_exit(int id);

extern void a800xl_init_machine(void);
extern int a800xl_load_rom(int id);
extern int a800xl_id_rom(int id);

extern void a5200_init_machine(void);
extern int a5200_id_rom(int id);
extern int a5200_rom_init(int id);
extern void a5200_rom_exit(int id);

extern void a800_close_floppy(void);

extern READ_HANDLER ( MRA_GTIA );
extern READ_HANDLER ( MRA_PIA );
extern READ_HANDLER ( MRA_ANTIC );

extern WRITE_HANDLER ( MWA_GTIA );
extern WRITE_HANDLER ( MWA_PIA  );
extern WRITE_HANDLER ( MWA_ANTIC );

extern READ_HANDLER ( atari_serin_r );
extern WRITE_HANDLER ( atari_serout_w );
extern void atari_interrupt_cb(int mask);

extern void a800_handle_keyboard(void);
extern void a5200_handle_keypads(void);

#endif
