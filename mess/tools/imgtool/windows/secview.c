//============================================================
//
//	secview.c - A Win32 sector editor control
//
//============================================================

#include <stdio.h>
#include <tchar.h>
#include "secview.h"
#include "wimgres.h"

const TCHAR sectorview_class[] = "sectorview_class";

struct sectorview_info
{
	imgtool_image *image;
	HFONT font;
	HWND hexview;
	HWND track_edit;
	HWND head_edit;
	HWND sector_edit;
};



static HFONT create_font(void)
{
	HDC temp_dc;
	HFONT font;

	temp_dc = GetDC(NULL);

	font = CreateFont(-MulDiv(8, GetDeviceCaps(temp_dc, LOGPIXELSY), 72), 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
				ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, TEXT("Lucida Console"));
	if (temp_dc)
		ReleaseDC(NULL, temp_dc);
	return font;
}



static INT_PTR CALLBACK win_sectorview_dialog_proc(HWND dialog, UINT message,
	WPARAM wparam, LPARAM lparam)
{
	INT_PTR rc = 0;
	LONG_PTR l;
	struct sectorview_info *info;

	switch(message)
	{
		case WM_INITDIALOG:
			info = (struct sectorview_info *) lparam;
			info->font = create_font();
			info->hexview = GetDlgItem(dialog, IDC_HEXVIEW);
			info->track_edit = GetDlgItem(dialog, IDC_TRACK);
			info->head_edit = GetDlgItem(dialog, IDC_HEAD);
			info->sector_edit = GetDlgItem(dialog, IDC_SECTOR);
			SendMessage(info->hexview, WM_SETFONT, (WPARAM) info->font, (LPARAM) TRUE);
			SetWindowLongPtr(dialog, GWLP_USERDATA, lparam);
			break;

		case WM_DESTROY:
			l = GetWindowLongPtr(dialog, GWLP_USERDATA);
			info = (struct sectorview_info *) l;
			if (info->font)
			{
				DeleteObject(info->font);
				info->font = NULL;
			}
			break;

		case WM_SYSCOMMAND:
			if (wparam == SC_CLOSE)
				EndDialog(dialog, 0);
			break;

		case WM_COMMAND:
			if (HIWORD(wparam) == BN_CLICKED)
			{
				switch(LOWORD(wparam))
				{
					case IDOK:
					case IDCANCEL:
						EndDialog(dialog, 0);
						break;
				}
			}

	}

	return rc;
}



void win_sectorview_dialog(HWND parent, imgtool_image *image)
{
	struct sectorview_info info;
	
	memset(&info, 0, sizeof(info));
	info.image = image;

	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SECTORVIEW), parent,
		win_sectorview_dialog_proc, (LPARAM) &info);
}
