/******************************************************************************

  MESS - device.c

  List of all available devices and Device handling interfaces.

******************************************************************************/

#include "device.h"

/* The List of Devices, with Associated Names - Be careful to ensure that 	*/
/* this list matches the ENUM from device.h, so searches can use IO_COUNT	*/
const struct Devices devices[IO_COUNT] =
{
	{IO_END,		"NONE",			"NONE"}, /*  0 */
	{IO_CARTSLOT,	"cartridge",	"cart"}, /*  1 */
	{IO_FLOPPY,		"floppydisk",	"flop"}, /*  2 */
	{IO_HARDDISK,	"harddisk",		"hard"}, /*  3 */
	{IO_CYLINDER,	"cylinder",		"cyln"}, /*  4 */
	{IO_CASSETTE,	"cassette",		"cass"}, /*  5 */
	{IO_PUNCHCARD,	"punchcard",	"pcrd"}, /*  6 */
	{IO_PUNCHTAPE,	"punchtape",	"ptap"}, /*  7 */
	{IO_PRINTER,	"printer",		"prin"}, /*  8 */
	{IO_SERIAL,		"serial",		"serl"}, /*  9 */
	{IO_PARALLEL,   "parallel",		"parl"}, /* 10 */
	{IO_SNAPSHOT,	"snapshot",		"dump"}, /* 11 */
	{IO_QUICKLOAD,	"quickLoad",	"quik"}, /* 12 */
};
