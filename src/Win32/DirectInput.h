/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __DIRECTINPUT_H__
#define __DIRECTINPUT_H__

#include <dinput.h>

extern LPDIRECTINPUT di;

extern BOOL DirectInputInitialize(void);
extern void DirectInputClose(void);

extern BOOL CALLBACK inputEnumDeviceProc(LPDIDEVICEINSTANCE pdidi, LPVOID pv);

extern HRESULT SetDIDwordProperty(LPDIRECTINPUTDEVICE2 pdev, REFGUID guidProperty,
                  DWORD dwObject, DWORD dwHow, DWORD dwValue);

#endif
