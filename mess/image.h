/****************************************************************************

	image.h

	Code for handling devices/software images

****************************************************************************/

#ifndef IMAGE_H
#define IMAGE_H

#include <stdlib.h>
#include "fileio.h"
#include "utils.h"
#include "osdepend.h"

/****************************************************************************
  A mess_image pointer represents one device entry; this could be an instance
  of a floppy drive or a printer.  The defining property is that either at
  startup or at runtime, it can become associated with a file on the hosting
  machine (for the examples above, a disk image or printer output file
  repsectively).  In fact, mess_image might be better renamed mess_device.

  MESS drivers declare what device types each system had and how many.  For
  each of these, a mess_image is allocated.  Devices can have init functions
  that allow the devices to set up any relevant internal data structures.
  It is recommended that devices use the tag allocation system described
  below.

  There are also various other accessors for other information about a
  device, from whether it is mounted or not, or information stored in a CRC
  file.
****************************************************************************/

/* not to be called by anything other than core */
int image_init(mess_image *img);
void image_exit(mess_image *img);

/****************************************************************************
  Device loading and unloading functions

  The UI can call image_load and image_unload to associate and disassociate
  with disk images on disk.  In fact, devices that unmount on their own (like
  Mac floppy drives) may call this from within a driver.
****************************************************************************/

int image_load(mess_image *img, const char *name);
void image_unload(mess_image *img);
void image_unload_all(int ispreload);

/****************************************************************************
  Tag management functions.
  
  When devices have private data structures that need to be associated with a
  device, it is recommended that image_alloctag() be called in the device
  init function.  If the allocation succeeds, then a pointer will be returned
  to a block of memory of the specified size that will last for the lifetime
  of the emulation.  This pointer can be retrieved with image_lookuptag().

  Note that since image_lookuptag() is used to index preallocated blocks of
  memory, image_lookuptag() cannot fail legally.  In fact, an assert will be
  raised if this happens
****************************************************************************/

void *image_alloctag(mess_image *img, const char *tag, size_t size);
void *image_lookuptag(mess_image *img, const char *tag);

/****************************************************************************
  Accessor functions

  These provide information about the device; and about the mounted image
****************************************************************************/

mame_file *image_fp(mess_image *img);
const struct IODevice *image_device(mess_image *img);
int image_exists(mess_image *img);
int image_slotexists(mess_image *img);

const char *image_typename_id(mess_image *img);
const char *image_filename(mess_image *img);
const char *image_basename(mess_image *img);
const char *image_filetype(mess_image *img);
const char *image_filedir(mess_image *img);
unsigned int image_length(mess_image *img);
unsigned int image_crc(mess_image *img);

/****************************************************************************
  Memory allocators

  These allow memory to be allocated for the lifetime of a mounted image.
  If these (and the above accessors) are used well enough, they should be
  able to eliminate the need for a unload function.
****************************************************************************/

void *image_malloc(mess_image *img, size_t size) FUNCATTR_MALLOC;
char *image_strdup(mess_image *img, const char *src) FUNCATTR_MALLOC;
void *image_realloc(mess_image *img, void *ptr, size_t size);
void image_freeptr(mess_image *img, void *ptr);

/****************************************************************************
  CRC Accessor functions

  When an image is mounted; these functions provide access to the information
  pertaining to that image in the CRC database
****************************************************************************/

const char *image_longname(mess_image *img);
const char *image_manufacturer(mess_image *img);
const char *image_year(mess_image *img);
const char *image_playable(mess_image *img);
const char *image_extrainfo(mess_image *img);

/****************************************************************************
  Battery functions

  These functions provide transparent access to battery-backed RAM on an
  image; typically for cartridges.
****************************************************************************/

int image_battery_load(mess_image *img, void *buffer, int length);
int image_battery_save(mess_image *img, const void *buffer, int length);

/****************************************************************************
  Deprecated functions

  The usage of these functions is to be phased out.  The first group because
  they reflect the outdated fixed relationship between devices and their
  type/id.
****************************************************************************/

mess_image *image_instance(int type, int id);
int image_type(mess_image *img);
int image_index(mess_image *img);

mame_file *image_fopen_custom(mess_image *img, int filetype, int read_or_write);

#endif /* IMAGE_H */
