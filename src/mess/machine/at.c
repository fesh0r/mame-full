#include "driver.h"
#include "mess/mess.h"
#include "cpu/i86/i286.h"

#include "pic8259.h"
#include "mc146818.h"
#include "mess/vidhrdw/vga.h"

#include "pc.h"
#include "at.h"

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

#define true TRUE
#define false FALSE
#define bool int

void init_at(void)
{
	init_pc();
	mc146818_init(MC146818_STANDARD);
}

void init_at_vga(void)
{
#if 0
        int i; 
        UINT8 *memory=memory_region(REGION_CPU1)+0xc0000;
        UINT8 chksum;

		/* oak vga */
        /* plausibility check of retrace signals goes wrong */
        memory[0x00f5]=memory[0x00f6]=memory[0x00f7]=0x90;
        memory[0x00f8]=memory[0x00f9]=memory[0x00fa]=0x90;
        for (chksum=0, i=0;i<0x7fff;i++) {
                chksum+=memory[i];
        }
        memory[i]=0x100-chksum;
#endif

#if 0
        for (chksum=0, i=0;i<0x8000;i++) {
                chksum+=memory[i];
        }
        printf("checksum %.2x\n",chksum);
#endif

		vga_init(input_port_15_r);
		//pc_blink_textcolors = NULL;
		mc146818_init(MC146818_STANDARD);
}

void at_driver_close(void)
{
	mc146818_close();
}


#define KEYBOARD_ON 1
#define PS2_MOUSE_ON 1

static struct {	
	int mf2;
	bool on;
	UINT8 delay;   /* 240/60 -> 0,25s */
	UINT8 repeat;   /* 240/ 8 -> 30/s */
	bool numlock;
	UINT8 queue[256];
	UINT8 head;
	UINT8 tail;
	UINT8 make[128];	
	int input_state;
} keyboard = { 0, 0, 60, 8 };

static const struct {
	char* pressed;
	char* released;
} keyboard_mf2_code[0x10][2/*numlock off, on*/]={ 
	{	{ "\xe0\x12", "\xe0\x92" } },
	{	{ "\xe0\x13", "\xe0\x93" } },
	{	{ "\xe0\x35", "\xe0\xb5" } },
	{	{ "\xe0\x37", "\xe0\xb7" } },
	{	{ "\xe0\x38", "\xe0\xb8" } },
	{	{ "\xe0\x47", "\xe0\xc7" }, { "\xe0\x2a\xe0\x47", "\xe0\xc7\xe0\xaa" } },
	{	{ "\xe0\x48", "\xe0\xc8" }, { "\xe0\x2a\xe0\x48", "\xe0\xc8\xe0\xaa" } },
	{	{ "\xe0\x49", "\xe0\xc9" }, { "\xe0\x2a\xe0\x49", "\xe0\xc9\xe0\xaa" } },
	{	{ "\xe0\x4b", "\xe0\xcb" }, { "\xe0\x2a\xe0\x4b", "\xe0\xcb\xe0\xaa" } },
	{	{ "\xe0\x4d", "\xe0\xcd" },	{ "\xe0\x2a\xe0\x4d", "\xe0\xcd\xe0\xaa" } },
	{	{ "\xe0\x4f", "\xe0\xcf" }, { "\xe0\x2a\xe0\x4f", "\xe0\xcf\xe0\xaa" } },
	{	{ "\xe0\x50", "\xe0\xd0" }, { "\xe0\x2a\xe0\x50", "\xe0\xd0\xe0\xaa" } },
	{	{ "\xe0\x51", "\xe0\xd1" }, { "\xe0\x2a\xe0\x51", "\xe0\xd1\xe0\xaa" } },
	{	{ "\xe0\x52", "\xe0\xd2" }, { "\xe0\x2a\xe0\x52", "\xe0\xd2\xe0\xaa" } },
	{	{ "\xe0\x53", "\xe0\xd3" }, { "\xe0\x2a\xe0\x53", "\xe0\xd3\xe0\xaa" } },
	{	{ "\xe1\x1d\x45\xe1\x9d\xc5" }, { "\xe0\x2a\xe1\x1d\x45\xe1\x9d\xc5" } }
};

#if 0
/* mouse */
static UINT8 m_queue[256];
static UINT8 m_head = 0, m_tail = 0, mb = 0;
static void *mouse_timer = NULL;

static void pc_mouse_scan(int n);
static void pc_mouse_poll(int n);
#endif

static void at_keyboard_queue_insert(UINT8 data)
{
	logerror("keyboard queueing %.2x\n",data);
	keyboard.queue[keyboard.head] = data;
	keyboard.head = ++keyboard.head % 256;
}

static void at_keyboard_helper(const char *codes)
{
	int i;
	for (i=0; codes[i]; i++) {
		keyboard.queue[keyboard.head] = codes[i];
		keyboard.head = ++keyboard.head % 256;
	}
}

/**************************************************************************
 *	scan keys and stuff make/break codes
 **************************************************************************/
static void at_keyboard_polling(void)
{
	int i;

	update_input_ports();
    for( i = 0x01; i < 0x60; i++  )
	{
		if( readinputport(i/16 + 4) & (1 << (i & 15)) )
		{
			if( keyboard.make[i] == 0 )
			{
				keyboard.make[i] = 1;
				if (i==0x45) keyboard.numlock^=1;
				at_keyboard_queue_insert(i);
			}
			else
			{
				keyboard.make[i] += 1;
				if( keyboard.make[i] == keyboard.delay )
				{
					at_keyboard_queue_insert(i);
				}
				else
				if( keyboard.make[i] == keyboard.delay + keyboard.repeat )
				{
					keyboard.make[i] = keyboard.delay;
					at_keyboard_queue_insert(i);
				}
			}
		}
		else
		if( keyboard.make[i] )
		{
			keyboard.make[i] = 0;
			at_keyboard_queue_insert(i|0x80);
		}
    }

    for( i = 0x60; i < 0x70; i++  )
	{
		if( readinputport(i/16 + 4) & (1 << (i & 15)) )
		{
			if( keyboard.make[i] == 0 )
			{
				keyboard.make[i] = 1;
				if (keyboard_mf2_code[i-0x60][keyboard.numlock].pressed)
					at_keyboard_helper(keyboard_mf2_code[i-0x60][keyboard.numlock].pressed);
				else
					at_keyboard_helper(keyboard_mf2_code[i-0x60][0].pressed);
			}
			else
			{
				keyboard.make[i] += 1;
				if( keyboard.make[i] == keyboard.delay )
				{
				if (keyboard_mf2_code[i-0x60][keyboard.numlock].pressed)
					at_keyboard_helper(keyboard_mf2_code[i-0x60][keyboard.numlock].pressed);
				else
					at_keyboard_helper(keyboard_mf2_code[i-0x60][0].pressed);
				}
				else
					if( keyboard.make[i] == keyboard.delay + keyboard.repeat )
					{
						keyboard.make[i]=keyboard.delay;
						if (keyboard_mf2_code[i-0x60][keyboard.numlock].pressed)
							at_keyboard_helper(keyboard_mf2_code[i-0x60][keyboard.numlock].pressed);
						else
							at_keyboard_helper(keyboard_mf2_code[i-0x60][0].pressed);
					}
			}
		}
		else
			if( keyboard.make[i] )
			{
				keyboard.make[i] = 0;
				if (keyboard_mf2_code[i-0x60][keyboard.numlock].released)
					at_keyboard_helper(keyboard_mf2_code[i-0x60][keyboard.numlock].released);
				else if (keyboard_mf2_code[i-0x60][0].released)
					at_keyboard_helper(keyboard_mf2_code[i-0x60][0].released);
			}
    }
}

static int at_keyboard_read(void)
{
	int data;
	if (keyboard.tail==keyboard.head) return -1;

	data=keyboard.queue[keyboard.tail];
	logerror("keyboard read %.2x\n",data);
	keyboard.tail = ++keyboard.tail%256;
	return data;
}

static void at_keyboard_write(UINT8 data)
{
	logerror("keyboard write %.2x\n",data);
	switch (keyboard.input_state) {
	case 0:
		switch (data) {
		case 0xed: // leds schalten
			keyboard.input_state=1;
			break;
		case 0xee: // echo
			at_keyboard_queue_insert(0xee);
			break;
		case 0xf0: // scancodes adjust
			keyboard.input_state=2;
			break;
		case 0xf2: // identify keyboard
			at_keyboard_queue_insert(0xfa);
			if (keyboard.mf2) {
				at_keyboard_queue_insert(0xab);
				at_keyboard_queue_insert(0x41);
			}
			break;
		case 0xf3: // adjust rates
			keyboard.input_state=3;
			break;
		case 0xf4: // activate
			keyboard.on=1;
			break;
		case 0xf5:
			// standardvalues
			keyboard.on=0;
			break;
		case 0xf6:
			// standardvalues
			keyboard.on=1;
			break;
		case 0xfe: // resend
			// should not occur
			break;
		case 0xff: // reset
			at_keyboard_queue_insert(0xfa);
			at_keyboard_queue_insert(0xaa);
			break;
		}
		break;
	case 1: 
		/* 0 scroll lock, 1 num lock, 2 capslock */
		keyboard.input_state=0;
		break;
	case 2:
		/* 01h, 02h, 03h !? */
		keyboard.input_state=0;
		break;
	case 3:
		/* 6,5: 250ms, 500ms, 750ms, 1s */
		/* 4..0: 30 26.7 .... 2 chars/s*/
		keyboard.input_state=0;
		break;
	}
}


static struct {
	UINT8 inport, outport, data;

	struct {
		bool received;
		bool on;
	} keyboard;
	struct {
		bool received;
		bool on;
	} mouse;

	bool last_write_to_control;
	bool sending;
	bool send_to_mouse;

	int operation_write_state;
	int status_read_mode;
} at_8042={ 0x80 };

static void at_8042_receive(UINT8 data)
{
	at_8042.data=data;
	at_8042.keyboard.received=1;
	if( !pic8259_0_irq_pending(1)) {
		pic8259_0_issue_irq(1);
	}
}


void at_8042_time(void)
{
#if 1
	if( !pic8259_0_irq_pending(1)
		&&!at_8042.keyboard.received )
#else
	if( !pic8259_0_irq_pending(1)
		&&(at_8042.keyboard.on&&!at_8042.keyboard.received)
		&&!at_8042.mouse.received )
#endif
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
	int data=0;
	switch (offset) {
	case 0:
		data=at_8042.data;
		at_8042.keyboard.received=0;
		at_8042.mouse.received=0;
		DBG_LOG(1,"AT 8042 read",("%.2x %02x\n",offset, data) );
		break;
	case 1:
		data=pc_PIO_r(offset);
		data&=~0xc0; // at bios don't likes this being set

		/* polled vor changes in at bios */
		pc_port[0x61]^=0x10;
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
	return data;
}

WRITE_HANDLER(at_8042_w)
{
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
			i286_set_address_mask(data&2?0xffffff:0xfffff);
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
		pc_PIO_w(offset,data);
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
		case 0xae: at_8042.keyboard.on=true;break;
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
		case 0xf0:
		case 0xf2:
		case 0xf4:
		case 0xf6:
		case 0xf8:
		case 0xfa:
		case 0xfc:
		case 0xfe:
			cpu_set_reset_line(0, PULSE_LINE);
			break;
		}
		at_8042.sending=1;
		break;
	}
}

/*************************************************************************
 *
 *		ATHD
 *		AT hard disk
 *
 *************************************************************************/
WRITE_HANDLER(at_mfm_0_w)
{
	switch (offset) {
	case 0: pc_ide_data_w(data);				break;
	case 1: pc_ide_write_precomp_w(data);		break;
	case 2: pc_ide_sector_count_w(data);		break;
	case 3: pc_ide_sector_number_w(data);		break;
	case 4: pc_ide_cylinder_number_l_w(data);	break;
	case 5: pc_ide_cylinder_number_h_w(data);	break;
	case 6: pc_ide_drive_head_w(data);			break;
	case 7: pc_ide_command_w(data); 			break;
	}
}

READ_HANDLER(at_mfm_0_r)
{
	int data=0;

	switch (offset) {
	case 0: data = pc_ide_data_r(); 			break;
	case 1: data = pc_ide_error_r();			break;
	case 2: data = pc_ide_sector_count_r(); 	break;
	case 3: data = pc_ide_sector_number_r();	break;
	case 4: data = pc_ide_cylinder_number_l_r();break;
	case 5: data = pc_ide_cylinder_number_h_r();break;
	case 6: data = pc_ide_drive_head_r();		break;
	case 7: data = pc_ide_status_r();			break;
	}
	return data;
}

int at_frame_interrupt (void)
{
	static int turboswitch=-1;

	if (turboswitch !=(input_port_3_r(0)&2)) {
		if (input_port_3_r(0)&2)
			timer_set_overclock(0, 1);
		else 
			timer_set_overclock(0, 4.77/12);
		turboswitch=input_port_3_r(0)&2;
	}

#if 0
	if( ((++pc_framecnt & 63) == 63)&&pc_blink_textcolors)
		(*pc_blink_textcolors)(pc_framecnt & 64 );
#endif
    if( !onscrd_active() && !setup_active() ) {
		at_keyboard_polling();
		at_8042_time();
	}

    return ignore_interrupt ();
}
