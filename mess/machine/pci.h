/***************************************************************************

	machine/pci.h

	PCI bus

***************************************************************************/

#ifndef PCI_H
#define PCI_H

#include "driver.h"

struct pci_device_info
{
	data32_t (*read_callback)(int function, int reg);
	void (*write_callback)(int function, int reg, data32_t data);
};


void pci_init(void);
void pci_add_device(int bus, int device, const struct pci_device_info *devinfo);

READ32_HANDLER(pci_32le_r);
WRITE32_HANDLER(pci_32le_w);

READ64_HANDLER(pci_64be_r);
WRITE64_HANDLER(pci_64be_w);

#endif /* PCI_H */


