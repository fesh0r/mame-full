
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

typedef struct nec765_interface
{

	/* seek to physical track */
	void (*seek_callback)(int drive, int physical_track);
	/* get number of sectors per track on side specified */
	int (*get_sectors_per_track)(int drive, int physical_side);
	/* get id from current track and specified side */
	void (*get_id_callback)(int drive, nec765_id *, int id_index, int physical_side);
	/* get sector data for current track and specified side and ID */
	void (*get_sector_data_callback)(int,int,int,char **);

	/* interrupt issued */
	void	(*interrupt)(int state);

} nec765_interface;

void nec765_set_int(int);
void nec765_init(nec765_interface *);
void nec765_set_tc_state(int);
/* read of data register */
READ_HANDLER(nec765_data_r);
/* write to data register */
WRITE_HANDLER(nec765_data_w);
/* read of main status register */
READ_HANDLER(nec765_status_r);


void	floppy_drive_set_geometry(int,floppy_type type);
void    floppy_drive_setup_drive_status(int drive);
void	floppy_drive_set_motor_state(int);


