//============================================================
//
//	emugx.cpp - GAPI emulation routines
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <gx.h>
#include <assert.h>

#include "emugx.h"

#if HAS_EMUGX

//============================================================
//	PARAMETERS
//============================================================

#define EMUGX_WIDTH		240
#define EMUGX_HEIGHT	320

//============================================================
//	LOCAL VARIABLES
//============================================================

struct bitmap_mode
{
	int depth;
	DWORD format;
	DWORD colors[3];
};

static const struct bitmap_mode bitmap_modes[] =
{
	{ 16, kfDirect565, { 0x0000f800, 0x000007e0, 0x0000001f } },
	{ 16, kfDirect555, { 0x00007c00, 0x000003e0, 0x0000001f } },
	{ 24, kfDirect888, { 0x00ff0000, 0x0000ff00, 0x000000ff } }
};

static HWND emulatedgapi_window;
static HDC emulatedgapi_dc;
static HBITMAP emulatedgapi_bitmap;
static const struct bitmap_mode *emulatedgapi_bitmap_mode;
static HDC emulatedgapi_bitmap_dc;
static void *emulatedgapi_bits;

//============================================================
//	acquire
//============================================================

static int acquire(void)
{
	PBITMAPINFO pbmi;
	int i;
	size_t pbmi_size;

	emulatedgapi_dc = GetDC(emulatedgapi_window);

	pbmi_size = sizeof(BITMAPINFOHEADER) + (sizeof(DWORD) * 3);
	pbmi = (PBITMAPINFO) alloca(pbmi_size);
	memset(pbmi, 0, pbmi_size);

	pbmi->bmiHeader.biSize = sizeof(pbmi->bmiHeader);
	pbmi->bmiHeader.biWidth = EMUGX_WIDTH;
	pbmi->bmiHeader.biHeight = -EMUGX_HEIGHT;
	pbmi->bmiHeader.biPlanes = 1;
	pbmi->bmiHeader.biCompression = BI_BITFIELDS;

	for (i = 0; i < sizeof(bitmap_modes) / sizeof(bitmap_modes[0]); i++)
	{
		emulatedgapi_bitmap_mode = &bitmap_modes[i];

		pbmi->bmiHeader.biBitCount = emulatedgapi_bitmap_mode->depth;
		memcpy(pbmi->bmiColors, emulatedgapi_bitmap_mode->colors, sizeof(emulatedgapi_bitmap_mode->colors));

		emulatedgapi_bitmap = CreateDIBSection(emulatedgapi_dc, pbmi, DIB_RGB_COLORS, &emulatedgapi_bits, NULL, 0);
		if (emulatedgapi_bitmap)
			break;
	}
	if (!emulatedgapi_bitmap)
		return 0;
	memset(emulatedgapi_bits, 0, EMUGX_WIDTH * EMUGX_HEIGHT * emulatedgapi_bitmap_mode->depth / 8);

	emulatedgapi_bitmap_dc = CreateCompatibleDC(emulatedgapi_dc);
	SelectObject(emulatedgapi_bitmap_dc, emulatedgapi_bitmap);
	return 1;
}

//============================================================
//	unacquire
//============================================================

static void unacquire(void)
{
	if (emulatedgapi_bitmap_dc)
	{
		DeleteDC(emulatedgapi_bitmap_dc);
		emulatedgapi_bitmap_dc = NULL;
	}
	if (emulatedgapi_bitmap)
	{
		DeleteObject(emulatedgapi_bitmap);
		emulatedgapi_bitmap = NULL;
		emulatedgapi_bits = NULL;
	}
	if (emulatedgapi_dc)
	{
		ReleaseDC(emulatedgapi_window, emulatedgapi_dc);
		emulatedgapi_dc = NULL;
	}
}

//============================================================

int __cdecl emu_GXOpenDisplay(HWND hWnd, DWORD dwFlags)
{
	if (emulatedgapi_window)
		return 0;

	emulatedgapi_window = hWnd;
	if (!acquire())
	{
		emulatedgapi_window = NULL;
		return 0;
	}
	return 1;
}

int __cdecl emu_GXCloseDisplay(void)
{
	unacquire();
	emulatedgapi_window = NULL;
	return 1;
}

void* __cdecl emu_GXBeginDraw(void)
{
	return emulatedgapi_bits;
}

int __cdecl emu_GXEndDraw(void)
{
	BitBlt(emulatedgapi_dc,
		0, 0, EMUGX_WIDTH, EMUGX_HEIGHT,
		emulatedgapi_bitmap_dc,
		0, 0,
		SRCCOPY);
	return 1;
}

int __cdecl emu_GXOpenInput()
{
	return 1;
}

int __cdecl emu_GXCloseInput()
{
	return 1;
}

GXDisplayProperties __cdecl emu_GXGetDisplayProperties()
{
	GXDisplayProperties display;
	memset(&display, 0, sizeof(display));
	display.cxWidth = EMUGX_WIDTH;
	display.cyHeight = EMUGX_HEIGHT;
	display.cbxPitch = emulatedgapi_bitmap_mode->depth / 8;
	display.cbyPitch = EMUGX_WIDTH * display.cbxPitch;
	display.cBPP = emulatedgapi_bitmap_mode->depth;
	display.ffFormat = emulatedgapi_bitmap_mode->format;
	return display;
}

GXKeyList __cdecl emu_GXGetDefaultKeys(int iOptions)
{
	GXKeyList keylist;
	memset(&keylist, 0, sizeof(keylist));
	return keylist;
}

int __cdecl emu_GXSuspend()
{
	unacquire();
	return 1;
}

int __cdecl emu_GXResume()
{
	return acquire();
}

//============================================================

#endif // HAS_EMUGX
