#include "apexc.h"

unsigned DasmAPEXC(char *buffer, unsigned pc)
{
	sprintf(buffer, "$%08X", cpu_readop(pc));
	return 1;
}
