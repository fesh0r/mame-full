# only OS specific output files and rules
OSOBJS = \
		$(OBJ)/wince/crtsuppl.o \
		$(OBJ)/wince/cesound.o \
		$(OBJ)/wince/ceinput.o \
		$(OBJ)/wince/video.o \
		$(OBJ)/wince/window.o \
		$(OBJ)/wince/cemain.o \
		$(OBJ)/wince/fronthlp.o \
		$(OBJ)/wince/snprintf.o \
		$(OBJ)/wince/misc.o \
		$(OBJ)/wince/dirent.o \
		$(OBJ)/wince/playgame.o \
		$(OBJ)/mess/Win32/SmartListView.o \
		$(OBJ)/mess/Win32/SoftwareList.o \
		$(OBJ)/zlib/crc32.o \
		$(OBJ)/zlib/adler32.o \
		$(OBJ)/zlib/compress.o \
		$(OBJ)/zlib/uncompr.o \
		$(OBJ)/zlib/deflate.o \
		$(OBJ)/zlib/zutil.o \
		$(OBJ)/zlib/trees.o \
		$(OBJ)/zlib/infcodes.o \
		$(OBJ)/zlib/infutil.o \
		$(OBJ)/zlib/inftrees.o \
		$(OBJ)/zlib/inflate.o \
		$(OBJ)/zlib/infblock.o \
		$(OBJ)/zlib/inffast.o

ifndef MESS
OSOBJS += $(OBJ)/windows/fileio.o 
else
OSOBJS += $(OBJ)/mess/windows/fileio.o	$(OBJ)/mess/windows/dirio.o \
	  $(OBJ)/mess/windows/messwin.o 
endif

RES = $(OBJ)/wince/mamece.res
