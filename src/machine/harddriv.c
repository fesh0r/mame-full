/***************************************************************************

	Hard Drivin' machine hardware

****************************************************************************/

#include "machine/atarigen.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/tms34010/34010ops.h"
#include "cpu/adsp2100/adsp2100.h"
#include "sndhrdw/atarijsa.h"


/*************************************
 *
 *	Constants and macros
 *
 *************************************/

#define DUART_CLOCK TIME_IN_HZ(36864000)

/* debugging tools */
#define LOG_COMMANDS		0



/*************************************
 *
 *	External definitions
 *
 *************************************/

/* externally accessible */
UINT8 *hdmsp_ram;
UINT8 *hd68k_slapstic_base;
UINT8 *hddsk_ram;
UINT8 *hddsk_rom;
UINT8 *hddsk_zram;

UINT8 *hdgsp_speedup_addr[2];
offs_t hdgsp_speedup_pc;

UINT8 *hdmsp_speedup_addr;
offs_t hdmsp_speedup_pc;

offs_t hdadsp_speedup_pc;


/* from slapstic.c */
int slapstic_tweak(offs_t offset);
void slapstic_reset(void);


/* from vidhrdw */
extern UINT8 *hdgsp_vram;
extern UINT8 *hdmsp_ram;



/*************************************
 *
 *	Static globals
 *
 *************************************/

static UINT8 gsp_cpu;
static UINT8 msp_cpu;
static UINT8 adsp_cpu;

static UINT8 irq_state;
static UINT8 gsp_irq_state;
static UINT8 msp_irq_state;
static UINT8 adsp_irq_state;
static UINT8 duart_irq_state;

static UINT8 duart_read_data[16];
static UINT8 duart_write_data[16];
static UINT8 duart_output_port;
static void *duart_timer;

static UINT8 pedal_value;

static UINT8 last_gsp_shiftreg;

static UINT8 m68k_zp1, m68k_zp2;
static UINT8 m68k_adsp_buffer_bank;

static UINT8 adsp_halt, adsp_br;
static UINT8 adsp_xflag;

static UINT16 adsp_sim_address;
static UINT16 adsp_som_address;
static UINT32 adsp_eprom_base;

static UINT8 *sim_memory;
static UINT8 *som_memory;
static UINT8 *adsp_memory;

static UINT8 ds3_gcmd, ds3_gflag, ds3_g68irqs, ds3_g68flag, ds3_send;
static UINT16 ds3_gdata, ds3_g68data;
static UINT32 ds3_sim_address;

static UINT16 adc_control;
static UINT8 adc8_select;
static UINT8 adc8_data;
static UINT8 adc12_select;
static UINT8 adc12_byte;
static UINT16 adc12_data;



/*************************************
 *
 *	Initialization
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (msp_irq_state)
		newstate = 1;
	if (adsp_irq_state)
		newstate = 2;
	if (gsp_irq_state)
		newstate = 3;
	if (atarigen_sound_int_state)	/* /LINKIRQ on STUN Runner */
		newstate = 4;
	if (irq_state)
		newstate = 5;
	if (duart_irq_state)
		newstate = 6;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


void harddriv_init_machine(void)
{
	int i;
	
	/* generic reset */
	atarigen_eeprom_reset();
	slapstic_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarijsa_reset();
	
	/* determine the CPU numbers */
	gsp_cpu = msp_cpu = adsp_cpu = 0;
	for (i = 1; i < MAX_CPU; i++)
		switch (Machine->drv->cpu[i].cpu_type & ~CPU_FLAGS_MASK)
		{
			case CPU_TMS34010:
				if (gsp_cpu == 0)
					gsp_cpu = i;
				else
					msp_cpu = i;
				break;
			
			case CPU_ADSP2100:
			case CPU_ADSP2105:
				adsp_cpu = i;
				break;
		}
	
	/* predetermine memory regions */
	sim_memory = memory_region(REGION_USER1);
	som_memory = memory_region(REGION_USER2);
	adsp_memory = memory_region(REGION_CPU1 + adsp_cpu);

	/* set up the mirrored GSP banks */
	cpu_setbank(2, hdgsp_vram);

	/* set up the mirrored MSP banks */
	cpu_setbank(4, hdmsp_ram);
	cpu_setbank(5, hdmsp_ram);

	/* halt the ADSP to start */
	cpu_set_halt_line(adsp_cpu, ASSERT_LINE);
	
	last_gsp_shiftreg = 0;
	
	m68k_adsp_buffer_bank = 0;
	
	irq_state = gsp_irq_state = msp_irq_state = adsp_irq_state = duart_irq_state = 0;

	memset(duart_read_data, 0, sizeof(duart_read_data));
	memset(duart_write_data, 0, sizeof(duart_write_data));
	duart_output_port = 0;
	duart_timer = NULL;
	
	adsp_halt = 1;
	adsp_br = 0;
	adsp_xflag = 0;
	
	pedal_value = 0;
}



/*************************************
 *
 *	Interrupt handling
 *
 *************************************/

int hd68k_vblank_gen(void)
{
	/* update the pedals once per frame */
	if (input_port_5_r(0) & 1)
	{
		if (pedal_value < 255 - 16)
			pedal_value += 16;
	}
	else
	{
		if (pedal_value > 16)
			pedal_value -= 16;
	}
	return atarigen_video_int_gen();
}


int hd68k_irq_gen(void)
{
	irq_state = 1;
	atarigen_update_interrupts();
	return 0;
}


WRITE_HANDLER( hd68k_irq_ack_w )
{
	irq_state = 0;
	atarigen_update_interrupts();
}


void hdgsp_irq_gen(int state)
{
	gsp_irq_state = state;
	atarigen_update_interrupts();
}


void hdmsp_irq_gen(int state)
{
	msp_irq_state = state;
	atarigen_update_interrupts();
}



/*************************************
 *
 *	I/O read dispatch.
 *
 *************************************/

READ_HANDLER( hd68k_port0_r )
{
	/* port is as follows:
	
		0x0001 = DIAGN
		0x0002 = /HSYNCB
		0x0004 = /VSYNCB
		0x0008 = EOC12
		0x0010 = EOC8
		0x0020 = SELF-TEST
		0x0040 = COIN2
		0x0080 = COIN1
		0x0100 = SW1 #8
		0x0200 = SW1 #7
			.....
		0x8000 = SW1 #1
	*/
	int temp = input_port_0_r(offset);
	if (atarigen_get_hblank()) temp ^= 0x0002;
	temp ^= 0x0018;		/* both EOCs always high for now */
	return temp;
}



/*************************************
 *
 *	GSP I/O register writes
 *
 *************************************/

WRITE_HANDLER( hdgsp_io_w )
{
	/* detect an enabling of the shift register and force yielding */
	if (offset == REG_DPYCTL*2)
	{
		UINT8 new_shiftreg = (data >> 11) & 1;
		if (new_shiftreg != last_gsp_shiftreg)
		{
			last_gsp_shiftreg = new_shiftreg;
			if (new_shiftreg)
				cpu_yield();
		}
	}
	tms34010_io_register_w(offset, data);
}



/*************************************
 *
 *	GSP I/O memory handlers
 *
 *************************************/

READ_HANDLER( hd68k_gsp_io_r )
{
	offset = (offset / 4) ^ 1;
	return tms34010_host_r(gsp_cpu, offset);
}

WRITE_HANDLER( hd68k_gsp_io_w )
{
	offset = (offset / 4) ^ 1;
	tms34010_host_w(gsp_cpu, offset, data);
}



/*************************************
 *
 *	MSP I/O memory handlers
 *
 *************************************/

READ_HANDLER( hd68k_msp_io_r )
{
	offset = (offset / 4) ^ 1;
	return msp_cpu ? tms34010_host_r(msp_cpu, offset) : 0xffff;
}

WRITE_HANDLER( hd68k_msp_io_w )
{
	offset = (offset / 4) ^ 1;
	if (msp_cpu)
		tms34010_host_w(msp_cpu, offset, data);
}



/*************************************
 *
 *	ADSP program memory handlers
 *
 *************************************/

READ_HANDLER( hd68k_adsp_program_r )
{
	UINT32 *base = (UINT32 *)&adsp_memory[ADSP2100_PGM_OFFSET + (offset & ~3)];
	UINT32 word = *base;
	return (!(offset & 2)) ? (word >> 16) : (word & 0xffff);
}

WRITE_HANDLER( hd68k_adsp_program_w )
{
	UINT32 *base = (UINT32 *)&adsp_memory[ADSP2100_PGM_OFFSET + (offset & ~3)];
	UINT32 oldword = *base;
	UINT16 temp;
	
	if (!(offset & 2))
	{
		temp = oldword >> 16;
		COMBINE_WORD_MEM(&temp, data);
		oldword = (oldword & 0x0000ffff) | (temp << 16);
	}
	else
	{
		temp = oldword & 0xffff;
		COMBINE_WORD_MEM(&temp, data);
		oldword = (oldword & 0xffff0000) | temp;
	}
	ADSP2100_WRPGM(base, oldword);
}



/*************************************
 *
 *	ADSP data memory handlers
 *
 *************************************/

READ_HANDLER( hd68k_adsp_data_r )
{
	return READ_WORD(&adsp_memory[ADSP2100_DATA_OFFSET + offset]);
}

WRITE_HANDLER( hd68k_adsp_data_w )
{
	COMBINE_WORD_MEM(&adsp_memory[ADSP2100_DATA_OFFSET + offset], data);
	
	/* any write to $1FFF is taken to be a trigger; synchronize the CPUs */
	if ((offset >> 1) == 0x1fff)
	{
		logerror("ADSP sync address written (%04X)\n", data);
		timer_set(TIME_NOW, 0, 0);
		cpu_trigger(554433);
	}
}



/*************************************
 *
 *	ADSP buffer memory handlers
 *
 *************************************/

READ_HANDLER( hd68k_adsp_buffer_r )
{
/*	logerror("hd68k_adsp_buffer_r(%04X)\n", offset);*/
	return READ_WORD(&som_memory[m68k_adsp_buffer_bank * 0x4000 + offset]);
}

WRITE_HANDLER( hd68k_adsp_buffer_w )
{
	COMBINE_WORD_MEM(&som_memory[m68k_adsp_buffer_bank * 0x4000 + offset], data);
}



/*************************************
 *
 *	ADSP control memory handlers
 *
 *************************************/

static void deferred_adsp_bank_switch(int data)
{
#if LOG_COMMANDS
	if (m68k_adsp_buffer_bank != data && keyboard_pressed(KEYCODE_L))
	{
		static FILE *commands;
		if (!commands) commands = fopen("commands.log", "w");
		if (commands)
		{
			INT16 *base = (INT16 *)&som_memory[data * 0x4000];
			INT16 *end = base + (UINT16)*base++;
			INT16 *current = base;
			INT16 *table = base + (UINT16)*current++;
			
			fprintf(commands, "\n---------------\n");
			
			while ((current + 5) < table)
			{
				int offset = (int)(current - base);
				int c1 = *current++;
				int c2 = *current++;
				int c3 = *current++;
				int c4 = *current++;
				fprintf(commands, "Cmd @ %04X = %04X  %d-%d @ %d\n", offset, c1, c2, c3, c4);
				while (current < table)
				{
					UINT32 rslope, lslope;
					rslope = (UINT16)*current++, 
					rslope |= *current++ << 16;
					if (rslope == 0xffffffff)
					{
						fprintf(commands, "  (end)\n");
						break;
					}
					lslope = (UINT16)*current++, 
					lslope |= *current++ << 16;
					fprintf(commands, "  L=%08X R=%08X count=%d\n",
							(int)lslope, (int)rslope, (int)*current++);
				}
			}
			fprintf(commands, "\nTable:\n");
			current = table;
			while (current < end)
				fprintf(commands, "  %04X\n", *current++);
		}
	}
#endif
	m68k_adsp_buffer_bank = data;
}

WRITE_HANDLER( hd68k_adsp_irq_clear_w )
{
	adsp_irq_state = 0;
	atarigen_update_interrupts();
}

WRITE_HANDLER( hd68k_adsp_control_w )
{
	int val = (offset >> 4) & 1;
	
	switch (offset & 0x0f)
	{
		case 0x00:
		case 0x02:
			/* LEDs */
			break;
				
		case 0x06:
			timer_set(TIME_NOW, val, deferred_adsp_bank_switch);
			break;
		
		case 0x0a:
			/* connected to the /BR (bus request) line; this effectively halts */
			/* the ADSP at the next instruction boundary */
			adsp_br = !val;
			if (adsp_br || adsp_halt)
				cpu_set_halt_line(adsp_cpu, ASSERT_LINE);
			else
			{
				cpu_set_halt_line(adsp_cpu, CLEAR_LINE);
				/* a yield in this case is not enough */
				/* we would need to increase the interleaving otherwise */
				/* note that this only affects the test mode */
				cpu_spin();
			}
			break;
		
		case 0x0c:
			/* connected to the /HALT line; this effectively halts */
			/* the ADSP at the next instruction boundary */
			adsp_halt = !val;
			if (adsp_br || adsp_halt)
				cpu_set_halt_line(adsp_cpu, ASSERT_LINE);
			else
			{
				cpu_set_halt_line(adsp_cpu, CLEAR_LINE);
				/* a yield in this case is not enough */
				/* we would need to increase the interleaving otherwise */
				/* note that this only affects the test mode */
				cpu_spin();
			}
			break;
		
		case 0x0e:
			cpu_set_reset_line(adsp_cpu, val ? CLEAR_LINE : ASSERT_LINE);
			cpu_yield();
			break;
		
		default:
			logerror("ADSP control %02X = %04X\n", offset, data);
			break;
	}
}

READ_HANDLER( hd68k_adsp_irq_state_r )
{
	int result = 0xfffd;
	if (adsp_xflag) result ^= 2;
	if (adsp_irq_state) result ^= 1;
	return result;
}


/*************************************
 *
 *	DS III I/O
 *
 *************************************/

WRITE_HANDLER( hd68k_ds3_control_w )
{
	int val = (offset >> 4) & 1;
	
	switch (offset & 0x0f)
	{
		case 0x04:
			/* connected to the /BR (bus request) line; this effectively halts */
			/* the ADSP at the next instruction boundary */
			adsp_br = !val;
			if (adsp_br)
				cpu_set_halt_line(adsp_cpu, ASSERT_LINE);
			else
			{
				cpu_set_halt_line(adsp_cpu, CLEAR_LINE);
				/* a yield in this case is not enough */
				/* we would need to increase the interleaving otherwise */
				/* note that this only affects the test mode */
				cpu_spin();
			}
			break;
		
		case 0x06:
			cpu_set_reset_line(adsp_cpu, val ? CLEAR_LINE : ASSERT_LINE);
			cpu_yield();
			break;
		
		case 0x0e:
			/* LED */
			break;
				
		default:
			logerror("DS III control %02X = %04X\n", offset, data);
			break;
	}
}

READ_HANDLER( hd68k_ds3_irq_state_r )
{
	int result = 0x0fff;
	if (ds3_g68flag) result ^= 0x8000;
	if (ds3_gflag) result ^= 0x4000;
	if (ds3_g68irqs) result ^= 0x2000;
	if (!adsp_irq_state) result ^= 0x1000;
	return result;
}


/*************************************
 *
 *	DS III internal I/O
 *
 *************************************/

static void update_ds3_irq(void)
{
	if ((ds3_g68flag || !ds3_g68irqs) && (!ds3_gflag || ds3_g68irqs))
		cpu_set_irq_line(adsp_cpu, ADSP2100_IRQ2, ASSERT_LINE);
	else
		cpu_set_irq_line(adsp_cpu, ADSP2100_IRQ2, CLEAR_LINE);
}

READ_HANDLER( hdds3_special_r )
{
	int result;

	switch (offset / 2)
	{
		case 0x00:
			ds3_g68flag = 0;
			update_ds3_irq();
			return ds3_g68data;
		
		case 0x01:
			result = 0x0fff;
			if (ds3_gcmd) result ^= 0x8000;
			if (ds3_g68flag) result ^= 0x4000;
			if (ds3_gflag) result ^= 0x2000;
			return result;
		
		case 0x06:
			return sim_memory[ds3_sim_address];
	}
	return 0;
}

WRITE_HANDLER( hdds3_special_w )
{
	switch (offset / 2)
	{
		case 0x00:
			ds3_gdata = data;
			ds3_gflag = 1;
			update_ds3_irq();
			break;
		
		case 0x01:
			adsp_irq_state = (data >> 9) & 1;
			update_interrupts();
			break;
		
		case 0x02:
			ds3_send = (data >> 8) & 1;
			break;
		
		case 0x03:
			ds3_g68irqs = (~data >> 9) & 1;
			break;
			
		case 0x04:
			ds3_sim_address = (ds3_sim_address & 0xffff0000) | (data & 0xffff);
			break;
		
		case 0x05:
			ds3_sim_address = (ds3_sim_address & 0xffff) | ((data << 16) & 0x00070000);
			break;
	}
}



/*************************************
 *
 *	DS III program memory handlers
 *
 *************************************/

READ_HANDLER( hd68k_ds3_program_r )
{
	UINT32 offs = ((offset & 0x3fff) << 1) | ((offset & 0x4000) >> 14);
	UINT32 *base = (UINT32 *)&adsp_memory[ADSP2100_PGM_OFFSET + (offs & ~3)];
	UINT32 word = *base;
	return (!(offs & 2)) ? (word >> 16) : (word & 0xffff);
}

WRITE_HANDLER( hd68k_ds3_program_w )
{
	UINT32 offs = ((offset & 0x3fff) << 1) | ((offset & 0x4000) >> 14);
	UINT32 *base = (UINT32 *)&adsp_memory[ADSP2100_PGM_OFFSET + (offs & ~3)];
	UINT32 oldword = *base;
	UINT16 temp;
	
	if (!(offs & 2))
	{
		temp = oldword >> 16;
		COMBINE_WORD_MEM(&temp, data);
		oldword = (oldword & 0x0000ffff) | (temp << 16);
	}
	else
	{
		temp = oldword & 0xffff;
		COMBINE_WORD_MEM(&temp, data);
		oldword = (oldword & 0xffff0000) | temp;
	}
	ADSP2100_WRPGM(base, oldword);
}



/*************************************
 *
 *	I/O write latches
 *
 *************************************/

READ_HANDLER( hd68k_sound_reset_r )
{
	atarijsa_reset();
	return 0xffff;
}


WRITE_HANDLER( hd68k_nwr_w )
{
	data = (offset >> 4) & 1;
	
	switch (offset & 0x000e)
	{
		case 0x00:	/* CR2 */
			set_led_status(0, data);
/*			logerror("Write to CR2(%d)\n", data);*/
			break;
		case 0x02:	/* CR1 */
			set_led_status(1, data);
/*			logerror("Write to CR1(%d)\n", data);*/
			break;
		case 0x04:	/* LC1 */
/*			logerror("Write to LC1(%d)\n", data);*/
			break;
		case 0x06:	/* LC2 */
			logerror("Write to LC2(%d)\n", data);
			break;
		case 0x08:	/* ZP1 */
			m68k_zp1 = data;
			break;
		case 0x0a:	/* ZP2 */
			m68k_zp2 = data;
			break;
		case 0x0c:	/* /GSPRES */
			logerror("Write to /GSPRES(%d)\n", data);
			cpu_set_reset_line(gsp_cpu, data ? CLEAR_LINE : ASSERT_LINE);
			break;
		case 0x0e:	/* /MSPRES */
			logerror("Write to /MSPRES(%d)\n", data);
			if (msp_cpu)
				cpu_set_reset_line(msp_cpu, data ? CLEAR_LINE : ASSERT_LINE);
			break;
	}
}



/*************************************
 *
 *	Input reading
 *
 *************************************/

READ_HANDLER( hd68k_adc8_r )
{
	return adc8_data;
}


READ_HANDLER( hd68k_adc12_r )
{
	return adc12_byte ? ((adc12_data >> 8) & 0x0f) : (adc12_data & 0xff);
}



/*************************************
 *
 *	ZRAM I/O
 *
 *************************************/

READ_HANDLER( hd68k_zram_r )
{
	return READ_WORD(&atarigen_eeprom[offset]);
}


WRITE_HANDLER( hd68k_zram_w )
{
	if (m68k_zp1 == 0 && m68k_zp2 == 1)
		COMBINE_WORD_MEM(&atarigen_eeprom[offset], data);
}



/*************************************
 *
 *	ADSP memory-mapped I/O
 *
 *************************************/

READ_HANDLER( hdadsp_special_r )
{
	switch (offset / 2)
	{
		case 0x00:	/* /SIMBUF */
			return READ_WORD(&sim_memory[(adsp_eprom_base + adsp_sim_address++) << 1]);
		
		case 0x01:	/* /SIMLD */
			break;
		
		case 0x02:	/* /SOMO */
			break;
		
		case 0x03:	/* /SOMLD */
			break;
		
		default:
			logerror("%04X:hdadsp_special_r(%04X)\n", cpu_getpreviouspc(), offset / 2);
			break;
	}
	return 0;
}


WRITE_HANDLER( hdadsp_special_w )
{
	switch (offset / 2)
	{
		case 0x01:	/* /SIMCLK */
			adsp_sim_address = data;
			break;
			
		case 0x02:	/* SOMLATCH */
			WRITE_WORD(&som_memory[(m68k_adsp_buffer_bank ^ 1) * 0x4000 + ((adsp_som_address++ & 0x1fff) << 1)], data);
			break;
			
		case 0x03:	/* /SOMCLK */
			adsp_som_address = data;
			break;
			
		case 0x05:	/* /XOUT */
			adsp_xflag = data & 1;
			break;
			
		case 0x06:	/* /GINT */
			logerror("%04X:ADSP signals interrupt\n", cpu_getpreviouspc());
			adsp_irq_state = 1;
			atarigen_update_interrupts();
			break;
			
		case 0x07:	/* /MP */
			adsp_eprom_base = 0x10000 * data;
			break;
			
		default:
			logerror("%04X:hdadsp_special_w(%04X)=%04X\n", cpu_getpreviouspc(), offset / 2, data);
			break;
	}
}


/*************************************
 *
 *	DUART memory handlers
 *
 *************************************/

/* 
									DUART registers

			Read								Write
			----------------------------------	-------------------------------------------
	0x00 = 	Mode Register A (MR1A, MR2A) 		Mode Register A (MR1A, MR2A)
	0x02 =	Status Register A (SRA) 			Clock-Select Register A (CSRA)
	0x04 =	Clock-Select Register A 1 (CSRA) 	Command Register A (CRA)
	0x06 = 	Receiver Buffer A (RBA) 			Transmitter Buffer A (TBA)
	0x08 =	Input Port Change Register (IPCR) 	Auxiliary Control Register (ACR)
	0x0a = 	Interrupt Status Register (ISR) 	Interrupt Mask Register (IMR)
	0x0c = 	Counter Mode: Current MSB of 		Counter/Timer Upper Register (CTUR)
					Counter (CUR) 
	0x0e = 	Counter Mode: Current LSB of 		Counter/Timer Lower Register (CTLR)
					Counter (CLR) 
	0x10 = Mode Register B (MR1B, MR2B) 		Mode Register B (MR1B, MR2B)
	0x12 = Status Register B (SRB) 				Clock-Select Register B (CSRB)
	0x14 = Clock-Select Register B 2 (CSRB) 	Command Register B (CRB)
	0x16 = Receiver Buffer B (RBB) 				Transmitter Buffer B (TBB)
	0x18 = Interrupt-Vector Register (IVR) 		Interrupt-Vector Register (IVR)
	0x1a = Input Port (IP) 						Output Port Configuration Register (OPCR)
	0x1c = Start-Counter Command 3				Output Port Register (OPR): Bit Set Command 3
	0x1e = Stop-Counter Command 3				Output Port Register (OPR): Bit Reset Command 3
*/

INLINE double duart_clock_period(void)
{
	int mode = (duart_write_data[0x04] >> 4) & 7;
	if (mode != 3)
		logerror("DUART: unsupported clock mode %d\n", mode);
	return DUART_CLOCK * 16.0;
}


static void duart_callback(int param)
{
	logerror("DUART timer fired\n");
	if (duart_write_data[0x05] & 0x08)
	{
		logerror("DUART interrupt generated\n");
		duart_read_data[0x05] |= 0x08;
		duart_irq_state = (duart_read_data[0x05] & duart_write_data[0x05]) != 0;
		atarigen_update_interrupts();
	}
	duart_timer = timer_set(duart_clock_period() * 65536.0, 0, duart_callback);
}


READ_HANDLER( hd68k_duart_r )
{
	offset /= 2;
	switch (offset)
	{
		case 0x00:		/* Mode Register A (MR1A, MR2A) */
		case 0x08:		/* Mode Register B (MR1B, MR2B) */
			return (duart_write_data[0x00] << 8) | 0x00ff;
		case 0x01:		/* Status Register A (SRA) */
		case 0x02:		/* Clock-Select Register A 1 (CSRA) */
		case 0x03:		/* Receiver Buffer A (RBA) */
		case 0x04:		/* Input Port Change Register (IPCR) */
		case 0x05:		/* Interrupt Status Register (ISR) */
		case 0x06:		/* Counter Mode: Current MSB of Counter (CUR) */
		case 0x07:		/* Counter Mode: Current LSB of Counter (CLR) */
		case 0x09:		/* Status Register B (SRB) */
		case 0x0a:		/* Clock-Select Register B 2 (CSRB) */
		case 0x0b:		/* Receiver Buffer B (RBB) */
		case 0x0c:		/* Interrupt-Vector Register (IVR) */
		case 0x0d:		/* Input Port (IP) */
			return (duart_read_data[offset] << 8) | 0x00ff;
		case 0x0e:		/* Start-Counter Command 3 */
		{
			int reps = (duart_write_data[0x06] << 8) | duart_write_data[0x07];
			if (duart_timer)
				timer_remove(duart_timer);
			duart_timer = timer_set(duart_clock_period() * (double)reps, 0, duart_callback);
			logerror("DUART timer started (period=%f)\n", duart_clock_period() * (double)reps);
			return 0x00ff;
		}
		case 0x0f:		/* Stop-Counter Command 3 */
			if (duart_timer)
			{
				int reps = timer_timeleft(duart_timer) / duart_clock_period();
				timer_remove(duart_timer);
				duart_read_data[0x06] = reps >> 8;
				duart_read_data[0x07] = reps & 0xff;
				logerror("DUART timer stopped (final count=%04X)\n", reps);
			}
			duart_timer = NULL;
			duart_read_data[0x05] &= ~0x08;
			duart_irq_state = (duart_read_data[0x05] & duart_write_data[0x05]) != 0;
			atarigen_update_interrupts();
			return 0x00ff;
	}
	return 0x00ff;
}


WRITE_HANDLER( hd68k_duart_w )
{
	offset /= 2;
	if (!(data & 0xff000000))
	{
//		int olddata = duart_write_data[offset];
		int newdata = (data >> 8) & 0xff;
		duart_write_data[offset] = newdata;
	
		switch (offset)
		{
			case 0x00:		/* Mode Register A (MR1A, MR2A) */
			case 0x01:		/* Clock-Select Register A (CSRA) */
			case 0x02:		/* Command Register A (CRA) */
			case 0x03:		/* Transmitter Buffer A (TBA) */
			case 0x04:		/* Auxiliary Control Register (ACR) */
			case 0x05:		/* Interrupt Mask Register (IMR) */
			case 0x06:		/* Counter/Timer Upper Register (CTUR) */
			case 0x07:		/* Counter/Timer Lower Register (CTLR) */
			case 0x08:		/* Mode Register B (MR1B, MR2B) */
			case 0x09:		/* Clock-Select Register B (CSRB) */
			case 0x0a:		/* Command Register B (CRB) */
			case 0x0b:		/* Transmitter Buffer B (TBB) */
			case 0x0c:		/* Interrupt-Vector Register (IVR) */
			case 0x0d:		/* Output Port Configuration Register (OPCR) */
				break;
			case 0x0e:		/* Output Port Register (OPR): Bit Set Command 3 */
				duart_output_port |= newdata;
				break;
			case 0x0f:		/* Output Port Register (OPR): Bit Reset Command 3 */
				duart_output_port &= ~newdata;
				break;
		}
		logerror("DUART write %02X @ %02X\n", (data >> 8) & 0xff, offset);
	}
	else
		logerror("Unexpected DUART write %02X @ %02X\n", data, offset);
}



/*************************************
 *
 *	Misc. I/O and latches
 *
 *************************************/

WRITE_HANDLER( hd68k_wr0_write )
{
	if (offset == 2) 		{ //	logerror("SEL1 low\n");
	} else if (offset == 4) { //	logerror("SEL2 low\n");
	} else if (offset == 6) { //	logerror("SEL3 low\n");
	} else if (offset == 8) { //	logerror("SEL4 low\n");
	} else if (offset == 12) { //	logerror("CC1 off\n");
	} else if (offset == 14) { //	logerror("CC2 off\n");
	} else if (offset == 18) { //	logerror("SEL1 high\n");
	} else if (offset == 20) { //	logerror("SEL2 high\n");
	} else if (offset == 22) { //	logerror("SEL3 high\n");
	} else if (offset == 24) { //	logerror("SEL4 high\n");
	} else if (offset == 28) { //	logerror("CC1 on\n");
	} else if (offset == 30) { //	logerror("CC2 on\n");
	} else { 				logerror("/WR1(%04X)=%02X\n", offset, data);
	}
}


WRITE_HANDLER( hd68k_wr1_write )
{
	if (offset == 0) { //	logerror("Shifter Interface Latch = %02X\n", data);
	} else { 				logerror("/WR1(%04X)=%02X\n", offset, data);
	}
}


WRITE_HANDLER( hd68k_wr2_write )
{
	if (offset == 0) { //	logerror("Steering Wheel Latch = %02X\n", data);
	} else { 				logerror("/WR2(%04X)=%02X\n", offset, data);
	}
}


WRITE_HANDLER( hd68k_adc_control_w )
{
	COMBINE_WORD_MEM(&adc_control, data);
	
	/* handle a write to the 8-bit ADC address select */
	if (adc_control & 0x08)
	{
		adc8_select = adc_control & 0x07;
		adc8_data = readinputport(2 + adc8_select);
//		logerror("8-bit ADC select %d\n", adc8_select, data);
	}
	
	/* handle a write to the 12-bit ADC address select */
	if (adc_control & 0x40)
	{
		adc12_select = (adc_control >> 4) & 0x03;
		adc12_data = readinputport(10 + adc12_select) << 4;
//		logerror("12-bit ADC select %d\n", adc12_select);
	}
	adc12_byte = (adc_control >> 7) & 1;
}



/*************************************
 *
 *	Race Drivin' RAM handlers
 *
 *************************************/

READ_HANDLER( hddsk_ram_r )
{
	return READ_WORD(&hddsk_ram[offset]);
}


WRITE_HANDLER( hddsk_ram_w )
{
	COMBINE_WORD_MEM(&hddsk_ram[offset], data);
}


READ_HANDLER( hddsk_zram_r )
{
	return READ_WORD(&hddsk_zram[offset]);
}


WRITE_HANDLER( hddsk_zram_w )
{
	COMBINE_WORD_MEM(&hddsk_zram[offset], data);
}


READ_HANDLER( hddsk_rom_r )
{
	return READ_WORD(&hddsk_rom[offset]);
}



/*************************************
 *
 *	Race Drivin' slapstic handling
 *
 *************************************/

WRITE_HANDLER( racedriv_68k_slapstic_w )
{
	slapstic_tweak((offset & 0x7fff) / 2);
}


READ_HANDLER( racedriv_68k_slapstic_r )
{
	int bank = slapstic_tweak((offset & 0x7fff) / 2) * 0x8000;
	return READ_WORD(&hd68k_slapstic_base[bank + (offset & 0x7fff)]);
}



/*************************************
 *
 *	Race Drivin' ASIC 65 handling
 *
 *	(ASIC 65 is actually a TMS32C015
 *	DSP chip clocked at 20MHz)
 *
 *************************************/

static UINT16 asic65_command;
static UINT16 asic65_param[32];
static UINT8  asic65_param_index;
static UINT16 asic65_result[32];
static UINT8  asic65_result_index;

static FILE * asic65_log;

WRITE_HANDLER( racedriv_asic65_w )
{
	if (!asic65_log) asic65_log = fopen("asic65.log", "w");

	/* parameters go to offset 0 */
	if (offset == 0)
	{
		if (asic65_log) fprintf(asic65_log, " W=%04X", data);
		
		asic65_param[asic65_param_index++] = data;
		if (asic65_param_index >= 32)
			asic65_param_index = 32;
	}
	
	/* commands go to offset 2 */
	else
	{
		if (asic65_log) fprintf(asic65_log, "\n(%06X) %04X:", cpu_getpreviouspc(), data);
		
		asic65_command = data;
		asic65_result_index = asic65_param_index = 0;
	}

	/* update results */
	switch (asic65_command)
	{
		case 0x01:	/* reflect data */
			if (asic65_param_index >= 1)
			{
				asic65_result[0] = asic65_param[0];
				asic65_result_index = asic65_param_index = 0;
			}
			break;
		
		case 0x02:	/* compute checksum (should be XX27) */
			asic65_result[0] = 0x0027;
			asic65_result_index = asic65_param_index = 0;
			break;
			
		case 0x03:	/* get version (returns 1.3) */
			asic65_result[0] = 0x0013;
			asic65_result_index = asic65_param_index = 0;
			break;
			
		case 0x04:	/* internal RAM test (result should be 0) */
			asic65_result[0] = 0;
			asic65_result_index = asic65_param_index = 0;
			break;
			
		case 0x10:	/* matrix multiply */
			if (asic65_param_index >= 9+6)
			{
				INT64 element, result;
				
				element = (INT32)((asic65_param[9] << 16) | asic65_param[10]);
				result = element * (INT16)asic65_param[0] +
						 element * (INT16)asic65_param[1] +
						 element * (INT16)asic65_param[2];
				result >>= 14;
				asic65_result[0] = result >> 16;
				asic65_result[1] = result & 0xffff;
				
				element = (INT32)((asic65_param[11] << 16) | asic65_param[12]);
				result = element * (INT16)asic65_param[3] +
						 element * (INT16)asic65_param[4] +
						 element * (INT16)asic65_param[5];
				result >>= 14;
				asic65_result[2] = result >> 16;
				asic65_result[3] = result & 0xffff;
				
				element = (INT32)((asic65_param[13] << 16) | asic65_param[14]);
				result = element * (INT16)asic65_param[6] +
						 element * (INT16)asic65_param[7] +
						 element * (INT16)asic65_param[8];
				result >>= 14;
				asic65_result[4] = result >> 16;
				asic65_result[5] = result & 0xffff;
			}
			break;
			
		case 0x14:	/* ??? */
			if (asic65_param_index >= 1)
			{
				asic65_result[0] = asic65_param[0];
				asic65_result_index = asic65_param_index = 0;
			}
			break;
			
		case 0x15:	/* ??? */
			if (asic65_param_index >= 1)
			{
				asic65_result[0] = asic65_param[0];
				asic65_result_index = asic65_param_index = 0;
			}
			break;
	}
}


READ_HANDLER( racedriv_asic65_r )
{
	int result;
	
	if (!asic65_log) asic65_log = fopen("asic65.log", "w");
	if (asic65_log) fprintf(asic65_log, " (R=%04X)", asic65_result[asic65_result_index]);
	
	/* return the next result */
	result = asic65_result[asic65_result_index++];
	if (asic65_result_index >= 32)
		asic65_result_index = 32;
	return result;
}


READ_HANDLER( racedriv_asic65_io_r )
{
	/* indicate that we always are ready to accept data and always ready to send */
	return 0x4000;
}



/*************************************
 *
 *	Race Drivin' ASIC 61 handling
 *
 *	(ASIC 61 is actually an AT&T/Lucent
 *	DSP32 chip clocked at 40MHz)
 *
 *************************************/

static UINT32 asic61_addr;
static FILE *asic61_log;

/* remove these when we get a real CPU to work with */
static UINT16 *asic61_ram_low;
static UINT16 *asic61_ram_mid;
static UINT16 *asic61_ram_high;

static WRITE_HANDLER( asic61_w )
{
	if (!asic61_ram_low)
	{
		asic61_ram_low = malloc(2 * 0x1000);
		asic61_ram_mid = malloc(2 * 0x20000);
		asic61_ram_high = malloc(2 * 0x00800);
		if (!asic61_ram_low || !asic61_ram_mid || !asic61_ram_high)
			return;
	}
	if (offset >= 0x000000 && offset < 0x001000)
		asic61_ram_low[offset & 0xfff] = data;
	else if (offset >= 0x600000 && offset < 0x620000)
		asic61_ram_mid[offset & 0x1ffff] = data;
	else if (offset >= 0xfff800 && offset < 0x1000000)
		asic61_ram_high[offset & 0x7ff] = data;
	else
		logerror("Unexpected ASIC61 write to %06X\n", offset);
}


static READ_HANDLER( asic61_r )
{
	if (!asic61_ram_low)
	{
		asic61_ram_low = malloc(2 * 0x1000);
		asic61_ram_mid = malloc(2 * 0x20000);
		asic61_ram_high = malloc(2 * 0x00800);
		if (!asic61_ram_low || !asic61_ram_mid || !asic61_ram_high)
			return 0;
	}
	if (offset >= 0x000000 && offset < 0x001000)
		return asic61_ram_low[offset & 0xfff];
	else if (offset >= 0x600000 && offset < 0x620000)
		return asic61_ram_mid[offset & 0x1ffff];
	else if (offset >= 0xfff800 && offset < 0x1000000)
		return asic61_ram_high[offset & 0x7ff];
	else
	{
		logerror("Unexpected ASIC61 read from %06X\n", offset);
		if (asic61_log) fprintf(asic61_log, "Unexpected ASIC61 read from %06X\n", offset);
	}
	return 0;
}


WRITE_HANDLER( racedriv_asic61_w )
{
	if (!asic61_log) asic61_log = fopen("asic61.log", "w");
	
	switch (offset)
	{
		case 0x00:
//			if (asic61_log) fprintf(asic61_log, "%06X:mem addr lo = %04X\n", cpu_getpreviouspc(), data);
			asic61_addr = (asic61_addr & 0x00ff0000) | (data & 0xfffe);
			break;
		
		case 0x04:
//			if (asic61_log) fprintf(asic61_log, "%06X:mem W @ %08X=%04X\n", cpu_getpreviouspc(), asic61_addr, data);
			asic61_w(asic61_addr, data);
			asic61_addr = (asic61_addr & 0xffff0000) | (++asic61_addr & 0x0000ffff);
			break;
		
		case 0x16:
//			if (asic61_log) fprintf(asic61_log, "%06X:mem addr hi = %04X\n", cpu_getpreviouspc(), data);
			asic61_addr = ((data << 16) & 0x00ff0000) | (asic61_addr & 0xfffe);
			break;
		
		default:
			if (asic61_log) fprintf(asic61_log, "%06X:W@%02X = %04X\n", cpu_getpreviouspc(), offset, data);
			break;
	}
}


READ_HANDLER( racedriv_asic61_r )
{
	UINT32 orig_addr = asic61_addr;
	
	if (!asic61_log) asic61_log = fopen("asic61.log", "w");
	
	switch (offset)
	{
		case 0x00:
//			if (asic61_log) fprintf(asic61_log, "%06X:read mem addr lo = %04X\n", cpu_getpreviouspc(), asic61_addr & 0xffff);
			return (asic61_addr & 0xfffe) | 1;
			
		case 0x04:
//			if (asic61_log) fprintf(asic61_log, "%06X:mem R @ %08X=%04X\n", cpu_getpreviouspc(), asic61_addr, asic61_r(asic61_addr));
			asic61_addr = (asic61_addr & 0xffff0000) | (++asic61_addr & 0x0000ffff);
			
			/* special case: reading from 0x613e08 seems to be a flag */
			if (orig_addr == 0x613e08 && asic61_r(orig_addr) == 1)
				return 0;
			return asic61_r(orig_addr);
		
		default:
//			if (asic61_log) fprintf(asic61_log, "%06X:R@%02X\n", cpu_getpreviouspc(), offset);
			break;
	}
	return 0;
}



/*************************************
 *
 *	Steel Talons slapstic handling
 *
 *************************************/

static READ_HANDLER( steeltal_slapstic_tweak )
{
	static int last_offset;
	static int bank = 0;
	
	if (last_offset == 0)
	{
		switch (offset)
		{
			case 0x78e8:
				bank = 0;
				break;
			case 0x6ca4:
				bank = 1;
				break;
			case 0x15ea:
				bank = 2;
				break;
			case 0x6b28:
				bank = 3;
				break;
		}
	}
	last_offset = offset;
	return bank;
}


WRITE_HANDLER( steeltal_68k_slapstic_w )
{
	steeltal_slapstic_tweak((offset & 0x7fff) / 2);
}


READ_HANDLER( steeltal_68k_slapstic_r )
{
	int bank = steeltal_slapstic_tweak(offset / 2) * 0x8000;
	return READ_WORD(&hd68k_slapstic_base[bank + (offset & 0x7fff)]);
}



/*************************************
 *
 *	ADSP Optimizations
 *
 *************************************/

#define READ_DATA(x) 	(READ_WORD(&adsp_memory[ADSP2100_DATA_OFFSET + (((x) & 0x3fff) << 1)]))
#define READ_PGM(x)		(READ_WORD(&adsp_memory[ADSP2100_PGM_OFFSET + 1 + (((x) & 0x3fff) << 2)]))

READ_HANDLER( hdadsp_speedup_r )
{
	int data = READ_DATA(0x1fff);

	if (data == 0xffff && cpu_get_pc() <= 0x14 && cpu_getactivecpu() == adsp_cpu)
		cpu_spinuntil_trigger(554433);

	return data;
}


READ_HANDLER( hdadsp_speedup2_r )
{
	UINT32 i5 = READ_DATA(0x0958);

	int pc = cpu_get_pc();
	if (pc >= 0x07c7 && pc <= 0x7d5)
	{
		UINT32 i4 = cpu_get_reg(ADSP2100_I4);
		INT32 ay = (READ_PGM(i4++) << 16) | (UINT16)cpu_get_reg(ADSP2100_AY1);
		INT32 sr = (cpu_get_reg(ADSP2100_SR1) << 16) | (UINT16)cpu_get_reg(ADSP2100_SR0);
		INT32 last_ay;
		
		if (ay < sr)
		{
			do
			{
				last_ay = ay;
				i4 = (UINT16)READ_PGM(i4);
				i5++;
				ay = (UINT16)READ_PGM(i4) | (READ_PGM(i4 + 1) << 16);
				i4 += 2;
			} while (sr >= last_ay);
			cpu_set_reg(ADSP2100_I4, i4);
			cpu_set_reg(ADSP2100_PC, pc + 0x7d0 - 0x7c7);
		}
		else
		{
			i4++;
			do
			{
				last_ay = ay;
				i4 = (UINT16)READ_PGM(i4);
				i5++;
				ay = (UINT16)READ_PGM(i4) | (READ_PGM(i4 + 1) << 16);
				i4 += 3;
			} while (last_ay >= sr);
			cpu_set_reg(ADSP2100_I4, i4);
			cpu_set_reg(ADSP2100_AR, i4 - 3);
			cpu_set_reg(ADSP2100_M4, 2);
			cpu_set_reg(ADSP2100_PC, pc + 0x7ec - 0x7c7);
		}
	}

	return i5;
}


WRITE_HANDLER( hdadsp_speedup2_w )
{
	int se = (INT8)cpu_get_reg(ADSP2100_SE);
	UINT32 pc = cpu_get_pc();
	INT16 *matrix;
	INT16 *vector;
	INT16 *trans;
	INT16 *result;
	INT64 mr;
	int count;
	
	COMBINE_WORD_MEM(&adsp_memory[ADSP2100_DATA_OFFSET + (0x0033 << 1)], data);
	if (pc != hdadsp_speedup_pc)
		return;
	adsp2100_icount -= 8;

	matrix = (INT16 *)&adsp_memory[ADSP2100_PGM_OFFSET + 1 + (0x1010 << 2)];
	trans  = (INT16 *)&adsp_memory[ADSP2100_DATA_OFFSET + (0x000c << 1)];
	result = (INT16 *)&adsp_memory[ADSP2100_DATA_OFFSET + (0x010d << 1)];
	vector = (INT16 *)&sim_memory[(adsp_eprom_base + adsp_sim_address) << 1];

	count = READ_DATA(0x0032);
	adsp2100_icount -= (9 + 6*3) * count;
	while (count--)
	{
		INT16 temp[3];
		
		if (se < 0)
		{
			temp[0] = vector[0] >> -se;
			temp[1] = vector[1] >> -se;
			temp[2] = vector[2] >> -se;
		}
		else
		{
			temp[0] = vector[0] << se;
			temp[1] = vector[1] << se;
			temp[2] = vector[2] << se;
		}

		mr = temp[0] * matrix[0*2];
		mr += temp[1] * matrix[1*2];
		mr += temp[2] * matrix[2*2];
		*result++ = ((INT32)mr >> (16 - 1)) + trans[0];

		mr = temp[0] * matrix[3*2];
		mr += temp[1] * matrix[4*2];
		mr += temp[2] * matrix[5*2];
		*result++ = ((INT32)mr >> (16 - 1)) + trans[1];

		mr = temp[0] * matrix[6*2];
		mr += temp[1] * matrix[7*2];
		mr += temp[2] * matrix[8*2];
		*result++ = ((INT32)mr >> (16 - 1)) + trans[2];
		
		vector += 3;
	}
	
	count = READ_DATA(0x08da);
	adsp2100_icount -= (9 + 9*3) * count;
	while (count--)
	{
		mr = vector[0] * matrix[0*2];
		mr += vector[1] * matrix[1*2];
		mr += vector[2] * matrix[2*2];
		*result++ = ((INT32)(mr << 1) + 0x4000) >> 15;

		mr = vector[0] * matrix[3*2];
		mr += vector[1] * matrix[4*2];
		mr += vector[2] * matrix[5*2];
		*result++ = ((INT32)(mr << 1) + 0x4000) >> 15;

		mr = vector[0] * matrix[6*2];
		mr += vector[1] * matrix[7*2];
		mr += vector[2] * matrix[8*2];
		*result++ = ((INT32)(mr << 1) + 0x4000) >> 15;
		
		vector += 3;
	}
	cpu_set_reg(ADSP2100_SI, *vector++);
	adsp_sim_address = (((UINT8 *)vector - sim_memory) >> 1) - adsp_eprom_base;
	
	cpu_set_reg(ADSP2100_PC, pc + 0x165 - 0x139);
}

	

/*************************************
 *
 *	GSP Optimizations
 *
 *************************************/

READ_HANDLER( hdgsp_speedup_r )
{
	int result = READ_WORD(&hdgsp_speedup_addr[0][offset]);
	
	if (offset != 0 || result == 0xffff)
		return result;
	
	if (cpu_get_pc() == hdgsp_speedup_pc && READ_WORD(hdgsp_speedup_addr[1]) != 0xffff && cpu_getactivecpu() == gsp_cpu)
		cpu_spinuntil_int();
	
	return result;
}


WRITE_HANDLER( hdgsp_speedup1_w )
{
	int oldword = READ_WORD(&hdgsp_speedup_addr[0][offset]);
	int newword = COMBINE_WORD(oldword, data);
	WRITE_WORD(&hdgsp_speedup_addr[0][offset], newword);
	
	if (offset == 0 && newword == 0xffff)
		cpu_trigger(-2000 + gsp_cpu);
}


WRITE_HANDLER( hdgsp_speedup2_w )
{
	int oldword = READ_WORD(&hdgsp_speedup_addr[1][offset]);
	int newword = COMBINE_WORD(oldword, data);
	WRITE_WORD(&hdgsp_speedup_addr[1][offset], newword);
	
	if (offset == 0 && newword == 0xffff)
		cpu_trigger(-2000 + gsp_cpu);
}



/*************************************
 *
 *	MSP Optimizations
 *
 *************************************/

READ_HANDLER( hdmsp_speedup_r )
{
	int result = READ_WORD(&hdmsp_speedup_addr[offset]);
	
	if (offset != 0 || result != 0)
		return result;
	
	if (cpu_get_pc() == hdmsp_speedup_pc && cpu_getactivecpu() == msp_cpu)
		cpu_spinuntil_int();

	return result;
}


WRITE_HANDLER( hdmsp_speedup_w )
{
	int oldword = READ_WORD(&hdmsp_speedup_addr[offset]);
	int newword = COMBINE_WORD(oldword, data);
	WRITE_WORD(&hdmsp_speedup_addr[offset], newword);
	
	if (offset == 0 && newword != 0)
		cpu_trigger(-2000 + msp_cpu);
}
