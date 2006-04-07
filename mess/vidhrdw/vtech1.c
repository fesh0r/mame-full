/******************************************************************************

Video Technology Laser 110-310 computers:

    Video Technology Laser 110
      Sanyo Laser 110
    Video Technology Laser 200
      Salora Fellow
      Texet TX-8000
      Video Technology VZ-200
    Video Technology Laser 210
      Dick Smith Electronics VZ-200
      Sanyo Laser 210
    Video Technology Laser 310
      Dick Smith Electronics VZ-300

Video hardware:

	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

	Thanks go to:
	- Guy Thomason
	- Jason Oakley
	- Bushy Maunder
	- and anybody else on the vzemu list :)
    - Davide Moretti for the detailed description of the colors.

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/vtech1.h"
#include "vidhrdw/m6847.h"

static ATTR_CONST UINT8 vtech1_get_attributes(UINT8 c)
{
	UINT8 result = 0x00;
	if (c & 0x40)				result |= M6847_INV;
	if (c & 0x80)				result |= M6847_AS;
	if (vtech1_latch & 0x10)	result |= M6847_CSS;
	if (vtech1_latch & 0x08)	result |= M6847_AG | M6847_GM1;
	return result;
}



static const UINT8 *vtech1_get_video_ram(int scanline)
{
	return videoram + (scanline / (vtech1_latch & 0x08 ? 3 : 12)) * 0x20;
}



VIDEO_START( vtech1 )
{
	m6847_config cfg;

	memset(&cfg, 0, sizeof(cfg));
	cfg.type = M6847_VERSION_ORIGINAL_PAL;
	cfg.get_attributes = vtech1_get_attributes;
	cfg.get_video_ram = vtech1_get_video_ram;
	m6847_init(&cfg);
	return 0;
}

