/******************************************************************************

	compis.h
	machine driver header

	Per Ola Ingvarsson
	Tomas Karlsson
		
 ******************************************************************************/

extern DRIVER_INIT(compis);
extern MACHINE_INIT(compis);
extern MACHINE_STOP(compis);
extern INTERRUPT_GEN(compis_vblank_int);

/* PPI 8255 */
READ_HANDLER (compis_ppi_r);
WRITE_HANDLER (compis_ppi_w);

/* PIT 8253 */
READ_HANDLER (compis_pit_r);
WRITE_HANDLER (compis_pit_w);

/* PIC 8259 (80150/80130) */
READ_HANDLER (compis_osp_pic_r);
WRITE_HANDLER (compis_osp_pic_w);

/* PIT 8254 (80150/80130) */
READ_HANDLER (compis_osp_pit_r);
WRITE_HANDLER (compis_osp_pit_w);

/* USART 8251 */
READ_HANDLER (compis_usart_r);
WRITE_HANDLER (compis_usart_w);

/* 80186 Internal */
READ_HANDLER (i186_internal_port_r);
WRITE_HANDLER (i186_internal_port_w);

/* FDC 8272 */
READ_HANDLER (compis_fdc_dack_r);
READ_HANDLER (compis_fdc_r);
WRITE_HANDLER (compis_fdc_w);
DEVICE_LOAD (compis_floppy);

/* RTC 58174 */
READ_HANDLER (compis_rtc_r);
WRITE_HANDLER (compis_rtc_w);
