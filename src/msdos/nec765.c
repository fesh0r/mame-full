/****************************************************************

        nec765.c

 allow direct access to a PCs NEC 765 floppy disc controller

 !!!! this is ALPHA - be careful !!!!

*****************************************************************/

#include <go32.h>
#include <pc.h>
#include <dpmi.h>
#include <time.h>
#include "driver.h"
#include "machine\wd179x.h"

#define OPTIMIZE_XLT

typedef struct  {
        unsigned drive_0:1;             /* drive 0 in seek mode / busy */
        unsigned drive_1:1;             /* drive 1 in seek mode / busy */
        unsigned drive_2:1;             /* drive 2 in seek mode / busy */
        unsigned drive_3:1;             /* drive 3 in seek mode / busy */
        unsigned io_active:1;           /* read or write command in progress */
        unsigned non_dma_mode:1;        /* fdc is in non DMA mode if set */
        unsigned read_from_fdc:1;       /* request for master (1 fdc->cpu, 0 cpu->fdc) */
        unsigned ready:1;               /* data reg ready for i/o */
}       STM;

typedef struct  {
        unsigned unit_select:2;         /* unit select */
        unsigned head:1;                /* head */
        unsigned not_ready:1;           /* not ready */
        unsigned equip_check:1;         /* equipment check (drive fault) */
        unsigned seek_end:1;            /* seek end */
        unsigned int_code:2;            /* interrupt code (0: ok, 1:abort, 2:ill cmd, 3:ready chg) */
}       ST0;

typedef struct  {
        unsigned missing_AM:1;          /* missing address mark */
        unsigned not_writeable:1;       /* not writeable */
        unsigned no_data:1;             /* no data */
        unsigned x3:1;                  /* always 0 */
        unsigned overrun:1;             /* overrun */
        unsigned data_error:1;          /* data error */
        unsigned x6:1;                  /* always 0 */
        unsigned end_of_cylinder:2;     /* end of cylinder */
}       ST1;

typedef struct  {
        unsigned missing_DAM:1;         /* missing (deleted) data address mark */
        unsigned bad_cylinder:1;        /* bad cylinder (cylinder == 255) */
        unsigned scan_not_satisfied:1;  /* scan not satisfied */
        unsigned scan_equal_hit:1;      /* scan equal hit */
        unsigned wrong_cylinder:1;      /* wrong cylinder (mismatch) */
        unsigned data_error:1;          /* data error */
        unsigned control_mark:1;        /* control mark (deleted address mark) */
        unsigned x7:1;                  /* always 0 */
}       ST2;

typedef struct  {
        unsigned unit_select:2;         /* unit select */
        unsigned head:1;                /* head address */
        unsigned two_side:1;            /* two side */
        unsigned track_0:1;             /* track 0 */
        unsigned ready:1;               /* ready */
        unsigned write_protect:1;       /* write protect */
        unsigned fault:1;               /* fault */
}       ST3;


typedef struct  {
        ST0      st0;                  /* status 0 */
        ST1      st1;                  /* status 1 */
        ST2      st2;                  /* status 2 */
        byte     c;                    /* cylinder */
        byte     h;                    /* head */
        byte     s;                    /* sector */
        byte     n;                    /* sector length */
        ST3      st3;                  /* status 3 (after sense drive status cmd) */
        byte     pcn;
}       STA;

typedef struct  {
        byte     unit;   /* unit number 0 = A:, 1 = B: */
        byte     type;   /* type of drive 0 = 360K, 1 = 720K, 2 = 1.2M, 3 = 1.44M */
        byte     eot;    /* end of track (sectors per track - 1 in our case) */
        byte     secl;   /* sector length code (1 for 256 bytes/sector) */
        byte     gap2;   /* read/write gap II */
        byte     gap3;   /* format gap III */
        byte     dtl;    /* data length (if secl == 0) */
        byte     filler; /* format filler byte (E5) */
        byte     den;    /* 0x00 = single, 0x40 = double density */
        byte     rcmd;   /* read command (0x26 normal, 0x2C deleted address mark) */
        byte     wcmd;   /* write command (0x05 normal, 0x09 deleted address mark) */
        byte     dstep;  /* double step shift factor (1 for 40 track disk in 80 track drive) */
        STM      stm;    /* main status */
        STA      sta;    /* status file */
}       NEC;

FILE    *log = 0;
NEC     nec;
byte    unit_type[4] = {0xff,0xff,0xff,0xff};

static __dpmi_paddr old_irq_E;  /* previous interrupt vector 0E */
static __dpmi_paddr new_irq_E;  /* new interrupt vector 0E */
static byte irq_flag = 0;       /* set by IRQ 6 (INT 0E) */

/* convert NEC 765 status to WD179X status 1 (restore, seek, step) */
static  byte    FDC_STA1(void)
{
byte    sta = 0;

        if (nec.sta.st3.track_0)        /* track 0 */
                sta |= STA_1_TRACK0;
        if (nec.sta.st3.write_protect)  /* write protect */
                sta |= STA_1_WRITE_PRO;
        if (!nec.sta.st3.ready)         /* not ready */
                sta |= STA_1_NOT_READY;

        return sta;
}

/* convert NEC 765 status to WD179X status 2 (read, write, format) */
static  byte    FDC_STA2(int inp)
{
byte    sta = 0;

        if (nec.sta.st1.overrun)        /* overrun ? */
                sta |= STA_2_LOST_DAT;
        if (nec.sta.st1.data_error ||   /* data error (DAM) ? */
            nec.sta.st2.data_error)     /* data error (SEC) ? */
                sta |= STA_2_CRC_ERR;
        if (nec.sta.st1.no_data ||      /* no data ? */
            nec.sta.st1.missing_AM)     /* missing address mark ? */
                sta |= STA_2_REC_N_FND;
        if (nec.sta.st2.control_mark)   /* control mark ? */
                sta |= STA_2_REC_TYPE;
        if (nec.sta.st1.not_writeable)  /* not writeable ? */
                sta |= STA_2_WRITE_PRO;
        if (nec.sta.st0.int_code == 3)  /* interrupt code == ready change ? */
                sta |= STA_2_NOT_READY;

        if (((sta & ~STA_2_REC_TYPE) == 0x00) && (inp))
                sta |= STA_2_DRQ | STA_2_BUSY;

        return sta;
}

static void dma_read(int ofs, int count)
{
        if (log) fprintf(log, "dma_read %08X %04X\n", ofs, count);
        outportb(0x0a, 0x04 | 0x02);            /* mask DMA channel 2 */
        outportb(0x0b, 0x46);                   /* DMA read command */
        outportb(0x0c, 0);                      /* reset first/last flipflop */
        outportb(0x05, (count - 1) & 255);      /* transfer size low */
        outportb(0x05, (count - 1) / 256);      /* transfer size high */
        outportb(0x04, ofs & 255);              /* transfer offset low */
        outportb(0x04, (ofs / 256) & 255);      /* transfer offset high */
        outportb(0x81, (ofs / 65536));          /* select 64k page */
        outportb(0x0a, 0x02);                   /* start DMA channel 2 */
}

static void dma_write(int ofs, int count)
{
        if (log) fprintf(log, "DMA write %08X %04X\n", ofs, count);
        outportb(0x0a, 0x04 | 0x02);            /* mask DMA channel 2 */
        outportb(0x0b, 0x4a);                   /* DMA write command */
        outportb(0x0c, 0);                      /* reset first/last flipflop */
        outportb(0x05, (count - 1) & 255);      /* transfer size low */
        outportb(0x05, (count - 1) / 256);      /* transfer size high */
        outportb(0x04, ofs & 255);              /* transfer offset low */
        outportb(0x04, (ofs / 256) & 255);      /* transfer offset high */
        outportb(0x81, (ofs / 65536));          /* select 64k page */
        outportb(0x0a, 0x02);                   /* start DMA channel 2 */
}

static  int     main_status(void)
{
uclock_t timeout;
uclock_t utime = uclock();

        timeout = utime + UCLOCKS_PER_SEC * 2;
        while (utime < timeout)
        {
                *(byte*)&nec.stm = inportb(0x3f4);
                if (nec.stm.ready)
                        break;
                utime = uclock();
        }

        timeout -= uclock();

        if (timeout <= 0)
        {
                if (log) fprintf(log, "STM: %02X RDY%c DIR%c IO%c NDMA%c %c%c%c%c\n",
                        *(byte*)&nec.stm,
                        (nec.stm.ready)?'*':'-',
                        (nec.stm.read_from_fdc)?'-':'+',
                        (nec.stm.io_active)?'*':'-',
                        (nec.stm.non_dma_mode)?'*':'-',
                        (nec.stm.drive_0)?'A':'*',
                        (nec.stm.drive_1)?'B':'*',
                        (nec.stm.drive_2)?'C':'*',
                        (nec.stm.drive_3)?'D':'*');
        }
        return (timeout > 0);
}

static  void    results(int sense_interrupt)
{

        if (sense_interrupt)
        {
                if (main_status() && nec.stm.read_from_fdc)
                {
                        *(byte*)&nec.sta.st3 = inportb(0x3F5);
                        if (log) fprintf(log, "ST3: 0x%02X US%d HD%c TS%c T0%c RY%c WP%c FT%c\n",
                                *(byte*)&nec.sta.st3,
                                nec.sta.st3.unit_select,
                                nec.sta.st3.head,
                                (nec.sta.st3.two_side)?'*':'-',
                                (nec.sta.st3.track_0)?'*':'-',
                                (nec.sta.st3.ready)?'*':'-',
                                (nec.sta.st3.write_protect)?'*':'-',
                                (nec.sta.st3.fault)?'*':'-');
                }
                if (main_status() && nec.stm.read_from_fdc)
                {
                        *(byte*)&nec.sta.pcn = inportb(0x3F5);
                        if (log) fprintf(log, "PCN: %02d\n", nec.sta.pcn);
                }
                return;
        }
        if (main_status() && nec.stm.read_from_fdc)
        {
                *(byte*)&nec.sta.st0 = inportb(0x3F5);
                if (log) fprintf(log, "ST0: 0x%02X US%d HD%d NR%c EC%c SE%c IC%d\n",
                        *(byte*)&nec.sta.st0,
                        nec.sta.st0.unit_select,
                        nec.sta.st0.head,
                        (nec.sta.st0.not_ready)?'*':'-',
                        (nec.sta.st0.equip_check)?'*':'-',
                        (nec.sta.st0.seek_end)?'*':'-',
                        nec.sta.st0.int_code);
        }
        if (main_status() && nec.stm.read_from_fdc)
        {
                *(byte*)&nec.sta.st1 = inportb(0x3F5);
                if (log) fprintf(log, "ST1: 0x%02X MA%c NW%c ND%c OR%c DE%c EC%c\n",
                        *(byte*)&nec.sta.st1,
                        (nec.sta.st1.missing_AM)?'*':'-',
                        (nec.sta.st1.not_writeable)?'*':'-',
                        (nec.sta.st1.no_data)?'*':'-',
                        (nec.sta.st1.overrun)?'*':'-',
                        (nec.sta.st1.data_error)?'*':'-',
                        (nec.sta.st1.end_of_cylinder)?'*':'-');
        }
        if (main_status() && nec.stm.read_from_fdc)
        {
                *(byte*)&nec.sta.st2 = inportb(0x3F5);
                if (log) fprintf(log, "ST2: 0x%02X MD%c BC%c SN%c SH%c WC%c DD%c CM%c\n",
                        *(byte*)&nec.sta.st2,
                        (nec.sta.st2.missing_DAM)?'*':'-',
                        (nec.sta.st2.bad_cylinder)?'*':'-',
                        (nec.sta.st2.scan_not_satisfied)?'*':'-',
                        (nec.sta.st2.scan_equal_hit)?'*':'-',
                        (nec.sta.st2.wrong_cylinder)?'*':'-',
                        (nec.sta.st2.data_error)?'*':'-',
                        (nec.sta.st2.control_mark)?'*':'-');
        }

        if (main_status() && nec.stm.read_from_fdc)
        {
                *(byte*)&nec.sta.c = inportb(0x3F5);
                if (log) fprintf(log, "     C:%02d", nec.sta.c);
        }
        if (main_status() && nec.stm.read_from_fdc)
        {
                *(byte*)&nec.sta.h = inportb(0x3F5);
                if (log) fprintf(log, " H:%d", nec.sta.h);
        }
        if (main_status() && nec.stm.read_from_fdc)
        {
                *(byte*)&nec.sta.s = inportb(0x3F5);
                if (log) fprintf(log, " S:%02d", nec.sta.s);
        }
        if (main_status() && nec.stm.read_from_fdc)
        {
                *(byte*)&nec.sta.n = inportb(0x3F5);
                if (log) fprintf(log, " N:%2d", nec.sta.n);
        }
        if (log) fprintf(log, "\n");

        /* more data? shouldn't happen, but read it away */
        while (main_status() && nec.stm.read_from_fdc)
        {
                inportb(0x3F5);
        }
}

static  int     out(byte b)
{
int     result = 1;

        if (main_status())
        {
                if (nec.stm.read_from_fdc)
                {
                        results(0);
                }
                else
                {
                        if (log) fprintf(log, " %02X", b);
                        outportb(0x3f5, b);
                        result = 0;
                }
        }
        return result;
}

static  void    seek_exec(byte * track)
{
uclock_t timeout;

        timeout = uclock() + UCLOCKS_PER_SEC * 5;

        while (uclock() < timeout)
        {
                main_status();
                if (irq_flag)
                        break;
        }

        timeout -= uclock();

        if (nec.stm.read_from_fdc)
                results(0);

        if (out(0x08))      /* sense interrupt status */
                return;
        if (log) fprintf(log, "\n");

        results(1);

        if (track)
                *track = nec.sta.pcn;
}

static  void    specify(void)
{
int     i;
        if (log) fprintf(log, "specify\n");

        outportb(0x3f2, 0x08);  /* reset fdc */
        for (i = 1; i < 1000; i++)
                ;
        outportb(0x3f2, 0x0c);  /* take away reset */

        if (out(0x03))      /* specify command */
                return;
        if (out(0xDF))      /* steprate, head unload timing */
                return;
        if (out(0x04))      /* head load timing, DMA mode */
                return;
        if (log) fprintf(log, "\n");

}

static  byte    read_sector(byte t, byte h, byte s)
{
int     i, try;

        if (log) fprintf(log, "read    unit:%d track:%02d head:%d sector:%d\n", nec.unit, t, h, s);

        /* maximum is two tries to detect normal / deleted data address mark */
        for (try = 0; try < 2; try++)
        {
                irq_flag = 0;

                dma_read(__tb, ((nec.secl)? nec.secl << 8 : nec.dtl));

                if (out(nec.den | nec.rcmd))     /* read sector */
                        return STA_2_NOT_READY;         
                if (out((h<<2)+nec.unit))        /* head * 4 + unit */
                        return STA_2_NOT_READY;
                if (out(t))                      /* track */
                        return STA_2_NOT_READY;
                if (out(h))                      /* head */
                        return STA_2_NOT_READY;
                if (out(s))                      /* sector */
                        return STA_2_NOT_READY;
                if (out(nec.secl))               /* bytes per sector */
                        return STA_2_NOT_READY;
                if (out(nec.eot))                /* end of track */
                        return STA_2_NOT_READY;
                if (out(nec.gap2))               /* gap II */
                        return STA_2_NOT_READY;
                if (out(nec.dtl))                /* data length */
                        return STA_2_NOT_READY;
                if (log) fprintf(log, "\n");

                for (i = 0; i < 3; i++)
                {
                        main_status();
                        if (!nec.stm.io_active)
                                break;
                        if (irq_flag)
                                break;
                }

                results(0);

                if (!nec.sta.st2.control_mark)
                        break;

                nec.rcmd ^= 0x0A;       /* toggle between 0x26 and 0x2C */
        }

/* set status 2 control_mark depending on read / read deleted */
        nec.sta.st2.control_mark = (nec.rcmd & 0x08) ? 1 : 0;

        return FDC_STA2(1);
}

static  byte    write_sector(byte t, byte h, byte s, byte deleted)
{
int     i;

        if (log) fprintf(log, "write   unit:%d track:%02d head:%d sector:%d\n", nec.unit, t, h, s);

        irq_flag = 0;

        dma_write(__tb, ((nec.secl)? nec.secl << 8 : nec.dtl));
        nec.wcmd = (deleted) ? 0x09 : 0x05;

        if (out(nec.den | nec.wcmd))     /* write sector */
                return STA_2_NOT_READY;
        if (out((h<<2)+nec.unit))        /* head * 4 + unit */
                return STA_2_NOT_READY;
        if (out(t))                      /* track */
                return STA_2_NOT_READY;
        if (out(h))                      /* head */
                return STA_2_NOT_READY;
        if (out(s))                      /* sector */
                return STA_2_NOT_READY;
        if (out(nec.secl))               /* bytes per sector */
                return STA_2_NOT_READY;
        if (out(nec.eot))                 /* end of track */
                return STA_2_NOT_READY;
        if (out(nec.gap2))               /* gap II */
                return STA_2_NOT_READY;
        if (out(nec.dtl))                /* data length */
                return STA_2_NOT_READY;
        if (log) fprintf(log, "\n");

        for (i = 0; i < 3; i++)
        {
                main_status();
                if (!nec.stm.io_active)
                        break;
                if (irq_flag)
                        break;
        }

        results(0);

        return FDC_STA2(0);
}

/* real mode interrupt 0x0E */
static  void    irq_E(void)
{
        asm(
        "nop\n"                         /* "nop" to find interrupt entry */
        "sti\n"                         /* enable interrupts */
        "pushl %eax\n"                  /* save EAX */
        "pushw %ds\n"                   /* save DS */
        "movw  $0x0000, %ax\n"          /* patched with _my_ds() */
        "movw  %ax, %ds\n"
        "orb   $0x80, _irq_flag\n"  /* set IRQ occured flag */
        "popw  %ds\n"                   /* get back DS */
        "movb  $0x20, %al\n"            /* end of interrupt */
        "outb  %al, $0x20\n"            /* to PIC */
        "popl  %eax\n"                  /* get back EAX */
        "iret\n");                      /* that's it... */
}

int     osd_fdc_init(void)
{
__dpmi_meminfo  meminfo;
unsigned long _my_cs_base;
unsigned long _my_ds_base;
byte * p;

        log = fopen("nec.log", "w");

        for (p = (byte*)&irq_E; p; p++)
        {
                if (p[0] == 0x90)       /* found "nop" */
                        new_irq_E.offset32 = (int)p + 1;
                if ((p[0]==0xB8) &&     /* found "movw $0000,%ax" ? */
                    (p[1]==0x00) &&
                    (p[2]==0x00))
                {
                        p++;
                        *(short *)p = _my_ds();
                        break;
                }
        }

        __dpmi_get_segment_base_address(_my_cs(), &_my_cs_base);
        __dpmi_get_segment_base_address(_my_ds(), &_my_ds_base);
        __dpmi_get_protected_mode_interrupt_vector(0x0E, &old_irq_E);

        meminfo.handle  = _my_cs();
        meminfo.size    = 128;  /* should be enough */
        meminfo.address = _my_cs_base + (unsigned long)&irq_E;
        if (__dpmi_lock_linear_region(&meminfo))
        {
                if (log)
                        fprintf(log,"init: could not lock code %lx addr:%08lx size:%ld\n",
                                meminfo.handle, meminfo.address, meminfo.size);
                return 0;
        }

        meminfo.handle  = _my_ds();
        meminfo.size    = 128;  /* should be enough */
        meminfo.address = _my_ds_base + (unsigned long)&old_irq_E;
        if (__dpmi_lock_linear_region(&meminfo))
        {
                if (log) fprintf(log,"init: could not lock data\n");
                return 0;
        }

        new_irq_E.selector = _my_cs();
        if (__dpmi_set_protected_mode_interrupt_vector(0x0E, &new_irq_E))
        {
                if (log) fprintf(log,"init: could not set vector 0x0E");
                return 0;
        }

        atexit(osd_fdc_exit);

        return 1;
}

void    osd_fdc_exit(void)
{
__dpmi_meminfo meminfo;
unsigned long _my_cs_base;
unsigned long _my_ds_base;
static int was_here = 0;

        if (was_here)
                return;
        was_here = 1;

        outportb(0x3f2, 0x0c);

        if (log)
                fclose(log);

        __dpmi_get_segment_base_address(_my_cs(), &_my_cs_base);
        __dpmi_get_segment_base_address(_my_ds(), &_my_ds_base);
        __dpmi_set_protected_mode_interrupt_vector(0x0E, &old_irq_E);

        meminfo.handle  = _my_cs();
        meminfo.size    = 128;  /* should be enough */
        meminfo.address = _my_cs_base + (unsigned long)&irq_E;
        __dpmi_unlock_linear_region(&meminfo);

        meminfo.handle  = _my_ds();
        meminfo.size    = 128;  /* should be enough */
        meminfo.address = _my_ds_base + (unsigned long)&old_irq_E;
        __dpmi_unlock_linear_region(&meminfo);

        if (log)
                fclose(log);
        log = 0;
}

void    osd_fdc_motors(byte unit, byte type)
{
        nec.unit = unit;
        nec.type = type;

        switch (nec.unit)
        {
                case 0:
                        if (log) fprintf(log, "FDD A: start\n");
                        outportb(0x3f2, 0x1c);  /* drive 0 */
                        break;
                case 1:
                        if (log) fprintf(log, "FDD B: start\n");
                        outportb(0x3f2, 0x2d);  /* drive 1 */
                        break;
        }
}

void    osd_fdc_density(byte den)
{
__dpmi_regs r;
byte f17_AL = 0x02;

        switch (nec.type)
        {
                case 0: /* 360K */
                        f17_AL     = 0x01;
                        nec.eot    = 18 - 1;
                        nec.dstep  = 0;
                        break;
                case 1: /* 720K */
                        f17_AL     = 0x04;
                        nec.eot    = 18 - 1;
                        nec.dstep  = 0;
                        break;
                case 2: /* 1.2M */
                        f17_AL     = 0x03;
                        nec.eot    = 30 - 1;
                        nec.dstep  = 0;
                        break;
                case 3: /* 1.44M */
                        f17_AL     = 0x03;
                        nec.eot    = 36 - 1;
                        nec.dstep  = 0;
                        break;
        }

        if (unit_type[nec.unit] != nec.type)
        {
                unit_type[nec.unit] = nec.type;
                r.h.ah = 0x17;
                r.h.dl = nec.unit;
                r.h.al = f17_AL;
                __dpmi_int(0x13, &r);
                if (log) fprintf(log, "FDD %c: Mode:%d SPT:%d (%04X)\n",
                        'A'+nec.unit, f17_AL, nec.eot + 1, r.x.ax);
        }

        nec.secl    = 1;       /* 256 bytes per sector */
        nec.gap2    = 0x0A;    /* read/write gap II */
        nec.gap3    = 0x13;    /* format gap III */
        nec.dtl     = 0xFF;    /* data length (only used if secl == 0) */
        nec.filler  = 0xE5;    /* format filler byte */
        nec.rcmd    = 0x26;    /* read command (0x26 normal, 0x2C deleted address mark) */
        nec.wcmd    = 0x05;    /* write command (0x05 normal, 0x09 deleted address mark) */

        if (den)
        {
                nec.den = 0x40;         /* DD commands */
                outportb(0x3f7, 0x02);  /* set 250 kbps */
        }
        else
        {
#if     0       /* doesn't work! */
                nec.den = 0x00;         /* SD commands */
                outportb(0x3f7, 0x02);  /* how do we set 125 kbps ?? */
#else
                nec.den = 0x40;         /* DD commands */
                outportb(0x3f7, 0x02);  /* set 250 kbps */
#endif
        }
}

void    osd_fdc_interrupt(void)
{
}

byte    osd_fdc_recal(byte * track)
{
        if (log) fprintf(log, "recal\n");

        irq_flag = 0;
        if (out(0x07))              /* 1st recal command */
                return STA_1_NOT_READY;
        if (out(nec.unit))
                return STA_1_NOT_READY;
        if (log) fprintf(log, "\n");

        seek_exec(track);

        irq_flag = 0;
        if (out(0x07))              /* 2nd recal command */
                return STA_1_NOT_READY;
        if (out(nec.unit))
                return STA_1_NOT_READY;
        if (log) fprintf(log, "\n");

        seek_exec(track);

        /* reset unit type */
        unit_type[nec.unit] = 0xff;

        return FDC_STA1();
}

byte    osd_fdc_seek(byte t, byte * track)
{
        if (nec.sta.pcn == (t << nec.dstep))
        {
                *track = t;
                return FDC_STA1();
        }

        if (log) fprintf(log, "seek    unit:%d track:%02d\n", nec.unit, t);
        irq_flag = 0;

        if (out(0x0f))              /* seek command */
                return STA_1_NOT_READY;
        if (out(nec.unit))          /* unit number */
                return STA_1_NOT_READY;
        if (out(t << nec.dstep))    /* track number */
                return STA_1_NOT_READY;
        if (log) fprintf(log, "\n");

        seek_exec(track);

        return FDC_STA1();
}

byte    osd_fdc_step(int dir, byte * track)
{
        if (log) fprintf(log, "step    unit:%d direction:%+d (PCN:%02d)\n", nec.unit, dir, nec.sta.pcn);
        irq_flag = 0;

        if (out(0x0f))              /* seek command */
                return STA_1_NOT_READY;
        if (out(nec.unit))
                return STA_1_NOT_READY;
        if (out(nec.sta.pcn + (dir << nec.dstep)))
                return STA_1_NOT_READY;
        if (log) fprintf(log, "\n");

        seek_exec(track);

        if (track)
                *track = nec.sta.pcn;

        return FDC_STA1();
}

byte    osd_fdc_format(byte t, byte h, byte spt, byte * fmt)
{
int     i;

        if (log) fprintf(log, "format  unit:%d track:%02d head:%d sec/track:%d\n", nec.unit, t, h, spt);

#ifdef  OPTIMIZE_XLT
        {
        byte idx = 0;
                for (i = 0; i < spt; i++)
                {
                        fmt[i*4+2] = idx;
                        idx += spt / 2;
                        if (idx >= spt)
                                idx = idx - spt + 1;
                }
        }
#endif

        nec.eot = 0;
        for (i = 0; i < spt; i++)
        {
                if (fmt[i*4+2] > nec.eot)
                        nec.eot = fmt[i*4+2];
        }

        movedata(_my_ds(), (unsigned) fmt, _dos_ds, __tb, spt * 4);

        irq_flag = 0;

        dma_write(__tb, spt * 4);

        if (out(nec.den | 0x0D))         /* format track */
                return STA_2_NOT_READY;
        if (out((h<<2)+nec.unit))        /* head * 4 + unit select */
                return STA_2_NOT_READY;
        if (out(nec.secl))               /* bytes per sector */
                return STA_2_NOT_READY;
        if (out(spt))                    /* sectors / track */
                return STA_2_NOT_READY;
        if (out(nec.gap3))               /* gap III */
                return STA_2_NOT_READY;
        if (out(nec.filler))             /* format filler byte */
                return STA_2_NOT_READY;
        if (log) fprintf(log, "\n");

        for (i = 0; i < 3; i++)
        {
                main_status();
                if (!nec.stm.io_active)
                        break;
                if (irq_flag)
                        break;
        }

        results(0);

        return FDC_STA2(0);
}

byte    osd_fdc_put_sector(byte t, byte h, byte s, byte * buff, byte deleted)
{
byte    result;

        movedata(_my_ds(), (unsigned)buff,
                 _dos_ds, __tb,
                 ((nec.secl)? nec.secl << 8 : nec.dtl));
        result = write_sector(t, h, s, deleted);

        return result;
}

byte    osd_fdc_get_sector(byte t, byte h, byte s, byte * buff)
{
byte    result;

        result = read_sector(t, h, s);
        movedata(_dos_ds, __tb,
                 _my_ds(), (unsigned)buff,
                 ((nec.secl)? nec.secl << 8 : nec.dtl));

        return result;
}
