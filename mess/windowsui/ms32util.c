#include "ms32util.h"
#include "driver.h"

BOOL DriverIsComputer(int driver_index)
{
	return (drivers[driver_index]->flags & GAME_COMPUTER) != 0;
}

BOOL DriverIsModified(int driver_index)
{
	return (drivers[driver_index]->flags & GAME_COMPUTER_MODIFIED) != 0;
}

