#include "osdepend.h"
#include "mess.h"

/* ************************************************************************ */
/* Floppy disk controller                                                   */
/* ************************************************************************ */

int osd_fdc_init(void)
{
	return 0;
}

void osd_fdc_motors(int unit, int state)
{
	return;
}


void osd_fdc_density(int unit, int density, int tracks, int spt, int eot, int secl)
{
	return;
}

unsigned char osd_fdc_step(int dir, unsigned char *track)
{
	return 0;
}


void osd_fdc_seek(int unit, int dir)
{
}

unsigned char osd_fdc_recal(unsigned char *track)
{
	return 0;
}


void osd_fdc_put_sector(int unit, int side, int C, int H, int R, int N, UINT8 *buff, int ddam)
{
}

void osd_fdc_get_sector(int unit, int side, int C, int H, int R, int N, UINT8 *buff, int ddma)
{
}

void osd_fdc_exit(void)
{
	return;
}

void osd_fdc_format(int t, int h, int spt, UINT8 *fmt)
{
}

int osd_fdc_get_status(int unit)
{
	return 0;
}

void osd_fdc_read_id(int unit, int side, unsigned char *pBuffer)
{
}

