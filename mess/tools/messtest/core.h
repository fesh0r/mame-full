/*********************************************************************

	core.h

	Automated testing for MAME/MESS

*********************************************************************/

#ifndef CORE_H
#define CORE_H

#include "mame.h"
#include "timer.h"


/***************************************************************************

	Type definitions

***************************************************************************/

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
	void (*start_handler)(struct messtest_state *state, const char **attributes);
	void (*end_handler)(struct messtest_state *state, const void *ptr, size_t len);
	const struct messtest_tagdispatch *subdispatch;
};



/***************************************************************************

	Prototypes

***************************************************************************/

/* executing the tests */
int messtest(const char *script_filename, int flags, int *test_count, int *failure_count);

/* utility functions to aid in parsing */
int memory_region_from_string(const char *region_name);
const char *memory_region_to_string(int region);
const char *find_attribute(const char **attributes, const char *seek_attribute);
mame_time parse_time(const char *s);
offs_t parse_offset(const char *s);

/* reporting */
void error_report(struct messtest_state *state, const char *message);
void error_reportf(struct messtest_state *state, const char *fmt, ...);
void error_missingattribute(struct messtest_state *state, const char *attribute);
void error_outofmemory(struct messtest_state *state);
void error_invalidmemregion(struct messtest_state *state, const char *s);
void error_baddevicetype(struct messtest_state *state, const char *s);
void report_testcase_ran(struct messtest_state *state, int failure);

#endif /* CORE_H */
