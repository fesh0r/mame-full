###########################################################################
#
#   lib.mak
#
#   MAME dependent library makefile
#
#   Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################


LIBOBJ = $(OBJ)/lib

OBJDIRS += \
	$(LIBOBJ)/util \
	$(LIBOBJ)/expat \
	$(LIBOBJ)/zlib \



#-------------------------------------------------
# utility library objects
#-------------------------------------------------

UTILOBJS = \
	$(LIBOBJ)/util/bitmap.o \
	$(LIBOBJ)/util/cdrom.o \
	$(LIBOBJ)/util/chd.o \
	$(LIBOBJ)/util/corefile.o \
	$(LIBOBJ)/util/harddisk.o \
	$(LIBOBJ)/util/jedparse.o \
	$(LIBOBJ)/util/md5.o \
	$(LIBOBJ)/util/png.o \
	$(LIBOBJ)/util/sha1.o \
	$(LIBOBJ)/util/unicode.o \
	$(LIBOBJ)/util/unzip.o \
	$(LIBOBJ)/util/xmlfile.o \

$(OBJ)/libutil.a: $(UTILOBJS)



#-------------------------------------------------
# expat library objects
#-------------------------------------------------

EXPATOBJS = \
	$(LIBOBJ)/expat/xmlparse.o \
	$(LIBOBJ)/expat/xmlrole.o \
	$(LIBOBJ)/expat/xmltok.o

$(OBJ)/libexpat.a: $(EXPATOBJS)



#-------------------------------------------------
# zlib library objects
#-------------------------------------------------

ZLIBOBJS = \
	$(LIBOBJ)/zlib/adler32.o \
	$(LIBOBJ)/zlib/compress.o \
	$(LIBOBJ)/zlib/crc32.o \
	$(LIBOBJ)/zlib/deflate.o \
	$(LIBOBJ)/zlib/gzio.o \
	$(LIBOBJ)/zlib/inffast.o \
	$(LIBOBJ)/zlib/inflate.o \
	$(LIBOBJ)/zlib/infback.o \
	$(LIBOBJ)/zlib/inftrees.o \
	$(LIBOBJ)/zlib/trees.o \
	$(LIBOBJ)/zlib/uncompr.o \
	$(LIBOBJ)/zlib/zutil.o

$(OBJ)/libz.a: $(ZLIBOBJS)
