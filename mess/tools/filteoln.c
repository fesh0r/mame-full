#include <string.h>
#include "imgtool.h"
#include "osdutils.h"

struct filter_eoln_state
{
	int after_cr;
};

static int filter_eoln_proc(struct filter_info *fi, void *buf, int buflen)
{
	struct filter_eoln_state *filterstate;
	char *cbuf;
	char c;
	int base;
	char *eoln;
	int eolnsize;
	int result, i;

	filterstate = (struct filter_eoln_state *) fi->filterstate;
	cbuf = (char *) buf;
	base = 0;
	eoln = (char *) fi->filterparam;
	eolnsize = strlen(eoln);
	result = 0;

	for (i = 0; i < buflen; i++) {
		c = cbuf[i];

		switch(c) {
		case '\x0a':
		case '\x0d':
			if ((c != '\x0a') || (!filterstate->after_cr)) {
				if (base != i)
					result += fi->sendproc(fi, &cbuf[base], i - base);
				result += fi->sendproc(fi, eoln, eolnsize);
			}
			base = i + 1;
			break;
		}

		filterstate->after_cr = (c == '\x0d');
	}
	if (base != i)
		result += fi->sendproc(fi, &cbuf[base], i - base);
	return result;
}

static void *filter_eoln_calcreadparam(const struct ImageModule *imgmod)
{
	return (void *) EOLN;
}

static void *filter_eoln_calcwriteparam(const struct ImageModule *imgmod)
{
	return (void *) imgmod->eoln;
}

struct filter_module filter_eoln =
{
	"ascii",
	"Ascii Text Filter",
	filter_eoln_calcreadparam,
	filter_eoln_calcwriteparam,
	filter_eoln_proc,
	sizeof(struct filter_eoln_state)
};