/***************************************************************************

	imgtool.c

	Miscellaneous stuff in the Imgtool core

***************************************************************************/

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
	if (module->path_separator)
		features.supports_directories = 1;
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
	struct imgtool_module_features features;

	err = imgtool_create_cannonical_library(&library);
	if (err)
		goto done;

	while((module = imgtool_library_iterate(library, module)) != NULL)
	{
		features = img_get_module_features(module);

		if (!module->name)
		{
			printf("imgtool module %s has null 'name'\n", module->name);
			error = 1;
		}
		if (!module->description)
		{
			printf("imgtool module %s has null 'description'\n", module->name);
			error = 1;
		}
		if (!module->extensions)
		{
			printf("imgtool module %s has null 'extensions'\n", module->extensions);
			error = 1;
		}

		if (features.supports_directories)
		{
			if (module->read_file)
			{
				printf("imgtool module %s supports directories and read_file without core support\n", module->name);
				error = 1;
			}

			if (module->write_file)
			{
				printf("imgtool module %s supports directories and write_file without core support\n", module->name);
				error = 1;
			}

			if (module->delete_file)
			{
				printf("imgtool module %s supports directories and delete_file without core support\n", module->name);
				error = 1;
			}
		}

		/* sanity checks on creation options */
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
								break;

							default:
								break;
						}
						if (!guide_entry->identifier)
						{
							printf("imgtool module %s creation option %d has null identifier\n",
								module->name, guide_entry - module->createimage_optguide);
							error = 1;
						}
						if (!guide_entry->display_name)
						{
							printf("imgtool module %s creation option %d has null display_name\n",
								module->name, guide_entry - module->createimage_optguide);
							error = 1;
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

