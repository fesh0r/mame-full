#ifndef __C64_H_
#define __C64_H_

#include "driver.h"
#include "c1551.h"

#define KEY_ARROW_LEFT (input_port_7_r(0)&0x8000)
#define KEY_1 (input_port_7_r(0)&0x4000)
#define KEY_2 (input_port_7_r(0)&0x2000)
#define KEY_3 (input_port_7_r(0)&0x1000)
#define KEY_4 (input_port_7_r(0)&0x800)
#define KEY_5 (input_port_7_r(0)&0x400)
#define KEY_6 (input_port_7_r(0)&0x200)
#define KEY_7 (input_port_7_r(0)&0x100)
#define KEY_8 (input_port_7_r(0)&0x80)
#define KEY_9 (input_port_7_r(0)&0x40)
#define KEY_0 (input_port_7_r(0)&0x20)
#define KEY_PLUS (input_port_7_r(0)&0x10)
#define KEY_MINUS (input_port_7_r(0)&8)
#define KEY_POUND (input_port_7_r(0)&4)
#define KEY_HOME (input_port_7_r(0)&2)
#define KEY_DEL (input_port_7_r(0)&1)

#define KEY_CTRL (input_port_8_r(0)&0x8000)
#define KEY_Q (input_port_8_r(0)&0x4000)
#define KEY_W (input_port_8_r(0)&0x2000)
#define KEY_E (input_port_8_r(0)&0x1000)
#define KEY_R (input_port_8_r(0)&0x800)
#define KEY_T (input_port_8_r(0)&0x400)
#define KEY_Y (input_port_8_r(0)&0x200)
#define KEY_U (input_port_8_r(0)&0x100)
#define KEY_I (input_port_8_r(0)&0x80)
#define KEY_O (input_port_8_r(0)&0x40)
#define KEY_P (input_port_8_r(0)&0x20)
#define KEY_ATSIGN (input_port_8_r(0)&0x10)
#define KEY_ASTERIX (input_port_8_r(0)&8)
#define KEY_ARROW_UP (input_port_8_r(0)&4)
#define KEY_RESTORE (input_port_8_r(0)&2)
#define KEY_STOP (input_port_8_r(0)&1)

#define KEY_SHIFTLOCK (input_port_9_r(0)&0x8000)
#define KEY_A (input_port_9_r(0)&0x4000)
#define KEY_S (input_port_9_r(0)&0x2000)
#define KEY_D (input_port_9_r(0)&0x1000)
#define KEY_F (input_port_9_r(0)&0x800)
#define KEY_G (input_port_9_r(0)&0x400)
#define KEY_H (input_port_9_r(0)&0x200)
#define KEY_J (input_port_9_r(0)&0x100)
#define KEY_K (input_port_9_r(0)&0x80)
#define KEY_L (input_port_9_r(0)&0x40)
#define KEY_SEMICOLON (input_port_9_r(0)&0x20)
#define KEY_COLON (input_port_9_r(0)&0x10)
#define KEY_EQUALS (input_port_9_r(0)&8)
#define KEY_RETURN (input_port_9_r(0)&4)
#define KEY_CBM (input_port_9_r(0)&2)
#define KEY_LEFT_SHIFT ((input_port_9_r(0)&1)||KEY_SHIFTLOCK)

#define KEY_F5 (input_port_11_r(0)&0x8000)
#define KEY_F7 (input_port_11_r(0)&0x4000)
#define KEY_CURSOR_UP (input_port_11_r(0)&0x2000)
#define KEY_CURSOR_LEFT (input_port_11_r(0)&0x1000)

#define KEY_Z (input_port_10_r(0)&0x8000)
#define KEY_X (input_port_10_r(0)&0x4000)
#define KEY_C (input_port_10_r(0)&0x2000)
#define KEY_V (input_port_10_r(0)&0x1000)
#define KEY_B (input_port_10_r(0)&0x800)
#define KEY_N (input_port_10_r(0)&0x400)
#define KEY_M (input_port_10_r(0)&0x200)
#define KEY_COMMA (input_port_10_r(0)&0x100)
#define KEY_POINT (input_port_10_r(0)&0x80)
#define KEY_SLASH (input_port_10_r(0)&0x40)
#define KEY_RIGHT_SHIFT ((input_port_10_r(0)&0x20)\
			 ||KEY_CURSOR_UP||KEY_CURSOR_LEFT)
#define KEY_CURSOR_DOWN ((input_port_10_r(0)&0x10)||KEY_CURSOR_UP)
#define KEY_CURSOR_RIGHT ((input_port_10_r(0)&8)||KEY_CURSOR_LEFT)
#define KEY_SPACE (input_port_10_r(0)&4)
#define KEY_F1 (input_port_10_r(0)&2)
#define KEY_F3 (input_port_10_r(0)&1)
     
     
/* macros to access the dip switches */
#define LIGHTPEN ((input_port_12_r(0)&0xc000)==0xc000)
#define PADDLES12 ((input_port_12_r(0)&0xc000)==0x8000)
#define JOYSTICK1 ((input_port_12_r(0)&0xc000)==0x4000)
#define LIGHTPEN_POINTER (LIGHTPEN&&(readinputport(10)&0x2000))
#define PADDLES34 ((input_port_12_r(0)&0x1800)==0x1000)
#define JOYSTICK2 ((input_port_12_r(0)&0x1800)==0x800)

#define DATASSETTE (input_port_12_r(0)&0x200)
#define DATASSETTE_TONE (input_port_12_r(0)&0x100)

#define AUTO_MODULE ((input_port_12_r(0)&0x1c)==0)
#define ULTIMAX_MODULE ((input_port_12_r(0)&0x1c)==4)
#define C64_MODULE ((input_port_12_r(0)&0x1c)==8)
#define SUPERGAMES_MODULE ((input_port_12_r(0)&0x1c)==0x10)
#define ROBOCOP2_MODULE ((input_port_12_r(0)&0x1c)==0x14)

#define SERIAL8ON (input_port_12_r(0)&2)
#define SERIAL9ON (input_port_12_r(0)&1)

#define PADDLE1_BUTTON	(PADDLES12&&(input_port_1_r(0)&0x100))
#define PADDLE1_VALUE	(PADDLES12?input_port_1_r(0)&0xff:0xff)
#define PADDLE2_BUTTON	(PADDLES12&&(input_port_2_r(0)&0x100))
#define PADDLE2_VALUE	(PADDLES12?input_port_2_r(0)&0xff:0xff)
#define PADDLE3_BUTTON	(PADDLES34&&(input_port_3_r(0)&0x100))
#define PADDLE3_VALUE	(PADDLES34?input_port_3_r(0)&0xff:0xff)
#define PADDLE4_BUTTON	(PADDLES34&&(input_port_4_r(0)&0x100))
#define PADDLE4_VALUE	(PADDLES34?input_port_4_r(0)&0xff:0xff)

#define LIGHTPEN_BUTTON (LIGHTPEN&&(readinputport(5)&0x8000))
#define LIGHTPEN_X_VALUE ((readinputport(5)&0x3ff)&~1) /* effectiv resolution */
#define LIGHTPEN_Y_VALUE (readinputport(6)&~1) /* effectiv resolution */

#define JOYSTICK_1_LEFT	(JOYSTICK1&&(input_port_0_r(0)&0x8000))
#define JOYSTICK_1_RIGHT	(JOYSTICK1&&(input_port_0_r(0)&0x4000))
#define JOYSTICK_1_UP		(JOYSTICK1&&(input_port_0_r(0)&0x2000))
#define JOYSTICK_1_DOWN	(JOYSTICK1&&(input_port_0_r(0)&0x1000))
#define JOYSTICK_1_BUTTON (JOYSTICK1&&(input_port_0_r(0)&0x800))
#define JOYSTICK_2_LEFT	(JOYSTICK2&&(input_port_0_r(0)&0x80))
#define JOYSTICK_2_RIGHT	(JOYSTICK2&&(input_port_0_r(0)&0x40))
#define JOYSTICK_2_UP		(JOYSTICK2&&(input_port_0_r(0)&0x20))
#define JOYSTICK_2_DOWN	(JOYSTICK2&&(input_port_0_r(0)&0x10))
#define JOYSTICK_2_BUTTON (JOYSTICK2&&(input_port_0_r(0)&8))

#define DATASSETTE_PLAY		(input_port_11_r(0)&4)
#define DATASSETTE_RECORD	(input_port_11_r(0)&2)
#define DATASSETTE_STOP		(input_port_11_r(0)&1)
#define QUICKLOAD		(input_port_11_r(0)&8)
#define JOYSTICK_SWAP		(input_port_11_r(0)&0x800)

extern UINT8 *c64_memory;
extern UINT8 *c64_colorram;
extern UINT8 *c64_basic;
extern UINT8 *c64_kernal;
extern UINT8 *c64_chargen;
extern UINT8 *c64_roml;
extern UINT8 *c64_romh;

extern void c64_m6510_port_w(int offset, int data);
extern int c64_m6510_port_r(int offset);
void c64_colorram_write(int offset, int value);

extern void c64_driver_init(void);
extern void c64pal_driver_init(void);
extern void ultimax_driver_init(void);
extern void c64_driver_shutdown(void);
extern void c64_init_machine(void);
extern void c64_shutdown_machine(void);
extern int  c64_frame_interrupt(void);

int c64_rom_init(int id, const char *name);
extern int  c64_rom_load(int id, const char *name);
extern int  c64_rom_id(const char *name, const char *gamename);

/*only for debugging */
extern void c64_status(char *text, int size);

#endif
