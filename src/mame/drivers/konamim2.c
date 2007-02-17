/*  Konami M2 Hardware

    Preliminary driver by Ville Linde

    Games on this hardware:
    =======================

    Game                 | Konami ID
    ---------------------------------
    Evil Night           | GN810
    Heat of Eleven '98   | GX703
    Tobe! Polystars      | GX623
    Battle Tryst         | G?636

*/

/*
Tobe! PolyStars
Konami, 1997

This game runs on 3DO-based Konami M2 hardware.

PCB Layout
----------

Top Board

[M]DFUP0882ZAM1
FZ-20B1AK 7BKSA03500 (sticker)
|---------------------------------------------------|
|            |--------------------|    |----------| |
|            |--------------------|    |----------| |
|    2902             |---|  |--------|             |
| AK4309 CY2292S|---| |*2 |  |  3DO   |  |-------|  |
|               |*1 | |---|  |        |  |IBM    |  |
|        18MHz  |---|        |        |  |POWERPC|  |
|                            |        |  |602    |  |
|                            |--------|  |-------|  |
|    D4516161  D4516161                             |
|                                 |---|  |-------|  |
|DSW                    |-------| |*3 |  |IBM    |  |
|                       |       | |---|  |POWERPC|  |
|    D4516161  D4516161 |  *4   |        |602    |  |
|                       |       |        |-------|  |
|                       |-------|                   |
|---------------------------------------------------|
Notes:
      AK4309  - Asahi Kasei Microsystems AK4309-VM Digital to Analog Converter (SOIC24)
      2902    - Japan Radio Co. JRC2902 Quad Operational Amplifier (SOIC14)
      CY2292S - Cypress CY2292S Three-PLL General-Purpose EPROM Programmable Clock Generator (SOIC16)
                XTALIN - 18.000MHz, XTALOUT - 18.000MHz, XBUF - 18.000MHz, CPUCLK - 25.2000MHz
                CLKA - , CLKB -  , CLKC - 16.9345MHz, CLKD -
      3DO     - 9701 B861131 VY21118- CDE2 3DO 02473-001-0F (QFP208)
      *1      - [M] JAPAN ASUKA 9651HX001 044 (QFP44)
      *2      - Motorola MC44200FT
      *3      - [M] BIG BODY 2 BU6244KS 704 157 (QFP56)
      *4      - Unknown BGA chip (Graphics Engine, with heatsink attached)
      DSW     - 2 position dip switch


Bottom Board

PWB403045B (C) 1997 KONAMI CO., LTD.
|----------------------------------------------------------|
|           CN16    |--------------------|    |----------| |
|LA4705             |--------------------|    |----------| |
|       NJM5532D                    9.83MHz                |
|                                   19.66MHz               |
|J                |--------|   93C46.7K                    |-|
|A                | 058232 |                623B01.8Q      | |
|M                |--------|   |------|                    | |
|M       |------|              |003461|                    | |
|A       |056879|              |      |                    | |CN15
|        |      |              |------|                    | |
| TEST   |------|                                          | |
|                                                          | |
|   DSW                                                    | |
|                                                          |-|
|                                                          |
|----------------------------------------------------------|
Notes:
      056879   - Konami custom IC, location 10E (QFP120)
      058232   - Konami custom ceramic flat pack IC, DAC?
      003461   - Konami custom IC, location 11K (QFP100)
      CN16     - 4 pin connector for CD-DA in from CDROM
      CN15     - Standard (PC-compatible) 40 pin IDE CDROM flat cable connector and 4 pin power plug connector,
                 connected to Panasonic CR-583 8-speed CDROM drive.
                 CDROM disc contains 1 data track & 24 audio tracks. Total Capacity: 581 MBytes
                 Konami part number: 003894
                 Software revision: 623JAA02
      LA4705   - LA4705 Power Amplifier
      623B01.8Q- 16MBit MASKROM. Location 8Q (DIP42)
      93C46.7K - 128bytes x8bit Serial EEPROM. Location 7K (DIP8)
      DSW      - 8 position dip switch

*/

#include "driver.h"
#include "cdrom.h"
#include "cpu/powerpc/ppc.h"

static UINT64 *main_ram;

static UINT32 vdl0_address;
static UINT32 vdl1_address;

static UINT32 irq_enable;
static UINT32 irq_active;

VIDEO_START( m2 )
{
	return 0;
}

VIDEO_UPDATE( m2 )
{
	int i, j;

	UINT32 fb_start = 0xffffffff;
	if (vdl0_address != 0)
	{
		fb_start = *(UINT32*)&main_ram[(vdl0_address - 0x40000000) / 8] - 0x40000000;
	}

	if (fb_start <= 0x800000)
	{
		UINT16 *frame = (UINT16*)&main_ram[fb_start/8];
		for (j=0; j < 384; j++)
		{
			UINT16 *fb = &frame[(j*512)];
			UINT16 *d = BITMAP_ADDR16(bitmap, j, 0);
			for (i=0; i < 512; i++)
			{
				d[i^3] = *fb++;
			}
		}
	}
	else
	{
		fillbitmap(bitmap, 0, cliprect);
	}
	return 0;
}

static READ64_HANDLER(irq_enable_r)
{
	UINT64 r = 0;

	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		r |= (UINT64)(irq_enable) << 32;
	}

	return r;
}

static WRITE64_HANDLER(irq_enable_w)
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		irq_enable |= (UINT32)(data >> 32);
	}
}

static READ64_HANDLER(irq_active_r)
{
	UINT64 r = 0;

	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		r |= (UINT64)(irq_active) << 32;
	}

	return r;
}



static READ64_HANDLER(unk1_r)
{
	return U64(0xffffffffffffffff);
	//return 0;
}

static READ64_HANDLER(unk2_r)
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		return (UINT64)0xa5 << 32;
	}
	return 0;
}

static UINT64 unk3 = -1;
static READ64_HANDLER(unk3_r)
{
	//return U64(0xffffffffffffffff);
	return unk3;
}

static UINT32 unk20004 = 0;
static READ64_HANDLER(unk4_r)
{
	UINT64 r = 0;
//  logerror("unk4_r: %08X, %08X%08X at %08X\n", offset, (UINT32)(mem_mask>>32), (UINT32)(mem_mask), activecpu_get_pc());

	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		// MCfg
		r |= (UINT64)((0 << 13) | (5 << 10)) << 32;
	}
	if (!(mem_mask & U64(0x00000000ffffffff)))
	{
		r |= unk20004 & ~0x800000;
	}
	return r;
}

static WRITE64_HANDLER(unk4_w)
{
//  logerror("unk4_w: %08X%08X, %08X, %08X%08X at %08X\n", (UINT32)(data >> 32), (UINT32)(data),
//      offset, (UINT32)(mem_mask>>32), (UINT32)(mem_mask), activecpu_get_pc());

	if (!(mem_mask & U64(0x00000000ffffffff)))
	{
		if (data & 0x800000)
		{
			mame_printf_debug("CPU%d: CPU1 IRQ at %08X\n", cpu_getactivecpu(), activecpu_get_pc());
			cpunum_set_input_line(1, INPUT_LINE_IRQ0, ASSERT_LINE);
		}

		unk20004 = (UINT32)(data);
		return;
	}
}

static int counter1 = 0;
static READ64_HANDLER(unk30000_r)
{
	counter1++;
	return (UINT64)(counter1 & 0x7f) << 32;
}

static READ64_HANDLER(unk30030_r)
{
	if (!(mem_mask & U64(0x00000000ffffffff)))
	{
		return 1;
	}
	return 0;
}

static WRITE64_HANDLER(video_w)
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		vdl0_address = (UINT32)(data >> 32);
	}
	if (!(mem_mask & U64(0x00000000ffffffff)))
	{
		vdl1_address = (UINT32)(data);
	}
}

static WRITE64_HANDLER(video_irq_ack_w)
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		if ((data >> 32) & 0x8000)
		{
			irq_active &= ~0x800000;
		}
	}
}



static READ64_HANDLER(unk4000280_r)
{
	// SysCfg

	UINT32 sys_config = 0x03600000;

	sys_config |= 0 << 0;			// Bit 0:       PAL/NTSC switch (default is selected by encoder)
	sys_config |= 0 << 2;			// Bit 2-3:     Video Encoder (0 = MEIENC, 1 = VP536, 2 = BT9103, 3 = DENC)
	sys_config |= 3 << 11;			// Bit 11-12:   Country
									//              0 = ???
									//              1 = UK
									//              2 = Japan
									//              3 = US
	sys_config |= 0xb << 15;		// Bit 15-18:   0x8 = AC-DevCard
									//              0xb = AC-CoreBoard
									//              0xc = DevCard (not allowed)
									//              0xe = Upgrade (not allowed)
									//              0xf = Multiplayer (not allowed)
	sys_config |= 3 << 29;			// Bit 29-30:   Audio chip (1 = CS4216, 3 = Asahi AK4309)

	return ((UINT64)(sys_config) << 32);

}

static WRITE64_HANDLER(unk4000010_w)
{
	if ((data & 0xff) == 0xd)
	{
		mame_printf_debug("\n");
	}
	else
	{
		mame_printf_debug("%c", (UINT8)(data & 0xff));
	}
}

static WRITE64_HANDLER(unk4000418_w)
{
}

static WRITE64_HANDLER(reset_w)
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		if (data & U64(0x100000000))
		{
			cpunum_set_input_line(0, INPUT_LINE_RESET, PULSE_LINE);
			unk3 = 0;
		}
	}
}


/*****************************************************************************/
/* CDE */

typedef struct
{
	UINT32 dst_addr;
	int length;
	UINT32 next_dst_addr;
	int next_length;
	int dma_done;
} CDE_DMA;

#define CDE_DRIVE_STATE_PAUSED			0x02
#define CDE_DRIVE_STATE_SEEK_DONE		0x03

static int cde_num_status_bytes = 0;
static UINT32 cde_status_bytes[16];
static int cde_status_byte_ptr = 0;

static UINT32 cde_command_bytes[16];
static int cde_command_byte_ptr = 0;

static int cde_response = 0;
static int cde_drive_state = 0;

static int cde_enable_qchannel_reports = 0;
static int cde_enable_seek_reports = 0;

static int cde_qchannel_offset = 0;

static cdrom_toc cde_toc;

static CDE_DMA cde_dma[2];


static void cde_init(void)
{
	cdrom_file *cd = cdrom_open(get_disk_handle(0));
	const cdrom_toc *toc = cdrom_get_toc(cd);

	memcpy(&cde_toc, toc, sizeof(cdrom_toc));

	/*
    printf("%d tracks\n", toc->numtrks);
    for (i=0; i < toc->numtrks; i++)
    {
        const cdrom_track_info *track = &toc->tracks[i];
        printf("Track %d: type %d, subtype %d, datasize %d, subsize %d, frames %d, extraframes %d, physframeofs %d\n",
            i, track->trktype, track->subtype, track->datasize, track->subsize,track->frames, track->extraframes, track->physframeofs);
    }
    */

	cdrom_close(cd);

	cde_drive_state = CDE_DRIVE_STATE_PAUSED;

	cde_num_status_bytes = 0;
	cde_status_byte_ptr = 0;
	cde_command_byte_ptr = 0;

	cde_response = 0;

	cde_enable_qchannel_reports = 0;
	cde_enable_seek_reports = 0;

	cde_qchannel_offset = 0;
}

static void cde_handle_command(void)
{
	switch (cde_command_bytes[0])
	{
		case 0x04:		// Set Speed
		{
			cde_num_status_bytes = 1;

			cde_status_bytes[0] = 0x04;
			cde_status_byte_ptr = 0;

			mame_printf_debug("CDE: SET SPEED %02X, %02X\n", cde_command_bytes[1], cde_command_bytes[2]);
			break;
		}
		case 0x06:		// Audio Format / Data Format
		{
			cde_num_status_bytes = 1;

			cde_status_bytes[0] = 0x06;
			cde_status_byte_ptr = 0;

			if (cde_command_bytes[1] == 0x00)		// Audio Format
			{
				mame_printf_debug("CDE: AUDIO FORMAT\n");
			}
			else if (cde_command_bytes[1] == 0x78)	// Data Format
			{
				mame_printf_debug("CDE: DATA FORMAT\n");
			}
			else
			{
				fatalerror("CDE: unknown command %02X, %02X\n", cde_command_bytes[0], cde_command_bytes[1]);
			}
			break;
		}
		case 0x08:		// Pause / Eject / Play
		{
			cde_num_status_bytes = 1;

			cde_status_bytes[0] = 0x08;
			cde_status_byte_ptr = 0;

			if (cde_command_bytes[1] == 0x00)		// Eject
			{
				mame_printf_debug("CDE: EJECT command\n");
			}
			else if (cde_command_bytes[1] == 0x02)	// Pause
			{
				mame_printf_debug("CDE: PAUSE command\n");
				cde_drive_state = CDE_DRIVE_STATE_PAUSED;
			}
			else if (cde_command_bytes[1] == 0x03)	// Play
			{
				mame_printf_debug("CDE: PLAY command\n");
			}
			else
			{
				fatalerror("CDE: unknown command %02X, %02X\n", cde_command_bytes[0], cde_command_bytes[1]);
			}
			break;
		}
		case 0x09:		// Seek
		{
			cde_num_status_bytes = 1;

			cde_status_bytes[0] = 0x1b;
			cde_status_byte_ptr = 0;

			cde_drive_state = CDE_DRIVE_STATE_SEEK_DONE;

			mame_printf_debug("CDE: SEEK %08X\n", (cde_command_bytes[1] << 16) | (cde_command_bytes[2] << 8) | (cde_command_bytes[3]));
			break;
		}
		case 0x0b:		// Get Drive State
		{
			cde_num_status_bytes = 0x3;

			cde_status_bytes[0] = 0x0b;
			cde_status_bytes[1] = 0x1b;
			cde_status_bytes[2] = cde_drive_state;
			cde_status_byte_ptr = 0;

			if (cde_command_bytes[1] & 0x02)
			{
				cde_enable_seek_reports = 1;
			}
			else
			{
				cde_enable_seek_reports = 0;
			}

			mame_printf_debug("CDE: GET DRIVE STATE %02X\n", cde_command_bytes[1]);
			break;
		}
		case 0x0c:		// ?
		{
			cde_num_status_bytes = 1;

			cde_status_bytes[0] = 0x0c;
			cde_status_byte_ptr = 0;

			if (cde_command_bytes[1] == 0x02)
			{
				cde_enable_qchannel_reports = 1;
				cde_drive_state = CDE_DRIVE_STATE_PAUSED;
			}
			else if (cde_command_bytes[0] == 0x00)
			{
				cde_enable_qchannel_reports = 0;
			}

			mame_printf_debug("CDE: UNKNOWN CMD 0x0c %02X\n", cde_command_bytes[1]);
			break;
		}
		case 0x0d:		// Get Switch State
		{
			cde_num_status_bytes = 0x4;

			cde_status_bytes[0] = 0x0d;
			cde_status_bytes[1] = 0x1d;
			cde_status_bytes[2] = 0x02;
			cde_status_byte_ptr = 0;

			mame_printf_debug("CDE: GET SWITCH STATE %02X\n", cde_command_bytes[1]);
			break;
		}
		case 0x21:		// Mech type
		{
			cde_num_status_bytes = 0x8;

			cde_status_bytes[0] = 0x21;
			cde_status_bytes[1] = 0xff;
			cde_status_bytes[2] = 0x08;		// Max Speed
			cde_status_bytes[3] = 0xff;
			cde_status_bytes[4] = 0xff;
			cde_status_bytes[5] = 0xff;
			cde_status_bytes[6] = 0xff;
			cde_status_bytes[7] = 0xff;

			cde_status_byte_ptr = 0;

			mame_printf_debug("CDE: MECH TYPE %02X, %02X, %02X\n", cde_command_bytes[1], cde_command_bytes[2], cde_command_bytes[3]);
			break;
		}
		case 0x83:		// Read ID
		{
			cde_num_status_bytes = 0xc;

			cde_status_bytes[0] = 0x03;
			cde_status_bytes[1] = 0xff;
			cde_status_bytes[2] = 0xff;
			cde_status_bytes[3] = 0xff;
			cde_status_bytes[4] = 0xff;
			cde_status_bytes[5] = 0xff;
			cde_status_bytes[6] = 0xff;
			cde_status_bytes[7] = 0xff;
			cde_status_bytes[8] = 0xff;
			cde_status_bytes[9] = 0xff;
			cde_status_bytes[10] = 0xff;
			cde_status_bytes[11] = 0xff;

			cde_status_byte_ptr = 0;

			mame_printf_debug("CDE: READ ID\n");
			break;
		}
		default:
		{
			fatalerror("CDE: unknown command %08X\n", cde_command_bytes[0]);
			break;
		}
	}
}

static void cde_handle_reports(void)
{
	switch (cde_command_bytes[0])
	{
		case 0x09:
		{
			if (cde_enable_seek_reports)
			{
				cde_num_status_bytes = 0x2;
				cde_status_bytes[0] = 0x02;

				cde_status_byte_ptr = 0;

				cde_command_bytes[0] = 0x0c;

				mame_printf_debug("CDE: SEEK REPORT\n");
			}
			break;
		}

		case 0x0b:
		{
			if (cde_enable_qchannel_reports)
			{
				int track, num_tracks;

				num_tracks = cde_toc.numtrks;
				track = cde_qchannel_offset % (num_tracks+3);

				cde_num_status_bytes = 0xb;
				cde_status_bytes[0] = 0x1c;

				/*
                cde_status_bytes[1] = 0x0;      // q-Mode
                cde_status_bytes[2] = 0x0;      // TNO
                cde_status_bytes[3] = 0x0;      // Index / Pointer
                cde_status_bytes[4] = 0x0;      // Min
                cde_status_bytes[5] = 0x0;      // Sec
                cde_status_bytes[6] = 0x0;      // Frac
                cde_status_bytes[7] = 0x0;      // Zero
                cde_status_bytes[8] = 0x0;      // A-Min
                cde_status_bytes[9] = 0x0;      // A-Sec
                cde_status_bytes[10] = 0x0;     // A-Frac
                */

				if (track < num_tracks)
				{
					int time = lba_to_msf(cde_toc.tracks[track].physframeofs);

					cde_status_bytes[1] = 0x41;					// q-Mode
					cde_status_bytes[2] = 0x0;					// TNO (Lead-in track)
					cde_status_bytes[3] = track+1;				// Pointer
					cde_status_bytes[4] = 0x0;					// Min
					cde_status_bytes[5] = 0x0;					// Sec
					cde_status_bytes[6] = 0x0;					// Frac
					cde_status_bytes[7] = 0x0;					// Zero
					cde_status_bytes[8] = (time >> 16) & 0xff;	// P-Min
					cde_status_bytes[9] = (time >>  8) & 0xff;	// P-Sec
					cde_status_bytes[10] = time & 0xff;			// P-Frac
				}
				else
				{
					if (track == num_tracks+0)
					{
						cde_status_bytes[1] = 0x41;					// q-Mode / Control
						cde_status_bytes[2] = 0x0;					// TNO (Lead-in track)
						cde_status_bytes[3] = 0xa0;					// Pointer
						cde_status_bytes[4] = 0x0;					// Min
						cde_status_bytes[5] = 0x0;					// Sec
						cde_status_bytes[6] = 0x0;					// Frac
						cde_status_bytes[7] = 0x0;					// Zero
						cde_status_bytes[8] = 1;					// P-Min
						cde_status_bytes[9] = 0x0;					// P-Sec
						cde_status_bytes[10] = 0x0;					// P-Frac
					}
					else if (track == num_tracks+1)
					{
						cde_status_bytes[1] = 0x41;					// q-Mode / Control
						cde_status_bytes[2] = 0x0;					// TNO (Lead-in track)
						cde_status_bytes[3] = 0xa1;					// Pointer
						cde_status_bytes[4] = 0x0;					// Min
						cde_status_bytes[5] = 0x0;					// Sec
						cde_status_bytes[6] = 0x0;					// Frac
						cde_status_bytes[7] = 0x0;					// Zero
						cde_status_bytes[8] = num_tracks;			// P-Min
						cde_status_bytes[9] = 0x0;					// P-Sec
						cde_status_bytes[10] = 0x0;					// P-Frac
					}
					else
					{
						int leadout_lba = cde_toc.tracks[num_tracks-1].physframeofs + cde_toc.tracks[num_tracks-1].frames;
						int leadout_time = lba_to_msf(leadout_lba);

						cde_status_bytes[1] = 0x41;					// q-Mode / Control
						cde_status_bytes[2] = 0x0;					// TNO (Lead-in track)
						cde_status_bytes[3] = 0xa2;					// Pointer
						cde_status_bytes[4] = 0x0;					// Min
						cde_status_bytes[5] = 0x0;					// Sec
						cde_status_bytes[6] = 0x0;					// Frac
						cde_status_bytes[7] = 0x0;					// Zero
						cde_status_bytes[8] = (leadout_time >> 16) & 0xff;	// P-Min
						cde_status_bytes[9] = (leadout_time >>  8) & 0xff;	// P-Sec
						cde_status_bytes[10] = leadout_time & 0xff;			// P-Frac
					}
				}

				cde_qchannel_offset++;

				cde_status_byte_ptr = 0;
				cde_command_bytes[0] = 0x0c;

				mame_printf_debug("CDE: QCHANNEL REPORT\n");
				break;
			}
		}
	}
}

static void cde_dma_transfer(int channel, int next)
{
	UINT32 address;
	int length;
	int i;

	if (next)
	{
		address = cde_dma[channel].next_dst_addr;
		length = cde_dma[channel].next_length;
	}
	else
	{
		address = cde_dma[channel].dst_addr;
		length = cde_dma[channel].length;
	}

	for (i=0; i < cde_dma[channel].next_length; i++)
	{
		program_write_byte_64be(address, 0xff);		// TODO: do the real transfer...
		address++;
	}
}

static READ64_HANDLER(cde_r)
{
	UINT32 r = 0;
	int reg = offset * 2;

	if (!(mem_mask & U64(0x00000000ffffffff)))
		reg++;

	switch (reg)
	{
		case 0x000/4:
		{
			r = (0x01) << 16;	// Device identifier, 1 = CDE
			break;
		}
		case 0x018/4:
		{
			r = 0x100038;

			r |= cde_dma[0].dma_done ? 0x400 : 0;
			r |= cde_dma[1].dma_done ? 0x800 : 0;
			break;
		}
		case 0x02c/4:
		{
			r = cde_status_bytes[cde_status_byte_ptr++];

			if (cde_status_byte_ptr <= cde_num_status_bytes)
			{
				r |= 0x100;
			}
			else
			{
				//if (cde_enable_reports &&
				//  !cde_response &&
				//  cde_command_bytes[0] != ((cde_report_type >> 8) & 0xff))

				if (!cde_response)
				{
					cde_handle_reports();

			//      cde_command_byte_ptr = 0;
			//      cde_command_bytes[cde_command_byte_ptr++] = 0x1c;

			//      cde_response = 1;
				}
			}

	//      printf("status byte %d\n", cde_status_byte_ptr);
			break;
		}

		case 0x2a0/4:
		{
			r = 0x20;
			break;
		}

		default:
		{
	//      mame_printf_debug("cde_r: %08X at %08X\n", reg*4, activecpu_get_pc());
			break;
		}
	}

	if (reg & 1)
	{
		return (UINT64)(r);
	}
	else
	{
		return (UINT64)(r) << 32;
	}
}

static WRITE64_HANDLER(cde_w)
{
	int reg = offset * 2;
	UINT32 d;

	if (!(mem_mask & U64(0x00000000ffffffff)))
	{
		reg++;
		d = (UINT32)(data);
	}
	else
	{
		d = (UINT32)(data >> 32);
	}

	switch (reg)
	{
		case 0x028/4:		// Command write
		{
			//printf("cde_w: %08X, %08X at %08X\n", d, reg*4, activecpu_get_pc());

			if (d == 0x0180)
			{
				if (cde_response)
				{
					cde_handle_command();

					cde_response = 0;
				}

				cde_command_byte_ptr = 0;
			}
			else if (cde_command_byte_ptr == 0)
			{
				cde_num_status_bytes = 1;

				cde_status_bytes[0] = d & 0xff;
				cde_status_byte_ptr = 0;

				cde_response = 1;
			}

			if (d != 0x180)
			{
				cde_command_bytes[cde_command_byte_ptr++] = d;
			}

			break;
		}

		case 0x300/4:		// DMA Channel 0 enable
		{
			mame_printf_debug("CDE: DMA0 enable %08X\n", d);

			if (d & 0x20)
			{
				cde_dma[0].dma_done = 1;

				cde_dma_transfer(0, 0);
			}
			if (d & 0x40)
			{
				cde_dma[0].dma_done = 1;

				cde_dma_transfer(0, 1);
			}
			break;
		}
		case 0x308/4:		// DMA Channel 0 destination address
		{
			mame_printf_debug("CDE: DMA0 dst addr %08X\n", d);

			cde_dma[0].dst_addr = d;
			break;
		}
		case 0x30c/4:		// DMA Channel 0 length?
		{
			mame_printf_debug("CDE: DMA0 length %08X\n", d);

			cde_dma[0].length = d;
			break;
		}
		case 0x318/4:		// DMA Channel 0 next destination address
		{
			mame_printf_debug("CDE: DMA0 next dst addr %08X\n", d);

			cde_dma[0].next_dst_addr = d;
			break;
		}
		case 0x31c/4:		// DMA Channel 0 next length?
		{
			mame_printf_debug("CDE: DMA0 next length %08X\n", d);

			cde_dma[0].next_length = d;
			break;
		}

		case 0x320/4:		// DMA Channel 1 enable
		{
			mame_printf_debug("CDE: DMA1 enable %08X\n", d);
			break;
		}
		case 0x328/4:		// DMA Channel 1 destination address
		{
			mame_printf_debug("CDE: DMA1 dst addr %08X\n", d);

			cde_dma[1].dst_addr = d;
			break;
		}
		case 0x32c/4:		// DMA Channel 1 length?
		{
			mame_printf_debug("CDE: DMA1 length %08X\n", d);

			cde_dma[1].length = d;
			break;
		}
		case 0x338/4:		// DMA Channel 1 next destination address
		{
			mame_printf_debug("CDE: DMA1 next dst addr %08X\n", d);

			cde_dma[1].next_dst_addr = d;
			break;
		}
		case 0x33c/4:		// DMA Channel 1 next length?
		{
			mame_printf_debug("CDE: DMA1 next length %08X\n", d);

			cde_dma[1].next_length = d;
			break;
		}

		case 0x418/4:		// ???
		{
			if (d & 0x80000000)
			{
				irq_active &= ~0x8;
			}
			if (d & 0x60000000)
			{
				cde_dma[0].dma_done = 0;
				cde_dma[1].dma_done = 0;
			}
			break;
		}

		default:
		{
	//      mame_printf_debug("cde_w: %08X, %08X at %08X\n", d, reg*4, activecpu_get_pc());
			break;
		}
	}
}

static READ64_HANDLER(device2_r)
{
	UINT32 r = 0;
	int reg = offset * 2;

	if (!(mem_mask & U64(0x00000000ffffffff)))
		reg++;

	switch (reg)
	{
		case 0x000/4:
		{
			r = (0x02) << 16;	// Device identifier
			break;
		}
		default:
		{
			break;
		}
	}

	if (reg & 1)
	{
		return (UINT64)(r);
	}
	else
	{
		return (UINT64)(r) << 32;
	}
}

static READ64_HANDLER(cpu_r)
{
	UINT64 r = 0;

	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		r = (UINT64)(cpu_getactivecpu() ? 0x80000000 : 0);
		//r |= 0x40000000;  // sets Video-LowRes !?
		return r << 32;
	}

	return 0;
}

static ADDRESS_MAP_START( m2_main, ADDRESS_SPACE_PROGRAM, 64 )
	AM_RANGE(0x00010040, 0x00010047) AM_READWRITE(irq_enable_r, irq_enable_w)
	AM_RANGE(0x00010050, 0x00010057) AM_READ(irq_active_r)
	AM_RANGE(0x00020000, 0x00020007) AM_READWRITE(unk4_r, unk4_w)
	AM_RANGE(0x00030000, 0x00030007) AM_READ(unk30000_r)
	AM_RANGE(0x00030010, 0x00030017) AM_WRITE(video_w)
	AM_RANGE(0x00030030, 0x00030037) AM_READ(unk30030_r)
	AM_RANGE(0x00030400, 0x00030407) AM_WRITE(video_irq_ack_w)
	AM_RANGE(0x01000000, 0x01000fff) AM_READWRITE(cde_r, cde_w)
	AM_RANGE(0x02000000, 0x02000fff) AM_READ(device2_r)
	AM_RANGE(0x04000010, 0x04000017) AM_WRITE(unk4000010_w)
	AM_RANGE(0x04000018, 0x0400001f) AM_READ(unk1_r)
	AM_RANGE(0x04000020, 0x04000027) AM_WRITE(reset_w)
	AM_RANGE(0x04000418, 0x0400041f) AM_WRITE(unk4000418_w)
	AM_RANGE(0x04000208, 0x0400020f) AM_READ(unk3_r)
	AM_RANGE(0x04000280, 0x04000287) AM_READ(unk4000280_r)
	AM_RANGE(0x10000000, 0x10000007) AM_READ(cpu_r)
	AM_RANGE(0x10000008, 0x10001007) AM_NOP		// ???
	AM_RANGE(0x20000000, 0x201fffff) AM_ROM AM_SHARE(2)
	AM_RANGE(0x40000000, 0x407fffff) AM_RAM AM_SHARE(3) AM_BASE(&main_ram)
	AM_RANGE(0xfff00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0) AM_SHARE(2)
ADDRESS_MAP_END

INPUT_PORTS_START( m2 )
INPUT_PORTS_END


static ppc_config ppc602_config =
{
	PPC_MODEL_602,
	0x20,				/* Multiplier 2, Bus = 33MHz, Core = 66MHz */
	BUS_FREQUENCY_33MHZ
};

static INTERRUPT_GEN(m2)
{
	if (irq_enable & 0x800000)
	{
		irq_active |= 0x800000;
	}

	/*if (irq_enable & 0x8)
    {
        irq_active |= 0x8;
    }*/

	cpunum_set_input_line(0, INPUT_LINE_IRQ0, ASSERT_LINE);
}

static MACHINE_DRIVER_START( m2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(PPC602, 33000000)	/* actually PPC602, 66MHz */
	MDRV_CPU_CONFIG(ppc602_config)
	MDRV_CPU_PROGRAM_MAP(m2_main, 0)
	MDRV_CPU_VBLANK_INT(m2, 1)

	MDRV_CPU_ADD(PPC602, 33000000)	/* actually PPC602, 66MHz */
	MDRV_CPU_CONFIG(ppc602_config)
	MDRV_CPU_PROGRAM_MAP(m2_main, 0)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(TIME_IN_USEC(0))

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB15)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 511, 0, 383)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(m2)
	MDRV_VIDEO_UPDATE(m2)

MACHINE_DRIVER_END


#define ROM_REGION64_BE(length,type,flags)	      ROM_REGION(length, type, (flags) | ROMREGION_64BIT | ROMREGION_BE)

ROM_START(polystar)
	ROM_REGION64_BE(0x200000, REGION_USER1, 0)	/* boot rom */
	ROM_LOAD16_WORD("623b01.8q", 0x000000, 0x200000, CRC(bd879f93) SHA1(e2d63bfbd2b15260a2664082652442eadea3eab6))

	ROM_REGION( 0x80, REGION_USER2, 0 ) /* serial eeprom */
	ROM_LOAD( "93c46.7k",  0x000000, 0x000080, CRC(66d02984) SHA1(d07c57d198c611b6ff67a783c20a3d038ba34cd1) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "623jaa02", 0, MD5(6071c1b70c190fa7c50676eb5308e024) SHA1(a7dc6086d0244b5472f61b41992623c7a9dc2e9c))
ROM_END

ROM_START(btltryst)
	ROM_REGION64_BE(0x200000, REGION_USER1, 0)	/* boot rom */
	ROM_LOAD16_WORD("636a01.8q", 0x000000, 0x200000, CRC(7b1dc738) SHA1(32ae8e7ddd38fcc70b4410275a2cc5e9a0d7d33b))

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "btltryst", 0, MD5(4286d25a896d1450705f742cccd26ef2) SHA1(becc606b8480f6a09365b611565d83cfdc82b0b3))
ROM_END

ROM_START(heatof11)
	ROM_REGION64_BE(0x200000, REGION_USER1, 0)	/* boot rom */
	ROM_LOAD16_WORD("636a01.8q", 0x000000, 0x200000, CRC(7b1dc738) SHA1(32ae8e7ddd38fcc70b4410275a2cc5e9a0d7d33b))

	ROM_REGION( 0x2000, REGION_USER2, 0 ) /* timekeeper eeprom */
	ROM_LOAD( "dallas.5e",  0x000000, 0x002000, CRC(8611ff09) SHA1(6410236947d99c552c4a1f7dd5fd8c7a5ae4cba1) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "heatof11", 0, MD5(1eef9f191439f13d1a867629226b0230) SHA1(87599d03aecadf5d119d05173bdef2940bcce515))
ROM_END

ROM_START(evilngt)
	ROM_REGION64_BE(0x200000, REGION_USER1, 0)	/* boot rom */
	ROM_LOAD16_WORD("636a01.8q", 0x000000, 0x200000, CRC(7b1dc738) SHA1(32ae8e7ddd38fcc70b4410275a2cc5e9a0d7d33b))

	ROM_REGION( 0x1000, REGION_USER2, 0 ) /* timekeeper eeprom */
	ROM_LOAD( "m48t58y.u1",  0x000000, 0x001000, CRC(169bb8f4) SHA1(55c0bafab5d309fe69156489186e232aa87ca0dd) )
ROM_END

ROM_START(totlvice)
	ROM_REGION64_BE(0x200000, REGION_USER1, 0)	/* boot rom */
	ROM_LOAD16_WORD("636a01.8q", 0x000000, 0x200000, CRC(7b1dc738) SHA1(32ae8e7ddd38fcc70b4410275a2cc5e9a0d7d33b))

	ROM_REGION( 0x100000, REGION_USER2, 0 ) /* sound rom on sub board */
	ROM_LOAD( "639jaa02.bin",  0x000000, 0x100000, CRC(c6163818) SHA1(b6f8f2d808b98610becc0a5be5443ece3908df0b) )
ROM_END

static DRIVER_INIT( m2 )
{
	cde_init();
}


static DRIVER_INIT( polystar )
{
	init_m2(machine);
}

static DRIVER_INIT( btltryst )
{
	init_m2(machine);
}

GAME( 1997, polystar,	0,	m2, m2, polystar,		ROT0,	"Konami",	"Tobe! Polystars (ver JAA)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1997, totlvice,	0,	m2, m2, m2,				ROT0,	"Konami",	"Total Vice (ver JAA)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1998, btltryst,	0,	m2,	m2,	btltryst,		ROT0,	"Konami",	"Battle Tryst (ver JAC)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1998, heatof11,	0,	m2,	m2,	m2,				ROT0,	"Konami",	"Heat of Eleven '98 (ver EAA)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1998, evilngt,	0,	m2,	m2,	m2,				ROT0,	"Konami",	"Evil Night (ver EAA)", GAME_NOT_WORKING | GAME_NO_SOUND )
