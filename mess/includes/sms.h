#ifndef _SMS_H_
#define _SMS_H_

#define LOG_REG
#define LOG_PAGING
//#define LOG_CURLINE
#define LOG_COLOR

#define NVRAM_SIZE								(0x8000)
#define CPU_ADDRESSABLE_SIZE			(0x10000)

/* Global data */

extern UINT8 smsFMDetect;
extern UINT8 smsVersion;
extern int systemType;

#define CONSOLE_SMS									0
#define CONSOLE_SMS_U_V13						1
#define CONSOLE_SMS_E_V13						2
#define CONSOLE_SMS_U_HACK_V13			3
#define CONSOLE_SMS_E_HACK_V13			4
#define CONSOLE_SMS_U_ALEX					5
#define CONSOLE_SMS_E_ALEX					6
#define CONSOLE_SMS_E_SONIC					7
#define CONSOLE_SMS_B_SONIC					8
#define CONSOLE_SMS_U_HOSH_V24			9
#define CONSOLE_SMS_E_HOSH_V24			10
#define CONSOLE_SMS_U_HO_V34				11
#define CONSOLE_SMS_E_HO_V34				12
#define CONSOLE_SMS_U_MD_3D					13
#define CONSOLE_SMS_E_MD_3D					14
#define CONSOLE_SMS_PAL							20
#define CONSOLE_SMS_J_V21						21
#define CONSOLE_SMS_J_M3						22
#define CONSOLE_SMS_J_SS						23
#define CONSOLE_GG_UE								30
#define CONSOLE_GG_J								31
#define CONSOLE_GG_MAJ_UE						32
#define CONSOLE_GG_MAJ_J						33

#define IS_SMS											(systemType == CONSOLE_SMS)
#define IS_SMS_U_V13								(systemType == CONSOLE_SMS_U_V13)
#define IS_SMS_E_V13								(systemType == CONSOLE_SMS_E_V13)
#define IS_SMS_U_HACK_V13						(systemType == CONSOLE_SMS_U_HACK_V13)
#define IS_SMS_E_HACK_V13						(systemType == CONSOLE_SMS_E_HACK_V13)
#define IS_SMS_U_ALEX								(systemType == CONSOLE_SMS_U_ALEX)
#define IS_SMS_E_ALEX								(systemType == CONSOLE_SMS_E_ALEX)
#define IS_SMS_E_SONIC							(systemType == CONSOLE_SMS_E_SONIC)
#define IS_SMS_B_SONIC							(systemType == CONSOLE_SMS_B_SONIC)
#define IS_SMS_U_HOSH_V24						(systemType == CONSOLE_SMS_U_HOSH_V24)
#define IS_SMS_E_HOSH_V24						(systemType == CONSOLE_SMS_E_HOSH_V24)
#define IS_SMS_U_HO_V34							(systemType == CONSOLE_SMS_U_HO_V34)
#define IS_SMS_E_HO_V34							(systemType == CONSOLE_SMS_E_HO_V34)
#define IS_SMS_U_MD_3D							(systemType == CONSOLE_SMS_U_MD_3D)
#define IS_SMS_E_MD_3D							(systemType == CONSOLE_SMS_E_MD_3D)
#define IS_SMS_PAL									(systemType == CONSOLE_SMS_PAL)
#define IS_SMS_J_V21								(systemType == CONSOLE_SMS_J_V21)
#define IS_SMS_J_M3									(systemType == CONSOLE_SMS_J_M3)
#define IS_SMS_J_SS									(systemType == CONSOLE_SMS_J_SS)
#define IS_GG_UE										(systemType == CONSOLE_GG_UE)
#define IS_GG_J											(systemType == CONSOLE_GG_J)
#define IS_GG_MAJ_UE								(systemType == CONSOLE_GG_MAJ_UE)
#define IS_GG_MAJ_J									(systemType == CONSOLE_GG_MAJ_J)
#define IS_GG_ANY										(IS_GG_UE || IS_GG_J || IS_GG_MAJ_UE || IS_GG_MAJ_J)

/* Function prototypes */

WRITE8_HANDLER(sms_cartram_w);
WRITE8_HANDLER(sms_fm_detect_w);
 READ8_HANDLER(sms_fm_detect_r);
 READ8_HANDLER(sms_input_port_0_r);
WRITE8_HANDLER(sms_YM2413_register_port_0_w);
WRITE8_HANDLER(sms_YM2413_data_port_0_w);
WRITE8_HANDLER(sms_version_w);
 READ8_HANDLER(sms_version_r);
WRITE8_HANDLER(sms_mapper_w);
WRITE8_HANDLER(sms_bios_w);
WRITE8_HANDLER(gg_sio_w);
 READ8_HANDLER(gg_sio_r);
 READ8_HANDLER(gg_psg_r);
WRITE8_HANDLER(gg_psg_w);
 READ8_HANDLER(gg_input_port_2_r);

void setup_rom(void);

DEVICE_LOAD( sms_cart );

MACHINE_INIT(sms);
MACHINE_STOP(sms);
INTERRUPT_GEN(sms);
NVRAM_HANDLER(sms);

#define IS_NTSC											(Machine->drv->frames_per_second == 60)
#define IS_PAL											(Machine->drv->frames_per_second == 50)

#define NTSC_X_PIXELS								(0x0156)				/* 342 pixels */
#define NTSC_Y_PIXELS								(0x0106)				/* 262 lines */
#define PAL_X_PIXELS								(NTSC_X_PIXELS)
#define PAL_Y_PIXELS								(0x0139)				/* 313 lines */
#define LBORDER_X_PIXELS						(0x0D)					/* 13 pixels */
#define RBORDER_X_PIXELS						(0x0F)					/* 15 pixels */
#define NTSC_192_TBORDER_Y_PIXELS		(0x1B)					/* 27 lines */
#define NTSC_192_BBORDER_Y_PIXELS		(0x18)					/* 24 lines */
#define NTSC_224_TBORDER_Y_PIXELS		(0x0B)					/* 11 lines */
#define NTSC_224_BBORDER_Y_PIXELS		(0x08)					/* 8 lines */
#define PAL_192_TBORDER_Y_PIXELS		(0x36)					/* 54 lines */
#define PAL_192_BBORDER_Y_PIXELS		(0x30)					/* 48 lines */
#define PAL_224_TBORDER_Y_PIXELS		(0x26)					/* 38 lines */
#define PAL_224_BBORDER_Y_PIXELS		(0x20)					/* 32 lines */
#define PAL_240_TBORDER_Y_PIXELS		(0x1E)					/* 30 lines */
#define PAL_240_BBORDER_Y_PIXELS		(0x18)					/* 24 lines */
#define TOP_192_BORDER							(IS_NTSC ? NTSC_192_TBORDER_Y_PIXELS : PAL_192_TBORDER_Y_PIXELS)
#define BOTTOM_192_BORDER						(IS_NTSC ? NTSC_192_BBORDER_Y_PIXELS : PAL_192_BBORDER_Y_PIXELS)

#define GG_CRAM_SIZE				(0x40)	/* 32 colors x 2 bytes per color = 64 bytes */
#define SMS_CRAM_SIZE				(0x20)	/* 32 colors x 1 bytes per color = 32 bytes */

#define VRAM_SIZE						(0x4000)

#define NUM_OF_REGISTER			(0x10)	/* 16 registers */

#define STATUS_VINT					(0x80)	/* Pending vertical interrupt flag */
#define STATUS_HINT					(0x40)	/* Pending horizontal interrupt flag */
#define STATUS_SPRCOL				(0x20)	/* Object collision flag */

#define IO_EXPANSION				(0x80)	/* Expansion slot enable (1= disabled, 0= enabled) */
#define IO_CARTRIDGE				(0x40)	/* Cartridge slot enable (1= disabled, 0= enabled) */
#define IO_CARD							(0x20)	/* Card slot disabled (1= disabled, 0= enabled) */
#define IO_WORK_RAM					(0x10)	/* Work RAM disabled (1= disabled, 0= enabled) */
#define IO_BIOS_ROM					(0x08)	/* BIOS ROM disabled (1= disabled, 0= enabled) */
#define IO_CHIP							(0x04)	/* I/O chip disabled (1= disabled, 0= enabled) */

#define BACKDROP_COLOR			(0x10 + (reg[0x07] & 0x0F))

/* Function prototypes */

extern int currentLine;

VIDEO_START(sms);
VIDEO_UPDATE(sms);
 READ8_HANDLER(sms_vdp_curline_r);
 READ8_HANDLER(sms_vdp_data_r);
WRITE8_HANDLER(sms_vdp_data_w);
 READ8_HANDLER(sms_vdp_ctrl_r);
WRITE8_HANDLER(sms_vdp_ctrl_w);
#ifdef MAME_DEBUG
void sms_show_tile_line(struct mame_bitmap *bitmap, int line, int palletteSelected);
#endif
void sms_refresh_line(struct mame_bitmap *bitmap, int line);
void sms_update_palette(void);

#endif /* _SMS_H_ */

