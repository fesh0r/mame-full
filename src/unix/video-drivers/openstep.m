/*
 * OpenStep specific file for XMAME. Here we define OpenStep specific
 * versions of all the MAME functions necessary to get it running under
 * OpenStep. 
 *
 * -bat. 11/1/1999
 */

#import <AppKit/AppKit.h>
#import <libc.h>
#import "xmame.h"
#import "osdepend.h"
#import "driver.h"
#import "keyboard.h"
#import "devices.h"

/* display options */

struct rc_option display_opts[] = {
	{
	"OpenStep related", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL
	}, {
	NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL
	}
};

/*
 * Some defines to control various code parameters.
 */

#define BORDER 12
#define INSET 4
#define FLUSH 30

/*
 * Variables used by command-line DPS window.
 */

NSWindow *thisWin = nil;		/* needed by keyboard code */
static NSView *thisView = nil;
static NSBitmapImageRep *thisBitmap = nil;
static DPSContext this_dps = NULL;
static int bitmap_width, bitmap_height;

/*
 * Screen bitmap variable
 */

static unsigned short *screen12bit = NULL;

/*
 * Autorelease pool variables. We have an outer pool which is never freed
 * an an inner one which is freed and released every few loops. This is
 * due to lessons learnt from EOF about always having at least *one* pool
 * active !
 */

static NSAutoreleasePool *outer_pool=nil;
static NSAutoreleasePool *inner_pool=nil;

/*
 * Intialise the two pools.
 */

int
sysdep_init(void)
{
	outer_pool = [NSAutoreleasePool new];
	inner_pool = [NSAutoreleasePool new];
	return OSD_OK;
}

/*
 * Free up the autorelease pools before exitting.
 */

void
sysdep_close(void)
{
	[inner_pool release];
	[outer_pool release];
}

/*
 * Shut down the display. We close the dps window and exit.
 */

void
sysdep_display_close(void)
{
	[thisView retain];
	[thisView lockFocus];

	DPSPrintf(this_dps,"currentwindow termwindow\n");
	DPSPrintf(this_dps,"nulldevice\n");
	PSWait();

	[thisView unlockFocus];
	[thisView release];

	this_dps = NULL;
	thisView = nil;
	thisWin = nil;
}

extern void openstep_keyboard_init(void);

/*
 * Create the display. We create a window of the appropriate size, then
 * make it display on the screen. Keyboard initialisation is also called
 * from this function.
 */

int
sysdep_create_display(void)
{
	NSRect content_rect = { {100,100}, {0,0} };
	bitmap_width = visual_width * widthscale;
	bitmap_height = visual_height * heightscale;

	/* set the size of the view */
	content_rect.size.width = bitmap_width + (BORDER*2) - 1;
	content_rect.size.height = bitmap_height + (BORDER*2) - 1;

	/* allocate memory for 12 bit colour version */
	screen12bit=malloc(2*bitmap_width*bitmap_height);
	if(!screen12bit) {
		fprintf(stderr,"12 bit memory allocate failed\n");
		return OSD_NOT_OK;
	}

	/* create bitmap object  */
	thisBitmap = [[NSBitmapImageRep alloc]
		initWithBitmapDataPlanes:(void*)&screen12bit
		pixelsWide:bitmap_width pixelsHigh:bitmap_height
		bitsPerSample:4 samplesPerPixel:3
		hasAlpha:NO isPlanar:NO
		colorSpaceName:NSDeviceRGBColorSpace
		bytesPerRow:2*bitmap_width bitsPerPixel:16];
	if(!thisBitmap) {
		fprintf(stderr,"Bitmap creation failed\n");
		return OSD_NOT_OK;
	}

	/* create an application object and a window */
	NSApp = [[NSApplication sharedApplication] retain];
	thisWin = [[NSWindow alloc] initWithContentRect:content_rect
		styleMask:(NSTitledWindowMask | NSMiniaturizableWindowMask)
		backing:NSBackingStoreRetained defer:NO];
	[thisWin setTitle:[NSString
		stringWithCString:Machine->gamedrv->description]];
	puts(Machine->gamedrv->description);

	/* bring to front and set the view variable */
	[thisWin makeKeyAndOrderFront:nil];
	thisView = [thisWin contentView];

	/* initialise it */
	[thisView lockFocus];
	this_dps = DPSGetCurrentContext();
	DPSPrintf(this_dps,"initgraphics 0 setgray\n");
	DPSFlushContext(this_dps);
	PSrectfill(INSET, INSET,
			content_rect.size.width + 1 - (INSET*2),
			content_rect.size.height + 1 - (INSET*2));
	PSWait();
	[thisView unlockFocus];

	/* set up the structure for the palette code */
	display_palette_info.writable_colors = 0;
	display_palette_info.depth = 16;
#ifdef LSB_FIRST
	display_palette_info.red_mask = 0x00f0;
	display_palette_info.green_mask = 0x000f;
	display_palette_info.blue_mask = 0xf000;
#else
	/* untested due to lack of big-endian machines at present */
	display_palette_info.red_mask = 0xf000;
	display_palette_info.green_mask = 0x0f00;
	display_palette_info.blue_mask = 0x00f0;
#endif

	/* shifts will be calculated from above settings */
	display_palette_info.red_shift = 0;
	display_palette_info.green_shift = 0;
	display_palette_info.blue_shift = 0;

	/* initialise the keyboard and return */
	openstep_keyboard_init();
	return OSD_OK;
}

/*
 * 8 bit display update. We use dirty unless the palette has been
 * changed, in which case the whole screen is updated.
 */

static void
update_display_8bpp(void)
{
#define	SRC_PIXEL	unsigned char
#define	DEST_PIXEL	unsigned short
#define	DEST		screen12bit
#define	DEST_WIDTH	bitmap_width
#define	INDIRECT	sysdep_palette->lookup
#include "blit.h"
#undef	SRC_PIXEL
#undef	DEST_PIXEL
#undef	DEST
#undef	DEST_WIDTH
#undef	INDIRECT
}

/*
 * 16 bit display update
 */

static void
update_display_16bpp(void)
{
#define	SRC_PIXEL	unsigned short
#define	DEST_PIXEL	unsigned short
#define	DEST		screen12bit
#define	DEST_WIDTH	bitmap_width
	if(sysdep_palette->lookup) {
#define	INDIRECT	sysdep_palette->lookup
#include "blit.h"
#undef	INDIRECT
	} else {
#include "blit.h"
	}

#undef	SRC_PIXEL
#undef	DEST_PIXEL
#undef	DEST
#undef	DEST_WIDTH
}

/*
 * Update the display.  We create the bitmapped data for the current frame
 * and draw it into the window.
 */

void
sysdep_update_display(void)
{
	static int flush_frame = FLUSH;
	int old_use_dirty;

	/* call appropriate function with dirty*/
	old_use_dirty = use_dirty;
	if(sysdep_palette->lookup_dirty)
		use_dirty = 0;
	if(bitmap->depth == 16)
		update_display_16bpp();
	else
		update_display_8bpp();
	use_dirty = old_use_dirty;

	/* lock focus and draw it */
	[thisView lockFocus];
	[thisBitmap drawInRect:(NSRect){ {BORDER, BORDER},
			{bitmap_width, bitmap_height}}];

	/* deal with inner autorelease pool every FLUSH frames */
	if(flush_frame-- <= 0) {
		[inner_pool release];
		inner_pool = [NSAutoreleasePool new];
		flush_frame = FLUSH;
	}

	/* wait for all PS operations too finish and unlock */
	PSWait();
	[thisView unlockFocus];
}

/*
 * OpenStep system are always 16bpp capable.
 */

int
sysdep_display_16bpp_capable(void)
{
	return 1;
}

/*
 * The following functions are dummies - they should never be called as
 * we never use 8 bit palletised output graphics under OpenStep. But they
 * must be present in order to link properly.
 */

int
sysdep_display_alloc_palette(int writable_colours)
{
	if(writable_colours) {
		fprintf(stderr,
			"Error: pallete allocation requested with %d colours\n",
			writable_colours);
		return -1;
	}

	return 0;
}

int
sysdep_display_set_pen(int pen,
		unsigned char r, unsigned char g, unsigned char b)
{
	fprintf(stderr, "Error: request made to set pen %d to %d/%d/%d\n",
			pen, r, g, b);
	return -1;
}
