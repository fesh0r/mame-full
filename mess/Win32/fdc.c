#include "osdepend.h"
#include "mess.h"

/* ************************************************************************ */
/* Floppy disk controller                                                   */
/* ************************************************************************ */

int osd_fdc_init(void)
{
	return 0;
}

void osd_fdc_motors(unsigned char unit)
{
	return;
}


void osd_fdc_density(unsigned char unit, unsigned char density,unsigned char tracks, unsigned char spt, unsigned char eot, unsigned char secl)
{
	return;
}

unsigned char osd_fdc_step(int dir, unsigned char *track)
{
	return 0;
}


unsigned char osd_fdc_seek(unsigned char t, unsigned char *track)
{
	return 0;
}

unsigned char osd_fdc_recal(unsigned char *track)
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

void osd_fdc_exit(void)
{
	return;
}

unsigned char osd_fdc_format(unsigned char t, unsigned char h, unsigned char spt, unsigned char *fmt)
{
	return 0;
}

