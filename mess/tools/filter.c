#include <assert.h>
#include "imgtool.h"

struct filter_instance {
	struct filter_module *module;
	void *filterparam;
	void *bufferbase;
	UINT8 *buffer;
	int buffersize;
	int statedata;
};

/* ----------------------------------------------------------------------- */

FILTER *filter_init(struct filter_module *filter, struct ImageModule *imgmod, int purpose)
{
	int instancesize;
	void *param;
	struct filter_instance *instance;

	instancesize = sizeof(struct filter_instance) - sizeof(int) + filter->statesize;
	instance = malloc(instancesize);
	if (!instance)
		return NULL;

	switch(purpose) {
	case PURPOSE_READ:
		param = filter->calcreadparam(imgmod);
		break;

	case PURPOSE_WRITE:
		param = filter->calcwriteparam(imgmod);
		break;

	default:
		assert(0);
		param = NULL;
		break;
	};

	memset(instance, 0, instancesize);
	instance->module = filter;
	instance->filterparam = param;
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

int filter_writetostream(FILTER *f, STREAM *s, void *buf, int buflen)
{
	struct filter_instance *instance = (struct filter_instance *) f;
	struct filter_info fi;

	fi.sendproc = filter_writetostream_sendproc;
	fi.filterstate = (void *) &instance->statedata;
	fi.filterparam = instance->filterparam;
	fi.internalparam = (void *) s;

	return instance->module->filterproc(&fi, buf, buflen);
}

/* ----------------------------------------------------------------------- */

struct filter_readfromstream_param {
	char *buf;
	int buflen;
	char *extrabuf;
	int extrabuflen;
};

static int filter_readfromstream_sendproc(struct filter_info *fi, void *buf, int buflen)
{
	int sz;
	int result = 0;
	char *p;
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
	struct filter_instance *instance = (struct filter_instance *) f;
	struct filter_info fi;
	struct filter_readfromstream_param param;
	char localbuffer[1024];

	sz = MIN(instance->buffersize, buflen);
	if (sz) {
		memcpy(buf, instance->buffer, sz);
		instance->buffersize -= sz;
		if (instance->buffersize == 0) {
			free(instance->bufferbase);
			instance->bufferbase = NULL;
			instance->buffer = NULL;
		}
		result += sz;
	}

	fi.sendproc = filter_readfromstream_sendproc;
	fi.filterstate = (void *) &instance->statedata;
	fi.filterparam = instance->filterparam;
	fi.internalparam = (void *) &param;
	param.buf = buf;
	param.buflen = buflen;
	param.extrabuf = NULL;
	param.extrabuflen = 0;

	while (buflen > 0) {
		sz = stream_read(s, localbuffer, MIN(buflen, sizeof(localbuffer) / sizeof(localbuffer[0])));
		instance->module->filterproc(&fi, localbuffer, sz);
		buflen -= sz;
	}

	if (param.extrabuf) {
		instance->buffer = param.extrabuf;
		instance->bufferbase = param.extrabuf;
		instance->buffersize = param.extrabuflen;
	}

	return result;
}

/* ----------------------------------------------------------------------- */

extern struct filter_module filter_eoln;

const struct filter_module *filters[] =
{
	&filter_eoln,
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

