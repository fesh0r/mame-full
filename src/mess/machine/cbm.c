#include <stdio.h>
#include <stdarg.h>

#include "driver.h"

#include "cbm.h"

/* safer replacement str[0]=0; */
int cbm_snprintf(char *str,size_t size,const char *format, ...)
{
  va_list list;
  va_start(list, format);

  return vsprintf(str,format,list);
}

void *cbm_memset16(void *dest,int value, size_t size)
{
  register int i;
  for (i=0;i<size;i++) ((short*)dest)[i]=value;
  return dest;
}

static struct {
  const char *name;
  unsigned short addr;
  UINT8 *data;
  int length;
} quick;

int cbm_quick_init(int id, const char *name)
{
  FILE *fp;
  int read;
  char *cp;
  
  memset(&quick, 0, sizeof(quick));

  quick.name=name;

  fp = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_IMAGE_R, 0);
  if(!fp) return 1;
    
  quick.length = osd_fsize(fp);
    
  if ((cp=strrchr(name,'.'))!=NULL) {
    if (stricmp(cp,".prg")==0) {
      osd_fread_lsbfirst(fp,&quick.addr, 2);
      quick.length-=2;
    } else if (stricmp(cp,".p00")==0) {
      char buffer[7];
      osd_fread(fp,buffer,sizeof(buffer));
      if (strncmp(buffer,"C64File", sizeof(buffer))==0) {
	osd_fseek(fp,26, SEEK_SET);
	osd_fread_lsbfirst(fp, &quick.addr, 2);
	quick.length-=28;
      }
    }
  }
  if (quick.addr==0) {
    osd_fclose(fp);
    return 1;
  }
  if ((quick.data=malloc(quick.length))==NULL) {
    osd_fclose(fp);
    return 1;
  }
  read=osd_fread(fp,quick.data,quick.length);
  osd_fclose(fp);
  return read!=quick.length;
}

void cbm_quick_exit(int id)
{
  if (quick.data!=NULL)
    free(quick.data);
}

int cbm_quick_open(int id, void *arg)
{
  int addr;
  UINT8 *memory=arg;

  if (quick.data==NULL) return 1;
  addr=quick.addr+quick.length;

  memcpy(memory+quick.addr, quick.data, quick.length);
  memory[0x31]=memory[0x2f]=memory[0x2d]=addr&0xff;
  memory[0x32]=memory[0x30]=memory[0x2e]=addr>>8;
  if (errorlog)
    fprintf(errorlog,"quick loading %s at %.4x size:%.4x\n",
	    quick.name, quick.addr, quick.length);

  return 0;
}
