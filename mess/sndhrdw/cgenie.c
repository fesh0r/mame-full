/***************************************************************************

  cgenie.c

***************************************************************************/

#include "driver.h"

static int control_port;

int cgenie_sh_control_port_r(int offset)
{
	return control_port;
}

int cgenie_sh_data_port_r(int offset)
{
	return AY8910_read_port_0_r(offset);
}

void cgenie_sh_control_port_w(int offset, int data)
{
	control_port = data;
	AY8910_control_port_0_w(offset, data);
}

void cgenie_sh_data_port_w(int offset, int data)
{
	AY8910_write_port_0_w(offset, data);
}

