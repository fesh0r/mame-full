#include "driver.h"
#include "mamedbg.h"

const char *cc[] = {
	"EQ", "NE",
	"CS", "CC",
	"MI", "PL",
	"VS", "VC",
	"HI", "LS",
	"GE", "LT",
	"GT", "LE",
	"  ", "NV"  /* AL for always isn't printed */
};

static char *RD(UINT32 op)
{
	static char buff[8];
	sprintf(buff, "R%d", (op >> 12) & 15);
	return buff;
}

static char *RN(UINT32 op)
{
	static char buff[8];
	sprintf(buff, "R%d", (op >> 16) & 15);
	return buff;
}

static char *RS(UINT32 op)
{
	static char buff[32];
    UINT32 m = (op >> 4) & 7;
    UINT32 s = (op >> 7) & 31;
	UINT32 rs = s >> 1;
	UINT32 rn = (op >> 16) & 15;

	buff[0] = '\0';

    switch( m )
	{
	case 0:
		/* LSL (aka ASL) #0 .. 31 */
		if (s > 0)
			sprintf(buff, "R%d LSL #%d", rn, s);
		else
			sprintf(buff, "R%d", rn);
		break;
	case 1:
		/* LSL (aka ASL) R0 .. R15 */
		sprintf(buff, "R%d LSL R%d", rn, rs);
		break;
	case 2:
        /* LSR #1 .. 32 */
		if (s == 0) s = 32;
		sprintf(buff, "R%d LSR #%d", rn, s);
        break;
	case 3:
		/* LSR R0 .. R15 */
		sprintf(buff, "R%d LSR R%d", rn, rs);
		break;
	case 4:
        /* ASR #1 .. 32 */
		if (s == 0) s = 32;
		sprintf(buff, "R%d ASR #%d", rn, s);
        break;
    case 5:
		/* ASR R0 .. R15 */
		sprintf(buff, "R%d ASR R%d", rn, rs);
		break;
	case 6:
		/* ASR #1 .. 32 */
        if (s == 0)
            sprintf(buff, "R%d RRX", rn);
        else
            sprintf(buff, "R%d ROR #%d", rn, s);
        break;
	case 7:
		/* ROR R0 .. R15  */
		sprintf(buff, "R%d ROR R%d", rn, rs);
        break;
    }
	return buff;
}

static char *IM(UINT32 op)
{
    static char buff[32];
	UINT32 val = op & 0xff, shift = (op >> 8) & 31;
	sprintf(buff, "$%x", (val << (31-shift*2)) | (val >> (shift*2)));
	return buff;
}

static char *RL(UINT32 op)
{
	static char buff[64];
	char *dst;
	int f, t, n;

	dst = buff;
	for(f = 0, n = 0; f < 16; f = t)
	{
		for (/* */; f < 16; f++)
			if( (op & (1L << f)) != 0 )
				break;
		for (t = f; t < 16; t++)
			if( (op & (1L << t)) == 0 )
				break;
		t--;

        if (f == 16 && n == 0)
			return "---";

		if (n)
			dst += sprintf(dst, ",");
		if (f == t)
			dst += sprintf(dst, "R%d", f);
		else
			dst += sprintf(dst, "R%d-R%d", f, t);
		n++;
	}
	return buff;
}

unsigned DasmARM(char *buffer, unsigned pc)
{
    UINT32 op = cpu_readmem26lew_dword(pc);

	buffer[0] = '\0';
    switch( (op>>20) & 0xff )
	{
	case 0x00: sprintf(buffer, "AND%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x01: sprintf(buffer, "ANDS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x02: sprintf(buffer, "EOR%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x03: sprintf(buffer, "EORS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x04: sprintf(buffer, "SUB%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x05: sprintf(buffer, "SUBS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x06: sprintf(buffer, "RSB%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x07: sprintf(buffer, "RSBS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x08: sprintf(buffer, "ADD%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x09: sprintf(buffer, "ADDS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x0a: sprintf(buffer, "ADC%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x0b: sprintf(buffer, "ADCS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x0c: sprintf(buffer, "SBC%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x0d: sprintf(buffer, "SBCS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x0e: sprintf(buffer, "RSC%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x0f: sprintf(buffer, "RSCS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;

	case 0x10: sprintf(buffer, "TST%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x11: sprintf(buffer, "TSTS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x12: sprintf(buffer, "TEQ%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x13: sprintf(buffer, "TEQS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x14: sprintf(buffer, "CMP%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x15: sprintf(buffer, "CMPS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x16: sprintf(buffer, "CMN%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x17: sprintf(buffer, "CMNS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x18: sprintf(buffer, "ORR%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x19: sprintf(buffer, "ORRS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x1a: sprintf(buffer, "MOV%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x1b: sprintf(buffer, "MOVS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x1c: sprintf(buffer, "BIC%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x1d: sprintf(buffer, "BICS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x1e: sprintf(buffer, "MVN%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x1f: sprintf(buffer, "MVNS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;

	case 0x20: sprintf(buffer, "AND%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x21: sprintf(buffer, "ANDS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x22: sprintf(buffer, "EOR%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x23: sprintf(buffer, "EORS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x24: sprintf(buffer, "SUB%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x25: sprintf(buffer, "SUBS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x26: sprintf(buffer, "RSB%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x27: sprintf(buffer, "RSBS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x28: sprintf(buffer, "ADD%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x29: sprintf(buffer, "ADDS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x2a: sprintf(buffer, "ADC%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x2b: sprintf(buffer, "ADCS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x2c: sprintf(buffer, "SBC%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x2d: sprintf(buffer, "SBCS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x2e: sprintf(buffer, "RSC%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x2f: sprintf(buffer, "RSCS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;

	case 0x30: sprintf(buffer, "TST%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x31: sprintf(buffer, "TSTS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x32: sprintf(buffer, "TEQ%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x33: sprintf(buffer, "TEQS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x34: sprintf(buffer, "CMP%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x35: sprintf(buffer, "CMPS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x36: sprintf(buffer, "CMN%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x37: sprintf(buffer, "CMNS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x38: sprintf(buffer, "ORR%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x39: sprintf(buffer, "ORRS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x3a: sprintf(buffer, "MOV%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x3b: sprintf(buffer, "MOVS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x3c: sprintf(buffer, "BIC%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x3d: sprintf(buffer, "BICS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x3e: sprintf(buffer, "MVN%s    %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x3f: sprintf(buffer, "MVNS%s   %s,%s,%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;

	case 0x40: sprintf(buffer, "STR%s    %s,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x41: sprintf(buffer, "LDR%s    %s,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x42: sprintf(buffer, "STR%s    %s!,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x43: sprintf(buffer, "LDR%s    %s!,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x44: sprintf(buffer, "STRB%s   %s,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x45: sprintf(buffer, "LDRB%s   %s,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x46: sprintf(buffer, "STRB%s   %s!,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x47: sprintf(buffer, "LDRB%s   %s!,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x48: sprintf(buffer, "STR%s    %s,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x49: sprintf(buffer, "LDR%s    %s,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x4a: sprintf(buffer, "STR%s    %s!,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x4b: sprintf(buffer, "LDR%s    %s!,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x4c: sprintf(buffer, "STRB%s   %s,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x4d: sprintf(buffer, "LDRB%s   %s,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x4e: sprintf(buffer, "STRB%s   %s!,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x4f: sprintf(buffer, "LDRB%s   %s!,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;

	case 0x50: sprintf(buffer, "STR%s    %s,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x51: sprintf(buffer, "LDR%s    %s,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x52: sprintf(buffer, "STR%s    %s!,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x53: sprintf(buffer, "LDR%s    %s!,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x54: sprintf(buffer, "STRB%s   %s,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x55: sprintf(buffer, "LDRB%s   %s,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x56: sprintf(buffer, "STRB%s   %s!,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x57: sprintf(buffer, "LDRB%s   %s!,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x58: sprintf(buffer, "STR%s    %s,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x59: sprintf(buffer, "LDR%s    %s,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x5a: sprintf(buffer, "STR%s    %s!,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x5b: sprintf(buffer, "LDR%s    %s!,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x5c: sprintf(buffer, "STRB%s   %s,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x5d: sprintf(buffer, "LDRB%s   %s,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x5e: sprintf(buffer, "STRB%s   %s!,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;
	case 0x5f: sprintf(buffer, "LDRB%s   %s!,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), RS(op)); break;

	case 0x60: sprintf(buffer, "STR%s    %s,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x61: sprintf(buffer, "LDR%s    %s,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x62: sprintf(buffer, "STR%s    %s!,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x63: sprintf(buffer, "LDR%s    %s!,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x64: sprintf(buffer, "STRB%s   %s,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x65: sprintf(buffer, "LDRB%s   %s,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x66: sprintf(buffer, "STRB%s   %s!,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x67: sprintf(buffer, "LDRB%s   %s!,[%s,-%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x68: sprintf(buffer, "STR%s    %s,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x69: sprintf(buffer, "LDR%s    %s,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x6a: sprintf(buffer, "STR%s    %s!,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x6b: sprintf(buffer, "LDR%s    %s!,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x6c: sprintf(buffer, "STRB%s   %s,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x6d: sprintf(buffer, "LDRB%s   %s,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x6e: sprintf(buffer, "STRB%s   %s!,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x6f: sprintf(buffer, "LDRB%s   %s!,[%s,%s]", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;

	case 0x70: sprintf(buffer, "STR%s    %s,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x71: sprintf(buffer, "LDR%s    %s,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x72: sprintf(buffer, "STR%s    %s!,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x73: sprintf(buffer, "LDR%s    %s!,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x74: sprintf(buffer, "STRB%s   %s,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x75: sprintf(buffer, "LDRB%s   %s,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x76: sprintf(buffer, "STRB%s   %s!,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x77: sprintf(buffer, "LDRB%s   %s!,[%s],-%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x78: sprintf(buffer, "STR%s    %s,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x79: sprintf(buffer, "LDR%s    %s,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x7a: sprintf(buffer, "STR%s    %s!,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x7b: sprintf(buffer, "LDR%s    %s!,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x7c: sprintf(buffer, "STRB%s   %s,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x7d: sprintf(buffer, "LDRB%s   %s,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x7e: sprintf(buffer, "STRB%s   %s!,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;
	case 0x7f: sprintf(buffer, "LDRB%s   %s!,[%s],%s", cc[(op>>28)&15], RD(op), RN(op), IM(op)); break;

	case 0x80: sprintf(buffer, "STMDA%s  %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x81: sprintf(buffer, "LDMDA%s  %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x82: sprintf(buffer, "STMDA%s  %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x83: sprintf(buffer, "LDMDA%s  %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x84: sprintf(buffer, "STMDAB%s %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x85: sprintf(buffer, "LDMDAB%s %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x86: sprintf(buffer, "STMDAB%s %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x87: sprintf(buffer, "LDMDAB%s %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x88: sprintf(buffer, "STMIA%s  %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x89: sprintf(buffer, "LDMIA%s  %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x8a: sprintf(buffer, "STMIA%s  %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x8b: sprintf(buffer, "LDMIA%s  %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x8c: sprintf(buffer, "STMIAB%s %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x8d: sprintf(buffer, "LDMIAB%s %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x8e: sprintf(buffer, "STMIAB%s %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x8f: sprintf(buffer, "LDMIAB%s %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;

	case 0x90: sprintf(buffer, "STMDB%s  %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x91: sprintf(buffer, "LDMDB%s  %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x92: sprintf(buffer, "STMDB%s  %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x93: sprintf(buffer, "LDMDB%s  %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x94: sprintf(buffer, "STMDBB%s %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x95: sprintf(buffer, "LDMDBB%s %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x96: sprintf(buffer, "STMDBB%s %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x97: sprintf(buffer, "LDMDBB%s %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x98: sprintf(buffer, "STMIB%s  %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x99: sprintf(buffer, "LDMIB%s  %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x9a: sprintf(buffer, "STMIB%s  %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x9b: sprintf(buffer, "LDMIB%s  %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x9c: sprintf(buffer, "STMIBB%s %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x9d: sprintf(buffer, "LDMIBB%s %s,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x9e: sprintf(buffer, "STMIBB%s %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;
	case 0x9f: sprintf(buffer, "LDMIBB%s %s!,{%s}", cc[(op>>28)&15], RD(op), RL(op)); break;

	case 0xa0: case 0xa1: case 0xa2: case 0xa3:
	case 0xa4: case 0xa5: case 0xa6: case 0xa7:
	case 0xa8: case 0xa9: case 0xaa: case 0xaf:
		sprintf(buffer, "B%s      $%x", cc[(op>>28)&15], (pc + 4*(op&0xffffff)) & 0x3ffffffc);
		break;
	case 0xb0: case 0xb1: case 0xb2: case 0xb3:
	case 0xb4: case 0xb5: case 0xb6: case 0xb7:
	case 0xb8: case 0xb9: case 0xba: case 0xbf:
		sprintf(buffer, "BL%s     $%x", cc[(op>>28)&15], (pc + 4*(op&0xffffff)) & 0x3ffffffc);
        break;
	case 0xc0: case 0xc1: case 0xc2: case 0xc3:
	case 0xc4: case 0xc5: case 0xc6: case 0xc7:
	case 0xc8: case 0xc9: case 0xca: case 0xcf:
		sprintf(buffer, "INVC%s   $%08x", cc[(op>>28)&15], op);
        break;
    case 0xd0: case 0xd1: case 0xd2: case 0xd3:
	case 0xd4: case 0xd5: case 0xd6: case 0xd7:
	case 0xd8: case 0xd9: case 0xda: case 0xdf:
		sprintf(buffer, "INVD%s   $%08x", cc[(op>>28)&15], op);
		break;
	case 0xe0: case 0xe1: case 0xe2: case 0xe3:
	case 0xe4: case 0xe5: case 0xe6: case 0xe7:
	case 0xe8: case 0xe9: case 0xea: case 0xef:
		sprintf(buffer, "INVE%s   $%08x", cc[(op>>28)&15], op);
        break;
    case 0xf0: case 0xf1: case 0xf2: case 0xf3:
	case 0xf4: case 0xf5: case 0xf6: case 0xf7:
	case 0xf8: case 0xf9: case 0xfa: case 0xff:
		sprintf(buffer, "SWI%s    $%08x", cc[(op>>28)&15], op);
		break;
	default:
		sprintf(buffer, "???%s    $%08x", cc[(op>>28)&15], op);
    }

    return 4;
}
