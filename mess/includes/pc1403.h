
int pc1403_reset(void);
int pc1403_brk(void);
void pc1403_outa(int data);
//void pc1403_outb(int data);
//void pc1403_outc(int data);
int pc1403_ina(void);
//int pc1403_inb(void);
void init_pc1403(void);
void pc1403_machine_init(void);
void pc1403_machine_stop(void);

READ_HANDLER(pc1403_asic_read);
WRITE_HANDLER(pc1403_asic_write);

/* in vidhrdw/pocketc.c */
int pc1403_vh_start(void);

extern READ_HANDLER(pc1403_lcd_read);
extern WRITE_HANDLER(pc1403_lcd_write);

void pc1403_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);

/* in systems/pocketc.c */
#define KEY_SMALL input_port_1_r(0)&0x40
#define RAM32K (input_port_10_r(0)&0x80)==0x80

