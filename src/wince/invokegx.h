#ifndef INVOKEGX_H
#define INVOKEGX_H

#ifndef __cplusplus
typedef void *GXDisplayProperties;
typedef void *GXKeyList;
#endif

#include <windows.h>
#include <gx.h>

#ifdef __cplusplus
extern "C" {
#endif

int gx_open_display(HWND hWnd, DWORD dwFlags);
int gx_close_display(void);
void *gx_begin_draw(void);
int gx_end_draw(void);
int gx_open_input(void);
int gx_close_input(void);
void gx_get_display_properties(struct GXDisplayProperties *properties);
void gx_get_default_keys(struct GXKeyList *keylist, int iOptions);
int gx_suspend(void);
int gx_resume(void);
int gx_set_viewport(DWORD dwTop, DWORD dwHeight);
BOOL gx_is_display_DRAM_buffer(void);

#ifdef __cplusplus
}
#endif

#endif /* INVOKEGX_H */

