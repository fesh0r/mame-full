//============================================================
//
//	win32.h - Win32 prefix file, included by ALL files
//
//============================================================

// missing math constants
#ifndef _MSC_VER
#define PI				3.1415927
#define M_PI			PI
#endif

#ifdef _MSC_VER
#define NONAMELESSUNION
#define HAS_DUMMYUNIONNAME 1
#define DX_SDK 8

#pragma warning (disable:4244)
#pragma warning (disable:4005)
#pragma warning (disable:4018)
#pragma warning (disable:4022)
#pragma warning (disable:4068)
#pragma warning (disable:4090)
#pragma warning (disable:4146)
#pragma warning (disable:4305)
#pragma warning (disable:4761)
#pragma warning (disable:4078)
#pragma warning (disable:4096)
#pragma warning (disable:4007)

#define DIDEVTYPE_MOUSE     2
#define DIDEVTYPE_KEYBOARD  3
#define DIDEVTYPE_JOYSTICK  4
#define DD_OK				0

#endif
