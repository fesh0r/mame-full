#include "driver.h"
#include "mess/mess.h"
#include "cpu/i86/i286.h"

#include "mess/includes/pic8259.h"
#include "mess/includes/mc146818.h"
#include "mess/includes/vga.h"
#include "mess/includes/pc_cga.h"
#include "mess/includes/pc.h"
#include "mess/includes/at.h"
#include "mess/includes/pckeybrd.h"

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

#define true TRUE
#define false FALSE
#define bool int

void init_at(void)
{
	init_pc();
	mc146818_init(MC146818_STANDARD);
	/* initialise keyboard */
	at_keyboard_init();
	at_keyboard_set_scan_code_set(1);
	at_keyboard_set_input_port_base(4);
	at_keyboard_set_type(AT_KEYBOARD_TYPE_AT);
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

	init_pc_common();
	mc146818_init(MC146818_STANDARD);
	/* initialise keyboard */
	at_keyboard_init();
	at_keyboard_set_scan_code_set(1);
	at_keyboard_set_input_port_base(4);
	at_keyboard_set_type(AT_KEYBOARD_TYPE_AT);

	vga_init(input_port_0_r);
}

void at_driver_close(void)
{
	mc146818_close();
}

void at_machine_init(void)
{

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

	// temporary hack
	int offset1;
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
		data=pc_ppi_portb_r(offset);
		data&=~0xc0; // at bios don't likes this being set

		/* polled vor changes in at bios */
		at_8042.offset1^=0x10;
		data=(data&~0x10)|at_8042.offset1;
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
		pc_ppi_portb_w(offset,data);
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

int at_cga_frame_interrupt (void)
{
	static int turboswitch=-1;

	if (turboswitch !=(input_port_3_r(0)&2)) {
		if (input_port_3_r(0)&2)
			timer_set_overclock(0, 1);
		else 
			timer_set_overclock(0, 4.77/12);
		turboswitch=input_port_3_r(0)&2;
	}

	pc_cga_timer();

    if( !onscrd_active() && !setup_active() ) {
		at_keyboard_polling();
		at_8042_time();
	}

    return ignore_interrupt ();
}

int at_vga_frame_interrupt (void)
{
	static int turboswitch=-1;

	if (turboswitch !=(input_port_3_r(0)&2)) {
		if (input_port_3_r(0)&2)
			timer_set_overclock(0, 1);
		else 
			timer_set_overclock(0, 4.77/12);
		turboswitch=input_port_3_r(0)&2;
	}

//	vga_timer();

    if( !onscrd_active() && !setup_active() ) {
		at_keyboard_polling();
		at_8042_time();
	}

    return ignore_interrupt ();
}
