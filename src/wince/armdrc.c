/*###################################################################################################
**
**
**		armdrc.c
**		ARM Dynamic recompiler support routines.
**		Written by Nathan Woods
**
**
**#################################################################################################*/

#include "driver.h"
#include "armdrc.h"

/*###################################################################################################
**	EXTERNAL INTERFACES
**#################################################################################################*/

/*------------------------------------------------------------------
	drc_init
------------------------------------------------------------------*/

struct drccore *drc_init(UINT8 cpunum, struct drcconfig *config)
{
	struct drccore *drc;

	/* allocate memory */
	drc = malloc(sizeof(*drc));
	if (!drc)
		return NULL;
	memset(drc, 0, sizeof(*drc));

	/* allocate cache */
	drc->cache_base = malloc(config->cachesize);
	if (!drc->cache_base)
		return NULL;
	drc->cache_top = drc->cache_base;
	drc->cache_end = drc->cache_base + config->cachesize;
	drc->cache_danger = drc->cache_end - 65536;
	return drc;
}

/*------------------------------------------------------------------
	drc_exit
------------------------------------------------------------------*/

void drc_exit(struct drccore *drc)
{
	int i;

	/* free the cache */
	if (drc->cache_base)
		free(drc->cache_base);

	/* and the drc itself */
	free(drc);
}

/*------------------------------------------------------------------
	build_immediate_operand
------------------------------------------------------------------*/

UINT32 build_immediate_operand(UINT32 *op)
{
	UINT64 extended_op;
	UINT32 shift, mask, result;

	extended_op = *op;
	extended_op |= extended_op << 32;

	shift = 0;
	while(!((extended_op << (shift * 2)) & 0x000000ff00000000) && (shift < 15))
		shift++;

	mask = (UINT32) (0xffffff00ffffff00 >> (shift * 2));
	result = (((UINT32) extended_op >> (shift * 2)) & 0xff) | (shift << 8);

	*op &= mask;
	return result;
}
