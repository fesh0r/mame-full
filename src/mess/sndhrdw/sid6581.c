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

static int sid6581[0x20];
static int(*paddle_read)(int offset);

void sid6581_init(int(*paddle)(int offset))
{
  paddle_read=paddle;
}

void sid6581_port_w(int offset, int data)
{
  offset&=0x1f;
  switch (offset) {
  case 0x1d: case 0x1e: case 0x1f:
    break;
  default:
    sid6581[offset]=data;
  }
}

int sid6581_port_r(int offset)
{
  offset&=0x1f;
  switch (offset) {
  case 0x1d: case 0x1e: case 0x1f:
    return 0xff;
  case 0x19: /* paddle 1*/
    if (paddle_read!=NULL) return paddle_read(0);
    return 0;
  case 0x1a: /* paddle 2*/
    if (paddle_read!=NULL) return paddle_read(1);
    return 0;
  default:
    return sid6581[offset];
  }
}
