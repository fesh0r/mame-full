/**********************************************************************

  Copyright (C) Antoine Mine' 2008

   Hewlett Packard HP48 S/SX & G/GX

**********************************************************************/

#ifdef CHARDEV
#include "devices/chardev.h"
#endif

/* model */
typedef enum {
	HP48_S,
	HP48_SX,
	HP48_G,
	HP48_GX,
	HP48_GP,
} hp48_models;

/* memory module configuration */
typedef struct
{
	/* static part */
	UINT32 off_mask;             /* offset bit-mask, indicates the real size */
	read8_space_func read;
	const char *read_name;
	write8_space_func write;
	const char *write_name;
	void* data;                  /* non-NULL for banks */
	int isnop;

	/* configurable part */
	UINT8  state;                /* one of HP48_MODULE_ */
	UINT32 base;                 /* base address */
	UINT32 mask;                 /* often improperly called size, it is an address select mask */

} hp48_module;


/* screen image averaging */
#define HP48_NB_SCREENS 3

class hp48_state : public driver_device
{
public:
	hp48_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	UINT8 *m_videoram;
	UINT8 m_io[64];
	hp48_models m_model;
	UINT16 m_out;
	UINT8 m_kdn;
	hp48_module m_modules[6];
	UINT32 m_port_size[2];
	UINT8 m_port_write[2];
	UINT8* m_port_data[2];
	UINT32 m_bank_switch;
	UINT32 m_io_addr;
	UINT16 m_crc;
	UINT8 m_timer1;
	UINT32 m_timer2;
#ifdef CHARDEV
	chardev* m_chardev;
#endif
	UINT8 m_screens[ HP48_NB_SCREENS ][ 64 ][ 144 ];
	int m_cur_screen;
	DECLARE_DRIVER_INIT(hp48);
};


/***************************************************************************
    MACROS
***************************************************************************/

/* read from I/O memory */
#define HP48_IO_4(x)   (state->m_io[(x)])
#define HP48_IO_8(x)   (state->m_io[(x)] | (state->m_io[(x)+1] << 4))
#define HP48_IO_12(x)  (state->m_io[(x)] | (state->m_io[(x)+1] << 4) | (state->m_io[(x)+2] << 8))
#define HP48_IO_20(x)  (state->m_io[(x)] | (state->m_io[(x)+1] << 4) | (state->m_io[(x)+2] << 8) | \
	               (state->m_io[(x)+3] << 12) | (state->m_io[(x)+4] << 16))


/*----------- defined in machine/hp48.c -----------*/

/***************************************************************************
    GLOBAL VARIABLES & CONSTANTS
***************************************************************************/

/* I/O memory */



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/


/************************ Saturn's I/O *******************************/

/* memory controller */
void hp48_mem_reset( device_t *device );
void hp48_mem_config( device_t *device, int v );
void hp48_mem_unconfig( device_t *device, int v );
int  hp48_mem_id( device_t *device );

/* CRC computation */
void hp48_mem_crc( device_t *device, int addr, int data );

/* IN/OUT registers */
int  hp48_reg_in( device_t *device );
void hp48_reg_out( device_t *device, int v );

/* keybord interrupt system */
void hp48_rsi( device_t *device );


/***************************** serial ********************************/

extern void hp48_rs232_start_recv_byte( running_machine &machine, UINT8 data );


/****************************** cards ********************************/

/* port specification */
struct hp48_port_interface
{
	int port;                 /* port index: 0 or 1 (for port 1 and 2) */
	int module;               /* memory module where the port is visible */
	int max_size;             /* maximum size, in bytes 128 KB or 4 GB */
};

class hp48_port_image_device :	public device_t,
								public device_image_interface,
								public hp48_port_interface
{
public:
	// construction/destruction
	hp48_port_image_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// image-level overrides
	virtual iodevice_t image_type() const { return IO_MEMCARD; }

	virtual bool is_readable()  const { return 1; }
	virtual bool is_writeable() const { return 1; }
	virtual bool is_creatable() const { return 1; }
	virtual bool must_be_loaded() const { return 0; }
	virtual bool is_reset_on_load() const { return 0; }
	virtual const char *image_interface() const { return NULL; }
	virtual const char *file_extensions() const { return "crd"; }
	virtual const option_guide *create_option_guide() const { return NULL; }

	virtual bool call_load();
	virtual void call_unload();
	virtual bool call_create(int format_type, option_resolution *format_options);
protected:
	// device-level overrides
    virtual void device_config_complete();
	virtual void device_start();
private:
	void hp48_fill_port();
	void hp48_unfill_port();
};

extern const struct hp48_port_interface hp48sx_port1_config;
extern const struct hp48_port_interface hp48sx_port2_config;
extern const struct hp48_port_interface hp48gx_port1_config;
extern const struct hp48_port_interface hp48gx_port2_config;

// device type definition
extern const device_type HP48_PORT;

#define MCFG_HP48_PORT_ADD(_tag, _intrf) \
	MCFG_DEVICE_ADD(_tag, HP48_PORT, 0) \
	MCFG_DEVICE_CONFIG(_intrf)

/****************************** machine ******************************/


extern MACHINE_START( hp48s  );
extern MACHINE_START( hp48sx );
extern MACHINE_START( hp48g  );
extern MACHINE_START( hp48gx );
extern MACHINE_START( hp48gp );

extern MACHINE_RESET( hp48 );

/*----------- defined in video/hp48.c -----------*/

/****************************** video ********************************/

extern SCREEN_UPDATE_IND16 ( hp48 );
extern PALETTE_INIT ( hp48 );
