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
#include "vidhrdw/generic.h"
#include "vidhrdw/pdp1.h"
#include "cpu/pdp1/pdp1.h"
#include "includes/pdp1.h"


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
		logerror("*** Warning! PDP1 Point list overflow!\n");
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
		osd_mark_dirty(x,y,x,y);
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
		osd_mark_dirty(x,y,x,y);
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
{  256,     0100, KEYCODE_DEL  , KEYCODE_DEL      ,"Delete"                    ,""             },
{  000,     0200, KEYCODE_SPACE, KEYCODE_SPACE    ,"Space"                     ,""             },
{  001,     0001, KEYCODE_1    , KEYCODE_SLASH    ,"1"                         ,"\""           },
{  002,     0002, KEYCODE_2    , KEYCODE_QUOTE    ,"2"                         ,"'"            },
{  003,     0203, KEYCODE_3    , KEYCODE_TILDE    ,"3"                         ,"~"            },
{  004,     0004, KEYCODE_4    , 0                ,"4"                         ,"(implies)"    },
{  005,     0205, KEYCODE_5    , 0                ,"5"                         ,"(or)"         },
{  006,     0206, KEYCODE_6    , 0                ,"6"                         ,"(and)"        },
{  007,     0007, KEYCODE_7    , 0                ,"7"                         ,"<"            },
{  010,     0010, KEYCODE_8    , 0                ,"8"                         ,">"            },
{  011,     0211, KEYCODE_9    , KEYCODE_UP       ,"9"                         ,"(up arrow)"   },
{  256,     0013, KEYCODE_STOP , KEYCODE_STOP     ,"Stop Code"                 ,""             },
{  020,     0020, KEYCODE_0    , KEYCODE_RIGHT    ,"0"                         ,"(right arrow)"},
{  021,     0221, KEYCODE_SLASH, 0                ,"/"                         ,"?"            },
{  022,     0222, KEYCODE_S    , KEYCODE_S        ,"s"                         ,"S"            },
{  023,     0023, KEYCODE_T    , KEYCODE_T        ,"t"                         ,"T"            },
{  024,     0224, KEYCODE_U    , KEYCODE_U        ,"u"                         ,"U"            },
{  025,     0025, KEYCODE_V    , KEYCODE_V        ,"v"                         ,"V"            },
{  026,     0026, KEYCODE_W    , KEYCODE_W        ,"w"                         ,"W"            },
{  027,     0227, KEYCODE_X    , KEYCODE_X        ,"x"                         ,"X"            },
{  030,     0230, KEYCODE_Y    , KEYCODE_Y        ,"y"                         ,"Y"            },
{  031,     0031, KEYCODE_Z    , KEYCODE_Z        ,"z"                         ,"Z"            },
{  033,     0233, KEYCODE_COMMA, KEYCODE_EQUALS   ,","                         ,"="            },
{  034,      256, 0            , 0                ,"black"                     ,""             },
{  035,      256, 0            , 0                ,"red"                       ,""             },
{  036,     0236, KEYCODE_TAB  , 0                ,"tab"                       ,""             },
{  040,     0040, 0            , 0                ," (non-spacing middle dot)" ,"_"            },
{  041,     0241, KEYCODE_J    , KEYCODE_J        ,"j"                         ,"J"            },
{  042,     0242, KEYCODE_K    , KEYCODE_K        ,"k"                         ,"K"            },
{  043,     0043, KEYCODE_L    , KEYCODE_L        ,"l"                         ,"L"            },
{  044,     0244, KEYCODE_M    , KEYCODE_M        ,"m"                         ,"M"            },
{  045,     0045, KEYCODE_N    , KEYCODE_N        ,"n"                         ,"N"            },
{  046,     0046, KEYCODE_O    , KEYCODE_O        ,"o"                         ,"O"            },
{  047,     0247, KEYCODE_P    , KEYCODE_P        ,"p"                         ,"P"            },
{  050,     0250, KEYCODE_Q    , KEYCODE_Q        ,"q"                         ,"Q"            },
{  051,     0051, KEYCODE_R    , KEYCODE_R        ,"r"                         ,"R"            },
{  054,     0054, KEYCODE_MINUS, KEYCODE_PLUS_PAD ,"-"                         ,"+"            },
{  055,     0255, KEYCODE_CLOSEBRACE, 0           ,")"                         ,"]"            },
{  056,     0256, 0            , 0                ," (non-spacing overstrike)" ,"|"            },
{  057,     0057, KEYCODE_OPENBRACE, 0            ,"("                         ,"["            },
{  061,     0061, KEYCODE_A    , KEYCODE_A        ,"a"                         ,"A"            },
{  062,     0062, KEYCODE_B    , KEYCODE_B        ,"b"                         ,"B"            },
{  063,     0263, KEYCODE_C    , KEYCODE_C        ,"c"                         ,"C"            },
{  064,     0064, KEYCODE_D    , KEYCODE_D        ,"d"                         ,"D"            },
{  065,     0265, KEYCODE_E    , KEYCODE_E        ,"e"                         ,"E"            },
{  066,     0266, KEYCODE_F    , KEYCODE_F        ,"f"                         ,"F"            },
{  067,     0067, KEYCODE_G    , KEYCODE_G        ,"g"                         ,"G"            },
{  070,     0070, KEYCODE_H    , KEYCODE_H        ,"h"                         ,"H"            },
{  071,     0271, KEYCODE_I    , KEYCODE_I        ,"i"                         ,"I"            },
{  072,     0272, KEYCODE_LSHIFT, 0               ,"Lower Case"                ,""             },
{  073,     0073, KEYCODE_ASTERISK,0              ,".(multiply)"               ,""             },
{  074,     0274, KEYCODE_LSHIFT, 0               ,"Upper Case"                ,""             },
{  075,     0075, KEYCODE_BACKSPACE,0             ,"Backspace"                 ,""             },
{  077,     0277, KEYCODE_ENTER, 0                ,"Carriage Return"           ,""             },
{  257,     0000, 0            , 0                ,""                          ,""             }
};

/* return first found pressed key */
int pdp1_keyboard(void)
{
	int i=0;

	fio_dec=0;
	concise=0;
	if (osd_is_key_pressed(KEYCODE_LSHIFT))
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
			if ((kbd[i].lower_osd)&&(osd_is_key_pressed(kbd[i].lower_osd)))
			{
				fio_dec=kbd[i].fio_dec;
				concise=kbd[i].concise;
				return 1;
			}
		}
		else
		{
			if ((kbd[i].upper_osd)&&(osd_is_key_pressed(kbd[i].upper_osd)))
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

	if (bitmap_width==0)
	{
		bitmap_width=bitmap->width;
		bitmap_height=bitmap->height;
	}
	clear_points(bitmap);
	set_points(bitmap);
	clear_point_list();
	cpu_set_reg(PDP1_S1,sense&0x80);
	cpu_set_reg(PDP1_S2,sense&0x40);
	cpu_set_reg(PDP1_S3,sense&0x20);
	cpu_set_reg(PDP1_S4,sense&0x10);
	cpu_set_reg(PDP1_S5,sense&0x08);
	cpu_set_reg(PDP1_S6,sense&0x04);
	cpu_set_reg(PDP1_F1,pdp1_keyboard());
}

