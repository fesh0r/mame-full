#ifndef PRINTER_H_INCLUDED
#define PRINTER_H_INCLUDED

#define MAX_PRINTER	(4)

#ifdef __cplusplus
extern "C" {
#endif

/*
 * functions for the IODevice entry IO_PRINTER
 * 
 * Currently only a simple port which only supports status 
 * ("online/offline") and output of bytes is supported.
 */

extern int printer_init (int id);
extern void printer_exit (int id);
extern int printer_status (int id, int newstatus);
extern void printer_output (int id, int data);
extern int printer_output_chunk (int id, void *src, int chunks);

#define IO_PRINTER_PORT(count,fileext)					\
{														\
	IO_PRINTER,					/* type */				\
	count,						/* count */				\
	fileext,					/* file extensions */	\
	IO_RESET_NONE,				/* reset depth */		\
	NULL,						/* id */				\
	printer_init,				/* init */				\
	printer_exit,				/* exit */				\
	NULL,						/* info */				\
	NULL,						/* open */				\
	NULL,						/* close */				\
	printer_status,				/* status */			\
	NULL,						/* seek */				\
	NULL,						/* tell */				\
	NULL,						/* input */				\
	printer_output,				/* output */			\
	NULL,						/* input chunk */		\
	printer_output_chunk,		/* output chunk */		\
}

#ifdef __cplusplus
}
#endif

#endif /* PRINTER_H_INCLUDED */
