extern int genesis_sharedram_size;
extern int genesis_soundram_size;
extern unsigned char genesis_sharedram[];
extern unsigned char * genesis_soundram;

void genesis_init_machine (void);
int genesis_load_rom (int id);
int genesis_id_rom (int id);

int genesis_interrupt (void);
WRITE_HANDLER ( genesis_io_w );
READ_HANDLER  ( genesis_io_r );
READ16_HANDLER  ( genesis_ctrl_r );
WRITE16_HANDLER ( genesis_ctrl_w );

READ_HANDLER  ( cartridge_ram_r );
WRITE_HANDLER ( cartridge_ram_w );

#define SINGLE_BYTE_ACCESS(d) (((d & 0xffff0000) == 0xff000000) || \
			       ((d & 0xffff0000) == 0x00ff0000))

#ifdef LSB_FIRST
#define ACTUAL_BYTE_ADDRESS(a) ((a) ^ 1)
#else
#define ACTUAL_BYTE_ADDRESS(a) ((a))
#endif

#define LOWER_BYTE_ACCESS(d) ((d & 0xff000000))
#define UPPER_BYTE_ACCESS(d) ((d & 0x00ff0000))
