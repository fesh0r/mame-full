#include "menuentr.h"

int menu_selection_is_first(MENU_SELECTION *entry)
{
	int j=entry->value-1;
	while ( (j>=0)&&(entry->options[j]==NULL)) j--;

	return j<0;
}

int menu_selection_is_last(MENU_SELECTION *entry)
{
	int j=entry->value+1;
	while ((j<ARRAY_LENGTH(entry->options))&&(entry->options[j]==NULL)) j++;

	return j>=ARRAY_LENGTH(entry->options);
}

void menu_selection_plus(MENU_SELECTION *entry, int id)
{
	int j;

	if (entry->editable) {
		for (j=entry->value+1;
			 (j<ARRAY_LENGTH(entry->options))
				 &&(entry->options[j]==0);j++) ;
		
		if ((j<ARRAY_LENGTH(entry->options))&&(j!=entry->value)) {
			entry->value=j;
			if (entry->changed) entry->changed(id, j);
		}
	}
}
		
void menu_selection_minus(MENU_SELECTION *entry, int id)
{
	int j;

	if (entry->editable) {
		j=entry->value-1;
		while ((j>=0)&&(entry->options[j]==0)) j--;
		if ((j>=0)&&(j!=entry->value)) {
			entry->value=j;
			if (entry->changed) entry->changed(id, j);
		}
	}
}

void menu_selection_set_value(MENU_SELECTION *entry, int id, int value)
{
	if (entry->value!=value) {
		entry->value=value;
		if (entry->changed) entry->changed(id, value);
	}
}

int menu_selection_get_value(MENU_SELECTION *entry)
{
	return entry->value;
}
