#ifndef YM2151IN_H
#define YM2151IN_H

#define MAX_YM2151 3

struct YM2151interface2
{
	int num;	/* total number of YM2151 in the machine */
	int clock;	/* chip clock */
	int volume[MAX_YM2151];
	void (*handler[MAX_YM2151])();
};

int YM2151_read_port_0_r(int offset);
int YM2151_read_port_1_r(int offset);
int YM2151_read_port_2_r(int offset);

void YM2151_control_port_0_w(int offset,int data);
void YM2151_control_port_1_w(int offset,int data);
void YM2151_control_port_2_w(int offset,int data);

void YM2151_write_port_0_w(int offset,int data);
void YM2151_write_port_1_w(int offset,int data);
void YM2151_write_port_2_w(int offset,int data);

int YM2151_1_sh_start(struct YM2151interface2 *interface);
void YM2151_1_sh_stop(void);
void YM2151_1_sh_update(void);

#endif
