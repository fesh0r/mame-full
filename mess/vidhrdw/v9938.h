/***************************************************************************

    v9938 / v9958 emulation

***************************************************************************/

/* init functions */

#define MODEL_V9938	(0)
#define MODEL_V9958	(1)

/* resolutions */
#define RENDER_HIGH	(0)
#define RENDER_LOW	(1)
#define RENDER_AUTO	(2)

int v9938_init (int, int, void (*callback)(int));
void v9938_reset (void);
void v9938_exit (void);
void v9958_init_palette (unsigned char *, unsigned short *,const unsigned char *);
void v9938_init_palette (unsigned char *, unsigned short *,const unsigned char *);
int v9938_interrupt (void);
void v9938_refresh (struct osd_bitmap *, int);
void v9938_set_sprite_limit (int);
void v9938_set_resolution (int);

WRITE_HANDLER (v9938_palette_w);
WRITE_HANDLER (v9938_vram_w);
READ_HANDLER (v9938_vram_r);
WRITE_HANDLER (v9938_command_w);
READ_HANDLER (v9938_status_r);
WRITE_HANDLER (v9938_register_w);
 
