/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

Amstrad hardware consists of:

- General Instruments AY-3-8912 (audio and keyboard scanning)
- Intel 8255PPI (keyboard, access to AY-3-8912, cassette etc)
- Z80A CPU
- 765 FDC (disc drive interface)
- "Gate Array" (custom chip by Amstrad controlling colour, mode,
rom/ram selection


***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
//#include "mess/vidhrdw/m6845.h"
#include "mess/machine/amstrad.h"
//#include "mess/systems/i8255.h"
#include "machine/8255ppi.h"
#include "mess/machine/nec765.h"

void    AmstradCPC_GA_Write(int);
void    AmstradCPC_SetUpperRom(int);
void    Amstrad_RethinkMemory(void);
void    Amstrad_Init(void);
void    amstrad_handle_snapshot(unsigned char *);
void     amstrad_disk_image_init(void);


/* hd6845s functions */
int     hd6845s_index_r(void);
int     hd6845s_register_r(void);
void    hd6845s_index_w(int data);
void    hd6845s_register_w(int data);
int 	hd6845s_getreg(int);


static unsigned char *file_loaded = NULL;
static int snapshot_loaded = 0;

extern unsigned char *Amstrad_Memory;

int amstrad_disk_image_type;

/* used to setup computer if a snapshot was specified */
int amstrad_opbaseoverride(int pc)
{
  /* clear op base override */
  cpu_setOPbaseoverride(0,0);

  if (snapshot_loaded)
  {
        /* its a snapshot file - setup hardware state */
        amstrad_handle_snapshot(file_loaded);
  }

  return (cpu_get_pc() & 0x0ffff);
}

void    amstrad_init_machine(void)
{
        //int i;
//        unsigned char *gfx = memory_region(REGION_GFX1);

        /* allocate ram - I control how it is accessed so I must
        allocate it somewhere - here will do */
        Amstrad_Memory = malloc(128*1024);

  //      for (i=0; i<256; i++)
    //    {
      //          gfx[i] = i;
        //}


        Amstrad_Init();

        if (file_loaded!=NULL)
        {
                /* snapshot? */
                if (memcmp(&file_loaded[0],"MV - SNA",8)==0)
                {
                   /* setup for snapshot */
                   snapshot_loaded = 1;
                   cpu_setOPbaseoverride(0,amstrad_opbaseoverride);
                }
                else
                {
                   /* must be a disk image */
                   amstrad_disk_image_init();
                }
        }

        if (!snapshot_loaded)
        {
              Amstrad_Reset();
        }
}

void    amstrad_shutdown_machine(void)
{
	if (Amstrad_Memory != NULL)
		free(Amstrad_Memory);

	Amstrad_Memory = NULL;

        if (file_loaded!=NULL)
                free(file_loaded);


}

/* load CPCEMU style snapshots */
void    amstrad_handle_snapshot(unsigned char *pSnapshot)
{
        int RegData;
        int i;


        /* init Z80 */
        RegData = (pSnapshot[0x011] & 0x0ff) | ((pSnapshot[0x012] & 0x0ff)<<8);
        cpu_set_reg(Z80_AF, RegData);

        RegData = (pSnapshot[0x013] & 0x0ff) | ((pSnapshot[0x014] & 0x0ff)<<8);
        cpu_set_reg(Z80_BC, RegData);

        RegData = (pSnapshot[0x015] & 0x0ff) | ((pSnapshot[0x016] & 0x0ff)<<8);
        cpu_set_reg(Z80_DE, RegData);

        RegData = (pSnapshot[0x017] & 0x0ff) | ((pSnapshot[0x018] & 0x0ff)<<8);
        cpu_set_reg(Z80_HL, RegData);

        RegData = (pSnapshot[0x019] & 0x0ff) ;
        cpu_set_reg(Z80_R, RegData);

        RegData = (pSnapshot[0x01a] & 0x0ff);
        cpu_set_reg(Z80_I, RegData);

        if ((pSnapshot[0x01b] & 1)==1)
        {
                cpu_set_reg(Z80_IFF1, 1);
        }
        else
        {
                cpu_set_reg(Z80_IFF1, 0);
        }

        if ((pSnapshot[0x01c] & 1)==1)
        {
                cpu_set_reg(Z80_IFF2, 1);
        }
        else
        {
                cpu_set_reg(Z80_IFF2, 0);
        }

        RegData = (pSnapshot[0x01d] & 0x0ff) | ((pSnapshot[0x01e] & 0x0ff)<<8);
        cpu_set_reg(Z80_IX, RegData);

        RegData = (pSnapshot[0x01f] & 0x0ff) | ((pSnapshot[0x020] & 0x0ff)<<8);
        cpu_set_reg(Z80_IY, RegData);

        RegData = (pSnapshot[0x021] & 0x0ff) | ((pSnapshot[0x022] & 0x0ff)<<8);
        cpu_set_reg(Z80_SP, RegData);
       cpu_set_sp(RegData);

        RegData = (pSnapshot[0x023] & 0x0ff) | ((pSnapshot[0x024] & 0x0ff)<<8);

        cpu_set_reg(Z80_PC, RegData);
//        cpu_set_pc(RegData);

        RegData = (pSnapshot[0x025] & 0x0ff);
        cpu_set_reg(Z80_IM, RegData);

        RegData = (pSnapshot[0x026] & 0x0ff) | ((pSnapshot[0x027] & 0x0ff)<<8);
        cpu_set_reg(Z80_AF2, RegData);

        RegData = (pSnapshot[0x028] & 0x0ff) | ((pSnapshot[0x029] & 0x0ff)<<8);
        cpu_set_reg(Z80_BC2, RegData);

        RegData = (pSnapshot[0x02a] & 0x0ff) | ((pSnapshot[0x02b] & 0x0ff)<<8);
        cpu_set_reg(Z80_DE2, RegData);

        RegData = (pSnapshot[0x02c] & 0x0ff) | ((pSnapshot[0x02d] & 0x0ff)<<8);
        cpu_set_reg(Z80_HL2, RegData);

        /* init GA */
        for (i=0; i<17; i++)
        {
                AmstradCPC_GA_Write(i);

                AmstradCPC_GA_Write(((pSnapshot[0x02f + i] & 0x01f) | 0x040));
        }

        AmstradCPC_GA_Write(pSnapshot[0x02e] & 0x01f);

        AmstradCPC_GA_Write(((pSnapshot[0x040] & 0x03f) | 0x080));

        AmstradCPC_GA_Write(((pSnapshot[0x041] & 0x03f) | 0x0c0));

        /* init CRTC */
        for (i=0; i<18; i++)
        {
                hd6845s_index_w(i);
                hd6845s_register_w(pSnapshot[0x043+i] & 0x0ff);
        }

        hd6845s_index_w(pSnapshot[0x042] & 0x0ff);

        /* upper rom selection */
        AmstradCPC_SetUpperRom(pSnapshot[0x055]);

        /* PPI */
        ppi8255_w(0,3,pSnapshot[0x059] & 0x0ff);

        ppi8255_w(0,0,pSnapshot[0x056] & 0x0ff);
        ppi8255_w(0,1,pSnapshot[0x057] & 0x0ff);
        ppi8255_w(0,2,pSnapshot[0x058] & 0x0ff);

        /* PSG */
        for (i=0; i<16; i++)
        {
                AY8910_control_port_0_w(0,i);

                AY8910_write_port_0_w(0,pSnapshot[0x05b + i] & 0x0ff);
        }

        AY8910_control_port_0_w(0,pSnapshot[0x05a]);

        {
                int MemSize;
                int MemorySize;

                MemSize = (pSnapshot[0x06b] & 0x0ff) | ((pSnapshot[0x06c] & 0x0ff)<<8);

                if (MemSize==128)
                {
                        MemorySize = 128*1024;
                }
                else
                {
                        MemorySize = 64*1024;
                }

                memcpy(Amstrad_Memory, &pSnapshot[0x0100], MemorySize);

        }

        Amstrad_RethinkMemory();

}

/* load image, either snapshot or disk image */
int	amstrad_rom_load()
{
        void *file;

        char *filename_source = NULL;

        /* rom_name for .sna etc */
        if (rom_name[0]!=0)
        {
                filename_source = (char *)&rom_name[0];
        }

        /* floppy_name for .dsk etc */
        if (floppy_name[0]!=0)
        {
                filename_source = (char *)&floppy_name[0];
        }

        if (filename_source!=NULL)
        {
                file = osd_fopen(Machine->gamedrv->name, filename_source, OSD_FILETYPE_IMAGE_RW, 0);

                if (file)
                {
                        int datasize;
                        unsigned char *data;

                        /* get file size */
                        datasize = osd_fsize(file);

                        if (datasize!=0)
                        {
                                /* malloc memory for this data */
                                data = malloc(datasize);

                                if (data!=NULL)
                                {
                                        /* read whole file */
                                        osd_fread(file, data, datasize);

                                        file_loaded = data;

                                        /* close file */
                                        osd_fclose(file);

                                        if (errorlog) fprintf(errorlog,"File loaded!\r\n");

                                        /* ok! */
                                        return 0;
                                }
                                osd_fclose(file);

                        }


                        return 1;
                }
        }
        return 0;
}

/* simple check to see if image file is supported */
int	amstrad_rom_id(const char *name, const char *gamename)
{
#if 0
        int return_value;
        FILE *romfile;
        unsigned char header_data[8];

        return_value = 0;

        /* load file */
        if (!(romfile=osd_fopen(name, gamename, OSD_FILETYPE_IMAGE_R,0))) return 0;

        /* read 8 bytes of the file */
        osd_fread(romfile, header_data, 8);

        /* check header */
        if (
                /* CPCEMU style snapshot? */
                (memcmp(header_data, "MV - SNA", 8)==0) ||
                /* CPCEMU style standard disk image? */
                (memcmp(header_data, "MV - CPC", 8)==0) ||
                /* CPCEMU style extended disk image? */
                (memcmp(header_data, "EXTENDED", 8)==0)
            )
        {
                return_value = 1;
        }

        osd_fclose(romfile);

        return return_value;
#endif

return 1;
}


/* disk image and extended disk image support code */
/* supports up to 84 tracks and 2 sides */

#define AMSTRAD_MAX_TRACKS 84
#define AMSTRAD_MAX_SIDES   2

/* offsets to start of each track's data */
unsigned long    amstrad_dsk_track_offset[AMSTRAD_MAX_TRACKS*AMSTRAD_MAX_SIDES];

/* offset to sector data on current track */
unsigned long   amstrad_dsk_sector_offset[18];

unsigned long   amstrad_drive_current_track[2];


void     amstrad_dsk_init_track_offsets(void)
{
        int track_offset;
        int i;
        int track_size;
        int tracks, sides;
        int skip, length,offs;

        /* get size of each track from main header. Size of each
        track includes a 0x0100 byte header, and the actual sector data for
        all sectors on the track */
        track_size = file_loaded[0x032] | (file_loaded[0x033]<<8);

        /* main header is 0x0100 in size */
        track_offset = 0x0100;

        sides = file_loaded[0x031];
        tracks = file_loaded[0x030];

        memset(amstrad_dsk_track_offset, 0, sizeof(amstrad_dsk_track_offset));

        /* single sided? */
        if (sides==1)
        {
                skip = 2;
                length = tracks;
        }
        else
        {
                skip = 1;
                length = tracks*sides;
        }

        offs = 0;
        for (i=0; i<length; i++)
        {
                amstrad_dsk_track_offset[offs] = track_offset;
                track_offset+=track_size;
                offs+=skip;
        }

}

void    amstrad_dsk_init_sector_offsets(int track,int side)
{
        int track_offset;

        /* get offset to track header in image */
        track_offset = amstrad_dsk_track_offset[(track<<1) + side];

        if (track_offset!=0)
        {
                int spt;
                int sector_offset;
                int sector_size;
                int i;

                unsigned char *track_header;

                track_header= &file_loaded[track_offset];

                /* sectors per track as specified in nec765 format command */
                /* sectors on this track */
                spt = track_header[0x015];

                sector_size = (1<<(track_header[0x014]+7));

                /* track header is 0x0100 bytes in size */
                sector_offset = 0x0100;

                for (i=0; i<spt; i++)
                {
                        amstrad_dsk_sector_offset[i] = sector_offset;
                        sector_offset+=sector_size;
                }
        }
}

void     amstrad_extended_dsk_init_track_offsets(void)
{
        int track_offset;
        int i;
        int track_size;
        int tracks, sides;
        int offs, skip, length;

        sides = file_loaded[0x031];
        tracks = file_loaded[0x030];

        if (sides==1)
        {
                skip = 2;
                length = tracks;
        }
        else
        {
                skip = 1;
                length = tracks*sides;
        }

        /* main header is 0x0100 in size */
        track_offset = 0x0100;
        offs = 0;
        for (i=0; i<length; i++)
        {
              int track_size_high_byte;

              /* track size is specified as a byte, and is multiplied
              by 256 to get size in bytes. If 0, track doesn't exist and
              is unformatted, otherwise it exists. Track size includes 0x0100
              header */
              track_size_high_byte = file_loaded[0x034 + i];

                if (track_size_high_byte != 0)
                {
                        /* formatted track */
                        track_size = track_size_high_byte<<8;

                        amstrad_dsk_track_offset[offs] = track_offset;
                        track_offset+=track_size;
               }

               offs+=skip;
       }
}


void    amstrad_extended_dsk_init_sector_offsets(int track,int side)
{
        int track_offset;

        /* get offset to track header in image */
        track_offset = amstrad_dsk_track_offset[(track<<1) + side];

        if (track_offset!=0)
        {
                int spt;
                int sector_offset;
                int sector_size;
                int i;
                unsigned char *id_info;
                unsigned char *track_header;

                track_header= &file_loaded[track_offset];

                /* sectors per track as specified in nec765 format command */
                /* sectors on this track */
                spt = track_header[0x015];

                id_info = track_header + 0x018;

                /* track header is 0x0100 bytes in size */
                sector_offset = 0x0100;

                for (i=0; i<spt; i++)
                {
                        sector_size = id_info[6] + (id_info[7]<<8);

                        amstrad_dsk_sector_offset[i] = sector_offset;
                        sector_offset+=sector_size;
                }
        }
}



void     amstrad_disk_image_init(void)
{

        if (memcmp(&file_loaded[0],"MV - CPC",8)==0)
        {
                amstrad_disk_image_type = 0;

                /* standard disk image */
                amstrad_dsk_init_track_offsets();

                nec765_setup_drive_status(0);
        }
        else if (memcmp(&file_loaded[0],"EXTENDED",8)==0)
        {
                amstrad_disk_image_type = 1;

                /* extended disk image */
                amstrad_extended_dsk_init_track_offsets();

                nec765_setup_drive_status(0);

        }
        else
        {
                amstrad_disk_image_type = -1;

                if (errorlog) fprintf(errorlog,"CPC6128 driver: Unsupported disk image format\r\n");
        }
}


void amstrad_seek_callback(int drive, int physical_track)
{
        amstrad_drive_current_track[(drive & 0x01)] = physical_track;
}

void    amstrad_get_id_callback(int drive, nec765_id *id, int id_index, int side)
{
        int id_offset;
        int track_offset;
        unsigned char *track_header;

        /* get offset to track header in image */
        track_offset = amstrad_dsk_track_offset[(amstrad_drive_current_track[(drive & 0x01)]<<1) + side];

        /* track exists? */
        if (track_offset==0)
                return;

        /* yes */
        track_header = &file_loaded[track_offset];

        id_offset = 0x018 + (id_index<<3);

        id->C = track_header[id_offset + 0];
        id->H = track_header[id_offset + 1];
        id->R = track_header[id_offset + 2];
        id->N = track_header[id_offset + 3];
}


void amstrad_get_sector_ptr_callback(int drive, int sector_index, int side, char **ptr)
{
        int track_offset;
        int sector_offset;
        int track;

        track = amstrad_drive_current_track[(drive & 0x01)];

        /* offset to track header in image */
        track_offset = amstrad_dsk_track_offset[(track<<1) + side];

        *ptr = 0;

        /* track exists? */
        if (track_offset==0)
                return;


        /* setup sector offsets */
        switch (amstrad_disk_image_type)
        {
                case 0:
                {
                        amstrad_dsk_init_sector_offsets(track, side);
                }
                break;


                case 1:
                {

                        amstrad_extended_dsk_init_sector_offsets(track, side);
                }
                break;

                default:
                        break;
        }

        sector_offset = amstrad_dsk_sector_offset[sector_index];

        *ptr = (char *)&file_loaded[track_offset + sector_offset];
}

int    amstrad_get_sectors_per_track(int drive, int side)
{
        int track_offset;
        unsigned char *track_header;

        /* get offset to track header in image */
        track_offset = amstrad_dsk_track_offset[(amstrad_drive_current_track[(drive & 0x01)]<<1) + side];

        /* track exists? */
        if (track_offset==0)
                return 0;

        /* yes, get sectors per track */
        track_header = &file_loaded[track_offset];

        return track_header[0x015];
}
