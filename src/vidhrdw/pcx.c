#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver.h"
#include "osdepend.h"
#include "vector.h"
#include "driver.h"
#include "generic.h"

typedef struct
{
 unsigned char          manifacturer;
 unsigned char          version;
 unsigned char          code;
 unsigned char          bits_per_pixel;
 unsigned short int     xmin;
 unsigned short int     ymin;
 unsigned short int     xmax;
 unsigned short int     ymax;
 unsigned short int     res_x;
 unsigned short int     res_y;
 unsigned char          rgb_colors[16][3];
 unsigned char          resered;
 unsigned char          number_of_color_plains;
 unsigned short int     bytes_per_line;
 unsigned short int     kind_of_color;
 unsigned char          filler[58];
} PCX_HEADER;

/*************************************************************************/
/* return 0 on success, fills pointer to 8bit bitmap on success, 1 on failure */
struct osd_bitmap *load_pcx8bit_overlay(char *filename)
{
 struct osd_bitmap *bitmap=NULL;
 PCX_HEADER pcx_header;
 FILE *handle=NULL;
 unsigned char *buffer=NULL;
 unsigned char *buffer_pointer=NULL;
 unsigned long int length_of_file;
 int i=0,y=0;
 if ((handle=fopen(filename,"rb"))==NULL)
  return NULL;
 fread(&pcx_header, sizeof(PCX_HEADER), 1, handle);
 if (!((pcx_header.bits_per_pixel==8)&&(pcx_header.number_of_color_plains==1)))
 {
  fclose(handle);
  return NULL;
 }
 fseek(handle,0,SEEK_END);
 length_of_file=ftell(handle);
 buffer=(unsigned char *)malloc(length_of_file);
 if (buffer==NULL)
 {
  fclose(handle);
  return NULL;
 }
 bitmap=osd_new_bitmap(pcx_header.xmax-pcx_header.xmin+1,pcx_header.ymax-pcx_header.ymin+1,8);
 if (bitmap==NULL)
 {
  fclose(handle);
  free(buffer);
  return NULL;
 }
 fseek(handle,128,SEEK_SET);
 fread(buffer,length_of_file,1,handle);
 buffer_pointer=buffer;
 /* decode the PCX file to bitmap */
 while (y<(pcx_header.ymax-pcx_header.ymin+1))
 {
  int crunched_value;
  int x=0;
  int number_of_bytes;
  while (x!=pcx_header.bytes_per_line)
  {
   crunched_value=*buffer_pointer++;
   if (crunched_value<192)
   {
    bitmap->line[y][x++]=crunched_value;
   }
   else
   {
    number_of_bytes=crunched_value&63;
    crunched_value=*buffer_pointer++;
    for (i=0;i<number_of_bytes;i++)
    {
     bitmap->line[y][x++]=crunched_value;
    }
   }
  } /* while (line_length_counter!=pcx_header.bytes_per_line) */
  y++;
 } /* while (k<(pcx_header.ymax-pcx_header.ymin+1)) */
 free(buffer);
 fclose(handle);
 return bitmap;
}
/*************************************************************************/

// 0 on success
// 1 on failure
// palette must have space for 768= 256*3 byte
// loaded are R G B values 0x00-0xff
int load_pcx8bit_colors(char *filename,unsigned char *palette)
{
 PCX_HEADER pcx_header;
 FILE *handle=NULL;
 if ((handle=fopen(filename,"rb"))==NULL)
  return 1;
 fread(&pcx_header, sizeof(PCX_HEADER), 1, handle);
 if (!((pcx_header.bits_per_pixel==8)&&(pcx_header.number_of_color_plains==1)))
 {
  fclose(handle);
  return 1;
 }
 fseek(handle,-768,SEEK_END);
 if (fread(palette,1,768,handle)!=768)
 {
  fclose(handle);
  return 1;
 }
 fclose(handle);
 return 0;
}
