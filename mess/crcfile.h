#ifndef CRCFILE_H
#define CRCFILE_H

typedef struct _crc_file crc_file;

/* Create a new config file; name may include the path. */
crc_file *crcfile_create(const char *gamename, const char *filename, int filetype);

/* Open an existing config file */
crc_file *crcfile_open(const char *gamename, const char *filename, int filetype);

/* Close a config file; free temporary memory at the same time */
void crcfile_close(crc_file *config);

/* Save data of various element size and signedness */
void crcfile_save_string(crc_file *config, const char *module,int instance,
	const char *name, const char *src);
void crcfile_save_UINT8(crc_file *config, const char *module,int instance,
    const char *name, const UINT8 *val, unsigned size);
void crcfile_save_INT8(crc_file *config, const char *module,int instance,
	const char *name, const INT8 *val, unsigned size);
void crcfile_save_UINT16(crc_file *config, const char *module,int instance,
	const char *name, const UINT16 *val, unsigned size);
void crcfile_save_INT16(crc_file *config, const char *module,int instance,
	const char *name, const INT16 *val, unsigned size);
void crcfile_save_UINT32(crc_file *config, const char *module,int instance,
	const char *name, const UINT32 *val, unsigned size);
void crcfile_save_INT32(crc_file *config, const char *module,int instance,
	const char *name, const INT32 *val, unsigned size);

/* Load data of various element size and signedness */
void crcfile_load_string(crc_file *config, const char *module,int instance,
	const char *name, char *dst, unsigned size);
void crcfile_load_UINT8(crc_file *config, const char *module,int instance,
	const char *name, UINT8 *val, unsigned size);
void crcfile_load_INT8(crc_file *config, const char *module,int instance,
	const char *name, INT8 *val, unsigned size);
void crcfile_load_UINT16(crc_file *config, const char *module,int instance,
	const char *name, UINT16 *val, unsigned size);
void crcfile_load_INT16(crc_file *config, const char *module,int instance,
	const char *name, INT16 *val, unsigned size);
void crcfile_load_UINT32(crc_file *config, const char *module,int instance,
	const char *name, UINT32 *val, unsigned size);
void crcfile_load_INT32(crc_file *config, const char *module,int instance,
	const char *name, INT32 *val, unsigned size);

#endif

