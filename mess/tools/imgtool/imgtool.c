#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "osdepend.h"
#include "imgtoolx.h"
#include "osd_cpu.h"
#include "crcfile.h"
#include "utils.h"
#include "library.h"

/* Arbitrary */
#define MAX_OPTIONS	32

/* ----------------------------------------------------------------------- */

#if 0
IMAGEMODULE_EXTERN(coco_rsdos);			/* CoCo RS-DOS disks */
IMAGEMODULE_EXTERN(cococas);			/* CoCo cassettes */
IMAGEMODULE_EXTERN(concept);			/* Concept Disks */
IMAGEMODULE_EXTERN(msdos);				/* FAT/MSDOS diskett images */
IMAGEMODULE_EXTERN(msdoshd);			/* FAT/MSDOS harddisk images */
IMAGEMODULE_EXTERN(lynx);				/* c64 archive */
IMAGEMODULE_EXTERN(t64);				/* c64 archive */
IMAGEMODULE_EXTERN(d64);				/* commodore sx64/vc1541/2031/1551 diskettes */
IMAGEMODULE_EXTERN(x64);				/* commodore vc1541 diskettes */
IMAGEMODULE_EXTERN(d71);				/* commodore 128d/1571 diskettes */
IMAGEMODULE_EXTERN(d81);				/* commodore 65/1565/1581 diskettes */
IMAGEMODULE_EXTERN(c64crt);				/* c64 cartridge */
IMAGEMODULE_EXTERN(vmsx_tap);			/* vMSX .tap archiv */
IMAGEMODULE_EXTERN(vmsx_gm2);			/* vMSX gmaster2.ram file */
IMAGEMODULE_EXTERN(fmsx_cas);			/* fMSX style .cas file */
/* IMAGEMODULE_EXTERN(svi_cas);		 */	/* SVI .cas file */
IMAGEMODULE_EXTERN(xsa);				/* XelaSoft Archive */
IMAGEMODULE_EXTERN(msx_img);			/* bogus MSX images */
IMAGEMODULE_EXTERN(msx_ddi);			/* bogus MSX images */
IMAGEMODULE_EXTERN(msx_msx);			/* bogus MSX images */
IMAGEMODULE_EXTERN(msx_mul);			/* bogus MSX images */
IMAGEMODULE_EXTERN(rom16);
IMAGEMODULE_EXTERN(nccard);				/* NC100/NC150/NC200 PCMCIA Card ram image */
IMAGEMODULE_EXTERN(ti85p);				/* TI-85 program file */
IMAGEMODULE_EXTERN(ti85s);				/* TI-85 string file */
IMAGEMODULE_EXTERN(ti85i);				/* TI-85 picture file */
IMAGEMODULE_EXTERN(ti85n);		/* TI-85 real number file */
IMAGEMODULE_EXTERN(ti85c);		/* TI-85 complex number file */
IMAGEMODULE_EXTERN(ti85l);		/* TI-85 list file */
IMAGEMODULE_EXTERN(ti85k);		/* TI-85 constant file */
IMAGEMODULE_EXTERN(ti85m);		/* TI-85 matrix file */
IMAGEMODULE_EXTERN(ti85v);		/* TI-85 vector file */
IMAGEMODULE_EXTERN(ti85d);		/* TI-85 graphics database file */
IMAGEMODULE_EXTERN(ti85e);		/* TI-85 equation file */
IMAGEMODULE_EXTERN(ti85r);		/* TI-85 range settings file */
IMAGEMODULE_EXTERN(ti85g);		/* TI-85 grouped file */
IMAGEMODULE_EXTERN(ti85);		/* TI-85 file */
IMAGEMODULE_EXTERN(ti85b);		/* TI-85 memory backup file */
IMAGEMODULE_EXTERN(ti86p);		/* TI-86 program file */
IMAGEMODULE_EXTERN(ti86s);		/* TI-86 string file */
IMAGEMODULE_EXTERN(ti86i);		/* TI-86 picture file */
IMAGEMODULE_EXTERN(ti86n);		/* TI-86 real number file */
IMAGEMODULE_EXTERN(ti86c);		/* TI-86 complex number file */
IMAGEMODULE_EXTERN(ti86l);		/* TI-86 list file */
IMAGEMODULE_EXTERN(ti86k);		/* TI-86 constant file */
IMAGEMODULE_EXTERN(ti86m);		/* TI-86 matrix file */
IMAGEMODULE_EXTERN(ti86v);		/* TI-86 vector file */
IMAGEMODULE_EXTERN(ti86d);		/* TI-86 graphics database file */
IMAGEMODULE_EXTERN(ti86e);		/* TI-86 equation file */
IMAGEMODULE_EXTERN(ti86r);		/* TI-86 range settings file */
IMAGEMODULE_EXTERN(ti86g);		/* TI-86 grouped file */
IMAGEMODULE_EXTERN(ti86);		/* TI-86 file */
IMAGEMODULE_EXTERN(ti99_old);	/* TI99 floppy (old MESS format) */
IMAGEMODULE_EXTERN(v9t9);		/* TI99 floppy (V9T9 format) */
IMAGEMODULE_EXTERN(pc99fm);		/* TI99 floppy (PC99 FM format) */
IMAGEMODULE_EXTERN(pc99mfm);	/* TI99 floppy (PC99 MFM format) */
IMAGEMODULE_EXTERN(ti99hd);		/* TI99 hard disk */
IMAGEMODULE_EXTERN(ti990dsk);	/* TI990 disk */
IMAGEMODULE_EXTERN(mac);		/* macintosh disk image */
IMAGEMODULE_EXTERN(sord_cas);	/* Sord M5 cassettes */

static const ImageModule_ctor module_ctors[] =
{
	IMAGEMODULE_DECL(coco_rsdos),
	IMAGEMODULE_DECL(cococas),
	IMAGEMODULE_DECL(concept),
	IMAGEMODULE_DECL(msdos),
	IMAGEMODULE_DECL(msdoshd),
	IMAGEMODULE_DECL(nes),
	IMAGEMODULE_DECL(a5200),
	IMAGEMODULE_DECL(a7800),
	IMAGEMODULE_DECL(advision),
	IMAGEMODULE_DECL(astrocde),
	IMAGEMODULE_DECL(c16),
	IMAGEMODULE_DECL(c64crt),
	IMAGEMODULE_DECL(t64),
	IMAGEMODULE_DECL(lynx),
	IMAGEMODULE_DECL(d64),
	IMAGEMODULE_DECL(x64),
	IMAGEMODULE_DECL(d71),
	IMAGEMODULE_DECL(d81),
	IMAGEMODULE_DECL(coleco),
	IMAGEMODULE_DECL(gameboy),
	IMAGEMODULE_DECL(gamegear),
	IMAGEMODULE_DECL(genesis),
	IMAGEMODULE_DECL(max),
	IMAGEMODULE_DECL(msx),
	IMAGEMODULE_DECL(pdp1),
	IMAGEMODULE_DECL(plus4),
	IMAGEMODULE_DECL(sms),
	IMAGEMODULE_DECL(ti99_4a),
	IMAGEMODULE_DECL(vc20),
	IMAGEMODULE_DECL(vectrex),
	IMAGEMODULE_DECL(vic20),
	IMAGEMODULE_DECL(vmsx_tap),
	IMAGEMODULE_DECL(vmsx_gm2),
	IMAGEMODULE_DECL(fmsx_cas),
	IMAGEMODULE_DECL(msx_img),
	IMAGEMODULE_DECL(msx_ddi),
	IMAGEMODULE_DECL(msx_msx),
	IMAGEMODULE_DECL(msx_mul),
	IMAGEMODULE_DECL(xsa),
/*	IMAGEMODULE_DECL(svi_cas),  -- doesn't work yet! */
	IMAGEMODULE_DECL(rom16),
	IMAGEMODULE_DECL(nccard),
	IMAGEMODULE_DECL(ti85p),
	IMAGEMODULE_DECL(ti85s),
	IMAGEMODULE_DECL(ti85i),
	IMAGEMODULE_DECL(ti85n),
	IMAGEMODULE_DECL(ti85c),
	IMAGEMODULE_DECL(ti85l),
	IMAGEMODULE_DECL(ti85k),
	IMAGEMODULE_DECL(ti85m),
	IMAGEMODULE_DECL(ti85v),
	IMAGEMODULE_DECL(ti85d),
	IMAGEMODULE_DECL(ti85e),
	IMAGEMODULE_DECL(ti85r),
	IMAGEMODULE_DECL(ti85g),
	IMAGEMODULE_DECL(ti85),
	IMAGEMODULE_DECL(ti85b),
	IMAGEMODULE_DECL(ti86p),
	IMAGEMODULE_DECL(ti86s),
	IMAGEMODULE_DECL(ti86i),
	IMAGEMODULE_DECL(ti86n),
	IMAGEMODULE_DECL(ti86c),
	IMAGEMODULE_DECL(ti86l),
	IMAGEMODULE_DECL(ti86k),
	IMAGEMODULE_DECL(ti86m),
	IMAGEMODULE_DECL(ti86v),
	IMAGEMODULE_DECL(ti86d),
	IMAGEMODULE_DECL(ti86e),
	IMAGEMODULE_DECL(ti86r),
	IMAGEMODULE_DECL(ti86g),
	IMAGEMODULE_DECL(ti86),
	IMAGEMODULE_DECL(ti99_old),
	IMAGEMODULE_DECL(v9t9),
	IMAGEMODULE_DECL(pc99fm),
	IMAGEMODULE_DECL(pc99mfm),
	IMAGEMODULE_DECL(ti99hd),
	IMAGEMODULE_DECL(ti990dsk),
	IMAGEMODULE_DECL(mac),
	IMAGEMODULE_DECL(sord_cas)
};
#endif

/* ----------------------------------------------------------------------- */


