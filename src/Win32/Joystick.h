/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

#include "osdepend.h"

struct OSDJoystick
{
    int         (*init)(options_type *options);
    void        (*exit)(void);
    const struct JoystickInfo* (*get_joy_list)(void);
    int         (*is_joy_pressed)(int joycode);
    void        (*poll_joysticks)(void);
    void        (*analogjoy_read)(int player, int *analog_x, int *analog_y);
    int         (*standard_analog_read)(int player, int axis);

    BOOL        (*Available)(int nJoyStick);
    BOOL        (*OnMessage)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
};

extern struct OSDJoystick Joystick;

/*
  limits:
  - 7 joysticks
  - 15 sticks on each joystick (15?)
  - 63 buttons on each joystick

  - 256 total inputs


   1 1 1 1 1 1
   5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  +---+-----------+---------+-----+
  |Dir|Axis/Button|   Stick | Joy |
  +---+-----------+---------+-----+

    Stick:  0 for buttons 1 for axis
    Joy:    1 for Mouse/track buttons

*/
#define JOYCODE(joy, stick, axis_or_button, dir) \
		((((dir)            & 0x03) << 14) |     \
         (((axis_or_button) & 0x3f) <<  8) |     \
         (((stick)          & 0x1f) <<  3) |     \
         (((joy)            & 0x07) <<  0))

#define GET_JOYCODE_JOY(code)    (((code) >> 0) & 0x07)
#define GET_JOYCODE_STICK(code)  (((code) >> 3) & 0x1f)
#define GET_JOYCODE_AXIS(code)   (((code) >> 8) & 0x3f)
#define GET_JOYCODE_BUTTON(code) GET_JOYCODE_AXIS(code)
#define GET_JOYCODE_DIR(code)    (((code) >>14) & 0x03)

#define JOYCODE_STICK_BTN    0
#define JOYCODE_STICK_AXIS   1
#define JOYCODE_STICK_POV    2

#define JOYCODE_DIR_BTN      0
#define JOYCODE_DIR_NEG      1
#define JOYCODE_DIR_POS      2

/* use otherwise unused joystick codes for the three mouse buttons */
#define MOUSE_BUTTON(button)    JOYCODE(1, JOYCODE_STICK_BTN, button, 3)


/* cmk temp */
enum
{
   OSD_JOY_LEFT,
   OSD_JOY_RIGHT,
   OSD_JOY_UP,
   OSD_JOY_DOWN,
   OSD_JOY_FIRE,
   OSD_JOY_FIRE1,
   OSD_JOY_FIRE2,
   OSD_JOY_FIRE3,
   OSD_JOY_FIRE4,
   OSD_MAX_JOY,
};


#endif
