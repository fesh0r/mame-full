#include <windows.h>

#include "messcmds.h"
#include "inputx.h"

int win_use_natural_keyboard;

//============================================================
//	win_paste
//============================================================

void win_paste(void)
{
	HANDLE h;
	LPSTR text;
	size_t mb_size;
	size_t w_size;
	LPWSTR wtext;

	if (!OpenClipboard(NULL))
		return;
	
	h = GetClipboardData(CF_TEXT);
	if (h)
	{
		text = GlobalLock(h);
		if (text)
		{
			mb_size = GlobalSize(h);
			w_size = MultiByteToWideChar(CP_ACP, 0, text, mb_size, NULL, 0);
			wtext = alloca(w_size * sizeof(WCHAR));
			MultiByteToWideChar(CP_ACP, 0, text, mb_size, wtext, w_size);
			inputx_postn_utf16(wtext, w_size);
			GlobalUnlock(h);
		}
	}

	CloseClipboard();
}

//============================================================
//	win_input_natural
//============================================================

void win_input_natural(void)
{
	win_use_natural_keyboard = 1;
}

//============================================================
//	win_input_emulated
//============================================================

void win_input_emulated(void)
{
	win_use_natural_keyboard = 0;
}
