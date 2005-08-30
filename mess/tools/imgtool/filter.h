/***************************************************************************

	filter.h

	Imgtool filters

***************************************************************************/

#ifndef FILTER_H
#define FILTER_H

#include <stdlib.h>
#include <stdio.h>

#include "osdepend.h"
#include "mess.h"
#include "library.h"

typedef struct _imgtool_filter imgtool_filter;

union filterinfo
{
	INT64	i;											/* generic integers */
	void *	p;											/* generic pointers */
	genf *  f;											/* generic function pointers */
	const char *s;										/* generic strings */

	imgtoolerr_t (*read_file)(imgtool_image *image, const char *filename, imgtool_stream *destf);
	imgtoolerr_t (*write_file)(imgtool_image *image, const char *filename, imgtool_stream *sourcef);
};

enum
{
	/* --- the following bits of info are returned as 64-bit signed integers --- */
	FILTINFO_INT_FIRST = 0x00000,
	FILTINFO_INT_STATESIZE,

	/* --- the following bits of info are returned as pointers to data or functions --- */
	FILTINFO_PTR_FIRST = 0x10000,
	FILTINFO_PTR_READFILE,
	FILTINFO_PTR_WRITEFILE,

	/* --- the following bits of info are returned as NULL-terminated strings --- */
	FILTINFO_STR_FIRST = 0x20000,
	FILTINFO_STR_NAME,
	FILTINFO_STR_HUMANNAME,
	FILTINFO_STR_EXTENSION
};

typedef void (*filter_getinfoproc)(UINT32 state, union filterinfo *info);

extern const filter_getinfoproc filters[];

filter_getinfoproc filter_lookup(const char *name);

/* ----------------------------------------------------------------------- */

INT64 filter_get_info_int(filter_getinfoproc get_info, UINT32 state);
void *filter_get_info_ptr(filter_getinfoproc get_info, UINT32 state);
genf *filter_get_info_fct(filter_getinfoproc get_info, UINT32 state);
const char *filter_get_info_string(filter_getinfoproc get_info, UINT32 state);

/* ----------------------------------------------------------------------- */

extern void filter_eoln_getinfo(UINT32 state, union filterinfo *info);
extern void filter_cocobas_getinfo(UINT32 state, union filterinfo *info);
extern void filter_dragonbas_getinfo(UINT32 state, union filterinfo *info);
extern void filter_macbinary_getinfo(UINT32 state, union filterinfo *info);



#endif /* FILTER_H */
