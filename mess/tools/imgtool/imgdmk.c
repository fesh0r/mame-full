#include <assert.h>
#include <string.h>
#include "imgtool.h"
#include "formats/dmkdsk.h"
#include "imgdmk.h"

typedef struct {
	IMAGE base;
	STREAM *f;
	dmkHeader head;
} imgfloppy_dmkdsk;

static int imgfloppy_dmkdsk_seek(imgfloppy_dmkdsk *fimg, UINT8 head, UINT8 track, UINT8 sector, int offset);

int imgfloppy_dmkdsk_init(const struct ImageModule *mod, const struct FloppyFormat *flopformat, STREAM *f, IMAGE **outimg)
{
	int err;
	imgfloppy_dmkdsk *img;
	size_t	image_size, calc_size;

	img = malloc(sizeof(imgfloppy_dmkdsk));
	if (!img) {
		err = IMGTOOLERR_OUTOFMEMORY;
		goto error;
	}
	
	memset(img, 0, sizeof(imgfloppy_dmkdsk));

	img->base.module = mod;
	img->f = f;
	*outimg = &img->base;
	
	err = stream_seek(f, 0, SEEK_SET);
	if (err) goto error;

	err = stream_read(f, &(img->head), sizeof( dmkHeader ));
	if (err) goto error;

	image_size =  stream_size( f );
	calc_size = dmkdsk_GetTrackLength( &(img->head) ) * img->head.trackCount * (DMKSIDECOUNT( (img->head) )) + sizeof( dmkHeader );

	if (  calc_size == image_size )
	{
		if ( dmkdsk_GetRealDiskCode( &(img->head) ) == 0x12345678 )  /* real disk files unsupported */
		{
			err = IMGTOOLERR_CORRUPTIMAGE;
			goto error;
		}
	}
	else
	{
		err = IMGTOOLERR_CORRUPTIMAGE;
		goto error;
	}

	return 0;

error:
	if (img)
		free(img);
	*outimg = NULL;
	return err;
}

void imgfloppy_dmkdsk_exit(IMAGE *img)
{
	imgfloppy_dmkdsk *fimg;

	fimg = (imgfloppy_dmkdsk *) img;
	stream_close(fimg->f);
	free(fimg);
}

static int imgfloppy_dmkdsk_seek(imgfloppy_dmkdsk *fimg, UINT8 head, UINT8 track, UINT8 sector, int offset)
{
	size_t		track_start;
	dmkTrack	track_data;
	int			err, i, j;
	dmkIDAM		idam;
	
	track_start = sizeof( dmkHeader ) + (track * dmkdsk_GetTrackLength( &(fimg->head) )) + (dmkdsk_GetTrackLength( &(fimg->head) ) * head);
	stream_seek(fimg->f, track_start, SEEK_SET);

	err = stream_read(fimg->f, &track_data, sizeof( dmkTrack ));
	if (err) return err;

	/* Loop thru IDAM pointers until the sector is found */
	
	for( i = 0; i < 64; i++ )
	{
		int disp;
		
		disp = LITTLE_ENDIANIZE_INT16(track_data.idamOffset[ i ]);
		disp &= 0x3fff;

		if( disp == 0 )
		{
			/* sector not found */
			return IMGTOOLERR_READERROR;
		}

		stream_seek(fimg->f, track_start + disp, SEEK_SET);
		
		err = stream_read(fimg->f, &idam, sizeof( dmkIDAM ));
		if (err) return err;
		
		if( idam.sectorNumber == sector )
		{
			/* Hey we found the correct sector */
			/* Now find the start of the track */
			
			for( j = 8; j<80; j++ )
			{
				char		buf[4];
				
				stream_seek(fimg->f, track_start + disp + j, SEEK_SET);
				err = stream_read(fimg->f, buf, 3);
				if (err) return err;

				if ( memcmp( buf, "\241\241\373", 3 ) == 0 )
				{
					//* We found the DAM */
					break;
				}
				
				if( j == 80 )
				{
					/* No DAM found */
					return IMGTOOLERR_READERROR;
				}
			}
		}
	}

	stream_seek(fimg->f, offset, SEEK_CUR);
	
	return 0;
}

int imgfloppy_dmkdsk_readdata(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, int length, UINT8 *buf)
{
	int err;
	imgfloppy_dmkdsk *fimg;

	fimg = (imgfloppy_dmkdsk *) img;
	err = imgfloppy_dmkdsk_seek(fimg, head, track, sector, offset);
	if (err)
		return err;

	if (stream_read(fimg->f, buf, length) != length)
		return IMGTOOLERR_READERROR;

	return 0;
}

int imgfloppy_dmkdsk_writedata(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, int length, const UINT8 *buf)
{
	int err;
	imgfloppy_dmkdsk *fimg;

	fimg = (imgfloppy_dmkdsk *) img;
	err = imgfloppy_dmkdsk_seek(fimg, head, track, sector, offset);
	if (err)
		return err;

	if (stream_write(fimg->f, buf, length) != length)
		return IMGTOOLERR_WRITEERROR;

	/* Need to update CRC here */
	
	return 0;
}

int imgfloppy_dmkdsk_create(const struct ImageModule *mod, const struct FloppyFormat *flopformat, STREAM *f, UINT8 sides, UINT8 tracks, UINT8 filler)
{
	return IMGTOOLERR_UNIMPLEMENTED;
}

void imgfloppy_dmkdsk_get_geometry(IMAGE *img, UINT8 *sides, UINT8 *tracks, UINT8 *sectors)
{
	imgfloppy_dmkdsk *fimg;

	fimg = (imgfloppy_dmkdsk *) img;
	*sides = ( (fimg->head.diskOptions & 0x10) == 0 ) ? 1 : 2;
	*tracks = fimg->head.trackCount;
	/* I guess I should count up the found sectors instead */
	*sectors = 18;
}

int imgfloppy_dmkdsk_isreadonly(IMAGE *img)
{
	/* Need to implement honoring write protect */
	imgfloppy_dmkdsk *fimg;
	fimg = (imgfloppy_dmkdsk *) img;
	return stream_isreadonly(fimg->f);
}

