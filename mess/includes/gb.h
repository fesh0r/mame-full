extern UINT8 *gb_ram;

void gameboy_sound_w(int offset, int data);


/* Custom Sound Interface */
int gameboy_sh_start(const struct MachineSound* driver);

extern VIDEO_UPDATE( gb );


