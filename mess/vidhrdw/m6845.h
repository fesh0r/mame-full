
/************************************************************************
	crct6845

	MESS Driver By:

 	Gordon Jefferyes
 	mess_bbc@gjeffery.dircon.co.uk

 ************************************************************************/

struct crtc6845_interface
{
	void (*out_MA_func)(int offset, int data);
	void (*out_RA_func)(int offset, int data);
	void (*out_HS_func)(int offset, int data);
	void (*out_VS_func)(int offset, int data);
	void (*out_DE_func)(int offset, int data);
	void (*out_CR_func)(int offset, int data);
};

/* set up the local copy of the 6845 external procedure calls */
void crtc6845_config(const struct crtc6845_interface *intf);


/* functions to set the 6845 registers */
int crtc6845_register_r(int offset);
void crtc6845_address_w(int offset, int data);
void crtc6845_register_w(int offset, int data);


/* clock the 6845 */
void crtc6845_clock(void);


/* functions to read the 6845 outputs */
int crtc6845_memory_address_r(int offset);
int crtc6845_row_address_r(int offset);
int crtc6845_horizontal_sync_r(int offset);
int crtc6845_vertical_sync_r(int offset);
int crtc6845_display_enabled_r(int offset);
int crtc6845_cursor_enabled_r(int offset);

int crtc6845_cycles_to_vsync(void);
int crtc6845_vsync_length_in_cycles(void);

