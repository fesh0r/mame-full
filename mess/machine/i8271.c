/* Intel 8271 Floppy Disc Controller */
/* used in BBC Micro B */
/* Jun 2000. Kev Thacker */


/* This works enough for the BBC Micro B driver to boot. */

/* TODO: */
/* - get disc image and get it to read data */
/* - Everything! */

#include "i8271.h"

static void i8271_command_execute(void);
static void i8271_command_continue(void);
static void i8271_data_request(void);

static char temp_buffer[16384];

// uncomment to give verbose debug information
#define VERBOSE

static I8271 i8271;

#ifdef VERBOSE
#define FDC_LOG(x) logerror("I8271: %s\r\n",x)
#define FDC_LOG_COMMAND(x) logerror("I8271: COMMAND %s\r\n",x)
#endif


static int floppy_drive_status[4] = {
	0,0};

void	i8271_init(i8271_interface *iface)
{
	memcpy(&i8271.fdc_interface, iface, sizeof(i8271_interface));
}


void	i8271_reset()
{
	i8271.StatusRegister = I8271_STATUS_INT_REQUEST | I8271_STATUS_NON_DMA_REQUEST;
	i8271.Mode = 0x0c0; /* bits 0, 1 are initialized to zero */
}

static void i8271_command_complete(void)
{
	/* clear command busy */
	i8271.StatusRegister &= ~I8271_STATUS_COMMAND_BUSY;
}

static void i8271_set_irq_state(int state)
{
	i8271.StatusRegister &= ~I8271_STATUS_INT_REQUEST;
	if (state)
	{
		i8271.StatusRegister |= I8271_STATUS_INT_REQUEST;
	}

	if (i8271.fdc_interface.interrupt)
	{
		i8271.fdc_interface.interrupt((i8271.StatusRegister & I8271_STATUS_INT_REQUEST));
	}
}

static void i8271_set_dma_drq(void)
{
	if (i8271.fdc_interface.dma_request)
	{
		i8271.fdc_interface.dma_request((i8271.flags & I8271_FLAGS_DATA_REQUEST), (i8271.flags & I8271_FLAGS_DATA_DIRECTION));
	}
}

static void i8271_load_bad_tracks(int surface)
{
	i8271.BadTracks[(surface<<1) + 0] = i8271.CommandParameters[1];
	i8271.BadTracks[(surface<<1) + 1] = i8271.CommandParameters[2];
	i8271.CurrentTrack[surface] = i8271.CommandParameters[3];
}

static void i8271_write_bad_track(int surface, int track, int data)
{
	i8271.BadTracks[(surface<<1) + (track-1)] = data;
}

static void i8271_write_current_track(int surface, int track)
{
	i8271.CurrentTrack[surface] = track;
}

static int i8271_read_current_track(int surface)
{
	return i8271.CurrentTrack[surface];
}

static int i8271_read_bad_track(int surface, int track)
{
	return i8271.BadTracks[(surface<<1) + (track-1)];
}

static void i8271_get_drive(void)
{
	i8271.drive = (i8271.CommandRegister>>6) & 0x03;
}

static void i8271_setup_result(int data)
{
	/* set byte in result register */
	i8271.ResultRegister = data;
	/* specify result register is full in status */
	i8271.StatusRegister |= I8271_STATUS_RESULT_FULL;
}

static void i8271_check_all_parameters_written(void)
{
	if (i8271.ParameterCount == i8271.ParameterCountWritten)
	{
		i8271.StatusRegister &= ~I8271_STATUS_COMMAND_FULL;

		i8271_command_execute();
	}
}

static void i8271_update_state(void)
{
	switch (i8271.state)
	{

		/* fdc reading data and passing it to cpu which must read it */
		case I8271_STATE_EXECUTION_READ:
		{
			/* setup result register with byte */
			i8271.data = i8271.pExecutionPhaseData[i8271.ExecutionPhaseCount];
			/* update counters */
			i8271.ExecutionPhaseCount++;
			i8271.ExecutionPhaseTransferCount--;

			/* completed? */
			if (i8271.ExecutionPhaseTransferCount==0)
			{
				/* yes */

				/* continue command */
				i8271_command_continue();
			}
			else
			{
				/* no */

				/* issue data request */
				i8271_data_request();
			}
		}
		break;
	}
}

static void i8271_initialise_execution_phase_read(char *buffer)
{
	i8271.ExecutionPhaseCount = 0;
	i8271.ExecutionPhaseTransferCount = 1<<(i8271.ID_N+7);
	i8271.pExecutionPhaseData = buffer;
	i8271.state = I8271_STATE_EXECUTION_READ;
}

/* for data transfers */
static void i8271_data_request()
{
	i8271.flags |= I8271_FLAGS_DATA_REQUEST;

	if ((i8271.Mode & 0x01)!=0)
	{
		/* non-dma */
		i8271.StatusRegister |= I8271_STATUS_NON_DMA_REQUEST;
		i8271.StatusRegister |= I8271_STATUS_INT_REQUEST;
		/* set int */
		i8271_set_irq_state(1);
	}
	else
	{
		/* dma */
		i8271.StatusRegister &= ~I8271_STATUS_NON_DMA_REQUEST;

		i8271_set_dma_drq();
	}
}


/* for data transfers */
static void i8271_clear_data_request(void)
{
	i8271_set_irq_state(0);

	i8271.flags &= ~I8271_FLAGS_DATA_REQUEST;

	i8271_set_dma_drq();

	if ((i8271.Mode & 0x01)!=0)
	{
		/* non-dma */
		i8271.StatusRegister &= ~I8271_STATUS_NON_DMA_REQUEST;
		i8271.StatusRegister &= ~I8271_STATUS_INT_REQUEST;
		/* set int */
		i8271_set_irq_state(1);
	}
	else
	{
		/* dma */
		i8271.StatusRegister &= ~I8271_STATUS_NON_DMA_REQUEST;

		i8271_set_dma_drq();
	}
}
static void i8271_command_continue()
{
	switch (i8271.Command)
	{
		case I8271_COMMAND_READ_DATA_MULTI_RECORD:
		case I8271_COMMAND_READ_DATA_SINGLE_RECORD:
		{
			/* completed all sectors? */
			i8271.SectorCount--;

			/* end command? */
			if (i8271.SectorCount==0)
			{




			}
		}
		break;
	}
}

static void i8271_command_execute()
{
	switch (i8271.Command)
	{
		case I8271_COMMAND_SPECIFY:
		{
			switch (i8271.CommandParameters[0])
			{
				case 0x0d:
				{
#ifdef VERBOSE
					logerror("Initialization\r\n");
#endif
					i8271.StepRate = i8271.CommandParameters[1];
					i8271.HeadSettlingTime = i8271.CommandParameters[2];
					i8271.IndexCountBeforeHeadUnload = (i8271.CommandParameters[3]>>4) & 0x0f;
					i8271.HeadLoadTime = (i8271.CommandParameters[3] & 0x0f);
				}
				break;

				case 0x010:
				{
#ifdef VERBOSE
					logerror("Load bad Tracks Surface 0\r\n");
#endif
					i8271_load_bad_tracks(0);

				}
				break;

				case 0x018:
				{
#ifdef VERBOSE
					logerror("Load bad Tracks Surface 1\r\n");
#endif
					i8271_load_bad_tracks(1);

				}
				break;
			}

			/* clear command busy */
			i8271_command_complete();
		}
		break;

		case I8271_COMMAND_READ_SPECIAL_REGISTER:
		{
			/* unknown - what is read when a special register that isn't allowed is specified? */
			int data = 0x0ff;

			switch (i8271.CommandParameters[0])
			{
				case I8271_SPECIAL_REGISTER_MODE_REGISTER:
				{
					data = i8271.Mode;
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_0_CURRENT_TRACK:
				{
					data = i8271_read_current_track(0);

				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_1_CURRENT_TRACK:
				{
					data = i8271_read_current_track(1);
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_0_BAD_TRACK_1:
				{
					data = i8271_read_bad_track(0,1);
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_0_BAD_TRACK_2:
				{
					data = i8271_read_bad_track(0,2);
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_1_BAD_TRACK_1:
				{
					data = i8271_read_bad_track(1,1);
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_1_BAD_TRACK_2:
				{
					data = i8271_read_bad_track(1,2);
				}
				break;

				case I8271_SPECIAL_REGISTER_DRIVE_CONTROL_OUTPUT_PORT:
				{

					/* assumption: select bits reflect the select bits from the previous
					command. i.e. read drive status */
					data = (i8271.drive_control_output & ~0x0c0)
						| ((i8271.drive)<<6);
				}
				break;

				case I8271_SPECIAL_REGISTER_DRIVE_CONTROL_INPUT_PORT:
				{
					/* bit 7: not used */
					/* bit 6: ready 1 */
					/* bit 5: write fault */
					/* bit 4: index */
					/* bit 3: wr prot */
					/* bit 2: rdy 0 */
					/* bit 1: track 0 */
					/* bit 0: cnt/opi */

					i8271.drive_control_input = (1<<6) | (1<<2);
					/* need to setup this register based on drive selected */
					data = i8271.drive_control_input;
				}
				break;

			}

			/* write doesn't supply a result */
			i8271_command_complete();

			i8271_setup_result(data);

		}
		break;


		case I8271_COMMAND_WRITE_SPECIAL_REGISTER:
		{
			switch (i8271.CommandParameters[0])
			{
				case I8271_SPECIAL_REGISTER_MODE_REGISTER:
				{
					/* TODO: Check bits 6-7 and 5-2 are valid */
					i8271.Mode = i8271.CommandParameters[1];

#ifdef VERBOSE
					if (i8271.Mode & 0x01)
					{
						logerror("Mode: Non-DMA\r\n");
					}
					else
					{
						logerror("Mode: DMA\r\n");
					}

					if (i8271.Mode & 0x02)
					{
						logerror("Single actuator\r\n");
					}
					else
					{
						logerror("Double actuator\r\n");
					}
#endif
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_0_CURRENT_TRACK:
				{
#ifdef VERBOSE
					logerror("Surface 0 Current Track\r\n");
#endif
					i8271_write_current_track(0, i8271.CommandParameters[1]);
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_1_CURRENT_TRACK:
				{
#ifdef VERBOSE
					logerror("Surface 1 Current Track\r\n");
#endif
					i8271_write_current_track(1, i8271.CommandParameters[1]);
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_0_BAD_TRACK_1:
				{
#ifdef VERBOSE
					logerror("Surface 0 Bad Track 1\r\n");
#endif
					i8271_write_bad_track(0, 1, i8271.CommandParameters[1]);
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_0_BAD_TRACK_2:
				{
#ifdef VERBOSE
					logerror("Surface 0 Bad Track 2\r\n");
#endif
				i8271_write_bad_track(0, 2,i8271.CommandParameters[1]);
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_1_BAD_TRACK_1:
				{
#ifdef VERBOSE
					logerror("Surface 1 Bad Track 1\r\n");
#endif


					i8271_write_bad_track(1, 1, i8271.CommandParameters[1]);
				}
				break;

				case I8271_SPECIAL_REGISTER_SURFACE_1_BAD_TRACK_2:
				{
#ifdef VERBOSE
					logerror("Surface 1 Bad Track 2\r\n");
#endif

					i8271_write_bad_track(1, 2, i8271.CommandParameters[1]);
				}
				break;

				case I8271_SPECIAL_REGISTER_DRIVE_CONTROL_OUTPUT_PORT:
				{
					/* get drive selected */
					i8271.drive = (i8271.CommandParameters[1]>>6) & 0x03;

					i8271.drive_control_output = i8271.CommandParameters[1];

#ifdef VERBOSE
					if (i8271.CommandParameters[1] & 0x01)
					{
						logerror("Write Enable\r\n");
					}
					if (i8271.CommandParameters[1] & 0x02)
					{
						logerror("Seek/Step\r\n");
					}
					if (i8271.CommandParameters[1] & 0x04)
					{
						logerror("Direction\r\n");
					}
					if (i8271.CommandParameters[1] & 0x08)
					{
						logerror("Load Head\r\n");
					}
					if (i8271.CommandParameters[1] & 0x010)
					{
						logerror("Low head current\r\n");
					}
					if (i8271.CommandParameters[1] & 0x020)
					{
						logerror("Write Fault Reset\r\n");
					}

					if (i8271.CommandParameters[1] & 0x040)
					{
						logerror("Select %02x\r\n", (i8271.CommandParameters[1] & 0x0c0)>>6);
					}
#endif


				}
				break;

				case I8271_SPECIAL_REGISTER_DRIVE_CONTROL_INPUT_PORT:
				{
//					i8271.drive_control_input = i8271.CommandParameters[1];
				}
				break;

			}

			/* write doesn't supply a result */
			i8271_command_complete();
		}
		break;

		case I8271_COMMAND_READ_DRIVE_STATUS:
		{
			i8271_get_drive();

			/* command is complete */
			i8271_command_complete();

			floppy_drive_status[i8271.drive] = (1<<6) | (1<<2);
			i8271_setup_result(floppy_drive_status[i8271.drive]);

		}
		break;

		case I8271_COMMAND_SEEK:
		{
			/* to be completed! */
			int completion_code = 0;

			i8271_get_drive();

			/* seek to track 0 */
		//	floppy_drive_seek(i8271.drive, i8271.CommandParameters[0]);

		//	i8271_write_current_track(0, i8271.CommandParameters[1]);


			/* check for bad seek */
			i8271_command_complete();
			i8271_setup_result(completion_code);
			i8271_set_irq_state(1);

		}
		break;

		case I8271_COMMAND_READ_DATA_MULTI_RECORD:
		{
			/* N value as stored in ID field */
			i8271.ID_N = (i8271.CommandParameters[2]>>5) & 0x07;

			/* starting sector id */
			i8271.ID_R = i8271.CommandParameters[1];

			/* number of sectors to transfer */
			i8271.SectorCount = i8271.CommandParameters[2] & 0x01f;

			/* read */
			i8271.flags |= I8271_FLAGS_DATA_DIRECTION;

#ifdef VERBOSE
			logerror("Track: %02x\r\n",i8271.CommandParameters[0]);
			logerror("Sector: %02x\r\n", i8271.CommandParameters[1]);
			logerror("Sector Length: %02x bytes", 1<<(i8271.ID_N+7));
#endif

			i8271_get_drive();
			i8271_initialise_execution_phase_read(temp_buffer);
			i8271_update_state();


//			i8271_seek_to_track(i8271.CommandParameters[0]);



		}
		break;

		case I8271_COMMAND_READ_DATA_SINGLE_RECORD:
		{
			i8271.ID_N = 0;
			i8271.SectorCount = 1;
			i8271.ID_R = i8271.CommandParameters[1];

			/* read */
			i8271.flags |= I8271_FLAGS_DATA_DIRECTION;

#ifdef VERBOSE
			logerror("Track: %02x\r\n",i8271.CommandParameters[0]);
			logerror("Sector: %02x\r\n", i8271.CommandParameters[1]);
			logerror("Sector Length: %02x bytes", 1<<(i8271.ID_N+7));
#endif
		}
		break;

		default:
#ifdef VERBOSE
			logerror("ERROR Unrecognised Command\r\n");
#endif
			break;
	}
}



WRITE_HANDLER(i8271_w)
{
	switch (offset & 3)
	{
		case 0:
		{
#ifdef VERBOSE
			logerror("I8271 W Command Register: %02x\r\n", data);
#endif

			i8271.CommandRegister = data;
			i8271.Command = i8271.CommandRegister & 0x03f;

			i8271.StatusRegister |= I8271_STATUS_COMMAND_BUSY | I8271_STATUS_COMMAND_FULL;
			i8271.StatusRegister &= ~I8271_STATUS_PARAMETER_FULL | I8271_STATUS_RESULT_FULL;
			i8271.ParameterCountWritten = 0;

			switch (i8271.Command)
			{
				case I8271_COMMAND_SPECIFY:
				{
					FDC_LOG_COMMAND("SPECIFY");

					i8271.ParameterCount = 4;
				}
				break;

				case I8271_COMMAND_SEEK:
				{
					FDC_LOG_COMMAND("SEEK");

					i8271.ParameterCount = 1;
				}
				break;

				case I8271_COMMAND_READ_DRIVE_STATUS:
				{
					FDC_LOG_COMMAND("READ DRIVE STATUS");

					i8271.ParameterCount = 0;
				}
				break;

				case I8271_COMMAND_READ_SPECIAL_REGISTER:
				case I8271_COMMAND_WRITE_SPECIAL_REGISTER:
				{
					FDC_LOG_COMMAND("READ/WRITE SPECIAL REGISTER");

					i8271.ParameterCount = 1;
				}
				break;

				case I8271_COMMAND_FORMAT:
				{
					i8271.ParameterCount = 5;
				}
				break;

				case I8271_COMMAND_READ_ID:
				{
					i8271.ParameterCount = 3;

				}
				break;


				case I8271_COMMAND_READ_DATA_SINGLE_RECORD:
				case I8271_COMMAND_READ_DATA_AND_DELETED_DATA_SINGLE_RECORD:
				case I8271_COMMAND_WRITE_DATA_SINGLE_RECORD:
				case I8271_COMMAND_WRITE_DELETED_DATA_SINGLE_RECORD:
				case I8271_COMMAND_VERIFY_DATA_AND_DELETED_DATA_SINGLE_RECORD:
				{
					i8271.ParameterCount = 2;
				}
				break;

				case I8271_COMMAND_READ_DATA_MULTI_RECORD:
				case I8271_COMMAND_READ_DATA_AND_DELETED_DATA_MULTI_RECORD:
				case I8271_COMMAND_WRITE_DATA_MULTI_RECORD:
				case I8271_COMMAND_WRITE_DELETED_DATA_MULTI_RECORD:
				case I8271_COMMAND_VERIFY_DATA_AND_DELETED_DATA_MULTI_RECORD:
				{
					i8271.ParameterCount = 3;
				}
				break;

				case I8271_COMMAND_SCAN_DATA:
				case I8271_COMMAND_SCAN_DATA_AND_DELETED_DATA:
				{
					i8271.ParameterCount = 5;
				}
				break;






			}

			i8271_check_all_parameters_written();
		}
		break;

		case 1:
		{
			logerror("I8271 W Parameter Register: %02x\r\n",data);

			i8271.ParameterRegister = data;

			i8271.CommandParameters[i8271.ParameterCountWritten] = data;
			i8271.ParameterCountWritten++;

			i8271_check_all_parameters_written();
		}
		break;

		case 2:
		{
			logerror("I8271 W Reset Register: %02x\r\n", data);
			i8271.ResetRegister = data;
		}
		break;

		default:
			break;
	}
}

READ_HANDLER(i8271_r)
{
	switch (offset & 3)
	{
		case 0:
		{
			/* bit 1,0 are zero other bits contain status data */
			i8271.StatusRegister &= ~0x03;

			logerror("I8271 R Status Register: %02x\r\n",i8271.StatusRegister);

			return i8271.StatusRegister;
		}

		case 1:
		{

			if ((i8271.StatusRegister & I8271_STATUS_COMMAND_BUSY)==0)
			{
				/* clear IRQ */
				i8271_set_irq_state(0);

				i8271.StatusRegister &= ~I8271_STATUS_RESULT_FULL;

				logerror("I8271 R Result Register %02x\r\n",i8271.ResultRegister);

				return i8271.ResultRegister;
			}

			/* not useful information when command busy */
			return 0x0ff;
		}


		default:
			break;
	}

	return 0x0ff;
}


/* to be completed! */
READ_HANDLER(i8271_dack_r)
{
	return i8271_data_r(offset);
}

/* to be completed! */
WRITE_HANDLER(i8271_dack_w)
{
	i8271_data_w(offset, data);
}

READ_HANDLER(i8271_data_r)
{

	i8271_clear_data_request();

	i8271_update_state();

	return i8271.data;
}

WRITE_HANDLER(i8271_data_w)
{
	i8271.data = data;

	i8271_clear_data_request();

	i8271_update_state();
}

