#ifndef _CONFIG_H
#define _CONFIG_H

typedef struct _config_file config_file;

/* Create a new config file; name may include the path. */
config_file *config_create(const char *gamename, const char *filename, int filetype);

/* Open an existing config file */
config_file *config_open(const char *gamename, const char *filename, int filetype);

/* Close a config file; free temporary memory at the same time */
void config_close(config_file *config);

/* Save data of various element size and signedness */
void config_save_string(config_file *config, const char *module,int instance,
	const char *name, const char *src);
void config_save_UINT8(config_file *config, const char *module,int instance,
    const char *name, const UINT8 *val, unsigned size);
void config_save_INT8(config_file *config, const char *module,int instance,
	const char *name, const INT8 *val, unsigned size);
void config_save_UINT16(config_file *config, const char *module,int instance,
	const char *name, const UINT16 *val, unsigned size);
void config_save_INT16(config_file *config, const char *module,int instance,
	const char *name, const INT16 *val, unsigned size);
void config_save_UINT32(config_file *config, const char *module,int instance,
	const char *name, const UINT32 *val, unsigned size);
void config_save_INT32(config_file *config, const char *module,int instance,
	const char *name, const INT32 *val, unsigned size);

/* Load data of various element size and signedness */
void config_load_string(config_file *config, const char *module,int instance,
	const char *name, char *dst, unsigned size);
void config_load_UINT8(config_file *config, const char *module,int instance,
	const char *name, UINT8 *val, unsigned size);
void config_load_INT8(config_file *config, const char *module,int instance,
	const char *name, INT8 *val, unsigned size);
void config_load_UINT16(config_file *config, const char *module,int instance,
	const char *name, UINT16 *val, unsigned size);
void config_load_INT16(config_file *config, const char *module,int instance,
	const char *name, INT16 *val, unsigned size);
void config_load_UINT32(config_file *config, const char *module,int instance,
	const char *name, UINT32 *val, unsigned size);
void config_load_INT32(config_file *config, const char *module,int instance,
	const char *name, INT32 *val, unsigned size);

#endif

