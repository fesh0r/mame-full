/******************************************************************************

******************************************************************************/

#include "vidhrdw/generic.h"
#include "vidhrdw/m6847.h"
#include "includes/apf.h"

unsigned char *apf_video_ram;

static void apf_charproc(UINT8 c)
{
	/* this seems to be the same so far, as it gives the same result as vapf */
	m6847_inv_w(0,		(c & 0x040));
	m6847_as_w(0,		(c & 0x080));	
}

#define APF_DUMP_RAM

#ifdef APF_DUMP_RAM
void apf_dump_ram(void)
{
	void *file;

	file = osd_fopen(Machine->gamedrv->name, "apfram.bin", OSD_FILETYPE_NVRAM,OSD_FOPEN_WRITE);

	if (file)
	{
		osd_fwrite(file, apf_video_ram, 0x0400);

		/* close file */
		osd_fclose(file);
	}
}
#endif


READ_HANDLER(apf_video_r)
{
	return apf_video_ram[offset];
}

WRITE_HANDLER(apf_video_w)
{
	apf_video_ram[offset] = data;
}

int apf_vh_start(void)
{
	struct m6847_init_params p;

	/* allocate video memory ram */
	apf_video_ram = (unsigned char *)malloc(0x0400);

	m6847_vh_normalparams(&p);
	p.version = M6847_VERSION_ORIGINAL_NTSC;
	p.ram = apf_video_ram+0x0200;
	p.ramsize = 0x0400;
	p.charproc = apf_charproc;

	if (m6847_vh_start(&p))
		return 1;

	return (0);
}

void apf_vh_stop(void)
{
	apf_dump_ram();

	/* free video memory ram */
	if (apf_video_ram!=NULL)
	{
		free(apf_video_ram);
		apf_video_ram = NULL;
	}

	m6847_vh_stop();
}

