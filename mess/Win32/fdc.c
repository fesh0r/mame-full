#include "osdepend.h"
#include "mess.h"

/* ************************************************************************ */
/* Floppy disk controller                                                   */
/* ************************************************************************ */

int osd_fdc_init(void)
{
	return 0;
}

void osd_fdc_motors(unsigned char unit, unsigned char state)
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


void osd_fdc_seek(unsigned char unit, signed int dir)
{
}

unsigned char osd_fdc_recal(unsigned char *track)
{
	return 0;
}


void osd_fdc_put_sector(unsigned char unit, unsigned char side, unsigned char C, unsigned char H, unsigned char R, unsigned char N,unsigned char *buff, unsigned char ddam)
{
}

void osd_fdc_get_sector(unsigned char unit,unsigned char side, unsigned char C, unsigned char H, unsigned char R, unsigned char N,unsigned char *buff)
{
}

void osd_fdc_exit(void)
{
	return;
}

void osd_fdc_format(unsigned char t, unsigned char h, unsigned char spt, unsigned char *fmt)
{
}

unsigned char osd_fdc_get_status(unsigned char unit)
{
	return 0;
}

