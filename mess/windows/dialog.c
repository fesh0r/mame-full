//============================================================
//
//	dialog.c - Win32 MESS dialogs handling
//
//============================================================

#include <windows.h>

#include "dialog.h"
#include "mame.h"
#include "../windows/window.h"
#include "ui_text.h"
#include "inputx.h"
#include "utils.h"
#include "strconv.h"
#include "mscommon.h"

#ifdef UNDER_CE
#include "invokegx.h"
#endif

//============================================================
//	These defines are necessary because the MinGW headers do
//	not have the latest definitions
//============================================================

#ifndef _WIN64
#ifndef GetWindowLongPtr
#define GetWindowLongPtr(hwnd, idx)			((LONG_PTR) GetWindowLong((hwnd), (idx)))
#endif

#ifndef SetWindowLongPtr
#define SetWindowLongPtr(hwnd, idx, val)	((LONG_PTR) SetWindowLong((hwnd), (idx), (val)))
#endif

#ifndef GWLP_USERDATA
#define GWLP_USERDATA						GWL_USERDATA
#endif

#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC						GWL_WNDPROC
#endif
#endif /* _WIN64 */

//============================================================

enum
{
	TRIGGER_INITDIALOG	= 1,
	TRIGGER_APPLY		= 2
};

typedef LRESULT (*trigger_function)(HWND dlgwnd, UINT message, WPARAM wparam, LPARAM lparam);

struct dialog_info_trigger
{
	struct dialog_info_trigger *next;
	WORD dialog_item;
	WORD trigger_flags;
	UINT message;
	WPARAM wparam;
	LPARAM lparam;
	UINT16 *result;
	trigger_function trigger_proc;
};

struct dialog_object_pool
{
	HGDIOBJ objects[16];
};

struct _dialog_box
{
	HGLOBAL handle;
	size_t handle_size;
	struct dialog_info_trigger *trigger_first;
	struct dialog_info_trigger *trigger_last;
	WORD item_count;
	WORD size_x, size_y;
	WORD pos_x, pos_y;
	WORD cursize_x, cursize_y;
	WORD home_y;
	int combo_string_count;
	int combo_default_value;
	memory_pool mempool;
	struct dialog_object_pool *objpool;
};

//============================================================
//	IMPORTS
//============================================================

// from input.c
extern void win_poll_input(void);


//============================================================
//	PARAMETERS
//============================================================

#define DIM_VERTICAL_SPACING	2
#define DIM_HORIZONTAL_SPACING	2
#define DIM_NORMAL_ROW_HEIGHT	10
#define DIM_COMBO_ROW_HEIGHT	12
#define DIM_BUTTON_ROW_HEIGHT	12
#define DIM_LABEL_WIDTH			80
#define DIM_SEQ_WIDTH			120
#define DIM_COMBO_WIDTH			140
#define DIM_BUTTON_WIDTH		50

#define MAX_DIALOG_HEIGHT		480

#define WNDLONG_DIALOG			GWLP_USERDATA

#define DLGITEM_BUTTON			0x0080
#define DLGITEM_EDIT			0x0081
#define DLGITEM_STATIC			0x0082
#define DLGITEM_LISTBOX			0x0083
#define DLGITEM_SCROLLBAR		0x0084
#define DLGITEM_COMBOBOX		0x0085

#define DLGTEXT_OK				ui_getstring(UI_OK)
#define DLGTEXT_APPLY			"Apply"
#define DLGTEXT_CANCEL			"Cancel"

#define FONT_SIZE				8
#define FONT_FACE				"Microsoft Sans Serif"

#define TIMER_ID				0xdeadbeef

//============================================================
//	dialog_trigger
//============================================================

static void dialog_trigger(HWND dlgwnd, WORD trigger_flags)
{
	LRESULT result;
	HWND dialog_item;
	struct _dialog_box *di;
	struct dialog_info_trigger *trigger;
	LONG l;

	l = GetWindowLongPtr(dlgwnd, WNDLONG_DIALOG);
	di = (struct _dialog_box *) l;
	assert(di);
	for (trigger = di->trigger_first; trigger; trigger = trigger->next)
	{
		if (trigger->trigger_flags & trigger_flags)
		{
			dialog_item = GetDlgItem(dlgwnd, trigger->dialog_item);
			assert(dialog_item);
			result = 0;

			if (trigger->message)
				result = SendMessage(dialog_item, trigger->message, trigger->wparam, trigger->lparam);
			if (trigger->trigger_proc)
				result = trigger->trigger_proc(dialog_item, trigger->message, trigger->wparam, trigger->lparam);

			if (trigger->result)
				*(trigger->result) = result;
		}
	}
}

//============================================================
//	dialog_proc
//============================================================

static INT_PTR CALLBACK dialog_proc(HWND dlgwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	INT_PTR handled = TRUE;
	TCHAR buf[32];
	const char *str;
	WORD command;

	switch(msg) {
	case WM_INITDIALOG:
		SetWindowLongPtr(dlgwnd, WNDLONG_DIALOG, (LONG_PTR) lparam);
		dialog_trigger(dlgwnd, TRIGGER_INITDIALOG);
		break;

	case WM_COMMAND:
		command = LOWORD(wparam);

		GetWindowText((HWND) lparam, buf, sizeof(buf) / sizeof(buf[0]));
		str = T2A(buf);
		if (!strcmp(str, DLGTEXT_OK))
			command = IDOK;
		else if (!strcmp(str, DLGTEXT_CANCEL))
			command = IDCANCEL;
		else
			command = 0;

		switch(command) {
		case IDOK:
			dialog_trigger(dlgwnd, TRIGGER_APPLY);
			/* fall through */

		case IDCANCEL:
			EndDialog(dlgwnd, 0);
			break;

		default:
			handled = FALSE;
			break;
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

static int dialog_write(struct _dialog_box *di, const void *ptr, size_t sz, int align)
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
		base = di->handle_size;
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
	di->handle_size = base + sz;
	return 0;
}


//============================================================
//	dialog_write_string
//============================================================

static int dialog_write_string(dialog_box *di, const char *str)
{
	const WCHAR *wstr;
	if (!str)
		str = "";
	wstr = A2W(str);	
	return dialog_write(di, wstr, (wcslen(wstr) + 1) * sizeof(WCHAR), 2);
}

//============================================================
//	dialog_write_item
//============================================================

static int dialog_write_item(dialog_box *di, DWORD style, short x, short y,
	 short width, short height, const char *str, WORD class_atom, WORD *id)
{
	DLGITEMTEMPLATE item_template;
	WORD w[2];

	memset(&item_template, 0, sizeof(item_template));
	item_template.style = style;
	item_template.x = x;
	item_template.y = y;
	item_template.cx = width;
	item_template.cy = height;
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

	if (id)
		*id = di->item_count;
	return 0;
}

//============================================================
//	dialog_add_trigger
//============================================================

static int dialog_add_trigger(struct _dialog_box *di, WORD dialog_item,
	WORD trigger_flags, UINT message, trigger_function trigger_proc,
	WPARAM wparam, LPARAM lparam, UINT16 *result)
{
	struct dialog_info_trigger *trigger;

	assert(di);
	assert(trigger_flags);

	trigger = (struct dialog_info_trigger *) pool_malloc(&di->mempool, sizeof(struct dialog_info_trigger));
	if (!trigger)
		return 1;

	trigger->next = NULL;
	trigger->trigger_flags = trigger_flags;
	trigger->dialog_item = dialog_item;
	trigger->message = message;
	trigger->trigger_proc = trigger_proc;
	trigger->wparam = wparam;
	trigger->lparam = lparam;
	trigger->result = result;

	if (di->trigger_last)
		di->trigger_last->next = trigger;
	else
		di->trigger_first = trigger;
	di->trigger_last = trigger;
	return 0;
}

//============================================================
//	dialog_add_object
//============================================================

static int dialog_add_object(dialog_box *di, HGDIOBJ obj)
{
	int i;
	struct dialog_object_pool *objpool;

	objpool = di->objpool;

	if (!di->objpool)
	{
		objpool = malloc(sizeof(struct dialog_object_pool));
		if (!objpool)
			return 1;
		memset(objpool, 0, sizeof(struct dialog_object_pool));
		di->objpool = objpool;
	}


	for (i = 0; i < sizeof(objpool->objects) / sizeof(objpool->objects[0]); i++)
	{
		if (!objpool->objects[i])
			break;
	}
	if (i >= sizeof(objpool->objects) / sizeof(objpool->objects[0]))
		return 1;

	objpool->objects[i] = obj;
	return 0;
}

//============================================================
//	dialog_prime
//============================================================

static void dialog_prime(dialog_box *di)
{
	DLGTEMPLATE *dlg_template;

	dlg_template = (DLGTEMPLATE *) GlobalLock(di->handle);
	dlg_template->cdit = di->item_count;
	dlg_template->cx = di->size_x;
	dlg_template->cy = di->size_y;
	GlobalUnlock(di->handle);
}

//============================================================
//	dialog_get_combo_value
//============================================================

static LRESULT dialog_get_combo_value(HWND dialog_item, UINT message, WPARAM wparam, LPARAM lparam)
{
	int idx;
	idx = SendMessage(dialog_item, CB_GETCURSEL, 0, 0);
	if (idx == CB_ERR)
		return 0;
	return SendMessage(dialog_item, CB_GETITEMDATA, idx, 0);
}

//============================================================
//	compute_dlgunits_multiple
//============================================================

static void compute_dlgunits_multiple(float *xratio, float *yratio)
{
	dialog_box *dialog;
	short offset_x = 2048;
	short offset_y = 2048;
	const char *wnd_title = "Foo";
	WORD id;
	HWND dlg_window, child_window;
	RECT r;
	
	dialog = win_dialog_init(NULL);
	if (!dialog)
		return;

	if (dialog_write_item(dialog, WS_CHILD | WS_VISIBLE | SS_LEFT,
			0, 0, offset_x, offset_y, wnd_title, DLGITEM_STATIC, &id))
		return;

	dialog_prime(dialog);
	dlg_window = CreateDialogIndirectParam(NULL, dialog->handle, win_video_window, NULL, 0);
	child_window = GetDlgItem(dlg_window, id);
	GetWindowRect(child_window, &r);
	CloseWindow(dlg_window);

	win_dialog_exit(dialog);

	*xratio = (float)(r.right - r.left) / offset_x;
	*yratio = (float)(r.bottom - r.top) / offset_y;
}

//============================================================
//	pixels_to_dlgunits
//============================================================

static void pixels_to_dlgunits(WORD *x, WORD *y)
{
	static float xratio, yratio;

	if ((xratio == 0) || (yratio == 0))
	{
		compute_dlgunits_multiple(&xratio, &yratio);
		if ((xratio == 0) || (yratio == 0))
			return;
	}
	*x /= xratio;
	*y /= yratio;
}

//============================================================
//	win_dialog_init
//============================================================

dialog_box *win_dialog_init(const char *title)
{
	struct _dialog_box *di;
	DLGTEMPLATE dlg_template;
	WORD w[2];

	// create the dialog structure
	di = malloc(sizeof(struct _dialog_box));
	if (!di)
		goto error;
	memset(di, 0, sizeof(*di));

	pool_init(&di->mempool);

	memset(&dlg_template, 0, sizeof(dlg_template));
	dlg_template.style = WS_POPUP | WS_BORDER | WS_SYSMENU | DS_MODALFRAME | WS_CAPTION | DS_SETFONT;
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

	w[0] = FONT_SIZE;
	if (dialog_write(di, w, sizeof(w[0]), 2))
		goto error;
	if (dialog_write_string(di, FONT_FACE))
		goto error;

	return di;

error:
	if (di)
		win_dialog_exit(di);
	return NULL;
}



//============================================================
//	dialog_new_control
//============================================================

static void dialog_new_control(struct _dialog_box *di, short *x, short *y)
{
	if (di->pos_y >= MAX_DIALOG_HEIGHT)
	{
		di->pos_x = di->cursize_x;
		di->pos_y = di->home_y;
	}

	*x = di->pos_x + DIM_HORIZONTAL_SPACING;
	*y = di->pos_y + DIM_VERTICAL_SPACING;
}



//============================================================
//	dialog_finish_control
//============================================================

static void dialog_finish_control(struct _dialog_box *di, short x, short y)
{
	di->pos_y = y;

	/* update dialog size */
	if (x > di->size_x)
		di->size_x = x;
	if (y > di->size_y)
		di->size_y = y;
	if (x > di->cursize_x)
		di->cursize_x = x;
	if (y > di->cursize_y)
		di->cursize_y = y;
}



//============================================================
//	win_dialog_add_combobox
//============================================================

int win_dialog_add_combobox(dialog_box *dialog, const char *item_label, UINT16 *value)
{
	dialog_box *di = dialog;
	short x;
	short y;

	assert(item_label);

	dialog_new_control(di, &x, &y);

	if (dialog_write_item(di, WS_CHILD | WS_VISIBLE | SS_LEFT,
			x, y, DIM_LABEL_WIDTH, DIM_COMBO_ROW_HEIGHT, item_label, DLGITEM_STATIC, NULL))
		return 1;

	x += DIM_LABEL_WIDTH + DIM_HORIZONTAL_SPACING;
	if (dialog_write_item(di, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
			x, y, DIM_COMBO_WIDTH, DIM_COMBO_ROW_HEIGHT * 8, "", DLGITEM_COMBOBOX, NULL))
		return 1;
	di->combo_string_count = 0;
	di->combo_default_value = *value;

	if (dialog_add_trigger(di, di->item_count, TRIGGER_APPLY, 0, dialog_get_combo_value, 0, 0, value))
		return 1;

	x += DIM_COMBO_WIDTH + DIM_HORIZONTAL_SPACING;
	y += DIM_COMBO_ROW_HEIGHT + DIM_VERTICAL_SPACING * 2;

	dialog_finish_control(di, x, y);
	return 0;
}

//============================================================
//	win_dialog_add_combobox_item
//============================================================

int win_dialog_add_combobox_item(dialog_box *dialog, const char *item_label, int item_data)
{
	dialog_box *di = dialog;
	if (dialog_add_trigger(di, di->item_count, TRIGGER_INITDIALOG, CB_ADDSTRING, NULL, 0, (LPARAM) item_label, NULL))
		return 1;
	di->combo_string_count++;
	if (dialog_add_trigger(di, di->item_count, TRIGGER_INITDIALOG, CB_SETITEMDATA, NULL, di->combo_string_count-1, (LPARAM) item_data, NULL))
		return 1;
	if (item_data == di->combo_default_value)
	{
		if (dialog_add_trigger(di, di->item_count, TRIGGER_INITDIALOG, CB_SETCURSEL, NULL, di->combo_string_count-1, 0, NULL))
			return 1;
	}
	return 0;
}

//============================================================
//	seqselect_wndproc
//============================================================

struct seqselect_stuff
{
	WNDPROC oldwndproc;
	InputSeq *code;
	InputSeq newcode;
	UINT_PTR timer;
	WORD pos;
};

static INT_PTR CALLBACK seqselect_wndproc(HWND editwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	struct seqselect_stuff *stuff;
	INT_PTR result;
	int dlgitem;
	HWND dlgwnd;
	HWND dlgitemwnd;
	InputCode code;
	LONG_PTR lp;

	lp = GetWindowLongPtr(editwnd, GWLP_USERDATA);
	stuff = (struct seqselect_stuff *) lp;

	switch(msg) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_CHAR:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		result = 1;
		break;

	case WM_TIMER:
		if (wparam == TIMER_ID)
		{
			win_poll_input();
			code = code_read_async();
			if (code != CODE_NONE)
			{
				seq_set_1(&stuff->newcode, code);
				SetWindowText(editwnd, A2T(code_name(code)));

				dlgwnd = GetParent(editwnd);

				dlgitem = stuff->pos;
				do
				{
					dlgitem++;
					dlgitemwnd = GetDlgItem(dlgwnd, dlgitem);
				}
				while(dlgitemwnd && (GetWindowLongPtr(dlgitemwnd, GWLP_WNDPROC) != (LONG_PTR) seqselect_wndproc));
				if (dlgitemwnd)
				{
					SetFocus(dlgitemwnd);
					SendMessage(dlgitemwnd, EM_SETSEL, 0, -1);
				}
				else
				{
					SetFocus(dlgwnd);
				}
			}
			result = 0;
		}
		else
		{
			result = CallWindowProc(stuff->oldwndproc, editwnd, msg, wparam, lparam);
		}
		break;

	case WM_SETFOCUS:
		if (stuff->timer)
			KillTimer(editwnd, stuff->timer);
		stuff->timer = SetTimer(editwnd, TIMER_ID, 100, (TIMERPROC) NULL);
		result = CallWindowProc(stuff->oldwndproc, editwnd, msg, wparam, lparam);
		break;

	case WM_KILLFOCUS:
		if (stuff->timer)
		{
			KillTimer(editwnd, stuff->timer);
			stuff->timer = 0;
		}
		result = CallWindowProc(stuff->oldwndproc, editwnd, msg, wparam, lparam);
		break;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		SetFocus(editwnd);
		SendMessage(editwnd, EM_SETSEL, 0, -1);
		result = 0;
		break;

	default:
		result = CallWindowProc(stuff->oldwndproc, editwnd, msg, wparam, lparam);
		break;
	}
	return result;
}

//============================================================
//	seqselect_setup
//============================================================

static LRESULT seqselect_setup(HWND editwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	char buf[256];
	struct seqselect_stuff *stuff = (struct seqselect_stuff *) lparam;
	LONG_PTR lp;

	memcpy(stuff->newcode, *(stuff->code), sizeof(stuff->newcode));
	seq_name(stuff->code, buf, sizeof(buf) / sizeof(buf[0]));
	SetWindowText(editwnd, A2T(buf));
	lp = SetWindowLongPtr(editwnd, GWLP_WNDPROC, (LONG_PTR) seqselect_wndproc);
	stuff->oldwndproc = (WNDPROC) lp;
	SetWindowLongPtr(editwnd, GWLP_USERDATA, lparam);
	return 0;
}

//============================================================
//	seqselect_apply
//============================================================

static LRESULT seqselect_apply(HWND editwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	struct seqselect_stuff *stuff;
	LONG_PTR lp;

	lp = GetWindowLongPtr(editwnd, GWLP_USERDATA);
	stuff = (struct seqselect_stuff *) lp;
	memcpy(*(stuff->code), stuff->newcode, sizeof(*(stuff->code)));
	return 0;
}

//============================================================
//	dialog_add_single_seqselect
//============================================================

static int dialog_add_single_seqselect(struct _dialog_box *di, short x, short y,
	short cx, short cy, struct InputPort *port)
{
	struct seqselect_stuff *stuff;
	InputSeq *code;

	code = input_port_seq(port);

	if (dialog_write_item(di, WS_CHILD | WS_VISIBLE | SS_ENDELLIPSIS | ES_CENTER | SS_SUNKEN,
			x, y, cx, cy, "", DLGITEM_EDIT, NULL))
		return 1;
	stuff = (struct seqselect_stuff *) pool_malloc(&di->mempool, sizeof(struct seqselect_stuff));
	if (!stuff)
		return 1;
	stuff->code = code;
	stuff->pos = di->item_count;
	stuff->timer = 0;
	if (dialog_add_trigger(di, di->item_count, TRIGGER_INITDIALOG, 0, seqselect_setup, di->item_count, (LPARAM) stuff, NULL))
		return 1;
	if (dialog_add_trigger(di, di->item_count, TRIGGER_APPLY, 0, seqselect_apply, 0, 0, NULL))
		return 1;
	return 0;
}

//============================================================
//	win_dialog_add_seqselect
//============================================================

int win_dialog_add_portselect(dialog_box *dialog, struct InputPort *port, RECT *r)
{
	dialog_box *di = dialog;
	short x;
	short y;
	short height;
	short width;
	const char *port_name;

	port_name = input_port_name(port);

	if (!r)
	{
		dialog_new_control(di, &x, &y);

		if (dialog_write_item(di, WS_CHILD | WS_VISIBLE | SS_LEFT, x, y, 
				DIM_LABEL_WIDTH, DIM_NORMAL_ROW_HEIGHT, port_name, DLGITEM_STATIC, NULL))
			return 1;
		x += DIM_LABEL_WIDTH + DIM_HORIZONTAL_SPACING;

		if (dialog_add_single_seqselect(di, x, y, DIM_SEQ_WIDTH, DIM_NORMAL_ROW_HEIGHT, port))
			return 1;
		y += DIM_VERTICAL_SPACING + DIM_NORMAL_ROW_HEIGHT;
		x += DIM_SEQ_WIDTH + DIM_HORIZONTAL_SPACING;

		dialog_finish_control(di, x, y);
	}
	else
	{
		x = r->left;
		y = r->top;
		width = r->right - r->left;
		height = r->bottom - r->top;

		pixels_to_dlgunits(&x, &y);
		pixels_to_dlgunits(&width, &height);

		if (dialog_add_single_seqselect(di, x, y, width, height, port))
			return 1;
	}
	return 0;
}

//============================================================
//	win_dialog_add_standard_buttons
//============================================================

int win_dialog_add_standard_buttons(dialog_box *dialog)
{
	dialog_box *di = dialog;
	short x;
	short y;

	x = di->size_x - DIM_HORIZONTAL_SPACING - DIM_BUTTON_WIDTH;
	y = di->size_y + DIM_VERTICAL_SPACING;

	if (dialog_write_item(di, WS_CHILD | WS_VISIBLE | SS_LEFT,
			x, y, DIM_BUTTON_WIDTH, DIM_BUTTON_ROW_HEIGHT, DLGTEXT_CANCEL, DLGITEM_BUTTON, NULL))
		return 1;

	x -= DIM_HORIZONTAL_SPACING + DIM_BUTTON_WIDTH;
	if (dialog_write_item(di, WS_CHILD | WS_VISIBLE | SS_LEFT,
			x, y, DIM_BUTTON_WIDTH, DIM_BUTTON_ROW_HEIGHT, DLGTEXT_OK, DLGITEM_BUTTON, NULL))
		return 1;
	di->size_y += DIM_BUTTON_ROW_HEIGHT + DIM_VERTICAL_SPACING * 2;
	return 0;
}

//============================================================
//	create_png_bitmap
//============================================================

static HBITMAP create_png_bitmap(const struct png_info *png)
{
	HBITMAP bitmap;
	BITMAPINFOHEADER bitmap_header;
	UINT8 *bitmap_data;
	UINT8 *src;
	UINT8 *dst;
	int x, y;
	HDC dc;
		
	memset(&bitmap_header, 0, sizeof(bitmap_header));
	bitmap_header.biSize = sizeof(BITMAPINFOHEADER);
	bitmap_header.biWidth = png->width;
	bitmap_header.biHeight = -png->height;
	bitmap_header.biPlanes = 1;
	bitmap_header.biBitCount = 24;
	bitmap_header.biCompression = BI_RGB;

	bitmap_data = alloca(png->width * png->height * 3);
	src = png->image;
	dst = bitmap_data;
	for (y = 0; y < png->height; y++)
	{
		for (x = 0; x < png->width; x++)
		{
			switch(png->color_type) {
			case 0:		// 8bpp grayscale case
				dst[2] = src[0];
				dst[1] = src[0];
				dst[0] = src[0];
				src++;
				break;

			case 2:		// 32bpp non-alpha case
				dst[2] = src[0];
				dst[1] = src[1];
				dst[0] = src[2];
				src += 3;
				break;

			case 3:		// 8bpp palettized case
				assert(png->palette);
				dst[2] = png->palette[*src * 3 + 0];
				dst[1] = png->palette[*src * 3 + 1];
				dst[0] = png->palette[*src * 3 + 2];
				src++;
				break;
			}
			dst += 3;
		}
	}

	dc = GetDC(NULL);
	bitmap = CreateDIBitmap(dc, &bitmap_header, CBM_INIT, bitmap_data, (BITMAPINFO *) &bitmap_header, DIB_RGB_COLORS);
	ReleaseDC(NULL, dc);
	return bitmap;
}

//============================================================
//	win_dialog_add_image
//============================================================

int win_dialog_add_image(dialog_box *dialog, const struct png_info *png)
{
	WORD id;
	HBITMAP bitmap;
	short x, y;
	short width, height;

	width = png->width;
	height = png->height;
	pixels_to_dlgunits(&width, &height);

	bitmap = create_png_bitmap(png);
	if (!bitmap)
		return 1;

	dialog_new_control(dialog, &x, &y);
	if (dialog_write_item(dialog, WS_CHILD | WS_VISIBLE | SS_LEFT | SS_BITMAP,
			x, y, width, height, "", DLGITEM_STATIC, &id))
		return 1;
	if (dialog_add_trigger(dialog, id, TRIGGER_INITDIALOG, STM_SETIMAGE, NULL, (WPARAM)IMAGE_BITMAP, (LPARAM) bitmap, NULL))
		return 1;
	if (dialog_add_object(dialog, bitmap))
		return 1;
	x += width;
	y += height;
	dialog_finish_control(dialog, x, y);
	return 0;
}

//============================================================
//	win_dialog_add_separator
//============================================================

int win_dialog_add_separator(dialog_box *dialog)
{
	dialog->home_y = dialog->cursize_y;
	dialog->cursize_x = 0;
	return 0;
}

//============================================================
//	win_dialog_exit
//============================================================

void win_dialog_exit(dialog_box *dialog)
{
	int i;
	struct dialog_object_pool *objpool;

	assert(dialog);

	objpool = dialog->objpool;
	if (objpool)
	{
		for (i = 0; i < sizeof(objpool->objects) / sizeof(objpool->objects[0]); i++)
			DeleteObject(objpool->objects[i]);
		free(objpool);
	}

	if (dialog->handle)
		GlobalFree(dialog->handle);
	pool_exit(&dialog->mempool);
	free(dialog);
}

//============================================================
//	win_dialog_runmodal
//============================================================

void win_dialog_runmodal(dialog_box *dialog)
{
	struct _dialog_box *di;
	extern void win_timer_enable(int enabled);
	
	di = (struct _dialog_box *) dialog;
	assert(di);

	// finishing touches on the dialog
	dialog_prime(di);

	// disable sound while in the dialog
	osd_sound_enable(0);

	// disable the timer while in the dialog
	win_timer_enable(0);

#ifdef UNDER_CE
	// on WinCE, suspend GAPI
	gx_suspend();
#endif

	DialogBoxIndirectParam(NULL, di->handle, win_video_window, dialog_proc, (LPARAM) di);

#ifdef UNDER_CE
	// on WinCE, resume GAPI
	gx_resume();
#endif

	// reenable timer
	win_timer_enable(1);

	// reenable sound
	osd_sound_enable(1);
}

