/**********************************************************************

	Motorola 6883 SAM interface and emulation

	This function emulates all the functionality of one M6883
	synchronous address multiplexer.

	Note that the real SAM chip was intimately involved in things like
	memory and video addressing, which are things that the MAME core
	largely handles.  Thus, this code only takes care of a small part
	of the SAM's actual functionality; it simply tracks the SAM
	registers and handles things like save states.  It then delegates
	the bulk of the responsibilities back to the host.

**********************************************************************/

#include "driver.h"

struct sam6883_interface
{
	void (*set_rowheight)(int rowheight);
	void (*set_displayoffset)(int offset);
	void (*set_pageonemode)(int val);
	void (*set_mpurate)(int val);
	void (*set_memorysize)(int val);
	void (*set_maptype)(int val);
};

void sam_init(void);
void sam_config(const struct sam6883_interface *intf);
void sam_reset(void);

WRITE_HANDLER(sam_w);

void sam_setstate(UINT16 state, UINT16 mask);
