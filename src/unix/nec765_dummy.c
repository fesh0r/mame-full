#include "xmame.h"
#ifdef MESS

int osd_fdc_init(void)
{
	/* this means init failed, since it isn't supported under unix */
	return 0; 
}

void osd_fdc_exit(void)
{
}

void osd_fdc_motors(unsigned char unit)
{
}

void osd_fdc_density(unsigned char unit, unsigned char density, unsigned char tracks, unsigned char spt, unsigned char eot, unsigned char secl)
{
}

void osd_fdc_interrupt(int param)
{
}

unsigned char osd_fdc_recal(unsigned char * track)
{
	return 0;
}

unsigned char osd_fdc_seek(unsigned char t, unsigned char * track)
{
	return 0;
}

unsigned char osd_fdc_step(int dir, unsigned char * track)
{
	return 0;
}

unsigned char osd_fdc_format(unsigned char t, unsigned char h, unsigned char spt, unsigned char * fmt)
{
	return 0;
}

unsigned char osd_fdc_put_sector(unsigned char track, unsigned char side, unsigned char head, unsigned char sector, unsigned char *buff, unsigned char ddam)
{
	return 0;
}

unsigned char osd_fdc_get_sector(unsigned char track, unsigned char side, unsigned char head, unsigned char sector, unsigned char *buff)
{
	return 0;
}

#endif /* ifdef MESS */
