/***************************************************************************

 SAM Coupe Driver - Written By Lee Hammerton

***************************************************************************/

#define DSK1_PORT	224						// Disk Drive 1 Port 8 ports - decode port number as (bit 0-1 address lines, bit 2 side)
#define DSK2_PORT	240						// Disk Drive 2 Port 8 ports

#define LPEN_PORT	248						// X location of raster (Not supported yet)
#define CLUT_PORT	248						// Base port for CLUT (Write Only)
#define LINE_PORT	249						// Line interrupt port (Write Only)
#define STAT_PORT	249						// Keyboard status hi (Read Only)
#define LMPR_PORT	250						// Low bank page register
#define HMPR_PORT	251						// Hi bank page register
#define VMPR_PORT	252						// Screen page register
#define KEYB_PORT	254						// Keyboard status low (Read Only)
#define BORD_PORT	254						// Border Port (Write Only)
#define SSND_DATA	255						// Sound data port
#define HPEN_PORT	504						// Y location of raster (currently == curvideo line + 10 vblank lines!)
#define SSND_ADDR	511						// Sound address port

#define LMPR_RAM0	0x20					// If bit set ram is paged into bank 0, else its rom0
#define LMPR_ROM1	0x40					// If bit set rom1 is paged into bank 3, else its ram

extern unsigned char LMPR,HMPR,VMPR;								// Bank Select Registers (Low Page p250, Hi Page p251, Video Page p252)
extern unsigned char CLUT[16];										// 16 entries in a palette (no line affects supported yet!)
extern unsigned char SOUND_ADDR;									// Current Address in sound registers
extern unsigned char SOUND_REG[32];									// 32 sound registers
extern unsigned char LINE_INT;										// Line interrupt register
extern unsigned char LPEN,HPEN;										// ???
extern unsigned char CURLINE;										// Current scanline
extern unsigned char STAT;											// returned when port 249 read

struct sCoupe_fdc1772
{
	void *f;												// Pointer to file used for image of disk in this drive
	unsigned char Dsk_Command;
	unsigned char Dsk_Status;
	unsigned char Dsk_Data;
	unsigned char Dsk_Track;
	unsigned char Dsk_Sector;
	int bytesLeft;
};											// Holds the floppy controller vars for each drive

extern struct sCoupe_fdc1772 coupe_fdc1772[2];
extern unsigned char Dsk_Command;
extern unsigned char Dsk_Status;
extern unsigned char Dsk_Data;
extern unsigned char Dsk_Track;
extern unsigned char Dsk_Sector;

void coupe_update_memory(void);
int coupe_fdc_init(int);
