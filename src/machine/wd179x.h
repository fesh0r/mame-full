#ifndef WD179X_H
#define WD179X_H

#define MAX_DRIVES      8

#define WD179X_IRQ_CLR  0
#define WD179X_IRQ_SET  1

typedef struct {
        byte    track;
        byte    sector;
        byte    status;
}       SECMAP;

typedef struct {
        void   (* callback)(int event);   /* callback for IRQ status */
        byte    tracks;                 /* maximum # of tracks */
        byte    heads;                  /* maximum # of heads */
        byte    density;                /* single / double density */
        word    offset;                 /* track 0 offset */
        byte    sec_per_track;          /* sectors per track */
        word    sector_length;          /* sector length (byte) */

        byte    head;                   /* current head # */
        byte    track;                  /* current track # */
        byte    track_reg;              /* value of track register */
        byte    direction;              /* last step direction */
        byte    sector;                 /* current sector # */
        byte    sector_dam;             /* current sector # to fake read DAM command */
        byte    data;                   /* value of data register */
        byte    command;                /* last command written */

        byte    read_cmd;               /* last read command issued */
        byte    write_cmd;              /* last write command issued */

        byte    status;                 /* status register */
        byte    status_drq;             /* status register data request bit */
        byte    status_ipl;             /* status register toggle index pulse bit */
        byte    busy_count;             /* how long to keep busy bit set */

        byte    buffer[6144];           /* I/O buffer (holds up to a whole track) */
        int     data_offset;            /* offset into I/O buffer */
        int     data_count;             /* transfer count from/into I/O buffer */

        char    *image_name;            /* file name for disc image */
        void    *image_file;            /* file handle for disc image */
        unsigned long image_size;       /* size of image file */

        word    dir_sector;             /* directory track for deleted DAM */
        word    dir_length;             /* directory length for deleted DAM */

        SECMAP  *secmap;

}       WD179X;

extern  void    wd179x_init(int active);

extern  void *  wd179x_select_drive(byte drive, byte head, void (*callback)(int), char * name);
extern  void    wd179x_stop_drive(void);

extern  void    wd179x_read_sectormap(byte drive, byte *tracks, byte *heads, byte *sec_per_track);
extern  void    wd179x_set_geometry(byte drive, byte tracks, byte heads, byte sec_per_track, word sector_length, word dir_sector, word dir_length);

extern  void    wd179x_command_w(int offset, int data);
extern  void    wd179x_track_w(int offset, int data);
extern  void    wd179x_sector_w(int offset, int data);
extern  void    wd179x_data_w(int offset, int data);

extern  int     wd179x_status_r(int offset);
extern  int     wd179x_track_r(int offset);
extern  int     wd179x_sector_r(int offset);
extern  int     wd179x_data_r(int offset);

#endif
