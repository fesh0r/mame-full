#ifndef EMUGX_H
#define EMUGX_H

#ifdef _X86_
#define HAS_EMUGX	1
#else
#define HAS_EMUGX	0
#endif

#if HAS_EMUGX
int __cdecl emu_GXOpenDisplay(HWND hWnd, DWORD dwFlags);
int __cdecl emu_GXCloseDisplay(void);
void* __cdecl emu_GXBeginDraw(void);
int __cdecl emu_GXEndDraw(void);
int __cdecl emu_GXOpenInput();
int __cdecl emu_GXCloseInput();
GXDisplayProperties __cdecl emu_GXGetDisplayProperties();
GXKeyList __cdecl emu_GXGetDefaultKeys(int iOptions);
int __cdecl emu_GXSuspend();
int __cdecl emu_GXResume();
#endif /* HAS_EMUGX */

#endif /* EMUGX_H */
