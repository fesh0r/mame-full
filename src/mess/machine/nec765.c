/***************************************************************************

	machine/nec765.c

	Functions to emulate a NEC765 compatible floppy disk controller

	TODO:
	implement more commands: eg. read ID, read/write deleted
	handle images containing some sort of geometry information
	(ie. support non constant sector lengths / sizes per disk).

***************************************************************************/
#include "driver.h"
#include "mess/machine/nec765.h"

typedef enum
{
        NEC765_COMMAND_PHASE_FIRST_BYTE,
        NEC765_COMMAND_PHASE_BYTES,
        NEC765_RESULT_PHASE,
        NEC765_EXECUTION_PHASE_READ,
        NEC765_EXECUTION_PHASE_WRITE
} NEC765_PHASE;

/* uncomment the following line for verbose information */
#define VERBOSE

unsigned char    FDC_main;
NEC765_PHASE    nec765_phase;
unsigned int    nec765_command_bytes[16];
unsigned int    nec765_result_bytes[16];
unsigned int    nec765_transfer_bytes_remaining;
unsigned int    nec765_transfer_bytes_count;
unsigned int    nec765_status[4];
unsigned int    nec765_pcn;
unsigned int    nec765_drive;
unsigned int    nec765_id_index;
unsigned int    nec765_side;
char *execution_phase_data;

nec765_interface nec765_iface;

#define NEC765_DISK_PRESENT     0x0001

int nec765_drive_state[4]=
{
        0,0,0,0
};

static int nec765_cmd_size[32] = {
	1,1,3,3,1,9,9,2,1,9,2,1,9,6,1,3,
	1,9,1,1,1,1,9,1,1,9,1,1,1,9,1,1
};

static int nec765_sta_size[32] = {
	1,1,7,0,1,7,7,0,2,7,7,1,7,7,1,0,
	1,7,1,1,1,1,7,1,1,7,1,1,1,7,1,1
};

static void     nec765_setup_execution_phase_read(char *ptr, int size)
{
        FDC_main |=0x080;                       /* DRQ */
        FDC_main |= 0x040;                     /* FDC->CPU */

        nec765_transfer_bytes_count = 0;
        nec765_transfer_bytes_remaining = size;
        execution_phase_data = ptr;
        nec765_phase = NEC765_EXECUTION_PHASE_READ;
}

static void     nec765_setup_execution_phase_write(char *ptr, int size)
{
        FDC_main |=0x080;                       /* DRQ */
        FDC_main &= ~0x040;                     /* FDC->CPU */

        nec765_transfer_bytes_count = 0;
        nec765_transfer_bytes_remaining = size;
        execution_phase_data = ptr;
        nec765_phase = NEC765_EXECUTION_PHASE_WRITE;
}


static void     nec765_setup_result_phase(void)
{
        FDC_main |=0x080;                       /* DRQ */
        FDC_main |= 0x040;                     /* FDC->CPU */
        FDC_main &= ~0x020;                    /* not execution phase */

        nec765_transfer_bytes_count = 0;
        nec765_transfer_bytes_remaining = nec765_sta_size[nec765_command_bytes[0] & 0x01f];
        nec765_phase = NEC765_RESULT_PHASE;
}

void nec765_idle(void)
{
        FDC_main |=0x080;                       /* DRQ */
        FDC_main &= ~0x040;                     /* CPU->FDC */
        FDC_main &= ~0x020;                    /* not execution phase */
        FDC_main &= ~0x010;                     /* not busy */
        nec765_phase = NEC765_COMMAND_PHASE_FIRST_BYTE;
}


void    nec765_init(nec765_interface *iface)
{
        memset(&nec765_iface, 0, sizeof(nec765_interface));

        if (iface)
        {
                memcpy(&nec765_iface, iface, sizeof(nec765_interface));
        }

        nec765_idle();
}

void    nec765_setup_drive_status(int drive)
{
        nec765_drive_state[drive] = NEC765_DISK_PRESENT;
}


int nec765_status_r(void)
{
	return FDC_main;
}


static void     nec765_read_data(void)
{
        int data_size;
        int idx;
        int spt;

        idx = -1;
        spt = 0;

        if (nec765_iface.get_sectors_per_track)
        {
                spt = nec765_iface.get_sectors_per_track(nec765_drive, nec765_side);
        }

        if (spt!=0)
        {
                nec765_id id;
                int i;

                for (i=0; i<spt; i++)
                {
                     nec765_iface.get_id_callback(nec765_drive, &id, i, nec765_side);

                     if ((id.R == nec765_command_bytes[4]) && (id.C == nec765_command_bytes[2]) && (id.H == nec765_command_bytes[3]))
                     {
                        idx = i;
                        break;
                     }
                }
        }

        if (idx!=-1)
        {
                char *ptr;

                nec765_iface.get_sector_data_callback(nec765_drive, idx, nec765_side, &ptr);


                /* 0-> 128 bytes, 1->256 bytes, 2->512 bytes etc */
                /* data_size = ((1<<(N+7)) */
                data_size = 1<<(nec765_command_bytes[5]+7);

                nec765_setup_execution_phase_read(ptr, data_size);
        }
        else
        {
                nec765_status[0] = 0x040 | nec765_drive;
                nec765_status[1] = 0x04;
                nec765_status[2] = 0x00;

                nec765_result_bytes[0] = nec765_status[0];
                nec765_result_bytes[1] = nec765_status[1];
                nec765_result_bytes[2] = nec765_status[2];
                nec765_result_bytes[3] = nec765_command_bytes[2]; /* C */
                nec765_result_bytes[4] = nec765_command_bytes[3]; /* H */
                nec765_result_bytes[5] = nec765_command_bytes[4]; /* R */
                nec765_result_bytes[6] = nec765_command_bytes[5]; /* N */

                nec765_setup_result_phase();
        }

}

static void     nec765_continue_command(void)
{
        switch (nec765_command_bytes[0] & 0x01f)
        {
                case 0x05:
                                nec765_result_bytes[0] = nec765_status[0];
                                nec765_result_bytes[1] = nec765_status[1];
                                nec765_result_bytes[2] = nec765_status[2];
                                nec765_result_bytes[3] = nec765_command_bytes[2]; /* C */
                                nec765_result_bytes[4] = nec765_command_bytes[3]; /* H */
                                nec765_result_bytes[5] = nec765_command_bytes[4]; /* R */
                                nec765_result_bytes[6] = nec765_command_bytes[5]; /* N */

                                nec765_setup_result_phase();
				break;
                case 0x06:
                {
                        /* read all sectors? */
                        if (nec765_command_bytes[4]==nec765_command_bytes[6])
                        {
                                nec765_result_bytes[0] = nec765_status[0];
                                nec765_result_bytes[1] = nec765_status[1];
                                nec765_result_bytes[2] = nec765_status[2];
                                nec765_result_bytes[3] = nec765_command_bytes[2]; /* C */
                                nec765_result_bytes[4] = nec765_command_bytes[3]; /* H */
                                nec765_result_bytes[5] = nec765_command_bytes[4]; /* R */
                                nec765_result_bytes[6] = nec765_command_bytes[5]; /* N */

                                nec765_setup_result_phase();
                        }
                        else
                        {
                                nec765_command_bytes[4]++;

                                nec765_read_data();
                        }
                }
                break;


                default:
                        break;
       }
}
static void     nec765_write_data(void)
{
        int data_size;
        int idx;
        int spt;

        idx = -1;
        spt = 0;

        if (nec765_iface.get_sectors_per_track)
        {
                spt = nec765_iface.get_sectors_per_track(nec765_drive, nec765_side);
        }

        if (spt!=0)
        {
                nec765_id id;
                int i;

                for (i=0; i<spt; i++)
                {
                     nec765_iface.get_id_callback(nec765_drive, &id, i, nec765_side);

                     if ((id.R == nec765_command_bytes[4]) && (id.C == nec765_command_bytes[2]) && (id.H == nec765_command_bytes[3]))
                     {
                        idx = i;
                        break;
                     }
                }
        }

        if (idx!=-1)
        {
                char *ptr;

                nec765_iface.get_sector_data_callback(nec765_drive, idx, nec765_side, &ptr);
                //ptr=&ptr;
                /* 0-> 128 bytes, 1->256 bytes, 2->512 bytes etc */
                /* data_size = ((1<<(N+7)) */
                data_size = 1<<(nec765_command_bytes[5]+7);

                nec765_setup_execution_phase_write(ptr, data_size);
        }
        else
        {
                nec765_status[0] = 0x040 | nec765_drive;
                nec765_status[1] = 0x04;
                nec765_status[2] = 0x00;

                nec765_result_bytes[0] = nec765_status[0];
                nec765_result_bytes[1] = nec765_status[1];
                nec765_result_bytes[2] = nec765_status[2];
                nec765_result_bytes[3] = nec765_command_bytes[2]; /* C */
                nec765_result_bytes[4] = nec765_command_bytes[3]; /* H */
                nec765_result_bytes[5] = nec765_command_bytes[4]; /* R */
                nec765_result_bytes[6] = nec765_command_bytes[5]; /* N */

                nec765_setup_result_phase();
        }

}

int nec765_data_r(void)
{
        switch (nec765_phase)
        {
            case NEC765_RESULT_PHASE:
            {
                int data;


                data = nec765_result_bytes[nec765_transfer_bytes_count];

#ifdef VERBOSE
                logerror("NEC765: RESULT: %02x\r\n", data);
#endif

                nec765_transfer_bytes_count++;
                nec765_transfer_bytes_remaining--;

                if (nec765_transfer_bytes_remaining==0)
                {
                        nec765_idle();
                }

                return data;
            }

            case NEC765_EXECUTION_PHASE_READ:
            {
                int data;

                data = execution_phase_data[nec765_transfer_bytes_count];       /*0x0e5;   //nec765_result_bytes[nec765_transfer_bytes_count]; */
                nec765_transfer_bytes_count++;
                nec765_transfer_bytes_remaining--;

                if (nec765_transfer_bytes_remaining==0)
                {
                        nec765_continue_command();
                }

                return data;
            }


            default:
                break;
        }

        return 0x0ff;
}





static void     nec765_setup_command(void)
{
        FDC_main &= ~0x080;             /* clear DRQ */
        FDC_main |= 0x020;              /* execution phase */

        switch (nec765_command_bytes[0] & 0x01f)
        {
            case 0x03:      /* specify */
                nec765_idle();
                break;

            case 0x04:  /* sense drive status */
                /* get drive */
                nec765_drive = nec765_command_bytes[1] & 0x03;
                /* get side */
                nec765_side = (nec765_command_bytes[1]>>2) & 0x01;

                nec765_status[3] = nec765_drive | (nec765_side<<2);
                nec765_status[3] |= 0x040;

                nec765_result_bytes[0] = nec765_status[3];

                nec765_setup_result_phase();
                break;

            case 0x07:          /* recalibrate */
                nec765_drive = nec765_command_bytes[1] & 0x03;

                if (nec765_drive_state[nec765_drive] & NEC765_DISK_PRESENT)
                {
                        nec765_pcn = 0;
                        nec765_status[0] = 0x020;
                        nec765_status[0] |= nec765_drive;

                        if (nec765_iface.seek_callback)
                           nec765_iface.seek_callback(nec765_drive, nec765_pcn);
                }
                else
                {
                        nec765_status[0] = 0x010 | 0x08 | 0x040;
                }

                nec765_idle();
                break;
            case 0x0f:          /* seek */
                nec765_drive = nec765_command_bytes[1] & 0x03;

                if (nec765_drive_state[nec765_drive] & NEC765_DISK_PRESENT)
                {
                        nec765_pcn = nec765_command_bytes[2];
                        nec765_status[0] = 0x020;
                        nec765_status[0] |= nec765_drive;

                        if (nec765_iface.seek_callback)
                           nec765_iface.seek_callback(nec765_drive, nec765_pcn);
                }
                else
                {
                        nec765_status[0] = 0x010 | 0x08 | 0x040;
                }

                nec765_idle();
                break;

            case 0x0a:      /* read id */
            {
                int spt;
                nec765_id id;

                /* get drive */
                nec765_drive = nec765_command_bytes[1] & 0x03;
                /* get side */
                nec765_side = (nec765_command_bytes[1]>>2) & 0x01;

                spt = 0;

                nec765_status[0] = 0;
                nec765_status[1] = 0;
                nec765_status[2] = 0;

                if (nec765_iface.get_sectors_per_track)
                {
                        spt = nec765_iface.get_sectors_per_track(nec765_drive, nec765_side);
                }

                if (spt!=0)
                {
                        nec765_id_index++;
                        nec765_id_index = nec765_id_index % spt;

                        if (nec765_iface.get_id_callback)
                                nec765_iface.get_id_callback(nec765_drive, &id, nec765_id_index, nec765_side);
                 }
                 else
                 {
                        nec765_status[0] = 0x040;
                        nec765_status[1] |= 0x04;       /* no data */
                 }

                nec765_result_bytes[0] = nec765_status[0];
                nec765_result_bytes[1] = nec765_status[1];
                nec765_result_bytes[2] = nec765_status[2];
                nec765_result_bytes[3] = id.C; /* C */
                nec765_result_bytes[4] = id.H; /* H */
                nec765_result_bytes[5] = id.R; /* R */
                nec765_result_bytes[6] = id.N; /* N */


                 nec765_setup_result_phase();
            }
            break;


		case 0x08: /* sense interrupt status */
  			nec765_result_bytes[0] = nec765_status[0];

			if (nec765_status[0] == 0x080)
			{
				nec765_command_bytes[0] = 0;
			}
			else
			{

		    		nec765_status[0] = 0x080;
                		nec765_result_bytes[1] = nec765_pcn;
			}
                nec765_setup_result_phase();
                break;

            case 0x06:  /* read data */
            {
                nec765_status[0] = 0;
                nec765_status[1] = 0;
                nec765_status[2] = 0;

                nec765_read_data();
            }
	    break;
            case 0x05:  /* write data */
            {
                nec765_status[0] = 0;
                nec765_status[1] = 0;
                nec765_status[2] = 0;

                nec765_write_data();
            }
            break;

            default:
                break;
        }
}


void nec765_data_w(int data)
{
        switch (nec765_phase)
        {
                case NEC765_COMMAND_PHASE_FIRST_BYTE:
                {
                        FDC_main |= 0x10;                      /* set BUSY */
#ifdef VERBOSE
                        logerror("NEC765: COMMAND: %02x\r\n",data);

#endif
                        nec765_command_bytes[0] = data;
                        nec765_transfer_bytes_remaining = nec765_cmd_size[data & 0x01f];
                        nec765_transfer_bytes_count = 1;

                        nec765_transfer_bytes_remaining--;

                        if (nec765_transfer_bytes_remaining==0)
                        {
                                nec765_setup_command();
                        }
                        else
                        {
                                nec765_phase = NEC765_COMMAND_PHASE_BYTES;
                        }
                }
                break;

                case NEC765_COMMAND_PHASE_BYTES:
                {
#ifdef VERBOSE
                        logerror("NEC765: COMMAND: %02x\r\n",data);
#endif
                        nec765_command_bytes[nec765_transfer_bytes_count] = data;
                        nec765_transfer_bytes_count++;
                        nec765_transfer_bytes_remaining--;

                        if (nec765_transfer_bytes_remaining==0)
                        {
                                nec765_setup_command();
                        }
                }
                break;

            case NEC765_EXECUTION_PHASE_WRITE:
            {
                execution_phase_data[nec765_transfer_bytes_count]=data;       /*0x0e5;   //nec765_result_bytes[nec765_transfer_bytes_count]; */
                nec765_transfer_bytes_count++;
                nec765_transfer_bytes_remaining--;

                if (nec765_transfer_bytes_remaining==0)
                {

                        nec765_continue_command();
                }

            }
	    break;


                default:
                        break;
        }
}
