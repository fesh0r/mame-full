#include "assoc.h"
#include "../imgtool.h"

static INT_PTR CALLBACK win_association_dialog_proc(HWND dialog, UINT message,
	WPARAM wparam, LPARAM lparam)
{
	INT_PTR rc = 0;
	imgtool_library *library;

	switch(message)
	{
		case WM_INITDIALOG:
			library = (imgtool_library *) lparam;
			break;
	}
	return rc;
}



static int CLIB_DECL extension_compare(const void *p1, const void *p2)
{
	const char *e1 = *((const char **) p1);
	const char *e2 = *((const char **) p2);
	return strcmp(e1, e2);
}



static void foo(imgtool_library *library)
{
	const struct ImageModule *module = NULL;
	const char *extensions[128];
	int extension_count = 0;
	const char *ext;
	int i;

	while((module = imgtool_library_iterate(library, module)) != NULL)
	{
		ext = module->extensions;
		while(ext[0])
		{
			for (i = 0; i < extension_count; i++)
			{
				if (!strcmp(extensions[i], ext))
					break;
			}
			if (i >= extension_count)
			{
				assert(extension_count < sizeof(extensions) / sizeof(extensions[0]));
				if (extension_count < sizeof(extensions) / sizeof(extensions[0]))
				{
					extensions[extension_count++] = ext;
				}
			}
			ext = ext + strlen(ext) + 1;
		}
	}

	qsort((void *) extensions, extension_count,
		sizeof(extensions[0]), extension_compare);
}



void win_association_dialog(HWND parent, imgtool_library *library)
{
	DLGTEMPLATE dlgtemp;

	foo(library);

	memset(&dlgtemp, 0, sizeof(dlgtemp));
	dlgtemp.style = DS_CENTER;
	dlgtemp.cx = 100;
	dlgtemp.cy = 100;
	DialogBoxIndirectParam(GetModuleHandle(NULL), &dlgtemp, parent,
		win_association_dialog_proc, (LPARAM) library);
	GetLastError();
}
