/*
	header file for the Geneve driver
*/

/* defines */

/* offsets for REGION_CPU1 */
enum
{
	offset_rom_geneve = 0x0000,			/* boot ROM (16 kbytes) */
	offset_sram_geneve = 0x4000,		/* static RAM (32 or 64 kbytes) */
	offset_dram_geneve = 0x14000,		/* dynamic RAM (512 kbytes) or SRAM (2MBytes) with Memex board and GenMod extension */
	region_cpu1_len_geneve = 0x214000	/* total len */
};


/* defines for input ports */
enum
{
	input_port_config_geneve = 0,
	input_port_joysticks_geneve,
	input_port_mouse_buttons_geneve,
	input_port_mouse_deltax_geneve,
	input_port_mouse_deltay_geneve,
	input_port_keyboard_geneve
};


/* prototypes for machine code */

void init_geneve(void);
void init_genmod(void);

void machine_init_geneve(void);
void machine_stop_geneve(void);

int video_start_geneve(void);
void geneve_hblank_interrupt(void);

READ_HANDLER ( geneve_r );
WRITE_HANDLER ( geneve_w );

WRITE_HANDLER ( geneve_peb_mode_cru_w );
