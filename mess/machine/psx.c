#include "includes/psx.h"

/* -----------------------------------------------------------------------
 * cd_stat values
 *
 * NoIntr       0x00 No interrupt
 * DataReady    0x01 Data Ready
 * Acknowledge  0x02 Command Complete
 * Complete     0x03 Acknowledge
 * DataEnd      0x04 End of Data Detected
 * DiskError    0x05 Error Detected
 * ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */

static int cd_param_p;
static int cd_result_p;
static int cd_result_c;
static int cd_result_ready;
static int cd_readed = -1;
static int cd_reset;
static UINT8 cd_stat;
static UINT8 cd_io_status;
static UINT8 cd_param[8];
static UINT8 cd_result[8];

/* ----------------------------------------------------------------------- */

static void psx_cdcmd_sync(void)
{
	/* NYI */
}

static void psx_cdcmd_nop(void)
{
	/* NYI */
}

static void psx_cdcmd_setloc(void)
{
	/* NYI */
}

static void psx_cdcmd_play(void)
{
	/* NYI */
}

static void psx_cdcmd_forward(void)
{
	/* NYI */
}

static void psx_cdcmd_backward(void)
{
	/* NYI */
}

static void psx_cdcmd_readn(void)
{
	/* NYI */
}

static void psx_cdcmd_standby(void)
{
	/* NYI */
}

static void psx_cdcmd_stop(void)
{
	/* NYI */
}

static void psx_cdcmd_pause(void)
{
	/* NYI */
}

static void psx_cdcmd_init(void)
{
	cd_result_p = 0;
	cd_result_c = 1;
	cd_stat = 0x02;
	cd_result[0] = cd_stat;
	/* NYI */
}

static void psx_cdcmd_mute(void)
{
	/* NYI */
}

static void psx_cdcmd_demute(void)
{
	/* NYI */
}

static void psx_cdcmd_setfilter(void)
{
	/* NYI */
}

static void psx_cdcmd_setmode(void)
{
	/* NYI */
}

static void psx_cdcmd_getparam(void)
{
	/* NYI */
}

static void psx_cdcmd_getlocl(void)
{
	/* NYI */
}

static void psx_cdcmd_getlocp(void)
{
	/* NYI */
}

static void psx_cdcmd_gettn(void)
{
	/* NYI */
}

static void psx_cdcmd_gettd(void)
{
	/* NYI */
}

static void psx_cdcmd_seekl(void)
{
	/* NYI */
}

static void psx_cdcmd_seekp(void)
{
	/* NYI */
}

static void psx_cdcmd_test(void)
{
	static const UINT8 test20[] = { 0x98, 0x06, 0x10, 0xC3 };
	static const UINT8 test22[] = { 0x66, 0x6F, 0x72, 0x20, 0x45, 0x75, 0x72, 0x6F };
	static const UINT8 test23[] = { 0x43, 0x58, 0x44, 0x32, 0x39 ,0x34, 0x30, 0x51 };

	switch(cd_param[0]) {
	case 0x04:	/* read SCEx counter (returned in 1st byte?) */
		break;
	case 0x05:	/* reset SCEx counter. */
		break;
	case 0x20:
		memcpy(cd_result, test20, sizeof(test20));
		cd_result_p = 0;
		cd_result_c = sizeof(test20);
		break;
	case 0x22:
		memcpy(cd_result, test22, sizeof(test22));
		cd_result_p = 0;
		cd_result_c = sizeof(test22);
		break;
	case 0x23:
		memcpy(cd_result, test23, sizeof(test23));
		cd_result_p = 0;
		cd_result_c = sizeof(test23);
		break;
	}
	cd_stat = 3;
}

static void psx_cdcmd_id(void)
{
	/* NYI */
}

static void psx_cdcmd_reads(void)
{
	/* NYI */
}

static void psx_cdcmd_reset(void)
{
	/* NYI */
}

static void psx_cdcmd_readtoc(void)
{
	/* NYI */
}

static void (*psx_cdcmds[])(void) =
{
	psx_cdcmd_sync,
	psx_cdcmd_nop,
	psx_cdcmd_setloc,
	psx_cdcmd_play,
	psx_cdcmd_forward,
	psx_cdcmd_backward,
	psx_cdcmd_readn,
	psx_cdcmd_standby,
	psx_cdcmd_stop,
	psx_cdcmd_pause,
	psx_cdcmd_init,
	psx_cdcmd_mute,
	psx_cdcmd_demute,
	psx_cdcmd_setfilter,
	psx_cdcmd_setmode,
	psx_cdcmd_getparam,
	psx_cdcmd_getlocl,
	psx_cdcmd_getlocp,
	psx_cdcmd_gettn,
	psx_cdcmd_gettd,
	psx_cdcmd_seekl,
	psx_cdcmd_seekp,
	NULL,
	NULL,
	NULL,
	psx_cdcmd_test,
	psx_cdcmd_id,
	psx_cdcmd_reads,
	psx_cdcmd_reset,
	NULL,
	psx_cdcmd_readtoc
};

/* ----------------------------------------------------------------------- */

READ32_HANDLER( psx_cd_r )
{
	UINT32 result = 0;

	if( mem_mask == 0xffffff00 )
	{
		logerror( "%08x cd0 read\n", activecpu_get_pc() );

		if (cd_result_ready && cd_result_c)
			cd_io_status |= 0x20;
		else
			cd_io_status &= ~0x20;

		if (cd_readed == 0)
			result = cd_io_status | 0x40;
		else
			result = cd_io_status;

		/* NPW 21-May-2003 - Seems to expect this on boot */
		result |= 0x0f;
	}
	else if( mem_mask == 0xffff00ff )
	{
		logerror( "%08x cd1 read\n", activecpu_get_pc() );

		if (cd_result_ready && cd_result_c)
			result = ((UINT32) cd_result[cd_result_p]) << 8;

		if (cd_result_ready)
		{
			cd_result_p++;
			if (cd_result_p == cd_result_c)
				cd_result_ready = 0;
		}
	}
	else if( mem_mask == 0xff00ffff )
	{
		logerror( "%08x cd2 read\n", activecpu_get_pc() );
	}
	else if( mem_mask == 0x00ffffff )
	{
		logerror( "%08x cd3 read\n", activecpu_get_pc() );

		result = ((UINT32) cd_stat) << 24;
	}
	else
	{
		logerror( "%08x cd_r( %08x, %08x ) unsupported transfer\n", activecpu_get_pc(), offset, mem_mask );
	}
	return result;
}

WRITE32_HANDLER( psx_cd_w )
{
	void (*psx_cdcmd)(void);

	if( mem_mask == 0xffffff00 )
	{
		/* write to CD register 0 */
		data = (data >> 0) & 0xff;
		logerror( "%08x cd0 write %02x\n", activecpu_get_pc(), data & 0xff );

		cd_param_p = 0;
		if (data == 0x00)
		{
			cd_result_ready = 0;
		}
		else
		{
			if (data & 0x01)
				cd_result_ready = 1;
			if (data == 0x01)
				cd_reset = 1;
		}
	}
	else if( mem_mask == 0xffff00ff )
	{
		/* write to CD register 1 */
		data = (data >> 8) & 0xff;
		logerror( "%08x cd1 write %02x\n", activecpu_get_pc(), data );

		if (data <= sizeof(psx_cdcmds) / sizeof(psx_cdcmds[0]))
			psx_cdcmd = psx_cdcmds[data];
		else
			psx_cdcmd = NULL;

		if (psx_cdcmd)
			psx_cdcmd();

		psx_irq_set(0x0004);
	}
	else if( mem_mask == 0xff00ffff )
	{
		/* write to CD register 2 */
		data = (data >> 16) & 0xff;
		logerror( "%08x cd2 write %02x\n", activecpu_get_pc(), data & 0xff );

		if ((cd_reset == 2) && (data == 0x07))
		{
			/* copied from SOPE; this code is weird */
			cd_param_p = 0;
			cd_result_ready = 1;
			cd_reset = 0;
			cd_stat = 0;
			cd_io_status = 0x00;
			if (cd_result_ready && cd_result_c)
				cd_io_status |= 0x20;
			else
				cd_io_status &= ~0x20;
		}
		else
		{
			/* used for sending arguments */
			cd_reset = 0;
			cd_param[cd_param_p] = data;
			if (cd_param_p < 7)
				cd_param_p++;
		}
	}
	else if( mem_mask == 0x00ffffff )
	{
		/* write to CD register 3 */
		data = (data >> 24) & 0xff;
		logerror( "%08x cd3 write %02x\n", activecpu_get_pc(), data & 0xff );

		if ((data == 0x07) && (cd_reset == 1))
		{
			cd_reset = 2;
//			psxirq_clear(0x0004);
		}
		else
		{
			cd_reset = 0;
		}
	}
	else
	{
		logerror( "%08x cd_w( %08x, %08x, %08x ) unsupported transfer\n", activecpu_get_pc(), offset, data, mem_mask );
	}
}

/* ----------------------------------------------------------------------- */

/* status Flags */
#define TX_RDY		0x0001
#define RX_RDY		0x0002
#define TX_EMPTY	0x0004
#define PARITY_ERR	0x0008
#define RX_OVERRUN	0x0010
#define FRAMING_ERR	0x0020
#define SYNC_DETECT	0x0040
#define DSR			0x0080
#define CTS			0x0100
#define IRQ			0x0200

/* control flags */
#define TX_PERM		0x0001
#define DTR			0x0002
#define RX_PERM		0x0004
#define BREAK		0x0008
#define RESET_ERR	0x0010
#define RTS			0x0020
#define SIO_RESET	0x0040

static UINT16 ser_statreg = TX_RDY | TX_EMPTY;
static UINT16 ser_modereg, ser_ctrlreg, ser_baudreg;

static UINT8 psx_sioread(void)
{
	UINT8 result = 0;

	if (ser_statreg & RX_RDY)
	{
	}

	return result;
}

static void psx_siowrite(UINT8 data)
{
}

static void psx_siowrite_ctrl(UINT16 data)
{
	ser_ctrlreg = data & ~RESET_ERR;
	if (data & RESET_ERR)
		ser_statreg &= ~IRQ;

	if ((ser_ctrlreg & SIO_RESET) || !ser_ctrlreg)
	{
		ser_statreg = TX_RDY | TX_EMPTY;
	}
}

READ32_HANDLER( psx_serial_r )
{
	UINT32 result = 0;

	logerror( "%08x serial read offset=%d mem_mask=0x%08x\n", activecpu_get_pc(), offset, mem_mask );

	switch(offset) {
	case 0:
		/* 0x1f801040 */
		result = psx_sioread();
		result *= 0x01010101;
		break;

	case 1:
		/* 0x1f801044 */
		result = ser_statreg;
		result *= 0x00010001;
		break;

	case 2:
		/* 0x1f801048 */
		result = ser_modereg;
		result *= 0x00010000;
		result |= ser_ctrlreg;

	case 3:
		/* 0x1f80104c */
		result = ser_baudreg;
		result *= 0x00010001;
		break;
	}
	return result;
}

WRITE32_HANDLER( psx_serial_w )
{
	logerror( "%08x serial write offset=%i mem_mask=0x%08x data=0x%08x\n", activecpu_get_pc(), offset, mem_mask, data );

	switch(offset) {
	case 0:
		/* 0x1f801040 */
		psx_siowrite((UINT8) (data >> 0));
		if (mem_mask == 0xffff0000)
			psx_siowrite((UINT8) (data >> 8));
		break;

	case 1:
		/* 0x1f801044 */
		break;

	case 2:
		if ((mem_mask & 0x0000ffff) == 0)
		{
			/* 0x1f801048 */
			ser_modereg = (UINT16) (data >> 0);
		}
		if ((mem_mask & 0xffff0000) == 0)
		{	
			/* 0x1f80104a */
			psx_siowrite_ctrl((UINT16) (data >> 16));
		}
		break;

	case 3:
		if ((mem_mask & 0xffff0000) == 0)
		{	
			/* 0x1f80104e */
			ser_baudreg = (UINT16) (data >> 16);
		}
		break;
	}
}

