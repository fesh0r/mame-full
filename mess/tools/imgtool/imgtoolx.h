/***************************************************************************

	imgtoolx.h

	Internal headers for Imgtool core

***************************************************************************/

#ifndef IMGTOOLX_H
#define IMGTOOLX_H

#include "imgtool.h"

/* ----------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Wave/Cassette calls
 * ---------------------------------------------------------------------------
 */

enum {
	WAVEIMAGE_LSB_FIRST = 0,
	WAVEIMAGE_MSB_FIRST = 1
};

struct WaveExtra
{
	int (*initalt)(STREAM *instream, STREAM **outstream, int *basepos, int *length, int *channels, int *frequency, int *resolution);
	int (*nextfile)(IMAGE *img, imgtool_dirent *ent);
	int (*readfile)(IMAGE *img, STREAM *destf);
	int zeropulse;
	int threshpulse;
	int onepulse;
	int waveflags;
	const UINT8 *blockheader;
	int blockheadersize;

};

#define WAVEMODULE(name_,humanname_,ext_,eoln_,flags_,zeropulse,onepulse,threshpulse,waveflags,blockheader,blockheadersize,\
		initalt,nextfile,readfilechunk)	\
	static struct WaveExtra waveextra_##name =								\
	{																		\
		(initalt),															\
		(nextfile),															\
		(readfilechunk),													\
		(zeropulse),														\
		(onepulse),															\
		(threshpulse),														\
		(waveflags),														\
		(blockheader),														\
		(blockheadersize),													\
	};																		\
																			\
	int construct_imgmod_##name_(struct ImageModuleCtorParams *params)		\
	{																		\
		struct ImageModule *imgmod = params->imgmod;						\
		memset(imgmod, 0, sizeof(*imgmod));									\
		imgmod->name = #name_;												\
		imgmod->humanname = (humanname_);									\
		imgmod->fileextension = (ext_);										\
		imgmod->eoln = (eoln_);												\
		imgmod->flags = (flags_);											\
		imgmod->init = imgwave_init;										\
		imgmod->exit = imgwave_exit;										\
		imgmod->begin_enum = imgwave_beginenum;								\
		imgmod->next_enum = imgwave_nextenum;								\
		imgmod->close_enum = imgwave_closeenum;								\
		imgmod->read_file = imgwave_readfile;								\
		imgmod->extra = (void *) &waveextra_##name;							\
		return 1;															\
	}

/* These are called internally */
int imgwave_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
void imgwave_exit(IMAGE *img);
int imgwave_beginenum(IMAGE *img, IMAGEENUM **outenum);
int imgwave_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
void imgwave_closeenum(IMAGEENUM *enumeration);
int imgwave_readfile(IMAGE *img, const char *fname, STREAM *destf);

/* These are callable from wave modules */
int imgwave_seek(IMAGE *img, int pos);
int imgwave_forward(IMAGE *img);
int imgwave_read(IMAGE *img, UINT8 *buf, int bufsize);


#endif /* IMGTOOLX_H */

