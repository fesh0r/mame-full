#include "invokegx.h"
#include "emugx.h"

extern "C" void CLIB_DECL logerror(const char *text,...);

struct gx_call
{
	int required;
	void **cdecl_call;
	void **stdcall_call;
	const char *call_name;
};

static HMODULE gx_library;

static int (__cdecl *cdecl_GXOpenDisplay)(HWND hWnd, DWORD dwFlags);
static int (__cdecl *cdecl_GXCloseDisplay)(void);
static void *(__cdecl *cdecl_GXBeginDraw)(void);
static int (__cdecl *cdecl_GXEndDraw)(void);
static int (__cdecl *cdecl_GXOpenInput)(void);
static int (__cdecl *cdecl_GXCloseInput)(void);
static GXDisplayProperties (__cdecl *cdecl_GXGetDisplayProperties)(void);
static GXKeyList (__cdecl *cdecl_GXGetDefaultKeys)(int iOptions);
static int (__cdecl *cdecl_GXSuspend)(void);
static int (__cdecl *cdecl_GXResume)(void);
static int (__cdecl *cdecl_GXSetViewport)(DWORD dwTop, DWORD dwHeight, DWORD dwReserved1, DWORD dwReserved2);
static BOOL (__cdecl *cdecl_GXIsDisplayDRAMBuffer)(void);

static int (__stdcall *stdcall_GXOpenDisplay)(HWND hWnd, DWORD dwFlags);
static int (__stdcall *stdcall_GXCloseDisplay)(void);
static void *(__stdcall *stdcall_GXBeginDraw)(void);
static int (__stdcall *stdcall_GXEndDraw)(void);
static int (__stdcall *stdcall_GXOpenInput)(void);
static int (__stdcall *stdcall_GXCloseInput)(void);
static GXDisplayProperties (__stdcall *stdcall_GXGetDisplayProperties)(void);
static GXKeyList (__stdcall *stdcall_GXGetDefaultKeys)(int iOptions);
static int (__stdcall *stdcall_GXSuspend)(void);
static int (__stdcall *stdcall_GXResume)(void);
static int (__stdcall *stdcall_GXSetViewport)(DWORD dwTop, DWORD dwHeight, DWORD dwReserved1, DWORD dwReserved2);
static BOOL (__stdcall *stdcall_GXIsDisplayDRAMBuffer)(void);

static struct gx_call gx_calls[] = 
{
	{ 1, (void **) &cdecl_GXBeginDraw,				(void **) &stdcall_GXBeginDraw,				"?GXBeginDraw@@YAPAXXZ" },
	{ 1, (void **) &cdecl_GXCloseDisplay,			(void **) &stdcall_GXCloseDisplay,			"?GXCloseDisplay@@YAHXZ" },
	{ 1, (void **) &cdecl_GXCloseInput,				(void **) &stdcall_GXCloseInput,			"?GXCloseInput@@YAHXZ" },
	{ 1, (void **) &cdecl_GXEndDraw,				(void **) &stdcall_GXEndDraw,				"?GXEndDraw@@YAHXZ" },
	{ 1, (void **) &cdecl_GXGetDefaultKeys,			(void **) &stdcall_GXGetDefaultKeys,		"?GXGetDefaultKeys@@YA?AUGXKeyList@@H@Z" },
	{ 1, (void **) &cdecl_GXGetDisplayProperties,	(void **) &stdcall_GXGetDisplayProperties,	"?GXGetDisplayProperties@@YA?AUGXDisplayProperties@@XZ" },
	{ 0, (void **) &cdecl_GXIsDisplayDRAMBuffer,	(void **) &stdcall_GXIsDisplayDRAMBuffer,	"?GXIsDisplayDRAMBuffer@@YAHXZ" },
	{ 1, (void **) &cdecl_GXOpenDisplay,			(void **) &stdcall_GXOpenDisplay,			"?GXOpenDisplay@@YAHPAUHWND__@@K@Z" },
	{ 1, (void **) &cdecl_GXOpenInput,				(void **) &stdcall_GXOpenInput,				"?GXOpenInput@@YAHXZ" },
	{ 0, (void **) &cdecl_GXResume,					(void **) &stdcall_GXResume,				"?GXResume@@YAHXZ" },
	{ 0, (void **) &cdecl_GXSetViewport,			(void **) &stdcall_GXSetViewport,			"?GXSetViewport@@YAHKKKK@Z" },
	{ 0, (void **) &cdecl_GXSuspend,				(void **) &stdcall_GXSuspend,				"?GXSuspend@@YAHXZ" }
};

static void gx_close(void);

#ifdef MAME_DEBUG
static void reporterror(const char *msg)
{
	DWORD err;
	TCHAR buffer[256];
	TCHAR *s;

	err = GetLastError();
	buffer[0] = ' ';
	buffer[1] = '(';
	if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, buffer + 2, sizeof(buffer) / sizeof(buffer[0]) - 3, NULL))
	{
		s = wcsrchr(buffer, '\r');
		if (s)
			*s = '\0';
		wcscat(buffer, TEXT(")"));
	}
	else
	{
		buffer[0] = '\0';
	}
	logerror("%s; GetLastError() returned 0x%08x%S\n", msg, err, buffer);
}
#else
#define reporterror(msg)
#endif

static int gx_open(void)
{
	int i, j;
	int at_pos;
	LPTSTR s;
	void *call;
	int rc;

	gx_library = LoadLibrary(TEXT("gx.dll"));
	if (!gx_library)
	{
#if HAS_EMUGX
		/* use emulated gapi */
		cdecl_GXOpenDisplay = emu_GXOpenDisplay;
		cdecl_GXCloseDisplay = emu_GXCloseDisplay;
		cdecl_GXBeginDraw = emu_GXBeginDraw;
		cdecl_GXEndDraw = emu_GXEndDraw;
		cdecl_GXOpenInput = emu_GXOpenInput;
		cdecl_GXCloseInput = emu_GXCloseInput;
		cdecl_GXGetDisplayProperties = emu_GXGetDisplayProperties;
		cdecl_GXGetDefaultKeys = emu_GXGetDefaultKeys;
		cdecl_GXSuspend = emu_GXSuspend;
		cdecl_GXResume = emu_GXResume;
		rc = 1;
#else
		reporterror("Could not load gx.dll");
		rc = 0;
#endif
	}
	else
	{
		rc = 1;
		for (i = 0; i < sizeof(gx_calls) / sizeof(gx_calls[0]); i++)
		{
			j = -1;
			s = (LPTSTR) alloca((strlen(gx_calls[i].call_name) + 1) * sizeof(TCHAR));
			do
			{
				j++;
				s[j] = gx_calls[i].call_name[j];
			}
			while(gx_calls[i].call_name[j]);

			*(gx_calls[i].cdecl_call) = call = GetProcAddress(gx_library, s);
			if (!call)
			{
				wcsstr(s, TEXT("@@YA"))[3] = 'G';
				*(gx_calls[i].stdcall_call) = call = GetProcAddress(gx_library, s);
				if (!call && gx_calls[i].required)
				{
					reporterror("Could not find required GAPI function");
					gx_close();
					rc = 0;
					break;
				}
			}
		}
	}
	return rc;
}

static void gx_close(void)
{
	int i;
	if (gx_library)
	{
		FreeLibrary(gx_library);
		gx_library = NULL;
	}

	for (i = 0; i < sizeof(gx_calls) / sizeof(gx_calls[0]); i++)
	{
		*(gx_calls[i].cdecl_call) = NULL;
		*(gx_calls[i].stdcall_call) = NULL;
	}
}

int gx_open_display(HWND hWnd, DWORD dwFlags)
{
	int rc;

	if (gx_open())
	{
		/* use normal GAPI calls */
		if (cdecl_GXOpenDisplay)
			rc = cdecl_GXOpenDisplay(hWnd, dwFlags);
		else
			rc = stdcall_GXOpenDisplay(hWnd, dwFlags);
	}

	if (rc == 0)
	{
		reporterror("Could not find open display");
	}
	return rc;
}

int gx_close_display(void)
{
	int rc = 1;

	if (cdecl_GXCloseDisplay)
		rc = cdecl_GXCloseDisplay();
	else if (stdcall_GXCloseDisplay)
		rc = stdcall_GXCloseDisplay();

	gx_close();
	return rc;
}

void *gx_begin_draw(void)
{
	if (cdecl_GXBeginDraw)
		return cdecl_GXBeginDraw();
	else
		return stdcall_GXBeginDraw();
}

int gx_end_draw(void)
{
	if (cdecl_GXEndDraw)
		return cdecl_GXEndDraw();
	else
		return stdcall_GXEndDraw();
}

int gx_open_input(void)
{
	if (cdecl_GXOpenInput)
		return cdecl_GXOpenInput();
	else if (stdcall_GXOpenInput)
		return stdcall_GXOpenInput();
	else
		return 1;
}

int gx_close_input(void)
{
	if (cdecl_GXCloseInput)
		return cdecl_GXCloseInput();
	else if (stdcall_GXCloseInput)
		return stdcall_GXCloseInput();
	else
		return 1;
}

void gx_get_display_properties(struct GXDisplayProperties *properties)
{
	if (cdecl_GXGetDisplayProperties)
		*properties = cdecl_GXGetDisplayProperties();
	else if (stdcall_GXGetDisplayProperties)
		*properties = stdcall_GXGetDisplayProperties();
}

void gx_get_default_keys(struct GXKeyList *keylist, int iOptions)
{
	if (cdecl_GXGetDefaultKeys)
		*keylist = cdecl_GXGetDefaultKeys(iOptions);
	else
		*keylist = stdcall_GXGetDefaultKeys(iOptions);
}

int gx_suspend(void)
{
	if (cdecl_GXSuspend)
		return cdecl_GXSuspend();
	else if (stdcall_GXSuspend)
		return stdcall_GXSuspend();
	else
		return 1;
}

int gx_resume(void)
{
	if (cdecl_GXResume)
		return cdecl_GXResume();
	else if (stdcall_GXResume)
		return stdcall_GXResume();
	else
		return 1;
}

int gx_set_viewport(DWORD dwTop, DWORD dwHeight)
{
	if (cdecl_GXSetViewport)
		return cdecl_GXSetViewport(dwTop, dwHeight, 0, 0);
	else if (stdcall_GXSetViewport)
		return stdcall_GXSetViewport(dwTop, dwHeight, 0, 0);
	else
		return 1;
}

BOOL gx_is_display_DRAM_buffer(void)
{
	if (cdecl_GXIsDisplayDRAMBuffer)
		return cdecl_GXIsDisplayDRAMBuffer();
	else if (stdcall_GXIsDisplayDRAMBuffer)
		return stdcall_GXIsDisplayDRAMBuffer();
	else
		return FALSE;
}

