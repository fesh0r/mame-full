/******************************************************************************
 Contributors:

	Marat Fayzullin (MG source)
	Charles Mac Donald
	Mathis Rosenhauer
	Brad Oliver
	Michael Luong

 To do:

 - PSG control for Game Gear (needs custom SN76489 with stereo output for each channel)
 - SIO interface for Game Gear (needs netplay, I guess)
 - TMS9928A support for 'f16ffight.sms'
 - SMS lightgun support
 - SMS Pause key - certainly there's an effective way to handle this
 - LCD persistence emulation for GG
 - SMS 3D glass support

 The Game Gear SIO and PSG hardware are not emulated but have some
 placeholders in 'machine/sms.c'

 Changes:
	Apr 02 - Added raster palette effects for SMS & GG (ML)
				 - Added sprite collision (ML)
				 - Added zoomed sprites (ML)
	May 02 - Fixed paging bug (ML)
				 - Fixed sprite and tile priority bug (ML)
				 - Fixed bug #66 (ML)
				 - Fixed bug #78 (ML)
				 - try to implement LCD persistence emulation for GG (ML)
	Jun 10, 02 - Added bios emulation (ML)
	Jun 12, 02 - Added PAL & NTSC systems (ML)
	Jun 25, 02 - Added border emulation (ML)
	Jun 27, 02 - Version bits for Game Gear (bits 6 of port 00) (ML)

 ******************************************************************************/

#include "driver.h"
#include "sound/sn76496.h"
#include "sound/2413intf.h"
#include "vidhrdw/generic.h"
#include "includes/sms.h"
#include "devices/cartslot.h"

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x3FFF, MRA_RAM }, /* ROM bank #1 */
	{ 0x4000, 0x7FFF, MRA_RAM }, /* ROM bank #2 */
	{ 0x8000, 0xBFFF, MRA_RAM }, /* ROM bank #3 / On-cart RAM */
	{ 0xC000, 0xDFFF, MRA_RAM }, /* RAM */
	{ 0xE000, 0xFFFF, MRA_RAM }, /* RAM (mirror) */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x3FFF, MWA_NOP }, /* ROM bank #1 */
	{ 0x4000, 0x7FFF, MWA_NOP }, /* ROM bank #2 */
	{ 0x8000, 0xBFFF, sms_cartram_w }, /* ROM bank #3 / On-cart RAM */
	{ 0xC000, 0xDFFF, sms_ram_w }, /* RAM */
//	{ 0xC000, 0xDFFB, sms_ram_w }, /* RAM */
//	{ 0xDFFC, 0xDFFF, sms_mapper_w }, /* Bankswitch control */
	{ 0xE000, 0xFFFB, sms_ram_w }, /* RAM (mirror) */
	{ 0xFFFC, 0xFFFF, sms_mapper_w }, /* Bankswitch control */
MEMORY_END

static PORT_READ_START( sms_readport )
	{ 0x00, 0x3F, gg_dummy_r },
	{ 0x40, 0x7F, sms_vdp_curline_r },
	{ 0x80, 0x80, sms_vdp_data_r },
	{ 0x81, 0x81, sms_vdp_ctrl_r },
	{ 0x82, 0x82, sms_vdp_data_r },
	{ 0x83, 0x83, sms_vdp_ctrl_r },
	{ 0x84, 0x84, sms_vdp_data_r },
	{ 0x85, 0x85, sms_vdp_ctrl_r },
	{ 0x86, 0x86, sms_vdp_data_r },
	{ 0x87, 0x87, sms_vdp_ctrl_r },
	{ 0x88, 0x88, sms_vdp_data_r },
	{ 0x89, 0x89, sms_vdp_ctrl_r },
	{ 0x8A, 0x8A, sms_vdp_data_r },
	{ 0x8B, 0x8B, sms_vdp_ctrl_r },
	{ 0x8C, 0x8C, sms_vdp_data_r },
	{ 0x8D, 0x8D, sms_vdp_ctrl_r },
	{ 0x8E, 0x8E, sms_vdp_data_r },
	{ 0x8F, 0x8F, sms_vdp_ctrl_r },
	{ 0x90, 0x90, sms_vdp_data_r },
	{ 0x91, 0x91, sms_vdp_ctrl_r },
	{ 0x92, 0x92, sms_vdp_data_r },
	{ 0x93, 0x93, sms_vdp_ctrl_r },
	{ 0x94, 0x94, sms_vdp_data_r },
	{ 0x95, 0x95, sms_vdp_ctrl_r },
	{ 0x96, 0x96, sms_vdp_data_r },
	{ 0x97, 0x97, sms_vdp_ctrl_r },
	{ 0x98, 0x98, sms_vdp_data_r },
	{ 0x99, 0x99, sms_vdp_ctrl_r },
	{ 0x9A, 0x9A, sms_vdp_data_r },
	{ 0x9B, 0x9B, sms_vdp_ctrl_r },
	{ 0x9C, 0x9C, sms_vdp_data_r },
	{ 0x9D, 0x9D, sms_vdp_ctrl_r },
	{ 0x9E, 0x9E, sms_vdp_data_r },
	{ 0x9F, 0x9F, sms_vdp_ctrl_r },
	{ 0xA0, 0xA0, sms_vdp_data_r },
	{ 0xA1, 0xA1, sms_vdp_ctrl_r },
	{ 0xA2, 0xA2, sms_vdp_data_r },
	{ 0xA3, 0xA3, sms_vdp_ctrl_r },
	{ 0xA4, 0xA4, sms_vdp_data_r },
	{ 0xA5, 0xA5, sms_vdp_ctrl_r },
	{ 0xA6, 0xA6, sms_vdp_data_r },
	{ 0xA7, 0xA7, sms_vdp_ctrl_r },
	{ 0xA8, 0xA8, sms_vdp_data_r },
	{ 0xA9, 0xA9, sms_vdp_ctrl_r },
	{ 0xAA, 0xAA, sms_vdp_data_r },
	{ 0xAB, 0xAB, sms_vdp_ctrl_r },
	{ 0xAC, 0xAC, sms_vdp_data_r },
	{ 0xAD, 0xAD, sms_vdp_ctrl_r },
	{ 0xAE, 0xAE, sms_vdp_data_r },
	{ 0xAF, 0xAF, sms_vdp_ctrl_r },
	{ 0xB0, 0xB0, sms_vdp_data_r },
	{ 0xB1, 0xB1, sms_vdp_ctrl_r },
	{ 0xB2, 0xB2, sms_vdp_data_r },
	{ 0xB3, 0xB3, sms_vdp_ctrl_r },
	{ 0xB4, 0xB4, sms_vdp_data_r },
	{ 0xB5, 0xB5, sms_vdp_ctrl_r },
	{ 0xB6, 0xB6, sms_vdp_data_r },
	{ 0xB7, 0xB7, sms_vdp_ctrl_r },
	{ 0xB8, 0xB8, sms_vdp_data_r },
	{ 0xB9, 0xB9, sms_vdp_ctrl_r },
	{ 0xBA, 0xBA, sms_vdp_data_r },
	{ 0xBB, 0xBB, sms_vdp_ctrl_r },
	{ 0xBC, 0xBC, sms_vdp_data_r },
	{ 0xBD, 0xBD, sms_vdp_ctrl_r },
	{ 0xBE, 0xBE, sms_vdp_data_r },
	{ 0xBF, 0xBF, sms_vdp_ctrl_r },
	{ 0xC0, 0xC0, sms_input_port_0_r },
	{ 0xC1, 0xC1, sms_version_r },
	{ 0xC2, 0xC2, sms_input_port_0_r },
	{ 0xC3, 0xC3, sms_version_r },
	{ 0xC4, 0xC4, sms_input_port_0_r },
	{ 0xC5, 0xC5, sms_version_r },
	{ 0xC6, 0xC6, sms_input_port_0_r },
	{ 0xC7, 0xC7, sms_version_r },
	{ 0xC8, 0xC8, sms_input_port_0_r },
	{ 0xC9, 0xC9, sms_version_r },
	{ 0xCA, 0xCA, sms_input_port_0_r },
	{ 0xCB, 0xCB, sms_version_r },
	{ 0xCC, 0xCC, sms_input_port_0_r },
	{ 0xCD, 0xCD, sms_version_r },
	{ 0xCE, 0xCE, sms_input_port_0_r },
	{ 0xCF, 0xCF, sms_version_r },
	{ 0xD0, 0xD0, sms_input_port_0_r },
	{ 0xD1, 0xD1, sms_version_r },
	{ 0xD2, 0xD2, sms_input_port_0_r },
	{ 0xD3, 0xD3, sms_version_r },
	{ 0xD4, 0xD4, sms_input_port_0_r },
	{ 0xD5, 0xD5, sms_version_r },
	{ 0xD6, 0xD6, sms_input_port_0_r },
	{ 0xD7, 0xD7, sms_version_r },
	{ 0xD8, 0xD8, sms_input_port_0_r },
	{ 0xD9, 0xD9, sms_version_r },
	{ 0xDA, 0xDA, sms_input_port_0_r },
	{ 0xDB, 0xDB, sms_version_r },
	{ 0xDC, 0xDC, sms_input_port_0_r },
	{ 0xDD, 0xDD, sms_version_r },
	{ 0xDE, 0xDE, sms_input_port_0_r },
	{ 0xDF, 0xDF, sms_version_r },
	{ 0xE0, 0xE0, sms_input_port_0_r },
	{ 0xE1, 0xE1, sms_version_r },
	{ 0xE2, 0xE2, sms_input_port_0_r },
	{ 0xE3, 0xE3, sms_version_r },
	{ 0xE4, 0xE4, sms_input_port_0_r },
	{ 0xE5, 0xE5, sms_version_r },
	{ 0xE6, 0xE6, sms_input_port_0_r },
	{ 0xE7, 0xE7, sms_version_r },
	{ 0xE8, 0xE8, sms_input_port_0_r },
	{ 0xE9, 0xE9, sms_version_r },
	{ 0xEA, 0xEA, sms_input_port_0_r },
	{ 0xEB, 0xEB, sms_version_r },
	{ 0xEC, 0xEC, sms_input_port_0_r },
	{ 0xED, 0xED, sms_version_r },
	{ 0xEE, 0xEE, sms_input_port_0_r },
	{ 0xEF, 0xEF, sms_version_r },
	{ 0xF0, 0xF0, sms_input_port_0_r },
	{ 0xF1, 0xF1, sms_version_r },
	{ 0xF2, 0xF2, sms_fm_detect_r },
	{ 0xF3, 0xF3, sms_version_r },
	{ 0xF4, 0xF4, sms_input_port_0_r },
	{ 0xF5, 0xF5, sms_version_r },
	{ 0xF6, 0xF6, sms_input_port_0_r },
	{ 0xF7, 0xF7, sms_version_r },
	{ 0xF8, 0xF8, sms_input_port_0_r },
	{ 0xF9, 0xF9, sms_version_r },
	{ 0xFA, 0xFA, sms_input_port_0_r },
	{ 0xFB, 0xFB, sms_version_r },
	{ 0xFC, 0xFC, sms_input_port_0_r },
	{ 0xFD, 0xFD, sms_version_r },
	{ 0xFE, 0xFE, sms_input_port_0_r },
	{ 0xFF, 0xFF, sms_version_r },
PORT_END

static PORT_WRITE_START( sms_writeport )
	{ 0x00, 0x00, sms_bios_w },
	{ 0x01, 0x01, sms_version_w },
	{ 0x02, 0x02, sms_bios_w },
	{ 0x03, 0x03, sms_version_w },
	{ 0x04, 0x04, sms_bios_w },
	{ 0x05, 0x05, sms_version_w },
	{ 0x06, 0x06, sms_bios_w },
	{ 0x07, 0x07, sms_version_w },
	{ 0x08, 0x08, sms_bios_w },
	{ 0x09, 0x09, sms_version_w },
	{ 0x0A, 0x0A, sms_bios_w },
	{ 0x0B, 0x0B, sms_version_w },
	{ 0x0C, 0x0C, sms_bios_w },
	{ 0x0D, 0x0D, sms_version_w },
	{ 0x0E, 0x0E, sms_bios_w },
	{ 0x0F, 0x0F, sms_version_w },
	{ 0x10, 0x10, sms_bios_w },
	{ 0x11, 0x11, sms_version_w },
	{ 0x12, 0x12, sms_bios_w },
	{ 0x13, 0x13, sms_version_w },
	{ 0x14, 0x14, sms_bios_w },
	{ 0x15, 0x15, sms_version_w },
	{ 0x16, 0x16, sms_bios_w },
	{ 0x17, 0x17, sms_version_w },
	{ 0x18, 0x18, sms_bios_w },
	{ 0x19, 0x19, sms_version_w },
	{ 0x1A, 0x1A, sms_bios_w },
	{ 0x1B, 0x1B, sms_version_w },
	{ 0x1C, 0x1C, sms_bios_w },
	{ 0x1D, 0x1D, sms_version_w },
	{ 0x1E, 0x1E, sms_bios_w },
	{ 0x1F, 0x1F, sms_version_w },
	{ 0x20, 0x20, sms_bios_w },
	{ 0x21, 0x21, sms_version_w },
	{ 0x22, 0x22, sms_bios_w },
	{ 0x23, 0x23, sms_version_w },
	{ 0x24, 0x24, sms_bios_w },
	{ 0x25, 0x25, sms_version_w },
	{ 0x26, 0x26, sms_bios_w },
	{ 0x27, 0x27, sms_version_w },
	{ 0x28, 0x28, sms_bios_w },
	{ 0x29, 0x29, sms_version_w },
	{ 0x2A, 0x2A, sms_bios_w },
	{ 0x2B, 0x2B, sms_version_w },
	{ 0x2C, 0x2C, sms_bios_w },
	{ 0x2D, 0x2D, sms_version_w },
	{ 0x2E, 0x2E, sms_bios_w },
	{ 0x2F, 0x2F, sms_version_w },
	{ 0x30, 0x30, sms_bios_w },
	{ 0x31, 0x31, sms_version_w },
	{ 0x32, 0x32, sms_bios_w },
	{ 0x33, 0x33, sms_version_w },
	{ 0x34, 0x34, sms_bios_w },
	{ 0x35, 0x35, sms_version_w },
	{ 0x36, 0x36, sms_bios_w },
	{ 0x37, 0x37, sms_version_w },
	{ 0x38, 0x38, sms_bios_w },
	{ 0x39, 0x39, sms_version_w },
	{ 0x3A, 0x3A, sms_bios_w },
	{ 0x3B, 0x3B, sms_version_w },
	{ 0x3C, 0x3C, sms_bios_w },
	{ 0x3D, 0x3D, sms_version_w },
	{ 0x3E, 0x3E, sms_bios_w },
	{ 0x3F, 0x3F, sms_version_w },
	{ 0x40, 0x7F, SN76496_0_w },
	{ 0x80, 0x80, sms_vdp_data_w },
	{ 0x81, 0x81, sms_vdp_ctrl_w },
	{ 0x82, 0x82, sms_vdp_data_w },
	{ 0x83, 0x83, sms_vdp_ctrl_w },
	{ 0x84, 0x84, sms_vdp_data_w },
	{ 0x85, 0x85, sms_vdp_ctrl_w },
	{ 0x86, 0x86, sms_vdp_data_w },
	{ 0x87, 0x87, sms_vdp_ctrl_w },
	{ 0x88, 0x88, sms_vdp_data_w },
	{ 0x89, 0x89, sms_vdp_ctrl_w },
	{ 0x8A, 0x8A, sms_vdp_data_w },
	{ 0x8B, 0x8B, sms_vdp_ctrl_w },
	{ 0x8C, 0x8C, sms_vdp_data_w },
	{ 0x8D, 0x8D, sms_vdp_ctrl_w },
	{ 0x8E, 0x8E, sms_vdp_data_w },
	{ 0x8F, 0x8F, sms_vdp_ctrl_w },
	{ 0x90, 0x90, sms_vdp_data_w },
	{ 0x91, 0x91, sms_vdp_ctrl_w },
	{ 0x92, 0x92, sms_vdp_data_w },
	{ 0x93, 0x93, sms_vdp_ctrl_w },
	{ 0x94, 0x94, sms_vdp_data_w },
	{ 0x95, 0x95, sms_vdp_ctrl_w },
	{ 0x96, 0x96, sms_vdp_data_w },
	{ 0x97, 0x97, sms_vdp_ctrl_w },
	{ 0x98, 0x98, sms_vdp_data_w },
	{ 0x99, 0x99, sms_vdp_ctrl_w },
	{ 0x9A, 0x9A, sms_vdp_data_w },
	{ 0x9B, 0x9B, sms_vdp_ctrl_w },
	{ 0x9C, 0x9C, sms_vdp_data_w },
	{ 0x9D, 0x9D, sms_vdp_ctrl_w },
	{ 0x9E, 0x9E, sms_vdp_data_w },
	{ 0x9F, 0x9F, sms_vdp_ctrl_w },
	{ 0xA0, 0xA0, sms_vdp_data_w },
	{ 0xA1, 0xA1, sms_vdp_ctrl_w },
	{ 0xA2, 0xA2, sms_vdp_data_w },
	{ 0xA3, 0xA3, sms_vdp_ctrl_w },
	{ 0xA4, 0xA4, sms_vdp_data_w },
	{ 0xA5, 0xA5, sms_vdp_ctrl_w },
	{ 0xA6, 0xA6, sms_vdp_data_w },
	{ 0xA7, 0xA7, sms_vdp_ctrl_w },
	{ 0xA8, 0xA8, sms_vdp_data_w },
	{ 0xA9, 0xA9, sms_vdp_ctrl_w },
	{ 0xAA, 0xAA, sms_vdp_data_w },
	{ 0xAB, 0xAB, sms_vdp_ctrl_w },
	{ 0xAC, 0xAC, sms_vdp_data_w },
	{ 0xAD, 0xAD, sms_vdp_ctrl_w },
	{ 0xAE, 0xAE, sms_vdp_data_w },
	{ 0xAF, 0xAF, sms_vdp_ctrl_w },
	{ 0xB0, 0xB0, sms_vdp_data_w },
	{ 0xB1, 0xB1, sms_vdp_ctrl_w },
	{ 0xB2, 0xB2, sms_vdp_data_w },
	{ 0xB3, 0xB3, sms_vdp_ctrl_w },
	{ 0xB4, 0xB4, sms_vdp_data_w },
	{ 0xB5, 0xB5, sms_vdp_ctrl_w },
	{ 0xB6, 0xB6, sms_vdp_data_w },
	{ 0xB7, 0xB7, sms_vdp_ctrl_w },
	{ 0xB8, 0xB8, sms_vdp_data_w },
	{ 0xB9, 0xB9, sms_vdp_ctrl_w },
	{ 0xBA, 0xBA, sms_vdp_data_w },
	{ 0xBB, 0xBB, sms_vdp_ctrl_w },
	{ 0xBC, 0xBC, sms_vdp_data_w },
	{ 0xBD, 0xBD, sms_vdp_ctrl_w },
	{ 0xBE, 0xBE, sms_vdp_data_w },
	{ 0xBF, 0xBF, sms_vdp_ctrl_w },
	{ 0xC0, 0xEF, IOWP_NOP }, /* Does nothing */
	{ 0xF0, 0xF0, sms_YM2413_register_port_0_w },
	{ 0xF1, 0xF1, sms_YM2413_data_port_0_w },
	{ 0xF2, 0xF2, sms_fm_detect_w },
	{ 0xF3, 0xFF, IOWP_NOP }, /* Does nothing */
PORT_END

static PORT_READ_START( gg_readport )
	{ 0x00, 0x00, gg_input_port_2_r },
	{ 0x01, 0x05, gg_sio_r },
	{ 0x06, 0x06, gg_psg_r },
	{ 0x07, 0x3F, gg_dummy_r },
	{ 0x40, 0x7F, sms_vdp_curline_r },
	{ 0x80, 0x80, sms_vdp_data_r },
	{ 0x81, 0x81, sms_vdp_ctrl_r },
	{ 0x82, 0x82, sms_vdp_data_r },
	{ 0x83, 0x83, sms_vdp_ctrl_r },
	{ 0x84, 0x84, sms_vdp_data_r },
	{ 0x85, 0x85, sms_vdp_ctrl_r },
	{ 0x86, 0x86, sms_vdp_data_r },
	{ 0x87, 0x87, sms_vdp_ctrl_r },
	{ 0x88, 0x88, sms_vdp_data_r },
	{ 0x89, 0x89, sms_vdp_ctrl_r },
	{ 0x8A, 0x8A, sms_vdp_data_r },
	{ 0x8B, 0x8B, sms_vdp_ctrl_r },
	{ 0x8C, 0x8C, sms_vdp_data_r },
	{ 0x8D, 0x8D, sms_vdp_ctrl_r },
	{ 0x8E, 0x8E, sms_vdp_data_r },
	{ 0x8F, 0x8F, sms_vdp_ctrl_r },
	{ 0x90, 0x90, sms_vdp_data_r },
	{ 0x91, 0x91, sms_vdp_ctrl_r },
	{ 0x92, 0x92, sms_vdp_data_r },
	{ 0x93, 0x93, sms_vdp_ctrl_r },
	{ 0x94, 0x94, sms_vdp_data_r },
	{ 0x95, 0x95, sms_vdp_ctrl_r },
	{ 0x96, 0x96, sms_vdp_data_r },
	{ 0x97, 0x97, sms_vdp_ctrl_r },
	{ 0x98, 0x98, sms_vdp_data_r },
	{ 0x99, 0x99, sms_vdp_ctrl_r },
	{ 0x9A, 0x9A, sms_vdp_data_r },
	{ 0x9B, 0x9B, sms_vdp_ctrl_r },
	{ 0x9C, 0x9C, sms_vdp_data_r },
	{ 0x9D, 0x9D, sms_vdp_ctrl_r },
	{ 0x9E, 0x9E, sms_vdp_data_r },
	{ 0x9F, 0x9F, sms_vdp_ctrl_r },
	{ 0xA0, 0xA0, sms_vdp_data_r },
	{ 0xA1, 0xA1, sms_vdp_ctrl_r },
	{ 0xA2, 0xA2, sms_vdp_data_r },
	{ 0xA3, 0xA3, sms_vdp_ctrl_r },
	{ 0xA4, 0xA4, sms_vdp_data_r },
	{ 0xA5, 0xA5, sms_vdp_ctrl_r },
	{ 0xA6, 0xA6, sms_vdp_data_r },
	{ 0xA7, 0xA7, sms_vdp_ctrl_r },
	{ 0xA8, 0xA8, sms_vdp_data_r },
	{ 0xA9, 0xA9, sms_vdp_ctrl_r },
	{ 0xAA, 0xAA, sms_vdp_data_r },
	{ 0xAB, 0xAB, sms_vdp_ctrl_r },
	{ 0xAC, 0xAC, sms_vdp_data_r },
	{ 0xAD, 0xAD, sms_vdp_ctrl_r },
	{ 0xAE, 0xAE, sms_vdp_data_r },
	{ 0xAF, 0xAF, sms_vdp_ctrl_r },
	{ 0xB0, 0xB0, sms_vdp_data_r },
	{ 0xB1, 0xB1, sms_vdp_ctrl_r },
	{ 0xB2, 0xB2, sms_vdp_data_r },
	{ 0xB3, 0xB3, sms_vdp_ctrl_r },
	{ 0xB4, 0xB4, sms_vdp_data_r },
	{ 0xB5, 0xB5, sms_vdp_ctrl_r },
	{ 0xB6, 0xB6, sms_vdp_data_r },
	{ 0xB7, 0xB7, sms_vdp_ctrl_r },
	{ 0xB8, 0xB8, sms_vdp_data_r },
	{ 0xB9, 0xB9, sms_vdp_ctrl_r },
	{ 0xBA, 0xBA, sms_vdp_data_r },
	{ 0xBB, 0xBB, sms_vdp_ctrl_r },
	{ 0xBC, 0xBC, sms_vdp_data_r },
	{ 0xBD, 0xBD, sms_vdp_ctrl_r },
	{ 0xBE, 0xBE, sms_vdp_data_r },
	{ 0xBF, 0xBF, sms_vdp_ctrl_r },
	{ 0xC0, 0xC0, input_port_0_r },
	{ 0xC1, 0xC1, input_port_1_r },
	{ 0xC2, 0xDB, gg_dummy_r },
	{ 0xDC, 0xDC, input_port_0_r },
	{ 0xDD, 0xDD, input_port_1_r },
	{ 0xDE, 0xFF, gg_dummy_r },
PORT_END

static PORT_WRITE_START( gg_writeport )
	{ 0x00, 0x00, IOWP_NOP },
	{ 0x01, 0x05, gg_sio_w },
	{ 0x06, 0x06, gg_psg_w },
	{ 0x07, 0x07, sms_version_w },
	{ 0x08, 0x08, sms_bios_w },
	{ 0x09, 0x09, sms_version_w },
	{ 0x0A, 0x0A, sms_bios_w },
	{ 0x0B, 0x0B, sms_version_w },
	{ 0x0C, 0x0C, sms_bios_w },
	{ 0x0D, 0x0D, sms_version_w },
	{ 0x0E, 0x0E, sms_bios_w },
	{ 0x0F, 0x0F, sms_version_w },
	{ 0x10, 0x10, sms_bios_w },
	{ 0x11, 0x11, sms_version_w },
	{ 0x12, 0x12, sms_bios_w },
	{ 0x13, 0x13, sms_version_w },
	{ 0x14, 0x14, sms_bios_w },
	{ 0x15, 0x15, sms_version_w },
	{ 0x16, 0x16, sms_bios_w },
	{ 0x17, 0x17, sms_version_w },
	{ 0x18, 0x18, sms_bios_w },
	{ 0x19, 0x19, sms_version_w },
	{ 0x1A, 0x1A, sms_bios_w },
	{ 0x1B, 0x1B, sms_version_w },
	{ 0x1C, 0x1C, sms_bios_w },
	{ 0x1D, 0x1D, sms_version_w },
	{ 0x1E, 0x1E, sms_bios_w },
	{ 0x1F, 0x1F, sms_version_w },
	{ 0x20, 0x20, sms_bios_w },
	{ 0x21, 0x21, sms_version_w },
	{ 0x22, 0x22, sms_bios_w },
	{ 0x23, 0x23, sms_version_w },
	{ 0x24, 0x24, sms_bios_w },
	{ 0x25, 0x25, sms_version_w },
	{ 0x26, 0x26, sms_bios_w },
	{ 0x27, 0x27, sms_version_w },
	{ 0x28, 0x28, sms_bios_w },
	{ 0x29, 0x29, sms_version_w },
	{ 0x2A, 0x2A, sms_bios_w },
	{ 0x2B, 0x2B, sms_version_w },
	{ 0x2C, 0x2C, sms_bios_w },
	{ 0x2D, 0x2D, sms_version_w },
	{ 0x2E, 0x2E, sms_bios_w },
	{ 0x2F, 0x2F, sms_version_w },
	{ 0x30, 0x30, sms_bios_w },
	{ 0x31, 0x31, sms_version_w },
	{ 0x32, 0x32, sms_bios_w },
	{ 0x33, 0x33, sms_version_w },
	{ 0x34, 0x34, sms_bios_w },
	{ 0x35, 0x35, sms_version_w },
	{ 0x36, 0x36, sms_bios_w },
	{ 0x37, 0x37, sms_version_w },
	{ 0x38, 0x38, sms_bios_w },
	{ 0x39, 0x39, sms_version_w },
	{ 0x3A, 0x3A, sms_bios_w },
	{ 0x3B, 0x3B, sms_version_w },
	{ 0x3C, 0x3C, sms_bios_w },
	{ 0x3D, 0x3D, sms_version_w },
	{ 0x3E, 0x3E, sms_bios_w },
	{ 0x3F, 0x3F, sms_version_w },
	{ 0x40, 0x7F, SN76496_0_w },
	{ 0x80, 0x80, sms_vdp_data_w },
	{ 0x81, 0x81, sms_vdp_ctrl_w },
	{ 0x82, 0x82, sms_vdp_data_w },
	{ 0x83, 0x83, sms_vdp_ctrl_w },
	{ 0x84, 0x84, sms_vdp_data_w },
	{ 0x85, 0x85, sms_vdp_ctrl_w },
	{ 0x86, 0x86, sms_vdp_data_w },
	{ 0x87, 0x87, sms_vdp_ctrl_w },
	{ 0x88, 0x88, sms_vdp_data_w },
	{ 0x89, 0x89, sms_vdp_ctrl_w },
	{ 0x8A, 0x8A, sms_vdp_data_w },
	{ 0x8B, 0x8B, sms_vdp_ctrl_w },
	{ 0x8C, 0x8C, sms_vdp_data_w },
	{ 0x8D, 0x8D, sms_vdp_ctrl_w },
	{ 0x8E, 0x8E, sms_vdp_data_w },
	{ 0x8F, 0x8F, sms_vdp_ctrl_w },
	{ 0x90, 0x90, sms_vdp_data_w },
	{ 0x91, 0x91, sms_vdp_ctrl_w },
	{ 0x92, 0x92, sms_vdp_data_w },
	{ 0x93, 0x93, sms_vdp_ctrl_w },
	{ 0x94, 0x94, sms_vdp_data_w },
	{ 0x95, 0x95, sms_vdp_ctrl_w },
	{ 0x96, 0x96, sms_vdp_data_w },
	{ 0x97, 0x97, sms_vdp_ctrl_w },
	{ 0x98, 0x98, sms_vdp_data_w },
	{ 0x99, 0x99, sms_vdp_ctrl_w },
	{ 0x9A, 0x9A, sms_vdp_data_w },
	{ 0x9B, 0x9B, sms_vdp_ctrl_w },
	{ 0x9C, 0x9C, sms_vdp_data_w },
	{ 0x9D, 0x9D, sms_vdp_ctrl_w },
	{ 0x9E, 0x9E, sms_vdp_data_w },
	{ 0x9F, 0x9F, sms_vdp_ctrl_w },
	{ 0xA0, 0xA0, sms_vdp_data_w },
	{ 0xA1, 0xA1, sms_vdp_ctrl_w },
	{ 0xA2, 0xA2, sms_vdp_data_w },
	{ 0xA3, 0xA3, sms_vdp_ctrl_w },
	{ 0xA4, 0xA4, sms_vdp_data_w },
	{ 0xA5, 0xA5, sms_vdp_ctrl_w },
	{ 0xA6, 0xA6, sms_vdp_data_w },
	{ 0xA7, 0xA7, sms_vdp_ctrl_w },
	{ 0xA8, 0xA8, sms_vdp_data_w },
	{ 0xA9, 0xA9, sms_vdp_ctrl_w },
	{ 0xAA, 0xAA, sms_vdp_data_w },
	{ 0xAB, 0xAB, sms_vdp_ctrl_w },
	{ 0xAC, 0xAC, sms_vdp_data_w },
	{ 0xAD, 0xAD, sms_vdp_ctrl_w },
	{ 0xAE, 0xAE, sms_vdp_data_w },
	{ 0xAF, 0xAF, sms_vdp_ctrl_w },
	{ 0xB0, 0xB0, sms_vdp_data_w },
	{ 0xB1, 0xB1, sms_vdp_ctrl_w },
	{ 0xB2, 0xB2, sms_vdp_data_w },
	{ 0xB3, 0xB3, sms_vdp_ctrl_w },
	{ 0xB4, 0xB4, sms_vdp_data_w },
	{ 0xB5, 0xB5, sms_vdp_ctrl_w },
	{ 0xB6, 0xB6, sms_vdp_data_w },
	{ 0xB7, 0xB7, sms_vdp_ctrl_w },
	{ 0xB8, 0xB8, sms_vdp_data_w },
	{ 0xB9, 0xB9, sms_vdp_ctrl_w },
	{ 0xBA, 0xBA, sms_vdp_data_w },
	{ 0xBB, 0xBB, sms_vdp_ctrl_w },
	{ 0xBC, 0xBC, sms_vdp_data_w },
	{ 0xBD, 0xBD, sms_vdp_ctrl_w },
	{ 0xBE, 0xBE, sms_vdp_data_w },
	{ 0xBF, 0xBF, sms_vdp_ctrl_w },
	{ 0xC0, 0xFF, IOWP_NOP }, /* Does nothing */
PORT_END

INPUT_PORTS_START( sms )

	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP			| IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN		| IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT		| IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	| IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1					| IPF_PLAYER1 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2					| IPF_PLAYER1 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP			| IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN		| IPF_8WAY | IPF_PLAYER2 )

	PORT_START	/* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT		| IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	| IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1					| IPF_PLAYER2 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2					| IPF_PLAYER2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) /* Software Reset bit */
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_START ) /* Game Gear START */

INPUT_PORTS_END

static struct SN76496interface sn76496_interface =
{
	1,				/* 1 chip */
	{ 4194304 },			/* 4.194304 MHz */
	{ 100 }
};

static struct YM2413interface ym2413_interface =
{
	1,
	8000000,
	{ 50 },
};

static MACHINE_DRIVER_START(sms)
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3597545)
	MDRV_CPU_MEMORY(readmem, writemem)
	MDRV_CPU_VBLANK_INT(sms, NTSC_Y_PIXELS)
	MDRV_CPU_PORTS(sms_readport, sms_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT(sms)
	MDRV_MACHINE_STOP(sms)
	MDRV_NVRAM_HANDLER(sms)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(NTSC_X_PIXELS, NTSC_Y_PIXELS)
	MDRV_VISIBLE_AREA(29 - 1, NTSC_X_PIXELS - 29 - 1, 10 - 1, NTSC_Y_PIXELS - 9 - 1)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(0)
	/*MDRV_PALETTE_INIT(sms)*/

	MDRV_VIDEO_START(sms)
	MDRV_VIDEO_STOP(sms)
	MDRV_VIDEO_UPDATE(sms)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(smspal)
	MDRV_IMPORT_FROM(sms)
	MDRV_CPU_VBLANK_INT(sms, PAL_Y_PIXELS)

	MDRV_FRAMES_PER_SECOND(50)

	MDRV_SCREEN_SIZE(PAL_X_PIXELS, PAL_Y_PIXELS)
	MDRV_VISIBLE_AREA(29 - 1, PAL_X_PIXELS - 29 - 1, 10 - 1, PAL_Y_PIXELS - 9 - 1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(smsj21)
	MDRV_IMPORT_FROM(smspal)

	MDRV_SOUND_ADD(YM2413, ym2413_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(smsm3)
	MDRV_IMPORT_FROM(smspal)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(gamegear)
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3597545)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(sms, NTSC_Y_PIXELS)
	MDRV_CPU_PORTS(gg_readport,gg_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT(sms)
	MDRV_MACHINE_STOP(sms)
	MDRV_NVRAM_HANDLER(sms)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(NTSC_X_PIXELS, NTSC_Y_PIXELS)
	MDRV_VISIBLE_AREA(6*8, 26*8-1, 3*8, 21*8-1)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(0)
	/*MDRV_PALETTE_INIT(advision)*/

	MDRV_VIDEO_START(sms)
	MDRV_VIDEO_STOP(sms)
	MDRV_VIDEO_UPDATE(sms)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END

ROM_START(sms)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
ROM_END

ROM_START(smspal)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
ROM_END

ROM_START(smsj21)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x2000, REGION_USER1, 0)
	ROM_LOAD("jbios21.rom", 0x0000, 0x2000, CRC(48D44A13))
ROM_END

ROM_START(smsm3)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x2000, REGION_USER1, 0)
	ROM_LOAD("jbios21.rom", 0x0000, 0x2000, CRC(48D44A13))
ROM_END

#define rom_smsss rom_smsj21

ROM_START(smsu13)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x2000, REGION_USER1, 0)
	ROM_LOAD("bios13.rom", 0x0000, 0x2000, CRC(5AD6EDAC))
ROM_END

ROM_START(smse13)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x2000, REGION_USER1, 0)
	ROM_LOAD("bios13.rom", 0x0000, 0x2000, CRC(5AD6EDAC))
ROM_END

ROM_START(smsu13h)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x2000, REGION_USER1, 0)
	ROM_LOAD("bios13fx.rom", 0x0000, 0x2000, CRC(0072ED54))
ROM_END

ROM_START(smse13h)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x2000, REGION_USER1, 0)
	ROM_LOAD("bios13fx.rom", 0x0000, 0x2000, CRC(0072ED54))
ROM_END

ROM_START(smsuam)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x20000, REGION_USER1, 0)
	ROM_LOAD("akbios.rom", 0x0000, 0x20000, CRC(CF4A09EA))
ROM_END

ROM_START(smseam)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x20000, REGION_USER1, 0)
	ROM_LOAD("akbios.rom", 0x0000, 0x20000, CRC(CF4A09EA))
ROM_END

ROM_START(smsesh)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x40000, REGION_USER1, 0)
	ROM_LOAD("sonbios.rom", 0x0000, 0x40000, CRC(81C3476B))
ROM_END

ROM_START(smsbsh)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x40000, REGION_USER1, 0)
	ROM_LOAD("sonbios.rom", 0x0000, 0x40000, CRC(81C3476B))
ROM_END

ROM_START(smsuhs24)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x20000, REGION_USER1, 0)
	ROM_LOAD("hshbios.rom", 0x0000, 0x20000, CRC(91E93385))
ROM_END

ROM_START(smsehs24)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x20000, REGION_USER1, 0)
	ROM_LOAD("hshbios.rom", 0x0000, 0x20000, CRC(91E93385))
ROM_END

ROM_START(smsuh34)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x20000, REGION_USER1, 0)
	ROM_LOAD("hangbios.rom", 0x0000, 0x20000, CRC(8EDF7AC6))
ROM_END

ROM_START(smseh34)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1, 0)
	ROM_REGION(0x20000, REGION_USER1, 0)
	ROM_LOAD("hangbios.rom", 0x0000, 0x20000, CRC(8EDF7AC6))
ROM_END

ROM_START(gamegear)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1,0)
ROM_END

#define rom_gamegj rom_gamegear

ROM_START(gamg)
	ROM_REGION(CPU_ADDRESSABLE_SIZE, REGION_CPU1,0)
	ROM_REGION(0x0400, REGION_USER1, 0)
	ROM_LOAD("majbios.rom", 0x0000, 0x0400, CRC(0EBEA9D4))
ROM_END

#define rom_gamgj rom_gamg

SYSTEM_CONFIG_START(sms)
	CONFIG_DEVICE_CARTSLOT_REQ(1, "sms\0", NULL, NULL, device_load_sms_cart, NULL, NULL, NULL)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(smso)
	CONFIG_DEVICE_CARTSLOT_OPT(1, "sms\0", NULL, NULL, device_load_sms_cart, NULL, NULL, NULL)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(gamegear)
	CONFIG_DEVICE_CARTSLOT_REQ(1, "gg\0", NULL, NULL, device_load_sms_cart, NULL, NULL, NULL)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(gamegearo)
	CONFIG_DEVICE_CARTSLOT_OPT(1, "gg\0", NULL, NULL, device_load_sms_cart, NULL, NULL, NULL)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*		YEAR	NAME		PARENT		COMPATIBLE	MACHINE		INPUT	INIT	CONFIG		COMPANY			FULLNAME */
CONS(	1987,	sms,		0,			0,			sms,		sms,	0,		sms,		"Sega",			"Master System - (NTSC)" )
CONS(	1986,	smsu13,		sms,		0,			sms,		sms,	0,		smso,		"Sega",			"Master System - (NTSC) US/European BIOS v1.3" )
CONS(	1986,	smse13,		sms,		0,			smsm3,		sms,	0,		smso,		"Sega",			"Master System - (PAL) US/European BIOS v1.3" )
CONS(	1986,	smsu13h,	sms,		0,			sms,		sms,	0,		smso,		"Sega",			"Master System - (NTSC) Hacked US/European BIOS v1.3" )
CONS(	1986,	smse13h,	sms,		0,			smsm3,		sms,	0,		smso,		"Sega",			"Master System - (PAL) Hacked US/European BIOS v1.3" )
/* next two systems are disabled because of missing bios roms */
//CONS(	1986,	smsumd3d,	sms,		0,			sms,		sms,	0,		smso,		"Sega",			"Super Master System - (NTSC) US/European Missile Defense 3D BIOS" )
//CONS(	1986,	smsemd3d,	sms,		0,			smsm3,		sms,	0,		smso,		"Sega",			"Super Master System - (PAL) US/European Missile Defense 3D BIOS" )
CONS(	1990,	smsuam,		sms,		0,			sms,		sms,	0,		smso,		"Sega",			"Master System II - (NTSC) US/European BIOS with Alex Kidd in Miracle World" )
CONS(	1990,	smseam,		sms,		0,			smsm3,		sms,	0,		smso,		"Sega",			"Master System II - (PAL) US/European BIOS with Alex Kidd in Miracle World" )
CONS(	1990,	smsesh,		sms,		0,			smsm3,		sms,	0,		smso,		"Sega",			"Master System II - (PAL) European BIOS with Sonic The Hedgehog" )
CONS(	1990,	smsbsh,		sms,		0,			smsm3,		sms,	0,		smso,		"Tec Toy",		"Master System III Compact (Brazil) - (PAL) European BIOS with Sonic The Hedgehog" )
CONS(	1988,	smsuhs24,	sms,		0,			sms,		sms,	0,		smso,		"Sega",			"Master System Plus - (NTSC) US/European BIOS v2.4 with Hang On and Safari Hunt" )
CONS(	1988,	smsehs24,	sms,		0,			smsm3,		sms,	0,		smso,		"Sega",			"Master System Plus - (PAL) US/European BIOS v2.4 with Hang On and Safari Hunt" )
CONS(	1988,	smsuh34,	sms,		0,			sms,		sms,	0,		smso,		"Sega",			"Master System - (NTSC) US/European BIOS v3.4 with Hang On" )
CONS(	1988,	smseh34,	sms,		0,			smsm3,		sms,	0,		smso,		"Sega",			"Master System - (PAL) US/European BIOS v3.4 with Hang On" )
CONS(	1985,	smspal,		sms,		0,			smspal,		sms,	0,		sms,		"Sega",			"Master System - (PAL)" )
CONS(	1985,	smsj21,		sms,		0,			smsj21,		sms,	0,		sms,		"Sega",			"Master System - (PAL) Japanese SMS BIOS v2.1" )
CONS(	1984,	smsm3,		sms,		0,			smsm3,		sms,	0,		smso,		"Sega",			"Mark III - (PAL) Japanese SMS BIOS v2.1" )
CONS(	198?,	smsss,		sms,		0,			smsj21,		sms,	0,		smso,		"Samsung",		"Gamboy - (PAL) Japanese SMS BIOS v2.1" )
CONS(	1990,	gamegear,	0,			sms,		gamegear,	sms,	0,		gamegear,	"Sega",			"Game Gear - European/American" )
CONS(	1990,	gamegj,		gamegear,	0,			gamegear,	sms,	0,		gamegear,	"Sega",			"Game Gear - Japanese" )
CONS(	1990,	gamg,		gamegear,	0,			gamegear,	sms,	0,		gamegearo,	"Majesco",		"Game Gear - European/American Majesco Game Gear BIOS" )
CONS(	1990,	gamgj,		gamegear,	0,			gamegear,	sms,	0,		gamegearo,	"Majesco",		"Game Gear - Japanese Majesco Game Gear BIOS" )

