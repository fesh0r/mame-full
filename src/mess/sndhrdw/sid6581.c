/***************************************************************************

  MOS sound interface device sid6581
***************************************************************************/
#include <math.h>
#include "osd_cpu.h"
#include "sound/streams.h"
#include "mame.h"

#define VERBOSE_DBG 0
#include "mess/machine/cbm.h"

#include "sid6581.h"

typedef struct {
	int (*paddle_read) (int offset); 
	int reg[0x20];
} SID6581;

static SID6581 sid6581[2]= {{0}};

static void sid6581_init (SID6581 *this, int (*paddle) (int offset))
{
	memset(this, 0, sizeof(SID6581));
	this->paddle_read = paddle;
}

static void sid6581_port_w (SID6581 *this, int offset, int data)
{
	offset &= 0x1f;
	switch (offset)
	{
	case 0x1d:
	case 0x1e:
	case 0x1f:
		break;
	default:
		this->reg[offset] = data;
	}
}

static int sid6581_port_r (SID6581 *this, int offset)
{
	offset &= 0x1f;
	switch (offset)
	{
	case 0x1d:
	case 0x1e:
	case 0x1f:
		return 0xff;
	case 0x19:						   /* paddle 1 */
		if (this->paddle_read != NULL)
			return this->paddle_read (0);
		return 0;
	case 0x1a:						   /* paddle 2 */
		if (this->paddle_read != NULL)
			return this->paddle_read (1);
		return 0;
	case 0x1b:case 0x1c: /* noise channel readback? */
		return rand();
	default:
		return this->reg[offset];
	}
}

void sid6581_0_init (int (*paddle) (int offset))
{
	sid6581_init(sid6581, paddle);
}

void sid6581_1_init (int (*paddle) (int offset))
{
	sid6581_init(sid6581+1, paddle);
}

int sid6581_0_port_r (int offset)
{
	return sid6581_port_r(sid6581, offset);
}

int sid6581_1_port_r (int offset)
{
	return sid6581_port_r(sid6581+1, offset);
}

void sid6581_0_port_w (int offset, int data)
{
	sid6581_port_w(sid6581, offset, data);
}

void sid6581_1_port_w (int offset, int data)
{
	sid6581_port_w(sid6581+1, offset, data);
}
