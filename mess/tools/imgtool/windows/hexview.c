//============================================================
//
//	hexview.c - A Win32 hex editor control
//
//============================================================

#include <stdio.h>
#include <tchar.h>
#include "hexview.h"

const TCHAR hexview_class[] = "hexview_class";


#ifndef GetWindowLongPtr
#define GetWindowLongPtr	GetWindowLong
#endif

#ifndef SetWindowLongPtr
#define SetWindowLongPtr	SetWindowLong
#endif

#ifndef GWLP_USERDATA
#define GWLP_USERDATA	GWL_USERDATA
#endif

#ifndef LONG_PTR
#define LONG_PTR LONG
#endif

#ifndef MAX
#define MAX(a, b)	((a > b) ? (a) : (b))
#endif

struct HexViewInfo
{
	void *data;
	size_t data_size;
	int index_width;
	int margin;
	HFONT font;
};



static struct HexViewInfo *get_hexview_info(HWND hexview)
{
	LONG_PTR l;
	l = GetWindowLongPtr(hexview, GWLP_USERDATA);
	return (struct HexViewInfo *) l;
}



static int hexview_create(HWND hexview, CREATESTRUCT *cs)
{
	struct HexViewInfo *info;

	info = (struct HexViewInfo *) malloc(sizeof(*info));
	if (!info)
		return -1;
	memset(info, '\0', sizeof(*info));

	info->index_width = 4;
	info->margin = 8;

	SetWindowLongPtr(hexview, GWLP_USERDATA, (LONG_PTR) info);
	return 0;
}



static void hexview_paint(HWND hexview)
{
	struct HexViewInfo *info;
	HDC dc;
	PAINTSTRUCT ps;
	LONG bytes_per_row;
	LONG begin_row, end_row, row, col, pos;
	TEXTMETRIC metrics;
	TCHAR buf[32];
	TCHAR offset_format[8];
	RECT r;
	BYTE b;

	info = get_hexview_info(hexview);
	dc = BeginPaint(hexview, &ps);

	if (info->font)
		SelectObject(dc, info->font);

	_sntprintf(offset_format, sizeof(offset_format) / sizeof(offset_format[0]),
		TEXT("%%0%dX"), info->index_width);

	// figure out how many bytes go in one row
	GetTextMetrics(dc, &metrics);
	GetWindowRect(hexview, &r);
	bytes_per_row = ((r.right - r.left) - (info->margin * 2) - metrics.tmMaxCharWidth * info->index_width)
		/ (metrics.tmMaxCharWidth * 3);
	bytes_per_row = MAX(bytes_per_row, 1);

	// draw the relevant rows
	begin_row = ps.rcPaint.top / metrics.tmHeight;
	end_row = (ps.rcPaint.bottom - 1) / metrics.tmHeight;
	for (row = begin_row; row <= end_row; row++)
	{
		_sntprintf(buf, sizeof(buf) / sizeof(buf[0]), offset_format, row * bytes_per_row);

		r.top = row * metrics.tmHeight;
		r.left = 0;
		r.bottom = r.top + metrics.tmHeight;
		r.right = r.left + metrics.tmMaxCharWidth * info->index_width;

		DrawText(dc, buf, -1, &r, DT_LEFT);

		for (col = 0; col < bytes_per_row; col++)
		{
			pos = row * bytes_per_row + col;
			if (pos < info->data_size)
			{
				b = ((BYTE *) info->data)[pos];

				r.left = (info->index_width + col * 2) * metrics.tmMaxCharWidth
					+ info->margin;
				r.right = r.left + metrics.tmMaxCharWidth * 2;
				_sntprintf(buf, sizeof(buf) / sizeof(buf[0]), TEXT("%02X"), b);
				DrawText(dc, buf, -1, &r, DT_LEFT);

				r.left = (info->index_width + bytes_per_row * 2 + col) * metrics.tmMaxCharWidth
					+ (info->margin * 2);
				r.right = r.left + metrics.tmMaxCharWidth * 2;
				buf[0] = ((b >= 0x20) && (b <= 0x7F)) ? (TCHAR) b : '.';
				DrawText(dc, buf, 1, &r, DT_LEFT);
			}
		}
	}

	
	EndPaint(hexview, &ps);
}



static LRESULT CALLBACK hexview_wndproc(HWND hexview, UINT message, WPARAM wparam, LPARAM lparam)
{
	struct HexViewInfo *info;

	info = get_hexview_info(hexview);
	
	switch(message)
	{
		case WM_CREATE:
			if (hexview_create(hexview, (CREATESTRUCT *) lparam))
				return -1;
			break;

		case WM_DESTROY:
			if (info->data)
				free(info->data);
			free(info);
			break;

		case WM_PAINT:
			hexview_paint(hexview);
			break;

		case WM_SIZE:
			InvalidateRect(hexview, NULL, TRUE);
			break;

		case WM_SETFONT:
			info->font = (HFONT) wparam;
			if (lparam)
				RedrawWindow(hexview, NULL, NULL, 0);
			return 0;

		case WM_GETFONT:
			return (LRESULT) info->font;
	}
	return DefWindowProc(hexview, message, wparam, lparam);
}



BOOL hexview_setdata(HWND hexview, const void *data, size_t data_size)
{
	struct HexViewInfo *info;
	void *data_copy;

	info = get_hexview_info(hexview);

	data_copy = malloc(data_size);
	if (!data_copy)
		return FALSE;
	memcpy(data_copy, data, data_size);

	if (info->data)
		free(info->data);
	info->data = data_copy;
	info->data_size = data_size;

	InvalidateRect(hexview, NULL, TRUE);
	return TRUE;
}



BOOL hexview_registerclass()
{
	WNDCLASS hexview_wndclass;

	memset(&hexview_wndclass, 0, sizeof(hexview_wndclass));
	hexview_wndclass.lpfnWndProc = hexview_wndproc;
	hexview_wndclass.lpszClassName = hexview_class;
	hexview_wndclass.hbrBackground = GetStockObject(WHITE_BRUSH);
	return RegisterClass(&hexview_wndclass);
}

