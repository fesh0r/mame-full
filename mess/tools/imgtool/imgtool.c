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
	int val;
	imgtoolerr_t err;
	imgtool_library *library;
	const struct ImageModule *module = NULL;
	const struct OptionGuide *guide_entry;

	err = imgtool_create_cannonical_library(&library);
	if (err)
		goto done;

	while((module = imgtool_library_iterate(library, module)) != NULL)
	{
		if (module->createimage_optguide || module->createimage_optspec)
		{
			if (!module->create)
			{
				printf("imgtool module %s has creation options without supporting create\n", module->name);
				error = 1;
			}
			if (!module->createimage_optguide || !module->createimage_optspec)
			{
				printf("imgtool module %s does has partially incomplete creation options\n", module->name);
				error = 1;
			}
			if (module->createimage_optguide && module->createimage_optspec)
			{
				guide_entry = module->createimage_optguide;
				while(guide_entry->option_type != OPTIONTYPE_END)
				{
					if (option_resolution_contains(module->createimage_optspec, guide_entry->parameter))
					{
						switch(guide_entry->option_type)
						{
							case OPTIONTYPE_INT:
							case OPTIONTYPE_ENUM_BEGIN:
								err = option_resolution_getdefault(module->createimage_optspec,
									guide_entry->parameter, &val);
								if (err)
									goto done;

								if (val <= 0)
								{
//									printf("imgtool module %s creation option %s has no default\n",
//										module->name, guide_entry->display_name);
//									error = 1;
								}
								break;

							default:
								break;
						}
					}
					guide_entry++;
				}
			}
		}
	}

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

