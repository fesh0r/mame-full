/***************************************************************************
  TI-85 driver by Krzysztof Strzecha
  
  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/ti85.h"

int timer_int_mask;
int timer_int_status;
int ON_int_mask;
int ON_int_status;
int ON_pressed;
UINT8 rom_page;
UINT8 LCD_memory;
UINT8 LCD_contrast;
int LCD_status;
int LCD_mask;
UINT8 power_mode;
UINT8 keypad_mask;

static void * ti85_timer;


static void ti85_timer_callback (int param)
{
	if (timer_int_mask)
	{
		cpu_set_irq_line(0,0, HOLD_LINE);
		timer_int_status = 1;
	}
	if (readinputport(8)&0x01)
	{
		if (ON_int_mask && !ON_pressed)
		{
			cpu_set_irq_line(0,0, HOLD_LINE);
			ON_int_status = 1;
			timer_int_mask = !timer_int_mask;
			logerror ("ON interrupt\n");
		}       
		ON_pressed = 1;
	}
	else
		ON_pressed = 0;
}

void update_ti85_memory (void)
{
	cpu_setbank(2,memory_region(REGION_CPU1) + 0x010000 + 0x004000*rom_page);
}

void ti85_init_machine(void)
{
	ti85_timer = timer_pulse(TIME_IN_HZ(200), 0, ti85_timer_callback);
	memory_set_bankhandler_r(1, 0, MRA_BANK1);
	memory_set_bankhandler_w(3, 0, MWA_ROM);
	memory_set_bankhandler_r(2, 0, MRA_BANK2);
	memory_set_bankhandler_w(4, 0, MWA_ROM);
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x010000);
	cpu_setbank(2,memory_region(REGION_CPU1) + 0x014000);
}

void ti85_stop_machine(void)
{
	/* free timer */
	if (ti85_timer!=NULL)
	{
		timer_remove(ti85_timer);
		ti85_timer = NULL;
	}

}

READ_HANDLER ( ti85_port_0000_r )
{
	return 0xff;
}

READ_HANDLER ( ti85_port_0001_r )
{
	int data = 0xff;
	int port;
	int bit;

	if (keypad_mask == 0x7f) return data;

	for (bit = 0; bit < 7; bit++)
		if (~keypad_mask&(0x01<<bit))
			for (port = 0; port < 8; port++)
				data ^= readinputport(port)&(0x01<<bit) ? 0x01<<port : 0x00;

	return data;
}

READ_HANDLER ( ti85_port_0002_r )
{
	return 0xff;
}

READ_HANDLER ( ti85_port_0003_r )
{
	int data = 0;

	if (LCD_status)
		if (LCD_mask)
			data |= 0x02;
	if (ON_int_status)
		data |= 0x01;
	if (timer_int_status)
		data |= 0x04;
	if (!ON_pressed)
		data |= 0x08;

	ON_int_status = 0;
	timer_int_status = 0;
	return data;
}

READ_HANDLER ( ti85_port_0004_r )
{
	return 0xff;
}

READ_HANDLER ( ti85_port_0005_r )
{
	return rom_page;
}

READ_HANDLER ( ti85_port_0006_r )
{
	return power_mode;
}

READ_HANDLER ( ti85_port_0007_r )
{
//	return ti85_hardware_registers[7];
	return 0xff;
}

WRITE_HANDLER ( ti85_port_0000_w )
{
	LCD_memory = data;
}

WRITE_HANDLER ( ti85_port_0001_w )
{
	keypad_mask = data&0x7f;
}

WRITE_HANDLER ( ti85_port_0002_w )
{
	LCD_contrast = data;
}

WRITE_HANDLER ( ti85_port_0003_w )
{
	ON_int_mask = data&0x01;
//	timer_int_mask = data&0x04;
	LCD_mask = data&0x02;
	LCD_status = data&0x08;
}

WRITE_HANDLER ( ti85_port_0004_w )
{
}

WRITE_HANDLER ( ti85_port_0005_w )
{
	rom_page = data;
	update_ti85_memory();
}

WRITE_HANDLER ( ti85_port_0006_w )
{
	power_mode = data;
}

WRITE_HANDLER ( ti85_port_0007_w )
{
//	ti85_hardware_registers[7] = data;
}
