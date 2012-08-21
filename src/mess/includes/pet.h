/***************************************************************************
    commodore pet series computer

    peter.trauner@jk.uni-linz.ac.at
***************************************************************************/

#ifndef PET_H_
#define PET_H_

#include "video/mc6845.h"
#include "machine/6821pia.h"
#include "machine/6522via.h"
#include "machine/ieee488.h"
#include "imagedev/cartslot.h"

typedef struct
{
	int bank; /* rambank to be switched in 0x9000 */
	int rom; /* rom socket 6502? at 0x9000 */
} spet_t;


class pet_state : public driver_device
{
public:
	pet_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_ieee(*this, IEEE488_TAG),
		m_videoram(*this, "videoram"),
		m_memory(*this, "memory"){ }

	required_device<ieee488_device> m_ieee;

	int m_pet_basic1; /* basic version 1 for quickloader */
	int m_superpet;
	int m_cbm8096;

	int m_pia0_irq;
	int m_pia1_irq;
	int m_via_irq;
	optional_shared_ptr<UINT8> m_videoram;
	int m_font;
	optional_shared_ptr<UINT8> m_memory;
	UINT8 *m_supermemory;
	UINT8 *m_pet80_bank1_base;
	int m_keyline_select;
	emu_timer *m_datasette1_timer;
	emu_timer *m_datasette2_timer;
	spet_t m_spet;
	int m_pia_level;
	DECLARE_DRIVER_INIT(superpet);
	DECLARE_DRIVER_INIT(pet80);
	DECLARE_DRIVER_INIT(pet);
	DECLARE_DRIVER_INIT(pet2001);
};

/*----------- defined in video/pet.c -----------*/

/* call to init videodriver */
void pet_vh_init (running_machine &machine);
void pet80_vh_init (running_machine &machine);
void superpet_vh_init (running_machine &machine);
SCREEN_UPDATE_IND16( pet );
MC6845_UPDATE_ROW( pet40_update_row );
MC6845_UPDATE_ROW( pet80_update_row );
WRITE_LINE_DEVICE_HANDLER( pet_display_enable_changed );


/*----------- defined in machine/pet.c -----------*/

extern const via6522_interface pet_via;
extern const pia6821_interface pet_pia0;
extern const pia6821_interface petb_pia0;
extern const pia6821_interface pet_pia1;


WRITE8_HANDLER(cbm8096_w);
extern READ8_HANDLER(superpet_r);
extern WRITE8_HANDLER(superpet_w);

MACHINE_RESET( pet );
INTERRUPT_GEN( pet_frame_interrupt );

MACHINE_CONFIG_EXTERN( pet_cartslot );
MACHINE_CONFIG_EXTERN( pet4_cartslot );

#endif /* PET_H_ */
