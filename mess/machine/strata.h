/*
	strata.h: header file for strata.c
*/

int strataflash_init(int id);
int strataflash_load(int id, mame_file *f);
int strataflash_save(int id, mame_file *file);
data8_t strataflash_8_r(int id, data32_t address);
void strataflash_8_w(int id, data32_t address, data8_t data);
data16_t strataflash_16_r(int id, offs_t offset);
void strataflash_16_w(int id, offs_t offset, data16_t data);
