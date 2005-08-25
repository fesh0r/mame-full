/***************************************************************************

    datafile.h

    Controls execution of the core MAME system.

***************************************************************************/

#pragma once

#ifndef __DATAFILE_H__
#define __DATAFILE_H__

struct tDatafileIndex
{
	long offset;
	const game_driver *driver;
};

extern int load_driver_history (const game_driver *drv, char *buffer, int bufsize);

#endif	/* __DATAFILE_H__ */
