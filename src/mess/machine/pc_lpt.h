/***************************************************************************

    IBM-PC printer interface

***************************************************************************/

#ifndef __PC_LPT_H__
#define __PC_LPT_H__

#include "isa.h"
#include "devcb.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _pc_lpt_interface pc_lpt_interface;
struct _pc_lpt_interface
{
	devcb_write_line out_irq_func;
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/
READ8_DEVICE_HANDLER( pc_lpt_r );
WRITE8_DEVICE_HANDLER( pc_lpt_w );

READ8_DEVICE_HANDLER( pc_lpt_data_r );
WRITE8_DEVICE_HANDLER( pc_lpt_data_w );
READ8_DEVICE_HANDLER( pc_lpt_status_r );
READ8_DEVICE_HANDLER( pc_lpt_control_r );
WRITE8_DEVICE_HANDLER( pc_lpt_control_w );


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(PC_LPT, pc_lpt);

#define MCFG_PC_LPT_ADD(_tag, _intf) \
	MCFG_DEVICE_ADD(_tag, PC_LPT, 0) \
	MCFG_DEVICE_CONFIG(_intf)

#define MCFG_PC_LPT_REMOVE(_tag) \
	MCFG_DEVICE_REMOVE(_tag)


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> isa8_lpt_device

class isa8_lpt_device :
		public device_t,
		public device_isa8_card_interface
{
public:
        // construction/destruction
        isa8_lpt_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

		// optional information overrides
		virtual machine_config_constructor device_mconfig_additions() const;
		virtual ioport_constructor device_input_ports() const;

		bool is_primary() { return m_is_primary; }
protected:
        // device-level overrides
        virtual void device_start();
        virtual void device_reset();
		virtual void device_config_complete() { m_shortname = "isa_lpt"; }
private:
        // internal state
		bool m_is_primary;
};


// device type definition
extern const device_type ISA8_LPT;

#endif /* __PC_LPT__ */
