/******************************************************************************

  MESS - device.c

  List of all available devices and Device handling interfaces.

******************************************************************************/

#include "device.h"

/* The List of Devices, with Associated Names */
const struct Devices devices[IO_COUNT] =
{
	{IO_END,		"NONE",			"NONE"},
	{IO_CARTSLOT,	"cartridge",	"cart"},
	{IO_FLOPPY,		"floppydisk",	"flop"},
	{IO_HARDDISK,	"harddisk",		"hard"},
	{IO_CYLINDER,	"cylinder",		"cyln"},
	{IO_CASSETTE,	"cassette",		"cass"},
	{IO_PUNCHCARD,	"punchcard",	"pcrd"},
	{IO_PUNCHTAPE,	"punchtape",	"ptap"},
	{IO_PRINTER,	"printer",		"prin"},
	{IO_SERIAL,		"serial",		"serl"},
	{IO_PARALLEL,   "parallel",		"parl"},
	{IO_SNAPSHOT,	"snapshot",		"dump"},
	{IO_QUICKLOAD,	"quickLoad",	"quik"}
};
