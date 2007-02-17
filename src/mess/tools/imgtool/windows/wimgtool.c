//============================================================
//
//	wimgtool.c - Win32 GUI Imgtool
//
//============================================================

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>
#include <ctype.h>

#include "wimgtool.h"
#include "wimgres.h"
#include "pile.h"
#include "pool.h"
#include "strconv.h"
#include "attrdlg.h"
#include "secview.h"
#include "winutil.h"

const TCHAR wimgtool_class[] = TEXT("wimgtool_class");
const TCHAR wimgtool_producttext[] = TEXT("MESS Image Tool");

extern void win_association_dialog(HWND parent);

typedef struct _wimgtool_info wimgtool_info;
struct _wimgtool_info
{
	HWND listview;					// handle to list view
	HWND statusbar;					// handle to the status bar
	imgtool_image *image;			// the currently loaded image
	imgtool_partition *partition;	// the currently loaded partition

	char *filename;
	int open_mode;
	char *current_directory;

	HIMAGELIST iconlist_normal;
	HIMAGELIST iconlist_small;
	mess_pile iconlist_extensions;

	HICON readonly_icon;
	int readonly_icon_index;
	int directory_icon_index;

	HIMAGELIST dragimage;
	POINT dragpt;
};



static wimgtool_info *get_wimgtool_info(HWND window)
{
	wimgtool_info *info;
	LONG_PTR l;
	l = GetWindowLongPtr(window, GWLP_USERDATA);
	info = (wimgtool_info *) l;
	return info;
}



struct foreach_entry
{
	imgtool_dirent dirent;
	struct foreach_entry *next;
};

static imgtoolerr_t foreach_selected_item(HWND window,
	imgtoolerr_t (*proc)(HWND, const imgtool_dirent *, void *), void *param)
{
	wimgtool_info *info;
	imgtoolerr_t err;
	int selected_item;
	int selected_index = -1;
	char *s;
	LVITEM item;
	struct foreach_entry *first_entry = NULL;
	struct foreach_entry *last_entry = NULL;
	struct foreach_entry *entry;
	
	info = get_wimgtool_info(window);

	if (info->image)
	{
		do
		{
			do
			{
				selected_index = ListView_GetNextItem(info->listview, selected_index, LVIS_SELECTED);
				if (selected_index < 0)
				{
					selected_item = -1;
				}
				else
				{
					item.mask = LVIF_PARAM;
					item.iItem = selected_index;
					ListView_GetItem(info->listview, &item);
					selected_item = item.lParam;
				}
			}
			while((selected_index >= 0) && (selected_item < 0));

			if (selected_item >= 0)
			{
				entry = alloca(sizeof(*entry));
				entry->next = NULL;

				// retrieve the directory entry
				err = imgtool_partition_get_directory_entry(info->partition, info->current_directory, selected_item, &entry->dirent);
				if (err)
					return err;

				// if we have a path, prepend the path 
				if (info->current_directory && info->current_directory[0])
				{
					char path_separator = (char) imgtool_partition_get_info_int(info->partition, IMGTOOLINFO_INT_PATH_SEPARATOR);

					// create a copy of entry->dirent.filename
					s = (char *) alloca(strlen(entry->dirent.filename) + 1);
					strcpy(s, entry->dirent.filename);

					// copy the full path back in
					snprintf(entry->dirent.filename, ARRAY_LENGTH(entry->dirent.filename),
						"%s%c%s", info->current_directory, path_separator, s);
				}

				// append to list
				if (last_entry)
					last_entry->next = entry;
				else
					first_entry = entry;
				last_entry = entry;
			}
		}
		while(selected_item >= 0);
	}

	// now that we have the list, call the callbacks
	for (entry = first_entry; entry; entry = entry->next)
	{
		err = proc(window, &entry->dirent, param);
		if (err)
			return err;
	}
	return IMGTOOLERR_SUCCESS;
}



void wimgtool_report_error(HWND window, imgtoolerr_t err, const char *imagename, const char *filename)
{
	const char *error_text;
	const char *source;
	char buffer[512];
	const char *message = buffer;

	error_text = imgtool_error(err);

	switch(ERRORSOURCE(err))
	{
		case IMGTOOLERR_SRC_IMAGEFILE:
			source = osd_basename((char *) imagename);
			break;
		case IMGTOOLERR_SRC_FILEONIMAGE:
			source = filename;
			break;
		default:
			source = NULL;
			break;
	}

	if (source)
		snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%s: %s", source, error_text);
	else
		message = error_text;
	win_message_box_utf8(window, message, wimgtool_producttext, MB_OK);
}



static HICON create_icon(int width, int height, const UINT32 *icondata)
{
	HDC dc = NULL;
	HICON icon = NULL;
	BYTE *color_bits, *mask_bits;
	HBITMAP color_bitmap = NULL, mask_bitmap = NULL;
	ICONINFO iconinfo;
	UINT32 pixel;
	UINT8 mask;
	int x, y;
	BITMAPINFO bmi;

	// we need a device context
	dc = CreateCompatibleDC(NULL);
	if (!dc)
		goto done;

	// create foreground bitmap
	memset(&bmi, 0, sizeof(bmi));
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	color_bitmap = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, (void **) &color_bits, NULL, 0);
	if (!color_bitmap)
		goto done;

	// create mask bitmap
	memset(&bmi, 0, sizeof(bmi));
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 1;
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	mask_bitmap = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, (void **) &mask_bits, NULL, 0);
	if (!color_bitmap)
		goto done;

	// transfer data from our structure to the bitmaps
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			mask = 1 << (7 - (x % 8));
			pixel = icondata[y * width + x];

			// foreground icon
			color_bits[((height - y - 1) * width + x) * 3 + 2] = (pixel >> 16);
			color_bits[((height - y - 1) * width + x) * 3 + 1] = (pixel >>  8);
			color_bits[((height - y - 1) * width + x) * 3 + 0] = (pixel >>  0);

			// mask
			if (pixel & 0x80000000)
				mask_bits[((height - y - 1) * width + x) / 8] &= ~mask;
			else
				mask_bits[((height - y - 1) * width + x) / 8] |= mask;
		}
	}

	// actually create the icon
	memset(&iconinfo, 0, sizeof(iconinfo));
	iconinfo.fIcon = TRUE;
	iconinfo.hbmColor = color_bitmap;
	iconinfo.hbmMask = mask_bitmap;
	icon = CreateIconIndirect(&iconinfo);

done:
	if (color_bitmap)
		DeleteObject(color_bitmap);
	if (mask_bitmap)
		DeleteObject(mask_bitmap);
	if (dc)
		DeleteDC(dc);
	return icon;
}



static imgtoolerr_t hicons_from_imgtool_icon(const imgtool_iconinfo *iconinfo,
    HICON *icon16x16, HICON *icon32x32)
{
	*icon16x16 = NULL;
	*icon32x32 = NULL;

	if (iconinfo->icon16x16_specified)
	{
	    *icon16x16 = create_icon(16, 16, (const UINT32 *) iconinfo->icon16x16);
		if (!*icon16x16)
	        return IMGTOOLERR_OUTOFMEMORY;
	}

	if (iconinfo->icon32x32_specified)
	{
	    *icon32x32 = create_icon(32, 32, (const UINT32 *) iconinfo->icon32x32);
		if (!*icon32x32)
	        return IMGTOOLERR_OUTOFMEMORY;
	}

    return IMGTOOLERR_SUCCESS;
}



#define FOLDER_ICON	((const char *) ~0)

static int append_associated_icon(HWND window, const char *extension)
{
	HICON icon;
	HANDLE file = INVALID_HANDLE_VALUE;
	WORD icon_index;
	TCHAR file_path[MAX_PATH];
	TCHAR *t_extension;
	int index = -1;
	wimgtool_info *info;

	info = get_wimgtool_info(window);

	/* retrieve temporary file path */
	GetTempPath(sizeof(file_path) / sizeof(file_path[0]), file_path);

	if (extension != FOLDER_ICON)
	{
		/* create bogus temporary file so that we can get the icon */
		_tcscat(file_path, TEXT("tmp"));
		if (extension)
		{
			t_extension = tstring_from_utf8(extension);
			_tcscat(file_path, t_extension);
			free(t_extension);
		}

		file = CreateFile(file_path, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
	}

	/* extract the icon */
	icon = ExtractAssociatedIcon(GetModuleHandle(NULL), file_path, &icon_index);
	if (icon)
	{
		index = ImageList_AddIcon(info->iconlist_normal, icon);
		ImageList_AddIcon(info->iconlist_small, icon);
		DestroyIcon(icon);
	}

	/* remote temporary file if we created one */
	if (file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(file);
		DeleteFile(file_path);
	}

	return index;
}



static imgtoolerr_t append_dirent(HWND window, int index, const imgtool_dirent *entry)
{
	LVITEM lvi;
	int new_index, column_index;
	wimgtool_info *info;
	TCHAR buffer[32];
	int icon_index = -1;
	const char *extension;
	const char *ptr;
	const char *s;
	size_t size, i;
	imgtool_partition_features features;
	struct tm *local_time;
	TCHAR *t_entry_filename;

	info = get_wimgtool_info(window);
	features = imgtool_partition_get_features(info->partition);

	/* try to get a custom icon */
	if (features.supports_geticoninfo)
	{
		char buf[256];
		HICON icon16x16, icon32x32;
		imgtool_iconinfo iconinfo;

		sprintf(buf, "%s%s", info->current_directory, entry->filename);
		imgtool_partition_get_icon_info(info->partition, buf, &iconinfo);

		hicons_from_imgtool_icon(&iconinfo, &icon16x16, &icon32x32);
		if (icon16x16 || icon32x32)
		{
			icon_index = ImageList_AddIcon(info->iconlist_normal, icon32x32);
			ImageList_AddIcon(info->iconlist_small, icon32x32);
		}
	}

	if (icon_index < 0)
	{
		if (entry->directory)
		{
			icon_index = info->directory_icon_index;
		}
		else
		{
			extension = strrchr(entry->filename, '.');
			if (!extension)
				extension = ".bin";

			ptr = pile_getptr(&info->iconlist_extensions);
			size = pile_size(&info->iconlist_extensions);
			icon_index = -1;

			i = 0;
			while(i < size)
			{
				s = &ptr[i];
				i += strlen(&ptr[i]) + 1;
				memcpy(&icon_index, &ptr[i], sizeof(icon_index));
				i += sizeof(icon_index);

				if (!mame_stricmp(s, extension))
					break;
			}

			if (i >= size)
			{
				icon_index = append_associated_icon(window, extension);
				if (pile_puts(&info->iconlist_extensions, extension))
					return IMGTOOLERR_OUTOFMEMORY;
				if (pile_putc(&info->iconlist_extensions, '\0'))
					return IMGTOOLERR_OUTOFMEMORY;
				if (pile_write(&info->iconlist_extensions, &icon_index, sizeof(icon_index)))
					return IMGTOOLERR_OUTOFMEMORY;
			}
		}
	}

	t_entry_filename = tstring_from_utf8(entry->filename);

	memset(&lvi, 0, sizeof(lvi));
	lvi.iItem = ListView_GetItemCount(info->listview);
	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.pszText = t_entry_filename;
	lvi.lParam = index;

	// if we have an icon, use it
	if (icon_index >= 0)
	{
		lvi.mask |= LVIF_IMAGE;
		lvi.iImage = icon_index;
	}

	new_index = ListView_InsertItem(info->listview, &lvi);

	free(t_entry_filename);

	if (entry->directory)
	{
		_sntprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), TEXT("<DIR>"));
	}
	else
	{
		// set the file size
		_sntprintf(buffer, sizeof(buffer) / sizeof(buffer[0]),
			TEXT("%d"), entry->filesize);
	}
	column_index = 1;
	ListView_SetItemText(info->listview, new_index, column_index++, buffer);

	// set creation time, if supported
	if (features.supports_creation_time)
	{
		if (entry->creation_time != 0)
		{
			local_time = localtime(&entry->creation_time);
			_sntprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), _tasctime(local_time));
			rtrim(buffer);
			ListView_SetItemText(info->listview, new_index, column_index, buffer);
		}
		column_index++;
	}

	// set last modified time, if supported
	if (features.supports_lastmodified_time)
	{
		if (entry->lastmodified_time != 0)
		{
			local_time = localtime(&entry->lastmodified_time);
			_sntprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), _tasctime(local_time));
			rtrim(buffer);
			ListView_SetItemText(info->listview, new_index, column_index, buffer);
		}
		column_index++;
	}

	// set attributes and corruption notice
	if (entry->attr)
	{
		TCHAR *tempstr = tstring_from_utf8(entry->attr);
		ListView_SetItemText(info->listview, new_index, column_index++, tempstr);
		free(tempstr);
	}
	if (entry->corrupt)
	{
		ListView_SetItemText(info->listview, new_index, column_index++, (LPTSTR) TEXT("Corrupt"));
	}
	return 0;
}



static imgtoolerr_t refresh_image(HWND window)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	wimgtool_info *info;
	imgtool_directory *imageenum = NULL;
	char size_buf[32];
	imgtool_dirent entry;
	UINT64 filesize;
	int i;
	BOOL is_root_directory;
	imgtool_partition_features features;
	char path_separator;
	TCHAR *tempstr;

	info = get_wimgtool_info(window);
	size_buf[0] = '\0';

	ListView_DeleteAllItems(info->listview);

	if (info->image)
	{
		features = imgtool_partition_get_features(info->partition);

		is_root_directory = TRUE;
		if (info->current_directory)
		{
			for (i = 0; info->current_directory[i]; i++)
			{
				path_separator = (char) imgtool_partition_get_info_int(info->partition, IMGTOOLINFO_INT_PATH_SEPARATOR);
				if (info->current_directory[i] != path_separator)
				{
					is_root_directory = FALSE;
					break;
				}
			}
		}

		memset(&entry, 0, sizeof(entry));

		// add ".." to non-root directory entries
		if (!is_root_directory)
		{
			strcpy(entry.filename, "..");
			entry.directory = 1;
			entry.attr[0] = '\0';
			err = append_dirent(window, -1, &entry);
			if (err)
				goto done;
		}

		err = imgtool_directory_open(info->partition, info->current_directory, &imageenum);
		if (err)
			goto done;

		i = 0;
		do
		{
			err = imgtool_directory_get_next(imageenum, &entry);
			if (err)
				goto done;

			if (entry.filename[0])
			{
				err = append_dirent(window, i++, &entry);
				if (err)
					goto done;
			}
		}
		while(!entry.eof);

		if (features.supports_freespace)
		{
			err = imgtool_partition_get_free_space(info->partition, &filesize);
			if (err)
				goto done;
			snprintf(size_buf, sizeof(size_buf) / sizeof(size_buf[0]), "%u bytes free", (unsigned) filesize);
		}

	}

	tempstr = tstring_from_utf8(size_buf);
	SendMessage(info->statusbar, SB_SETTEXT, 2, (LPARAM) tempstr);
	free(tempstr);

done:
	if (imageenum)
		imgtool_directory_close(imageenum);
	return err;
}



static imgtoolerr_t full_refresh_image(HWND window)
{
	wimgtool_info *info;
	LVCOLUMN col;
	int column_index = 0;
	int i;
	char buf[256];
	char imageinfo_buf[256];
	const char *imageinfo = NULL;
	TCHAR file_title_buf[MAX_PATH];
	char *file_title;
	const char *statusbar_text[2];
	TCHAR *t_filename;
	imgtool_partition_features features;

	info = get_wimgtool_info(window);

	// get the modules and features
	if (info->partition)
		features = imgtool_partition_get_features(info->partition);
	else
		memset(&features, 0, sizeof(features));

	if (info->filename)
	{
		// get file title from Windows
		t_filename = tstring_from_utf8(info->filename);
		GetFileTitle(t_filename, file_title_buf, sizeof(file_title_buf) / sizeof(file_title_buf[0]));
		free(t_filename);
		file_title = utf8_from_tstring(file_title_buf);

		// get info from image
		if (info->image && (imgtool_image_info(info->image, imageinfo_buf, sizeof(imageinfo_buf)
			/ sizeof(imageinfo_buf[0])) == IMGTOOLERR_SUCCESS))
		{
			if (imageinfo_buf[0])
				imageinfo = imageinfo_buf;
		}

		// combine all of this into a title bar
		if (info->current_directory && info->current_directory[0])
		{
			// has a current directory
			if (imageinfo)
			{
				snprintf(buf, sizeof(buf) / sizeof(buf[0]),
					"%s (\"%s\") - %s", file_title, imageinfo, info->current_directory);
			}
			else
			{
				snprintf(buf, sizeof(buf) / sizeof(buf[0]),
					"%s - %s", file_title, info->current_directory);
			}
		}
		else
		{
			// no current directory
			snprintf(buf, sizeof(buf) / sizeof(buf[0]),
				imageinfo ? "%s (\"%s\")" : "%s",
				file_title, imageinfo);
		}

		statusbar_text[0] = osd_basename((char *) info->filename);
		statusbar_text[1] = imgtool_image_module(info->image)->description;

		free(file_title);
	}
	else
	{
		snprintf(buf, sizeof(buf) / sizeof(buf[0]),
			"%s %s", wimgtool_producttext, build_version);
		statusbar_text[0] = NULL;
		statusbar_text[1] = NULL;
	}

	win_set_window_text_utf8(window, buf);
	
	for (i = 0; i < sizeof(statusbar_text) / sizeof(statusbar_text[0]); i++)
	{
		TCHAR *tempstr = statusbar_text[i] ? tstring_from_utf8(statusbar_text[i]) : NULL;
		SendMessage(info->statusbar, SB_SETTEXT, i, (LPARAM) tempstr);
		if (tempstr)
			free(tempstr);
	}

	// set the icon
	if (info->image && (info->open_mode == OSD_FOPEN_READ))
		SendMessage(info->statusbar, SB_SETICON, 0, (LPARAM) info->readonly_icon);
	else
		SendMessage(info->statusbar, SB_SETICON, 0, (LPARAM) 0);

	DragAcceptFiles(window, info->filename != NULL);

	// create the listview columns
	col.mask = LVCF_TEXT | LVCF_WIDTH;
	col.cx = 200;
	col.pszText = (LPTSTR) TEXT("Filename");
	if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
		return IMGTOOLERR_OUTOFMEMORY;

	col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
	col.cx = 60;
	col.pszText = (LPTSTR) TEXT("Size");
	col.fmt = LVCFMT_RIGHT;
	if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
		return IMGTOOLERR_OUTOFMEMORY;

	if (features.supports_creation_time)
	{
		col.mask = LVCF_TEXT | LVCF_WIDTH;
		col.cx = 160;
		col.pszText = (LPTSTR) TEXT("Creation time");
		if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
			return IMGTOOLERR_OUTOFMEMORY;
	}

	if (features.supports_lastmodified_time)
	{
		col.mask = LVCF_TEXT | LVCF_WIDTH;
		col.cx = 160;
		col.pszText = (LPTSTR) TEXT("Last modified time");
		if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
			return IMGTOOLERR_OUTOFMEMORY;
	}

	col.mask = LVCF_TEXT | LVCF_WIDTH;
	col.cx = 60;
	col.pszText = (LPTSTR) TEXT("Attributes");
	if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
		return IMGTOOLERR_OUTOFMEMORY;

	col.mask = LVCF_TEXT | LVCF_WIDTH;
	col.cx = 60;
	col.pszText = (LPTSTR) TEXT("Notes");
	if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
		return IMGTOOLERR_OUTOFMEMORY;

	// delete extraneous columns
	while(ListView_DeleteColumn(info->listview, column_index))
		;
	return refresh_image(window);
}



static imgtoolerr_t setup_openfilename_struct(OPENFILENAME *ofn, memory_pool *pool,
	HWND window, BOOL creating_file)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	mess_pile pile;
	const imgtool_module *default_module = NULL;
	const imgtool_module *module = NULL;
	const char *s;
	TCHAR *filename;
	TCHAR *filter;
	TCHAR *initial_dir = NULL;
	TCHAR *dir_char;
	imgtool_module_features features;
	DWORD filter_index = 0, current_index = 0;
	const wimgtool_info *info;

	info = get_wimgtool_info(window);
	if (info->image)
		default_module = imgtool_image_module(info->image);

	pile_init(&pile);

	if (!creating_file)
	{
		current_index++;
		pile_puts(&pile, "Autodetect (*.*)");
		pile_putc(&pile, '\0');
		pile_puts(&pile, "*.*");
		pile_putc(&pile, '\0');
	}

	// write out library modules
	for (module = imgtool_find_module(NULL); module; module = module->next)
	{
		// check to see if we have the right features
		features = imgtool_get_module_features(module);
		if (creating_file ? features.supports_create : features.supports_open)
		{
			// is this the filter we are asking for?
			current_index++;
			if (module == default_module)
				filter_index = current_index;

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

	// can we specify an initial directory?
	if (info->filename)
	{
		initial_dir = alloca((_tcslen(info->filename) + 1) * sizeof(*info->filename));
		_tcscpy(initial_dir, info->filename);
		dir_char = _tcsrchr(initial_dir, '\\');
		if (dir_char)
			dir_char[1] = '\0';
		else
			initial_dir = NULL;
	}

	// populate the actual OPENFILENAME structure
	memset(ofn, 0, sizeof(*ofn));
	ofn->lStructSize = sizeof(*ofn);
	ofn->Flags = OFN_EXPLORER;
	ofn->hwndOwner = window;
	ofn->lpstrFile = filename;
	ofn->nMaxFile = MAX_PATH;
	ofn->lpstrFilter = filter;
	ofn->nFilterIndex = filter_index;
	ofn->lpstrInitialDir = initial_dir;

done:
	pile_delete(&pile);
	return err;
}



const imgtool_module *find_filter_module(int filter_index,
	BOOL creating_file)
{
	const imgtool_module *module = NULL;
	imgtool_module_features features;

	if (filter_index-- == 0)
		return NULL;
	if (!creating_file && (filter_index-- == 0))
		return NULL;

	for (module = imgtool_find_module(NULL); module; module = module->next)
	{
		features = imgtool_get_module_features(module);
		if (creating_file ? features.supports_create : features.supports_open)
		{
			if (filter_index-- == 0)
				return module;
		}
	}
	return NULL;
}



static imgtoolerr_t get_recursive_directory(imgtool_partition *partition, const char *path, LPCTSTR local_path)
{
	imgtoolerr_t err;
	imgtool_directory *imageenum = NULL;
	imgtool_dirent entry;
	const char *subpath;
	char local_subpath[MAX_PATH];

	if (!CreateDirectory(local_path, NULL))
	{
		err = IMGTOOLERR_UNEXPECTED;
		goto done;
	}

	err = imgtool_directory_open(partition, path, &imageenum);
	if (err)
		goto done;

	do
	{
		err = imgtool_directory_get_next(imageenum, &entry);
		if (err)
			goto done;

		if (!entry.eof)
		{
			snprintf(local_subpath, sizeof(local_subpath) / sizeof(local_subpath[0]),
				TEXT("%s\\%s"), local_path, entry.filename);
			subpath = imgtool_partition_path_concatenate(partition, path, entry.filename);
			
			if (entry.directory)
				err = get_recursive_directory(partition, subpath, local_subpath);
			else
				err = imgtool_partition_get_file(partition, subpath, NULL, local_subpath, NULL);
			if (err)
				goto done;
		}
	}
	while(!entry.eof);

done:
	if (imageenum)
		imgtool_directory_close(imageenum);
	return err;
}




static imgtoolerr_t put_recursive_directory(imgtool_partition *partition, LPCTSTR local_path, const char *path)
{
	imgtoolerr_t err;
	HANDLE h = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA wfd;
	const char *subpath;
	char *filename;
	TCHAR local_subpath[MAX_PATH];

	err = imgtool_partition_create_directory(partition, path);
	if (err)
		goto done;

	_sntprintf(local_subpath, sizeof(local_subpath) / sizeof(local_subpath[0]), TEXT("%s\\*.*"), local_path, wfd.cFileName);

	h = FindFirstFile(local_subpath, &wfd);
	if (h && (h != INVALID_HANDLE_VALUE))
	{
		do
		{
			if (_tcscmp(wfd.cFileName, TEXT(".")) && _tcscmp(wfd.cFileName, TEXT("..")))
			{
				_sntprintf(local_subpath, sizeof(local_subpath) / sizeof(local_subpath[0]), TEXT("%s\\%s"), local_path, wfd.cFileName);
				filename = utf8_from_tstring(wfd.cFileName);
				subpath = imgtool_partition_path_concatenate(partition, path, filename);
				free(filename);

				if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					err = put_recursive_directory(partition, local_subpath, subpath);
				}
				else
				{
					char *tempstr = utf8_from_tstring(local_subpath);
					err = imgtool_partition_put_file(partition, subpath, NULL, tempstr, NULL, NULL);
					free(tempstr);
				}
				if (err)
					goto done;
			}
		}
		while(FindNextFile(h, &wfd));
	}

done:
	if (h && (h != INVALID_HANDLE_VALUE))
		FindClose(h);
	return err;
}



imgtoolerr_t wimgtool_open_image(HWND window, const imgtool_module *module,
	const char *filename, int read_or_write)
{
	imgtoolerr_t err;
	imgtool_image *image;
	imgtool_partition *partition;
	imgtool_module *identified_module;
	wimgtool_info *info;
	int partition_index = 0;
	const char *root_path;
	imgtool_module_features features = { 0, };

	info = get_wimgtool_info(window);

	// if the module is not specified, auto detect the format
	if (!module)
	{
		err = imgtool_identify_file(filename, &identified_module, 1);
		if (err)
			goto done;
		module = identified_module;
	}

	// check to see if this module actually supports writing
	if (read_or_write != OSD_FOPEN_READ)
	{
		features = imgtool_get_module_features(module);
		if (features.is_read_only)
			read_or_write = OSD_FOPEN_READ;
	}

	info->filename = mame_strdup(filename);
	if (!info->filename)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	
	// try to open the image
	err = imgtool_image_open(module, filename, read_or_write, &image);
	if ((ERRORCODE(err) == IMGTOOLERR_READONLY) && read_or_write)
	{
		// if we failed when open a read/write image, try again
		read_or_write = OSD_FOPEN_READ;
		err = imgtool_image_open(module, filename, read_or_write, &image);
	}
	if (err)
		goto done;

	// try to open the partition
	err = imgtool_partition_open(image, partition_index, &partition);
	if (err)
		goto done;

	// unload current image and partition
	if (info->partition)
		imgtool_partition_close(info->partition);
	if (info->image)
		imgtool_image_close(info->image);
	info->image = image;
	info->partition = partition;
	info->open_mode = read_or_write;
	if (info->current_directory)
	{
		free(info->current_directory);
		info->current_directory = NULL;
	}

	// do we support directories?
	if (imgtool_partition_get_features(info->partition).supports_directories)
	{
		root_path = imgtool_partition_get_root_path(partition);
		info->current_directory = mame_strdup(root_path);
		if (!info->current_directory)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}
	}

	// refresh the window
	full_refresh_image(window);
	
done:
	return err;
}



static void menu_new(HWND window)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	memory_pool pool;
	OPENFILENAME ofn;
	const imgtool_module *module;
	char *filename = NULL;
	option_resolution *resolution = NULL;

	pool_init(&pool);

	err = setup_openfilename_struct(&ofn, &pool, window, TRUE);
	if (err)
		goto done;
	ofn.Flags |= OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
	ofn.hInstance = GetModuleHandle(NULL);
	ofn.lpTemplateName = MAKEINTRESOURCE(IDD_FILETEMPLATE);
	ofn.lpfnHook = win_new_dialog_hook;
	ofn.lCustData = (LPARAM) &resolution;
	if (!GetSaveFileName(&ofn))
		goto done;

	filename = utf8_from_tstring(ofn.lpstrFile);

	module = find_filter_module(ofn.nFilterIndex, TRUE);
	
	err = imgtool_image_create(module, filename, resolution, NULL);
	if (err)
		goto done;

	err = wimgtool_open_image(window, module, filename, OSD_FOPEN_RW);
	if (err)
		goto done;

done:
	if (err)
		wimgtool_report_error(window, err, filename, NULL);
	if (resolution)
		option_resolution_close(resolution);
	if (filename)
		free(filename);
	pool_exit(&pool);
}



static void menu_open(HWND window)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	memory_pool pool;
	OPENFILENAME ofn;
	const imgtool_module *module;
	char *filename = NULL;
	wimgtool_info *info;
	int read_or_write;

	pool_init(&pool);

	info = get_wimgtool_info(window);

	err = setup_openfilename_struct(&ofn, &pool, window, FALSE);
	if (err)
		goto done;
	ofn.Flags |= OFN_FILEMUSTEXIST;
	if (!GetOpenFileName(&ofn))
		goto done;

	filename = utf8_from_tstring(ofn.lpstrFile);
	module = find_filter_module(ofn.nFilterIndex, FALSE);

	// is this file read only?
	if ((ofn.Flags & OFN_READONLY) || (GetFileAttributes(ofn.lpstrFile) & FILE_ATTRIBUTE_READONLY))
		read_or_write = OSD_FOPEN_READ;
	else
		read_or_write = OSD_FOPEN_RW;

	err = wimgtool_open_image(window, module, filename, read_or_write);
	if (err)
		goto done;

done:
	if (err)
		wimgtool_report_error(window, err, filename, NULL);
	if (filename)
		free(filename);
	pool_exit(&pool);
}



static void menu_insert(HWND window)
{
	imgtoolerr_t err;
	char *image_filename = NULL;
	TCHAR host_filename[MAX_PATH] = { 0 };
	const TCHAR *s1;
	char *s2;
	OPENFILENAME ofn;
	wimgtool_info *info;
	option_resolution *opts = NULL;
	BOOL cancel;
	const imgtool_module *module;
	const char *fork = NULL;
	struct transfer_suggestion_info suggestion_info;
	int use_suggestion_info;
	imgtool_stream *stream = NULL;
	filter_getinfoproc filter = NULL;
	const struct OptionGuide *writefile_optguide;
	const char *writefile_optspec;
	TCHAR *filename;

	info = get_wimgtool_info(window);

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = window;
	ofn.lpstrFile = host_filename;
	ofn.nMaxFile = sizeof(host_filename) / sizeof(host_filename[0]);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER;
	if (!GetOpenFileName(&ofn))
	{
		err = 0;
		goto done;
	}

	/* we need to open the stream at this point, so that we can suggest the transfer */
	filename = utf8_from_tstring(ofn.lpstrFile);
	stream = stream_open(filename, OSD_FOPEN_READ);
	free(filename);
	if (!stream)
	{
		err = IMGTOOLERR_FILENOTFOUND;
		goto done;
	}

	module = imgtool_image_module(info->image);

	/* figure out which filters are appropriate for this file */
	imgtool_partition_suggest_file_filters(info->partition, NULL, stream, suggestion_info.suggestions,
		sizeof(suggestion_info.suggestions) / sizeof(suggestion_info.suggestions[0]));

	/* do we need to show an option dialog? */
	writefile_optguide = (const struct OptionGuide *) imgtool_partition_get_info_ptr(info->partition, IMGTOOLINFO_PTR_WRITEFILE_OPTGUIDE);
	writefile_optspec = imgtool_partition_get_info_string(info->partition, IMGTOOLINFO_STR_WRITEFILE_OPTSPEC);
	if (suggestion_info.suggestions[0].viability || (writefile_optguide && writefile_optspec))
	{
		use_suggestion_info = (suggestion_info.suggestions[0].viability != SUGGESTION_END);
		err = win_show_option_dialog(window, use_suggestion_info ? &suggestion_info : NULL,
			writefile_optguide, writefile_optspec, &opts, &cancel);
		if (err || cancel)
			goto done;

		if (use_suggestion_info)
		{
			fork = suggestion_info.suggestions[suggestion_info.selected].fork;
			filter = suggestion_info.suggestions[suggestion_info.selected].filter;
		}
	}

	/* figure out the image filename */
	s1 = _tcsrchr(ofn.lpstrFile, '\\');
	s1 = s1 ? s1 + 1 : ofn.lpstrFile;
	image_filename = utf8_from_tstring(s1);

	if (info->current_directory)
	{
		s2 = (char *) alloca(strlen(info->current_directory) + strlen(image_filename) + 1);
		strcpy(s2, info->current_directory);
		strcat(s2, image_filename);
		free(image_filename);
		image_filename = mame_strdup(image_filename);
	}

	err = imgtool_partition_write_file(info->partition, image_filename, fork, stream, opts, filter);
	if (err)
		goto done;

	err = refresh_image(window);
	if (err)
		goto done;

done:
	if (image_filename)
		free(image_filename);
	if (opts)
		option_resolution_close(opts);
	if (err)
		wimgtool_report_error(window, err, image_filename, ofn.lpstrFile);
}



static UINT_PTR CALLBACK extract_dialog_hook(HWND dlgwnd, UINT message,
	WPARAM wparam, LPARAM lparam)
{
	UINT_PTR rc = 0;
	int i;
	HWND filter_combo;
	struct transfer_suggestion_info *info;
	OPENFILENAME *ofi;
	LONG_PTR l;

	filter_combo = GetDlgItem(dlgwnd, IDC_FILTERCOMBO);

	switch(message)
	{
		case WM_INITDIALOG:
			ofi = (OPENFILENAME *) lparam;
			info = (struct transfer_suggestion_info *) ofi->lCustData;
			SetWindowLongPtr(dlgwnd, GWLP_USERDATA, (LONG_PTR) info);
			
			for (i = 0; info->suggestions[i].viability; i++)
			{
				SendMessage(filter_combo, CB_ADDSTRING, 0, (LPARAM) info->suggestions[i].description);
			}
			SendMessage(filter_combo, CB_SETCURSEL, info->selected, 0);

			rc = TRUE;
			break;

		case WM_COMMAND:
			switch(HIWORD(wparam))
			{
				case CBN_SELCHANGE:
					if (LOWORD(wparam) == IDC_FILTERCOMBO)
					{
						l = GetWindowLongPtr(dlgwnd, GWLP_USERDATA);
						info = (struct transfer_suggestion_info *) l;
						info->selected = SendMessage(filter_combo, CB_GETCURSEL, 0, 0);
					}
					break;
			}
			break;
	}
	return rc;
}



static imgtoolerr_t menu_extract_proc(HWND window, const imgtool_dirent *entry, void *param)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	TCHAR host_filename[MAX_PATH];
	OPENFILENAME ofn;
	wimgtool_info *info;
	const char *filename;
	const char *image_basename;
	const char *fork;
	TCHAR *tempstr;
	struct transfer_suggestion_info suggestion_info;
	int i;
	filter_getinfoproc filter;

	info = get_wimgtool_info(window);

	filename = entry->filename;

	// figure out a suggested host filename
	image_basename = imgtool_partition_get_base_name(info->partition, entry->filename);
	tempstr = tstring_from_utf8(image_basename);
	_tcscpy(host_filename, tempstr);
	free(tempstr);

	// try suggesting some filters (only if doing a single file)
	if (!entry->directory)
	{
		imgtool_partition_suggest_file_filters(info->partition, filename, NULL, suggestion_info.suggestions,
			sizeof(suggestion_info.suggestions) / sizeof(suggestion_info.suggestions[0]));

		suggestion_info.selected = 0;
		for (i = 0; i < sizeof(suggestion_info.suggestions) / sizeof(suggestion_info.suggestions[0]); i++)
		{
			if (suggestion_info.suggestions[i].viability == SUGGESTION_RECOMMENDED)
			{
				suggestion_info.selected = i;
				break;
			}
		}
	}
	else
	{
		memset(&suggestion_info, 0, sizeof(suggestion_info));
		suggestion_info.selected = -1;
	}

	// set up the OPENFILENAME struct
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = window;
	ofn.Flags = OFN_EXPLORER;
	ofn.lpstrFile = host_filename;
	ofn.lpstrFilter = TEXT("All files (*.*)\0*.*\0");
	ofn.nMaxFile = sizeof(host_filename) / sizeof(host_filename[0]);

	if (suggestion_info.suggestions[0].viability)
	{
		ofn.Flags |= OFN_ENABLEHOOK | OFN_ENABLESIZING | OFN_ENABLETEMPLATE;
		ofn.lpfnHook = extract_dialog_hook;
		ofn.hInstance = GetModuleHandle(NULL);
		ofn.lpTemplateName = MAKEINTRESOURCE(IDD_EXTRACTOPTIONS);
		ofn.lCustData = (LPARAM) &suggestion_info;
	}

	if (!GetSaveFileName(&ofn))
		goto done;

	if (entry->directory)
	{
		err = get_recursive_directory(info->partition, filename, ofn.lpstrFile);
		if (err)
			goto done;
	}
	else
	{
		if (suggestion_info.selected >= 0)
		{
			fork = suggestion_info.suggestions[suggestion_info.selected].fork;
			filter = suggestion_info.suggestions[suggestion_info.selected].filter;
		}
		else
		{
			fork = NULL;
			filter = NULL;
		}

		err = imgtool_partition_get_file(info->partition, filename, fork, ofn.lpstrFile, filter);
		if (err)
			goto done;
	}

done:
	if (err)
		wimgtool_report_error(window, err, filename, ofn.lpstrFile);
	return err;
}



static void menu_extract(HWND window)
{
	foreach_selected_item(window, menu_extract_proc, NULL);
}



struct createdir_dialog_info
{
	HWND ok_button;
	HWND edit_box;
	TCHAR buf[256];
};



static INT_PTR CALLBACK createdir_dialog_proc(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam)
{
	struct createdir_dialog_info *info;
	LONG_PTR l;
	INT_PTR rc = 0;
	int id;

	switch(message)
	{
		case WM_INITDIALOG:
			EnableWindow(GetDlgItem(dialog, IDOK), FALSE);
			SetWindowLongPtr(dialog, GWLP_USERDATA, lparam);
			info = (struct createdir_dialog_info *) lparam;

			info->ok_button = GetDlgItem(dialog, IDOK);
			info->edit_box = GetDlgItem(dialog, IDC_EDIT);
			SetFocus(info->edit_box);
			break;

		case WM_COMMAND:
			l = GetWindowLongPtr(dialog, GWLP_USERDATA);
			info = (struct createdir_dialog_info *) l;

			switch(HIWORD(wparam))
			{
				case BN_CLICKED:
					id = LOWORD(wparam);
					if (id == IDCANCEL)
						info->buf[0] = '\0';
					EndDialog(dialog, id);
					break;

				case EN_CHANGE:
					GetWindowText(info->edit_box,
						info->buf, sizeof(info->buf) / sizeof(info->buf[0]));
					EnableWindow(info->ok_button, info->buf[0] != '\0');
					break;
			}
			break;
	}
	return rc;
}



static void menu_createdir(HWND window)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	struct createdir_dialog_info cdi;
	wimgtool_info *info;
	char *dirname = NULL;
	char *s;

	info = get_wimgtool_info(window);

	memset(&cdi, 0, sizeof(cdi));
	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CREATEDIR),
		window, createdir_dialog_proc, (LPARAM) &cdi);

	if (cdi.buf[0] == '\0')
		goto done;
	dirname = utf8_from_tstring(cdi.buf);

	if (info->current_directory)
	{
		s = (char *) alloca(strlen(info->current_directory) + strlen(dirname) + 1);
		strcpy(s, info->current_directory);
		strcat(s, dirname);
		free(dirname);
		dirname = mame_strdup(s);
	}

	err = imgtool_partition_create_directory(info->partition, dirname);
	if (err)
		goto done;

	err = refresh_image(window);
	if (err)
		goto done;

done:
	if (err)
		wimgtool_report_error(window, err, NULL, dirname);
	if (dirname)
		free(dirname);
}



static imgtoolerr_t menu_delete_proc(HWND window, const imgtool_dirent *entry, void *param)
{
	imgtoolerr_t err;
	wimgtool_info *info;

	info = get_wimgtool_info(window);

	if (entry->directory)
		err = imgtool_partition_delete_directory(info->partition, entry->filename);
	else
		err = imgtool_partition_delete_file(info->partition, entry->filename);
	if (err)
		goto done;

done:
	if (err)
		wimgtool_report_error(window, err, NULL, entry->filename);
	return err;
}



static void menu_delete(HWND window)
{
	imgtoolerr_t err;

	foreach_selected_item(window, menu_delete_proc, NULL);

	err = refresh_image(window);
	if (err)
		wimgtool_report_error(window, err, NULL, NULL);
}



static void menu_sectorview(HWND window)
{
	wimgtool_info *info;
	info = get_wimgtool_info(window);
	win_sectorview_dialog(window, info->image);
}



static void set_listview_style(HWND window, DWORD style)
{
	wimgtool_info *info;

	info = get_wimgtool_info(window);
	style &= LVS_TYPEMASK;
	style |= (GetWindowLong(info->listview, GWL_STYLE) & ~LVS_TYPEMASK);
	SetWindowLong(info->listview, GWL_STYLE, style);
}



static LRESULT wimgtool_create(HWND window, CREATESTRUCT *pcs)
{
	wimgtool_info *info;
	static int status_widths[3] = { 200, 400, -1 };

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
	SendMessage(info->statusbar, SB_SETPARTS, sizeof(status_widths) / sizeof(status_widths[0]),
		(LPARAM) status_widths);

	// create imagelists
	info->iconlist_normal = ImageList_Create(32, 32, ILC_COLORDDB | ILC_MASK , 0, 0);
	info->iconlist_small = ImageList_Create(16, 16, ILC_COLORDDB | ILC_MASK , 0, 0);
	if (!info->iconlist_normal || !info->iconlist_small)
		return -1;
	ListView_SetImageList(info->listview, info->iconlist_normal, LVSIL_NORMAL);
	ListView_SetImageList(info->listview, info->iconlist_small, LVSIL_SMALL);

	// get icons
	info->readonly_icon = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_READONLY), IMAGE_ICON, 16, 16, 0);
	info->readonly_icon_index = ImageList_AddIcon(info->iconlist_normal, info->readonly_icon);
	ImageList_AddIcon(info->iconlist_small, info->readonly_icon);
	info->directory_icon_index = append_associated_icon(window, FOLDER_ICON);

	full_refresh_image(window);
	return 0;
}



static void wimgtool_destroy(HWND window)
{
	wimgtool_info *info;

	info = get_wimgtool_info(window);

	if (info)
	{
		if (info->partition)
			imgtool_partition_close(info->partition);
		if (info->image)
			imgtool_image_close(info->image);
		pile_delete(&info->iconlist_extensions);
		DestroyIcon(info->readonly_icon);
		if (info->current_directory)
			free(info->current_directory);
		free(info);
	}
}



static void drop_files(HWND window, HDROP drop)
{
	wimgtool_info *info;
	UINT count, i;
	TCHAR buffer[MAX_PATH];
	char subpath[1024];
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	char *filename = NULL;

	info = get_wimgtool_info(window);

	count = DragQueryFile(drop, 0xFFFFFFFF, NULL, 0);
	for (i = 0; i < count; i++)
	{
		DragQueryFile(drop, i, buffer, sizeof(buffer) / sizeof(buffer[0]));
		filename = utf8_from_tstring(buffer);

		// figure out the file/dir name on the image
		snprintf(subpath, sizeof(subpath) / sizeof(subpath[0]), "%s%s",
			info->current_directory ? info->current_directory : "", osd_basename(filename));

		if (GetFileAttributes(buffer) & FILE_ATTRIBUTE_DIRECTORY)
			err = put_recursive_directory(info->partition, buffer, subpath);
		else
			err = imgtool_partition_put_file(info->partition, subpath, NULL, filename, NULL, NULL);
		if (err)
			goto done;

		free(filename);
		filename = NULL;
	}

done:
	refresh_image(window);
	if (err)
		wimgtool_report_error(window, err, NULL, NULL);
	if (filename)
		free(filename);
}



static imgtoolerr_t change_directory(HWND window, const char *dir)
{
	wimgtool_info *info;
	char *new_current_dir;

	info = get_wimgtool_info(window);

	new_current_dir = mame_strdup(imgtool_partition_path_concatenate(info->partition, info->current_directory, dir));
	if (!new_current_dir)
		return IMGTOOLERR_OUTOFMEMORY;
	info->current_directory = new_current_dir;
	return full_refresh_image(window);
}



static imgtoolerr_t double_click(HWND window)
{
	imgtoolerr_t err;
	wimgtool_info *info;
	LVHITTESTINFO htinfo;
	LVITEM item;
	POINTS pt;
	RECT r;
	DWORD pos;
	imgtool_dirent entry;
	int selected_item;

	info = get_wimgtool_info(window);

	memset(&htinfo, 0, sizeof(htinfo));
	pos = GetMessagePos();
	pt = MAKEPOINTS(pos);
	GetWindowRect(info->listview, &r);
	htinfo.pt.x = pt.x - r.left;
	htinfo.pt.y = pt.y - r.top;
	ListView_HitTest(info->listview, &htinfo);
	
	if (htinfo.flags & LVHT_ONITEM)
	{
		memset(&entry, 0, sizeof(entry));

		item.mask = LVIF_PARAM;
		item.iItem = htinfo.iItem;
		ListView_GetItem(info->listview, &item);

		selected_item = item.lParam;
	
		if (selected_item < 0)
		{
			strcpy(entry.filename, "..");
			entry.directory = 1;
		}
		else
		{
			err = imgtool_partition_get_directory_entry(info->partition, info->current_directory, selected_item, &entry);
			if (err)
				return err;
		}

		if (entry.directory)
		{
			err = change_directory(window, entry.filename);
			if (err)
				return err;
		}
	}
	return IMGTOOLERR_SUCCESS;
}



static BOOL context_menu(HWND window, LONG x, LONG y)
{
	wimgtool_info *info;
	LVHITTESTINFO hittest;
	BOOL rc = FALSE;
	HMENU menu;

	info = get_wimgtool_info(window);

	memset(&hittest, 0, sizeof(hittest));
	hittest.pt.x = x;
	hittest.pt.y = y;
	ScreenToClient(info->listview, &hittest.pt);
	ListView_HitTest(info->listview, &hittest);

	if (hittest.flags & LVHT_ONITEM)
	{
		menu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_FILECONTEXT_MENU));
		TrackPopupMenu(GetSubMenu(menu, 0), TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, 0, window, NULL);
		DestroyMenu(menu);
		rc = TRUE;
	}
	return rc;
}



struct selection_info
{
	unsigned int has_files : 1;
	unsigned int has_directories : 1;
};



static imgtoolerr_t init_menu_proc(HWND window, const imgtool_dirent *entry, void *param)
{
	struct selection_info *si;
	si = (struct selection_info *) param;
	if (entry->directory)
		si->has_directories = 1;
	else
		si->has_files = 1;
	return IMGTOOLERR_SUCCESS;
}



static void init_menu(HWND window, HMENU menu)
{
	wimgtool_info *info;
	imgtool_module_features module_features;
	imgtool_partition_features partition_features;
	struct selection_info si;
	unsigned int can_read, can_write, can_createdir, can_delete;
	LONG lvstyle;

	memset(&module_features, 0, sizeof(module_features));
	memset(&partition_features, 0, sizeof(partition_features));
	memset(&si, 0, sizeof(si));

	info = get_wimgtool_info(window);

	if (info->image)
	{
		module_features = imgtool_get_module_features(imgtool_image_module(info->image));
		partition_features = imgtool_partition_get_features(info->partition);
		foreach_selected_item(window, init_menu_proc, &si);
	}

	can_read      = partition_features.supports_reading && (si.has_files || si.has_directories);
	can_write     = partition_features.supports_writing && (info->open_mode != OSD_FOPEN_READ);
	can_createdir = partition_features.supports_createdir && (info->open_mode != OSD_FOPEN_READ);
	can_delete    = (si.has_files || si.has_directories)
						&& (!si.has_files || partition_features.supports_deletefile)
						&& (!si.has_directories || partition_features.supports_deletedir)
						&& (info->open_mode != OSD_FOPEN_READ);

	EnableMenuItem(menu, ID_IMAGE_INSERT,
		MF_BYCOMMAND | (can_write ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(menu, ID_IMAGE_EXTRACT,
		MF_BYCOMMAND | (can_read ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(menu, ID_IMAGE_CREATEDIR,
		MF_BYCOMMAND | (can_createdir ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(menu, ID_IMAGE_DELETE,
		MF_BYCOMMAND | (can_delete ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(menu, ID_IMAGE_SECTORVIEW,
		MF_BYCOMMAND | (module_features.supports_readsector ? MF_ENABLED : MF_GRAYED));

	lvstyle = GetWindowLong(info->listview, GWL_STYLE) & LVS_TYPEMASK;
	CheckMenuItem(menu, ID_VIEW_ICONS,
		MF_BYCOMMAND | (lvstyle == LVS_ICON) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(menu, ID_VIEW_LIST,
		MF_BYCOMMAND | (lvstyle == LVS_LIST) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(menu, ID_VIEW_DETAILS,
		MF_BYCOMMAND | (lvstyle == LVS_REPORT) ? MF_CHECKED : MF_UNCHECKED);
}



static LRESULT CALLBACK wimgtool_wndproc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	wimgtool_info *info;
	RECT window_rect;
	RECT status_rect;
	int window_width;
	int window_height;
	int status_height;
	NMHDR *notify;
	POINT pt;
	LRESULT lres;
	HWND target_window;
	DWORD style;

	info = get_wimgtool_info(window);

	switch(message)
	{
		case WM_CREATE:
			if (wimgtool_create(window, (CREATESTRUCT *) lparam))
				return -1;
			break;

		case WM_DESTROY:
			wimgtool_destroy(window);
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
			init_menu(window, (HMENU) wparam);
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

				case ID_IMAGE_CREATEDIR:
					menu_createdir(window);
					break;

				case ID_IMAGE_DELETE:
					menu_delete(window);
					break;

				case ID_IMAGE_SECTORVIEW:
					menu_sectorview(window);
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

				case ID_VIEW_ASSOCIATIONS:
					win_association_dialog(window);
					break;
			}
			break;

		case WM_NOTIFY:
			notify = (NMHDR *) lparam;
			switch(notify->code)
			{
				case NM_DBLCLK:
					double_click(window);
					break;

				case LVN_BEGINDRAG:
					pt.x = 8;
					pt.y = 8;

					lres = SendMessage(info->listview, LVM_CREATEDRAGIMAGE,
						(WPARAM) ((NM_LISTVIEW *) lparam)->iItem, (LPARAM) &pt);
					info->dragimage = (HIMAGELIST) lres;

					pt = ((NM_LISTVIEW *) notify)->ptAction;
					ClientToScreen(info->listview, &pt);

					ImageList_BeginDrag(info->dragimage, 0, 0, 0);
					ImageList_DragEnter(GetDesktopWindow(), pt.x, pt.y);
					SetCapture(window);
					info->dragpt = pt;
					break;
			}
			break;

		case WM_MOUSEMOVE:
			if (info->dragimage)
			{
				pt.x = LOWORD(lparam);
				pt.y = HIWORD(lparam);
				ClientToScreen(window, &pt);
				info->dragpt = pt;

				ImageList_DragMove(pt.x, pt.y);
			}
			break;

		case WM_LBUTTONUP:
			if (info->dragimage)
			{
				target_window = WindowFromPoint(info->dragpt);

				ImageList_DragLeave(info->listview);
				ImageList_EndDrag();
				ImageList_Destroy(info->dragimage);
				ReleaseCapture();
				info->dragimage = NULL;
				info->dragpt.x = 0;
				info->dragpt.y = 0;

				style = GetWindowLong(target_window, GWL_EXSTYLE);
				if (style & WS_EX_ACCEPTFILES)
				{
				}
			}
			break;

		case WM_CONTEXTMENU:
			context_menu(window, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
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
	wimgtool_wndclass.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_IMGTOOL));
	return RegisterClass(&wimgtool_wndclass);
}
