#ifdef xgl

#include "xmame.h"
#include "glmame.h"

GLubyte *read_JPEG_file(char *);

GLuint cablist;
int numtex;
GLuint *cabtex=NULL;
GLubyte **cabimg=NULL;

struct CameraPan *cpan=NULL;
int numpans;
int pannum;
int inpan=0;

static int inscreen=0;
static int scrvert;
static int inlist=0;

extern GLfloat cscrx1,cscry1,cscrz1,cscrx2,cscry2,cscrz2,
  cscrx3,cscry3,cscrz3,cscrx4,cscry4,cscrz4;


/* Skip until we hit whitespace */

char *SkipToSpace(char *buf)
{
  while(*buf&&!(isspace(*buf)||*buf==',')) buf++;

  return buf;
}

/* Skip whitespace and commas */

char *SkipSpace(char *buf)
{
  while(*buf&&(isspace(*buf)||*buf==',')) buf++;

  return buf;
}

/* Parse a string for a 4-component vector */

char *ParseVec4(char *buf,GLfloat *x,GLfloat *y,GLfloat *z,GLfloat *a)
{
  *x=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);
  buf=SkipSpace(buf);

  *y=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);
  buf=SkipSpace(buf);

  *z=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);
  buf=SkipSpace(buf);

  *a=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);

  return buf;
}

/* Parse a string for a 3-component vector */

char *ParseVec3(char *buf,GLfloat *x,GLfloat *y,GLfloat *z)
{
  *x=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);
  buf=SkipSpace(buf);

  *y=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);
  buf=SkipSpace(buf);

  *z=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);

  return buf;
}

/* Parse a string for a 2-component vector */

char  *ParseVec2(char *buf,GLfloat *x,GLfloat *y)
{
  *x=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);
  buf=SkipSpace(buf);

  *y=atof(buf);

  buf=SkipSpace(buf);
  buf=SkipToSpace(buf);

  return buf;
}

/* Null-terminate a string after the text is done */

void MakeString(char *buf)
{
  while(*buf&&!isspace(*buf)) buf++;

  *buf='\0';
}

/* Parse a camera pan */

void ParsePan(char *buf,PanType type)
{
  if(pannum==numpans) {
	printf("Error: too many camera pans specified\n");
	return;
  }

  cpan[pannum].type=type;
  buf=ParseVec3(buf,&cpan[pannum].lx,&cpan[pannum].ly,&cpan[pannum].lz);
  buf=ParseVec3(buf,&cpan[pannum].px,&cpan[pannum].py,&cpan[pannum].pz);
  buf=ParseVec3(buf,&cpan[pannum].nx,&cpan[pannum].ny,&cpan[pannum].nz);

  if(type==pan_moveto) cpan[pannum].frames=atoi(buf);

  pannum++;
}

/* Parse a line of the .cab file */

void ParseLine(char *buf)
{
  GLfloat x,y,z,a;
  int texnum;
  int xdim,ydim;

  buf=SkipSpace(buf);

  if(!*buf||*buf=='#'||*buf=='\n') return;

  if(!strncasecmp(buf,"startgeom",9)) {
	if(inlist) printf("Error: second call to startgeom\n");
	else {
	  glNewList(cablist,GL_COMPILE);
	  inlist=1;
	}
  }
  else if(!strncasecmp(buf,"numtex",6)) {
	if(inlist)
	  printf("numtex must be called before beginning model geometry\n");
	else {
	  numtex=atoi(buf+7);

	  if(numtex) {
		cabtex=(GLuint *)malloc(numtex*sizeof(GLuint));
		cabimg=(GLubyte **)malloc(numtex*sizeof(GLubyte *));
	  }
	}
  }
  else if(!strncasecmp(buf,"loadtex",7)) {
	if(inlist)
	  printf("loadtex calls cannot come after beginning model geometry\n");
	else {
	  if(!cabtex)
		printf("Error: Number of textures must be declared before texture loading\n");
	  else {
		buf=SkipToSpace(buf);
		buf=SkipSpace(buf);
		
		texnum=atoi(buf);
		
		if(texnum>=numtex)
		  printf("Error: Hightest possible texture number is %d\n",numtex-1);
		else {
		  buf=SkipToSpace(buf);
		  buf=SkipSpace(buf);
		  
		  xdim=atoi(buf);
		  
		  buf=SkipToSpace(buf);
		  buf=SkipSpace(buf);
		  
		  ydim=atoi(buf);
		  
		  buf=SkipToSpace(buf);
		  buf=SkipSpace(buf);
		  
		  MakeString(buf);
		  
		  printf("Loading texture %d (%dx%d) from %s\n",texnum,xdim,ydim,buf);
		  
		  glGenTextures(1,cabtex+texnum);
		  cabtex[texnum]=glGenLists(1);
		  glBindTexture(GL_TEXTURE_2D,cabtex[texnum]);
		  
		  cabimg[texnum]=read_JPEG_file(buf);
		  if(!cabimg[texnum])
			printf("Error: Unable to read %s\n",buf);
		  
		  glTexImage2D(GL_TEXTURE_2D,0,3,xdim,ydim,0,GL_RGB,GL_UNSIGNED_BYTE,
					   cabimg[texnum]);
		  
		  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		  
		  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
		  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
		}
	  }
	}
  }
  else if(!strncasecmp(buf,"camerapan",9)) {
	numpans=atoi(buf+9);

	cpan=(struct CameraPan *)malloc(numpans*sizeof(struct CameraPan));

	pannum=0;
	inpan=1;
  }
  else if(!strncasecmp(buf,"goto",4)) {
	if(!inpan) printf("Error: pan command outside of camerapan\n");
	else ParsePan(buf+4,pan_goto);
  }
  else if(!strncasecmp(buf,"moveto",6)) {
	if(!inpan) printf("Error: pan command outside of camerapan\n");
	else ParsePan(buf+6,pan_moveto);
  }
  else if(!strncasecmp(buf,"end",3)) {
	inscreen=0;
	inpan=0;
	glEnd();
  }
  else {
	if(!inlist) printf("A startgeom call is needed before specifying any geometry\n");
	else if(!strncasecmp(buf,"begin",5)) {
	  if(!strncasecmp(buf+6,"polygon",7)) {
		glBegin(GL_POLYGON);
	  }
	  else if(!strncasecmp(buf+6,"quads",5)) {
		glBegin(GL_QUADS);
	  }
	  else if(!strncasecmp(buf+6,"quad_strip",10)) {
	  glBegin(GL_QUAD_STRIP);
	  }
	  else if(!strncasecmp(buf+6,"screen",6)) {
		inscreen=1;
		scrvert=1;
	  }
	  else printf("Invalid object type -- %s",buf+6);
	}
	else if(!strncasecmp(buf,"color3",6)) {
	  ParseVec3(buf+7,&x,&y,&z);
	  glColor3f(x,y,z);
	}
	else if(!strncasecmp(buf,"color4",6)) {
	  ParseVec4(buf+7,&x,&y,&z,&a);
	  glColor4f(x,y,z,a);
	}
	else if(!strncasecmp(buf,"vertex",6)) {
	  if(inscreen) {
		switch(scrvert) {
		case 1:
		  ParseVec3(buf+7,&cscrx1,&cscry1,&cscrz1);
		  break;
		case 2:
		  ParseVec3(buf+7,&cscrx2,&cscry2,&cscrz2);
		  break;
		case 3:
		  ParseVec3(buf+7,&cscrx3,&cscry3,&cscrz3);
		  break;
		case 4:
		  ParseVec3(buf+7,&cscrx4,&cscry4,&cscrz4);
		  break;
		default:
		  printf("Error: Too many vertices in screen definition\n");
		  break;
		}
		
		scrvert++;
	  }
	  else {
		ParseVec3(buf+7,&x,&y,&z);
		glVertex3f(x,y,z);
	  }
	}
	else if(!strncasecmp(buf,"shading",7)) {
	  if(!strncasecmp(buf+8,"flat",4))
		glShadeModel(GL_FLAT);
	  else if(!strncasecmp(buf+8,"smooth",6))
		glShadeModel(GL_SMOOTH);
	  else printf("Invalid shading model -- %s",buf+8);
	}
	else if(!strncasecmp(buf,"enable",6)) {
	  if(!strncasecmp(buf+7,"texture",7))
		glEnable(GL_TEXTURE_2D);
	  else printf("Invalid feature to enable -- %s",buf+7);
	}
	else if(!strncasecmp(buf,"disable",7)) {
	  if(!strncasecmp(buf+8,"texture",7))
		glDisable(GL_TEXTURE_2D);
	  else printf("Invalid feature to disable -- %s",buf+7);
	}
	else if(!strncasecmp(buf,"settex",6)) {
	  texnum=atoi(buf+7);
	  
	  if(texnum>=numtex)
		printf("Error: Hightest possible texture number is %d\n",numtex-1);
	  else
		glBindTexture(GL_TEXTURE_2D,cabtex[texnum]);
	}
	else if(!strncasecmp(buf,"texcoord",8)) {
	  ParseVec2(buf+9,&x,&y);
	  glTexCoord2f(x,y);
	}
	else printf("Invalid command -- %s",buf);
  }
}

void InitCabGlobals()
{
  int i;

  for(i=0; cabimg!=0 && i<numtex; i++)
  {
	    if(cabimg[i]!=0)
		{
			free(cabimg[i]);
			cabimg[i]=0;
		}
  }
  if(cabimg!=0) 
		free(cabimg);
  cabimg=0;

  if(cabtex!=0) 
		free(cabtex);
  cabtex=0;

  if(cpan!=0) 
		free(cpan);
  cpan=0;

  numtex=0;
  cablist=0;
  inlist=0;
  numpans=0;
  pannum=0;
  inpan=0;
  inscreen=0;
  scrvert=0;
  inlist=0;
}

/* Load the cabinet */

int LoadCabinet(char *cabname)
{
  FILE *cfp;
  char buf[256];

  InitCabGlobals();

  sprintf(buf,"%s/cab/%s/%s.cab",XMAMEROOT,cabname,cabname);

  if(!(cfp=fopen(buf,"r")))
	return 0;

  printf("Loading Cabinet from %s\n",buf);

  cablist=glGenLists(1);

  if(!fgets(buf,256,cfp)) {
	printf("File is empty\n");
	return 0;
  }

  if(strncasecmp(buf,"cabv1.0",7)) {
	printf("File is not a v1.0 cabinet file -- cannot load\n");
	return 0;
  }

  while(fgets(buf,256,cfp)) {
	ParseLine(buf);
  }

  glEndList();

  fclose(cfp);

  return(1);
}

#endif /*ifdef xgl */
