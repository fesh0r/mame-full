                   	---MAME32 Windows Help File---
                   	---Source Distribution Files--
                   	---As of MAME32 0.37 Beta 5---
			---------30 July 2000---------

			*******Disclaimer Begin*******
The help file source is being distributed free of charge as part of the MAME32 source code package.  If you wish to strip out the help file for use by itself, just extract these two files:
MAME32.HLP and MAME32.CNT.
			********Disclaimer End********


INDEX:

i.  Disclaimer
1.  Files needed to compile Windows Help for MAME32
	1.a  Source Files
	1.b  "Shed Graphic" Files
	1.c  Files created as part of the compile
2.  Distributing MAME32 Help Files
3.  My help compilation environment
4.  Assistance


1.  Files needed to compile the Windows Help for MAME32
------------------------------------------------------
To compile the help file, you will need the source files listed below as well as the Help Compiler for Windows (HCW.exe) version 4.x.  See "3. My help compilation environment" below for more information.

1.a  Source Files
-----------------
The following source files contain all the project information necessary to compile the MAME32 help file.

File Name			Description
---------			-----------
Game List as of 0.37.xls	Excel file that holds list of games for each version since version 0.33
MAME32.cnt			Help Contents File
MAME32.hpj			Help Project File
MAME32.rtf			MAME32 Help File Source (all topics in Rich Text Format)
README_HELP.TXT			This File


1.b  "Shed Graphic Files"
-------------------------
The following files are the "shed graphics" that are embedded in the help files.  These graphics are bitmaps that have been stored in .shg format.  They all contain "hotspots" that contain jumps and popups to various help topics throughout the Help Project.

File Name			Description
---------			-----------
MAMEAUDT.shg			Audit dialog graphic
MAMEFILE.shg			File Menu graphic
MAMEFILT.shg			Custom Filter dialog graphic
MAMEFOLD.shg			Folder List graphic
MAMEGAME.shg			Games Tab graphic
MAMEHELP.shg			Help Menu graphic
MAMEMENU.shg			Menu Options graphic
MAMEOPT.shg			Options Menu graphic
MAMEOPTD.shg			Options Directories graphic
MAMEPROP.shg			Properties Tabs graphic
MAMERES.shg			Reset to Default dialog graphic
MAMETOOL.shg			Toolbar graphic
MAMEVERS.shg			Startup Dialog to Upgrade Versions
MAMEVIEW.shg			View Menu graphic
MAMEVWAI.shg			View Arrange Icons graphic
MAMEVWCU.shg			View Customize Fields graphic


1.C  Files created as part of the compile
----------------------------------------------
After you compile and view the Help File, four new files will be present in the help file directory:

File Name			Description
---------			-----------
MAME32.hlp			Compiled Help File
MAME32.fts			MAME32 Full-Text Search File
MAME32.gid			MAME32 Glossary File
MAME32.txt			Log file from the Help Compiler (can be deleted)


2.  Distributing MAME32 Help Files
----------------------------------
Once you have compiled the Help File, you should distribute these two files:

File Name			Description
---------			-----------
MAME32.hlp			Compiled Help File
MAME32.cnt			Help Contents File


3.  My help compilation environment
-----------------------------------
To compile this help file, I have the following components on my desktop:

- Microsoft Word (Office 2000 Version)
- Microsoft Excel (Office 2000 Version) - to store game info
- Microsoft Help Workshop (as part of the Office Developer's Edition (ODE))
- Microsoft Shed Graphics Editor (distributed as part of the ODE)

You can substitute the following components:

- A help workbench package (such as RoboHelp [RoboHelp is a Copyright of Blue Sky Software])
- Microsoft WordPad or any other word processing program that can handle Rich Text, footnotes, endnotes, double-underline, and hidden text
- Microsoft Works or any spreadsheet program capable of reading Microsoft Excel 2000 files
- Microsoft's Help Compiler for Windows (HCW.exe) version 4.x - you can find this on Microsoft's WWW site
- Microsoft's Shed Graphics Editor (shed.exe) - you can find this on Microsoft's WWW site


4.  Assistance
--------------
If you have any questions about compiling the help file, please e-mail me at danmiller@mail.postnet.com.  Please keep your questions specific to compiling the MAME32 help file. 