#include <assert.h>
#include <string.h>
#include "imgtool.h"
#include "utils.h"

struct filter_instance
{
	FILTERMODULE module;
	int (*filterproc)(struct filter_info *fi, void *buf, int buflen);
	void *filterparam;
	void *bufferbase;
	UINT8 *buffer;
	int buffersize;
	int statedata;
};

/* ----------------------------------------------------------------------- */

FILTER *filter_init(FILTERMODULE filter, const struct ImageModule *imgmod, int purpose)
{
	int instancesize;
	struct filter_instance *instance;

	instancesize = sizeof(struct filter_instance) - sizeof(int) + filter->statesize;
	instance = malloc(instancesize);
	if (!instance)
		return NULL;

	memset(instance, 0, instancesize);

	switch(purpose) {
	case PURPOSE_READ:
		instance->filterparam = filter->calcreadparam(imgmod);
		instance->filterproc = filter->filterreadproc;
		break;

	case PURPOSE_WRITE:
		instance->filterparam = filter->calcwriteparam(imgmod);
		instance->filterproc = filter->filterwriteproc;
		break;

	default:
		assert(0);
		break;
	};

	assert(instance->filterproc);

	instance->module = filter;
	return (FILTER *) instance;
}

void filter_term(FILTER *f)
{
	free(f);
}

/* ----------------------------------------------------------------------- */

static int filter_writetostream_sendproc(struct filter_info *fi, void *buf, int buflen)
{
	return stream_write((STREAM *) fi->internalparam, buf, buflen);
}

int filter_writetostream(FILTER *f, STREAM *s, const void *buf, int buflen)
{
	struct filter_instance *instance = (struct filter_instance *) f;
	struct filter_info fi;

	fi.sendproc = filter_writetostream_sendproc;
	fi.filterstate = (void *) &instance->statedata;
	fi.filterparam = instance->filterparam;
	fi.internalparam = (void *) s;

	return instance->filterproc(&fi, (void *) buf, buflen);
}

/* ----------------------------------------------------------------------- */

struct filter_readfromstream_param {
	/*char **/UINT8 *buf;
	int buflen;
	/*char **/UINT8 *extrabuf;
	int extrabuflen;
};

static int filter_readfromstream_sendproc(struct filter_info *fi, void *buf, int buflen)
{
	int sz;
	int result = 0;
	/*char **/UINT8 *p;
	struct filter_readfromstream_param *param = (struct filter_readfromstream_param *) fi->internalparam;

	sz = MIN(param->buflen, buflen);
	if (sz) {
		memcpy(param->buf, buf, buflen);
		param->buf += sz;
		param->buflen -= sz;
		buflen -= sz;
		result += sz;
	}

	if (buflen) {
		if (param->extrabuf) {
			assert(param->extrabuflen > 0);
			p = realloc(param->extrabuf, param->extrabuflen + buflen);
			if (!p)
				return result;	/* Should report out of memory!!! */
			param->extrabuf = p;
			p += param->extrabuflen;
		}
		else {
			assert(param->extrabuflen == 0);
			p = realloc(param->extrabuf, param->extrabuflen + buflen);
			if (!p)
				return result;	/* Should report out of memory!!! */
			param->extrabuf = p;
		}
		memcpy(p, buf, buflen);
		param->extrabuflen += buflen;
	}

	return result;
}

int filter_readfromstream(FILTER *f, STREAM *s, void *buf, int buflen)
{
	int sz;
	int result = 0;
	UINT8 *bbuf = (UINT8 *) buf;
	struct filter_instance *instance = (struct filter_instance *) f;
	struct filter_info fi;
	struct filter_readfromstream_param param;
	char localbuffer[1024];

	sz = MIN(instance->buffersize, buflen);
	if (sz) {
		memcpy(bbuf, instance->buffer, sz);
		instance->buffersize -= sz;
		instance->buffer += sz;
		if (instance->buffersize == 0) {
			free(instance->bufferbase);
			instance->bufferbase = NULL;
			instance->buffer = NULL;
		}
		bbuf += sz;
		buflen -= sz;
		result += sz;
	}

	fi.sendproc = filter_readfromstream_sendproc;
	fi.filterstate = (void *) &instance->statedata;
	fi.filterparam = instance->filterparam;
	fi.internalparam = (void *) &param;
	param.buf = bbuf;
	param.buflen = buflen;
	param.extrabuf = NULL;
	param.extrabuflen = 0;

	sz = -1;

	while (sz && (buflen > 0)) {
		sz = stream_read(s, localbuffer, MIN(buflen, sizeof(localbuffer) / sizeof(localbuffer[0])));
		instance->filterproc(&fi, localbuffer, sz);
		buflen -= sz;
	}

	if (param.extrabuf) {
		instance->buffer = param.extrabuf;
		instance->bufferbase = param.extrabuf;
		instance->buffersize = param.extrabuflen;
	}

	return result;
}

/* This function processes all data, reads it into its buffer, and returns the size of the data */
int filter_readintobuffer(FILTER *f, STREAM *s)
{
	int sz;
	int result = 0;
	struct filter_instance *instance = (struct filter_instance *) f;
	struct filter_info fi;
	struct filter_readfromstream_param param;
	char localbuffer[1024];

	/* Need to normalize buffer */
	if (instance->bufferbase != instance->buffer) {
		memmove(instance->bufferbase, instance->buffer, instance->buffersize);
		instance->buffer = instance->bufferbase;
	}

	fi.sendproc = filter_readfromstream_sendproc;
	fi.filterstate = (void *) &instance->statedata;
	fi.filterparam = instance->filterparam;
	fi.internalparam = (void *) &param;
	param.buf = NULL;
	param.buflen = 0;
	param.extrabuf = instance->buffer;
	param.extrabuflen = instance->buffersize;

	result = -param.extrabuflen;

	do {
		sz = stream_read(s, localbuffer, sizeof(localbuffer) / sizeof(localbuffer[0]));
		instance->filterproc(&fi, localbuffer, sz);
	}
	while(sz > 0);

	result += param.extrabuflen;
	instance->buffer = param.extrabuf;
	instance->bufferbase = param.extrabuf;
	instance->buffersize = param.extrabuflen;

	return result;
}


/* ----------------------------------------------------------------------- */

extern struct filter_module filter_eoln;
extern struct filter_module filter_cocobas;
extern struct filter_module filter_dragonbas;

const struct filter_module *filters[] =
{
	&filter_eoln,
	&filter_cocobas,
	&filter_dragonbas,
	NULL
};

const struct filter_module *filter_lookup(const char *name)
{
	int i;

	for (i = 0; filters[i]; i++) {
		if (!strcmp(name, filters[i]->name))
			return filters[i];
	}

	return NULL;
}

