/***************************************************************************
    mos tri port interface 6525

    peter.trauner@jk.uni-linz.ac.at

	used in commodore b series
***************************************************************************/
#ifndef __TPI6525_H_
#define __TPI6525_H_

/* fill in the callback functions */
typedef struct {
	int number;
	struct { 
		int (*read)(void);
		void (*output)(int data);
		int port, ddr; 
	} a,b,c;

	struct {
		void (*output)(int level);
		int level;
	} ca, cb, interrupt;

	int cr;
	int air;

	int irq_level[5];
} TPI6525;

extern TPI6525 tpi6525[2];

void tpi6525_0_reset(void);
void tpi6525_1_reset(void);

void tpi6525_0_irq0_level(int level);
void tpi6525_0_irq1_level(int level);
void tpi6525_0_irq2_level(int level);
void tpi6525_0_irq3_level(int level);
void tpi6525_0_irq4_level(int level);

void tpi6525_1_irq0_level(int level);
void tpi6525_1_irq1_level(int level);
void tpi6525_1_irq2_level(int level);
void tpi6525_1_irq3_level(int level);
void tpi6525_1_irq4_level(int level);

int tpi6525_0_port_r(int offset);
int tpi6525_1_port_r(int offset);

void tpi6525_0_port_w(int offset, int data);
void tpi6525_1_port_w(int offset, int data);

#endif
