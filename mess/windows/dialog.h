#ifndef DIALOG_H
#define DIALOG_H

#include <windows.h>
#include "mame.h"
#include "png.h"

typedef struct _dialog_box dialog_box;

dialog_box *win_dialog_init(const char *title);
void win_dialog_exit(dialog_box *dialog);
void win_dialog_runmodal(dialog_box *dialog);
int win_dialog_add_combobox(dialog_box *dialog, const char *label, UINT16 *value);
int win_dialog_add_combobox_item(dialog_box *dialog, const char *item_label, int item_data);
int win_dialog_add_portselect(dialog_box *dialog, struct InputPort *port, RECT *r);
int win_dialog_add_standard_buttons(dialog_box *dialog);
int win_dialog_add_image(dialog_box *dialog, const struct png_info *png);
int win_dialog_add_separator(dialog_box *dialog);

#endif /* DIALOG_H */
