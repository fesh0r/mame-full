/*********************************************************************

    kcexp.h

    KC85_2/3/4/5 expansion slot emulation

*********************************************************************/

#ifndef __KCEXP_H__
#define __KCEXP_H__

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

// ======================> kcext_interface

struct kcexp_interface
{
    devcb_write_line				m_out_irq_cb;
    devcb_write_line				m_out_nmi_cb;
	devcb_write_line				m_out_halt_cb;
};


// ======================> device_kcexp_interface

class device_kcexp_interface : public device_slot_card_interface
{
public:
	// construction/destruction
	device_kcexp_interface(const machine_config &mconfig, device_t &device);
	virtual ~device_kcexp_interface();

	// reading and writing
	virtual UINT8 module_id_r() { return 0xff; }
	virtual void control_w(UINT8 data) { }
	virtual void read(offs_t offset, UINT8 &data) { }
	virtual void write(offs_t offset, UINT8 data) { }
	virtual void io_read(offs_t offset, UINT8 &data) { }
	virtual void io_write(offs_t offset, UINT8 data) { }
	virtual UINT8* get_cart_base() { return NULL; }
	virtual DECLARE_WRITE_LINE_MEMBER( mei_w ) { };
};

// ======================> kcexp_slot_device

class kcexp_slot_device : public device_t,
						  public kcexp_interface,
						  public device_slot_interface
{
public:
	// construction/destruction
	kcexp_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	kcexp_slot_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);
	virtual ~kcexp_slot_device();

	// device-level overrides
	virtual void device_start();
	virtual void device_config_complete();

	// inline configuration
	static void static_set_next_slot(device_t &device, const char *next_module_tag);

	// reading and writing
	virtual UINT8 module_id_r();
	virtual void control_w(UINT8 data);
	virtual void read(offs_t offset, UINT8 &data);
	virtual void write(offs_t offset, UINT8 data);
	virtual void io_read(offs_t offset, UINT8 &data);
	virtual void io_write(offs_t offset, UINT8 data);
	virtual DECLARE_WRITE_LINE_MEMBER( mei_w );
	virtual DECLARE_WRITE_LINE_MEMBER( meo_w );

	devcb_resolved_write_line	m_out_irq_func;
	devcb_resolved_write_line	m_out_nmi_func;
	devcb_resolved_write_line	m_out_halt_func;

	device_kcexp_interface*		m_cart;

	const char*					m_next_slot_tag;
	kcexp_slot_device*			m_next_slot;
};

// ======================> kccart_slot_device

class kccart_slot_device : public kcexp_slot_device,
						   public device_image_interface
{
public:
	// construction/destruction
	kccart_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	virtual ~kccart_slot_device();

	// device-level overrides
	virtual void device_config_complete();

	// image-level overrides
	virtual bool call_load();
	virtual bool call_softlist_load(char *swlist, char *swname, rom_entry *start_entry);

	virtual iodevice_t image_type() const { return IO_CARTSLOT; }
	virtual bool is_readable()  const { return 1; }
	virtual bool is_writeable() const { return 0; }
	virtual bool is_creatable() const { return 0; }
	virtual bool must_be_loaded() const { return 0; }
	virtual bool is_reset_on_load() const { return 1; }
	virtual const char *image_interface() const { return "kc_cart"; }
	virtual const char *file_extensions() const { return "bin"; }
	virtual const option_guide *create_option_guide() const { return NULL; }

	// slot interface overrides
	virtual const char * get_default_card_software(const machine_config &config, emu_options &options);
};

// device type definition
extern const device_type KCEXP_SLOT;
extern const device_type KCCART_SLOT;


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MCFG_KC85_EXPANSION_ADD(_tag,_next_slot_tag,_config,_slot_intf,_def_slot,_def_inp) \
	MCFG_DEVICE_ADD(_tag, KCEXP_SLOT, 0) \
	MCFG_DEVICE_CONFIG(_config) \
	MCFG_DEVICE_SLOT_INTERFACE(_slot_intf, _def_slot, _def_inp, false) \
	kcexp_slot_device::static_set_next_slot(*device, _next_slot_tag);

#define MCFG_KC85_CARTRIDGE_ADD(_tag,_next_slot_tag,_config,_slot_intf,_def_slot,_def_inp) \
	MCFG_DEVICE_ADD(_tag, KCCART_SLOT, 0) \
	MCFG_DEVICE_CONFIG(_config) \
	MCFG_DEVICE_SLOT_INTERFACE(_slot_intf, _def_slot, _def_inp, false) \
	kcexp_slot_device::static_set_next_slot(*device, _next_slot_tag);

#endif /* __KCEXP_H__ */
