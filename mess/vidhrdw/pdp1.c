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

#include "includes/pdp1.h"


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

static void clear_point_list (void)
{
	point *tmp;

	old_index = new_index;
	tmp = old_list;
	old_list = new_list;
	new_list = tmp;
	new_index = 0;
}

static void clear_points(struct mame_bitmap *bitmap)
{
	unsigned char bg=Machine->pens[0];
	int i;

	for (i=old_index-1; i>=0; i--)
	{
		int x=(&old_list[i])->x;
		int y=(&old_list[i])->y;

		/*bitmap->line[y][x]=bg;*/
		plot_pixel(bitmap, x, y, bg);
		osd_mark_dirty(x,y,x,y);
	}
	old_index=0;
}

static void set_points(struct mame_bitmap *bitmap)
{
	unsigned char fg=Machine->pens[1];
	int i;

	for (i=new_index-1; i>=0; i--)
	{
		int x=(&new_list[i])->x;
		int y=(&new_list[i])->y;

		/*bitmap->line[y][x]=fg;*/
		plot_pixel(bitmap, x, y, fg);
		osd_mark_dirty(x,y,x,y);
	}
}


void pdp1_vh_update (struct mame_bitmap *bitmap, int full_refresh)
{
	if (bitmap_width==0)
	{
		bitmap_width=bitmap->width;
		bitmap_height=bitmap->height;
	}
	if (full_refresh)
		fillbitmap(bitmap, Machine->pens[0], /*&Machine->visible_area*/NULL);
	else
		clear_points(bitmap);
	set_points(bitmap);
	clear_point_list();
}

