#ifndef __MMC_H
#define __MMC_H

typedef struct __mmc
{
	int iNesMapper; /* iNES Mapper # */
	char *desc;     /* Mapper description */
	void (*mmc_write)(int offset, int data); /* Write routine */
} mmc;

extern mmc mmc_list[];
extern int MMC_Table;
extern int MMC1_extended; /* 0 = normal MMC1 cart, 1 = 512k MMC1, 2 = 1024k MMC1 */

extern int MMC3_DOIRQ, MMC3_IRQ;
extern int MMC2_bank0, MMC2_bank1, MMC2_hibank1_val, MMC2_hibank2_val;
extern int Mapper;
extern int uses_irqs;
extern void (*mmc_write)(int offset, int data);

int Reset_Mapper (int mapperNum);
void nes_mapper_w (int offset, int data);

#endif