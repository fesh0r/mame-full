/*
	rtc65271.h: include file for rtc65271.c
*/

#include "mame.h"

extern void rtc65271_init(UINT8 *xram);
extern UINT8 rtc65271_r(int xramsel, offs_t offset);
extern void rtc65271_w(int xramsel, offs_t offset, UINT8 data);
