/***************************************************************************

  6845 CRT controller registers

***************************************************************************/
#define HTOTAL  0       /* horizontal total         */
#define HDISP   1       /* horizontal displayed     */
#define HSYNCP  2       /* horizontal sync position */
#define HSYNCW  3       /* horizontal sync width    */

#define VTOTAL  4       /* vertical total           */
#define VTADJ   5       /* vertical total adjust    */
#define VDISP   6       /* vertical displayed       */
#define VSYNCW  7       /* vertical sync width      */

#define INTLACE 8       /* interlaced / graphics    */
#define SCNLINE 9       /* scan lines per char row  */

#define CURTOP  10      /* cursor top row / enable  */
#define CURBOT  11      /* cursor bottom row        */

#define VIDH    12      /* video buffer start high  */
#define VIDL    13      /* video buffer start low   */

#define CURH    14      /* cursor address high      */
#define CURL    15      /* cursor address low       */

#define LPENH   16      /* light pen high           */
#define LPENL   17      /* light pen low            */

extern int pc_blink;
extern int pc_fill_odd_scanlines;

extern int DOCLIP(struct rectangle *r1, const struct rectangle *r2);
extern void pc_fullblit(struct osd_bitmap *dest,const struct GfxElement *gfx,
	unsigned int code,unsigned int color,int sx,int sy,const struct rectangle *clip);
extern void pc_scanblit(struct osd_bitmap *dest,const struct GfxElement *gfx,
    unsigned int code,unsigned int color,int sx,int sy,const struct rectangle *clip);

