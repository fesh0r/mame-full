#include "dialog.h"
#include "mame.h"
#include "../windows/window.h"

struct dialog_info_setupmsg
{
	struct dialog_info_setupmsg *next;
	int dialog_item;
	UINT message;
	WPARAM wparam;
	LPARAM lparam;
};

struct dialog_info
{
	HGLOBAL handle;
	struct dialog_info_setupmsg *setup_messages;
	struct dialog_info_setupmsg *setup_messages_last;
	WORD item_count;
	WORD cx, cy;
	int combo_string_count;
	int combo_default_value;
};

//============================================================

#define DIM_VERTICAL_SPACING	2
#define DIM_HORIZONTAL_SPACING	2
#define DIM_ROW_HEIGHT			12
#define DIM_LABEL_WIDTH			80
#define DIM_COMBO_WIDTH			140

//============================================================
//	dialog_proc
//============================================================

static INT_PTR CALLBACK dialog_proc(HWND dlgwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	HWND dialog_item;
	INT_PTR handled = TRUE;
	struct dialog_info *di;
	struct dialog_info_setupmsg *setup_msg;

	switch(msg) {
	case WM_INITDIALOG:
		di = (struct dialog_info *) lparam;
		SetWindowLong(dlgwnd, 0, (LONG_PTR) di);

		for (setup_msg = di->setup_messages; setup_msg; setup_msg = setup_msg->next)
		{
			dialog_item = GetDlgItem(dlgwnd, setup_msg->dialog_item);
			SendMessage(dialog_item, setup_msg->message, setup_msg->wparam, setup_msg->lparam);
		}
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

static int dialog_write_string(struct dialog_info *di, const char *str)
{
	int sz;
	WCHAR *wstr;
	
	sz = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	wstr = alloca(sz * sizeof(WCHAR));
	MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, sz);
	return dialog_write(di, wstr, sz * sizeof(WCHAR), 2);
}

//============================================================
//	dialog_write_item
//============================================================

static int dialog_write_item(struct dialog_info *di, DWORD style, short x, short y,
	 short cx, short cy, const char *str, WORD class_atom)
{
	DLGITEMTEMPLATE item_template;
	WORD w[2];

	memset(&item_template, 0, sizeof(item_template));
	item_template.style = style;
	item_template.x = x;
	item_template.y = y;
	item_template.cx = cx;
	item_template.cy = cy;
	item_template.id = di->item_count + 1;

	if (dialog_write(di, &item_template, sizeof(item_template), 4))
		return 1;

	w[0] = 0xffff;
	w[1] = class_atom;
	if (dialog_write(di, w, sizeof(w), 2))
		return 1;

	if (dialog_write_string(di, str))
		return 1;

	w[0] = 0;
	if (dialog_write(di, w, sizeof(w[0]), 2))
		return 1;

	di->item_count++;
	return 0;
}

//============================================================
//	dialog_add_setup_message
//============================================================

static int dialog_add_setup_message(struct dialog_info *di, WORD dialog_item,
	UINT message, WPARAM wparam, LPARAM lparam)
{
	struct dialog_info_setupmsg *setupmsg;

	setupmsg = malloc(sizeof(struct dialog_info_setupmsg));
	if (!setupmsg)
		return 1;

	setupmsg->next = NULL;
	setupmsg->dialog_item = dialog_item;
	setupmsg->message = message;
	setupmsg->wparam = wparam;
	setupmsg->lparam = lparam;

	if (di->setup_messages_last)
		di->setup_messages_last->next = setupmsg;
	else
		di->setup_messages = setupmsg;
	di->setup_messages_last = setupmsg;
	return 0;
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

void *win_dialog_init(const char *title)
{
	struct dialog_info *di;
	DLGTEMPLATE dlg_template;
	WORD w[2];

	// create the dialog structure
	di = malloc(sizeof(struct dialog_info));
	if (!di)
		goto error;
	memset(di, 0, sizeof(*di));

	di->cx = 0;
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
//	win_dialog_add_combobox
//============================================================

int win_dialog_add_combobox(void *dialog, const char *label, int default_value)
{
	struct dialog_info *di = (struct dialog_info *) dialog;
	short x;
	short y;

	x = DIM_HORIZONTAL_SPACING;
	y = di->cy + DIM_VERTICAL_SPACING;

	if (dialog_write_item(di, WS_CHILD | WS_VISIBLE | SS_LEFT,
			x, y, DIM_LABEL_WIDTH, DIM_ROW_HEIGHT, label, 0x0082))
		return 1;

	x += DIM_LABEL_WIDTH + DIM_HORIZONTAL_SPACING;
	if (dialog_write_item(di, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
			x, y, DIM_COMBO_WIDTH, DIM_ROW_HEIGHT * 8, "", 0x0085))
		return 1;
	di->combo_string_count = 0;
	di->combo_default_value = default_value;

	x += DIM_COMBO_WIDTH + DIM_HORIZONTAL_SPACING;

	if (x > di->cx)
		di->cx = x;
	di->cy += DIM_ROW_HEIGHT + DIM_VERTICAL_SPACING * 2;
	return 0;
}

//============================================================
//	win_dialog_add_combobox_item
//============================================================

int win_dialog_add_combobox_item(void *dialog, const char *item_label, int item_data)
{
	struct dialog_info *di = (struct dialog_info *) dialog;
	if (dialog_add_setup_message(di, di->item_count, CB_ADDSTRING, 0, (LPARAM) item_label))
		return 1;
	di->combo_string_count++;
	if (dialog_add_setup_message(di, di->item_count, CB_SETITEMDATA, di->combo_string_count-1, (LPARAM) item_data))
		return 1;
	if (item_data == di->combo_default_value)
	{
		if (dialog_add_setup_message(di, di->item_count, CB_SETCURSEL, di->combo_string_count-1, 0))
			return 1;
	}
	return 0;
}


//============================================================
//	win_dialog_exit
//============================================================

void win_dialog_exit(void *dialog)
{
	struct dialog_info *di = (struct dialog_info *) dialog;
	struct dialog_info_setupmsg *setupmsg;
	struct dialog_info_setupmsg *next;

	assert(di);
	if (di->handle)
		GlobalFree(di->handle);

	setupmsg = di->setup_messages;
	while(setupmsg)
	{
		next = setupmsg->next;
		free(setupmsg);
		setupmsg = next;
	}
	free(di);
}

//============================================================
//	win_dialog_runmodal
//============================================================

void win_dialog_runmodal(void *dialog)
{
	struct dialog_info *di;
	
	di = (struct dialog_info *) dialog;
	assert(di);
	dialog_prime(di);
	DialogBoxIndirectParam(NULL, di->handle, win_video_window, dialog_proc, (LPARAM) di);
}

