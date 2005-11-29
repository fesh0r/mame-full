/****************************************************************************

	macutil.h

	Imgtool Utility code for manipulating certain Apple/Mac data structures
	and conventions

*****************************************************************************/

#ifndef MACUTIL_H
#define MACUTIL_H

#include "imgtool.h"

typedef enum
{
	MAC_FORK_DATA,
	MAC_FORK_RESOURCE
} mac_fork_t;

typedef enum
{
	MAC_FILECATEGORY_DATA,
	MAC_FILECATEGORY_TEXT,
	MAC_FILECATEGORY_FORKED
} mac_filecategory_t;



imgtoolerr_t mac_identify_fork(const char *fork_string, mac_fork_t *fork_num);

void mac_suggest_transfer(mac_filecategory_t file_category, imgtool_transfer_suggestion *suggestions, size_t suggestions_length);

#endif /* MACUTIL_H */
