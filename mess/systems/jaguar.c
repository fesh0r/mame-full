/***************************************************************************

	Atari Jaguar

	Nathan Woods
	based on MAME cojag driver by Aaron Giles

****************************************************************************

	Memory map (TBA)

	========================================================================
	MAIN CPU
	========================================================================

	------------------------------------------------------------
	000000-3FFFFF   R/W   xxxxxxxx xxxxxxxx   DRAM 0
	400000-7FFFFF   R/W   xxxxxxxx xxxxxxxx   DRAM 1
	F00000-F000FF   R/W   xxxxxxxx xxxxxxxx   Tom Internal Registers
	F00400-F005FF   R/W   xxxxxxxx xxxxxxxx   CLUT - color lookup table A
	F00600-F007FF   R/W   xxxxxxxx xxxxxxxx   CLUT - color lookup table B
	F00800-F00D9F   R/W   xxxxxxxx xxxxxxxx   LBUF - line buffer A
	F01000-F0159F   R/W   xxxxxxxx xxxxxxxx   LBUF - line buffer B
	F01800-F01D9F   R/W   xxxxxxxx xxxxxxxx   LBUF - line buffer currently selected
	F02000-F021FF   R/W   xxxxxxxx xxxxxxxx   GPU control registers
	F02200-F022FF   R/W   xxxxxxxx xxxxxxxx   Blitter registers
	F03000-F03FFF   R/W   xxxxxxxx xxxxxxxx   Local GPU RAM
	F08800-F08D9F   R/W   xxxxxxxx xxxxxxxx   LBUF - 32-bit access to line buffer A
	F09000-F0959F   R/W   xxxxxxxx xxxxxxxx   LBUF - 32-bit access to line buffer B
	F09800-F09D9F   R/W   xxxxxxxx xxxxxxxx   LBUF - 32-bit access to line buffer currently selected
	F0B000-F0BFFF   R/W   xxxxxxxx xxxxxxxx   32-bit access to local GPU RAM
	F10000-F13FFF   R/W   xxxxxxxx xxxxxxxx   Jerry
	F14000-F17FFF   R/W   xxxxxxxx xxxxxxxx   Joysticks and GPIO0-5
	F18000-F1AFFF   R/W   xxxxxxxx xxxxxxxx   Jerry DSP
	F1B000-F1CFFF   R/W   xxxxxxxx xxxxxxxx   Local DSP RAM
	F1D000-F1DFFF   R     xxxxxxxx xxxxxxxx   Wavetable ROM
	------------------------------------------------------------

***************************************************************************/


#include "driver.h"
#include "cpu/mips/r3000.h"
#include "cpu/jaguar/jaguar.h"
#include "jaguar.h"
#include "devices/cartslot.h"



/*************************************
 *
 *	Global variables
 *
 *************************************/

data32_t *jaguar_shared_ram;
data32_t *jaguar_gpu_ram;
data32_t *jaguar_gpu_clut;
data32_t *jaguar_dsp_ram;
data32_t *jaguar_wave_rom;
UINT8 cojag_is_r3000 = FALSE;



/*************************************
 *
 *	Local variables
 *
 *************************************/

static data32_t misc_control_data;
static UINT8 eeprom_enable;

static data32_t *cart_base;
static size_t cart_size;

static data32_t *rom_base;
static size_t rom_size;



static int jaguar_irq_callback(int level)
{
	return (level == 6) ? 0x40 : -1;
}

/*************************************
 *
 *	Machine init
 *
 *************************************/

static MACHINE_INIT( jaguar )
{
	cpu_set_irq_callback(0, jaguar_irq_callback);

	*((UINT32 *) jaguar_gpu_ram) = 0x3d0dead;

	memset(jaguar_shared_ram, 0, 0x400000);
	memcpy(jaguar_shared_ram, rom_base, 0x10);

	/* set up main CPU RAM/ROM banks */
	cpu_setbank(3, jaguar_gpu_ram);
	cpu_setbank(4, jaguar_shared_ram);

	/* set up DSP RAM/ROM banks */
	cpu_setbank(10, jaguar_shared_ram);
	cpu_setbank(11, jaguar_gpu_clut);
	cpu_setbank(12, jaguar_gpu_ram);
	cpu_setbank(13, jaguar_dsp_ram);
	cpu_setbank(14, jaguar_shared_ram);
	cpu_setbank(15, cart_base);
	cpu_setbank(16, rom_base);

	/* clear any spinuntil stuff */
	jaguar_gpu_resume();
	jaguar_dsp_resume();

	/* halt the CPUs */
	jaguargpu_ctrl_w(1, G_CTRL, 0, 0);
	jaguardsp_ctrl_w(2, D_CTRL, 0, 0);

	/* init the sound system */
	cojag_sound_reset();
}



/*************************************
 *
 *	Misc. control bits
 *
 *************************************/

static READ32_HANDLER( misc_control_r )
{
	/*	D7    = board reset (low)
		D6    = audio must & reset (high)
		D5    = volume control data (invert on write)
		D4    = volume control clock
	 	D0    = shared memory select (0=XBUS) */

	return misc_control_data ^ 0x20;
}


static WRITE32_HANDLER( misc_control_w )
{
	logerror("%08X:misc_control_w(%02X)\n", activecpu_get_previouspc(), data);

	/*	D7    = board reset (low)
		D6    = audio must & reset (high)
		D5    = volume control data (invert on write)
		D4    = volume control clock
	 	D0    = shared memory select (0=XBUS) */

	/* handle resetting the DSPs */
	if (!(data & 0x80))
	{
		/* clear any spinuntil stuff */
		jaguar_gpu_resume();
		jaguar_dsp_resume();

		/* halt the CPUs */
		jaguargpu_ctrl_w(1, G_CTRL, 0, 0);
		jaguardsp_ctrl_w(2, D_CTRL, 0, 0);
	}

	COMBINE_DATA(&misc_control_data);
}



/*************************************
 *
 *	32-bit access to the GPU
 *
 *************************************/

static READ32_HANDLER( gpuctrl_r )
{
	return jaguargpu_ctrl_r(1, offset);
}


static WRITE32_HANDLER( gpuctrl_w )
{
	jaguargpu_ctrl_w(1, offset, data, mem_mask);
}



/*************************************
 *
 *	32-bit access to the DSP
 *
 *************************************/

static READ32_HANDLER( dspctrl_r )
{
	return jaguardsp_ctrl_r(2, offset);
}


static WRITE32_HANDLER( dspctrl_w )
{
	jaguardsp_ctrl_w(2, offset, data, mem_mask);
}



/*************************************
 *
 *	Input ports
 *
 *	Information from "The Jaguar Underground Documentation"
 *	by Klaus and Nat!
 *
 *************************************/

static READ32_HANDLER( joystick_r )
{
	/*
	 *   16        12        8         4         0
	 *   +---------+---------+---------^---------+
	 *   |  pad 1  |  pad 0  |      unused       |
	 *   +---------+---------+-------------------+
	 *     15...12   11...8          7...0
	 *
	 *   Reading this register gives you the output of the selected columns
	 *   of the pads.
	 *   The buttons pressed will appear as cleared bits. 
	 *   See the description of the column addressing to map the bits 
	 *   to the buttons.
	 */
	return 0;
}

static WRITE32_HANDLER( joystick_w )
{
	/*
	 *   16        12         8         4         0
	 *   +-+-------^------+--+---------+---------+
	 *   |r|    unused    |mu|  col 1  |  col 0  |  
	 *   +-+--------------+--+---------+---------+
	 *    15                8   7...4     3...0
	 *
	 *   col 0:   column control of joypad 0 
	 *
	 *      Here you select which column of the joypad to poll. 
	 *      The columns are:
	 *
	 *                Joystick       Joybut 
	 *      col_bit|11 10  9  8     1    0  
	 *      -------+--+--+--+--    ---+------
	 *         0   | R  L  D  U     A  PAUSE       (RLDU = Joypad directions)
	 *         1   | *  7  4  1     B       
	 *         2   | 2  5  8  0     C       
	 *         3   | 3  6  9  #   OPTION
	 *
	 *      You select a column my clearing the appropriate bit and setting
	 *      all the other "column" bits. 
	 *
	 *
	 *   col1:    column control of joypad 1
	 *
	 *      This is pretty much the same as for joypad EXCEPT that the
	 *      column addressing is reversed (strange!!)
	 *            
	 *                Joystick      Joybut 
	 *      col_bit|15 14 13 12     3    2
	 *      -------+--+--+--+--    ---+------
	 *         4   | 3  6  9  #   OPTION
	 *         5   | 2  5  8  0     C
	 *         6   | *  7  4  1     B
	 *         7   | R  L  D  U     A  PAUSE     (RLDU = Joypad directions)
	 *
	 *   mute (mu):   sound control
	 *
	 *      You can turn off the sound by clearing this bit.
	 *
	 *   read enable (r):  
	 *
	 *      Set this bit to read from the joysticks, clear it to write
	 *      to them.
	 */
}



/*************************************
 *
 *	Output ports
 *
 *************************************/

static WRITE32_HANDLER( latch_w )
{
	logerror("%08X:latch_w(%X)\n", activecpu_get_previouspc(), data);
}



/*************************************
 *
 *	EEPROM access
 *
 *************************************/

static READ32_HANDLER( eeprom_data_r )
{
	return ((UINT32 *)generic_nvram)[offset] | 0x00ffffff;
}


static WRITE32_HANDLER( eeprom_enable_w )
{
	eeprom_enable = 1;
}


static WRITE32_HANDLER( eeprom_data_w )
{
//	if (eeprom_enable)
	{
		((UINT32 *)generic_nvram)[offset] = data & 0xff000000;
	}
//	else
//		logerror("%08X:error writing to disabled EEPROM\n", activecpu_get_previouspc());
	eeprom_enable = 0;
}



/*************************************
 *
 *	GPU synchronization & speedup
 *
 *************************************/

/*
	Explanation:

	The GPU generally sits in a tight loop waiting for the main CPU to store
	a jump address into a specific memory location. This speedup is designed
	to catch that loop, which looks like this:

		load    (r28),r21
		jump    (r21)
		nop

	When nothing is pending, the GPU keeps the address of the load instruction
	at (r28) so that it loops back on itself. When the main CPU wants to execute
	a command, it stores an alternate address to (r28).

	Even if we don't optimize this case, we do need to detect when a command
	is written to the GPU in order to improve synchronization until the GPU
	has finished. To do this, we start a temporary high frequency timer and
	run it until we get back to the spin loop.
*/

static data32_t *gpu_jump_address;
static UINT8 gpu_command_pending;
static data32_t gpu_spin_pc;

static void gpu_sync_timer(int param)
{
	/* if a command is still pending, and we haven't maxed out our timer, set a new one */
	if (gpu_command_pending && param < 1000)
		timer_set(TIME_IN_USEC(50), ++param, gpu_sync_timer);
}


static WRITE32_HANDLER( gpu_jump_w )
{
	/* update the data in memory */
	COMBINE_DATA(gpu_jump_address);
	logerror("%08X:GPU jump address = %08X\n", activecpu_get_previouspc(), *gpu_jump_address);

	/* if the GPU is suspended, release it now */
	jaguar_gpu_resume();

	/* start the sync timer going, and note that there is a command pending */
	timer_set(TIME_NOW, 0, gpu_sync_timer);
	gpu_command_pending = 1;
}


static READ32_HANDLER( gpu_jump_r )
{
	/* if the current GPU command is just pointing back to the spin loop, and */
	/* we're reading it from the spin loop, we can optimize */
	if (*gpu_jump_address == gpu_spin_pc && activecpu_get_previouspc() == gpu_spin_pc)
	{
		/* no command is pending */
		gpu_command_pending = 0;
	}

	/* return the current value */
	return *gpu_jump_address;
}


/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/


static MEMORY_READ32_START( jaguar_readmem )
	{ 0x000000, 0x1fffff, MRA32_RAM },
	{ 0x200000, 0x3fffff, MRA32_BANK4 },		/* mirror */
	{ 0x800000, 0xdfffff, MRA32_ROM },
	{ 0xe00000, 0xe1ffff, MRA32_ROM },
	{ 0xf00000, 0xf003ff, jaguar_tom_regs32_r },
	{ 0xf00400, 0xf007ff, MRA32_RAM },
	{ 0xf02100, 0xf021ff, gpuctrl_r },
	{ 0xf02200, 0xf022ff, jaguar_blitter_r },
	{ 0xf03000, 0xf03fff, MRA32_RAM },
	{ 0xf10000, 0xf103ff, jaguar_jerry_regs32_r },
	{ 0xf14000, 0xf14003, joystick_r },
	{ 0xf16000, 0xf1600b, cojag_gun_input_r },
	{ 0xf1a100, 0xf1a13f, dspctrl_r },
	{ 0xf1a140, 0xf1a17f, jaguar_serial_r },
	{ 0xf1b000, 0xf1cfff, MRA32_RAM },
MEMORY_END


static MEMORY_WRITE32_START( jaguar_writemem )
	{ 0x000000, 0x1fffff, MWA32_RAM, &jaguar_shared_ram },
	{ 0x200000, 0x3fffff, MWA32_BANK4 },		/* mirror */
	{ 0x800000, 0xdfffff, MWA32_ROM, &cart_base, &cart_size },
	{ 0xe00000, 0xe1ffff, MWA32_ROM, &rom_base, &rom_size },
	{ 0xf00000, 0xf003ff, jaguar_tom_regs32_w },
	{ 0xf00400, 0xf007ff, MWA32_RAM, &jaguar_gpu_clut },
	{ 0xf01400, 0xf01403, joystick_w },
	{ 0xf02100, 0xf021ff, gpuctrl_w },
	{ 0xf02200, 0xf022ff, jaguar_blitter_w },
	{ 0xf03000, 0xf03fff, MWA32_RAM, &jaguar_gpu_ram },
	{ 0xf0b000, 0xf0bfff, MWA32_BANK3 },
	{ 0xf10000, 0xf103ff, jaguar_jerry_regs32_w },
//	{ 0xf17800, 0xf17803, latch_w },	// GPI04
	{ 0xf1a100, 0xf1a13f, dspctrl_w },
	{ 0xf1a140, 0xf1a17f, jaguar_serial_w },
	{ 0xf1b000, 0xf1cfff, MWA32_RAM, &jaguar_dsp_ram },
MEMORY_END



/*************************************
 *
 *	GPU memory handlers
 *
 *************************************/

static MEMORY_READ32_START( gpu_readmem )
	{ 0x000000, 0x1fffff, MRA32_BANK10 },
	{ 0x200000, 0x3fffff, MRA32_BANK14 },
	{ 0x800000, 0xdfffff, MRA32_BANK15 },
	{ 0xe00000, 0xe1ffff, MRA32_BANK16 },
	{ 0xf00000, 0xf003ff, jaguar_tom_regs32_r },
	{ 0xf00400, 0xf007ff, MRA32_BANK11 },
	{ 0xf02100, 0xf021ff, gpuctrl_r },
	{ 0xf02200, 0xf022ff, jaguar_blitter_r },
	{ 0xf03000, 0xf03fff, MRA32_BANK12 },
	{ 0xf10000, 0xf103ff, jaguar_jerry_regs32_r },
MEMORY_END


static MEMORY_WRITE32_START( gpu_writemem )
	{ 0x000000, 0x1fffff, MWA32_BANK10 },
	{ 0x200000, 0x3fffff, MWA32_BANK14 },
	{ 0x800000, 0xdfffff, MWA32_ROM },
	{ 0xe00000, 0xe1ffff, MWA32_ROM },
	{ 0xf00000, 0xf003ff, jaguar_tom_regs32_w },
	{ 0xf00400, 0xf007ff, MWA32_BANK11 },
	{ 0xf02100, 0xf021ff, gpuctrl_w },
	{ 0xf02200, 0xf022ff, jaguar_blitter_w },
	{ 0xf03000, 0xf03fff, MWA32_BANK12 },
	{ 0xf10000, 0xf103ff, jaguar_jerry_regs32_w },
MEMORY_END



/*************************************
 *
 *	DSP memory handlers
 *
 *************************************/

static MEMORY_READ32_START( dsp_readmem )
	{ 0x000000, 0x1fffff, MRA32_BANK10 },
	{ 0x200000, 0x3fffff, MRA32_BANK14 },
	{ 0x800000, 0xdfffff, MRA32_BANK15 },
	{ 0xe00000, 0xe1ffff, MRA32_BANK16 },
	{ 0xf10000, 0xf103ff, jaguar_jerry_regs32_r },
	{ 0xf1a100, 0xf1a13f, dspctrl_r },
	{ 0xf1a140, 0xf1a17f, jaguar_serial_r },
	{ 0xf1b000, 0xf1cfff, MRA32_BANK13 },
	{ 0xf1d000, 0xf1dfff, MRA32_ROM },
MEMORY_END


static MEMORY_WRITE32_START( dsp_writemem )
	{ 0x000000, 0x1fffff, MWA32_BANK10 },
	{ 0x200000, 0x3fffff, MWA32_BANK14 },
	{ 0x800000, 0xdfffff, MWA32_ROM },
	{ 0xe00000, 0xe1ffff, MWA32_ROM },
	{ 0xf10000, 0xf103ff, jaguar_jerry_regs32_w },
	{ 0xf1a100, 0xf1a13f, dspctrl_w },
	{ 0xf1a140, 0xf1a17f, jaguar_serial_w },
	{ 0xf1b000, 0xf1cfff, MWA32_BANK13 },
	{ 0xf1d000, 0xf1dfff, MWA32_ROM, &jaguar_wave_rom },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( jaguar )
INPUT_PORTS_END


/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct DACinterface dac_interface =
{
	2,
	{ MIXER(100, MIXER_PAN_LEFT), MIXER(100, MIXER_PAN_RIGHT) }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/


static struct jaguar_config gpu_config =
{
	jaguar_gpu_cpu_int
};


static struct jaguar_config dsp_config =
{
	jaguar_dsp_cpu_int
};

MACHINE_DRIVER_START( jaguar )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020, 13295000)
	MDRV_CPU_MEMORY(jaguar_readmem, jaguar_writemem)

	MDRV_CPU_ADD(JAGUARGPU, 52000000/2)
	MDRV_CPU_CONFIG(gpu_config)
	MDRV_CPU_MEMORY(gpu_readmem,gpu_writemem)

	MDRV_CPU_ADD(JAGUARDSP, 52000000/2)
	MDRV_CPU_CONFIG(dsp_config)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(dsp_readmem,dsp_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(jaguar)
	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(40*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
	MDRV_PALETTE_LENGTH(65534)

	MDRV_VIDEO_START(cojag)
	MDRV_VIDEO_UPDATE(cojag)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( jaguar )
	ROM_REGION( 0xe20000, REGION_CPU1, 0 )  /* 4MB for RAM at 0 */
	ROM_LOAD16_WORD( "jagboot.rom", 0xe00000, 0x20000, CRC(fb731aaa) )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static void common_init(UINT8 crosshair, UINT16 gpu_jump_offs, UINT16 spin_pc)
{
	/* copy over the ROM */
	cojag_draw_crosshair = crosshair;

	/* install synchronization hooks for GPU */
	//install_mem_write32_handler(0, 0xf0b000 + gpu_jump_offs, 0xf0b003 + gpu_jump_offs, gpu_jump_w);
	//install_mem_read32_handler(1, 0xf03000 + gpu_jump_offs, 0xf03003 + gpu_jump_offs, gpu_jump_r);
	gpu_jump_address = &jaguar_gpu_ram[gpu_jump_offs/4];
	gpu_spin_pc = 0xf03000 + spin_pc;

	/* init the sound system and install DSP speedups */
	cojag_sound_init();
}

static DRIVER_INIT( jaguar )
{
	common_init(0, 0x0c0, 0x09e);
}

static DEVICE_LOAD( jaguar_cart )
{
	return cartslot_load_generic(file, REGION_CPU1, 0x800000, 1, 0x600000, CARTLOAD_32BIT_BE);
}

SYSTEM_CONFIG_START(jaguar)
	CONFIG_DEVICE_CARTSLOT_REQ(1, "jag\0abs\0bin\0rom\0", NULL, NULL, device_load_jaguar_cart, NULL, NULL, NULL)
SYSTEM_CONFIG_END



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

/*    YEAR	NAME      PARENT    COMPAT	MACHINE   INPUT     INIT      CONFIG	COMPANY		FULLNAME */
CONSX(1993,	jaguar,   0,        0,		jaguar,   jaguar,   jaguar,   jaguar,	"Atari",	"Atari Jaguar", GAME_NOT_WORKING)
