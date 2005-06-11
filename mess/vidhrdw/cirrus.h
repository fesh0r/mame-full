/***************************************************************************

	vidhrdw/cirrus.h

	Cirrus SVGA card emulation (preliminary)

***************************************************************************/

#ifndef CIRRUS_H
#define CIRRUS_H

#include "pc_vga.h"
#include "machine/pci.h"


extern const struct pc_svga_interface cirrus_svga_interface;
extern const struct pci_device_info cirrus5430_callbacks;

#endif /* CIRRUS_H */
