#include "driver.h"
#include "includes/europc.h"

/*
  europc
  fe107 bios checksum test
  fe145 
  fe14b
  fe169 fd774 // test of special europc registers 254 354
  fe16c
  fe16f // test of special europc registers
 */

/*
  254..257 r/w memory ? JIM asic?
  354..357 r/w memory ? JIM asic?
  25a ?
*/

static struct {
	UINT8 data[4];
} europc= { { 0 } } ;

extern WRITE_HANDLER ( europc_w )
{
	europc.data[offset]=data;
}

extern READ_HANDLER ( europc_r )
{
	return europc.data[offset];
}
