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

void osd_fdc_motors(int unit, int state)
{
}

void osd_fdc_density(int unit, int density, int tracks, int spt, int eot, int secl)
{
}

void osd_fdc_interrupt(int param)
{
}

unsigned char osd_fdc_recal(unsigned char * track)
{
	return 0;
}

void osd_fdc_seek(int unit, int dir)
{
}

unsigned char osd_fdc_step(int dir, unsigned char * track)
{
	return 0;
}

void osd_fdc_format(int t, int h, int spt, unsigned char * fmt)
{
}

void osd_fdc_put_sector(int unit, int side, int C, int H, int R, int N, 
						unsigned char *buff, int ddam)
{
}
void osd_fdc_get_sector(int unit, int side, int C, int H, int R, int N, 
						unsigned char *buff, int ddma)
{
}

int osd_fdc_get_status(int unit)
{
	return 0;
}

void osd_fdc_read_id(int unit, int side, unsigned char *pBuffer)
{
}

#endif /* ifdef MESS */
