/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef DIJOYSTICK_H
#define DIJOYSTICK_H

#include "joystick.h"

extern struct OSDJoystick DIJoystick;

extern BOOL DIJoystick_IsHappInterface(void);
extern BOOL DIJoystick_KeyPressed(int osd_key);

extern int   DIJoystick_GetNumPhysicalJoysticks(void);
extern char* DIJoystick_GetPhysicalJoystickName(int num_joystick);

extern int   DIJoystick_GetNumPhysicalJoystickAxes(int num_joystick);
extern char* DIJoystick_GetPhysicalJoystickAxisName(int num_joystick, int num_axis);

#endif
