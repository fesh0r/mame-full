#ifndef __SVISION_H_
#define __SVISION_H_

/*
  this shows the interface that your files offer
  (the only exception is your GAME/CONS/COMP structure!)

  allows the compiler generate the correct dependancy,
  so it can recognize which files to rebuild

  the programmer sees, when he changes this interface, he might
  have to adapt other drivers

  you must not do declarations in c files with external bindings
*/
#ifdef RUNTIME_LOADER
# ifdef __cplusplus
extern "C" void svision_runtime_loader_init(void);
# else
extern void svision_runtime_loader_init(void);
# endif
#endif

typedef struct {
    UINT8 reg[3];
    int l_pos;
    int r_pos;
    int size;
} SVISION_CHANNEL;
extern SVISION_CHANNEL svision_channel[2];
extern int svision_custom_start (const struct MachineSound *driver);
extern void svision_custom_stop (void);
extern void svision_custom_update (void);
extern void svision_soundport_w (SVISION_CHANNEL *channel, int offset, int data);

#endif
