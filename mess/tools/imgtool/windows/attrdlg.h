#include <windows.h>
#include "../imgtool.h"

UINT_PTR CALLBACK win_new_dialog_hook(HWND dlgwnd, UINT message,
	WPARAM wparam, LPARAM lparam);

imgtoolerr_t win_show_option_dialog(HWND parent, const struct OptionGuide *guide, const char *optspec,
	option_resolution **result, BOOL *cancel);

