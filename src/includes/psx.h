/***************************************************************************

  includes/psx.h

***************************************************************************/

/* vidhrdw */
READ32_HANDLER( psx_gpu_r );
WRITE32_HANDLER( psx_gpu_w );

PALETTE_INIT( psx );
VIDEO_START( psx_type1_1024x1024 );
VIDEO_START( psx_type2_1024x512 );
VIDEO_START( psx_type2_1024x1024 );
VIDEO_UPDATE( psx );
VIDEO_STOP( psx );
INTERRUPT_GEN( psx_vblank );
void psx_gpu_reset( void );

/* machine */
WRITE32_HANDLER( psx_irq_w );
READ32_HANDLER( psx_irq_r );
void psx_irq_set( UINT32 );
WRITE32_HANDLER( psx_sio_w );
READ32_HANDLER( psx_sio_r );
WRITE32_HANDLER( psx_mdec_w );
READ32_HANDLER( psx_mdec_r );
WRITE32_HANDLER( psx_dma_w );
READ32_HANDLER( psx_dma_r );
WRITE32_HANDLER( psx_counter_w );
READ32_HANDLER( psx_counter_r );
void psx_machine_init( void );
void psx_driver_init( void );
