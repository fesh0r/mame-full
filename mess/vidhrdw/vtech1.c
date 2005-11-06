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

/*
bit 3 of cassette/speaker/vdp control latch defines if the mode is text or
graphics
*/

static void vtech1_charproc(UINT8 c)
{
	/* bit 7 defines if semigraphics/text used */
	/* bit 6 defines if chars should be inverted */

	m6847_inv_w(0,      (c & 0x40));
	m6847_as_w(0,		(c & 0x80));
	/* it appears this is fixed */
	m6847_intext_w(0,	0);
}

VIDEO_START( vtech1 )
{
	struct m6847_init_params p;

	m6847_vh_normalparams(&p);
	p.version = M6847_VERSION_ORIGINAL_PAL;
	p.ram = videoram;
	p.ramsize = videoram_size;
	p.charproc = vtech1_charproc;

	if (video_start_m6847(&p))
		return 1;

	return (0);
}
