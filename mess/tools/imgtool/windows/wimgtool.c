//============================================================
//
//	wimgtool.c - Win32 GUI Imgtool
//
//============================================================

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>

#include "wimgtool.h"
#include "wimgres.h"
#include "pile.h"
#include "pool.h"
#include "opcntrl.h"
#include "strconv.h"

const TCHAR wimgtool_class[] = TEXT("wimgtoolclass");

static TCHAR product_text[] = TEXT("MESS Image Tool");

struct wimgtool_info
{
	HWND listview;
	HWND statusbar;
	imgtool_image *image;
	char *filename;

	HIMAGELIST iconlist_normal;
	HIMAGELIST iconlist_small;
	mess_pile iconlist_extensions;
};



static void nyi(HWND window)
{
	MessageBox(window, TEXT("Not yet implemented"), product_text, MB_OK);
}



static struct wimgtool_info *get_wimgtool_info(HWND window)
{
	struct wimgtool_info *info;
	LONG_PTR l;
	l = GetWindowLongPtr(window, GWLP_USERDATA);
	info = (struct wimgtool_info *) l;
	return info;
}



static imgtoolerr_t get_selected_dirent(HWND window, imgtool_dirent *entry)
{
	struct wimgtool_info *info;
	int selected_item;
	imgtool_imageenum *imageenum = NULL;
	imgtoolerr_t err;
	
	info = get_wimgtool_info(window);
	if (!info->image)
	{
		err = IMGTOOLERR_UNEXPECTED;
		goto done;
	}
	selected_item = ListView_GetNextItem(info->listview, -1, LVIS_SELECTED | LVIS_FOCUSED);
	if (selected_item < 0)
	{
		err = IMGTOOLERR_UNEXPECTED;
		goto done;
	}

	err = img_beginenum(info->image, &imageenum);
	if (err)
		goto done;
	do
	{
		err = img_nextenum(imageenum, entry);
		if (err)
			goto done;
		if (entry->eof)
		{
			err = IMGTOOLERR_FILENOTFOUND;
			goto done;
		}
	}
	while(selected_item--);

done:
	if (err)
		memset(entry->filename, 0, entry->filename_len);
	if (imageenum)
		img_closeenum(imageenum);
	return err;
}



static void report_error(HWND window, imgtoolerr_t err)
{
	MessageBox(window, imgtool_error(err), product_text, MB_OK);
}



static int append_associated_icon(HWND window, const char *extension)
{
	HICON icon;
	WORD icon_index;
	TCHAR icon_path[MAX_PATH];
	int index = -1;
	struct wimgtool_info *info;

	info = get_wimgtool_info(window);

	_tcscpy(icon_path, TEXT("nul"));
	if (extension)
		_tcscat(icon_path, A2T(extension));

	icon = ExtractAssociatedIcon(NULL, icon_path, &icon_index);
	if (icon)
	{
		index = ImageList_AddIcon(info->iconlist_normal, icon);
		ImageList_AddIcon(info->iconlist_small, icon);
		DestroyIcon(icon);
	}
	return index;
}



static imgtoolerr_t append_dirent(HWND window, const imgtool_dirent *entry)
{
	LVITEM lvi;
	int new_index;
	struct wimgtool_info *info;
	TCHAR buffer[32];
	int icon_index;
	const char *extension;
	const char *ptr;
	size_t size, i;

	info = get_wimgtool_info(window);
	extension = strchr(entry->filename, '.');
	if (!extension)
		extension = ".bin";

	ptr = pile_getptr(&info->iconlist_extensions);
	size = pile_size(&info->iconlist_extensions);
	icon_index = 0;
	for (i = 0; i < size; i += strlen(&ptr[i]) + 1)
	{
		if (!stricmp(&ptr[i], extension))
			break;
		icon_index++;
	}

	if (i >= size)
	{
		icon_index = append_associated_icon(window, extension);
		if (icon_index < 0)
			return IMGTOOLERR_UNEXPECTED;
		if (pile_puts(&info->iconlist_extensions, extension))
			return IMGTOOLERR_OUTOFMEMORY;
		if (pile_putc(&info->iconlist_extensions, '\0'))
			return IMGTOOLERR_OUTOFMEMORY;
	}

	memset(&lvi, 0, sizeof(lvi));
	lvi.iItem = ListView_GetItemCount(info->listview);
	lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
	lvi.pszText = entry->filename;
	lvi.iImage = icon_index;
	new_index = ListView_InsertItem(info->listview, &lvi);

	_sntprintf(buffer, sizeof(buffer) / sizeof(buffer[0]),
		TEXT("%d"), entry->filesize);
	ListView_SetItemText(info->listview, new_index, 1, buffer);

	return 0;
}



static imgtoolerr_t refresh_image(HWND window)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	imgtool_imageenum *imageenum = NULL;
	char buf[256];
	imgtool_dirent entry;
	struct wimgtool_info *info;

	info = get_wimgtool_info(window);

	ListView_DeleteAllItems(info->listview);

	if (info->image)
	{
		err = img_beginenum(info->image, &imageenum);
		if (err)
			goto done;

		memset(&entry, 0, sizeof(entry));
		entry.filename = buf;
		entry.filename_len = sizeof(buf) / sizeof(buf[0]);
		do
		{
			err = img_nextenum(imageenum, &entry);
			if (err)
				goto done;

			if (entry.filename[0])
			{
				err = append_dirent(window, &entry);
				if (err)
					goto done;
			}
		}
		while(!entry.eof);
	}

done:
	if (imageenum)
		img_closeenum(imageenum);
	return err;
}



static imgtoolerr_t full_refresh_image(HWND window)
{
	struct wimgtool_info *info;
	LVCOLUMN col;
	const struct OptionGuide *guide;
	const struct ImageModule *module;
	int column_index = 0;
	char buf[256];

	info = get_wimgtool_info(window);

	module = info->image ? img_module(info->image) : NULL;
	if (info->filename)
	{
		SetWindowText(window, U2T(info->filename));
		snprintf(buf, sizeof(buf) / sizeof(buf[0]),
			"%s: %s", osd_basename((char *) info->filename), module->description);
		SetWindowText(info->statusbar, U2T(buf));
	}
	else
	{
		snprintf(buf, sizeof(buf) / sizeof(buf[0]),
			"%s %s", product_text, build_version);
		SetWindowText(window, U2T(buf));
		SetWindowText(info->statusbar, TEXT(""));
	}
	DragAcceptFiles(window, info->filename != NULL);

	// create the listview columns
	col.mask = LVCF_TEXT | LVCF_WIDTH;
	col.cx = 100;
	col.pszText = (LPTSTR) TEXT("Filename");
	if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
		return IMGTOOLERR_OUTOFMEMORY;
	col.pszText = (LPTSTR) TEXT("Size");
	if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
		return IMGTOOLERR_OUTOFMEMORY;

	// create module specific listview columns
	guide = module ? module->writefile_optguide : NULL;
	if (guide)
	{
		while(guide->option_type != OPTIONTYPE_END)
		{
			switch(guide->option_type)
			{
				case OPTIONTYPE_INT:
				case OPTIONTYPE_STRING:
				case OPTIONTYPE_ENUM_BEGIN:
					col.pszText = (LPTSTR) U2T(guide->display_name);
					if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
						return -1;
					break;

				default:
					break;
			}
			guide++;
		}
	}

	// delete extraneous columns
	while(ListView_DeleteColumn(info->listview, column_index))
		;
	return refresh_image(window);
}



static imgtoolerr_t setup_openfilename_struct(OPENFILENAME *ofn, memory_pool *pool,
	HWND window, int has_autodetect_option)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	mess_pile pile;
	const struct ImageModule *module = NULL;
	const char *s;
	TCHAR *filename;
	TCHAR *filter;

	memset(ofn, 0, sizeof(*ofn));
	pile_init(&pile);

	if (has_autodetect_option)
	{
		pile_puts(&pile, "Autodetect (*.*)");
		pile_putc(&pile, '\0');
		pile_puts(&pile, "*.*");
		pile_putc(&pile, '\0');
	}

	// write out library modules
	while((module = imgtool_library_iterate(library, module)) != NULL)
	{
		pile_puts(&pile, module->description);
		pile_puts(&pile, " (");

		s = module->extensions;
		while(*s)
		{
			if (s != module->extensions)
				pile_putc(&pile, ';');
			pile_printf(&pile, "*.%s", s);
			s += strlen(s) + 1;
		}
		pile_putc(&pile, ')');
		pile_putc(&pile, '\0');

		s = module->extensions;
		while(*s)
		{
			if (s != module->extensions)
				pile_putc(&pile, ';');
			pile_printf(&pile, "*.%s", s);
			s += strlen(s) + 1;
		}

		pile_putc(&pile, '\0');
	}
	pile_putc(&pile, '\0');
	pile_putc(&pile, '\0');

	filename = pool_malloc(pool, sizeof(TCHAR) * MAX_PATH);
	if (!filename)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	filename[0] = '\0';

	filter = pool_malloc(pool, pile_size(&pile));
	if (!filter)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	memcpy(filter, pile_getptr(&pile), pile_size(&pile));

	ofn->lStructSize = sizeof(*ofn);
	ofn->Flags = OFN_EXPLORER;
	ofn->hwndOwner = window;
	ofn->lpstrFile = filename;
	ofn->nMaxFile = MAX_PATH;
	ofn->lpstrFilter = filter;

done:
	pile_delete(&pile);
	return err;
}



static imgtoolerr_t open_image(HWND window, const struct ImageModule *module,
	const char *filename, int read_or_write)
{
	imgtoolerr_t err;
	imgtool_image *image;
	struct wimgtool_info *info;

	info = get_wimgtool_info(window);

	if (!module)
	{
		err = img_identify(library, filename, &module, 1);
		if (err)
			goto done;
	}

	info->filename = strdup(filename);
	if (!info->filename)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	
	err = img_open(module, filename, read_or_write, &image);
	if (err)
		goto done;

	if (info->image)
		img_close(info->image);
	info->image = image;

	// refresh the window
	full_refresh_image(window);
	
done:
	return err;
}



#define CONTROL_START 10000

struct new_dialog_info
{
	HWND parent;
	const struct ImageModule *module;
	int control_count;
	int margin;
	unsigned int expanded : 1;
};



static void adjust_dialog_height(HWND dlgwnd)
{
	struct new_dialog_info *info;
	HWND more_button;
	HWND control;
	RECT r1, r2, r3;
	int control_id;
	LONG_PTR l;

	l = GetWindowLongPtr(dlgwnd, GWLP_USERDATA);
	info = (struct new_dialog_info *) l;
	more_button = GetDlgItem(dlgwnd, IDC_MORE);

	if (!info->expanded || (info->control_count == 0))
		control_id = IDC_MORE;
	else
		control_id = CONTROL_START + info->control_count - 1;
	control = GetDlgItem(dlgwnd, control_id);
	assert(control);

	GetWindowRect(control, &r1);
	GetWindowRect(dlgwnd, &r2);
	GetWindowRect(info->parent, &r3);

	SetWindowPos(dlgwnd, NULL, 0, 0, r2.right - r2.left,
		r1.bottom + info->margin - r2.top, SWP_NOMOVE | SWP_NOZORDER);
	SetWindowPos(info->parent, NULL, 0, 0, r3.right - r3.left,
		r1.bottom + info->margin - r3.top, SWP_NOMOVE | SWP_NOZORDER);

	SetWindowText(more_button, info->expanded ? TEXT("Less <<") : TEXT("More >>"));
}



static UINT_PTR new_dialog_typechange(HWND dlgwnd, int filter_index)
{
	int label_width = 100;
	int control_height = 20;
	struct new_dialog_info *info;
	int i, x, y, height;
	const struct OptionGuide *guide;
	LONG_PTR l;
	HWND more_button;
	HWND control;
	LRESULT lres;
	DWORD style;
	RECT r1, r2;
	char buf[256];
	LPCTSTR class_name;
	HFONT font;

	l = GetWindowLongPtr(dlgwnd, GWLP_USERDATA);
	info = (struct new_dialog_info *) l;
	more_button = GetDlgItem(dlgwnd, IDC_MORE);

	info->module = imgtool_library_index(library, filter_index - 1);

	// clean out existing control windows
	for (i = 0; i < info->control_count; i++)
	{
		control = GetDlgItem(dlgwnd, CONTROL_START + i);
		if (control)
			DestroyWindow(control);
	}
	info->control_count = 0;

	guide = info->module->createimage_optguide;
	if (guide)
	{
		lres = SendMessage(more_button, WM_GETFONT, 0, 0);
		font = (HFONT) lres;
		GetWindowRect(more_button, &r1);
		GetWindowRect(dlgwnd, &r2);
		y = r1.bottom + info->margin - r2.top;

		for (i = 0; guide[i].option_type != OPTIONTYPE_END; i++)
		{
			snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%s:", guide[i].display_name);
			control = CreateWindow("STATIC", U2T(buf), WS_CHILD | WS_VISIBLE,
				r1.left - r2.left, y, label_width, control_height, dlgwnd, NULL, NULL, NULL);
			SetWindowLong(control, GWL_ID, CONTROL_START + info->control_count++);
			SendMessage(control, WM_SETFONT, (WPARAM) font, 0);

			switch(guide[i].option_type)
			{
				case OPTIONTYPE_STRING:
					class_name = TEXT("Edit");
					style = WS_CHILD | WS_VISIBLE;
					height = control_height;
					break;

				case OPTIONTYPE_INT:
				case OPTIONTYPE_ENUM_BEGIN:
					class_name = TEXT("ComboBox");
					style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST;
					height = control_height * 8;
					break;

				default:
					class_name = NULL;
					style = 0;
					height = 0;
					break;
			}
			if (class_name)
			{
				x = r1.left - r2.left + label_width;
				control = CreateWindow(class_name, NULL, style, x, y,
					r2.right - r2.left - x - (r1.left - r2.left), height, dlgwnd, NULL, NULL, NULL);
				SetWindowLong(control, GWL_ID, CONTROL_START + info->control_count++);
				SendMessage(control, WM_SETFONT, (WPARAM) font, 0);
				win_prepare_option_control(control, &guide[i], 
					info->module->createimage_optspec);
			}

			y += control_height;
		}
	}
	adjust_dialog_height(dlgwnd);
	return 0;
}



static UINT_PTR CALLBACK new_dialog_hook(HWND dlgwnd, UINT message,
	WPARAM wparam, LPARAM lparam)
{
	int i;
	UINT_PTR rc = 0;
	const OFNOTIFY *notify;
	struct new_dialog_info *info;
	const struct ImageModule *module;
	HWND control;
	RECT r1, r2;
	LONG_PTR l;
	option_resolution *resolution;
	HWND more_button;

	l = GetWindowLongPtr(dlgwnd, GWLP_USERDATA);
	info = (struct new_dialog_info *) l;

	more_button = GetDlgItem(dlgwnd, IDC_MORE);

	switch(message)
	{
		case WM_INITDIALOG:
			info = (struct new_dialog_info *) malloc(sizeof(*info));
			if (!info)
				return -1;
			memset(info, 0, sizeof(*info));
			SetWindowLongPtr(dlgwnd, GWLP_USERDATA, (LONG_PTR) info);

			info->parent = GetParent(dlgwnd);

			// compute lower margin
			GetWindowRect(more_button, &r1);
			GetWindowRect(dlgwnd, &r2);
			info->margin = r2.bottom - r1.bottom;

			// change dlgwnd height
			GetWindowRect(info->parent, &r1);
			SetWindowPos(dlgwnd, NULL, 0, 0, r1.right - r1.left, r2.bottom - r2.top, SWP_NOMOVE | SWP_NOZORDER);
			break;

		case WM_DESTROY:
			if (info)
				free(info);
			break;

		case WM_COMMAND:
			if (LOWORD(wparam) == IDC_MORE)
			{
				info->expanded = !info->expanded;
				adjust_dialog_height(dlgwnd);
			}
			break;

		case WM_NOTIFY:
			notify = (const OFNOTIFY *) lparam;
			switch(notify->hdr.code)
			{
				case CDN_INITDONE:
					rc = new_dialog_typechange(dlgwnd, notify->lpOFN->nFilterIndex);
					break;

				case CDN_FILEOK:
					module = info->module;
					resolution = NULL;

					if (module->createimage_optguide && module->createimage_optspec)
					{
						resolution = option_resolution_create(module->createimage_optguide, module->createimage_optspec);

						for (i = 1; i < info->control_count; i += 2)
						{
							control = GetDlgItem(dlgwnd, CONTROL_START + i);
							if (control)
								win_add_resolution_parameter(control, resolution);
						}

						option_resolution_finish(resolution);
					}
					*((option_resolution **) notify->lpOFN->lCustData) = resolution;
					break;

				case CDN_TYPECHANGE:
					rc = new_dialog_typechange(dlgwnd, notify->lpOFN->nFilterIndex);
					break;
			}
			break;
	}

	return rc;
}



static void menu_new(HWND window)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	memory_pool pool;
	OPENFILENAME ofn;
	const struct ImageModule *module;
	const char *filename;
	option_resolution *resolution = NULL;

	pool_init(&pool);

	err = setup_openfilename_struct(&ofn, &pool, window, FALSE);
	if (err)
		goto done;
	ofn.Flags |= OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
	ofn.hInstance = GetModuleHandle(NULL);
	ofn.lpTemplateName = MAKEINTRESOURCE(IDD_FILETEMPLATE);
	ofn.lpfnHook = new_dialog_hook;
	ofn.lCustData = (LPARAM) &resolution;
	if (!GetSaveFileName(&ofn))
		goto done;

	filename = T2A(ofn.lpstrFile);

	module = imgtool_library_index(library, ofn.nFilterIndex - 1);
	
	err = img_create(module, filename, resolution);
	if (err)
		goto done;

	err = open_image(window, module, filename, OSD_FOPEN_RW);
	if (err)
		goto done;

done:
	if (err)
		report_error(window, err);
	if (resolution)
		option_resolution_close(resolution);
	pool_exit(&pool);
}



static void menu_open(HWND window)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	memory_pool pool;
	OPENFILENAME ofn;
	const struct ImageModule *module = NULL;
	const char *filename;
	struct wimgtool_info *info;

	info = get_wimgtool_info(window);
	pool_init(&pool);

	err = setup_openfilename_struct(&ofn, &pool, window, TRUE);
	if (err)
		goto done;
	ofn.Flags |= OFN_FILEMUSTEXIST;
	if (!GetOpenFileName(&ofn))
		goto done;

	filename = T2A(ofn.lpstrFile);

	if (ofn.nFilterIndex > 2)
	{
		module = imgtool_library_index(library, ofn.nFilterIndex - 2);
	}

	err = open_image(window, module, filename, (ofn.Flags & OFN_READONLY)
		? OSD_FOPEN_READ : OSD_FOPEN_RW);
	if (err)
		goto done;

done:
	if (err)
		report_error(window, err);
	pool_exit(&pool);
}



static void menu_insert(HWND window)
{
	imgtoolerr_t err;
	const char *image_filename;
	TCHAR host_filename[MAX_PATH] = { 0 };
	const TCHAR *s;
	OPENFILENAME ofn;
	struct wimgtool_info *info;

	info = get_wimgtool_info(window);

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = host_filename;
	ofn.nMaxFile = sizeof(host_filename) / sizeof(host_filename[0]);
	if (!GetOpenFileName(&ofn))
	{
		err = 0;
		goto done;
	}

	s = _tcsrchr(ofn.lpstrFile, '\\');
	s = s ? s + 1 : ofn.lpstrFile;
	image_filename = T2A(s);

	err = img_putfile(info->image, NULL, ofn.lpstrFile, NULL, NULL);
	if (err)
		goto done;

	err = refresh_image(window);
	if (err)
		goto done;

done:
	if (err)
		report_error(window, err);
}



static void menu_extract(HWND window)
{
	imgtoolerr_t err;
	imgtool_dirent entry;
	char image_filename[MAX_PATH];
	TCHAR host_filename[MAX_PATH];
	OPENFILENAME ofn;
	struct wimgtool_info *info;

	info = get_wimgtool_info(window);

	memset(&entry, 0, sizeof(entry));
	entry.filename = image_filename;
	entry.filename_len = sizeof(image_filename) / sizeof(image_filename[0]);
	err = get_selected_dirent(window, &entry);
	if (err)
		goto done;

	strcpy(host_filename, image_filename);

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = host_filename;
	ofn.nMaxFile = sizeof(host_filename) / sizeof(host_filename[0]);
	if (!GetSaveFileName(&ofn))
		goto done;

	err = img_getfile(info->image, entry.filename, ofn.lpstrFile, NULL);
	if (err)
		goto done;

done:
	if (err)
		report_error(window, err);
}



static void menu_delete(HWND window)
{
	imgtoolerr_t err;
	imgtool_dirent entry;
	char image_filename[MAX_PATH];
	struct wimgtool_info *info;

	info = get_wimgtool_info(window);

	memset(&entry, 0, sizeof(entry));
	entry.filename = image_filename;
	entry.filename_len = sizeof(image_filename) / sizeof(image_filename[0]);
	err = get_selected_dirent(window, &entry);
	if (err)
		goto done;

	err = img_deletefile(info->image, image_filename);
	if (err)
		goto done;

	err = refresh_image(window);
	if (err)
		goto done;

done:
	if (err)
		report_error(window, err);
}



static void set_listview_style(HWND window, DWORD style)
{
	struct wimgtool_info *info;

	info = get_wimgtool_info(window);
	style &= LVS_TYPEMASK;
	style |= (GetWindowLong(info->listview, GWL_STYLE) & ~LVS_TYPEMASK);
	SetWindowLong(info->listview, GWL_STYLE, style);
}



static LRESULT wimgtool_create(HWND window, CREATESTRUCT *pcs)
{
	struct wimgtool_info *info;

	info = malloc(sizeof(*info));
	if (!info)
		return -1;
	memset(info, 0, sizeof(*info));
	pile_init(&info->iconlist_extensions);

	SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR) info);

	// create the list view
	info->listview = CreateWindow(WC_LISTVIEW, NULL,
		WS_VISIBLE | WS_CHILD, 0, 0, pcs->cx, pcs->cy, window, NULL, NULL, NULL);
	if (!info->listview)
		return -1;
	set_listview_style(window, LVS_REPORT);

	// create the status bar
	info->statusbar = CreateWindow(STATUSCLASSNAME, NULL, WS_VISIBLE | WS_CHILD,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, window, NULL, NULL, NULL);
	if (!info->statusbar)
		return -1;

	// create imagelists
	info->iconlist_normal = ImageList_Create(32, 32, ILC_COLOR, 0, 0);
	info->iconlist_small = ImageList_Create(16, 16, ILC_COLOR, 0, 0);
	if (!info->iconlist_normal || !info->iconlist_small)
		return -1;
	ListView_SetImageList(info->listview, info->iconlist_normal, LVSIL_NORMAL);
	ListView_SetImageList(info->listview, info->iconlist_small, LVSIL_SMALL);

	full_refresh_image(window);
	return 0;
}



static void drop_files(HWND window, HDROP drop)
{
	struct wimgtool_info *info;
	UINT count, i;
	TCHAR buffer[MAX_PATH];
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;

	info = get_wimgtool_info(window);

	count = DragQueryFile(drop, 0xFFFFFFFF, NULL, 0);
	for (i = 0; i < count; i++)
	{
		DragQueryFile(drop, i, buffer, sizeof(buffer) / sizeof(buffer[0]));

		err = img_putfile(info->image, NULL, buffer, NULL, NULL);
		if (err)
			goto done;
	}

done:
	refresh_image(window);
	if (err)
		report_error(window, err);
}



static LRESULT CALLBACK wimgtool_wndproc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	struct wimgtool_info *info;
	RECT window_rect;
	RECT status_rect;
	int window_width;
	int window_height;
	int status_height;

	info = get_wimgtool_info(window);

	switch(message)
	{
		case WM_CREATE:
			if (wimgtool_create(window, (CREATESTRUCT *) lparam))
				return -1;
			break;

		case WM_DESTROY:
			if (info)
			{
				if (info->image)
					img_close(info->image);
				pile_delete(&info->iconlist_extensions);
				free(info);
			}
			break;

		case WM_SIZE:
			GetClientRect(window, &window_rect);
			GetClientRect(info->statusbar, &status_rect);

			window_width = window_rect.right - window_rect.left;
			window_height = window_rect.bottom - window_rect.top;
			status_height = status_rect.bottom - status_rect.top;

			SetWindowPos(info->listview, NULL, 0, 0, window_width,
				window_height - status_height, SWP_NOMOVE | SWP_NOZORDER);
			SetWindowPos(info->statusbar, NULL, 0, window_height - status_height,
				window_width, status_height, SWP_NOMOVE | SWP_NOZORDER);
			break;

		case WM_INITMENU:
			EnableMenuItem((HMENU) wparam, ID_IMAGE_INSERT,
				MF_BYCOMMAND | (info->image ? MF_ENABLED : MF_GRAYED));
			EnableMenuItem((HMENU) wparam, ID_IMAGE_EXTRACT,
				MF_BYCOMMAND | (info->image ? MF_ENABLED : MF_GRAYED));
			EnableMenuItem((HMENU) wparam, ID_IMAGE_DELETE,
				MF_BYCOMMAND | (info->image ? MF_ENABLED : MF_GRAYED));
			break;

		case WM_DROPFILES:
			drop_files(window, (HDROP) wparam);
			break;

		case WM_COMMAND:
			switch(LOWORD(wparam))
			{
				case ID_FILE_NEW:
					menu_new(window);
					break;

				case ID_FILE_OPEN:
					menu_open(window);
					break;

				case ID_FILE_CLOSE:
					PostMessage(window, WM_CLOSE, 0, 0);
					break;

				case ID_IMAGE_INSERT:
					menu_insert(window);
					break;

				case ID_IMAGE_EXTRACT:
					menu_extract(window);
					break;

				case ID_IMAGE_DELETE:
					menu_delete(window);
					break;

				case ID_VIEW_ICONS:
					set_listview_style(window, LVS_ICON);
					break;

				case ID_VIEW_LIST:
					set_listview_style(window, LVS_LIST);
					break;

				case ID_VIEW_DETAILS:
					set_listview_style(window, LVS_REPORT);
					break;

			}
			break;
	}

	return DefWindowProc(window, message, wparam, lparam);
}



BOOL wimgtool_registerclass(void)
{
	WNDCLASS wimgtool_wndclass;

	memset(&wimgtool_wndclass, 0, sizeof(wimgtool_wndclass));
	wimgtool_wndclass.lpfnWndProc = wimgtool_wndproc;
	wimgtool_wndclass.lpszClassName = wimgtool_class;
	wimgtool_wndclass.lpszMenuName = MAKEINTRESOURCE(IDR_WIMGTOOL_MENU);
	return RegisterClass(&wimgtool_wndclass);
}



