#include "mess.h"

#ifdef DEBUG
#define GUARD_BYTES
#endif

struct pool_memory_header
{
	struct pool_memory_header *next;
#ifdef GUARD_BYTES
	size_t size;
	UINT32 canary;
#endif
};

void pool_init(void **pool)
{
	*pool = NULL;
}

void pool_exit(void **pool)
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

void *pool_malloc(void **pool, size_t size)
{
	struct pool_memory_header *block;
	size_t actual_size;

	actual_size = sizeof(struct pool_memory_header) + size;
#ifdef GUARD_BYTES
	actual_size += sizeof(block->canary);
#endif

	block = malloc(actual_size);
	if (!block)
		return NULL;

	block->next = (struct pool_memory_header *) *pool;
#ifdef GUARD_BYTES
	block->size = size;
	block->canary = 0xdeadbeef;
	memcpy(((char *) (block+1)) + size, &block->canary, sizeof(block->canary));
#endif

	*pool = block;
	return (void *) (block+1);
}

char *pool_strdup(void **pool, const char *src)
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

