extern WRITE_HANDLER ( dma8237_0_w );
extern WRITE_HANDLER ( dma8237_1_w );

extern READ_HANDLER ( dma8237_0_r );
extern READ_HANDLER ( dma8237_1_r );

extern WRITE_HANDLER ( dma8237_at_0_w );
extern WRITE_HANDLER ( dma8237_at_1_w );

extern READ_HANDLER ( dma8237_at_0_r );
extern READ_HANDLER ( dma8237_at_1_r );

WRITE_HANDLER ( dma8237_0_page_w );
WRITE_HANDLER ( dma8237_1_page_w );

extern READ_HANDLER ( dma8237_0_page_r );
extern READ_HANDLER ( dma8237_1_page_r );

typedef struct {
	UINT8 msb;
	UINT8 temp;
	int address[4];
	int count[4];
	int page[4];
	UINT8 transfer_mode[4];
	INT8 direction[4];
	UINT8 operation[4];
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

	struct {
		UINT8 data[8];
	} pagereg;
} DMA8237;

extern DMA8237 dma8237[2];

#define pc_DMA_msb dma8237->msb
#define pc_DMA_temp dma8237->pc_DMA_temp
#define pc_DMA_address dma8237->address
#define pc_DMA_count dma8237->count
#define pc_DMA_page dma8237->page
#define pc_DMA_transfer_mode dma8237->transfer_mode
#define pc_DMA_direction dma8237->direction
#define pc_DMA_operation dma8237->operation
#define pc_DMA_status dma8237->status
#define pc_DMA_mask dma8237->mask
#define pc_DMA_command dma8237->command
#define pc_DMA_DACK_hi dma8237->DACK_hi
#define pc_DMA_DREQ_hi dma8237->DREQ_hi
#define pc_DMA_write_extended dma8237->write_extended
#define pc_DMA_rotate_priority dma8237->rotate_priority
#define pc_DMA_compressed_timing dma8237->compressed_timing
#define pc_DMA_enable_controller dma8237->enable_controller
#define pc_DMA_channel dma8237->channel
