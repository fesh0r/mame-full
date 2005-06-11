/***************************************************************************

	machine/bebox.c

	BeBox

	Memory map:

	00000000 - 3FFFFFFF   Physical RAM
	40000000 - 7FFFFFFF   Motherboard glue registers
	80000000 - 807FFFFF   ISA I/O
	81000000 - BF7FFFFF   PCI I/O
	BFFFFFF0 - BFFFFFFF   PCI/ISA interrupt acknowledge
	FF000000 - FFFFFFFF   ROM/flash

	In ISA space, peripherals are generally in similar places as they are on
	standard PC hardware (e.g. - the keyboard is 80000060 and 80000064).  The
	following table shows more:

	Keyboard/Mouse      (Intel 8242)   80000060, 80000064
	Real Time Clock     (BQ3285)       80000070, 80000074
	IDE ATA Interface	               800001F0-F7, 800003F6-7
	COM2                               800002F8-F
	GeekPort A/D Control               80000360-1
	GeekPort A/D Data                  80000362-3
	GeekPort A/D Rate                  80000364
	GeekPort OE                        80000366
	Infrared Interface                 80000368-A
	COM3                               80000380-7
	COM4                               80000388-F
	GeekPort D/A                       80000390-3
	GeekPort GPA/GPB                   80000394
	Joystick Buttons                   80000397
	SuperIO Config (PReP standard)     80000398-9
	MIDI Port 1                        800003A0-7
	MIDI Port 2                        800003A8-F
	Parallel                           800003BC-E
	Floppy                             800003F0-7
	COM1                               800003F8-F
	AD1848                             80000830-4


  Interrupt bit masks:

	bit 31	- N/A (used to set/clear masks)
	bit 30	- SMI interrupt to CPU 0 (CPU 0 only)
	bit 29	- SMI interrupt to CPU 1 (CPU 1 only)
	bit 28	- Unused
	bit 27	- COM1 (PC IRQ #4)
	bit 26	- COM2 (PC IRQ #3)
	bit 25	- COM3
	bit 24	- COM4
	bit 23	- MIDI1
	bit 22	- MIDI2
	bit 21	- SCSI
	bit 20	- PCI Slot #1
	bit 19	- PCI Slot #2
	bit 18	- PCI Slot #3
	bit 17	- Sound
	bit 16	- Keyboard (PC IRQ #1)
	bit 15	- Real Time Clock (PC IRQ #8)
	bit 14	- PC IRQ #5
	bit 13	- Floppy Disk (PC IRQ #6)
	bit 12	- Parallel Port (PC IRQ #7)
	bit 11	- PC IRQ #9
	bit 10	- PC IRQ #10
	bit  9	- PC IRQ #11
	bit  8	- Mouse (PC IRQ #12)
	bit  7	- IDE (PC IRQ #14)
	bit  6	- PC IRQ #15
	bit  5	- PIC8259
	bit  4  - Infrared Controller
	bit  3  - Analog To Digital
	bit  2  - GeekPort
	bit  1  - Unused
	bit  0  - Unused

	Be documentation uses PowerPC bit numbering conventions (i.e. - bit #0 is
	the most significant bit)

	PCI Devices:
		#0		Motorola MPC105
		#11		Intel 82378 PCI/ISA bridge
		#12		NCR 53C810 SCSI

	More hardware information at http://www.netbsd.org/Ports/bebox/hardware.html

***************************************************************************/

#include "includes/bebox.h"
#include "vidhrdw/pc_vga.h"
#include "vidhrdw/cirrus.h"
#include "cpu/powerpc/ppc.h"
#include "machine/uart8250.h"
#include "machine/pc_fdc.h"
#include "machine/mpc105.h"
#include "machine/mc146818.h"
#include "machine/pic8259.h"
#include "machine/idectrl.h"
#include "machine/pci.h"

#define LOG_CPUIMASK	1
#define LOG_UART		1
#define LOG_INTERRUPTS	1

static data32_t bebox_cpu_imask[2];
static data32_t bebox_interrupts;
static data32_t bebox_crossproc_interrupts;



/*************************************
 *
 *	Interrupts and Motherboard Registers
 *
 *************************************/

static void bebox_mbreg32_w(data32_t *target, data64_t data, data64_t mem_mask)
{
	int i;

	for (i = 1; i < 32; i++)
	{
		if ((data >> (63 - i)) & 1)
		{
			if ((data >> 63) & 1)
				*target |= 0x80000000 >> i;
			else
				*target &= ~(0x80000000 >> i);
		}
	}
}



READ64_HANDLER( bebox_cpu0_imask_r )		{ return ((data64_t) bebox_cpu_imask[0]) << 32; }
READ64_HANDLER( bebox_cpu1_imask_r )		{ return ((data64_t) bebox_cpu_imask[1]) << 32; }
READ64_HANDLER( bebox_interrupt_sources_r )	{ return ((data64_t) bebox_interrupts) << 32; }

WRITE64_HANDLER( bebox_cpu0_imask_w )
{
	data32_t old_imask = bebox_cpu_imask[0];

	bebox_mbreg32_w(&bebox_cpu_imask[0], data, mem_mask);
	if (LOG_CPUIMASK && (old_imask != bebox_cpu_imask[0]))
	{
		logerror("BeBox CPU #0 pc=0x%08X imask=0x%08x\n",
			(unsigned) activecpu_get_reg(REG_PC), bebox_cpu_imask[0]);
	}
}

WRITE64_HANDLER( bebox_cpu1_imask_w )
{
	data32_t old_imask = bebox_cpu_imask[1];

	bebox_mbreg32_w(&bebox_cpu_imask[1], data, mem_mask);
	if (LOG_CPUIMASK && (old_imask != bebox_cpu_imask[1]))
	{
		logerror("BeBox CPU #1 pc=0x%08X imask=0x%08x\n",
			(unsigned) activecpu_get_reg(REG_PC), bebox_cpu_imask[1]);
	}
}

READ64_HANDLER( bebox_crossproc_interrupts_r )
{
	data32_t result;
	result = bebox_crossproc_interrupts;
	if (cpu_getactivecpu())
		result |= 0x02000000;
	else
		result &= ~0x02000000;
	return ((data64_t) result) << 32;
}

WRITE64_HANDLER( bebox_crossproc_interrupts_w )
{
	static const struct
	{
		data32_t mask;
		int cpunum;
		int active_high;
		int inputline;
	} crossproc_map[] =
	{
		{ 0x40000000, 0, 1, PPC_INPUT_LINE_SMI },
		{ 0x20000000, 1, 1, PPC_INPUT_LINE_SMI },
		{ 0x08000000, 0, 0, PPC_INPUT_LINE_TLBISYNC },
		{ 0x04000000, 1, 0, PPC_INPUT_LINE_TLBISYNC }
	};
	int i, line;
	data32_t old_crossproc_interrupts = bebox_crossproc_interrupts;

	bebox_mbreg32_w(&bebox_crossproc_interrupts, data, mem_mask);

	for (i = 0; i < sizeof(crossproc_map) / sizeof(crossproc_map[0]); i++)
	{
		if ((old_crossproc_interrupts ^ bebox_crossproc_interrupts) & crossproc_map[i].mask)
		{
			if (bebox_crossproc_interrupts & crossproc_map[i].mask)
				line = crossproc_map[i].active_high ? ASSERT_LINE : CLEAR_LINE;
			else
				line = crossproc_map[i].active_high ? CLEAR_LINE : ASSERT_LINE;

			if (LOG_INTERRUPTS)
			{
				logerror("bebox_crossproc_interrupts_w(): CPU #%d %s %s\n",
					crossproc_map[i].cpunum, line ? "Asserting" : "Clearing",
					(crossproc_map[i].inputline == PPC_INPUT_LINE_SMI) ? "SMI" : "TLBISYNC");
			}

			cpunum_set_input_line(crossproc_map[i].cpunum, crossproc_map[i].inputline, line);
		}
	}
}

WRITE64_HANDLER( bebox_processor_resets_w )
{
	UINT8 b = (UINT8) (data >> 56);

	if (b & 0x20)
	{
		cpunum_set_input_line(1, INPUT_LINE_RESET, (b & 0x80) ? CLEAR_LINE : ASSERT_LINE);
	}
}



static void bebox_update_interrupts(void)
{
	int cpunum;
	data32_t interrupt;

	for (cpunum = 0; cpunum < 2; cpunum++)
	{
		interrupt = bebox_interrupts & bebox_cpu_imask[cpunum];
		
		if (LOG_INTERRUPTS)
		{
			logerror("bebox_update_interrupts(): CPU #%d [%08X|%08X] IRQ %s\n", cpunum,
				bebox_interrupts, bebox_cpu_imask[cpunum], interrupt ? "on" : "off");
		}

		cpunum_set_input_line(cpunum, INPUT_LINE_IRQ0, interrupt ? ASSERT_LINE : CLEAR_LINE);
	}
}



static void bebox_set_irq_bit(unsigned int interrupt_bit, int val)
{
	static const char *interrupt_names[32] =
	{
		NULL,
		NULL,
		"GEEKPORT",
		"ADC",
		"IR",
		"PIC8259",
		"PCIRQ 15",
		"IDE",
		"MOUSE",
		"PCIRQ 11",
		"PCIRQ 10",
		"PCIRQ 9",
		"PARALLEL",
		"FLOPPY",
		"PCIRQ 5",
		"RTC",
		"KEYBOARD",
		"SOUND",
		"PCI2",
		"PCI1",
		"SCSI",
		"MIDI2",
		"MIDI1",
		"COM4",
		"COM3",
		"COM2",
		"COM1",
		NULL,
		"SMI1",
		"SMI0",
		NULL
	};
	data32_t old_interrupts;

	if (LOG_INTERRUPTS)
	{
		/* make sure that we don't shoot ourself in the foot */
		if ((interrupt_bit > sizeof(interrupt_names) / sizeof(interrupt_names[0])) && !interrupt_names[interrupt_bit])
			osd_die("Raising invalid interrupt %u", interrupt_bit);

		logerror("bebox_set_irq_bit(): pc[0]=0x%08x pc[1]=0x%08x %s interrupt #%u (%s)\n",
			(unsigned) cpunum_get_reg(0, REG_PC),
			(unsigned) cpunum_get_reg(1, REG_PC),
			val ? "Asserting" : "Clearing",
			interrupt_bit, interrupt_names[interrupt_bit]);
	}

	old_interrupts = bebox_interrupts;
	if (val)
		bebox_interrupts |= 1 << interrupt_bit;
	else
		bebox_interrupts &= ~(1 << interrupt_bit);

	/* if interrupt values have changed, update the lines */
	if (bebox_interrupts != old_interrupts)
		bebox_update_interrupts();
}



/*************************************
 *
 *	COM ports
 *
 *************************************/

static void bebox_uart_transmit(int id, int data)
{
	if (LOG_UART)
		logerror("bebox_uart_transmit(): id=%d data=0x%02X\n", id, data);
}



static void bebox_uart_handshake(int id, int data)
{
	if (LOG_UART)
		logerror("bebox_uart_handshake(): id=%d data=0x%02X\n", id, data);
}



static const uart8250_interface bebox_uart_inteface =
{
	TYPE8250,
	0,
	NULL,
	bebox_uart_transmit,
	bebox_uart_handshake
};



/*************************************
 *
 *	Floppy Disk Controller
 *
 *************************************/

static void bebox_fdc_interrupt_delayed(int state)
{
	bebox_set_irq_bit(13, state);
	pic8259_set_irq_line(0, 6, state);
}

static void bebox_fdc_interrupt(int state)
{
	timer_set(TIME_IN_MSEC(60), state, bebox_fdc_interrupt_delayed);
}



static const struct pc_fdc_interface bebox_fdc_interface =
{
	bebox_fdc_interrupt,
	NULL
};



/*************************************
 *
 *	8259 PIC
 *
 *************************************/

READ64_HANDLER( bebox_interrupt_ack_r )
{
	int result;
	result = 1 << pic8259_acknowledge(0);
	bebox_set_irq_bit(5, 0);	/* HACK */
	return ((data64_t) result) << 56;
}



static void bebox_pic_set_int_line(int interrupt)
{
	bebox_set_irq_bit(5, interrupt);
}



/*************************************
 *
 *	Floppy/IDE/ATA
 *
 *************************************/

static READ8_HANDLER( bebox_800001F0_8_r ) { return ide_controller_0_r(offset + 0x1F0); }
static WRITE8_HANDLER( bebox_800001F0_8_w ) { ide_controller_0_w(offset + 0x1F0, data); }

READ64_HANDLER( bebox_800001F0_r ) { return read64be_with_read8_handler(bebox_800001F0_8_r, offset, mem_mask); }
WRITE64_HANDLER( bebox_800001F0_w ) { write64be_with_write8_handler(bebox_800001F0_8_w, offset, data, mem_mask); }



READ64_HANDLER( bebox_800003F0_r )
{
	data64_t result = pc64be_fdc_r(offset, mem_mask | 0xFFFF);

	if (((mem_mask >> 8) & 0xFF) == 0)
	{
		result &= ~(0xFF << 8);
		result |= ide_controller_0_r(0x3F6) << 8;
	}
	
	if (((mem_mask >> 0) & 0xFF) == 0)
	{
		result &= ~(0xFF << 0);
		result |= ide_controller_0_r(0x3F7) << 0;
	}
	return result;
}



WRITE64_HANDLER( bebox_800003F0_w )
{
	pc64be_fdc_w(offset, data, mem_mask | 0xFFFF);

	if (((mem_mask >> 8) & 0xFF) == 0)
		ide_controller_0_w(0x3F6, (data >> 8) & 0xFF);
	
	if (((mem_mask >> 0) & 0xFF) == 0)
		ide_controller_0_w(0x3F7, (data >> 0) & 0xFF);
}



static void bebox_ide_interrupt(int state)
{
	bebox_set_irq_bit(7, state);
	pic8259_set_irq_line(0, 14, state);
}



static struct ide_interface bebox_ide_interface =
{
	bebox_ide_interrupt
};



/*************************************
 *
 *	Video card (Cirrus Logic CL-GD5430)
 *
 *************************************/

static READ64_HANDLER( bebox_video_r )
{
	const UINT64 *mem = (const UINT64 *) pc_vga_memory();
	return BIG_ENDIANIZE_INT64(mem[offset]);
}



static WRITE64_HANDLER( bebox_video_w )
{
	UINT64 *mem = (UINT64 *) pc_vga_memory();
	data = BIG_ENDIANIZE_INT64(data);
	mem_mask = BIG_ENDIANIZE_INT64(mem_mask);
	COMBINE_DATA(&mem[offset]);
}



static const struct pc_vga_interface bebox_vga_interface =
{
	NULL,
	ADDRESS_SPACE_PROGRAM,
	0x80000000
};



/*************************************
 *
 *	Driver main
 *
 *************************************/

MACHINE_INIT( bebox )
{
	cpunum_set_input_line(0, INPUT_LINE_RESET, CLEAR_LINE);
	cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
}



DRIVER_INIT( bebox )
{
	int cpu;

	mpc105_init(0);
	pci_add_device(0, 1, &cirrus5430_callbacks);

	cpu_setbank(1, memory_region(REGION_USER1));
	cpu_setbank(2, memory_region(REGION_USER2));

	/* install MESS managed RAM */
	for (cpu = 0; cpu < 2; cpu++)
	{
		memory_install_read64_handler(cpu, ADDRESS_SPACE_PROGRAM, 0, mess_ram_size - 1, 0, 0x02000000, MRA64_BANK3);
		memory_install_write64_handler(cpu, ADDRESS_SPACE_PROGRAM, 0, mess_ram_size - 1, 0, 0x02000000, MWA64_BANK3);
	}
	cpu_setbank(3, mess_ram);

	uart8250_init(0, NULL);
	uart8250_init(1, NULL);
	uart8250_init(2, NULL);
	uart8250_init(3, &bebox_uart_inteface);

	pc_fdc_init(&bebox_fdc_interface);
	mc146818_init(MC146818_STANDARD);
	pic8259_init(2, bebox_pic_set_int_line);
	ide_controller_init_custom(0, &bebox_ide_interface, NULL);
	pc_vga_init(&bebox_vga_interface, &cirrus_svga_interface);

	/* install VGA memory */
	for (cpu = 0; cpu < 2; cpu++)
	{
		memory_install_read64_handler(cpu, ADDRESS_SPACE_PROGRAM, 0xC1000000, 0xC103FFFF, 0, 0, bebox_video_r);
		memory_install_write64_handler(cpu, ADDRESS_SPACE_PROGRAM, 0xC1000000, 0xC103FFFF, 0, 0, bebox_video_w);
	}
}
