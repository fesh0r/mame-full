#include "driver.h"

#include "includes/pckeybrd.h"
#include "includes/ibmat.h"
#include "includes/pit8253.h"
#include "includes/pic8259.h"
#include "includes/pcshare.h"

#define PS2_MOUSE_ON 1
#define KEYBOARD_ON 1

#undef DBG_LOG
#if 1
#define DBG_LOG(level, text, print) \
		if (level>0) { \
				logerror("%s\t", text); \
				logerror print; \
		}
#else
#define DBG_LOG(level, text, print)
#endif


/*
at post
-------
  f81d2 01
  f82e6 05
  f8356 07
  f83e5 0a
  f847e 0e
  f8e7c 10
  f8f3a 13
  f9058 1a
  f913a 1e
  fa8ba 30
  fa96c 36
  fa9d3 3c
  fa9f4 3e
  ff122 50
  ff226 5b
  ff29f 5f
  f9228 70
  f92b2 74 ide?

ibm at bios
-----------
f0101 after switch to real mode jump back!!!!!!!!!!
jumping table
f0123
f098e memory tests
f10b4 ???
not reached
f1050
f1617
f0119
f10d8
f10b7 system board error

f019f
f025b
f02e6
f0323
f03b3 0e
f03d7 0f
f058d
 at8042 signal timing test
 sets errorcode!

f0655
f06a3 postcode 15
f06ba 16
f0747 18
f0763 enter pm! (0x81, 0x85)
f0766 1a
f07b3 first 640kb memory test
 f084c 1c
f086e extended memory test
f0928 1f
f097d 20
???
f0ff4 34
???
f1675 f0
f16cb f2
 illegal access trap test!!!!
f16fe f3
 task descriptor test!!!!
f174a f4
f17af f5
f1800 f6 writing to non write segment
f1852 f7 arpl
f1880 f8 lar, lsl
f18ca fa
f10d8
f10ec 35
f1106 36
f1137 !!!!!!!keyboard test fails

f11aa 3a
f1240 3c harddisk controller test!!!
f13f3 3b
 f1a6d xthdd bios init
f1429
f1462
f1493 40
f1532
 keyboard lock
 f1 to unlock
f155c
 jumps to f0050 (reset)  without enabling of the a20 gate --> hangs
0412 bit 5 must be set to reach f1579
f1579
f15c3 41
f1621 43

routines
f1945 read cmos ram
f195f write to cmos al value ah
f1a3a poll 0x61 bit 4
f1a49 sets something in cmos ram
f1d30 switch to protected mode

neat
----
f80b9

at386
-----
fd28c fd
fd2c3 fc
f40dc
fd949
fd8e3
fd982
f4219 01
f4296 03
f42f3 04
f4377 05
f43ec 06
f4430 08
f6017 switches to PM
f4456 09
f55a2
f44ec 0d
f4557 20
f462d 27 my special friend, the keyboard controller once more
 ed0a1
f4679 28
 fa16a
f46d6
f4768 2c
f47f0 2e
 f5081
 fa16a
 f9a83
  checksum error on extended cmos
f4840 34
f488c 35
 reset
f48ee
f493e 3a
f49cd
 f4fc7
  fe842
f4a5a
f4b01 38
 memory test
f4b41 3b
f4c0f
 Invalid ...
f4c5c
 f86fc
  f8838

f9a83 output text at return address!, return after text


at486
-----
f81a5 03
f1096 0f 09 wbinvd i486 instruction
*/

static struct {
	AT8042_TYPE type;
	void (*set_address_mask)(unsigned mask);

	UINT8 inport, outport, data;

	struct {
		int received;
		int on;
	} keyboard;
	struct {
		int received;
		int on;
	} mouse;

	int last_write_to_control;
	int sending;
	int send_to_mouse;

	int operation_write_state;
	int status_read_mode;

	int speaker;

	// temporary hack
	int offset1;
} at_8042;

void at_8042_init(AT8042_CONFIG *config)
{
	at_8042.type=config->type;
	at_8042.set_address_mask=config->set_address_mask;
	at_8042.inport=0xa0; // ibmat bios wants 0x20 set! (keyboard locked when not set)
						//	0x80

}

static void at_8042_receive(UINT8 data)
{
	logerror("at8042 %.2x received\n",data);
	at_8042.data=data;
	at_8042.keyboard.received=1;
	if( !pic8259_0_irq_pending(1)) { // this forgets some interrupts
		pic8259_0_issue_irq(1);
	}
}


void at_8042_time(void)
{
	if( !pic8259_0_irq_pending(1)
		&&(at_8042.keyboard.on&&!at_8042.keyboard.received)
		&&!at_8042.mouse.received )
	{
		int data;
		if ( (data=at_keyboard_read())!=-1) {
			at_8042_receive(data);
		}
	}
}

/* 0x60 in- and output buffer ( keyboard mouse data)
 0x64 read status register
 write operation for controller

 output port controller
  7: keyboard data
  6: keyboard clock
  5: mouse buffer full
  4: keyboard buffer full
  3: mouse clock
  2: mouse data
  1: 0 a20 cleared
  0: 0 system reset

 input port controller
  7: 0 keyboard locked
  6: 1 monochrom?
  5..2: res
  1: mouse data in
  0: keyboard data in
*/


READ_HANDLER(at_8042_r)
{
	static int poll_delay=10;
	int data=0;
	switch (offset) {
	case 0:
		data=at_8042.data;
		if (at_8042.type!=AT8042_AT386) {
			at_8042.keyboard.received=0; //at386 self test dont likes this
			at_8042.mouse.received=0;
		}
		DBG_LOG(1,"AT 8042 read",("%.2x %02x\n",offset, data) );
		break;
	case 1:
		data=at_8042.speaker;
		data&=~0xc0; // at bios don't likes this being set

// needed for ami bios, maybe only some keyboard controller revisions!
		at_8042.keyboard.received=0;
		at_8042.mouse.received=0;

		/* polled for changes in ibmat bios */
		if (--poll_delay<0) {
			if (at_8042.type!=AT8042_PS2) {
				poll_delay=4; // ibmat
			} else {
				poll_delay=8; // ibm ps2m30
			}
			at_8042.offset1^=0x10;
		}
		data=(data&~0x10)|at_8042.offset1;

		if (at_8042.speaker&1) data|=0x20;
		else data&=~0x20; // ps2m30 wants this

		break;
	case 2:
		if (pit8253_get_output(0,2)) data|=0x20;
		else data&=~0x20;
		break;
	case 4:
		if (!at_8042.keyboard.received && !at_8042.mouse.received) {
			int temp;
			if ( (temp=at_keyboard_read())!=-1) at_8042_receive(temp);
		}
		if (at_8042.keyboard.received
			||at_8042.mouse.received) data|=1;
		if (at_8042.sending) data|=2;
		at_8042.sending=0; /* quicker than normal */
		data |= 4; /* selftest ok */
		if (at_8042.last_write_to_control) data|=8;
		switch (at_8042.status_read_mode) {
		case 0:
			if (!at_8042.keyboard.on) data|=0x10;
			if (at_8042.mouse.received) data|=0x20;
			break;
		case 1:
			data|=at_8042.inport&0xf;
			break;
		case 2:
			data|=at_8042.inport<<4;
			break;
		}
		//DBG_LOG(1,"AT 8042 read",("%.2x %02x\n",offset, data) );
		break;
	}
//	logerror("at_8042 read %.2x %.2x\n",offset,data);
	return data;
}

WRITE_HANDLER(at_8042_w)
{
	logerror("at_8042 write %.2x %.2x\n",offset,data);
	switch (offset) {
	case 0:
		DBG_LOG(1,"AT 8042 write",("%.2x %02x\n",offset, data) );
		at_8042.last_write_to_control=0;
		at_8042.status_read_mode=0;
		switch (at_8042.operation_write_state) {
		case 0:
			at_8042.data=data;
			at_8042.sending=1;
			at_keyboard_write(data);
			break;
		case 1:
			at_8042.operation_write_state=0;
			at_8042.outport=data;
			//if (data&1) cpu_set_reset_line(0, PULSE_LINE);
#if 0
			logerror("addressmask %.6x\n",data&2?0xffffff:0xfffff);
#endif
			at_8042.set_address_mask(data&2?0xffffff:0xfffff);
			break;
		case 2:
			at_8042.operation_write_state=0;
			at_8042.data=data;
			at_8042.sending=1;
			at_keyboard_write(data);
			break;
		case 3:
			at_8042.operation_write_state=0;
			at_8042.data=data;
			break;
		case 4:
			at_8042.operation_write_state=0;
			at_8042.data=data;
			break;
		}
		break;
	case 1:
		at_8042.speaker=data;
		pc_sh_speaker(data&3);
		break;
	case 4:
		DBG_LOG(1,"AT 8042 write",("%.2x %02x\n",offset, data) );
		at_8042.last_write_to_control=0;
		switch(data) {
		case 0xa7: at_8042.mouse.on=false;break;
		case 0xa8: at_8042.mouse.on=true;break;
		case 0xa9: /* test mouse */
			if (PS2_MOUSE_ON)
				at_8042_receive(0);
			else
				at_8042_receive(0xff);
			break;
		case 0xaa: /* selftest */
			at_8042_receive(0x55);
			break;
		case 0xab: /* test keyboard */
			if (KEYBOARD_ON)
				at_8042_receive(0);
			else
				at_8042_receive(0xff);
			break;
		case 0xad: at_8042.keyboard.on=false;break;
		case 0xae: //at_8042.keyboard.received=0;
			at_8042.keyboard.on=true;break;
		case 0xc0: at_8042_receive(at_8042.inport);break;
		case 0xc1: /* read inputport 3..0 until write to 0x60 */
			at_8042.status_read_mode=1;break;
		case 0xc2: /* read inputport 7..4 until write to 0x60 */
			at_8042.status_read_mode=2;break;
		case 0xd0:
			at_8042_receive(at_8042.outport);
			break;
		case 0xd1:
			at_8042.operation_write_state=1;
			break;
		case 0xd2:
			at_8042.operation_write_state=2;
			at_8042.send_to_mouse=0;
			break;
		case 0xd3:
			at_8042.operation_write_state=3;
			at_8042.send_to_mouse=1;
			break;
		case 0xd4:
			at_8042.operation_write_state=4;
			break;
			/* 0xf0 .. 0xff bit 0..3 0 means low pulse of 6ms in outputport */
//		case 0xe0: // ibmat bios
		case 0xf0:
		case 0xf2:
		case 0xf4:
		case 0xf6:
		case 0xf8:
		case 0xfa:
		case 0xfc:
		case 0xfe:
			cpu_set_reset_line(0, PULSE_LINE);
			at_8042.set_address_mask(0xffffff);
			break;
		}
		at_8042.sending=1;
		break;
	}
}

