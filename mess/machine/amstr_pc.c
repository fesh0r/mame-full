#include "driver.h"

#include "includes/pit8253.h"
#include "includes/pc.h"
#include "includes/amstr_pc.h"
#include "includes/pclpt.h"
#include "includes/vga.h"

/* bios power up self test
   important addresses

   fc0c9
   fc0f2
   fc12e
   fc193
   fc20e
   fc2c1
   fc2d4


   fc375
   fc3ba
   fc3e1
   fc412
   fc43c
   fc47d
   fc48f
   fc51f
   fc5a2

   fc1c0
   fc5fa
    the following when language selection not 0 (test for presence of 0x80000..0x9ffff ram)
    fc60e
	fc667
   fc678
   fc6e5
   fc72e
   fc78f
   fc7cb ; coprocessor related
   fc834
    feda6 no problem with disk inserted

   fca2a

   cmos ram 28 0 amstrad pc1512 integrated cga
   cmos ram 23 dipswitches?
*/

static struct {
	// 64 system status register?
	UINT8 port62;
	UINT8 port65;
} pc1640;

/* test sequence in bios
 write 00 to 65
 write 30 to 61
 read 62 and (0x10)
 write 34 to 61
 read 62 and (0x0f)
 return or of the 2 62 reads

 allows set of the original ibm pc "dipswitches"!!!!

 66 write gives reset?
*/

WRITE_HANDLER( pc1640_port60_w )
{
	switch (offset) {
	case 1:
		if (data==0x30) pc1640.port62=(pc1640.port65&0x10)>>4;
		else if (data==0x34) pc1640.port62=pc1640.port65&0xf;
		break;
	case 4:
		if (data&0x80) {
			pc_port[0x60]=data^0x8d;
		} else {
			pc_port[0x60]=data;
		}
		break;
	case 5: pc1640.port65=data;
		break;
	}

	logerror("pc1640 write %.2x %.2x\n",offset,data);
}

READ_HANDLER( pc1640_port60_r )
{
	int data=0;
	switch (offset) {
	case 0: data=pc_port[0x60];break;
	case 2:
		data=pc1640.port62;
		if (pit8253_get_output(0,2)) data|=0x20;
		break;
	}
	return data;
}

int dipstate=0;
READ_HANDLER( pc1640_port378_r )
{
	int data=pc_LPT2_r(offset);
	if (offset==1) data=(data&~7)|(input_port_1_r(0)&7);
	if (offset==2) {
		switch (dipstate) {
		case 0:
			data=(data&~0xe0)|(input_port_1_r(0)&0xe0);
			break;
		case 1:
			data=(data&~0xe0)|((input_port_1_r(0)&0xe000)>>8);
			break;
		case 2:
			data=(data&~0x20)|((input_port_1_r(0)&8)<<2);
			break;

		}
	}
	return data;
}

READ_HANDLER( pc1640_port3d0_r )
{
	if (offset==0xa) dipstate=0;
	return vga_port_03d0_r(offset);
}

READ_HANDLER( pc1640_port4278_r )
{
	if (offset==2) dipstate=1;
	return 0;
}

READ_HANDLER( pc1640_port278_r )
{
	/*if (offset==2) */dipstate=2;
	return 0;
}
