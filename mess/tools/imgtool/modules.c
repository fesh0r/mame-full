#include "modules.h"

#ifndef MODULES_RECURSIVE
#define MODULES_RECURSIVE

/* step 1: declare all external references */
#define MODULE(name)	extern imgtoolerr_t name##_createmodule(imgtool_library *library);
#include "modules.c"
#undef MODULE

/* step 2: define the modules[] array */
#define MODULE(name)	name##_createmodule,
static imgtoolerr_t (*modules[])(imgtool_library *library) =
{
#include "modules.c"
};

/* step 3: declare imgtool_create_cannonical_library() */
imgtoolerr_t imgtool_create_cannonical_library(imgtool_library **library)
{
	imgtoolerr_t err;
	size_t i;
	imgtool_library *lib;

	static const char *irrelevant_modules[] =
	{
		"coco_os9_rsdos"
	};

	lib = imgtool_library_create();
	if (!lib)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto error;
	}

	for (i = 0; i < sizeof(modules) / sizeof(modules[0]); i++)
	{
		err = modules[i](lib);
		if (err)
			goto error;
	}

	/* remove irrelevant modules */
	for (i = 0; i < sizeof(irrelevant_modules)
			/ sizeof(irrelevant_modules[0]); i++)
	{
		imgtool_library_unlink(lib, irrelevant_modules[i]);
	}

	*library = lib;
	return IMGTOOLERR_SUCCESS;

error:
	if (lib)
		imgtool_library_close(lib);
	return err;

}


#else /* MODULES_RECURSIVE */

MODULE(concept)
MODULE(mac)
MODULE(mess_hd)
MODULE(rsdos)
MODULE(os9)
MODULE(ti99)
MODULE(ti990)
MODULE(fat)

#endif /* MODULES_RECURSIVE */
