//============================================================
//
//	opcntrl.c - Code for handling option resolutions in
//	Win32 controls
//
//	This code was factored out of menu.c when Windows Imgtool
//	started to have dynamic controls
//
//============================================================

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <tchar.h>

#include "opcntrl.h"
#include "strconv.h"


static const TCHAR guide_prop[] = TEXT("opcntrl_prop");

static BOOL prepare_combobox(HWND control, const struct OptionGuide *guide,
	const char *optspec)
{
	struct OptionRange ranges[128];
	int default_value, default_index, current_index, option_count;
	int i, j;
	BOOL has_option;
	TCHAR buf1[256];
	TCHAR buf2[256];

	SendMessage(control, CB_GETLBTEXT, SendMessage(control, CB_GETCURSEL, 0, 0), (LPARAM) buf1);
	SendMessage(control, CB_RESETCONTENT, 0, 0);
	has_option = guide && optspec;

	if (has_option)
	{
		option_resolution_listranges(optspec, guide->parameter,
			ranges, sizeof(ranges) / sizeof(ranges[0]));
		option_resolution_getdefault(optspec, guide->parameter, &default_value);

		option_count = 0;
		default_index = -1;
		current_index = -1;

		for (i = 0; ranges[i].min >= 0; i++)
		{
			for (j = ranges[i].min; j <= ranges[i].max; j++)
			{
				_sntprintf(buf2, sizeof(buf2) / sizeof(buf2[0]), TEXT("%d"), j);
				SendMessage(control, CB_ADDSTRING, 0, (LPARAM) buf2);
				SendMessage(control, CB_SETITEMDATA, option_count, j);

				if (j == default_value)
					default_index = option_count;
				if (!strcmp(buf1, buf2))
					current_index = option_count;
				option_count++;
			}
		}
		
		// if there is only one option, it is effectively disabled
		if (option_count <= 1)
			has_option = FALSE;

		if (current_index >= 0)
			SendMessage(control, CB_SETCURSEL, current_index, 0);
		else if (default_index >= 0)
			SendMessage(control, CB_SETCURSEL, default_index, 0);
	}
	else
	{
		// this item is non applicable
		SendMessage(control, CB_ADDSTRING, 0, (LPARAM) TEXT("N/A"));
		SendMessage(control, CB_SETCURSEL, 0, 0);
	}
	EnableWindow(control, has_option);
	return TRUE;
}



BOOL win_prepare_option_control(HWND control, const struct OptionGuide *guide,
	const char *optspec)
{
	BOOL rc = FALSE;
	TCHAR class_name[32];

	GetClassName(control, class_name, sizeof(class_name)
		/ sizeof(class_name[0]));

	if (!_tcsicmp(class_name, TEXT("ComboBox")))
		rc = prepare_combobox(control, guide, optspec);

	if (rc)
		SetProp(control, guide_prop, (HANDLE) guide);
	return rc;
}



optreserr_t win_add_resolution_parameter(HWND control, option_resolution *resolution)
{
	const struct OptionGuide *guide;
	TCHAR buf[256];
	optreserr_t err;

	if (!GetWindowText(control, buf, sizeof(buf) / sizeof(buf[0])))
		return OPTIONRESOLTUION_ERROR_INTERNAL;

	guide = (const struct OptionGuide *) GetProp(control, guide_prop);
	if (!guide)
		return OPTIONRESOLTUION_ERROR_INTERNAL;

	err = option_resolution_add_param(resolution, guide->identifier, U2T(buf));
	if (err)
		return err;

	return OPTIONRESOLUTION_ERROR_SUCCESS;
}
