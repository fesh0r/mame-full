#include "dialog.h"
#include "mame.h"
#include "../windows/window.h"

struct dialog_info
{
	HGLOBAL handle;
	WORD item_count;
	WORD cx, cy;
};

//============================================================
//	dialog_proc
//============================================================

static INT_PTR CALLBACK dialog_proc(HWND dlgwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	INT_PTR handled = TRUE;

	switch(msg) {
	case WM_INITDIALOG:
		break;

	default:
		handled = FALSE;
		break;
	}
	return handled;
}


//============================================================
//	dialog_write
//============================================================

static int dialog_write(struct dialog_info *di, const void *ptr, size_t sz, int align)
{
	HGLOBAL newhandle;
	size_t base;
	UINT8 *mem;
	UINT8 *mem2;

	if (!di->handle)
	{
		newhandle = GlobalAlloc(GMEM_ZEROINIT, sz);
		base = 0;
	}
	else
	{
		base = GlobalSize(di->handle);
		base += align - 1;
		base -= base % align;
		newhandle = GlobalReAlloc(di->handle, base + sz, GMEM_ZEROINIT);
		if (!newhandle)
		{
			newhandle = GlobalAlloc(GMEM_ZEROINIT, base + sz);
			if (newhandle)
			{
				mem = GlobalLock(di->handle);
				mem2 = GlobalLock(newhandle);
				memcpy(mem2, mem, base);
				GlobalUnlock(di->handle);
				GlobalUnlock(newhandle);
				GlobalFree(di->handle);
			}
		}
	}
	if (!newhandle)
		return 1;

	mem = GlobalLock(newhandle);
	memcpy(mem + base, ptr, sz);
	GlobalUnlock(newhandle);
	di->handle = newhandle;
	return 0;
}


//============================================================
//	dialog_write_string
//============================================================

static int dialog_write_string(struct dialog_info *di, const WCHAR *str)
{
	return dialog_write(di, str, (wcslen(str) + 1) * sizeof(WCHAR), 2);
}

//============================================================
//	dialog_prime
//============================================================

static void dialog_prime(struct dialog_info *di)
{
	DLGTEMPLATE *dlg_template;

	dlg_template = (DLGTEMPLATE *) GlobalLock(di->handle);
	dlg_template->cdit = di->item_count;
	dlg_template->cx = di->cx;
	dlg_template->cy = di->cy;
	GlobalUnlock(di->handle);
}

//============================================================
//	win_dialog_init
//============================================================

void *win_dialog_init(const WCHAR *title)
{
	struct dialog_info *di;
	DLGTEMPLATE dlg_template;
	WORD w[2];

	// create the dialog structure
	di = malloc(sizeof(struct dialog_info));
	if (!di)
		goto error;
	memset(di, 0, sizeof(*di));

	di->cx = 100;
	di->cy = 0;

	memset(&dlg_template, 0, sizeof(dlg_template));
	dlg_template.style = WS_POPUP | WS_BORDER | WS_SYSMENU | DS_MODALFRAME | WS_CAPTION;
	dlg_template.x = 10;
	dlg_template.y = 10;
	if (dialog_write(di, &dlg_template, sizeof(dlg_template), 4))
		goto error;

	w[0] = 0;
	w[1] = 0;
	if (dialog_write(di, w, sizeof(w), 2))
		goto error;

	if (dialog_write_string(di, title))
		goto error;

	return (void *) di;

error:
	if (di)
		win_dialog_exit(di);
	return NULL;
}


//============================================================
//	win_dialog_add_label
//============================================================

int win_dialog_add_label(void *dialog, const WCHAR *label)
{
	short vertical_spacing = 5;
	DLGITEMTEMPLATE item_template;
	struct dialog_info *di = (struct dialog_info *) dialog;
	WORD w[2];

	memset(&item_template, 0, sizeof(item_template));
	item_template.x = 10;
	item_template.y = di->cy + vertical_spacing;
	item_template.cx = 50;
	item_template.cy = 20;
	item_template.style = WS_CHILD | WS_VISIBLE | SS_LEFT;

	if (dialog_write(di, &item_template, sizeof(item_template), 4))
		goto error;

	w[0] = 0xffff;
	w[1] = 0x0082;
	if (dialog_write(di, w, sizeof(w), 2))
		goto error;

	if (dialog_write_string(di, label))
		goto error;

	w[0] = 0;
	if (dialog_write(di, w, sizeof(w[0]), 2))
		goto error;

	di->item_count++;
	di->cy += item_template.cy + (vertical_spacing * 2);

	return 0;
error:
	return 1;
}

//============================================================
//	win_dialog_exit
//============================================================

void win_dialog_exit(void *dialog)
{
	struct dialog_info *di = (struct dialog_info *) dialog;

	assert(di);
	if (di->handle)
		GlobalFree(di->handle);
	free(di);
}

//============================================================
//	win_dialog_runmodal
//============================================================

void win_dialog_runmodal(void *dialog)
{
	struct dialog_info *di;
	int res;
	
	di = (struct dialog_info *) dialog;
	assert(di);
	dialog_prime(di);
	res = DialogBoxIndirect(NULL, di->handle, win_video_window, dialog_proc);
	res = GetLastError();
}

