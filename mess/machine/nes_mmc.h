#ifndef __MMC_H
#define __MMC_H

typedef struct __mmc
{
	int iNesMapper; /* iNES Mapper # */
	const char *desc;     /* Mapper description */
	mem_write_handler mmc_write_low; /* $4100-$5fff write routine */
	mem_read_handler mmc_read_low; /* $4100-$5fff read routine */
	mem_write_handler mmc_write_mid; /* $6000-$7fff write routine */
	mem_write_handler mmc_write; /* $8000-$ffff write routine */
	void (*ppu_latch)(offs_t offset);
	ppu2c03b_scanline_cb mmc_irq;
} mmc;

extern mmc mmc_list[];
extern int MMC1_extended; /* 0 = normal MMC1 cart, 1 = 512k MMC1, 2 = 1024k MMC1 */

#define MMC5_VRAM

extern UINT8 MMC5_vram[0x400];
extern int MMC5_vram_control;

extern int Mapper;

extern mem_write_handler mmc_write_low;
extern mem_read_handler mmc_read_low;
extern mem_write_handler mmc_write_mid;
extern mem_read_handler mmc_read_mid;
extern mem_write_handler mmc_write;
extern void (*ppu_latch)(offs_t offset);

int mapper_reset (int mapperNum);

WRITE_HANDLER ( nes_mid_mapper_w );
WRITE_HANDLER ( nes_mapper_w );

READ_HANDLER ( fds_r );
WRITE_HANDLER ( fds_w );

#endif
