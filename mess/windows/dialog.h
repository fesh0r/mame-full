#ifndef DIALOG_H
#define DIALOG_H

#include <windows.h>
#include "mame.h"

void *win_dialog_init(const char *title);
void win_dialog_exit(void *dialog);
void win_dialog_runmodal(void *dialog);
int win_dialog_add_combobox(void *dialog, const char *label, UINT16 *value);
int win_dialog_add_combobox_item(void *dialog, const char *item_label, int item_data);
int win_dialog_add_portselect(void *dialog, struct InputPort *port);
int win_dialog_add_standard_buttons(void *dialog);

#endif /* DIALOG_H */
