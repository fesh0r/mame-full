#ifndef _8255PPI_H_
#define _8255PPI_H_

#define MAX_8255 4

/* Note: handler offset == chip number */
typedef struct {
	int num;							 /* number of PPIs to emulate */
	mem_read_handler portA_r;
	mem_read_handler portB_r;
	mem_read_handler portC_r;
	mem_write_handler portA_w;
	mem_write_handler portB_w;
	mem_write_handler portC_w;
} ppi8255_interface;

/* Init */
void ppi8255_init( ppi8255_interface *intfce );

/* Read/Write */
data_t ppi8255_r( int which, offs_t offset );
void ppi8255_w( int which, offs_t offset, data_t data );

/* Peek at the ports */
data_t ppi8255_peek( int which, offs_t offset );

/* Helpers */
READ_HANDLER( ppi8255_0_r );
READ_HANDLER( ppi8255_1_r );
READ_HANDLER( ppi8255_2_r );
READ_HANDLER( ppi8255_3_r );
WRITE_HANDLER( ppi8255_0_w );
WRITE_HANDLER( ppi8255_1_w );
WRITE_HANDLER( ppi8255_2_w );
WRITE_HANDLER( ppi8255_3_w );

#endif	/* _8255_PPI_H */
