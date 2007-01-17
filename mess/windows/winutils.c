//============================================================
//
//	winutils.c - Generic Win32 utility code
//
//============================================================

#include "winutils.h"
#include "strconv.h"


//============================================================
//	win_message_box_utf8
//============================================================

int win_message_box_utf8(HWND window, const char *text, const char *caption, UINT type)
{
	int result = 0;
	TCHAR *t_text = NULL;
	TCHAR *t_caption = NULL;

	if (text)
	{
		t_text = tstring_from_utf8(text);
		if (!t_text)
			goto done;
	}

	if (caption)
	{
		t_caption = tstring_from_utf8(caption);
		if (!t_caption)
			goto done;
	}

	result = MessageBox(window, t_text, t_caption, type);

done:
	if (t_text)
		free(t_text);
	if (t_caption)
		free(t_caption);
	return result;
}



//============================================================
//	win_set_window_text_utf8
//============================================================

BOOL win_set_window_text_utf8(HWND window, const char *text)
{
	BOOL result = FALSE;
	TCHAR *t_text = NULL;

	if (text)
	{
		t_text = tstring_from_utf8(text);
		if (!t_text)
			goto done;
	}

	result = SetWindowText(window, t_text);

done:
	if (t_text)
		free(t_text);
	return result;
}



//============================================================
//	win_scroll_window
//============================================================

void win_scroll_window(HWND window, WPARAM wparam, int scroll_bar, int scroll_delta_line)
{
	SCROLLINFO si;
	int scroll_pos;

	// retrieve vital info about the scroll bar
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
	GetScrollInfo(window, scroll_bar, &si);

	scroll_pos = si.nPos;

	// change scroll_pos in accordance with this message
	switch(LOWORD(wparam)) 
	{
		case SB_BOTTOM:
			scroll_pos = si.nMax;
			break;
		case SB_LINEDOWN:
			scroll_pos += scroll_delta_line;
			break;
		case SB_LINEUP:
			scroll_pos -= scroll_delta_line;
			break;
		case SB_PAGEDOWN:
			scroll_pos += scroll_delta_line;
			break;
		case SB_PAGEUP:
			scroll_pos -= scroll_delta_line;
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			scroll_pos = HIWORD(wparam);
			break;
		case SB_TOP:
			scroll_pos = si.nMin;
			break;
	}

	// max out the scroll bar value
	if (scroll_pos < si.nMin)
		scroll_pos = si.nMin;
	else if (scroll_pos > (si.nMax - si.nPage))
		scroll_pos = (si.nMax - si.nPage);

	// if the value changed, set the scroll position
	if (scroll_pos != si.nPos)
	{
		SetScrollPos(window, scroll_bar, scroll_pos, TRUE);
		ScrollWindow(window, 0, si.nPos - scroll_pos, NULL, NULL);
	}
}



