#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "osdepend.h"
#include "imgtoolx.h"
#include "osd_cpu.h"
#include "utils.h"
#include "library.h"
#include "modules.h"

/* ----------------------------------------------------------------------- */

struct imgtool_module_features img_get_module_features(const struct ImageModule *module)
{
	struct imgtool_module_features features;
	memset(&features, 0, sizeof(features));

	if (module->create)
		features.supports_create = 1;
	if (module->open)
		features.supports_open = 1;
	if (module->read_file)
		features.supports_reading = 1;
	if (module->write_file)
		features.supports_writing = 1;
	if (module->delete_file)
		features.supports_deleting = 1;
	return features;
}



int imgtool_validitychecks(void)
{
	int error = 0;
	imgtoolerr_t err;
	imgtool_library *library;

	err = imgtool_create_cannonical_library(&library);
	if (err)
		goto done;

done:
	if (err)
	{
		printf("imgtool: %s\n", imgtool_error(err));
		error = 1;
	}
	if (library)
		imgtool_library_close(library);
	return error;
}

