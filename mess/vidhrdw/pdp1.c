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

/*
	Theory of operation:

	What makes the pdp-1 CRT so odd is that there is no video processor, no scan logic,
	no refresh logic.  The beam position and intensity is controlled by the program completely:
	in order to draw an object, the program must direct the beam to each point of the object,
	and in order to refresh it, the program must redraw the object periodically.

	Since the refresh rates are highly variable (completely controlled by the program),
	I simulate CRT remanence: the intensity of each pixel on display decreases regularly.
	In order to keep this efficient, I keep a list of non-black pixels.

	However, this was a cause for additional flicker when doing frame skipping.  Basically,
	a pixel was displayed grey instead of white if it was drawn during a skipped frame.
	In order to fix this, I keep track of a pixel's maximum intensity since the last redraw,
	and each pixel is drawn at its maximum, not current, value.
*/

#include "driver.h"
#include "vidhrdw/generic.h"

#include "includes/pdp1.h"


static int bitmap_width;	/* width of machine bitmap */
static int bitmap_height;	/* height of machine bitmap */

typedef struct
{
	int cur_intensity;		/* current intensity of the pixel */
	int max_intensity;		/* maximum intensity since the last effective redraw */
							/* a node is not in the list when (max_intensity = -1) */
	int next;				/* index of next pixel in list */
} point;

static point *list;			/* array of (bitmap_width*bitmap_height) point */
static int list_head;		/* head of list in the array */


/*
	video init
*/
int pdp1_vh_start(void)
{
	int i;


	bitmap_width = Machine->scrbitmap->height;
	bitmap_height = Machine->scrbitmap->width;

	/* alloc bitmap for our private fun */
	tmpbitmap = bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
	if (! tmpbitmap)
		return 1;

	/* alloc the array */
	list = malloc(bitmap_width * bitmap_height * sizeof(point));
	if (! list)
	{
		bitmap_free(tmpbitmap);
		tmpbitmap = NULL;

		return 1;
	}
	/* fill with black and set up list as empty */
	for (i=0; i<(bitmap_width * bitmap_height); i++)
	{
		list[i].cur_intensity = 0;
		list[i].max_intensity = -1;
	}

	list_head = -1;

	return 0;
}


/*
	video clean-up
*/
void pdp1_vh_stop(void)
{
	if (list)
	{
		free(list);
		list = NULL;
	}
	if (tmpbitmap)
	{
		bitmap_free(tmpbitmap);
		tmpbitmap = NULL;
	}

	return;
}

/*
	plot a pixel (this is actually deferred)
*/
void pdp1_plot(int x, int y)
{
	point *node;
	int index;
	int intensity;

	x = x*bitmap_width/01777;
	y = y*bitmap_height/01777;
	if (x<0) x=0;
	if (y<0) y=0;
	if ((x>(bitmap_width-1))||((y>bitmap_height-1)))
		return;
	y = (bitmap_height-1) - y;
	intensity = VIDEO_MAX_INTENSITY;

	index = x + y*bitmap_width;

	node = & list[index];

	if (node->max_intensity == -1)
	{	/* insert node in list if it is not in it */
		node->max_intensity = 0;
		node->next = list_head;
		list_head = index;
	}
	if (intensity > node->cur_intensity)
		node->cur_intensity = intensity;
}


/*
	decrease pixel intensity
*/
static void update_points(struct mame_bitmap *bitmap)
{
	int i, p_i=-1;

	for (i=list_head; (i != -1); i=list[i].next)
	{
		point *node = & list[i];
		int x = i % bitmap_width, y = i / bitmap_width;

		/* remember maximum */
		if (node->cur_intensity > node->max_intensity)
			node->max_intensity = node->cur_intensity;

		/* reduce intensity for next update */
		if (node->cur_intensity > 0)
			node->cur_intensity--;

		p_i = i;	/* current node will be the previous node */
	}
}


/*
	update the bitmap
*/
static void set_points(struct mame_bitmap *bitmap)
{
	int i, p_i=-1;

	for (i=list_head; (i != -1); i=list[i].next)
	{
		point *node = & list[i];
		int x = i % bitmap_width, y = i / bitmap_width;

		if (node->cur_intensity > node->max_intensity)
			node->max_intensity = node->cur_intensity;

		plot_pixel(bitmap, x, y, Machine->pens[node->max_intensity]);
		osd_mark_dirty(x, y, x, y);

		if (/*(node->cur_intensity > 0) ||*/ (node->max_intensity != 0))
		{
			/* reset maximum */
			node->max_intensity = 0;
			p_i = i;	/* current node will be the previous node */
		}
		else
		{	/* delete current node */
			node->max_intensity = -1;
			if (p_i != -1)
				list[p_i].next = node->next;
			else
				list_head = node->next;
		}
	}
}


/*
	pdp1_screen_update: called regularly in simulated CPU time
*/
void pdp1_screen_update(void)
{
	update_points(tmpbitmap);
}


/*
	pdp1_vh_update: effectively redraw the screen
*/
void pdp1_vh_update (struct mame_bitmap *bitmap, int full_refresh)
{
	set_points(tmpbitmap);
	copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, &Machine->visible_area, TRANSPARENCY_NONE, 0);
}

