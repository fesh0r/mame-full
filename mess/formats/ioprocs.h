/*********************************************************************

	ioprocs.h

	File IO abstraction layer

*********************************************************************/

#ifndef IOPROCS_H
#define IOPROCS_H

#include <stdlib.h>
#include "osd_cpu.h"


/***************************************************************************

	Type definitions

***************************************************************************/

struct io_procs
{
	void (*closeproc)(void *file);
	int (*seekproc)(void *file, INT64 offset, int whence);
	size_t (*readproc)(void *file, void *buffer, size_t length);
	size_t (*writeproc)(void *file, const void *buffer, size_t length);
	UINT64 (*filesizeproc)(void *file);
};



/***************************************************************************

	Globals

***************************************************************************/

extern struct io_procs stdio_ioprocs;
extern struct io_procs stdio_ioprocs_noclose;

#endif /* IOPROCS_H */
