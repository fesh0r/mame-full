/*****************************************************************

  Generic OpenGL routines

  Copyright 1998 by Mike Oliphant - oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

  This code may be used and distributed under the terms of the
  Mame license

*****************************************************************/

#ifdef xgl

#include <GL/gl.h>
#include <GL/glu.h>
#include "driver.h"
#include "xmame.h"
#include "glmame.h"

#ifdef UTAH_GLX
#define NVIDIA
#endif

#ifdef NVIDIA
#undef GL_EXT_paletted_texture
#endif

int LoadCabinet(const char *fname);
void SwapBuffers(void);
void vector_vh_update(struct osd_bitmap *bitmap,int full_refresh);
void vector_clear_list(void);


void DeltaVec(GLfloat x1,GLfloat y1,GLfloat z1,
			  GLfloat x2,GLfloat y2,GLfloat z2,
			  GLfloat *dx,GLfloat *dy,GLfloat *dz);
void CalcFlatPoint(int x,int y,GLfloat *px,GLfloat *py);
void CalcCabPoint(int x,int y,GLfloat *px,GLfloat *py,GLfloat *pz);
void SetupFrustum(void);
void SetupOrtho(void);
void InitTextures(void);
void InitVScreen(void);
void CloseVScreen(void);
void UpdateTexture(struct osd_bitmap *bitmap);
void WAvg(GLfloat perc,GLfloat x1,GLfloat y1,GLfloat z1,
		  GLfloat x2,GLfloat y2,GLfloat z2,
		  GLfloat *ax,GLfloat *ay,GLfloat *az);
void UpdateCabDisplay(void);
void DrawFlatBitmap(void);
void UpdateFlatDisplay(void);
void UpdateGLDisplay(struct osd_bitmap *bitmap);

int cabspecified=0;

extern GLuint cablist;
extern int winwidth;
extern int winheight;
extern int doublebuffer;

static double scrnaspect,vscrnaspect;

GLubyte *ctable;
GLfloat *rcolmap, *gcolmap, *bcolmap;

static int palette_changed=0;
static int frame=0;

int bilinear=1; /* Do binlinear filtering? */
int drawbitmap=1;
int dopersist=0;
int screendirty=1;  /* Has the bitmap been modified since the last frame? */
int dodepth=1;

/* The squares that are tiled to make up the game screen polygon */

struct TexSquare
{
  GLubyte *texture;
  GLuint texobj;
  GLfloat x1,y1,z1,x2,y2,z2,x3,y3,z3,x4,y4,z4;
  GLfloat fx1,fy1,fx2,fy2,fx3,fy3,fx4,fy4;
  GLfloat xcov,ycov;
};

static struct TexSquare *texgrid=NULL;
static int texsize=256;
static int texnumx;
static int texnumy;
static GLfloat texpercx;
static GLfloat texpercy;
static GLfloat polysize=10.0;
static GLfloat vscrntlx;
static GLfloat vscrntly;
static GLfloat vscrnwidth;
static GLfloat vscrnheight;
static GLfloat xinc,yinc;

GLfloat cscrx1,cscry1,cscrz1,cscrx2,cscry2,cscrz2,
  cscrx3,cscry3,cscrz3,cscrx4,cscry4,cscrz4;
GLfloat cscrwdx,cscrwdy,cscrwdz;
GLfloat cscrhdx,cscrhdy,cscrhdz;

extern GLubyte *cabimg[5];
extern GLuint cabtex[5];

extern struct CameraPan *cpan;
extern int numpans;
int currentpan=0;
int lastpan=0;
int panframe=0;

/* Vector variables */

int vecgame=0;

extern int antialias;
extern GLuint veclist;
extern int inlist;

int sysdep_display_16bpp_capable(void)
{
   return 0;
}

/* Allocate a palette */
int sysdep_display_alloc_palette(int writable_colors)
{
  int col;

#ifdef GL_EXT_paletted_texture
  ctable=(GLubyte *)malloc(256*4);

  for(col=0;col<(256*4);col+=4) {
	ctable[col]=0;
	ctable[col+1]=0;
	ctable[col+2]=0;
	ctable[col+3]=255;
  }
#else
  rcolmap=(GLfloat *)malloc(256*sizeof(GLfloat));
  gcolmap=(GLfloat *)malloc(256*sizeof(GLfloat));
  bcolmap=(GLfloat *)malloc(256*sizeof(GLfloat));

  for(col=0;col<256;col++) {
	rcolmap[col]=0.0;
	gcolmap[col]=0.0;
	bcolmap[col]=0.0;
  }
#endif

  InitTextures();
  return 0;
}

/* Change the color of a pen */
int sysdep_display_set_pen(int pen, unsigned char red,unsigned char green,
					   unsigned char blue) 
{
  int cofs;

  palette_changed=1;

#ifdef GL_EXT_paletted_texture
  cofs=pen*4;

  ctable[cofs]=red; 
  ctable[cofs+1]=green;
  ctable[cofs+2]=blue;
#else
  rcolmap[pen]=(GLfloat)red/255.0;
  gcolmap[pen]=(GLfloat)green/255.0;
  bcolmap[pen]=(GLfloat)blue/255.0;
#endif
  return 0;
}

/* Compute a delta vector between two points */

void DeltaVec(GLfloat x1,GLfloat y1,GLfloat z1,
			  GLfloat x2,GLfloat y2,GLfloat z2,
			  GLfloat *dx,GLfloat *dy,GLfloat *dz)
{
  *dx=x2-x1;
  *dy=y2-y1;
  *dz=z2-z1;
}

/* Calculate points for flat screen */

void CalcFlatPoint(int x,int y,GLfloat *px,GLfloat *py)
{
  *px=vscrntlx+(float)x*texpercx*vscrnwidth;
  if(*px>vscrntlx+vscrnwidth) *px=vscrntlx+vscrnwidth;

  *py=vscrntly+vscrnheight-(float)y*texpercy*vscrnheight;
  if(*py<vscrntly) *py=vscrntly;
}

/* Calculate points for cabinet screen */

void CalcCabPoint(int x,int y,GLfloat *px,GLfloat *py,GLfloat *pz)
{
  GLfloat xperc,yperc;

  xperc=x*texpercx;
  if(xperc>1.0) xperc=1.0;

  yperc=y*texpercy;
  if(yperc>1.0) yperc=1.0;

  *px=cscrx1+xperc*cscrwdx+yperc*cscrhdx;
  *py=cscry1+xperc*cscrwdy+yperc*cscrhdy;
  *pz=cscrz1+xperc*cscrwdz+yperc*cscrhdz;
}


/* Set up a frustum projection */

void SetupFrustum(void)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-vscrnaspect,vscrnaspect,-1.0,1.0,5.0,100.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0,0.0,-20.0);
}


/* Set up an orthographic projection */

void SetupOrtho(void)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0,(GLfloat)winwidth,0.0,(GLfloat)winheight,-1.0,1.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void InitTextures(void)
{
  int x,y,point;
  GLuint *tobj;
  struct TexSquare *tsq;

  /* Allocate the texture memory */
  
  texnumx=visual_width/texsize;
  if(texnumx*texsize!=visual_width) texnumx++;
  texnumy=visual_height/texsize;
  if(texnumy*texsize!=visual_height) texnumy++;
  
  xinc=vscrnwidth*((double)texsize/(double)visual_width);
  yinc=vscrnheight*((double)texsize/(double)visual_height);
  
  /* printf("%d %d %d %d\n",visual_width,visual_height,texnumx,texnumy); */
  
  texpercx=(GLfloat)texsize/(GLfloat)visual_width;
  if(texpercx>1.0) texpercx=1.0;
  
  texpercy=(GLfloat)texsize/(GLfloat)visual_height;
  if(texpercy>1.0) texpercy=1.0;
  
  /*	printf("-- %f %f\n",texpercx,texpercy); */
  
  texgrid=(struct TexSquare *)
	malloc(texnumx*texnumy*sizeof(struct TexSquare));
  
  for(y=0;y<texnumy;y++) {
	for(x=0;x<texnumx;x++) {
	  tsq=texgrid+y*texnumx+x;
	  
	  if(x==texnumx-1 && visual_width%texsize)
		tsq->xcov=(GLfloat)(visual_width%texsize)/(GLfloat)texsize;
	  else tsq->xcov=1.0;
	  
	  if(y==texnumy-1 && visual_height%texsize)
		tsq->ycov=(GLfloat)(visual_height%texsize)/(GLfloat)texsize;
	  else tsq->ycov=1.0;
	  
	  CalcCabPoint(x,y,&(tsq->x1),&(tsq->y1),&(tsq->z1));
	  CalcCabPoint(x+1,y,&(tsq->x2),&(tsq->y2),&(tsq->z2));
	  CalcCabPoint(x+1,y+1,&(tsq->x3),&(tsq->y3),&(tsq->z3));
	  CalcCabPoint(x,y+1,&(tsq->x4),&(tsq->y4),&(tsq->z4));

	  CalcFlatPoint(x,y,&(tsq->fx1),&(tsq->fy1));
	  CalcFlatPoint(x+1,y,&(tsq->fx2),&(tsq->fy2));
	  CalcFlatPoint(x+1,y+1,&(tsq->fx3),&(tsq->fy3));
	  CalcFlatPoint(x,y+1,&(tsq->fx4),&(tsq->fy4));
	  
	  tsq->texture=malloc(texsize*texsize);

	  /* Initialize the texture memory */

	  for(point=0;point<(texsize*texsize);point++)
		tsq->texture[point]=0;
	  
	  tobj=&(tsq->texobj);
	  
	  glGenTextures(1,tobj);
	  
	  *tobj=glGenLists(1);
	  glBindTexture(GL_TEXTURE_2D,*tobj);
	  
#ifdef GL_EXT_paletted_texture
	  printf("Paletted textures not supported, using (slow) workaround\n");
	  printf("Ask your OpenGL supplier to implement glColorTableEXT\n");

	  glColorTableEXT(GL_TEXTURE_2D,
					  GL_RGBA,
					  256,
					  GL_RGBA,
					  GL_UNSIGNED_BYTE,
					  ctable);
	  
	  glTexImage2D(GL_TEXTURE_2D,0,GL_COLOR_INDEX8_EXT,texsize,texsize,0,
				   GL_COLOR_INDEX,GL_UNSIGNED_BYTE,NULL);
#else
#ifdef NVIDIA
	  glTexImage2D(GL_TEXTURE_2D,0,3,texsize,texsize,0,
				   GL_COLOR_INDEX,GL_UNSIGNED_BYTE,tobj);
#else
	  glPixelMapfv(GL_PIXEL_MAP_I_TO_R,256,rcolmap);
 	  glPixelMapfv(GL_PIXEL_MAP_I_TO_G,256,gcolmap);
	  glPixelMapfv(GL_PIXEL_MAP_I_TO_B,256,bcolmap);

	  glTexImage2D(GL_TEXTURE_2D,0,3,texsize,texsize,0,
				   GL_COLOR_INDEX,GL_UNSIGNED_BYTE,NULL);
#endif
#endif
	  
	  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	  
	  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	  
	  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	}
  }
}

/* Set up the virtual screen */

void InitVScreen(void)
{
  /* clear the buffer */

  glClearColor(0,0,0,1);
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();

  if(!dodepth) cabview=0;

  scrnaspect=(double)visual_width/(double)visual_height;
  vscrnaspect=(double)winwidth/(double)winheight;

  glViewport(0,0,winwidth,winheight);

  if(cabview)
    SetupFrustum();
  else SetupOrtho();

  glShadeModel(GL_SMOOTH);

  glEnable(GL_POLYGON_SMOOTH);
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  if(dodepth) glDepthFunc(GL_LEQUAL);

  if(scrnaspect<vscrnaspect) {
	vscrnheight=(GLfloat)winheight;
	vscrnwidth=vscrnheight*scrnaspect;
	vscrntlx=((GLfloat)winwidth-vscrnwidth)/2.0;
	vscrntly=0.0;
  }
  else {
	vscrnwidth=(GLfloat)winwidth;
	vscrnheight=vscrnwidth/scrnaspect;
	vscrntlx=0.0;
	vscrntly=((GLfloat)winheight-vscrnheight)/2.0;
  }

  if(cabspecified||!LoadCabinet(drivers[game_index]->name)) {
	if(!LoadCabinet(cabname)) {
	  printf("Unable to load cabinet %s\n",cabname);
	}
  }

  /* Calulate delta vectors for screen height and width */

  DeltaVec(cscrx1,cscry1,cscrz1,cscrx2,cscry2,cscrz2,
		   &cscrwdx,&cscrwdy,&cscrwdz);
	
  DeltaVec(cscrx1,cscry1,cscrz1,cscrx4,cscry4,cscrz4,
		   &cscrhdx,&cscrhdy,&cscrhdz);
	

  if(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	vecgame=1;

  /* fill the display_palette_info struct */
  memset(&display_palette_info, 0, sizeof(struct sysdep_palette_info));
  display_palette_info.depth = 8;
  display_palette_info.writable_colors = 256;
}

/* Close down the virtual screen */

void CloseVScreen(void)
{
  int x,y;

#ifdef GL_EXT_paletted_texture
  free(ctable);
#else
  free(rcolmap);
  free(bcolmap);
  free(gcolmap);
#endif

  if(cpan) free(cpan);

  /* Free Texture stuff */

  if(texgrid) {
	for(y=0;y<texnumy;y++) {
	  for(x=0;x<texnumx;x++) {
		free(texgrid[y*texnumx+x].texture);
	  }
	}
	
	free(texgrid);
  }
}

/* Not needed under GL */

void sysdep_clear_screen(void)
{
}


/* Update the texture with the contents of the game screen */

void UpdateTexture(struct osd_bitmap *bitmap)
{
  unsigned line,rline,texline,xsquare,ysquare,ofs,width;

  if(visual_width<=texsize) width=visual_width;
  else width=texsize;

  for(line=visual.min_y;line<=visual.max_y;line++) {
	rline=line-visual.min_y;
	ysquare=rline/texsize;
	texline=rline%texsize;
	  
	for(xsquare=0;xsquare<texnumx;xsquare++) {
	  ofs=xsquare*texsize;
		
	  if(xsquare<(texnumx-1) || !(visual_width%texsize))
		width=texsize;
	  else width=visual_width%texsize;
	  
	  memcpy(texgrid[ysquare*texnumx+xsquare].texture+texline*texsize,
			 bitmap->line[line]+visual.min_x+ofs,width);
	}
  }
}

/* Compute an average between two sets of 3D coordinates */

void WAvg(GLfloat perc,GLfloat x1,GLfloat y1,GLfloat z1,
		  GLfloat x2,GLfloat y2,GLfloat z2,
		  GLfloat *ax,GLfloat *ay,GLfloat *az)
{
  *ax=(1.0-perc)*x1+perc*x2;
  *ay=(1.0-perc)*y1+perc*y2;
  *az=(1.0-perc)*z1+perc*z2;
}

/* Draw a frame in Cabinet mode */

void UpdateCabDisplay(void)
{
  struct TexSquare *square;
  int x,y;
  GLfloat camx,camy,camz;
  GLfloat dirx,diry,dirz;
  GLfloat normx,normy,normz;
  GLfloat perc;
  struct CameraPan *pan,*lpan;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushMatrix();


  /* Do the camera panning */

  if(cpan) {
	pan=cpan+currentpan;

	if(panframe>pan->frames) {
	  lastpan=currentpan;
	  currentpan=(currentpan+1)%numpans;
	  panframe=0;
	}

	switch(pan->type) {
	case pan_goto:
	  camx=pan->lx; camy=pan->ly; camz=pan->lz;
	  dirx=pan->px; diry=pan->px; dirz=pan->pz;
	  normx=pan->nx; normy=pan->ny; normz=pan->nz;
	  break;
	case pan_moveto:
	  lpan=cpan+lastpan;
	  perc=(GLfloat)panframe/(GLfloat)pan->frames;
	  WAvg(perc,lpan->lx,lpan->ly,lpan->lz,
		   pan->lx,pan->ly,pan->lz,&camx,&camy,&camz);
	  WAvg(perc,lpan->px,lpan->py,lpan->pz,
		   pan->px,pan->py,pan->pz,&dirx,&diry,&dirz);
	  WAvg(perc,lpan->nx,lpan->ny,lpan->nz,
		   pan->nx,pan->ny,pan->nz,&normx,&normy,&normz);
	  break;
	default:
	  break;
	}

	gluLookAt(camx,camy,camz,
			  dirx,diry,dirz,
			  normx,normy,normz);
	
	panframe++;
  }
  else gluLookAt(-5.0,0.0,5.0,0.0,0.0,-5.0,0.0,1.0,0.0);

  glEnable(GL_DEPTH_TEST);

  /* Draw the cabinet */

  glCallList(cablist);

  /* Draw the screen if in vector mode */

  if(vecgame) {
	glDisable(GL_TEXTURE_2D);
	glShadeModel(GL_FLAT);

	if(antialias) {
	  glEnable(GL_LINE_SMOOTH);
	  glEnable(GL_POINT_SMOOTH);
	}

	glCallList(veclist);

	if(antialias) {
	  glDisable(GL_LINE_SMOOTH);
	  glDisable(GL_POINT_SMOOTH);
	}

	glShadeModel(GL_SMOOTH);

	if(drawbitmap&&screendirty) {
	  glClear(GL_DEPTH_BUFFER_BIT);

	  SetupOrtho();
	  
	  glColor4f(1.0,1.0,1.0,0.7);
	  
	  DrawFlatBitmap();
	  
	  SetupFrustum();
	}
  }
  else {  /* Draw the screen of a bitmapped game */
	glEnable(GL_TEXTURE_2D);
 
	for(y=0;y<texnumy;y++) {
	  for(x=0;x<texnumx;x++) {
		square=texgrid+y*texnumx+x;
		
		glBindTexture(GL_TEXTURE_2D,square->texobj);
		
		/* If the palette was changed, update the color table */
		
		if(palette_changed) {
#ifdef GL_EXT_paletted_texture
		  glColorTableEXT(GL_TEXTURE_2D,
						  GL_RGBA,
						  256,
						  GL_RGBA,
						  GL_UNSIGNED_BYTE,
						  ctable);
#else
		  glPixelMapfv(GL_PIXEL_MAP_I_TO_R,256,rcolmap);
		  glPixelMapfv(GL_PIXEL_MAP_I_TO_G,256,gcolmap);
		  glPixelMapfv(GL_PIXEL_MAP_I_TO_B,256,bcolmap);
#endif	
		}
	
		/* This is the quickest way I know of to update the texture */
		
		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,texsize,texsize,
						GL_COLOR_INDEX,GL_UNSIGNED_BYTE,square->texture);
		
		glBegin(GL_QUADS);
		glTexCoord2f(0,0); glVertex3f(square->x1,square->y1,square->z1);
		glTexCoord2f(square->xcov,0);
		glVertex3f(square->x2,square->y2,square->z2);
		glTexCoord2f(square->xcov,square->ycov);
		glVertex3f(square->x3,square->y3,square->z3);
		glTexCoord2f(0,square->ycov);
		glVertex3f(square->x4,square->y4,square->z4);
		glEnd();
	  }
	}
  }

  glDisable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);

  glPopMatrix();

  if(doublebuffer) SwapBuffers();
  else glFlush();

  palette_changed=0;
}

void DrawFlatBitmap(void)
{
  struct TexSquare *square;
  int x,y;

  glEnable(GL_TEXTURE_2D);

  for(y=0;y<texnumy;y++) {
	for(x=0;x<texnumx;x++) {
	  square=texgrid+y*texnumx+x;
	  
	  glBindTexture(GL_TEXTURE_2D,square->texobj);
	  
	  /* If the palette was changed, update the color table */
	  
	  if(palette_changed) {
#ifdef GL_EXT_paletted_texture
		glColorTableEXT(GL_TEXTURE_2D,
						GL_RGBA,
						256,
						GL_RGBA,
						GL_UNSIGNED_BYTE,
						ctable);
#else
		  glPixelMapfv(GL_PIXEL_MAP_I_TO_R,256,rcolmap);
		  glPixelMapfv(GL_PIXEL_MAP_I_TO_G,256,gcolmap);
		  glPixelMapfv(GL_PIXEL_MAP_I_TO_B,256,bcolmap);
#endif	
						}

	  glTexSubImage2D(GL_TEXTURE_2D,0,0,0,texsize,texsize,
					  GL_COLOR_INDEX,GL_UNSIGNED_BYTE,square->texture);
	  
	  glBegin(GL_QUADS);
	  glTexCoord2f(0,0); glVertex2f(square->fx1,square->fy1);
	  glTexCoord2f(square->xcov,0);
	  glVertex2f(square->fx2,square->fy2);
	  glTexCoord2f(square->xcov,square->ycov);
	  glVertex2f(square->fx3,square->fy3);
	  glTexCoord2f(0,square->ycov);
	  glVertex2f(square->fx4,square->fy4);
	  glEnd();
	}
  }

  glDisable(GL_TEXTURE_2D);
}

void UpdateFlatDisplay(void)
{
  if(!vecgame||!dopersist) glClear(GL_COLOR_BUFFER_BIT);

  glPushMatrix();

  if(dopersist&&vecgame) {
	glColor4f(0.0,0.0,0.0,0.2);

	glBegin(GL_QUADS);
	glVertex2f(0.0,0.0);
	glVertex2f((GLfloat)winwidth,0.0);
	glVertex2f((GLfloat)winwidth,(GLfloat)winheight);
	glVertex2f(0.0,(GLfloat)winheight);
  }

  if(vecgame) {
	glShadeModel(GL_FLAT);

	if(antialias) {
	  glEnable(GL_LINE_SMOOTH);
	  glEnable(GL_POINT_SMOOTH);
	}

	glCallList(veclist);

	if(antialias) {
	  glDisable(GL_LINE_SMOOTH);
	  glDisable(GL_POINT_SMOOTH);
	}

	glShadeModel(GL_SMOOTH);
  }

  if(!vecgame||(drawbitmap&&screendirty)) {
	if(vecgame) glColor4f(1.0,1.0,1.0,0.7);
	else glColor3f(1.0,1.0,1.0);

	DrawFlatBitmap();
  }

  glPopMatrix();
  SwapBuffers();

  palette_changed=0;
}

void UpdateGLDisplay(struct osd_bitmap *bitmap)
{
  if(bitmap && (!vecgame||drawbitmap)) UpdateTexture(bitmap);

  if(vecgame&&inlist)
	vector_vh_update(NULL,0);

  if(cabview) UpdateCabDisplay();
  else UpdateFlatDisplay();

  if(vecgame&&inlist)
	vector_clear_list();

  screendirty=0;
}

/* used when expose events received */

void osd_refresh_screen(void)
{
  /* Just re-draw the whole screen */
  UpdateGLDisplay(NULL);
}


/* invoked by main tree code to update bitmap into screen */

void sysdep_update_display(struct osd_bitmap *bitmap)
{
  int x,y;

  UpdateGLDisplay(bitmap);

  frame++;

  if(keyboard_pressed(KEYCODE_RCONTROL)) {
#if 0
	if(keyboard_pressed_memory(KEYCODE_A))
	  antialias=1-antialias;
#endif
	if(keyboard_pressed_memory(KEYCODE_C)&&dodepth)
	{
	  cabview=1-cabview;

	  if(cabview)
		SetupFrustum();
	  else SetupOrtho();
	}
	if(keyboard_pressed_memory(KEYCODE_B))
	{
	  bilinear=1-bilinear;

	  if(bilinear) {
		glBindTexture(GL_TEXTURE_2D,cabtex[0]);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

		for(y=0;y<texnumy;y++) {
		  for(x=0;x<texnumx;x++) {
			glBindTexture(GL_TEXTURE_2D,texgrid[y*texnumx+x].texobj);
			
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		  }
		}
	  }
	  else {
		glBindTexture(GL_TEXTURE_2D,cabtex[0]);

		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

		for(y=0;y<texnumy;y++) {
		  for(x=0;x<texnumx;x++) {
			glBindTexture(GL_TEXTURE_2D,texgrid[y*texnumx+x].texobj);
			
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		  }
		}
	  }

	}
#if 0
	if(keyboard_pressed_memory(KEYCODE_O))
	  drawbitmap=1-drawbitmap;
#endif
	if(keyboard_pressed_memory(KEYCODE_T))
	  dopersist=1-dopersist;
  }
}

void osd_mark_dirty(int x1, int y1, int x2, int y2)
{
  screendirty=1;
}


#endif /* ifdef xgl */
