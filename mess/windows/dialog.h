#ifndef DIALOG_H
#define DIALOG_H

#include <windows.h>

void *win_dialog_init(const WCHAR *title);
void win_dialog_exit(void *dialog);
void win_dialog_runmodal(void *dialog);
int win_dialog_add_label(void *dialog, const WCHAR *label);

#endif /* DIALOG_H */
