/***************************************************************************

  amstrad.c

***************************************************************************/

#include "driver.h"

static  int control_port;

int     amstrad_sh_control_port_r(int offset)
{
        return control_port;
}

int     amstrad_sh_data_port_r(int offset)
{
        return AY8910_read_port_0_r(offset);
}

void    amstrad_sh_control_port_w(int data)
{
        control_port = data;
        AY8910_control_port_0_w(0, data);
}

void    amstrad_sh_data_port_w(int data)
{
        AY8910_write_port_0_w(0, data);
}

