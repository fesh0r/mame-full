#ifndef __MMC_H
#define __MMC_H

typedef struct __mmc
{
	int iNesMapper; /* iNES Mapper # */
	char *desc;     /* Mapper description */
	void (*mmc_write_low)(int offset, int data); /* $4100-$5fff write routine */
	int (*mmc_read_low)(int offset); /* $4100-$5fff read routine */
	void (*mmc_write_mid)(int offset, int data); /* $6000-$7fff write routine */
	void (*mmc_write)(int offset, int data); /* $8000-$ffff write routine */
	void (*ppu_latch)(int offset);
	int (*mmc_irq)(int scanline);
} mmc;

extern mmc mmc_list[];
extern int MMC1_extended; /* 0 = normal MMC1 cart, 1 = 512k MMC1, 2 = 1024k MMC1 */

#define MMC5_VRAM

extern UINT8 MMC5_vram[0x400];
extern int MMC5_vram_control;

extern int Mapper;

extern void (*mmc_write_low)(int offset, int data);
extern int (*mmc_read_low)(int offset);
extern void (*mmc_write_mid)(int offset, int data);
extern int (*mmc_read_mid)(int offset);
extern void (*mmc_write)(int offset, int data);
extern void (*ppu_latch)(int offset);
extern int (*mmc_irq)(int scanline);

int mapper_reset (int mapperNum);

WRITE_HANDLER ( nes_mid_mapper_w );
WRITE_HANDLER ( nes_mapper_w );

READ_HANDLER ( fds_r );
WRITE_HANDLER ( fds_w );

#endif
