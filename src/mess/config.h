#ifndef _CONFIG_H
#define _CONFIG_H

/* Amount of memory to allocate while reading variable size arrays */
/* Tradeoff between calling realloc() all the time and wasting memory */
#define CHUNK_SIZE	512

/* Close a config file; free temporary memory at the same time */
void config_close(void *config);

/* Create a new config file; name may include the path. */
void *config_create(const char *name);

/* Open an existing config file */
void *config_open(const char *name);

/* Save data of various element size and signedness */
void config_save_string(void *config, const char *module,int instance,
	const char *name, const char *src);
void config_save_UINT8(void *config, const char *module,int instance,
    const char *name, const UINT8 *val, unsigned size);
void config_save_INT8(void *config, const char *module,int instance,
	const char *name, const INT8 *val, unsigned size);
void config_save_UINT16(void *config, const char *module,int instance,
	const char *name, const UINT16 *val, unsigned size);
void config_save_INT16(void *config, const char *module,int instance,
	const char *name, const INT16 *val, unsigned size);
void config_save_UINT32(void *config, const char *module,int instance,
	const char *name, const UINT32 *val, unsigned size);
void config_save_INT32(void *config, const char *module,int instance,
	const char *name, const INT32 *val, unsigned size);

/* Load data of various element size and signedness */
void config_load_string(void *config, const char *module,int instance,
	const char *name, char *dst, unsigned size);
void config_load_UINT8(void *config, const char *module,int instance,
	const char *name, UINT8 *val, unsigned size);
void config_load_INT8(void *config, const char *module,int instance,
	const char *name, INT8 *val, unsigned size);
void config_load_UINT16(void *config, const char *module,int instance,
	const char *name, UINT16 *val, unsigned size);
void config_load_INT16(void *config, const char *module,int instance,
	const char *name, INT16 *val, unsigned size);
void config_load_UINT32(void *config, const char *module,int instance,
	const char *name, UINT32 *val, unsigned size);
void config_load_INT32(void *config, const char *module,int instance,
	const char *name, INT32 *val, unsigned size);

#endif

