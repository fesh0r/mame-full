
typedef struct nec765_id
{
	unsigned char C;
	unsigned char H;
	unsigned char R;
	unsigned char N;
} nec765_id;


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
} nec765_interface;

/* not used yet, but will be */
typedef struct disk_drive
{
	/* number of sides */
	int num_sides;
	/* maximum number of tracks */
	int num_tracks;
	/* current track */
	int current_track;
} disk_drive;


void nec765_init(nec765_interface *);
void nec765_setup_drive_status(int drive);

/* read of data register */
int nec765_data_r(void);
/* write to data register */
void nec765_data_w(int);
/* read of main status register */
int nec765_status_r(void);



