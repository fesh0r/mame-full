                    M.E.S.S.  -  Multi-Emulator Super System
                      Copyright (C) 1998  by the MESS team

Please note that many people helped with this project, either directly or by
releasing source code which was used to write the drivers. We are not trying to
appropriate merit which isn't ours. See the acknowledgemnts section for a list
of contributors, however please note that the list is largely incomplete. See
also the CREDITS section in the emulator to see the people who contributed to a
specific driver. Again, that list might be incomplete. We apologize in advance
for any omission.

All trademarks cited in this document are property of their respective owners.


Usage and Distribution Licence
------------------------------

I. Purpose
----------
   MESS is strictly a no profit project. Its main purpose is to be a reference
   to the inner workings of the many existing console and computer systems.
   This is done for educational purposes and to preserve them from the oblivion
   they would sink into when the hardware they run on will stop working.
   Of course to preserve the systems you must also be able to actually use them;
   you can see that as a nice side effect.
   It is not our intention to infringe any copyrights or patents (pending or active)
   on the original games. All of the source code is either our own or freely
   available. To work, the emulator often requires ROMs, cartridges, disk images,
   and/or tape images for the original systems. No portion of the code of the
   original systems is included in the executable.

II. Cost
--------
   MESS is free. The source code is free. Selling it is not allowed. Charging
   for the use of MESS is not allowed.

III. Images
-----------
   You are not allowed to distribute MESS and copyrighted images (including but
   not limited to ROMs, cartridge images, disk images, and cassette images) on 
   the same physical medium. You are allowed to make them available for download
   on the same web site, but only if you warn users about the copyright status of
   the images and the legal issues involved. You are NOT allowed to make MESS
   available for download together with one giant big file containing any or all
   of the supported images, or any files containing multiple non-associated images.
   You are not allowed to distribute MESS in any form if you sell, advertise or
   publicize illegal CD-ROMs or other media containing illegal images. Note that
   the restriction applies even if you don't directly make money out of that.
   The restriction of course does not apply if the CD-ROMs are published by the
   copyright owners.

IV. Distribution Integrity
--------------------------
   MESS must be distributed only in the original archives. You are not allowed
   to distribute a modified version, nor to remove and/or add files to the
   archive. Adding one text file to advertise your web site is tolerated only
   if your site contributes original material to the emulation scene.

V. Source Code Distribution
---------------------------
   If you distribute the binary, you should also distribute the source code. If
   you can't do that, you must provide a pointer to a place where the source
   can be obtained.

VI. Reuse of Source Code
-------------------------
   This chapter might not apply to specific portions of MESS (e.g. CPU
   emulators) which bear different copyright notices.
   The source code cannot be used in a commercial product without a written
   authorization of the authors. Use in non commercial products is allowed and
   indeed encouraged; however if you use portions of the MESS source code in
   your program, you must make the full source code freely available as well.
   Derivative works are allowed (provided source code is available), but
   discouraged: MESS is a project continuously evolving, and you should, in
   your best interest, submit your contributions to the development team, so
   that they are integrated in the main distribution. Usage of the
   _information_ contained in the source code is free for any use. However,
   given the amount of time and energy it took to collect this information, we
   would appreciate if you made the additional information you might have
   freely available as well.



How to Contact Us
-----------------

Here are some of the people contributing to MESS. If you have comments,
suggestions or bug reports about an existing driver, check the driver's Credits
section to find who has worked on it, and send comments to that person. If you
are not sure who to contact, write to Brad (bradman@primenet.com) - he is the
current coordinator of the MESS project. If you have comments specific to a
given operating system, they should be sent to the respective port maintainer.
You will find their e-mail address in the "readme.1st" file that comes with MESS.

Brad Oliver           bradman@primenet.com (current coordinator)

Mike Balfour          mab22@po.cwru.edu
Richard Bannister     titan@indigo.ie
Juergen Buchmueller   pullmoll@t-online.de
Gareth Long           gatch@elecslns.demon.co.uk
Jeff Mitchell         skeezix@skeleton.org
Frank Palazzolo       palazzol@home.com
Mathis Rosenhauer     rosenhau@mailserv.sm.go.dlr.de
Chris Salomon         chrissalo@aol.com

DON'T SEND BINARY ATTACHMENTS WITHOUT ASKING FIRST, *ESPECIALLY* ROM IMAGES.

THESE ARE NOT SUPPORT ADDRESSES. Support questions sent to these addresses
*will* be ignored. Please understand that this is a *free* project, mostly
targeted at experienced users. We don't have the resources to provide end user
support. Basically, if you can't get the emulator to work, you are on your own.
First of all, read the docs carefully. If you still can't find an answer to
your question, try checking the beginner's sections that many emulation pages
have, or ask on the appropriate Usenet newsgroups (e.g. comp.emulators.misc)
or on the emulation message boards (e.g. http://www.escape.com/~ego/dave/console/).

Also, DO NOT SEND REQUESTS FOR NEW SYSTEMS TO ADD, unless you have some original
info on the hardware or, even better, have the technical expertise needed to
help us. Please don't send us information widely available on the Internet -
we are perfectly capable of finding it ourselves, thank you.

Usage
-----
Please see the "readme.1st" file that comes with the binary - it explains the
options that are specific to each platform.


Supported Systems
-----------------

Apple II Family:
----------------
Usage:
	See the "Keyboard Setup" menu, accessible by hitting TAB, for key mappings.

Compatibility:
	It boots up some disk images if you pick the Apple IIc driver. Others haven't
	been tested very extensively and probably won't work. This driver is still
	preliminary.
	
	
Astrocade:
----------
	
	
Atari 800:
----------
Usage:
	See the "Keyboard Setup" menu, accessible by hitting TAB, for key mappings.

Compatibility:
	It runs just about anything you throw at it.
	
	
Atari 5200:
-----------
Usage:
	See the "Keyboard Setup" menu, accessible by hitting TAB, for key mappings.

Compatibility:
	This driver is preliminary and won't run past the demo screen. Help is needed.
	
	
ColecoVision:
-------------
Usage:
	arrow keys - 1p move
	ctrl, alt - 1p fire
	1-0 - number keys on the Colecovision controller
	minus - '#' key on the Colecovision controller
	equals - '.' key on the Colecovision controller
	See the "Keyboard Setup" menu, accessible by hitting TAB, for more key mappings.

Compatibility:
	To the best of my knowledge, this emulation should run any cartridge
	accurately.

Notes:
	The Colecovision driver requires the presence of "COLECO.ROM".  The driver
	will not function without this ROM.  A real Colecovision also will not
	function without this ROM.

Thanks:
	Special thanks to Marat Fayzullin for providing loads of Colecovision
	information through the ColEm source.


Colour Genie:
-------------
Usage:
	keyboard - relatively close to the original layout.
	    Take a look into the "Options" menu, accessible by hitting
	    TAB, and choose "Keyboard Setup" to find out more about
	    the key mapping - changing is not yet supported.
	joystick - emulation of dual Colour Genie joysticks with keypads.
	cassette - emulation of virtual tapes supported.
	    Use SYSTEM or CLOAD commands to read images.
	    SYSTEM supports 6 character filenames, CLOAD loads
	    a file named BASIC[N].CAS, where [N] is the character
	    you supplied (e.g. CLOAD"M" loads BASICM.CAS)
	floppies - emulation of virtual floppy discs supported.
	    Use CMD"I0" to "CMDI3" to get an inventory of drive 0 to 3,
	    use CMD"S FILENAME/CMD" to start a binary executable or use
	    LOAD"FILENAME/BAS" to load a basic program.

Options:
Under the "Options" menu, accessible by hitting TAB, there are three settings.
	"Floppy Disc Drives"  - enable or disable floppy disc controller.
	"DOS ROM C000-DFFF"   - enable 8K DOS ROM or make it RAM.
	"EXT ROM E000-EFFF"   - enable 4K Extension ROM or make it RAM.

Compatibility:
	The Colour Genie driver should run most of the known programs out there.
	It supports the Motorola 6845 CRT controller with text and graphics
	modes (LGR and FGR), the AY-3-8910 sound chip with three audio channels
	and noise and the WD 179x floppy disc controller with up to four
	virtual floppy disc drives contained in image files.

Known issues:
	Startup with CAS or CMD images does not always work. Use the BASIC
	SYSTEM or CLOAD commands to read cassette image files, or use
	the Colour Genie DOS ROM with floppy disc images to run programs.
	The driver does not yet emulate the printer port mode for AY-3-8910.
	Right now it always uses the AY-3-8910 ports for joystick emulation.

Notes:
The Colour Genie driver requires the presence of the following images:
	CGENIE.ROM      16K Basic and BIOS.
	CGENIE1.FNT     8x8 default character set with graphics.
To access the virtual floppy disc capabilities you need:
	CGDOS.ROM       8K Disk Operation System.
And as an add-on "DOS Interface" you can use:
	NEWE000.ROM     4K Extension


Genesis (MegaDrive in Japan/Europe):
------------------------------------
Usage:
	arrow keys - 1p move
	control, alt - 1p fire
	1,2 - 1, 2 player select
	return - 1p start
	See the "Keyboard Setup" menu, accessible by hitting TAB, for more key mappings.

Options:
	Under the "Options" menu, accessible by hitting TAB, you will find the
	following option:

	"Country" - this allows you to alter the 'flavour' of Genesis, to either European,
	            Japanese or American. Many cartridges compare their country codes with
	            that of the Genesis itself and may behave differently, or lock up if the
	            two types do not match.
	            'Auto' attempts to auto-set the Genesis country code to the value the
	            cartridge will appear to be checking for. This may not be 100% accurate,
	            however.

Compatibility:
	Most games tend to run fairly well. some with slight graphic glitches, incorrect
	colours part way down the screen or other b'zarre effects.

	Cartridges up to 32MBit (4MByte) are supported.

	The controller may not work well with some games. This will be resolved when I
	obtain more information. Similarly, 6-button joypads aren't emulated yet.

	Split screen effects and interlacing are not yet supported.

	Sprite/layer priority should be perfect.

	Sound is emulated at Z80 and PSG76489 level. There is preliminary YM2612 support.

Notes:
	I still consider the Genesis driver extremely preliminary. I have a lot of further
	work to do before I consider it anywhere near complete. My initial goal is to make
	it as compatible as possible. Currently there is scope within the driver to handle
	split screen colour changes/any VDP effect, interlacing, without rewriting. The
	graphics renderer emulates VDP at scanline level, and does not use tile-based
	methods.
	
Wanted:
	Any Sega Megadrive information! Information is extremely sparse; typically the same
	three-four documents, with parts incomplete, conflicting, and corrupted. Specifically,
	some of the finer points of DMA transfer, and access of the 68K memory map by the Z80.
	Generally, points which conflict with themselves in the documentation available on
	the net...

Thanks:
	...which I'm thankful for, as there certainly isn't any source available to look at!
	Thanks to the rest of the MESS team, Terence & Philip and the MAME team, Kevin Lingley
	for support and ex(p|t)ensive games testing in this driver's ARM code & early forms,
	and, of course, Stan, Kyle, Eric, Kenny...
	
	Please note that no goats were sacrificed during the development of this driver. 


Kaypro CP/M:
------------


NES:
----
Usage:
	arrow keys - 1p move
	control, alt - 1p fire
	1,2 - 1, 2 player select
	return - 1p start
	See the "Keyboard Setup" menu, accessible by hitting TAB, for more key mappings.
	
Options:
	Under the "Options" menu, accessible by hitting TAB, are two settings.
	"Renderer" - can be set to Scanline or Experimental. The Scanline renderer gives
	             the most accurate display, but can be slow. The Experimental
	             option is much faster, but still needs quite a bit of work before it
	             displays properly in all cases. It's best used for games with no
	             scrolling playfields, like Donkey Kong, etc.
	"Split-Screen fix" - defaults to off. Turn it on to see proper split screens in
	             games like Kirby's Adventure and Airwolf. This will most likely go
	             away in the future as we figure out how these split screens really work.

Compatibility:
    The NES driver should run 99% of the ROMs currently out there with very little problem.
    It doesn't yet support a lot of the obscure mappers used in fwNES, but this is on deck
    for the next release. For the record, it supports mappers 1, 2, 3, 4, 5 (incomplete),
    7, 8, 9 & 10 (incomplete), 11, 15, 16, 18 (vrom probs), 25 (vrom probs), 33, 34,
    64, 65, 66, 68 (incomplete), 69 (incomplete), 71, 78, 79.
    Some of the mappers > 63 haven't been fully tested yet.
    
Notes:
    The main focus up until this point has been in getting the NES driver as accurate
    as possible. Unfortunately, the scanline rendering method is quite slow, so future
    efforts will be focusing on bringing up the speed. Also, the sound code is still a
    bit shaky. However, the NES driver does have quite accurate sample playback. For example,
    you can hear speech samples in Bayou Billy, Gauntlet, Dirty Harry, and Skate or Die
    that are not present in a few other NES emulators.
    
Wanted:
	I'd appreciate any info on some of the more obscure mappers. If you have
	any of this, please drop me a line at bradman@primenet.com.
	
Thanks:
	Special thanks to Nicolas Hamel for xNES. Also, thanks to Marat Fayzullin, D,
	Icer Addis, Matt Conte, Arthur Langereis, and John Stiles for tips and moral support during
	the development of the NES driver. And last but not least, thanks to Jeremy Chadwick
	and Firebug for their respective NES docs, without which this driver would have taken
	considerably longer. I'll be sending you guys updates for both files soon. ;)


PDP/1:
------


Sega MasterSystem/GameGear:
---------------------------
Usage:
	arrow keys - 1p move
	control, alt - 1p fire
	1,2 - 1, 2 player select
	return - 1p start
	See the "Keyboard Setup" menu, accessible by hitting TAB, for more key mappings.
	
Compatibility:
    It's not very compatible right now. The code is still quite preliminary. It should run
    most of the ROMs, but quite a few exhibit graphics glitches. The MasterSystem has
    totally incorrect color also. Sound emulation appears to be near perfect though.
    
Wanted:
	We'd like some spare time to work on this. Care to donate any? :)
	
Thanks:
	Thanks to Marat Fayzullin for his MasterGear emulation.


TRS-80:
-------
Usage:
	keyboard - relatively close to the original layout.
	    Take a look into the "Options" menu, accessible by hitting
	    TAB, and choose "Keyboard Setup" to find out more about
	    the key mapping - changing is not yet supported.
	cassette - emulation of virtual tapes supported.
	    Use SYSTEM or CLOAD commands to read images.
	    SYSTEM supports 6 character filenames and loads the
	    corresponding FILENM.CAS image, CLOAD loads a file named
	    BASIC[N].CAS, where [N] is the character you supplied
	    (e.g. CLOAD"M" loads BASICM.CAS)
	floppies - emulation of virtual floppy discs supported.
	    Use a NEWDOS/80 boot disk for drive 0 and set up correct
	    PDRIVE parameters for the other drives.
	    If only drives 0 to 2 are used they can be double sided
	    by interpreting drive select 3 as head select bit.

Options:
Under the "Options" menu, accessible by hitting TAB, there are three settings.
	"Floppy Disc Drives"  - enable or disable floppy disc controller.
	"Video RAM"           - enable upper case only or upper/lower font.
	"Virtual E000-EFFF"   - enable 4K Extension ROM or make it RAM.

Compatibility:
	The TRS-80 driver should run most of the known programs out there.
	It supports a WD 179x floppy disc controller with up to four
	virtual floppy disc drives contained in image files.

Notes:
The TRS-80 driver requires the presence of the following images:
	TRS80.ROM         12K Level II Basic.
	TRS80M1.CHR       6x15 default character set with block graphics.


Vectrex:
--------
Usage:
	See the "Keyboard Setup" menu, accessible by hitting TAB, for key mappings.

Compatibility:
	To the best of our knowledge, this emulation should run most cartridges
	accurately.

Notes:
	The Vectrex driver requires the presence of "SYSTEM.IMG".  The driver
	will not function without this ROM.  A real Vectrex also will not
	function without this ROM. Also, overlay support isn't present but is
	being worked on for a future release.

Thanks:
	Special thanks to Keith Wilkins and Chris Salomon for their work on DVE
	and their help,	without which this driver would have taken much longer.



Acknowledgments
---------------

First off, we'd like to thank Nicola Salmoria and the MAME team for letting us
play with their code. Without it, MESS would be substantially less than what
it is right now.

Z80Em Portable Zilog Z80 Emulator Copyright (C) Marcel de Kogel 1996,1997
   Note: the version used in MESS is slightly modified. You can find the
   original version at http://www.komkon.org/~dekogel/misc.html.
M6502 Emulator Copyright (C) Marat Fayzullin, Alex Krasivsky 1996
   Note: the version used in MESS is slightly modified. You can find the
   original version at http://freeflight.com/fms/.
I86 emulator by David Hedley, modified by Fabrice Frances (frances@ensica.fr)
M6809 emulator by John Butler, based on L.C. Benschop's 6809 Simulator V09.
M68000 emulator taken from the System 16 Arcade Emulator by Thierry Lescot.
8039 emulator by Mirko Buffoni, based on 8048 emulator by Dan Boris.
TMS5220 emulator by Frank Palazzolo.
AY-3-8910 emulation based on various code snippets by Ville Hallik,
  Michael Cuddy, Tatsuyuki Satoh, Fabrice Frances, Nicola Salmoria.
YM-2203, YM-2151, YM-2608 and YM-2612 emulation by Tatsuyuki Satoh.
OPL based YM-2203 emulation by Ishmair (ishmair@vnet.es).
POKEY emulator by Ron Fries (rfries@aol.com).
Many thanks to Eric Smith, Hedley Rainne and Sean Trowbridge for information
   on the Pokey random number generator.
NES sound hardware info by Jeremy Chadwick and Hedley Rainne.

Allegro library by Shawn Hargreaves, 1994/97
SEAL Synthetic Audio Library API Interface Copyright (C) 1995, 1996
   Carlos Hasan. All Rights Reserved.
Video modes created using Tweak 1.6b by Robert Schmidt, who also wrote
   TwkUser.c.
"inflate" code for zip file support by Mark Adler.

