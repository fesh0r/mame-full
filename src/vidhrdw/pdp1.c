/*
 * vidhrdw/pdp1.c
 *
 * This is not a very effective emulation, but frankly, the PDP1 was so slow
 * (compared to machines running MESS), that it really doesn't matter if this
 * is not all that effecient.
 *
 * This is not the video hardware either, this is just a dot plotting routine
 * for display hardware 'emulation' look at the machine/pdp1.c file.
 *
 */

#include "driver.h"
#include "generic.h"
#include "vidhrdw/pdp1.h"
#include "pdp1/pdp1.h"

int fio_dec=0;
int concise=0;
int case_state=0;
int reader_buffer=0;

typedef struct
{
	unsigned short int x;
	unsigned short int y;
} point;

static int bitmap_width=0;
static int bitmap_height=0;

static point *new_list;
static point *old_list;
static int new_index;
static int old_index;
#define MAX_POINTS (VIDEO_BITMAP_WIDTH*VIDEO_BITMAP_HEIGHT)

int pdp1_vh_start(void)
{
	videoram_size=(VIDEO_BITMAP_WIDTH*VIDEO_BITMAP_HEIGHT);
	old_list = malloc (MAX_POINTS * sizeof (point));
	new_list = malloc (MAX_POINTS * sizeof (point));
	if (!(old_list && new_list))
	{
		if (old_list)
			free(old_list);
		if (new_list)
			free(new_list);
		old_list=NULL;
		new_list=NULL;
		return 1;
	}
	new_index = 0;
	old_index = 0;
	return generic_vh_start();
}

void pdp1_vh_stop(void)
{
	if (old_list)
		free(old_list);
	if (new_list)
		free(new_list);
	generic_vh_stop();
	old_list=NULL;
	new_list=NULL;
	bitmap_width=0;
	bitmap_height=0;
	return;
}

void pdp1_plot(int x, int y)
{
	point *new;

	new = &new_list[new_index];
	x=(x)*bitmap_width/0777777;
	y=(y)*bitmap_height/0777777;
	if (x<0) x=0;
	if (y<0) y=0;
	if ((x>(bitmap_width-1))||((y>bitmap_height-1)))
		return;
	y*=-1;
	y+=(bitmap_height-1);
	new->x = x;
	new->y = y;
	new_index++;
	if (new_index >= MAX_POINTS)
	{
		new_index--;
		if (errorlog)
			fprintf (errorlog, "*** Warning! PDP1 Point list overflow!\n");
	}
}

void clear_point_list (void)
{
	point *tmp;

	old_index = new_index;
	tmp = old_list;
	old_list = new_list;
	new_list = tmp;
	new_index = 0;
}

static void clear_points(struct osd_bitmap *bitmap)
{
	unsigned char bg=Machine->pens[0];
	int i;

	for (i=old_index-1; i>=0; i--)
	{
		int x=(&old_list[i])->x;
		int y=(&old_list[i])->y;

		bitmap->line[y][x]=bg;
		osd_mark_dirty(x,y,x,y,1);
	}
	old_index=0;
}

static void set_points(struct osd_bitmap *bitmap)
{
	unsigned char fg=Machine->pens[1];
	int i;

	for (i=new_index-1; i>=0; i--)
	{
		int x=(&new_list[i])->x;
		int y=(&new_list[i])->y;

		bitmap->line[y][x]=fg;
		osd_mark_dirty(x,y,x,y,1);
	}
}

typedef struct
{
	int     fio_dec;
	int     concise;
	int     lower_osd;
	int     upper_osd;
	char   *lower_text;
	char   *upper_text;
} key_trans_pdp1;

static key_trans_pdp1 kbd[] =
{
{  000,     0000, 0            , 0                ,"Tape Feed"                 ,""             },
{  256,     0100, OSD_KEY_DEL  , OSD_KEY_DEL      ,"Delete"                    ,""             },
{  000,     0200, OSD_KEY_SPACE, OSD_KEY_SPACE    ,"Space"                     ,""             },
{  001,     0001, OSD_KEY_1    , OSD_KEY_SLASH    ,"1"                         ,"\""           },
{  002,     0002, OSD_KEY_2    , OSD_KEY_QUOTE    ,"2"                         ,"'"            },
{  003,     0203, OSD_KEY_3    , OSD_KEY_TILDE    ,"3"                         ,"~"            },
{  004,     0004, OSD_KEY_4    , 0                ,"4"                         ,"(implies)"    },
{  005,     0205, OSD_KEY_5    , 0                ,"5"                         ,"(or)"         },
{  006,     0206, OSD_KEY_6    , 0                ,"6"                         ,"(and)"        },
{  007,     0007, OSD_KEY_7    , 0                ,"7"                         ,"<"            },
{  010,     0010, OSD_KEY_8    , 0                ,"8"                         ,">"            },
{  011,     0211, OSD_KEY_9    , OSD_KEY_UP       ,"9"                         ,"(up arrow)"   },
{  256,     0013, OSD_KEY_STOP , OSD_KEY_STOP     ,"Stop Code"                 ,""             },
{  020,     0020, OSD_KEY_0    , OSD_KEY_RIGHT    ,"0"                         ,"(right arrow)"},
{  021,     0221, OSD_KEY_SLASH, 0                ,"/"                         ,"?"            },
{  022,     0222, OSD_KEY_S    , OSD_KEY_S        ,"s"                         ,"S"            },
{  023,     0023, OSD_KEY_T    , OSD_KEY_T        ,"t"                         ,"T"            },
{  024,     0224, OSD_KEY_U    , OSD_KEY_U        ,"u"                         ,"U"            },
{  025,     0025, OSD_KEY_V    , OSD_KEY_V        ,"v"                         ,"V"            },
{  026,     0026, OSD_KEY_W    , OSD_KEY_W        ,"w"                         ,"W"            },
{  027,     0227, OSD_KEY_X    , OSD_KEY_X        ,"x"                         ,"X"            },
{  030,     0230, OSD_KEY_Y    , OSD_KEY_Y        ,"y"                         ,"Y"            },
{  031,     0031, OSD_KEY_Z    , OSD_KEY_Z        ,"z"                         ,"Z"            },
{  033,     0233, OSD_KEY_COMMA, OSD_KEY_EQUALS   ,","                         ,"="            },
{  034,      256, 0            , 0                ,"black"                     ,""             },
{  035,      256, 0            , 0                ,"red"                       ,""             },
{  036,     0236, OSD_KEY_TAB  , 0                ,"tab"                       ,""             },
{  040,     0040, 0            , 0                ," (non-spacing middle dot)" ,"_"            },
{  041,     0241, OSD_KEY_J    , OSD_KEY_J        ,"j"                         ,"J"            },
{  042,     0242, OSD_KEY_K    , OSD_KEY_K        ,"k"                         ,"K"            },
{  043,     0043, OSD_KEY_L    , OSD_KEY_L        ,"l"                         ,"L"            },
{  044,     0244, OSD_KEY_M    , OSD_KEY_M        ,"m"                         ,"M"            },
{  045,     0045, OSD_KEY_N    , OSD_KEY_N        ,"n"                         ,"N"            },
{  046,     0046, OSD_KEY_O    , OSD_KEY_O        ,"o"                         ,"O"            },
{  047,     0247, OSD_KEY_P    , OSD_KEY_P        ,"p"                         ,"P"            },
{  050,     0250, OSD_KEY_Q    , OSD_KEY_Q        ,"q"                         ,"Q"            },
{  051,     0051, OSD_KEY_R    , OSD_KEY_R        ,"r"                         ,"R"            },
{  054,     0054, OSD_KEY_MINUS, OSD_KEY_PLUS_PAD ,"-"                         ,"+"            },
{  055,     0255, OSD_KEY_CLOSEBRACE, 0           ,")"                         ,"]"            },
{  056,     0256, 0            , 0                ," (non-spacing overstrike)" ,"|"            },
{  057,     0057, OSD_KEY_OPENBRACE, 0            ,"("                         ,"["            },
{  061,     0061, OSD_KEY_A    , OSD_KEY_A        ,"a"                         ,"A"            },
{  062,     0062, OSD_KEY_B    , OSD_KEY_B        ,"b"                         ,"B"            },
{  063,     0263, OSD_KEY_C    , OSD_KEY_C        ,"c"                         ,"C"            },
{  064,     0064, OSD_KEY_D    , OSD_KEY_D        ,"d"                         ,"D"            },
{  065,     0265, OSD_KEY_E    , OSD_KEY_E        ,"e"                         ,"E"            },
{  066,     0266, OSD_KEY_F    , OSD_KEY_F        ,"f"                         ,"F"            },
{  067,     0067, OSD_KEY_G    , OSD_KEY_G        ,"g"                         ,"G"            },
{  070,     0070, OSD_KEY_H    , OSD_KEY_H        ,"h"                         ,"H"            },
{  071,     0271, OSD_KEY_I    , OSD_KEY_I        ,"i"                         ,"I"            },
{  072,     0272, OSD_KEY_LSHIFT, 0               ,"Lower Case"                ,""             },
{  073,     0073, OSD_KEY_ASTERISK,0              ,".(multiply)"               ,""             },
{  074,     0274, OSD_KEY_LSHIFT, 0               ,"Upper Case"                ,""             },
{  075,     0075, OSD_KEY_BACKSPACE,0             ,"Backspace"                 ,""             },
{  077,     0277, OSD_KEY_ENTER, 0                ,"Carriage Return"           ,""             },
{  257,     0000, 0            , 0                ,""                          ,""             }
};

/* return first found pressed key */
int pdp1_keyboard(void)
{
	int i=0;

	fio_dec=0;
	concise=0;
	if (osd_key_pressed(OSD_KEY_LSHIFT))
	{
		if (case_state)
		{
			fio_dec=072;
			concise=0272;
		}
		else
		{
			fio_dec=074;
			concise=0274;
		}
		case_state=(case_state+1)%2;
		return 1;
	}
	
	while (kbd[i].fio_dec!=257)
	{
		if (case_state==0) /* lower case */
		{
			if ((kbd[i].lower_osd)&&(osd_key_pressed(kbd[i].lower_osd)))
			{
				fio_dec=kbd[i].fio_dec;
				concise=kbd[i].concise;
				return 1;
			}
		}
		else
		{
			if ((kbd[i].upper_osd)&&(osd_key_pressed(kbd[i].upper_osd)))
			{
				fio_dec=kbd[i].fio_dec;
				concise=kbd[i].concise;
				return 1;
			}
		}
		i++;
	}
	return 0;
}

void pdp1_vh_update (struct osd_bitmap *bitmap, int full_refresh)
{
	int sense=readinputport(1);
	pdp1_Regs Regs;

	if (bitmap_width==0)
	{
		bitmap_width=bitmap->width;
		bitmap_height=bitmap->height;
	}
	clear_points(bitmap);
	set_points(bitmap);
	clear_point_list();
	pdp1_GetRegs(&Regs);
	Regs.sense[1]=sense&0x80;
	Regs.sense[2]=sense&0x40;
	Regs.sense[3]=sense&0x20;
	Regs.sense[4]=sense&0x10;
	Regs.sense[5]=sense&0x08;
	Regs.sense[6]=sense&0x04;
	Regs.flag[1]=pdp1_keyboard();
	/* doesn't work while debugging -> context saving and restoring!!! */
	//pdp1_SetRegs(&Regs);
	pdp1_set_regs_true(&Regs);
}

