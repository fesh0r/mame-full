#ifdef __cplusplus
extern "C" {
#endif

extern WRITE_HANDLER ( dma8237_0_w );
extern WRITE_HANDLER ( dma8237_1_w );

extern READ_HANDLER ( dma8237_0_r );
extern READ_HANDLER ( dma8237_1_r );

// dma chip a0..a15 connected to a1..a16
extern WRITE_HANDLER ( dma8237_at_0_w );
extern WRITE_HANDLER ( dma8237_at_1_w );
extern READ_HANDLER ( dma8237_at_0_r );
extern READ_HANDLER ( dma8237_at_1_r );

// page register normally latch
// pc/xt 
// ls670 4x4 bit ram
// dack2 readb, dack3 reada
WRITE_HANDLER ( pc_page_w );
WRITE_HANDLER ( at_page_w );
extern READ_HANDLER ( pc_page_r );
extern READ_HANDLER ( at_page_r );


typedef enum { DMA8237_PC, DMA8237_AT } DMA8237_TYPE;

typedef struct {
	DMA8237_TYPE type;
} DMA8237_CONFIG;


typedef struct {
	DMA8237_CONFIG config;
	UINT8 msb;
	UINT8 temp;
	struct {
		UINT16 address, base_address;
		UINT16 count, base_count;
		UINT8 transfer_mode;
		INT8 direction;
		UINT8 operation;

		int page; // this is outside of the chip!!!
	} chan[4];
	UINT8 status;
	UINT8 mask;
	UINT8 command;
	UINT8 DACK_hi;
	UINT8 DREQ_hi;
	UINT8 write_extended;
	UINT8 rotate_priority;
	UINT8 compressed_timing;
	UINT8 enable_controller;
	UINT8 channel;
} DMA8237;

	void dma8237_config(DMA8237 *This, DMA8237_CONFIG *config);

	void dma8237_reset(DMA8237 *This);

	UINT8 dma8237_write(DMA8237 *This, int channel);
	void dma8237_read(DMA8237 *This, int channel, UINT8 data);

extern DMA8237 dma8237[2];

#ifdef __cplusplus
}
#endif
