# Microsoft Developer Studio Project File - Name="dat2html" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=dat2html - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dat2html.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dat2html.mak" CFG="dat2html - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dat2html - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "dat2html - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "dat2html - Win32 Hybrid" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dat2html - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "mess32.obj\mess\tools\dat2html"
# PROP Intermediate_Dir "mess32.obj\mess\tools\dat2html"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "src/cpu/t11" /I "src/cpu/s2650" /I "src/cpu/nec" /I "src/cpu/konami" /I "src/cpu/tms34010" /I "src" /I "src/cpu/m6809" /I "src/cpu/h6280" /I "src/cpu/i86" /I "src/cpu/m6502" /I "src/cpu/m6808" /I "src/cpu/m68000" /I "obj/cpu/m68000" /I "src/cpu/z80" /I "src/cpu/i8039" /I "src/cpu/tms9900" /I "src/cpu/z8000" /I "src/cpu/tms32010" /I "MIDAS" /I "SEAL" /I "ZLIB" /I "src/Win32" /I "mess" /I "." /I "src/cpu/arm" /I "src/cpu/sc61860" /I "src/win32/zlib" /I MAME32NAME=\MESS32\ /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D PI=3.141592635 /D VERSION=37 /D RELEASE_CANDIDATE=0 /D "MAME_MMX" /D "PNG_SAVE_SUPPORT" /D "LSB_FIRST" /D "_WINDOWS" /D INLINE="static __inline" /D inline=__inline /D __inline__=__inline /D CLIB_DECL=__cdecl /D DECL_SPEC=__cdecl /D "ZLIB_DLL" /D "MESS" /D "NEOFREE" /D M_PI=3.141592654 /D "NOMIDAS" /YX /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"dat2html.exe"

!ELSEIF  "$(CFG)" == "dat2html - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "dat2html___Win32_Debug"
# PROP BASE Intermediate_Dir "dat2html___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "mess32d.obj\mess\tools\dat2html"
# PROP Intermediate_Dir "mess32d.obj\mess\tools\dat2html"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "src/cpu/t11" /I "src/cpu/s2650" /I "src/cpu/nec" /I "src/cpu/konami" /I "src/cpu/tms34010" /I "src" /I "src/cpu/m6809" /I "src/cpu/h6280" /I "src/cpu/i86" /I "src/cpu/m6502" /I "src/cpu/m6808" /I "src/cpu/m68000" /I "obj/cpu/m68000" /I "src/cpu/z80" /I "src/cpu/i8039" /I "src/cpu/tms9900" /I "src/cpu/z8000" /I "src/cpu/tms32010" /I "MIDAS" /I "SEAL" /I "ZLIB" /I "src/Win32" /I "mess" /I "." /I "src/cpu/arm" /I "src/cpu/sc61860" /I "src/win32/zlib" /I MAME32NAME=\MESS32\ /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D PI=3.141592635 /D VERSION=37 /D RELEASE_CANDIDATE=0 /D "MAME_MMX" /D "PNG_SAVE_SUPPORT" /D "LSB_FIRST" /D "_WINDOWS" /D INLINE="static __inline" /D inline=__inline /D __inline__=__inline /D CLIB_DECL=__cdecl /D DECL_SPEC=__cdecl /D "ZLIB_DLL" /D "MESS" /D "NEOFREE" /D M_PI=3.141592654 /D "NOMIDAS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"dat2htmld.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "dat2html - Win32 Hybrid"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "dat2html___Win32_Hybrid"
# PROP BASE Intermediate_Dir "dat2html___Win32_Hybrid"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "mess32h.obj\mess\tools\dat2html"
# PROP Intermediate_Dir "mess32h.obj\mess\tools\dat2html"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "src/cpu/t11" /I "src/cpu/s2650" /I "src/cpu/nec" /I "src/cpu/konami" /I "src/cpu/tms34010" /I "src" /I "src/cpu/m6809" /I "src/cpu/h6280" /I "src/cpu/i86" /I "src/cpu/m6502" /I "src/cpu/m6808" /I "src/cpu/m68000" /I "obj/cpu/m68000" /I "src/cpu/z80" /I "src/cpu/i8039" /I "src/cpu/tms9900" /I "src/cpu/z8000" /I "src/cpu/tms32010" /I "MIDAS" /I "SEAL" /I "ZLIB" /I "src/Win32" /I "mess" /I "." /I "src/cpu/arm" /I "src/cpu/sc61860" /I "src/win32/zlib" /I MAME32NAME=\MESS32\ /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D PI=3.141592635 /D VERSION=37 /D RELEASE_CANDIDATE=0 /D "MAME_MMX" /D "PNG_SAVE_SUPPORT" /D "LSB_FIRST" /D "_WINDOWS" /D INLINE="static __inline" /D inline=__inline /D __inline__=__inline /D CLIB_DECL=__cdecl /D DECL_SPEC=__cdecl /D "ZLIB_DLL" /D "MESS" /D "NEOFREE" /D M_PI=3.141592654 /D "NOMIDAS" /YX /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"dat2htmlh.exe"

!ENDIF 

# Begin Target

# Name "dat2html - Win32 Release"
# Name "dat2html - Win32 Debug"
# Name "dat2html - Win32 Hybrid"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\mess\tools\dat2html.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
