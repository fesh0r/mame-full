#include "imgtool.h"

struct command
{
	const char *name;
	const char *usage;
	int (*cmdproc)(struct command *c, int argc, char *argv[]);
	int minargs;
	int maxargs;
	int lastargrepeats;
};

void reporterror(int err, struct command *c, const char *format, const char *imagename,
	const char *filename, const char *newname, const struct NamedOption *namedoptions);

#ifdef MAME_DEBUG
int cmd_testsuite(struct command *c, int argc, char *argv[]);
#endif
