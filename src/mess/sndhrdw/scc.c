/*
**
** File: scc.c -- software implementation of the Konami SCC
** emulator. More than one SCC supported, and SCC+ as well as
** normal megarom SCC.
**
** For information on the scc, see:
**    http://www.msxnet.org/tech/scc.html
** and
**    http://www.msxnet.org/tech/SoundCartridge.html
**
** Todo:
** Check the sound emulation itself (scc-u.c)
**
** By Sean Young <sean@msxnet.org>
*/

#include "driver.h"
#include "scc.h"

/* the struct */
struct KONSCC {
    UINT8	Regs[17] ;
    UINT8 	Waves[5*32] ;

    double	UpdateStep;
    int		SampleRate;
    int		Channel;

    /* state variables */
    int         Counter0, Counter1, Counter2, Counter3, Counter4 ;
};

static struct KONSCC SCC[MAX_SCC];

/* the registers */
#define SCC_1FINE       (0)
#define SCC_1COARSE     (1)
#define SCC_2FINE       (2)
#define SCC_2COARSE     (3)
#define SCC_3FINE       (4)
#define SCC_3COARSE     (5)
#define SCC_4FINE       (6)
#define SCC_4COARSE     (7)
#define SCC_5FINE       (8)
#define SCC_5COARSE     (9)
#define SCC_1VOL        (10)
#define SCC_2VOL        (11)
#define SCC_3VOL        (12)
#define SCC_4VOL        (13)
#define SCC_5VOL        (14)
#define SCC_ENABLE      (15)
#define SCC_DEFORM      (16)

/*
** reset all chip registers.
*/
void SCCResetChip(int num)
{
    int i;

    /* initialize hardware registers */
    memset (SCC[num].Regs, 0, 17) ;
    for( i=10; i<15; i++ ) SCC[num].Regs[i] = 10;

    SCC[num].Counter0 = SCC[num].Counter1 = SCC[num].Counter2 = 0;
    SCC[num].Counter3 = SCC[num].Counter4 = 0;

    /* waves memory is not reset */
}

void SCCWriteReg (int n, int r, int v, int t)
{
    //char buf[40];
    static const int Mask[16] = { 0xff, 0xf, 0xff, 0xf, 0xff, 0xf,
	   0xff, 0xf, 0xff, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0x1f } ;
    r &= 0xff;
    v &= 0xff;

    switch (t) {
	case SCC_MEGAROM:
	case SCC_PLUSCOMP:
	    if (r < 0x80) {
		/* 0 - 0x7f is wave forms, 0x60-0x7f for channel 4 and 5 */
		SCC[n].Waves[r] = v ;
		if (r >= 0x60) {
		    SCC[n].Waves[r + 0x20] = v ;
		}
	    } else if (r < 0xa0) {
		/* 9880-988f = 9890-989f */
		SCC[n].Regs[r & 15] = (v & Mask[r & 15]) ;
	    } else switch (t) {
		case SCC_MEGAROM:
		    if (r >= 0xe0) {
			SCC[n].Regs[SCC_DEFORM] = v ;
			if (v && errorlog) fprintf (errorlog,
			    "SCC: %02xh written to unemulated register\n", v);

		    }
		    break ;
		case SCC_PLUSCOMP:
		    if ( (r < 0xe0) && (r >= 0xc0) ) {
			SCC[n].Regs[SCC_DEFORM] = v ;
			if (v && errorlog) fprintf (errorlog,
			    "SCC: %02xh written to unemulated register\n", v);
		    }
		    break ;
		}
	    break ;
	case SCC_PLUSEXT:
	    if (r < 0xa0) {
		SCC[n].Waves[r] = v ;
	    } else if (r < 0xc0) {
		SCC[n].Regs[r & 15] = (v & Mask[r & 15]) ;
	    } else if (r < 0xe0) {
		SCC[n].Regs[SCC_DEFORM] = v ;
		if (v && errorlog) fprintf (errorlog,
		    "SCC: %02xh written to unemulated register\n", v);
	    }
	    break ;
    }
}

void SCCUpdate16 (int n, void *buffer, int length) {
#define DATATYPE UINT16
#define DATACONV(A) ( (A) * 3)
#include "scc-u.c"
#undef DATACONV
#undef DATATYPE
}

void SCCUpdate8 (int n, void *buffer, int length) {
#define DATATYPE UINT8
#define DATACONV(A) ( (A) / 75)
#include "scc-u.c"
#undef DATACONV
#undef DATATYPE
}

void SCCSetClock (int chip, int clock) {
    SCC[chip].UpdateStep = 512.0 * (double)clock /
	((double)SCC[chip].SampleRate / 4.0);
}

static int SCCInit(const struct MachineSound *msound,int chip,
                int clock,int volume,int sample_rate,int sample_bits) {
    char name[40];
    //int vol;

    memset (&SCC[chip],0,sizeof (struct KONSCC) );
    SCC[chip].SampleRate = sample_rate;

    sprintf (name, "Konami SCC/SCC+ #%d",chip);

    SCC[chip].Channel = stream_init (name,volume,sample_rate,sample_bits,chip,
	(sample_bits == 16) ? SCCUpdate16 : SCCUpdate8);

    if (SCC[chip].Channel == -1) return 1;

    SCCSetClock (chip, clock);
    SCCResetChip (chip);

    return 0;
}

int SCC_sh_start (const struct MachineSound *msound) {
    int chip;
    //const struct CustomSound_interface *intf = msound->sound_interface;

    for (chip=0;chip<1;chip++) {
	if (SCCInit (msound,chip,3579545, 20,
		Machine->sample_rate, Machine->sample_bits) )
		return 1;
	}
    return 0;
}
