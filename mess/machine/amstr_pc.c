#include "driver.h"

#include "includes/pit8253.h"
#include "includes/pcshare.h"
#include "includes/amstr_pc.h"
#include "includes/pclpt.h"
#include "includes/pc_vga.h"

/* pc20 (v2)
   fc078
   fc102 color/mono selection
   fc166
   fc1b4
   fd841 (output something)
   ff17c (output something, read monitor type inputs)
   fc212
   fc26c
   fc2df
   fc3fe
   fc0f4
   fc432
   fc49f
   fc514
   fc566
   fc5db
   fc622 in 3de

port 0379 read
port 03de write/read
 */

/* pc1512 (v1)
   fc1b5
   fc1f1
   fc264
   fc310
   fc319
   fc385
   fc436
   fc459
   fc4cb
   fc557
   fc591
   fc624
   fc768
   fc818
   fc87d display amstrad ..
    fca17 keyboard check
	 fca69
	 fd680
      fd7f9
	 fca7b !keyboard interrupt routine for this check 
 */


/* pc1640 (v3)
   bios power up self test
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
   fc5dd mouse

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
	struct {
		UINT8 x,y; //byte clipping needed
	} mouse;

	// 64 system status register?
	UINT8 port60;
	UINT8 port61;
	UINT8 port62;
	UINT8 port65;

	int dipstate;
} pc1640={{0}, 0};

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

/* mouse x counter at 0x78 (read- writable)
   mouse y counter at 0x7a (read- writable) 

   mouse button 1,2 keys
   joystick (4 directions, 2 buttons) keys
   these get value from cmos ram
   74 15 00 enter
   70 17 00 forward del
   77 1b 00 joystick button 1
   78 19 00 joystick button 2


   79 00 4d right
   7a 00 4b left
   7b 00 50 down
   7c 00 48 up

   7e 00 01 mouse button left
   7d 01 01 mouse button right
*/

WRITE8_HANDLER( pc1640_port60_w )
{
	switch (offset) {
	case 1:
		pc1640.port61=data;
		if (data==0x30) pc1640.port62=(pc1640.port65&0x10)>>4;
		else if (data==0x34) pc1640.port62=pc1640.port65&0xf;
		pc_sh_speaker(data&3);
		pc_keyb_set_clock(data&0x40);
		break;
	case 4:
		if (data&0x80) {
			pc1640.port60=data^0x8d;
		} else {
			pc1640.port60=data;
		}
		break;
	case 5: 
		// stores the configuration data for port 62 configuration dipswitch emulation
		pc1640.port65=data; 
		break;
	}
	
	logerror("pc1640 write %.2x %.2x\n",offset,data);
}


 READ8_HANDLER( pc1640_port60_r )
{
	int data=0;
	switch (offset) {
	case 0:
		if (pc1640.port61&0x80)
			data=pc1640.port60;
		else 
			data = pc_keyb_read();
		break;

	case 1:
		data = pc1640.port61;
		break;

	case 2: 
		data = pc1640.port62;
		if (pit8253_get_output(0, 2))
			data |= 0x20;
		break;
	}
	return data;
}

 READ8_HANDLER( pc200_port378_r )
{
	int data=pc_parallelport1_r(offset);
	if (offset==1) data=(data&~7)|(input_port_1_r(0)&7);
	if (offset==2) data=(data&~0xe0)|(input_port_1_r(0)&0xc0);
	return data;
}


 READ8_HANDLER( pc1640_port378_r )
{
	int data=pc_parallelport1_r(offset);
	if (offset==1) data=(data&~7)|(input_port_1_r(0)&7);
	if (offset==2) {
		switch (pc1640.dipstate) {
		case 0:
			data=(data&~0xe0)|(input_port_1_r(0)&0xe0);
			break;
		case 1:
			data=(data&~0xe0)|((input_port_1_r(0)&0xe000)>>8);
			break;
		case 2:
			data=(data&~0xe0)|((input_port_1_r(0)&0xe00)>>4);
			break;

		}
	}
	return data;
}

 READ8_HANDLER( pc1640_port3d0_r )
{
	if (offset==0xa) pc1640.dipstate=0;
	return vga_port_03d0_r(offset);
}

 READ8_HANDLER( pc1640_port4278_r )
{
	if (offset==2) pc1640.dipstate=1;
	// read parallelport
	return 0;
}

 READ8_HANDLER( pc1640_port278_r )
{
	if ((offset==2)||(offset==0)) pc1640.dipstate=2;
	// read parallelport
	return 0;
}

 READ8_HANDLER( pc1640_mouse_x_r )
{
	return pc1640.mouse.x-input_port_13_r(0);
}

 READ8_HANDLER( pc1640_mouse_y_r )
{
	return pc1640.mouse.y-input_port_14_r(0);
}

WRITE8_HANDLER( pc1640_mouse_x_w )
{
	pc1640.mouse.x=data+input_port_13_r(0);
}

WRITE8_HANDLER( pc1640_mouse_y_w )
{
	pc1640.mouse.y=data+input_port_14_r(0);
}

