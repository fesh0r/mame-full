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

void osd_fdc_motors(unsigned char unit, unsigned char state)
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

void osd_fdc_seek(unsigned char unit, signed int dir)
{
}

unsigned char osd_fdc_step(int dir, unsigned char * track)
{
	return 0;
}

void osd_fdc_format(unsigned char t, unsigned char h, unsigned char spt, unsigned char *fmt)
{
}

void osd_fdc_put_sector(unsigned char unit, unsigned char side, unsigned char C, unsigned char H, unsigned char R, unsigned char N,unsigned char *buff, unsigned char ddam)
{
}

void osd_fdc_get_sector(unsigned char unit,unsigned char side, unsigned char C, unsigned char H, unsigned char R, unsigned char N,unsigned char *buff)
{
}

unsigned char osd_fdc_get_status(unsigned char unit)
{
	return 0;
}

#endif /* ifdef MESS */
