
int pc1401_reset(void);
int pc1401_brk(void);
void pc1401_outa(int data);
void pc1401_outb(int data);
void pc1401_outc(int data);
int pc1401_ina(void);
int pc1401_inb(void);
void init_pc1401(void);
void pc1401_machine_init(void);
void pc1401_machine_stop(void);

int pc1350_brk(void);
int pc1350_ina(void);
int pc1350_inb(void);
void init_pc1350(void);
void pc1350_machine_init(void);
void pc1350_machine_stop(void);

int pc1251_brk(void);
int pc1251_ina(void);
int pc1251_inb(void);
void init_pc1251(void);
void pc1251_machine_init(void);
void pc1251_machine_stop(void);

/* in vidhrdw/pocketc.c */
extern READ_HANDLER(pc1401_lcd_read);
extern WRITE_HANDLER(pc1401_lcd_write);
extern READ_HANDLER(pc1251_lcd_read);
extern WRITE_HANDLER(pc1251_lcd_write);
extern READ_HANDLER(pc1350_lcd_read);
extern WRITE_HANDLER(pc1350_lcd_write);

int pc1350_keyboard_line_r(void);
extern unsigned char pc1401_palette[248][3];
extern unsigned short pc1401_colortable[8][2];
void pocketc_init_colors (unsigned char *sys_palette,
						  unsigned short *sys_colortable,
						  const unsigned char *color_prom);
int pocketc_vh_start(void);
void pocketc_vh_stop(void);
void pc1401_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);
void pc1251_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);
void pc1350_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);

/* in systems/pocketc.c */
#define KEY_OFF input_port_0_r(0)&0x80
#define KEY_CAL input_port_0_r(0)&0x40
#define KEY_BASIC input_port_0_r(0)&0x20
#define KEY_BRK input_port_0_r(0)&0x10
#define KEY_DEF input_port_0_r(0)&8
#define KEY_DOWN input_port_0_r(0)&4
#define KEY_UP input_port_0_r(0)&2
#define KEY_LEFT input_port_0_r(0)&1

#define KEY_RIGHT input_port_1_r(0)&0x80
#define KEY_SHIFT input_port_1_r(0)&0x40
#define KEY_Q input_port_1_r(0)&0x20
#define KEY_W input_port_1_r(0)&0x10
#define KEY_E input_port_1_r(0)&8
#define KEY_R input_port_1_r(0)&4
#define KEY_T input_port_1_r(0)&2
#define KEY_Y input_port_1_r(0)&1

#define KEY_U input_port_2_r(0)&0x80
#define KEY_I input_port_2_r(0)&0x40
#define KEY_O input_port_2_r(0)&0x20
#define KEY_P input_port_2_r(0)&0x10
#define KEY_A input_port_2_r(0)&8
#define KEY_S input_port_2_r(0)&4
#define KEY_D input_port_2_r(0)&2
#define KEY_F input_port_2_r(0)&1

#define KEY_G input_port_3_r(0)&0x80
#define KEY_H input_port_3_r(0)&0x40
#define KEY_J input_port_3_r(0)&0x20
#define KEY_K input_port_3_r(0)&0x10
#define KEY_L input_port_3_r(0)&8
#define KEY_COMMA input_port_3_r(0)&4
#define KEY_Z input_port_3_r(0)&2
#define KEY_X input_port_3_r(0)&1

#define KEY_C input_port_4_r(0)&0x80
#define KEY_V input_port_4_r(0)&0x40
#define KEY_B input_port_4_r(0)&0x20
#define KEY_N input_port_4_r(0)&0x10
#define KEY_M input_port_4_r(0)&8
#define KEY_SPC input_port_4_r(0)&4
#define KEY_ENTER input_port_4_r(0)&2
#define KEY_HYP input_port_4_r(0)&1

#define KEY_SIN input_port_5_r(0)&0x80
#define KEY_COS input_port_5_r(0)&0x40
#define KEY_TAN input_port_5_r(0)&0x20
#define KEY_FE input_port_5_r(0)&0x10
#define KEY_CCE input_port_5_r(0)&8
#define KEY_HEX input_port_5_r(0)&4
#define KEY_DEG input_port_5_r(0)&2
#define KEY_LN input_port_5_r(0)&1

#define KEY_LOG input_port_6_r(0)&0x80
#define KEY_1X input_port_6_r(0)&0x40
#define KEY_STAT input_port_6_r(0)&0x20
#define KEY_EXP input_port_6_r(0)&0x10
#define KEY_POT input_port_6_r(0)&8
#define KEY_ROOT input_port_6_r(0)&4
#define KEY_SQUARE input_port_6_r(0)&2
#define KEY_BRACE_LEFT input_port_6_r(0)&1

#define KEY_BRACE_RIGHT input_port_7_r(0)&0x80
#define KEY_7 input_port_7_r(0)&0x40
#define KEY_8 input_port_7_r(0)&0x20
#define KEY_9 input_port_7_r(0)&0x10
#define KEY_DIV input_port_7_r(0)&8
#define KEY_XM input_port_7_r(0)&4
#define KEY_4 input_port_7_r(0)&2
#define KEY_5 input_port_7_r(0)&1

#define KEY_6 input_port_8_r(0)&0x80
#define KEY_MUL input_port_8_r(0)&0x40
#define KEY_RM input_port_8_r(0)&0x20
#define KEY_1 input_port_8_r(0)&0x10
#define KEY_2 input_port_8_r(0)&8
#define KEY_3 input_port_8_r(0)&4
#define KEY_MINUS input_port_8_r(0)&2
#define KEY_MPLUS input_port_8_r(0)&1

#define KEY_0 input_port_9_r(0)&0x80
#define KEY_SIGN input_port_9_r(0)&0x40
#define KEY_POINT input_port_9_r(0)&0x20
#define KEY_PLUS input_port_9_r(0)&0x10
#define KEY_EQUALS input_port_9_r(0)&8
#define KEY_RESET input_port_9_r(0)&4

#define RAM4K (input_port_10_r(0)&0x80)==0x40
#define RAM10K (input_port_10_r(0)&0x80)==0x80
#define CONTRAST (input_port_10_r(0)&7)

#define PC1251_SWITCH_MODE (input_port_0_r(0)&7)

#define PC1251_KEY_DEF input_port_0_r(0)&0x100
#define PC1251_KEY_SHIFT input_port_0_r(0)&0x80
#define PC1251_KEY_DOWN input_port_0_r(0)&0x40
#define PC1251_KEY_UP input_port_0_r(0)&0x20
#define PC1251_KEY_LEFT input_port_0_r(0)&0x10
#define PC1251_KEY_RIGHT input_port_0_r(0)&8

#define PC1251_KEY_BRK input_port_1_r(0)&0x80
#define PC1251_KEY_Q input_port_1_r(0)&0x40
#define PC1251_KEY_W input_port_1_r(0)&0x20
#define PC1251_KEY_E input_port_1_r(0)&0x10
#define PC1251_KEY_R input_port_1_r(0)&8
#define PC1251_KEY_T input_port_1_r(0)&4
#define PC1251_KEY_Y input_port_1_r(0)&2
#define PC1251_KEY_U input_port_1_r(0)&1

#define PC1251_KEY_I input_port_2_r(0)&0x80
#define PC1251_KEY_O input_port_2_r(0)&0x40
#define PC1251_KEY_P input_port_2_r(0)&0x20
#define PC1251_KEY_A input_port_2_r(0)&0x10
#define PC1251_KEY_S input_port_2_r(0)&8
#define PC1251_KEY_D input_port_2_r(0)&4
#define PC1251_KEY_F input_port_2_r(0)&2
#define PC1251_KEY_G input_port_2_r(0)&1

#define PC1251_KEY_H input_port_3_r(0)&0x80
#define PC1251_KEY_J input_port_3_r(0)&0x40
#define PC1251_KEY_K input_port_3_r(0)&0x20
#define PC1251_KEY_L input_port_3_r(0)&0x10
#define PC1251_KEY_EQUALS input_port_3_r(0)&8
#define PC1251_KEY_Z input_port_3_r(0)&4
#define PC1251_KEY_X input_port_3_r(0)&2
#define PC1251_KEY_C input_port_3_r(0)&1

#define PC1251_KEY_V input_port_4_r(0)&0x80
#define PC1251_KEY_B input_port_4_r(0)&0x40
#define PC1251_KEY_N input_port_4_r(0)&0x20
#define PC1251_KEY_M input_port_4_r(0)&0x10
#define PC1251_KEY_SPACE input_port_4_r(0)&8
#define PC1251_KEY_ENTER input_port_4_r(0)&4
#define PC1251_KEY_7 input_port_4_r(0)&2
#define PC1251_KEY_8 input_port_4_r(0)&1

#define PC1251_KEY_9 input_port_5_r(0)&0x80
#define PC1251_KEY_CL input_port_5_r(0)&0x40
#define PC1251_KEY_4 input_port_5_r(0)&0x20
#define PC1251_KEY_5 input_port_5_r(0)&0x10
#define PC1251_KEY_6 input_port_5_r(0)&8
#define PC1251_KEY_SLASH input_port_5_r(0)&4
#define PC1251_KEY_1 input_port_5_r(0)&2
#define PC1251_KEY_2 input_port_5_r(0)&1

#define PC1251_KEY_3 input_port_6_r(0)&0x80
#define PC1251_KEY_ASTERIX input_port_6_r(0)&0x40
#define PC1251_KEY_0 input_port_6_r(0)&0x20
#define PC1251_KEY_POINT input_port_6_r(0)&0x10
#define PC1251_KEY_PLUS input_port_6_r(0)&8
#define PC1251_KEY_MINUS input_port_6_r(0)&4

#define PC1251_RAM4K (input_port_7_r(0)&0xc0)==0x40
#define PC1251_RAM6K (input_port_7_r(0)&0xc0)==0x80
#define PC1251_RAM11K (input_port_7_r(0)&0xc0)==0xc0

#define PC1251_CONTRAST (input_port_7_r(0)&7)

#define PC1350_KEY_OFF input_port_0_r(0)&0x80
#define PC1350_KEY_DOWN input_port_0_r(0)&0x40
#define PC1350_KEY_UP input_port_0_r(0)&0x20
#define PC1350_KEY_MODE input_port_0_r(0)&0x10
#define PC1350_KEY_CLS input_port_0_r(0)&8
#define PC1350_KEY_LEFT input_port_0_r(0)&4
#define PC1350_KEY_RIGHT input_port_0_r(0)&2
#define PC1350_KEY_DEL input_port_0_r(0)&1

#define PC1350_KEY_INS input_port_1_r(0)&0x80
#define PC1350_KEY_BRK input_port_1_r(0)&0x40
#define PC1350_KEY_RSHIFT input_port_1_r(0)&0x20
#define PC1350_KEY_7 input_port_1_r(0)&0x10
#define PC1350_KEY_8 input_port_1_r(0)&8
#define PC1350_KEY_9 input_port_1_r(0)&4
#define PC1350_KEY_BRACE_OPEN input_port_1_r(0)&2
#define PC1350_KEY_BRACE_CLOSE input_port_1_r(0)&1

#define PC1350_KEY_4 input_port_2_r(0)&0x80
#define PC1350_KEY_5 input_port_2_r(0)&0x40
#define PC1350_KEY_6 input_port_2_r(0)&0x20
#define PC1350_KEY_SLASH input_port_2_r(0)&0x10
#define PC1350_KEY_COLON input_port_2_r(0)&8
#define PC1350_KEY_1 input_port_2_r(0)&4
#define PC1350_KEY_2 input_port_2_r(0)&2
#define PC1350_KEY_3 input_port_2_r(0)&1

#define PC1350_KEY_ASTERIX input_port_3_r(0)&0x80
#define PC1350_KEY_SEMICOLON input_port_3_r(0)&0x40
#define PC1350_KEY_0 input_port_3_r(0)&0x20
#define PC1350_KEY_POINT input_port_3_r(0)&0x10
#define PC1350_KEY_PLUS input_port_3_r(0)&8
#define PC1350_KEY_MINUS input_port_3_r(0)&4
#define PC1350_KEY_COMMA input_port_3_r(0)&2
#define PC1350_KEY_LSHIFT input_port_3_r(0)&1

#define PC1350_KEY_Q input_port_4_r(0)&0x80
#define PC1350_KEY_W input_port_4_r(0)&0x40
#define PC1350_KEY_E input_port_4_r(0)&0x20
#define PC1350_KEY_R input_port_4_r(0)&0x10
#define PC1350_KEY_T input_port_4_r(0)&8
#define PC1350_KEY_Y input_port_4_r(0)&4
#define PC1350_KEY_U input_port_4_r(0)&2
#define PC1350_KEY_I input_port_4_r(0)&1

#define PC1350_KEY_O input_port_5_r(0)&0x80
#define PC1350_KEY_P input_port_5_r(0)&0x40
#define PC1350_KEY_DEF input_port_5_r(0)&0x20
#define PC1350_KEY_A input_port_5_r(0)&0x10
#define PC1350_KEY_S input_port_5_r(0)&8
#define PC1350_KEY_D input_port_5_r(0)&4
#define PC1350_KEY_F input_port_5_r(0)&2
#define PC1350_KEY_G input_port_5_r(0)&1

#define PC1350_KEY_H input_port_6_r(0)&0x80
#define PC1350_KEY_J input_port_6_r(0)&0x40
#define PC1350_KEY_K input_port_6_r(0)&0x20
#define PC1350_KEY_L input_port_6_r(0)&0x10
#define PC1350_KEY_EQUALS input_port_6_r(0)&8
#define PC1350_KEY_SML input_port_6_r(0)&4
#define PC1350_KEY_Z input_port_6_r(0)&2
#define PC1350_KEY_X input_port_6_r(0)&1

#define PC1350_KEY_C input_port_7_r(0)&0x80
#define PC1350_KEY_V input_port_7_r(0)&0x40
#define PC1350_KEY_B input_port_7_r(0)&0x20
#define PC1350_KEY_N input_port_7_r(0)&0x10
#define PC1350_KEY_M input_port_7_r(0)&8
#define PC1350_KEY_SPACE input_port_7_r(0)&4
#define PC1350_KEY_ENTER input_port_7_r(0)&2

#define PC1350_RAM12K (input_port_8_r(0)&0x80)==0x40
#define PC1350_RAM20K (input_port_8_r(0)&0x80)==0x80
#define PC1350_CONTRAST (input_port_8_r(0)&7)

