@ECHO OFF
REM
REM Install batch script for the Auxiliary MAME32 OPL Access Driver
REM
IF "%OS%" == "Windows_NT" GOTO WINDOWSNT
ECHO The MAMEOPL driver is only required for Windows NT.
GOTO EXIT
:WINDOWSNT
IF "%1" == "install" GOTO INSTOPL
IF "%1" == "remove"  GOTO REMOPL
:HELP
ECHO Usage: %0 install - install the MAMEOPL driver permanently
ECHO        %0 remove  - remove MAMEOPL driver from the system
GOTO EXIT
:INSTOPL
ECHO Installing the Auxiliary MAME32 OPL Access Driver...
RUNDLL32.EXE SETUPAPI,InstallHinfSection DefaultInstall 132 .\MAMEOPL.INF
NET START MAMEOPL
GOTO EXIT
:REMOPL
ECHO Removing the Auxiliary MAME32 OPL Access Driver...
NET STOP MAMEOPL
RUNDLL32.EXE SETUPAPI,InstallHinfSection DefaultRemove 132 .\MAMEOPL.INF
:EXIT