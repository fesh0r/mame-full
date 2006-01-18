/***************************************************************************

    mesvalid.c

    Validity checks on internal MESS data structures.

***************************************************************************/

#include "mess.h"
#include "device.h"
#include "ui_text.h"
#include "inputx.h"

int mess_validitychecks(void)
{
	int i, j;
	int is_invalid;
	int error = 0;
	iodevice_t devtype;
	struct IODevice *devices;
	const struct IODevice *dev;
	const char *name;
	const char *s1;
	const char *s2;
	input_port_entry *inputports = NULL;
	extern int device_valididtychecks(void);
	extern const char *mess_default_text[];

	/* make sure that all of the UI_* strings are set for all devices */
	for (devtype = 0; devtype < IO_COUNT; devtype++)
	{
		name = mess_default_text[UI_cartridge - IO_CARTSLOT - UI_last_mame_entry + devtype];
		if (!name || !name[0])
		{
			printf("Device type %d does not have an associated UI string\n", devtype);
			error = 1;
		}
	}

	/* MESS specific driver validity checks */
	for (i = 0; drivers[i]; i++)
	{
		devices = devices_allocate(drivers[i]);

		/* make sure that there are no clones that reference nonexistant drivers */
		if (drivers[i]->clone_of && !(drivers[i]->clone_of->flags & NOT_A_DRIVER))
		{
			if (drivers[i]->compatible_with && !(drivers[i]->compatible_with->flags & NOT_A_DRIVER))
			{
				printf("%s: both compatbile_with and clone_of are specified\n", drivers[i]->name);
				error = 1;
			}

			for (j = 0; drivers[j]; j++)
			{
				if (drivers[i]->clone_of == drivers[j])
					break;
			}
			if (!drivers[j])
			{
				printf("%s: is a clone of %s, which is not in drivers[]\n", drivers[i]->name, drivers[i]->clone_of->name);
				error = 1;
			}
		}

		/* make sure that there are no clones that reference nonexistant drivers */
		if (drivers[i]->compatible_with && !(drivers[i]->compatible_with->flags & NOT_A_DRIVER))
		{
			for (j = 0; drivers[j]; j++)
			{
				if (drivers[i]->compatible_with == drivers[j])
					break;
			}
			if (!drivers[j])
			{
				printf("%s: is compatible with %s, which is not in drivers[]\n", drivers[i]->name, drivers[i]->compatible_with->name);
				error = 1;
			}
		}

		/* check device array */
		for (j = 0; devices[j].type < IO_COUNT; j++)
		{
			dev = &devices[j];

			if (dev->type >= IO_COUNT)
			{
				printf("%s: invalid device type %i\n", drivers[i]->name, dev->type);
				error = 1;
			}

			/* File Extensions Checks
			 * 
			 * Checks the following
			 *
			 * 1.  Tests the integrity of the string list
			 * 2.  Checks for duplicate extensions
			 * 3.  Makes sure that all extensions are either lower case chars or numbers
			 */
			if (!dev->file_extensions)
			{
				printf("%s: device type '%s' has null file extensions\n", drivers[i]->name, device_typename(dev->type));
				error = 1;
			}
			else
			{
				s1 = dev->file_extensions;
				while(*s1)
				{
					/* check for invalid chars */
					is_invalid = 0;
					for (s2 = s1; *s2; s2++)
					{
						if (!isdigit(*s2) && !islower(*s2))
							is_invalid = 1;
					}
					if (is_invalid)
					{
						printf("%s: device type '%s' has an invalid extension '%s'\n", drivers[i]->name, device_typename(dev->type), s1);
						error = 1;
					}
					s2++;

					/* check for dupes */
					is_invalid = 0;
					while(*s2)
					{
						if (!strcmp(s1, s2))
							is_invalid = 1;
						s2 += strlen(s2) + 1;
					}
					if (is_invalid)
					{
						printf("%s: device type '%s' has duplicate extensions '%s'\n", drivers[i]->name, device_typename(dev->type), s1);
						error = 1;
					}

					s1 += strlen(s1) + 1;
				}
			}

			/* enforce certain rules for certain device types */
			switch(dev->type)
			{
				case IO_QUICKLOAD:
				case IO_SNAPSHOT:
					if (dev->count != 1)
					{
						printf("%s: there can only be one instance of devices of type '%s'\n", drivers[i]->name, device_typename(dev->type));
						error = 1;
					}
					/* fallthrough */

				case IO_CARTSLOT:
					if (!dev->readable || dev->writeable || dev->creatable)
					{
						printf("%s: devices of type '%s' has invalid open modes\n", drivers[i]->name, device_typename(dev->type));
						error = 1;
					}
					break;
					
				default:
					break;
			}
		}

		/* check system config */
		ram_option_count(drivers[i]);

		/* make sure that our input system likes this driver */
		if (inputx_validitycheck(drivers[i], &inputports))
			error = 1;

		devices = NULL;
	}

	/* call other validity checks */
	if (inputx_validitycheck(NULL, &inputports))
		error = 1;
	if (device_valididtychecks())
		error = 1;

#ifdef WIN32
	/* MESS on windows has its own validity checks */
	{
		extern int win_mess_validitychecks(void);
		if (win_mess_validitychecks())
			error = 1;
	}
#endif /* WIN32 */

	/* now that we are completed, re-expand the actual driver to compensate
	 * for the tms9928a hack */
	if (Machine && Machine->gamedrv)
	{
		machine_config drv;
		expand_machine_driver(Machine->gamedrv->drv, &drv);
	}

	return error;
}
