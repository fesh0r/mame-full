#ifndef __DEVICES_H_
#define __DEVICES_H_

#ifdef __DEVICES_C_
#define EXTERN
#else
#define EXTERN extern
#endif

#define JOY_CODES		256
#define JOY_MAX			8
#define JOY_BUTTONS		32
#define JOY_AXES		8
#define JOY_DIRS		2
#define JOY_NAME_LEN		20
#define HISTORY_LENGTH		16

#ifndef USE_XINPUT_DEVICES
/* only one mouse for now */
#	define MOUSE_MAX	1
#else
/* now we have 4 */
#	define MOUSE_MAX	5
#endif
#define MOUSE_BUTTONS		8
#define MOUSE_AXES		8

#define GUN_MAX			4

/* now axis entries in the mouse_list, these are get through another way,
   like the analog joy-values */
#define MOUSE_LIST_TOTAL_ENTRIES MOUSE_BUTTONS
#define MOUSE_LIST_LEN (MOUSE * MOUSE_LIST_TOTAL_ENTRIES)

enum
{
	JOY_NONE,
	JOY_I386,
	JOY_PAD,
	JOY_X11,
	JOY_I386NEW,
	JOY_USB,
	JOY_PS2,
	JOY_SDL
};

enum
{
	AXIS_TYPE_INVALID = 0,
	AXIS_TYPE_DIGITAL,
	AXIS_TYPE_ANALOG
};

struct xmame_keyboard_event
{
	unsigned char press;
	unsigned char scancode;
	unsigned short unicode;
};

int xmame_keyboard_init(void);
void xmame_keyboard_exit();
void xmame_keyboard_register_event(struct xmame_keyboard_event *event);
void xmame_keyboard_clear(void);

struct axisdata_struct
{
	/* current value */
	int val;
	/* calibration data */
	int min;
	int center;
	int max;
	/* boolean values */
	int dirs[JOY_DIRS];
};

struct joydata_struct
{
	int fd;
	int num_axes;
	int num_buttons;
	struct axisdata_struct axis[JOY_AXES];
	int buttons[JOY_BUTTONS];
};

struct mousedata_struct
{
	int buttons[MOUSE_BUTTONS];
	int deltas[MOUSE_AXES];
};

struct rapidfire_struct
{
	int setting[10];
	int status[10];
	int enable;
	int ctrl_button;
	int ctrl_prev_status;
};

EXTERN struct joydata_struct joy_data[JOY_MAX];
EXTERN struct mousedata_struct mouse_data[MOUSE_MAX];
EXTERN struct rapidfire_struct rapidfire_data[4];
EXTERN void (*joy_poll_func)(void);
EXTERN int joytype;
EXTERN int is_usb_ps_gamepad;
EXTERN int rapidfire_enable;

extern struct rc_option joy_i386_opts[];
extern struct rc_option joy_pad_opts[];
extern struct rc_option joy_x11_opts[];
extern struct rc_option joy_usb_opts[];
extern struct rc_option joy_ps2_opts[];

#ifdef USE_LIGHTGUN_ABS_EVENT
#include "joystick-drivers/lightgun_abs_event.h"
#endif

/*** prototypes ***/
void joy_evaluate_moves(void);
void joy_i386_init(void);
void joy_pad_init(void);
void joy_x11_init(void);
void joy_usb_init(void);
void joy_ps2_init(void);
void joy_ps2_exit(void);
void joy_SDL_init(void);
#undef EXTERN

#endif /* ifndef __DEVICES_H_ */
