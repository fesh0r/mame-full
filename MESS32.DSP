# Microsoft Developer Studio Project File - Name="mame32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=MAME32 - WIN32 DEBUG
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mess32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mess32.mak" CFG="MAME32 - WIN32 DEBUG"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mame32 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "mame32 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "mame32 - Win32 Hybrid" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/mame32", BUAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "mame32__"
# PROP BASE Intermediate_Dir "mame32__"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "obj/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /Gr /MT /W3 /Oy /Ob2 /I "src/cpu/t11" /I "src/cpu/s2650" /I "src/cpu/nec" /I "src/cpu/konami" /I "src/cpu/tms34010" /I "src" /I "src/cpu/m6809" /I "src/cpu/h6280" /I "src/cpu/i86" /I "src/cpu/m6502" /I "src/cpu/m6808" /I "src/cpu/m68000" /I "obj/cpu/m68000" /I "src/cpu/z80" /I "src/cpu/i8039" /I "src/cpu/tms9900" /I "src/cpu/z8000" /I "src/cpu/tms32010" /I "MIDAS" /I "SEAL" /I "ZLIB" /I "src/Win32" /I "mess" /I "." /I "src/cpu/arm" /I "src/cpu/sc61860" /I "src/win32/zlib" /I MAME32NAME=\MESS32\ /D "NDEBUG" /D PI=3.141592635 /D VERSION=37 /D RELEASE_CANDIDATE=0 /D "MAME_MMX" /D "PNG_SAVE_SUPPORT" /D "LSB_FIRST" /D "WIN32" /D "_WINDOWS" /D INLINE="static __inline" /D inline=__inline /D __inline__=__inline /D CLIB_DECL=__cdecl /D DECL_SPEC=__cdecl /D "ZLIB_DLL" /D "MESS" /D "NEOFREE" /D M_PI=3.141592654 /D "NOMIDAS" /D "MAME32NAME" /YX /FD \"MESS32\""" /c
# SUBTRACT CPP /X /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 libcmt.lib kernel32.lib user32.lib gdi32.lib comctl32.lib comdlg32.lib advapi32.lib winmm.lib shell32.lib dinput.lib dxguid.lib wsock32.lib vfw32.lib ZLIB/zlib.lib /nologo /subsystem:windows /incremental:yes /machine:I386 /nodefaultlib:"libc.lib" /nodefaultlib:"libcmt.lib" /out:"mess32.exe" /libpath:"src/win32"
# SUBTRACT LINK32 /pdb:none /debug /nodefaultlib

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "mame32_0"
# PROP BASE Intermediate_Dir "mame32_0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug/"
# PROP Intermediate_Dir "obj/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /Gr /MTd /W3 /ZI /Od /I "src/cpu/t11" /I "src/cpu/s2650" /I "src/cpu/nec" /I "src/cpu/konami" /I "src/cpu/tms34010" /I "src" /I "src/cpu/m6809" /I "src/cpu/h6280" /I "src/cpu/i86" /I "src/cpu/m6502" /I "src/cpu/m6808" /I "src/cpu/m68000" /I "obj/cpu/m68000" /I "src/cpu/z80" /I "src/cpu/i8039" /I "src/cpu/tms9900" /I "src/cpu/z8000" /I "src/cpu/tms32010" /I "MIDAS" /I "SEAL" /I "ZLIB" /I "src/Win32" /I "mess" /I "." /I "src/cpu/arm" /I "src/cpu/sc61860" /I "src/win32/zlib" /I MAME32NAME=\MESS32\ /D PI=3.141592635 /D VERSION=37 /D RELEASE_CANDIDATE=0 /D "MAME_MMX" /D "PNG_SAVE_SUPPORT" /D "LSB_FIRST" /D "WIN32" /D "_WINDOWS" /D INLINE="static __inline" /D inline=__inline /D __inline__=__inline /D CLIB_DECL=__cdecl /D DECL_SPEC=__cdecl /D "ZLIB_DLL" /D "MESS" /D "NEOFREE" /D M_PI=3.141592654 /D "NOMIDAS" /D "MAME32NAME" /YX /FD /D "\" MESS32\"""" MAME32NAME="\"MESS32\"" /c
# SUBTRACT CPP /X /u
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comctl32.lib comdlg32.lib advapi32.lib winmm.lib shell32.lib dinput.lib dxguid.lib wsock32.lib vfw32.lib ZLIB/zlib.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"LIBC.LIB" /nodefaultlib:"LIBCMT.LIB" /out:"mess32d.exe" /libpath:"src/win32"
# SUBTRACT LINK32 /profile /incremental:no /map

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "mame32___Win32_Hybrid"
# PROP BASE Intermediate_Dir "mame32___Win32_Hybrid"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "mame32___Win32_Hybrid"
# PROP Intermediate_Dir "mame32___Win32_Hybrid"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /Gr /MT /W3 /Oy /Ob2 /I "src/cpu/t11" /I "src/cpu/s2650" /I "src/cpu/nec" /I "src/cpu/konami" /I "src/cpu/tms34010" /I "src" /I "src/cpu/m6809" /I "src/cpu/h6280" /I "src/cpu/i86" /I "src/cpu/m6502" /I "src/cpu/m6808" /I "src/cpu/m68000" /I "obj/cpu/m68000" /I "src/cpu/z80" /I "src/cpu/i8039" /I "src/cpu/tms9900" /I "src/cpu/z8000" /I "src/cpu/tms32010" /I "MIDAS" /I "SEAL" /I "ZLIB" /I "src/Win32" /D "NDEBUG" /D PI=3.1415926535 /D VERSION=36 /D RELEASE_CANDIDATE=1 /D "MAME_MMX" /D "PNG_SAVE_SUPPORT" /D "LSB_FIRST" /D "WIN32" /D "_WINDOWS" /D INLINE="static __inline" /D inline=__inline /D __inline__=__inline /D CLIB_DECL=__cdecl /D DECL_SPEC=__cdecl /D "ZLIB_DLL" /YX /FD /c
# SUBTRACT BASE CPP /X /Fr
# ADD CPP /nologo /G5 /Gr /MT /W3 /Zi /Oy /Ob2 /I "src/cpu/t11" /I "src/cpu/s2650" /I "src/cpu/nec" /I "src/cpu/konami" /I "src/cpu/tms34010" /I "src" /I "src/cpu/m6809" /I "src/cpu/h6280" /I "src/cpu/i86" /I "src/cpu/m6502" /I "src/cpu/m6808" /I "src/cpu/m68000" /I "obj/cpu/m68000" /I "src/cpu/z80" /I "src/cpu/i8039" /I "src/cpu/tms9900" /I "src/cpu/z8000" /I "src/cpu/tms32010" /I "MIDAS" /I "SEAL" /I "ZLIB" /I "src/Win32" /I "mess" /I "." /I "src/cpu/arm" /I "src/cpu/sc61860" /I "src/win32/zlib" /I MAME32NAME=\MESS32\ /D "NDEBUG" /D PI=3.141592635 /D VERSION=37 /D RELEASE_CANDIDATE=0 /D "MAME_MMX" /D "PNG_SAVE_SUPPORT" /D "LSB_FIRST" /D "WIN32" /D "_WINDOWS" /D INLINE="static __inline" /D inline=__inline /D __inline__=__inline /D CLIB_DECL=__cdecl /D DECL_SPEC=__cdecl /D "ZLIB_DLL" /D "MESS" /D "NEOFREE" /D M_PI=3.141592654 /D "NOMIDAS" /YX /FD /D MAME32NAME="\"MESS32\"" \"MESS32\""" /c
# SUBTRACT CPP /X /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libcmt.lib kernel32.lib user32.lib gdi32.lib comctl32.lib comdlg32.lib advapi32.lib winmm.lib shell32.lib dinput.lib dxguid.lib wsock32.lib vfw32.lib MIDAS/midasdll.lib ZLIB/zlib.lib /nologo /subsystem:windows /incremental:yes /machine:I386 /nodefaultlib:"libc.lib" /nodefaultlib:"libcmt.lib"
# SUBTRACT BASE LINK32 /pdb:none /debug /nodefaultlib
# ADD LINK32 libcmt.lib kernel32.lib user32.lib gdi32.lib comctl32.lib comdlg32.lib advapi32.lib winmm.lib shell32.lib dinput.lib dxguid.lib wsock32.lib vfw32.lib ZLIB/zlib.lib /nologo /subsystem:windows /incremental:yes /debug /machine:I386 /nodefaultlib:"libc.lib" /nodefaultlib:"libcmt.lib" /out:"mess32h.exe"
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ENDIF 

# Begin Target

# Name "mame32 - Win32 Release"
# Name "mame32 - Win32 Debug"
# Name "mame32 - Win32 Hybrid"
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\artwork.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\artwork.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\audit.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\audit.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cheat.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cheat.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\common.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\common.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpuintrf.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpuintrf.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\datafile.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\datafile.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\drawgfx.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\drawgfx.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gfxobj.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gfxobj.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\hiscore.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\hiscore.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\info.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\info.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\inptport.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\inptport.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\input.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\input.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\legacy.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\mame.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\mame.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\mamedbg.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\mamedbg.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\memory.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\memory.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\network.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\network.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\osdepend.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\palette.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\palette.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\png.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\png.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\profiler.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\profiler.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\romcmp.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "mess32h.obj\src"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sndintrf.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sndintrf.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sprite.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sprite.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\state.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\state.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\tilemap.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\tilemap.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\timer.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\timer.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ui_text.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\unzip.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\unzip.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\usrintrf.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\usrintrf.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\version.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\window.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\window.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src"

!ENDIF 

# End Source File
# End Group
# Begin Group "cpu"

# PROP Default_Filter ""
# Begin Group "adsp2100"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\adsp2100\2100dasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/adsp2100/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/adsp2100/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/adsp2100/Release"
# PROP Intermediate_Dir "obj/cpu/adsp2100/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\adsp2100\2100ops.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/adsp2100/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/adsp2100/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/adsp2100/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/adsp2100/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\adsp2100\adsp2100.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/adsp2100/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/adsp2100/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/adsp2100/Release"
# PROP Intermediate_Dir "obj/cpu/adsp2100/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\adsp2100\adsp2100.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/adsp2100/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/adsp2100/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/adsp2100/Release"
# PROP Intermediate_Dir "obj/cpu/adsp2100/Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "ccpu"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\ccpu\ccpu.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/ccpu/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/ccpu/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/ccpu/Release"
# PROP Intermediate_Dir "obj/cpu/ccpu/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\ccpu\ccpu.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/ccpu/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/ccpu/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/ccpu/Release"
# PROP Intermediate_Dir "obj/cpu/ccpu/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\ccpu\ccpudasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/ccpu/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/ccpu/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/ccpu/Release"
# PROP Intermediate_Dir "obj/cpu/ccpu/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\ccpu\ccputabl.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/ccpu/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/ccpu/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/ccpu/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/ccpu/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "h6280"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Cpu\h6280\6280dasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/h6280/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/H6280/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/h6280/Release"
# PROP Intermediate_Dir "obj/cpu/h6280/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\h6280\h6280.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/h6280/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/H6280/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/h6280/Release"
# PROP Intermediate_Dir "obj/cpu/h6280/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\h6280\h6280.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/h6280/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/H6280/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/h6280/Release"
# PROP Intermediate_Dir "obj/cpu/h6280/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\h6280\h6280ops.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/h6280/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/H6280/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/h6280/Release"
# PROP Intermediate_Dir "obj/cpu/h6280/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\h6280\tblh6280.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/h6280/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/H6280/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/h6280/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/h6280/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "i8039"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\i8039\8039dasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/i8039/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/i8039/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/i8039/Release"
# PROP Intermediate_Dir "obj/cpu/i8039/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\i8039\i8039.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/i8039/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/i8039/Debug"
# ADD CPP /W3

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/i8039/Release"
# PROP Intermediate_Dir "obj/cpu/i8039/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\i8039\i8039.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/i8039/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/i8039/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/i8039/Release"
# PROP Intermediate_Dir "obj/cpu/i8039/Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "i8085"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\i8085\8085dasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/i8085/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/i8085/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/i8085/Release"
# PROP Intermediate_Dir "obj/cpu/i8085/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\i8085\i8085.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/i8085/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/i8085/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/i8085/Release"
# PROP Intermediate_Dir "obj/cpu/i8085/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\i8085\i8085.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/i8085/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/i8085/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/i8085/Release"
# PROP Intermediate_Dir "obj/cpu/i8085/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\i8085\i8085cpu.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/i8085/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/i8085/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/i8085/Release"
# PROP Intermediate_Dir "obj/cpu/i8085/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\i8085\i8085daa.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/i8085/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/i8085/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/i8085/Release"
# PROP Intermediate_Dir "obj/cpu/i8085/Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "i86"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\i86\ea.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/i86/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/i86/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/i86/Release"
# PROP Intermediate_Dir "obj/cpu/i86/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\host.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/i86/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/i86/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/i86/Release"
# PROP Intermediate_Dir "obj/cpu/i86/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\i286.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\cpu\m6502"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src\cpu\m6502"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src\cpu\i86"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\i86.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/i86/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/i86/Debug"
# ADD CPP /W3

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/i86/Release"
# PROP Intermediate_Dir "obj/cpu/i86/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\i86.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/i86/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/i86/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/i86/Release"
# PROP Intermediate_Dir "obj/cpu/i86/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\i86\i86dasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/i86/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/i86/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/i86/Release"
# PROP Intermediate_Dir "obj/cpu/i86/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\i86intrf.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/i86/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/i86/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/i86/Release"
# PROP Intermediate_Dir "obj/cpu/i86/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\instr.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/i86/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/i86/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/i86/Release"
# PROP Intermediate_Dir "obj/cpu/i86/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\modrm.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/i86/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/i86/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/i86/Release"
# PROP Intermediate_Dir "obj/cpu/i86/Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "konami"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\konami\knmidasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/konami/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/konami/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/konami/Release"
# PROP Intermediate_Dir "obj/cpu/konami/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\konami\konami.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/konami/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/konami/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/konami/Release"
# PROP Intermediate_Dir "obj/cpu/konami/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\konami\konami.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/konami/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/konami/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/konami/Release"
# PROP Intermediate_Dir "obj/cpu/konami/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\konami\konamops.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/konami/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/konami/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/konami/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/konami/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\konami\konamtbl.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/konami/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/konami/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/konami/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/konami/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "m6502"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\m6502\6502dasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m6502/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m6502/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m6502/Release"
# PROP Intermediate_Dir "obj/cpu/m6502/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\m4510.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32h.obj\src\cpu\m6502"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src\cpu\m6502"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src\cpu\m6502"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\m6502.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m6502/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m6502/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m6502/Release"
# PROP Intermediate_Dir "obj/cpu/m6502/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\m6502.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m6502/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m6502/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m6502/Release"
# PROP Intermediate_Dir "obj/cpu/m6502/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\m6509.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\cpu\m6502"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src\cpu\m6502"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src\cpu\m6502"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\m65ce02.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\cpu\m6502"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src\cpu\m6502"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src\cpu\m6502"

!ENDIF 

# End Source File
# End Group
# Begin Group "m6800"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\m6800\6800dasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m6800/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m6800/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m6800/Release"
# PROP Intermediate_Dir "obj/cpu/m6800/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6800\6800ops.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m6800/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m6800/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m6800/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/m6800/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6800\m6800.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m6800/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m6800/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m6800/Release"
# PROP Intermediate_Dir "obj/cpu/m6800/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6800\m6800.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m6800/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m6800/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m6800/Release"
# PROP Intermediate_Dir "obj/cpu/m6800/Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "m68000"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Cpu\m68000\Asmintf.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m68000/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m68000/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m68000/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/m68000/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\m68000\cpudefs.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m68000/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m68000/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m68000/Release"
# PROP Intermediate_Dir "obj/cpu/m68000/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\m68000\d68k.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m68000/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m68000/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m68000/Release"
# PROP Intermediate_Dir "obj/cpu/m68000/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\m68000\d68k.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m68000/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m68000/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m68000/Release"
# PROP Intermediate_Dir "obj/cpu/m68000/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\m68000\D68kconf.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m68000/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m68000/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m68000/Release"
# PROP Intermediate_Dir "obj/cpu/m68000/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\m68000\m68000.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m68000/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m68000/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m68000/Release"
# PROP Intermediate_Dir "obj/cpu/m68000/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\m68000\m68k.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m68000/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m68000/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m68000/Release"
# PROP Intermediate_Dir "obj/cpu/m68000/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\m68000\m68k_in.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m68000/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m68000/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m68000/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/m68000/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\m68000\m68kconf.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m68000/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m68000/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m68000/Release"
# PROP Intermediate_Dir "obj/cpu/m68000/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\m68000\m68kmake.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m68000/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m68000/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m68000/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/m68000/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\m68000\Make68k.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m68000/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m68000/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m68000/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/m68000/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "m6805"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\m6805\6805dasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m6805/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m6805/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m6805/Release"
# PROP Intermediate_Dir "obj/cpu/m6805/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6805\6805ops.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m6805/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m6805/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m6805/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/m6805/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6805\m6805.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m6805/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m6805/Debug"
# ADD CPP /W3

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m6805/Release"
# PROP Intermediate_Dir "obj/cpu/m6805/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6805\m6805.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m6805/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m6805/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m6805/Release"
# PROP Intermediate_Dir "obj/cpu/m6805/Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "m6809"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\m6809\6809dasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m6809/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m6809/Debug"
# ADD CPP /Gr

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m6809/Release"
# PROP Intermediate_Dir "obj/cpu/m6809/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6809\6809ops.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m6809/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m6809/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m6809/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/m6809/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6809\6809tbl.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m6809/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m6809/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m6809/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/m6809/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6809\m6809.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m6809/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m6809/Debug"
# ADD CPP /W3

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m6809/Release"
# PROP Intermediate_Dir "obj/cpu/m6809/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6809\m6809.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/m6809/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/m6809/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/m6809/Release"
# PROP Intermediate_Dir "obj/cpu/m6809/Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "nec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\nec\nec.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/nec/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/nec/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/nec/Release"
# PROP Intermediate_Dir "obj/cpu/nec/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\nec\nec.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/nec/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/nec/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/nec/Release"
# PROP Intermediate_Dir "obj/cpu/nec/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\nec\necdasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/nec/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/nec/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/nec/Release"
# PROP Intermediate_Dir "obj/cpu/nec/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\nec\necea.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/nec/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/nec/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/nec/Release"
# PROP Intermediate_Dir "obj/cpu/nec/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\nec\nechost.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/nec/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/nec/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/nec/Release"
# PROP Intermediate_Dir "obj/cpu/nec/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\nec\necinstr.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/nec/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/nec/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/nec/Release"
# PROP Intermediate_Dir "obj/cpu/nec/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\nec\necintrf.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/nec/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/nec/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/nec/Release"
# PROP Intermediate_Dir "obj/cpu/nec/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\nec\necmodrm.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/nec/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/nec/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/nec/Release"
# PROP Intermediate_Dir "obj/cpu/nec/Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "pdp1"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\pdp1\pdp1.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/pdp1/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/pdp1/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/pdp1/Release"
# PROP Intermediate_Dir "obj/cpu/pdp1/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\pdp1\pdp1.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/pdp1/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/pdp1/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/pdp1/Release"
# PROP Intermediate_Dir "obj/cpu/pdp1/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\pdp1\pdp1dasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/pdp1/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/pdp1/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/pdp1/Release"
# PROP Intermediate_Dir "obj/cpu/pdp1/Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "s2650"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\s2650\2650dasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/s2650/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/s2650/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/s2650/Release"
# PROP Intermediate_Dir "obj/cpu/s2650/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\s2650\s2650.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/s2650/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/s2650/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/s2650/Release"
# PROP Intermediate_Dir "obj/cpu/s2650/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\s2650\s2650.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/s2650/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/s2650/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/s2650/Release"
# PROP Intermediate_Dir "obj/cpu/s2650/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\s2650\s2650cpu.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/s2650/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/s2650/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/s2650/Release"
# PROP Intermediate_Dir "obj/cpu/s2650/Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "t11"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\t11\t11.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/t11/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/t11/Debug"
# ADD CPP /W3

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/t11/Release"
# PROP Intermediate_Dir "obj/cpu/t11/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\t11\t11.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/t11/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/t11/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/t11/Release"
# PROP Intermediate_Dir "obj/cpu/t11/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\t11\t11dasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/t11/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/t11/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/t11/Release"
# PROP Intermediate_Dir "obj/cpu/t11/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\t11\t11ops.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/t11/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/t11/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/t11/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/t11/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\t11\t11table.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/t11/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/t11/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/t11/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/t11/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "tms32010"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Cpu\tms32010\32010dsm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms32010/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms32010/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms32010/Release"
# PROP Intermediate_Dir "obj/cpu/tms32010/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\tms32010\dis32010.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms32010/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms32010/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms32010/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/tms32010/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\tms32010\tms32010.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms32010/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms32010/Debug"
# ADD CPP /W3

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms32010/Release"
# PROP Intermediate_Dir "obj/cpu/tms32010/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\tms32010\tms32010.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms32010/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms32010/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms32010/Release"
# PROP Intermediate_Dir "obj/cpu/tms32010/Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "tms34010"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\tms34010\34010dsm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms34010/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms34010/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms34010/Release"
# PROP Intermediate_Dir "obj/cpu/tms34010/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms34010\34010fld.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms34010/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms34010/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms34010/Release"
# PROP Intermediate_Dir "obj/cpu/tms34010/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms34010\34010ops.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms34010/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms34010/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms34010/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/tms34010/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms34010\34010ops.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms34010/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms34010/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms34010/Release"
# PROP Intermediate_Dir "obj/cpu/tms34010/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms34010\34010tbl.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms34010/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms34010/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms34010/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/tms34010/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms34010\dis34010.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms34010/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms34010/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms34010/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/tms34010/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms34010\tms34010.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms34010/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms34010/Debug"
# ADD CPP /W3

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms34010/Release"
# PROP Intermediate_Dir "obj/cpu/tms34010/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms34010\tms34010.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms34010/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms34010/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms34010/Release"
# PROP Intermediate_Dir "obj/cpu/tms34010/Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "tms9900"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\tms9900\9900dasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms9900/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms9900/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms9900/Release"
# PROP Intermediate_Dir "obj/cpu/tms9900/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms9900\99xxcore.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms9900/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms9900/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms9900/Release"
# PROP Intermediate_Dir "obj/cpu/tms9900/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms9900\99xxstat.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms9900/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms9900/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms9900/Release"
# PROP Intermediate_Dir "obj/cpu/tms9900/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms9900\tms9900.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms9900/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms9900/Debug"
# ADD CPP /W3

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms9900/Release"
# PROP Intermediate_Dir "obj/cpu/tms9900/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms9900\tms9900.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms9900/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms9900/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms9900/Release"
# PROP Intermediate_Dir "obj/cpu/tms9900/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms9900\tms9980a.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms9900/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms9900/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms9900/Release"
# PROP Intermediate_Dir "obj/cpu/tms9900/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms9900\tms9995.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/tms9900/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/tms9900/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/tms9900/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/tms9900/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "z80"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\z80\makez80.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z80/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z80/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z80/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/z80/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\z80\z80.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z80/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z80/Debug"
# ADD CPP /W3 /Od

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z80/Release"
# PROP Intermediate_Dir "obj/cpu/z80/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\z80\z80.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z80/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z80/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z80/Release"
# PROP Intermediate_Dir "obj/cpu/z80/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\z80\z80DAA.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z80/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z80/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z80/Release"
# PROP Intermediate_Dir "obj/cpu/z80/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\z80\z80Dasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z80/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z80/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z80/Release"
# PROP Intermediate_Dir "obj/cpu/z80/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\z80\z80Dasm.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z80/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z80/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z80/Release"
# PROP Intermediate_Dir "obj/cpu/z80/Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "z8000"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\Cpu\z8000\8000dasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z8000/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z8000/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z8000/Release"
# PROP Intermediate_Dir "obj/cpu/z8000/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\z8000\Makedab.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z8000/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z8000/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z8000/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/z8000/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\z8000\z8000.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z8000/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z8000/Debug"
# ADD CPP /W3

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z8000/Release"
# PROP Intermediate_Dir "obj/cpu/z8000/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\z8000\z8000.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z8000/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z8000/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z8000/Release"
# PROP Intermediate_Dir "obj/cpu/z8000/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\z8000\z8000cpu.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z8000/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z8000/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z8000/Release"
# PROP Intermediate_Dir "obj/cpu/z8000/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\z8000\z8000dab.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z8000/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z8000/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z8000/Release"
# PROP Intermediate_Dir "obj/cpu/z8000/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\z8000\z8000ops.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z8000/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z8000/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z8000/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/z8000/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Cpu\z8000\z8000tbl.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z8000/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z8000/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z8000/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/z8000/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "z80gb"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\z80gb\daa_tab.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z80gb/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z80gb/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z80gb/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/z80gb/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\z80gb\opc_cb.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z80gb/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z80gb/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z80gb/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/z80gb/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\z80gb\opc_main.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z80gb/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z80gb/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z80gb/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/z80gb/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\z80gb\z80gb.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z80gb/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z80gb/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z80gb/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/z80gb/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\z80gb\z80gb.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z80gb/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z80gb/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z80gb/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/z80gb/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\z80gb\z80gbd.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/cpu/z80gb/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/cpu/z80gb/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/cpu/z80gb/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/cpu/z80gb/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "sc61860"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\sc61860\disasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\cpu\sc61860"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src\cpu\sc61860"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src\cpu\sc61860"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\sc61860\sc.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\cpu\sc61860"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src\cpu\sc61860"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src\cpu\sc61860"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\sc61860\sc61860.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\cpu\sc61860"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src\cpu\sc61860"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src\cpu\sc61860"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\sc61860\sc61860.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\cpu\sc61860"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src\cpu\sc61860"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src\cpu\sc61860"

!ENDIF 

# End Source File
# End Group
# Begin Group "arm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\arm\arm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\cpu\arm"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src\cpu\arm"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src\cpu\arm"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\cpu\arm\dasm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\src\cpu\arm"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\src\cpu\arm"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\src\cpu\arm"

!ENDIF 

# End Source File
# End Group
# End Group
# Begin Group "drivers"

# PROP Default_Filter ""
# End Group
# Begin Group "machine"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\machine\6821pia.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/machine/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/machine/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/machine/Release"
# PROP Intermediate_Dir "obj/machine/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\machine\6821pia.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/machine/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/machine/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/machine/Release"
# PROP Intermediate_Dir "obj/machine/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\machine\74123.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/machine/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/machine/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/machine/Release"
# PROP Intermediate_Dir "obj/machine/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\machine\74123.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/machine/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/machine/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/machine/Release"
# PROP Intermediate_Dir "obj/machine/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\machine\8254pit.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/machine/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/machine/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/machine/Release"
# PROP Intermediate_Dir "obj/machine/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\machine\8254pit.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/machine/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/machine/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/machine/Release"
# PROP Intermediate_Dir "obj/machine/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\machine\8255ppi.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/machine/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/machine/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/machine/Release"
# PROP Intermediate_Dir "obj/machine/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\machine\8255ppi.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/machine/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/machine/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/machine/Release"
# PROP Intermediate_Dir "obj/machine/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\machine\eeprom.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\eeprom.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\pd4990a.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\z80fmly.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/machine/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/machine/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/machine/Release"
# PROP Intermediate_Dir "obj/machine/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\machine\z80fmly.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/machine/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/machine/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/machine/Release"
# PROP Intermediate_Dir "obj/machine/Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "msdos"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\msdos\ati15khz.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\ati15khz.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Msdos\blit.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\config.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Msdos\dirty.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\fileio.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\fronthlp.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\gen15khz.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\gen15khz.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\input.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\makefile

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\mamalleg.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\monitors.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\msdos.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\nec765.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\osd_cpu.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\osd_dbg.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\osinline.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\sound.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\twkuser.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\twkuser.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\vector.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\vgafreq.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\msdos\video.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "sndhrdw"

# PROP Default_Filter ""
# End Group
# Begin Group "sound"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\sound\2151intf.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\2151intf.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\2203intf.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\2413intf.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\2608intf.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\2608intf.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\2610intf.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\2610intf.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\2612intf.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\2612intf.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\3812intf.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\3812intf.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\5220intf.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\5220intf.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\adpcm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\adpcm.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\astrocde.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\astrocde.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\ay8910.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\ay8910.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\c140.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\cem3394.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\dac.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\dac.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\fm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\fm.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\fmopl.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\fmopl.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\hc55516.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\k005289.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\k007232.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\k051649.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\k053260.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\k053260.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\mixer.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\mixer.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\msm5205.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\msm5205.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\namco.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\nes_apu.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\nes_apu.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\nes_defs.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\pokey.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\pokey.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\qsound.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\qsound.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\rf5c68.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\samples.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\samples.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\segapcm.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\sn76477.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\sn76496.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\sn76496.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\speaker.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/sound/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\speaker.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\streams.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\streams.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\tiaintf.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\tiaintf.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\tiasound.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\tiasound.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\tms5220.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\tms5220.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\tms5220r.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/sound/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\upd7759.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\upd7759.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\vlm5030.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\vlm5030.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\votrax.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\wave.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "obj/sound/Release"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\wave.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\ym2151.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\ym2151.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\ym2413.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\ymdeltat.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\sound\ymdeltat.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/sound/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/sound/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/sound/Release"
# PROP Intermediate_Dir "obj/sound/Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "vidhrdw"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\vidhrdw\avgdvg.c
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\cps1.c
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\generic.c
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\generic.h
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\vector.c
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\vector.h
# End Source File
# End Group
# Begin Group "win32"

# PROP Default_Filter ""
# Begin Group "Resources"

# PROP Default_Filter ".ico;.bmp;.cur"
# Begin Source File

SOURCE=.\src\win32\Res\about.bmp

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\bkgnd16.bmp
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\bkground.bmp
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\checkmark.ico
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\custom.ico
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\display.ico
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\foldavail.ico
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\folder.ico
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\foldmanu.ico
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\foldopen.ico
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\foldunav.ico
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\foldyear.ico
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\joystick.ico
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\keyboard.ico
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\mame32.ico

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\manufact.ico
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\neogeo.ico
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\netplay.ico
# End Source File
# Begin Source File

SOURCE=.\src\Win32\Res\nonwork.ico
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\prop_dis.bmp
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\samples.ico
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\Splith.cur
# End Source File
# Begin Source File

SOURCE=.\src\Win32\Res\toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\win_noro.ico

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\win_redx.ico

!IF  "$(CFG)" == "mame32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\win_roms.ico

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\win_unkn.ico

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\working.ico
# End Source File
# Begin Source File

SOURCE=.\src\win32\Res\year.ico
# End Source File
# End Group
# Begin Group "NTopldrv"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\win32\mameopl\Instopl.bat

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\mameopl\Makefile

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\mameopl\Mameopl.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\mameopl\Mameopl.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\mameopl\mameopl.inf

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\mameopl\Mameopl.rc

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\mameopl\Sources

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Help Files"

# PROP Default_Filter "cnt;rtf"
# Begin Source File

SOURCE=".\src\Win32\hlp\Game List as of 0.36.xls"
# End Source File
# Begin Source File

SOURCE=.\src\Win32\Hlp\MAME32.cnt

!IF  "$(CFG)" == "mame32 - Win32 Release"

# Begin Custom Build - Copying contents file...
OutDir=.\Release
InputPath=.\src\Win32\Hlp\MAME32.cnt
InputName=MAME32

"$(OutDir)\..\$(InputName).cnt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "src\Win32\hlp\$(InputName).cnt" $(OutDir)\..

# End Custom Build

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Copying contents file...
ProjDir=.
InputPath=.\src\Win32\Hlp\MAME32.cnt
InputName=MAME32

"$(ProjDir)\$(InputName).cnt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "src\Win32\hlp\$(InputName).cnt" $(ProjDir)

# End Custom Build

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# Begin Custom Build - Copying contents file...
OutDir=.\mame32___Win32_Hybrid
InputPath=.\src\Win32\Hlp\MAME32.cnt
InputName=MAME32

"$(OutDir)\..\$(InputName).cnt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "src\Win32\hlp\$(InputName).cnt" $(OutDir)\..

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Mame32.hlp
# End Source File
# Begin Source File

SOURCE=.\src\Win32\Hlp\Mame32.rtf

!IF  "$(CFG)" == "mame32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Hlp\Mameaudt.shg

!IF  "$(CFG)" == "mame32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Hlp\Mamefile.shg

!IF  "$(CFG)" == "mame32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\Hlp\Mamefilt.shg
# End Source File
# Begin Source File

SOURCE=.\src\win32\Hlp\Mamefold.shg
# End Source File
# Begin Source File

SOURCE=.\src\Win32\Hlp\Mamegame.shg

!IF  "$(CFG)" == "mame32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Hlp\Mamehelp.shg

!IF  "$(CFG)" == "mame32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Hlp\Mamemenu.shg

!IF  "$(CFG)" == "mame32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Hlp\Mameopt.shg

!IF  "$(CFG)" == "mame32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Hlp\Mameoptd.shg

!IF  "$(CFG)" == "mame32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Hlp\Mameprop.shg

!IF  "$(CFG)" == "mame32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\Hlp\Mameres.shg
# End Source File
# Begin Source File

SOURCE=.\src\win32\Hlp\Mametool.shg
# End Source File
# Begin Source File

SOURCE=.\src\win32\Hlp\Mamevers.shg
# End Source File
# Begin Source File

SOURCE=.\src\Win32\Hlp\Mameview.shg

!IF  "$(CFG)" == "mame32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Hlp\Mamevwai.shg

!IF  "$(CFG)" == "mame32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Hlp\Mamevwcu.shg

!IF  "$(CFG)" == "mame32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\hlp\README_HELP.txt
# End Source File
# End Group
# Begin Group "Makefiles"

# PROP Default_Filter ".mak;.bat"
# Begin Source File

SOURCE=.\MakeHelp.bat
# End Source File
# Begin Source File

SOURCE=.\mame32.mak
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=.\src\win32\audit32.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Avi.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\Bitmask.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\ColumnEdit.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\DataMap.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\DDrawDisplay.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\DDrawWindow.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\DIJoystick.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\DIKeyboard.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\directdraw.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\DirectInput.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\Directories.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\DirectSound.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\dirty.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Display.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\dxdecode.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\file.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\FilePrivate.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\fmsynth.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\GDIDisplay.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Joystick.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Keyboard.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\led.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\M32Util.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\MAME32.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\Mameopl.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\MidasSound.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\mzip.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\net32.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\netchat32.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\Ntddkbd.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\NTFMsynth.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\NullSound.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\options.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\osd_cpu.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\osd_dbg.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\osinline.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\Properties.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\RenderBitmap.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\resource.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"
# PROP Ignore_Default_Tool 1
# Begin Custom Build - Making help include files...
TargetName=mess32
InputPath=.\src\Win32\resource.h

"src\Win32\hlp\$(TargetName).hm" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo. >"src\Win32\hlp\$(TargetName).hm" 
	echo // Commands (ID_* and IDM_*) >>"src\Win32\hlp\$(TargetName).hm" 
	makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 src\Win32\resource.h >>"src\Win32\hlp\$(TargetName).hm" 
	echo. >>"src\Win32\hlp\$(TargetName).hm" 
	echo // Prompts (IDP_*) >>"src\Win32\hlp\$(TargetName).hm" 
	makehm IDP_,HIDP_,0x30000 src\Win32\resource.h >>"src\Win32\hlp\$(TargetName).hm" 
	echo. >>"src\Win32\hlp\$(TargetName).hm" 
	echo // Resources (IDR_*) >>"src\Win32\hlp\$(TargetName).hm" 
	makehm IDR_,HIDR_,0x20000 src\Win32\resource.h >>"src\Win32\hlp\$(TargetName).hm" 
	echo. >>"src\Win32\hlp\$(TargetName).hm" 
	echo // Dialogs (IDD_*) >>"src\Win32\hlp\$(TargetName).hm" 
	makehm IDD_,HIDD_,0x20000 src\Win32\resource.h >>"src\Win32\hlp\$(TargetName).hm" 
	echo. >>"src\Win32\hlp\$(TargetName).hm" 
	echo // Frame Controls (IDW_*) >>"src\Win32\hlp\$(TargetName).hm" 
	makehm IDW_,HIDW_,0x50000 src\Win32\resource.h >>"src\Win32\hlp\$(TargetName).hm" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"
# PROP Ignore_Default_Tool 1
# Begin Custom Build - Making help include files...
TargetName=mess32d
InputPath=.\src\Win32\resource.h

"src\Win32\hlp\$(TargetName).hm" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo. >"src\Win32\hlp\$(TargetName).hm" 
	echo // Commands (ID_* and IDM_*) >>"src\Win32\hlp\$(TargetName).hm" 
	makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 src\Win32\resource.h >>"src\Win32\hlp\$(TargetName).hm" 
	echo. >>"src\Win32\hlp\$(TargetName).hm" 
	echo // Prompts (IDP_*) >>"src\Win32\hlp\$(TargetName).hm" 
	makehm IDP_,HIDP_,0x30000 src\Win32\resource.h >>"src\Win32\hlp\$(TargetName).hm" 
	echo. >>"src\Win32\hlp\$(TargetName).hm" 
	echo // Resources (IDR_*) >>"src\Win32\hlp\$(TargetName).hm" 
	makehm IDR_,HIDR_,0x20000 src\Win32\resource.h >>"src\Win32\hlp\$(TargetName).hm" 
	echo. >>"src\Win32\hlp\$(TargetName).hm" 
	echo // Dialogs (IDD_*) >>"src\Win32\hlp\$(TargetName).hm" 
	makehm IDD_,HIDD_,0x20000 src\Win32\resource.h >>"src\Win32\hlp\$(TargetName).hm" 
	echo. >>"src\Win32\hlp\$(TargetName).hm" 
	echo // Frame Controls (IDW_*) >>"src\Win32\hlp\$(TargetName).hm" 
	makehm IDW_,HIDW_,0x50000 src\Win32\resource.h >>"src\Win32\hlp\$(TargetName).hm" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP BASE Ignore_Default_Tool 1
# PROP Intermediate_Dir "obj/Win32/Release"
# PROP Ignore_Default_Tool 1
# Begin Custom Build - Making help include files...
TargetName=mess32h
InputPath=.\src\Win32\resource.h

"src\Win32\hlp\$(TargetName).hm" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo. >"src\Win32\hlp\$(TargetName).hm" 
	echo // Commands (ID_* and IDM_*) >>"src\Win32\hlp\$(TargetName).hm" 
	makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 src\Win32\resource.h >>"src\Win32\hlp\$(TargetName).hm" 
	echo. >>"src\Win32\hlp\$(TargetName).hm" 
	echo // Prompts (IDP_*) >>"src\Win32\hlp\$(TargetName).hm" 
	makehm IDP_,HIDP_,0x30000 src\Win32\resource.h >>"src\Win32\hlp\$(TargetName).hm" 
	echo. >>"src\Win32\hlp\$(TargetName).hm" 
	echo // Resources (IDR_*) >>"src\Win32\hlp\$(TargetName).hm" 
	makehm IDR_,HIDR_,0x20000 src\Win32\resource.h >>"src\Win32\hlp\$(TargetName).hm" 
	echo. >>"src\Win32\hlp\$(TargetName).hm" 
	echo // Dialogs (IDD_*) >>"src\Win32\hlp\$(TargetName).hm" 
	makehm IDD_,HIDD_,0x20000 src\Win32\resource.h >>"src\Win32\hlp\$(TargetName).hm" 
	echo. >>"src\Win32\hlp\$(TargetName).hm" 
	echo // Frame Controls (IDW_*) >>"src\Win32\hlp\$(TargetName).hm" 
	makehm IDW_,HIDW_,0x50000 src\Win32\resource.h >>"src\Win32\hlp\$(TargetName).hm" 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\resource.hm
# End Source File
# Begin Source File

SOURCE=.\src\Win32\Screenshot.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Sound.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\Splitters.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\status.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\strings.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Trak.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\TreeView.h
# End Source File
# Begin Source File

SOURCE=.\src\Win32\uclock.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\win32ui.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter ".c;.rc;.rtf"
# Begin Source File

SOURCE=.\src\win32\audit32.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Avi.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\Bitmask.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\ColumnEdit.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\DataMap.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\DDrawDisplay.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\DDrawWindow.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\debug.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\DIJoystick.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\DIKeyboard.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\directdraw.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\directinput.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\Directories.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\DirectSound.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\dirty.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Display.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\dxdecode.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\file.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\fmsynth.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\GDIDisplay.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Joystick.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Keyboard.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\led.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\M32Util.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\MAME32.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Hlp\MAME32.hpj

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"
# PROP Ignore_Default_Tool 1
USERDEP__MAME3="src\Win32\hlp\$(InputName).rtf"	"src\Win32\hlp\$(InputName).hm"	
# Begin Custom Build - Making help files...
ProjDir=.
InputPath=.\src\Win32\Hlp\MAME32.hpj
InputName=MAME32

"src\Win32\hlp\$(InputName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	start /wait hcw /C /E /M "src\Win32\hlp\$(InputName).hpj" 
	if errorlevel 1 goto :Error 
	if not exist "src\Win32\hlp\$(InputName).hlp" goto :Error 
	copy "src\Win32\hlp\$(InputName).hlp" $(ProjDir) 
	goto :done 
	:Error 
	echo src\Win32\ hlp\$(InputName).hpj(1) : error: 
	type "src\Win32\hlp\$(InputName).log" 
	:done 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"
# PROP Ignore_Default_Tool 1
USERDEP__MAME3="src\Win32\hlp\$(InputName).rtf"	"src\Win32\hlp\$(InputName).hm"	
# Begin Custom Build - Making help files...
ProjDir=.
InputPath=.\src\Win32\Hlp\MAME32.hpj
InputName=MAME32

"src\Win32\hlp\$(InputName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	start /wait hcw /C /E /M "src\Win32\hlp\$(InputName).hpj" 
	if errorlevel 1 goto :Error 
	if not exist "src\Win32\hlp\$(InputName).hlp" goto :Error 
	copy "src\Win32\hlp\$(InputName).hlp" $(ProjDir) 
	goto :done 
	:Error 
	echo src\Win32\ hlp\$(InputName).hpj(1) : error: 
	type "src\Win32\hlp\$(InputName).log" 
	:done 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP BASE Ignore_Default_Tool 1
# PROP Intermediate_Dir "obj/Win32/Release"
# PROP Ignore_Default_Tool 1
USERDEP__MAME3="src\Win32\hlp\$(InputName).rtf"	"src\Win32\hlp\$(InputName).hm"	
# Begin Custom Build - Making help files...
ProjDir=.
InputPath=.\src\Win32\Hlp\MAME32.hpj
InputName=MAME32

"src\Win32\hlp\$(InputName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	start /wait hcw /C /E /M "src\Win32\hlp\$(InputName).hpj" 
	if errorlevel 1 goto :Error 
	if not exist "src\Win32\hlp\$(InputName).hlp" goto :Error 
	copy "src\Win32\hlp\$(InputName).hlp" $(ProjDir) 
	goto :done 
	:Error 
	echo src\Win32\ hlp\$(InputName).hpj(1) : error: 
	type "src\Win32\hlp\$(InputName).log" 
	:done 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\mame32.rc

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"
# ADD BASE RSC /l 0x409 /i "src\Win32"
# ADD RSC /l 0x409 /i "src\Win32"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"
# ADD BASE RSC /l 0x409 /i "src\Win32"
# ADD RSC /l 0x409 /i "src\Win32"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"
# ADD BASE RSC /l 0x409 /i "src\Win32"
# ADD RSC /l 0x409 /i "src\Win32"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\mzip.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\net32.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\netchat32.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\NTFMsynth.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\NullSound.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\options.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\osdepend.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\Properties.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\RenderBitmap.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\RenderMMX.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Screenshot.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\Splitters.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\status.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\Trak.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\win32\TreeView.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\uclock.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Win32\win32ui.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "obj/Win32/Release"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "obj/Win32/Debug"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Intermediate_Dir "obj/Win32/Release"
# PROP Intermediate_Dir "obj/Win32/Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "Text Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\midas.txt

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\quickstart.htm
# End Source File
# Begin Source File

SOURCE=.\Readme32.txt

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\WhatsNew32.txt

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# End Group
# Begin Group "TextFiles"

# PROP Default_Filter "txt"
# Begin Source File

SOURCE=.\gamelist.txt

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\readme.txt

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\seal.txt

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\whatsnew.txt

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "mess"

# PROP Default_Filter ""
# Begin Group "mess-systems"

# PROP Default_Filter "*.c"
# Begin Source File

SOURCE=.\mess\systems\a2600.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\a310.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\a7800.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\advision.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\amiga.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\amstrad.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\apple1.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\apple2.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\aquarius.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\astrocde.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\atari.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\atom.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\bbc.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\c128.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\c16.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\c64.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\c65.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\cbmb.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\cgenie.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\coleco.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\coupe.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\cpschngr.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\dragon.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\enterp.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\gb.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\genesis.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\jupiter.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\kaypro.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\kim1.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\lisa.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\mac.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\mbee.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\mc10.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\microtan.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\msx.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\mtx.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\mz700.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\nascom1.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\nes.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\oric.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\p2000t.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\pc.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\pce.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\pcw.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\pcw16.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\pdp1.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\pet.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\pocketc.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\sms.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\spectrum.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\ti990_4.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\ti99_2.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\ti99_4x.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\trs80.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\uk101.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\vc20.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\vectrex.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\vtech1.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\vtech2.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\systems\zx.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\systems"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\systems"

!ENDIF 

# End Source File
# End Group
# Begin Group "mess-vidhrdw"

# PROP Default_Filter "*.c,*.h"
# Begin Source File

SOURCE=.\mess\vidhrdw\a7800.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\advision.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\amiga.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\amstrad.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\antic.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\apple1.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\apple2.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\aquarius.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\astrocde.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\atari.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\atari.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\atom.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\bbc.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\bbc.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\border.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\border.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\cgenie.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\cgenie.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\coleco.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\coupe.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\crtc6845.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\dragon.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\enterp.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\enterp.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\epnick.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\epnick.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\gb.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\genesis.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\genesis.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\gtia.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\hd6845s.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\hd6845s.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\jupiter.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\kaypro.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\kc.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\kc.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\kim1.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\M6845.C

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\M6845.H

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\m6847.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\m6847.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\mac.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\mbee.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\microtan.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\mz700.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\nascom1.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\nes.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\nick.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\oric.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\p2000m.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\pc_cga.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\pc_mda.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\pc_t1t.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\pcw.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\pcw16.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\pdp1.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\pdp1.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\pet.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\pocketc.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\praster.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\saa5050.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\smsvdp.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\smsvdp.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\spectrum.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\ted7360.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\tms9928a.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\tms9928a.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\trs80.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\ttchar.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\uk101.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\vdc.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\vdc.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\vdc8563.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\vectrex.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\vga.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\vic6560.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\vic6567.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\vtech1.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\vtech2.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\vidhrdw\zx.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\vidhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\vidhrdw"

!ENDIF 

# End Source File
# End Group
# Begin Group "mess-sndhrdw"

# PROP Default_Filter "*.c,*.h"
# Begin Source File

SOURCE=.\mess\sndhrdw\6581_.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\6581_.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\beep.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\cgenie.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\dave.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\dave.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\enve_dl.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\envelope.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\envelope.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\genesis.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\kaypro.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\mac.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\mixing.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\mixing.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\mytypes.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\mz700.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\opstruct.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\pc.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\samples.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\samples.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\scc.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\scc.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\sid6581.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\ted7360.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\vic6560.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\wave6581.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\sndhrdw\wave8580.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\sndhrdw"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\sndhrdw"

!ENDIF 

# End Source File
# End Group
# Begin Group "mess-machine"

# PROP Default_Filter "*.c,*.h"
# Begin Source File

SOURCE=.\mess\machine\6522via.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\6522via.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\a2600.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\a7800.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\advision.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\amiga.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\amstr_pc.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\amstrad.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\ap_disk2.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\apple1.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\apple2.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\aquarius.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\astrocde.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\at.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\atari.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\atari.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\atom.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\ay3600.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\bbc.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\bbc.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\c128.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\c1551.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\c16.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\c64.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\c65.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\cbm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\cbmb.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\cbmdrive.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\cbmieeeb.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\cgenie.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\cia6526.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\coleco.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\coupe.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\cpm_bios.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\cpm_bios.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\dma8237.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\dragon.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\dsk.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\enterp.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\enterp.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\flopdrv.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\gb.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\gb.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\genesis.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\genesis.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\i8271.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\i8271.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\iwm.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\iwm.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\iwm_lisa.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\iwm_lisa.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\jupiter.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\kaypro.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\kc.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\kim1.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\lisa.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\lisa.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\mac.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\mbee.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\mbee.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\mc10.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\mc146818.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\mc6850.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\mc6850.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\microtan.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\msx.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\msx.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\mz700.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\nascom1.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\NEC765.C

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\nes.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\nes_mmc.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\nes_mmc.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\oric.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\p2000t.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\pc.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\pc_fdc.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\pc_fdc_h.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\pc_flopp.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\pc_hdc.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\pc_ide.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\pc_mouse.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\pce.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\pckeybrd.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\pclpt.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\pdp1.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\pet.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\pic8259.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\pit8253.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\pocketc.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\riot.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\riot.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\sms.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\sms.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\spectrum.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\tandy1t.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\ti99_4x.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\ti99_4x.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\tia.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\tms9901.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\tms9901.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\tpi6525.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\trs80.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\uart8250.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\uk101.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\vc1541.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\vc20.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\vc20tape.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\vectrex.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\vtech1.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\vtech2.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\wd179x.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\wd179x.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\machine\zx.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess\machine"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess\machine\"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess\machine"

!ENDIF 

# End Source File
# End Group
# Begin Group "mess-includes"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\mess\includes\advision.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\amiga.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\amstr_pc.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\amstrad.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\apple1.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\apple2.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\aquarius.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\at.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\atom.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\beep.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\c128.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\c1551.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\c16.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\c64.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\c65.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\cbm.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\cbmb.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\cbmdrive.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\cbmieeeb.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\cia6526.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\coleco.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\coupe.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\crtc6845.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\dma8237.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\dragon.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\dsk.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\Flopdrv.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\flopdrv2.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\jupiter.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\kaypro.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\mac.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\mc146818.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\microtan.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\mz700.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\nascom1.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\nec765.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\nes.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\p2000t.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\pc.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\pc_cga.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\pc_fdc_h.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\pc_flopp.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\pc_mda.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\pc_mouse.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\pc_t1t.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\pckeybrd.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\pclpt.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\pcw.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\pcw16.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\pet.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\pic8259.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\pit8253.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\pocketc.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\praster.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\saa5050.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\sid6581.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\spectrum.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\tandy1t.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\ted7360.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\tpi6525.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\trs80.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\uart8250.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\uk101.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\vc1541.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\vc20.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\vc20tape.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\vdc8563.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\vga.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\vic4567.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\vic6560.h
# End Source File
# Begin Source File

SOURCE=.\mess\includes\vic6567.h
# End Source File
# End Group
# Begin Group "mess-formats"

# PROP Default_Filter "*.c, *.h"
# Begin Source File

SOURCE=.\mess\formats\cocopak.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj/mess/formats"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj/mess/formats"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj/mess/formats"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\formats\cocopak.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj/mess/formats"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj/mess/formats"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj/mess/formats"

!ENDIF 

# End Source File
# End Group
# Begin Group "mess-docs"

# PROP Default_Filter "*.txt, *.dat"
# Begin Source File

SOURCE=.\credits.txt
# End Source File
# Begin Source File

SOURCE=.\imgtool.txt
# End Source File
# Begin Source File

SOURCE=.\mess.txt
# End Source File
# Begin Source File

SOURCE=.\messnew.txt
# End Source File
# Begin Source File

SOURCE=.\messold.txt
# End Source File
# Begin Source File

SOURCE=.\sysinfo.dat
# End Source File
# End Group
# Begin Source File

SOURCE=.\mess\config.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\config.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\eventlst.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\eventlst.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\filemngr.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\mess.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\mess.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\system.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\tapectrl.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\types.h

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mess\win32.c

!IF  "$(CFG)" == "mame32 - Win32 Release"

# PROP Intermediate_Dir "mess32.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Debug"

# PROP Intermediate_Dir "mess32d.obj\mess"

!ELSEIF  "$(CFG)" == "mame32 - Win32 Hybrid"

# PROP Intermediate_Dir "mess32h.obj\mess"

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
