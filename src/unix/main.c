/*
 * X-mame main-routine
 */

#define __MAIN_C_
#include "xmame.h"

#ifdef xgl
	#include "driver.h"
	#include <math.h>
#endif

/* put here anything you need to do when the program is started. Return 0 if */
/* initialization was successful, nonzero otherwise. */
int osd_init(void)
{
	/* now invoice system-dependent initialization */
#ifdef MAME_NET
	if (osd_net_init()      !=OSD_OK) return OSD_NOT_OK;
#endif	
	if (osd_input_initpre() !=OSD_OK) return OSD_NOT_OK;
	
	return OSD_OK;
}

/*
 * Cleanup routines to be executed when the program is terminated.
 */
void osd_exit (void)
{
#ifdef MAME_NET
	osd_net_close();
#endif
	osd_input_close();
}


int main (int argc, char **argv)
{
	int res;
#ifdef xgl
	int i, color_depth;
#endif
	
	/* some display methods need to do some stuff with root rights */
	if (sysdep_init()!= OSD_OK) exit(OSD_NOT_OK);
	
	/* to be absolutly safe force giving up root rights here in case
	   a display method doesn't */
	if(setuid(getuid()))
	{
		perror("setuid");
		sysdep_close();
		return OSD_NOT_OK;
	}
	
        /* Set the title, now auto build from defines from the makefile */
        sprintf(title,"%s (%s) version %s", NAME, DISPLAY_METHOD,
           build_version);

	/* parse configuration file and environment */
	if ((res = config_init(argc, argv)) != 1234) goto leave;
	
        /* Check the colordepth we're requesting */
        if (!options.color_depth && !sysdep_display_16bpp_capable())
           options.color_depth = 8;
#ifdef xgl

	/**
	 * this hack is just to offer the
	 * game's true color as the default,
	 * if -bpp does not tell us otherwise ...
	 *
	 * just the options.color_depth is written,
	 * if not set by the user ...
	 *
	 * diff's to mame.c:
	 * the usage of the video attribute 'VIDEO_NEEDS_6BITS_PER_GUN'
	 * is not logical for me (the opengl display) .. ??
	 */
	Machine->gamedrv = drivers[game_index];
	Machine->drv = Machine->gamedrv->drv;

	i=0; 
	color_depth=1;
	while(i<Machine->drv->color_table_len)
	{
		color_depth*=2;
		i=(int)pow(2,color_depth);
	}

	fprintf(stderr,"**** machine orig. color number: %d\n", 
		Machine->drv->color_table_len);

	fprintf(stderr,"**** machine's orig. color_depth: %d\n", color_depth);

	if( (Machine->drv->video_attributes & VIDEO_RGB_DIRECT) >0 )
	{
		if(color_depth==16) color_depth=15;
		else color_depth=32;
	} else {
		if(Machine->drv->color_table_len>256)
			color_depth=16;
	}

	fprintf(stderr,"**** color_depth regarding video attr.: %d\n", color_depth);

	if(  options.color_depth==0 ||
	    (options.color_depth==16 && color_depth==15)
	  )
	{
		options.color_depth = color_depth;
		fprintf(stderr,"****  setting options.color_depth := %d\n",
			options.color_depth);
	} else {
		fprintf(stderr,"****  given options.color_depth := %d\n",
			options.color_depth);
	}
#endif

	/* go for it */
	res = run_game (game_index);

leave:
	sysdep_close();
	/* should be done last since this also closes stdout and stderr */
	config_exit();

	return res;
}
