/*********************************************************************

	messtest.h

	Automated testing for MAME/MESS

*********************************************************************/

#ifndef MESSTEST_H
#define MESSTEST_H

#include <expat.h>
#include "mame.h"
#include "timer.h"

int messtest(const char *script_filename, int flags, int *test_count, int *failure_count);

int memory_region_from_string(const char *region_name);
const char *memory_region_to_string(int region);

const XML_Char *find_attribute(const XML_Char **attributes, const XML_Char *seek_attribute);
mame_time parse_time(const char *s);
void parse_offset(const char *s, offs_t *result);

struct messtest_state;

typedef enum
{
	DATA_NONE,
	DATA_TEXT,
	DATA_BINARY
} tagdatatype_t;

struct messtest_tagdispatch
{
	const char *tag;
	tagdatatype_t datatype;
	void (*start_handler)(struct messtest_state *state, const XML_Char **attributes);
	void (*end_handler)(struct messtest_state *state, const void *ptr, size_t len);
	struct messtest_tagdispatch *subdispatch;
};

void error_missingattribute(struct messtest_state *state, const char *attribute);
void error_outofmemory(struct messtest_state *state);
void error_invalidmemregion(struct messtest_state *state, const char *s);
void error_baddevicetype(struct messtest_state *state, const char *s);

void report_testcase_ran(struct messtest_state *state, int failure);

extern struct messtest_tagdispatch test_dispatch[];
void test_start_handler(struct messtest_state *state, const XML_Char **attributes);
void test_end_handler(struct messtest_state *state, const void *buffer, size_t size);

#endif /* MESSTEST_H */
