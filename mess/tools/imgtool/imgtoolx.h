/***************************************************************************

	imgtool.h

	Internal headers for Imgtool core

***************************************************************************/

#ifndef IMGTOOLX_H
#define IMGTOOLX_H

#include "imgtool.h"

/* ----------------------------------------------------------------------- */

#define IMAGEMODULE_EXTERN(name)	extern void construct_imgmod_##name(struct ImageModule *imgmod)
#define IMAGEMODULE_DECL(name)		(construct_imgmod_##name)

/* Use IMAGEMODULE for (potentially) full featured images */
#define IMAGEMODULE(name_,humanname_,ext_,crcfile_,crcsysname_,eoln_,flags_,\
		init_,exit_,info_,beginenum_,nextenum_,closeenum_,freespace_,readfile_,writefile_, \
		deletefile_,create_,read_sector_,write_sector_,fileoptions_template_,createoptions_template_)	\
																		\
	void construct_imgmod_##name_(struct ImageModule *imgmod)			\
	{																	\
		memset(imgmod, 0, sizeof(*imgmod));								\
		imgmod->name = #name_;											\
		imgmod->humanname = (humanname_);								\
		imgmod->fileextension = (ext_);									\
		imgmod->crcfile = (crcfile_);									\
		imgmod->crcsysname = (crcsysname_);								\
		imgmod->eoln = (eoln_);											\
		imgmod->flags = (flags_);										\
		imgmod->init = (init_);											\
		imgmod->exit = (exit_);											\
		imgmod->info = (info_);											\
		imgmod->begin_enum = (beginenum_);								\
		imgmod->next_enum = (nextenum_);								\
		imgmod->close_enum = (closeenum_);								\
		imgmod->free_space = (freespace_);								\
		imgmod->read_file = (readfile_);								\
		imgmod->write_file = (writefile_);								\
		imgmod->delete_file = (deletefile_);							\
		imgmod->create = (create_);										\
		imgmod->read_sector = (read_sector_);							\
		imgmod->write_sector = (write_sector_);							\
		if (fileoptions_template_)										\
			copy_option_template(										\
				imgmod->fileoptions_template,							\
				sizeof(imgmod->fileoptions_template) / sizeof(imgmod->fileoptions_template[0]),	\
				(fileoptions_template_));								\
		if (createoptions_template_)									\
			copy_option_template(										\
				imgmod->createoptions_template,							\
				sizeof(imgmod->createoptions_template) / sizeof(imgmod->createoptions_template[0]),	\
				(createoptions_template_));								\
	}

/* Use CARTMODULE for cartriges (where the only relevant option is CRC checking */
#define CARTMODULE(name_,humanname_,ext_)	\
																		\
	void construct_imgmod_##name_(struct ImageModule *imgmod)			\
	{																	\
		memset(imgmod, 0, sizeof(*imgmod));								\
		imgmod->name = #name_;											\
		imgmod->humanname = (humanname_);								\
		imgmod->fileextension = (ext_);									\
		imgmod->crcfile = (#name_ ".crc");								\
		imgmod->crcsysname = (#name_);									\
	}

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
	static struct WaveExtra waveextra_##name =							\
	{																	\
		(initalt),														\
		(nextfile),														\
		(readfilechunk),												\
		(zeropulse),													\
		(onepulse),														\
		(threshpulse),													\
		(waveflags),													\
		(blockheader),													\
		(blockheadersize),												\
	};																	\
																		\
	void construct_imgmod_##name_(struct ImageModule *imgmod)			\
	{																	\
		memset(imgmod, 0, sizeof(*imgmod));								\
		imgmod->name = #name_;											\
		imgmod->humanname = (humanname_);								\
		imgmod->fileextension = (ext_);									\
		imgmod->eoln = (eoln_);											\
		imgmod->flags = (flags_);										\
		imgmod->init = imgwave_init;									\
		imgmod->exit = imgwave_exit;									\
		imgmod->begin_enum = imgwave_beginenum;							\
		imgmod->next_enum = imgwave_nextenum;							\
		imgmod->close_enum = imgwave_closeenum;							\
		imgmod->read_file = imgwave_readfile;							\
		imgmod->extra = (void *) &waveextra_##name;						\
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

/* ---------------------------------------------------------------------------
 * Bridge into BDF code
 * ---------------------------------------------------------------------------
 */

#define FLOPPYMODULE_BEGIN(name_)												\
	void int_construct_imgmod_##name_(struct ImageModule *imgmod, int *fileopt);\
	void construct_imgmod_##name_(struct ImageModule *imgmod)					\
	{																			\
		int fileopt = 0;														\
		memset(imgmod, 0, sizeof(*imgmod));										\
		imgmod->name = #name_;													\
		imgmod->init = imgtool_bdf_open;										\
		imgmod->exit = imgtool_bdf_close;										\
		imgmod->create = imgtool_bdf_create;									\
		imgmod->get_geometry = imgtool_bdf_get_geometry;						\
		imgmod->read_sector = imgtool_bdf_read_sector;							\
		imgmod->write_sector = imgtool_bdf_write_sector;						\
		int_construct_imgmod_##name_(imgmod, &fileopt);							\
	}																			\
	void int_construct_imgmod_##name_(struct ImageModule *imgmod, int *fileopt)	\
	{																			\

#define FLOPPYMODULE_END														\
	}

#define FMOD_HUMANNAME(humanname_)		imgmod->humanname = (humanname_);
#define FMOD_CRCFILE(crcfile_)			imgmod->crcfile = (crcfile_);
#define FMOD_CRCSYSFILE(crcsysname_)	imgmod->crcsysname = (crcsysname_);
#define FMOD_EOLN(eoln_)				imgmod->eoln = (eoln_);
#define FMOD_FLAGS(flags_)				imgmod->flags = (flags_);
#define FMOD_INFO(info_)				imgmod->info = (info_);
#define FMOD_FREESPACE(freespace_)		imgmod->free_space = (freespace_);
#define FMOD_READFILE(readfile_)		imgmod->read_file = (readfile_);
#define FMOD_WRITEFILE(writefile_)		imgmod->write_file = (writefile_);
#define FMOD_DELETEFILE(deletefile_)	imgmod->delete_file = (deletefile_);

#define FMOD_ENUMERATE(beginenum_, nextenum_, closeenum_)										\
		imgmod->begin_enum = (beginenum_);														\
		imgmod->next_enum = (nextenum_);															\
		imgmod->close_enum = (closeenum_);

#define FMOD_FILEOPTION(name_, description_, flags_, min_, max_, defaultvalue_)					\
		imgmod->fileoptions_template[*fileopt].name = (name_);									\
		imgmod->fileoptions_template[*fileopt].description = (description_);					\
		imgmod->fileoptions_template[*fileopt].flags = (flags_);								\
		imgmod->fileoptions_template[*fileopt].min = (min_);									\
		imgmod->fileoptions_template[*fileopt].max = (max_);									\
		imgmod->fileoptions_template[*fileopt].defaultvalue = (defaultvalue_);					\
		(*fileopt)++;																			\

#define FMOD_FORMAT(format_name)																\
		imgmod->extra = (void *) construct_formatdriver_##format_name;							\
		memset(imgmod->createoptions_template, 0, sizeof(imgmod->createoptions_template));		\
		imgtool_bdf_getcreateoptions(imgmod->createoptions_template,							\
			sizeof(imgmod->createoptions_template) / sizeof(imgmod->createoptions_template[0]),	\
			construct_formatdriver_##format_name);												\

#define FMOD_IMPORT_FROM(name_)																	\
		int_construct_imgmod_##name_(imgmod, fileopt);											\

int imgtool_bdf_open(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
void imgtool_bdf_close(IMAGE *img);
int imgtool_bdf_create(const struct ImageModule *mod, STREAM *f, const ResolvedOption *createoptions);
int imgtool_bdf_is_readonly(IMAGE *img);
const struct disk_geometry *imgtool_bdf_get_geometry(IMAGE *img);
int imgtool_bdf_read_sector(IMAGE *img, UINT8 track, UINT8 head, UINT8 sector, int offset, void *buffer, int size);
int imgtool_bdf_write_sector(IMAGE *img, UINT8 track, UINT8 head, UINT8 sector, int offset, const void *buffer, int size);
int imgtool_bdf_read_sector_to_stream(IMAGE *img, UINT8 track, UINT8 head, UINT8 sector, int offset, int length, STREAM *s);
int imgtool_bdf_write_sector_from_stream(IMAGE *img, UINT8 track, UINT8 head, UINT8 sector, int offset, int length, STREAM *s);
void imgtool_bdf_getcreateoptions(struct OptionTemplate *opts, size_t max_opts, formatdriver_ctor format);

#endif /* IMGTOOLX_H */

