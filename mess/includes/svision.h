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

#endif
