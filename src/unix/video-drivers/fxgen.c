/*****************************************************************

  Generic glide routines

  Copyright 1998 by Mike Oliphant - oliphant@ling.ed.ac.uk
  Copyright 2004 Hans de Goede - j.w.r.degoede@hhs.nl

    http://www.ling.ed.ac.uk/~oliphant/glmame

  This code may be used and distributed under the terms of the
  Mame license
  
*****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "osd_cpu.h"
#include "fxcompat.h"
#include "sysdep/sysdep_display_priv.h"
#include "fxgen.h"

/* defined in svgafx.c or x11.c */
extern int window_width, window_height, custom_window_width, custom_window_height;

/* from fxvec.c */
int fxvec_renderer(point *pt, int num_points);

/* The squares that are tiled to make up the game screen polygon */
struct TexSquare
{
  UINT16 *texture;
  unsigned int texobj;
  long texadd;
  GrVertex vtxA, vtxB, vtxC, vtxD;
  float xcov,ycov;
};

int vscrntlx;
int vscrntly;
int vscrnwidth;
int vscrnheight;
int vecvscrntlx;
int vecvscrntly;
int vecvscrnwidth;
int vecvscrnheight;

static char version[80];
static GrTexInfo texinfo;
static int bilinear=1; /* Do binlinear filtering? */
static const int texsize=256;
static GrContext_t context=0;
static int bitmap_dirty;
static struct TexSquare *texgrid=NULL;
static int texnumx;
static int texnumy;
static int texdestwidth;
static int texdestheight;
static int firsttexdestwidth;
static int firsttexdestheight;
static struct sigaction orig_sigaction[32];
static struct sigaction vscreen_sa;  
static int signals_to_catch[] = { SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGABRT,
   SIGFPE, SIGKILL, SIGSEGV, SIGPIPE, SIGTERM, SIGBUS, -1 };
static int signals_caught = 0;
static GrScreenResolution_t Gr_resolution;

static int  InitTextures(void);
static void UpdateTexture(struct mame_bitmap *bitmap,
	  struct rectangle *dirty_area,  struct rectangle *vis_area,
	  struct sysdep_palette_struct *palette);
static void DrawFlatBitmap(void);


static void CalcPoint(GrVertex *vert,int x,int y)
{
  if(x)
  {
    vert->x=vscrntlx+firsttexdestwidth+(x-1)*texdestwidth;
    if(vert->x>vscrntlx+vscrnwidth) vert->x=vscrntlx+vscrnwidth;
  }
  else
    vert->x = vscrntlx;

  if(y)
  {
    vert->y=vscrntly+vscrnheight-(firsttexdestheight+(y-1)*texdestheight);
    if(vert->y<vscrntly) vert->y=vscrntly;
  }
  else
    vert->y = vscrntly+vscrnheight;
}

int InitGlide(void)
{
  int fd = open("/dev/3dfx", O_RDWR);
  if ((fd < 0) && geteuid())
  {
    fprintf(stderr, "Glide error: couldn't open /dev/3dfx and not running as root, disabling use of Glide");
    return 1;
  }
  if (fd >= 0)
    close(fd);
  putenv("FX_GLIDE_NO_SPLASH=");
  grGlideInit();
  grSetupVertexLayout();
  return 0;
}

void ExitGlide(void)
{
  grGlideShutdown();
}

static void VScreenSignalHandler(int signo)
{
  grEnablePassThru();
  orig_sigaction[signo].sa_handler(signo);
}

void VScreenCatchSignals(void)
{
  int i;
  
  /* catch fatal signals and restore the vgapassthru before exiting */
  memset(&vscreen_sa, 0, sizeof(vscreen_sa));
  vscreen_sa.sa_handler = VScreenSignalHandler;
  for (i=0; signals_to_catch[i] != -1; i++)
  {
     sigaction(signals_to_catch[i], &vscreen_sa, &(orig_sigaction[signals_to_catch[i]]));
  }
  signals_caught = 1;
}

void VScreenRestoreSignals(void)
{
  int i;
  
  if (!signals_caught)
     return;
  
  /* restore signal handlers */
  for (i=0; signals_to_catch[i] != -1; i++)
  {
     sigaction(signals_to_catch[i], &(orig_sigaction[signals_to_catch[i]]), NULL);
  }
  signals_caught = 0;
}

int InitTextures(void)
{
  int i,j,x=0,y=0;
  struct TexSquare *tsq;
  long texmem,memaddr;
  float firsttexdestwidthfac=0.0, firsttexdestheightfac=0.0;
  float texpercx, texpercy;

  texinfo.smallLod=texinfo.largeLod=GR_LOD_256;
  texinfo.aspectRatio=GR_ASPECT_1x1;
  texinfo.format=GR_TEXFMT_ARGB_1555;

  texmem=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&texinfo);

  if(sysdep_display_params.vec_src_bounds)
  {
    grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,
                                       GR_COMBINE_FACTOR_LOCAL,
                                       GR_COMBINE_LOCAL_CONSTANT,
                                       GR_COMBINE_OTHER_NONE,
                                       FXFALSE);

    grAlphaBlendFunction(GR_BLEND_ALPHA_SATURATE,GR_BLEND_ONE,
						 GR_BLEND_ALPHA_SATURATE,GR_BLEND_ONE);
  }                                          
	
  grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
				   GR_COMBINE_FACTOR_ONE,
				   GR_COMBINE_LOCAL_NONE,
				   GR_COMBINE_OTHER_TEXTURE,
				   FXFALSE);

  grTexCombine(GR_TMU0,
			   GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
			   GR_COMBINE_FUNCTION_NONE,GR_COMBINE_FACTOR_NONE,
			   FXFALSE, FXFALSE);

  grTexMipMapMode(GR_TMU0,
				  GR_MIPMAP_DISABLE,
				  FXFALSE);

  grTexClampMode(GR_TMU0,
				 GR_TEXTURECLAMP_CLAMP,
				 GR_TEXTURECLAMP_CLAMP);

  if(bilinear)
	grTexFilterMode(GR_TMU0,
					GR_TEXTUREFILTER_BILINEAR,
					GR_TEXTUREFILTER_BILINEAR);
  else
	grTexFilterMode(GR_TMU0,
					GR_TEXTUREFILTER_POINT_SAMPLED,
					GR_TEXTUREFILTER_POINT_SAMPLED);

  /* Allocate the texture memory */
  
  texnumx=sysdep_display_params.orig_width/texsize;
  if(texnumx*texsize!=sysdep_display_params.orig_width) texnumx++;
  texnumy=sysdep_display_params.orig_height/texsize;
  if(texnumy*texsize!=sysdep_display_params.orig_height) texnumy++;
  
  if ((sysdep_display_params.orientation&SYSDEP_DISPLAY_SWAPXY))
  {
    texpercx=(float)texsize/(float)sysdep_display_params.orig_height;
    texpercy=(float)texsize/(float)sysdep_display_params.orig_width;
  }
  else
  {
    texpercx=(float)texsize/(float)sysdep_display_params.orig_width;
    texpercy=(float)texsize/(float)sysdep_display_params.orig_height;
  }

  if(texpercx>1.0) texpercx=1.0;
  if(texpercy>1.0) texpercy=1.0;

  texdestwidth=vscrnwidth*texpercx;
  texdestheight=vscrnheight*texpercy;

  texgrid=(struct TexSquare *)
	calloc(texnumx*texnumy, sizeof(struct TexSquare));
  memaddr=grTexMinAddress(GR_TMU0);
  
  for(i=0;i<texnumy;i++) {
	for(j=0;j<texnumx;j++) {
	  tsq=texgrid+i*texnumx+j;

	  tsq->texadd=memaddr;
	  memaddr+=texmem;

	  if(j==(texnumx-1) && sysdep_display_params.orig_width%texsize)
		tsq->xcov=(float)((sysdep_display_params.orig_width)%texsize)/(float)texsize;
	  else tsq->xcov=1.0;
	  
	  if(i==(texnumy-1) && sysdep_display_params.orig_height%texsize)
		tsq->ycov=(float)((sysdep_display_params.orig_height)%texsize)/(float)texsize;
	  else tsq->ycov=1.0;

	  tsq->vtxA.oow=1.0;
	  tsq->vtxB=tsq->vtxC=tsq->vtxD=tsq->vtxA;

          if((sysdep_display_params.orientation&SYSDEP_DISPLAY_FLIPY))
          {
            tsq->vtxA.tmuvtx[0].tow=256.0;
            tsq->vtxB.tmuvtx[0].tow=256.0;
            tsq->vtxC.tmuvtx[0].tow=0.0;
            tsq->vtxD.tmuvtx[0].tow=0.0;
            if ((sysdep_display_params.orientation&SYSDEP_DISPLAY_SWAPXY))
            {
              y = (texnumx-1) - j;
              firsttexdestheightfac = (sysdep_display_params.orig_width%texsize)/(float)texsize;
            }
            else
            {
              y = (texnumy-1) - i;
              firsttexdestheightfac = (sysdep_display_params.orig_height%texsize)/(float)texsize;
            }
          }
          else
          {
            tsq->vtxA.tmuvtx[0].tow=0.0;
            tsq->vtxB.tmuvtx[0].tow=0.0;
            tsq->vtxC.tmuvtx[0].tow=256.0;
            tsq->vtxD.tmuvtx[0].tow=256.0;
            if ((sysdep_display_params.orientation&SYSDEP_DISPLAY_SWAPXY))
            {
              y = j;
              firsttexdestheightfac = 1.0;
            }
            else
            {
              y = i;
              firsttexdestheightfac = 1.0;
            }
          }

          if((sysdep_display_params.orientation&SYSDEP_DISPLAY_FLIPX))
          {
            tsq->vtxA.tmuvtx[0].sow=256.0;
            tsq->vtxB.tmuvtx[0].sow=0.0;
            tsq->vtxC.tmuvtx[0].sow=0.0;
            tsq->vtxD.tmuvtx[0].sow=256.0;
            if ((sysdep_display_params.orientation&SYSDEP_DISPLAY_SWAPXY))
            {
              x = (texnumy-1) - i;
              firsttexdestwidthfac = (sysdep_display_params.orig_height%texsize)/(float)texsize;
            }
            else
            {
              x = (texnumx-1) - j;
              firsttexdestwidthfac = (sysdep_display_params.orig_width%texsize)/(float)texsize;
            }
          }
          else
          {
            tsq->vtxA.tmuvtx[0].sow=0.0;
            tsq->vtxB.tmuvtx[0].sow=256.0;
            tsq->vtxC.tmuvtx[0].sow=256.0;
            tsq->vtxD.tmuvtx[0].sow=0.0;
            if ((sysdep_display_params.orientation&SYSDEP_DISPLAY_SWAPXY))
            {
              x = i;
              firsttexdestwidthfac = 1.0;
            }
            else
            {
              x = j;
              firsttexdestwidthfac = 1.0;
            }
          }
          
          if((sysdep_display_params.orientation&SYSDEP_DISPLAY_SWAPXY))
          {
            float temp;
            
            temp=tsq->vtxA.tmuvtx[0].sow;
            tsq->vtxA.tmuvtx[0].sow=tsq->vtxA.tmuvtx[0].tow;
            tsq->vtxA.tmuvtx[0].tow=temp;

            temp=tsq->vtxB.tmuvtx[0].sow;
            tsq->vtxB.tmuvtx[0].sow=tsq->vtxB.tmuvtx[0].tow;
            tsq->vtxB.tmuvtx[0].tow=temp;

            temp=tsq->vtxC.tmuvtx[0].sow;
            tsq->vtxC.tmuvtx[0].sow=tsq->vtxC.tmuvtx[0].tow;
            tsq->vtxC.tmuvtx[0].tow=temp;

            temp=tsq->vtxD.tmuvtx[0].sow;
            tsq->vtxD.tmuvtx[0].sow=tsq->vtxD.tmuvtx[0].tow;
            tsq->vtxD.tmuvtx[0].tow=temp;
          }

          tsq->vtxA.tmuvtx[0].tow*=tsq->ycov;
          tsq->vtxB.tmuvtx[0].tow*=tsq->ycov;
          tsq->vtxC.tmuvtx[0].tow*=tsq->ycov;
          tsq->vtxD.tmuvtx[0].tow*=tsq->ycov;

          tsq->vtxA.tmuvtx[0].sow*=tsq->xcov;
          tsq->vtxB.tmuvtx[0].sow*=tsq->xcov;
          tsq->vtxC.tmuvtx[0].sow*=tsq->xcov;
          tsq->vtxD.tmuvtx[0].sow*=tsq->xcov;

          firsttexdestwidth=texdestwidth*firsttexdestwidthfac;
          firsttexdestheight=texdestheight*firsttexdestheightfac;

          CalcPoint(&(tsq->vtxA), x  , y  );
          CalcPoint(&(tsq->vtxB), x+1, y  );
          CalcPoint(&(tsq->vtxC), x+1, y+1);
          CalcPoint(&(tsq->vtxD), x  , y+1);
	
          /* Initialize the texture memory */
          if (!(tsq->texture=calloc(texsize*texsize, sizeof *(tsq->texture))))
             return 1;
        }
  }
  return 0;
}

typedef struct {
    int         res;
    int       width;
    int       height;
} ResToRes;
		
static ResToRes resTable[] = {
    { GR_RESOLUTION_320x200,   320,  200 },  /* 0x0 */
    { GR_RESOLUTION_320x240,   320,  240 },  /* 0x1 */
    { GR_RESOLUTION_400x256,   400,  256 },  /* 0x2 */
    { GR_RESOLUTION_512x384,   512,  384 },  /* 0x3 */
    { GR_RESOLUTION_640x200,   640,  200 },  /* 0x4 */
    { GR_RESOLUTION_640x350,   640,  350 },  /* 0x5 */
    { GR_RESOLUTION_640x400,   640,  400 },  /* 0x6 */
    { GR_RESOLUTION_640x480,   640,  480 },  /* 0x7 */
    { GR_RESOLUTION_800x600,   800,  600 },  /* 0x8 */
    { GR_RESOLUTION_960x720,   960,  720 },  /* 0x9 */
    { GR_RESOLUTION_856x480,   856,  480 },  /* 0xA */
    { GR_RESOLUTION_512x256,   512,  256 },  /* 0xB */
    { GR_RESOLUTION_1024x768,  1024, 768 },  /* 0xC */
    { GR_RESOLUTION_1280x1024, 1280, 1024 }, /* 0xD */
    { GR_RESOLUTION_1600x1200, 1600, 1200 }, /* 0xE */
    { GR_RESOLUTION_400x300,   400,  300 }   /* 0xF */
};
			
static long resTableSize = sizeof( resTable ) / sizeof( ResToRes );

int InitParams(void)
{
  int i;
  
  if(custom_window_width)
  {
    for( i = 0; i < resTableSize; i++ )
    {
      if(custom_window_width==resTable[i].width && custom_window_height==resTable[i].height)
      {
         Gr_resolution = resTable[i].res;
         break;
      }
    }
    if(i == resTableSize)
    {
      fprintf(stderr,
          "error: unknown resolution: \"%dx%d\".\n"
          "   Valid resolutions are:\n", window_width, window_height);
      for( i = 0; i < resTableSize; i++ )
      {
         fprintf(stderr, "   \"%dx%d\"", resTable[i].width,
            resTable[i].height);
         if (i && (i % 5) == 0)
            fprintf(stderr, "\n");
      }
      return 1;
    } 
  }
  else
    Gr_resolution = GR_RESOLUTION_640x480;

  window_width  = resTable[Gr_resolution].width;
  window_height = resTable[Gr_resolution].height;

  return 0;
}

/* Set up the virtual screen */
int InitVScreen(void)
{
  grGlideGetVersion(version);
  fprintf(stderr, "info: using Glide version %s\n", version);

  mode_set_aspect_ratio((double)window_width/window_height);
  
  grSstSelect(0);
  if(!(context = grSstWinOpen(0,Gr_resolution,GR_REFRESH_60Hz,GR_COLORFORMAT_ABGR,
     GR_ORIGIN_LOWER_LEFT,2,1)))
  {
     fprintf(stderr, "error opening Glide window, do you have enough memory on your 3dfx for the selected mode?\n");
     return 1;
  }
  fprintf(stderr,
     "info: screen resolution set to %dx%d\n", window_width, window_height);

  /* clear the buffer */
  grBufferClear(0,0,0);
  
  /* calculate the vscreen boundaries */
  mode_clip_aspect(window_width, window_height, &vscrnwidth, &vscrnheight);
  vscrntlx=(window_width -vscrnwidth )/2;
  vscrntly=(window_height-vscrnheight)/2;
  if(sysdep_display_params.vec_dest_bounds)
  {
    struct rectangle vec_bounds = *(sysdep_display_params.vec_dest_bounds);
    sysdep_display_orient_bounds(&vec_bounds, sysdep_display_params.orig_width,
      sysdep_display_params.orig_height);
    vecvscrnwidth  = ((vec_bounds.max_x+1)-vec_bounds.min_x) *
      ((double)vscrnwidth/sysdep_display_params.width);
    vecvscrnheight = ((vec_bounds.max_y+1)-vec_bounds.min_y) *
      ((double)vscrnheight/sysdep_display_params.height);
    vecvscrntlx = vscrntlx + ((double)vscrnwidth/sysdep_display_params.width)
      * vec_bounds.min_x;
    vecvscrntly = vscrntly + ((double)vscrnheight/sysdep_display_params.height)
      * vec_bounds.min_y;
    fprintf(stderr, "vec: %dx%d, %dx%d\n", vecvscrntlx, vecvscrntly,
      vecvscrnwidth, vecvscrnheight);
  }
  else
  {
    vecvscrnwidth  = vscrnwidth;
    vecvscrnheight = vscrnheight;
    vecvscrntlx    = vscrntlx;
    vecvscrntly    = vscrntly;
  }
  
  /* fill the sysdep_display_properties struct */
  memset(&sysdep_display_properties, 0, sizeof(sysdep_display_properties));
  switch(sysdep_display_params.depth) {
    case 15:
    case 16:
      sysdep_display_properties.palette_info.red_mask   = 0x7C00;
      sysdep_display_properties.palette_info.green_mask = 0x03E0;
      sysdep_display_properties.palette_info.blue_mask  = 0x001F;
      break;
    case 32:
      sysdep_display_properties.palette_info.red_mask   = 0xFF0000;
      sysdep_display_properties.palette_info.green_mask = 0x00FF00;
      sysdep_display_properties.palette_info.blue_mask  = 0x0000FF;
      break;
  }
  sysdep_display_properties.vector_renderer = fxvec_renderer;
  
  /* force an update of the bitmap for the first frame */
  bitmap_dirty = 1;
   
  return InitTextures();
}

/* Close down the virtual screen */
void CloseVScreen(void)
{
  int x,y;

  /* Free Texture stuff */
  if(texgrid) {
	for(y=0;y<texnumy;y++) {
	  for(x=0;x<texnumx;x++) {
            if (texgrid[y*texnumx+x].texture)
	      free(texgrid[y*texnumx+x].texture);
	  }
	}
	
	free(texgrid);
	texgrid = NULL;
  }

  if (context)
  {
    grSstWinClose(context);
    context = 0;
  }
}

/* Update the texture with the contents of the game screen */
/* FIXME: do partial updates */
void UpdateTexture(struct mame_bitmap *bitmap,
	  struct rectangle *vis_area,  struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette)
{
	int y,rline,texline,xsquare,ysquare,ofs,width, i;
	struct TexSquare *square;

	if(sysdep_display_params.orig_width<=texsize) width=sysdep_display_params.orig_width;
	else width=texsize;

	switch (sysdep_display_params.depth) {

	case 15:
		for(y=vis_area->min_y;y<=vis_area->max_y;y++) {
			rline=y-vis_area->min_y;
			ysquare=rline/texsize;
			texline=rline%texsize;
			
			for(xsquare=0;xsquare<texnumx;xsquare++) {
				ofs=xsquare*texsize;
				
				if(xsquare<(texnumx-1) || !(sysdep_display_params.orig_width%texsize))
					width=texsize;
				else width=sysdep_display_params.orig_width%texsize;
				
				square=texgrid+(ysquare*texnumx)+xsquare;

                                memcpy(square->texture+texline*texsize,
                                       (UINT16*)(bitmap->line[y])+vis_area->min_x+ofs,
                                       width*2);
			}
		} 
		break;

	case 16:
		for(y=vis_area->min_y;y<=vis_area->max_y;y++) {
			rline=y-vis_area->min_y;
			ysquare=rline/texsize;
			texline=rline%texsize;
			
			for(xsquare=0;xsquare<texnumx;xsquare++) {
				ofs=xsquare*texsize;
				
				if(xsquare<(texnumx-1) || !(sysdep_display_params.orig_width%texsize))
					width=texsize;
				else width=sysdep_display_params.orig_width%texsize;
				
				square=texgrid+(ysquare*texnumx)+xsquare;
				for(i = 0;i < width;i++) {
					square->texture[texline*texsize+i] = 
						palette->lookup[(((UINT16*)(bitmap->line[y]))[vis_area->min_x+ofs+i])];
				}
			}
		}
		break;

	case 32:
		for(y=vis_area->min_y;y<=vis_area->max_y;y++) {
			rline=y-vis_area->min_y;
			ysquare=rline/texsize;
			texline=rline%texsize;
			
			for(xsquare=0;xsquare<texnumx;xsquare++) {
				ofs=xsquare*texsize;
				
				if(xsquare<(texnumx-1) || !(sysdep_display_params.orig_width%texsize))
					width=texsize;
				else width=sysdep_display_params.orig_width%texsize;
				
				square=texgrid+(ysquare*texnumx)+xsquare;
					
				for(i = 0;i < width;i++) {
					square->texture[texline*texsize+i] = 
						_32TO16_RGB_555((((UINT32*)(bitmap->line[y]))[vis_area->min_x+ofs+i]));
				}
			}
		}
		break;
	}

        for(ysquare=0;ysquare<texnumy;ysquare++) {
              for(xsquare=0;xsquare<texnumx;xsquare++) {
		square=texgrid+(ysquare*texnumx)+xsquare;
                texinfo.data=(void *)square->texture;
                grTexDownloadMipMapLevel(GR_TMU0,square->texadd,
                         GR_LOD_256,GR_LOD_256,GR_ASPECT_1x1,
                         GR_TEXFMT_ARGB_1555,
                         GR_MIPMAPLEVELMASK_BOTH,texinfo.data);
              }
        }
}

void DrawFlatBitmap(void)
{
  struct TexSquare *square;
  int x,y;

  for(y=0;y<texnumy;y++) {
	for(x=0;x<texnumx;x++) {
	  square=texgrid+y*texnumx+x;

	  grTexSource(GR_TMU0,square->texadd,
				  GR_MIPMAPLEVELMASK_BOTH,&texinfo);

	  grDrawTriangle(&(square->vtxA),&(square->vtxD),&(square->vtxC));
	  grDrawTriangle(&(square->vtxA),&(square->vtxB),&(square->vtxC));
	}
  }
}

void xfx_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *vis_area, struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette, unsigned int flags)
{
  if(!sysdep_display_params.vec_src_bounds || (flags & SYSDEP_DISPLAY_UI_DIRTY))
    bitmap_dirty=2;

  if(flags & SYSDEP_DISPLAY_HOTKEY_OPTION1)
  {
    bilinear=1-bilinear;

    if(bilinear)
          grTexFilterMode(GR_TMU0,
                                          GR_TEXTUREFILTER_BILINEAR,
                                          GR_TEXTUREFILTER_BILINEAR);
    else
          grTexFilterMode(GR_TMU0,
                                          GR_TEXTUREFILTER_POINT_SAMPLED,
                                          GR_TEXTUREFILTER_POINT_SAMPLED);
  }
  
  if(bitmap_dirty)
  {
    UpdateTexture(bitmap, vis_area, dirty_area, palette);
    bitmap_dirty--;
  }

  DrawFlatBitmap();
  grBufferSwap(1);
  grBufferClear(0,0,0);
}
