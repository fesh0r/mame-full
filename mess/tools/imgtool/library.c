/****************************************************************************

	library.c

	Code relevant to the Imgtool library; analgous to the MESS/MAME driver
	list.

****************************************************************************/

#include <assert.h>
#include <string.h>

#include "library.h"
#include "pool.h"

struct _imgtool_library
{
	memory_pool pool;
	struct ImageModule *first;
	struct ImageModule *last;
};



imgtool_library *imgtool_library_create(void)
{
	imgtool_library *library;

	library = malloc(sizeof(struct _imgtool_library));
	if (!library)
		return NULL;
	memset(library, 0, sizeof(*library));

	pool_init(&library->pool);
	return library;
}



void imgtool_library_close(imgtool_library *library)
{
	pool_exit(&library->pool);
	free(library);
}



void imgtool_library_sort(imgtool_library *library, imgtool_libsort_t sort)
{
	/* NYI */
}



imgtoolerr_t imgtool_library_createmodule(imgtool_library *library,
	const char *module_name, struct ImageModule **module)
{
	struct ImageModule *newmodule;
	char *alloc_module_name;

	newmodule = pool_malloc(&library->pool, sizeof(struct ImageModule));
	if (!newmodule)
		goto outofmemory;

	alloc_module_name = pool_strdup(&library->pool, module_name);
	if (!alloc_module_name)
		goto outofmemory;

	memset(newmodule, 0, sizeof(*newmodule));
	newmodule->previous = library->last;
	newmodule->name = alloc_module_name;

	if (library->last)
		library->last->next = newmodule;
	else
		library->first = newmodule;
	library->last = newmodule;

	*module = newmodule;
	return IMGTOOLERR_SUCCESS;

outofmemory:
	*module = NULL;
	return IMGTOOLERR_OUTOFMEMORY;
}



const struct ImageModule *imgtool_library_findmodule(
	imgtool_library *library, const char *module_name)
{
	const struct ImageModule *module;

	assert(library);
	module = library->first;
	while(module && module_name && strcmp(module->name, module_name))
		module = module->next;
	return module;
}



const struct ImageModule *imgtool_library_iterate(
	imgtool_library *library, const struct ImageModule *module)
{
	return module ? module->next : library->first;
}



const struct ImageModule *imgtool_library_index(
	imgtool_library *library, int i)
{
	const struct ImageModule *module;
	module = library->first;
	while(module && i--)
		module = module->next;
	return module;
}



void *imgtool_library_alloc(imgtool_library *library, size_t mem)
{
	return pool_malloc(&library->pool, mem);
}



char *imgtool_library_strdup(imgtool_library *library, const char *s)
{
	return pool_strdup(&library->pool, s);
}


