//============================================================
//
//	secview.c - A Win32 sector editor dialog
//
//============================================================

#include <stdio.h>
#include <tchar.h>
#include "secview.h"
#include "wimgres.h"
#include "hexview.h"

struct sectorview_info
{
	imgtool_image *image;
	HFONT font;
	HWND hexview;
	HWND track_edit;
	HWND head_edit;
	HWND sector_edit;
	LONG bottom_margin;
};



static struct sectorview_info *get_sectorview_info(HWND dialog)
{
	LONG_PTR l;
	l = GetWindowLongPtr(dialog, GWLP_USERDATA);
	return (struct sectorview_info *) l;
}



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



static imgtoolerr_t read_sector_data(HWND dialog, UINT32 track, UINT32 head, UINT32 sector)
{
	imgtoolerr_t err;
	struct sectorview_info *info;
	UINT32 length;
	void *data;

	info = get_sectorview_info(dialog);

	err = img_getsectorsize(info->image, track, head, sector, &length);
	if (err)
		return err;

	data = alloca(length);
	err = img_readsector(info->image, track, head, sector, data, length);
	if (err)
		return err;

	if (!hexview_setdata(info->hexview, data, length))
		return IMGTOOLERR_OUTOFMEMORY;

	return IMGTOOLERR_SUCCESS;
}



static INT_PTR CALLBACK win_sectorview_dialog_proc(HWND dialog, UINT message,
	WPARAM wparam, LPARAM lparam)
{
	INT_PTR rc = 0;
	LONG_PTR l;
	struct sectorview_info *info;
	RECT dialog_rect;
	RECT hexedit_rect;
	LONG xmargin, dialog_width, dialog_height;

	switch(message)
	{
		case WM_INITDIALOG:
			info = (struct sectorview_info *) lparam;
			info->font = create_font();
			info->hexview = GetDlgItem(dialog, IDC_HEXVIEW);
			info->track_edit = GetDlgItem(dialog, IDC_TRACK);
			info->head_edit = GetDlgItem(dialog, IDC_HEAD);
			info->sector_edit = GetDlgItem(dialog, IDC_SECTOR);

			GetWindowRect(dialog, &dialog_rect);
			GetWindowRect(info->hexview, &hexedit_rect);
			info->bottom_margin = (dialog_rect.bottom - dialog_rect.top)
				- (hexedit_rect.bottom - hexedit_rect.top);

			SendMessage(info->hexview, WM_SETFONT, (WPARAM) info->font, (LPARAM) TRUE);
			SetWindowLongPtr(dialog, GWLP_USERDATA, lparam);

			read_sector_data(dialog, 0, 0, 1);
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
			break;

		case WM_SIZE:
			l = GetWindowLongPtr(dialog, GWLP_USERDATA);
			info = (struct sectorview_info *) l;

			GetWindowRect(dialog, &dialog_rect);
			GetWindowRect(info->hexview, &hexedit_rect);

			xmargin = hexedit_rect.left - dialog_rect.left;
			dialog_width = dialog_rect.right - dialog_rect.left;
			dialog_height = dialog_rect.bottom - dialog_rect.top;

			SetWindowPos(info->hexview, 0, 0, 0, dialog_width - xmargin * 2,
				dialog_height - info->bottom_margin, SWP_NOZORDER | SWP_NOMOVE);
			break;

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
