/*
  defines centronics/parallel port printer interface  

  provides a centronics printer simulation (sends output to IO_PRINTER)
*/
#include "includes/centroni.h"
#include "printer.h"

typedef struct {
	CENTRONICS_CONFIG *config;
	UINT8 data;
	UINT8 control;
	double time_;
} CENTRONICS;

static CENTRONICS cent[3]={
	{ 0,0,0, 0.0 },
	{ 0,0,0, 0.0 },
	{ 0,0,0, 0.0 }
};

void centronics_config(int nr, CENTRONICS_CONFIG *config)
{
	CENTRONICS *This=cent+nr;
	This->config=config;
}

void centronics_write_data(int nr, UINT8 data)
{
	CENTRONICS *This=cent+nr;
	This->data=data;
}

void centronics_write_handshake(int nr, int data, int mask)
{
	CENTRONICS *This=cent+nr;
	
	int neu=(data&mask)|(This->control&~mask);
	
	if (neu&CENTRONICS_NO_RESET) {
		if ( !(This->control&CENTRONICS_STROBE) && (neu&CENTRONICS_STROBE) ) {
			printer_output(nr, This->data);
			// after a while a acknowledge should occur
			This->time_=timer_get_time();
		}
	}
	This->control=neu;
}

int centronics_read_handshake(int nr)
{
	CENTRONICS *This=cent+nr;
	UINT8 data=0;

	data|=CENTRONICS_NOT_BUSY;
	if (This->config->type==PRINTER_IBM) {
		data|=CENTRONICS_ONLINE;
	} else {
		if (This->control&CENTRONICS_SELECT) 
			data|=CENTRONICS_ONLINE;
	}
	data|=CENTRONICS_NO_ERROR;
	if (!printer_status(nr, 0)) data|=CENTRONICS_NO_PAPER;

	if (timer_get_time()-This->time_<10e-6) data|=CENTRONICS_ACKNOWLEDGE;

	return data;
}

CENTRONICS_DEVICE CENTRONICS_PRINTER_DEVICE= {
	NULL,
	centronics_write_data,
	centronics_read_handshake,
	centronics_write_handshake
};

