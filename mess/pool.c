#include <assert.h>
#include <string.h>

#include "pool.h"

/***************************************************************************

	Pool code

***************************************************************************/

#ifdef DEBUG
#include "osd_cpu.h"
#define GUARD_BYTES
#endif

struct pool_memory_header
{
	struct pool_memory_header *next;
	struct pool_memory_header **prev;
#ifdef GUARD_BYTES
	size_t size;
	UINT32 canary;
#endif
};

void pool_init(memory_pool *pool)
{
	*pool = NULL;
}

void pool_exit(memory_pool *pool)
{
	struct pool_memory_header *mem;
	struct pool_memory_header *next;

	mem = (struct pool_memory_header *) *pool;
	while(mem)
	{
#ifdef GUARD_BYTES
		assert(mem->canary == 0xdeadbeef);
		assert(!memcmp(&mem->canary, ((char *) (mem+1)) + mem->size, sizeof(mem->canary)));
#endif
		next = mem->next;
		free(mem);
		mem = next;
	}
	*pool = NULL;
}

void *pool_realloc(memory_pool *pool, void *ptr, size_t size)
{
	struct pool_memory_header *block;
	size_t actual_size;

	actual_size = sizeof(struct pool_memory_header) + size;
#ifdef GUARD_BYTES
	actual_size += sizeof(block->canary);
#endif

	if (ptr)
	{
		block = ((struct pool_memory_header *) ptr) - 1;
#ifdef GUARD_BYTES
		assert(block->canary == 0xdeadbeef);
#endif
		block = realloc(block, actual_size);
		if (!block)
			return NULL;
	}
	else
	{
		block = malloc(actual_size);
		if (!block)
			return NULL;
		block->next = (struct pool_memory_header *) *pool;
		block->prev = (struct pool_memory_header **) pool;
	}
	if (block->next)
		block->next->prev = &block->next;
	*(block->prev) = block;

#ifdef GUARD_BYTES
	block->size = size;
	block->canary = 0xdeadbeef;
	memcpy(((char *) (block+1)) + size, &block->canary, sizeof(block->canary));
#endif
	return (void *) (block+1);
}

void *pool_malloc(memory_pool *pool, size_t size)
{
	return pool_realloc(pool, NULL, size);
}

char *pool_strdup(memory_pool *pool, const char *src)
{
	char *dst = NULL;
	if (src)
	{
		dst = pool_malloc(pool, strlen(src) + 1);
		if (dst)
			strcpy(dst, src);
	}
	return dst;
}

void pool_freeptr(memory_pool *pool, void *ptr)
{
	struct pool_memory_header *block;

	block = ((struct pool_memory_header *) ptr) - 1;

	if (block->next)
		block->next->prev = block->prev;
	if (block->prev)
		*(block->prev) = block->next;

	free(block);
}


/***************************************************************************

	Tagpool code

***************************************************************************/

struct tag_pool_header
{
	struct tag_pool_header *next;
	const char *tagname;
	void *tagdata;
};

void tagpool_init(tag_pool *tpool)
{
	pool_init(&tpool->mempool);
	tpool->header = NULL;
}

void tagpool_exit(tag_pool *tpool)
{
	pool_exit(&tpool->mempool);
	tpool->header = NULL;
}

void *tagpool_alloc(tag_pool *tpool, const char *tag, size_t size)
{
	struct tag_pool_header **header;
	struct tag_pool_header *newheader;

	newheader = (struct tag_pool_header *) pool_malloc(&tpool->mempool, sizeof(struct tag_pool_header));
	if (!newheader)
		return NULL;

	newheader->tagdata = pool_malloc(&tpool->mempool, size);
	if (!newheader->tagdata)
		return NULL;

	newheader->next = NULL;
	newheader->tagname = tag;
	memset(newheader->tagdata, 0, size);

	header = &tpool->header;
	while (*header)
		header = &(*header)->next;
	*header = newheader;

	return newheader->tagdata;
}

void *tagpool_lookup(tag_pool *tpool, const char *tag)
{
	struct tag_pool_header *header;

	assert(tpool);

	header = tpool->header;
	while(header && strcmp(header->tagname, tag))
		header = header->next;

	/* a tagpool_lookup with an invalid tag is BAD */
	assert(header);

	return header->tagdata;
}


