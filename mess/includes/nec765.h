

#define TWO_SIDE		0x08
#define READY			0x20
#define WRITE_PROTECT	0x40
#define FAULT			0x80

typedef struct nec765_id
{
	unsigned char C;
	unsigned char H;
	unsigned char R;
	unsigned char N;

	/* status bits - used to hold data/deleted data type and
	crc errors */
	unsigned char ST0;
	unsigned char ST1;
} nec765_id;

/* floppy drive types */
typedef enum
{
	FLOPPY_DRIVE_SS_40,
	FLOPPY_DRIVE_DS_80
} floppy_type;

typedef struct floppy_interface
{
	/* seek to physical track */
	void (*seek_callback)(int drive, int physical_track);
	/* get number of sectors per track on side specified */
	int (*get_sectors_per_track)(int drive, int physical_side);
	/* get id from current track and specified side */
	void (*get_id_callback)(int drive, nec765_id *, int id_index, int physical_side);
	/* get sector data for current track and specified side and ID */
	void (*get_sector_data_callback)(int,int,int,char **);
} floppy_interface;

typedef struct nec765_interface
{
	/* interrupt issued */
	void	(*interrupt)(int state);

	/* dma data request */
	void	(*dma_drq)(int state,int read_write);
} nec765_interface;

/* set nec765 dma drq output state */
void	nec765_set_dma_drq(int state);
/* set nec765 int output state */
void nec765_set_int(int);

/* init nec765 interface */
void nec765_init(nec765_interface *, int version);
/* set nec765 terminal count input state */
void nec765_set_tc_state(int);
/* set nec765 ready input*/
void	nec765_set_ready_state(int);


void nec765_idle(void);

/* read of data register */
READ_HANDLER(nec765_data_r);
/* write to data register */
WRITE_HANDLER(nec765_data_w);
/* read of main status register */
READ_HANDLER(nec765_status_r);

/* supported versions */
enum
{
	NEC765A=0,
	NEC765B=1,
	SMC37C78=2
} NEC765_VERSION;


void	floppy_drive_set_geometry(int,floppy_type type);
void    floppy_drive_setup_drive_status(int drive);
void	floppy_drive_set_motor_state(int);
void	floppy_set_interface(int, floppy_interface *);

/* dma acknowledge with write */
WRITE_HANDLER(nec765_dack_w);
/* dma acknowledge with read */
READ_HANDLER(nec765_dack_r);

/* reset nec765 */
void	nec765_reset(int);

/* reset pin of nec765 */
void	nec765_set_reset_state(int);
