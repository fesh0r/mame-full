#ifndef __VC20_TAPE_H_
#define __VC20_TAPE_H_

/* put this into your gamedriver */
extern struct DACinterface vc20tape_sound_interface;

#define IODEVICE_VC20TAPE \
{\
   IO_CASSETTE,        /* type */\
   1,                  /* count */\
   "wav\0",    /* TAP, LNX and T64(maybe) later file extensions */\
   NULL,               /* private */\
   NULL,               /* id */\
   vc20_tape_attach_image,	/* init */\
   vc20_tape_detach_image,	/* exit */\
   NULL,               /* info */\
   NULL,               /* open */\
   NULL,               /* close */\
   NULL,               /* status */\
   NULL,               /* seek */\
   NULL,			   /* tell */\
   NULL,               /* input */\
   NULL,               /* output */\
   NULL,               /* input_chunk */\
   NULL                /* output_chunk */\
}

/* the function which should be called by change on readline */
extern void vc20_tape_open (void (*read_callback) (UINT32, UINT32));
extern void c16_tape_open (void);
extern void vc20_tape_close (void);

/* call this with the name of the tape image */
int vc20_tape_attach_image (int id);
void vc20_tape_detach_image (int id);

/* must be high active keys */
/* set it frequently */
extern void vc20_tape_buttons (int play, int record, int stop);
extern void vc20_tape_config (int on, int noise);

extern int vc20_tape_read (void);
extern void vc20_tape_write (int data);
extern void vc20_tape_motor (int data);

/* pressed wenn cassette is in */
extern int vc20_tape_switch (void);

/* delivers status for displaying */
extern void vc20_tape_status (char *text, int size);

#endif
