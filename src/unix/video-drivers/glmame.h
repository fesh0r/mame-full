/*****************************************************************

  GLmame include file

  Copyright 1998 by Mike Oliphant - oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

  This code may be used and distributed under the terms of the
  Mame license

*****************************************************************/


/* Camera panning stuff */

typedef enum {pan_goto,pan_moveto,pan_repeat,pan_end,pan_nocab} PanType;

struct CameraPan {
  PanType type;      /* Type of pan */
  GLfloat lx,ly,lz;  /* Location of camera */
  GLfloat px,py,pz;  /* Vector to point camera along */
  GLfloat nx,ny,nz;  /* Normal to camera direction */
  int frames;        /* Number of frames for transition */
};
