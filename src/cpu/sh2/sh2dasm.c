#include "driver.h"
#ifdef	MAME_DEBUG
#include "mamedbg.h"
#include "sh2.h"

#define SIGNX8(x)	(((INT32)(x) << 24) >> 24)
#define SIGNX12(x)	(((INT32)(x) << 20) >> 20)

#define Rn ((opcode >> 8) & 15)
#define Rm ((opcode >> 4) & 15)

void code0000(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;

	switch(opcode & 0x3f)
	{
	case 0x02:
		sprintf(buffer,"STC     SR,R%d", Rn);
		break;
	case 0x03:
		sym = set_ea_info(0, cpu_get_reg(SH2_R0 + Rn), EA_UINT32, EA_ABS_PC);
		sprintf(buffer,"BSRF    R%d", Rn);
		break;
	case 0x08:
		sprintf(buffer,"CLRT");
		break;
	case 0x09:
		sprintf(buffer,"NOP");
		break;
	case 0x0A:
		sprintf(buffer,"STS     MACH,R%d", Rn);
		break;
	case 0x0B:
		sprintf(buffer,"RTS");
		break;
	case 0x12:
		sprintf(buffer,"STS     GBR,R%d", Rn);
		break;
	case 0x18:
		sprintf(buffer,"SETT");
		break;
	case 0x19:
		sprintf(buffer,"DIV0U");
		break;
	case 0x1A:
		sprintf(buffer,"STS     MACL,R%d", Rn);
		break;
	case 0x1B:
		sprintf(buffer,"SLEEP");
		break;
	case 0x22:
		sprintf(buffer,"STC     VBR,R%d", Rn);
		break;
	case 0x23:
		sym = set_ea_info(0, cpu_get_reg(SH2_R0 + Rn), EA_UINT32, EA_ABS_PC);
		sprintf(buffer,"BRAF    R%d", Rn);
		break;
	case 0x28:
		sprintf(buffer,"CLRMAC");
		break;
	case 0x29:
		sprintf(buffer,"MOVT    R%d", Rn);
		break;
	case 0x2A:
		sprintf(buffer,"STS     PR,R%d", Rn);
		break;
	case 0x2B:
		sprintf(buffer,"RTE");
		break;
	default:
		switch(opcode & 15)
		{
		case  0:
			sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
			sprintf(buffer, "??????  %s", sym);
			break;
		case  1:
			sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
			sprintf(buffer, "??????  %s", sym);
			break;
		case  2:
			sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
			sprintf(buffer, "??????  %s", sym);
			break;
		case  3:
			sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
			sprintf(buffer, "??????  %s", sym);
			break;
		case  4:
			sym = set_ea_info(0, cpu_get_reg(SH2_R0) + cpu_get_reg(SH2_R0+Rm), EA_UINT8, EA_MEM_RD);
			sprintf(buffer, "MOV.B   R%d,@(R0,R%d)", Rn, Rm);
			break;
		case  5:
			sym = set_ea_info(0, cpu_get_reg(SH2_R0) + cpu_get_reg(SH2_R0+Rm), EA_UINT16, EA_MEM_RD);
			sprintf(buffer, "MOV.W   R%d,@(R0,R%d)", Rn, Rm);
			break;
		case  6:
			sym = set_ea_info(0, cpu_get_reg(SH2_R0) + cpu_get_reg(SH2_R0+Rm), EA_UINT32, EA_MEM_RD);
			sprintf(buffer, "MOV.L   R%d,@(R0,R%d)", Rn, Rm);
			break;
		case  7:
			sprintf(buffer, "MUL.L   R%d,R%d\n", Rn, Rm);
			break;
		case  8:
			sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
			sprintf(buffer, "??????  %s", sym);
			break;
		case  9:
			sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
			sprintf(buffer, "??????  %s", sym);
			break;
		case 10:
			sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
			sprintf(buffer, "??????  %s", sym);
			break;
		case 11:
			sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
			sprintf(buffer, "??????  %s", sym);
			break;
		case 12:
			sym = set_ea_info(0, cpu_get_reg(SH2_R0) + cpu_get_reg(SH2_R0+Rm), EA_UINT8, EA_MEM_WR);
			sprintf(buffer, "MOV.B   @(R0,R%d),R%d", Rn, Rm);
			break;
		case 13:
			sym = set_ea_info(0, cpu_get_reg(SH2_R0) + cpu_get_reg(SH2_R0+Rm), EA_UINT16, EA_MEM_WR);
			sprintf(buffer, "MOV.W   @(R0,R%d),R%d", Rn, Rm);
			break;
		case 14:
			sym = set_ea_info(0, cpu_get_reg(SH2_R0) + cpu_get_reg(SH2_R0+Rm), EA_UINT32, EA_MEM_WR);
			sprintf(buffer, "MOV.L   @(R0,R%d),R%d", Rn, Rm);
			break;
		case 15:
			set_ea_info(0, cpu_get_reg(SH2_R0+Rn), EA_UINT32, EA_MEM_RD);
			set_ea_info(1, cpu_get_reg(SH2_R0+Rm), EA_UINT32, EA_MEM_RD);
			sprintf(buffer, "MAC.L   @R%d+,@R%d+", Rn, Rm);
			break;
		}
	}
}

void code0001(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	sym = set_ea_info(0, (opcode & 15) * 4, EA_UINT8, EA_VALUE);
	set_ea_info(1, cpu_get_reg(SH2_R0+Rm) + (opcode & 15) * 4, EA_UINT32, EA_MEM_RD);
	sprintf(buffer,"MOV.L   R%d,@(%s,R%d)\n", Rn, sym, Rm);
}

void code0010(char *buffer, UINT32 pc, UINT16 opcode)
{
	switch (opcode & 15)
	{
	case  0:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rm), EA_UINT8, EA_MEM_WR);
		sprintf(buffer, "MOV.B   R%d,@R%d", Rm, Rn);
		break;
	case  1:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rm), EA_UINT16, EA_MEM_WR);
		sprintf(buffer, "MOV.W   R%d,@R%d", Rm, Rn);
		break;
	case  2:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rm), EA_UINT32, EA_MEM_WR);
		sprintf(buffer, "MOV.L   R%d,@R%d", Rm, Rn);
		break;
	case  3:
		sprintf(buffer, "??????  $%04X", opcode);
		break;
	case  4:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rm) - 1, EA_UINT8, EA_MEM_WR);
		sprintf(buffer, "MOV.B   R%d,@-R%d", Rm, Rn);
		break;
	case  5:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rm) - 2, EA_UINT16, EA_MEM_WR);
		sprintf(buffer, "MOV.W   R%d,@-R%d", Rm, Rn);
		break;
	case  6:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rm) - 4, EA_UINT32, EA_MEM_WR);
		sprintf(buffer, "MOV.L   R%d,@-R%d", Rm, Rn);
		break;
	case  7:
		sprintf(buffer, "DIV0S   R%d,R%d", Rm, Rn);
		break;
	case  8:
		sprintf(buffer, "TST     R%d,R%d", Rm, Rn);
		break;
	case  9:
		sprintf(buffer, "AND     R%d,R%d", Rm, Rn);
		break;
	case 10:
		sprintf(buffer, "XOR     R%d,R%d", Rm, Rn);
		break;
	case 11:
		sprintf(buffer, "OR      R%d,R%d", Rm, Rn);
		break;
	case 12:
		sprintf(buffer, "CMP/STR R%d,R%d", Rm, Rn);
		break;
	case 13:
		sprintf(buffer, "XTRCT   R%d,R%d", Rm, Rn);
		break;
	case 14:
		sprintf(buffer, "MULU.W  R%d,R%d", Rm, Rn);
		break;
	case 15:
		sprintf(buffer, "MULS.W  R%d,R%d", Rm, Rn);
		break;
	}
}

void code0011(char *buffer, UINT32 pc, UINT16 opcode)
{
	switch (opcode & 15)
	{
	case  0:
		sprintf(buffer, "CMP/EQ  R%d,R%d", Rm, Rn);
		break;
	case  1:
		sprintf(buffer, "??????  R%d,R%d", Rm, Rn);
		break;
	case  2:
		sprintf(buffer, "CMP/HS  R%d,R%d", Rm, Rn);
		break;
	case  3:
		sprintf(buffer, "CMP/GE  R%d,R%d", Rm, Rn);
		break;
	case  4:
		sprintf(buffer, "DIV1    R%d,R%d", Rm, Rn);
		break;
	case  5:
		sprintf(buffer, "DMULU.L R%d,R%d", Rm, Rn);
		break;
	case  6:
		sprintf(buffer, "CMP/HI  R%d,R%d", Rm, Rn);
		break;
	case  7:
		sprintf(buffer, "CMP/GT  R%d,R%d", Rm, Rn);
		break;
	case  8:
		sprintf(buffer, "SUB     R%d,R%d", Rm, Rn);
		break;
	case  9:
		sprintf(buffer, "??????  R%d,R%d", Rm, Rn);
		break;
	case 10:
		sprintf(buffer, "SUBC    R%d,R%d", Rm, Rn);
		break;
	case 11:
		sprintf(buffer, "SUBV    R%d,R%d", Rm, Rn);
		break;
	case 12:
		sprintf(buffer, "ADD     R%d,R%d", Rm, Rn);
		break;
	case 13:
		sprintf(buffer, "DMULS.L R%d,R%d", Rm, Rn);
		break;
	case 14:
		sprintf(buffer, "ADDC    R%d,R%d", Rm, Rn);
		break;
	case 15:
		sprintf(buffer, "ADDV    R%d,R%d", Rm, Rn);
		break;
	}
}

void code0100(char *buffer, UINT32 pc, UINT16 opcode)
{
	switch(opcode & 0x3F)
	{
	case 0x00:
		sprintf(buffer, "SHLL    R%d",Rn);
		break;
	case 0x01:
		sprintf(buffer, "SHLR    R%d",Rn);
		break;
	case 0x02:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rn) - 4, EA_UINT32, EA_MEM_WR);
		sprintf(buffer, "STS.L   MACH,@-R%d", Rn);
		break;
	case 0x03:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rn) - 4, EA_UINT32, EA_MEM_WR);
		sprintf(buffer, "STC.L   SR,@-R%d", Rn);
		break;
	case 0x04:
		sprintf(buffer, "ROTL    R%d",Rn);
		break;
	case 0x05:
		sprintf(buffer, "ROTR    R%d",Rn);
		break;
	case 0x06:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rn), EA_UINT32, EA_MEM_RD);
		sprintf(buffer, "LDS.L   @R%d+,MACH", Rn);
		break;
	case 0x07:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rn), EA_UINT32, EA_MEM_RD);
		sprintf(buffer, "LDC.L   @R%d+,SR", Rn);
		break;
	case 0x08:
		sprintf(buffer, "SHLL2   R%d", Rn);
		break;
	case 0x09:
		sprintf(buffer, "SHLR2   R%d", Rn);
		break;
	case 0x0a:
		sprintf(buffer, "LDS     R%d,MACH", Rn);
		break;
	case 0x0b:
		sprintf(buffer, "JSR     R%d", Rn);
		break;
	case 0x0e:
		sprintf(buffer, "LDC     R%d,SR", Rn);
		break;
	case 0x10:
		sprintf(buffer, "DT      R%d", Rn);
		break;
	case 0x11:
		sprintf(buffer, "CMP/PZ  R%d" ,Rn);
		break;
	case 0x12:
		sprintf(buffer, "STS.L   MACL,-R%d", Rn);
		break;
	case 0x13:
		sprintf(buffer, "STC.L   GBR,-R%d", Rn);
		break;
	case 0x15:
		sprintf(buffer, "CMP/PL  R%d", Rn);
		break;
	case 0x16:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rn), EA_UINT32, EA_MEM_RD);
		sprintf(buffer, "STS.L   @R%d+,MACL", Rn);
		break;
	case 0x17:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rn), EA_UINT32, EA_MEM_RD);
		sprintf(buffer, "STC.L   @R%d+,GBR", Rn);
		break;
	case 0x18:
		sprintf(buffer, "SHLL8   R%d",Rn);
		break;
	case 0x19:
		sprintf(buffer, "SHLR8   R%d",Rn);
		break;
	case 0x1a:
		sprintf(buffer, "LDS     R%d,MACL",Rn);
		break;
	case 0x1b:
		sprintf(buffer, "TAS     R%d",Rn);
		break;
	case 0x1e:
		sprintf(buffer, "LDC     R%d,GBR",Rn);
		break;
	case 0x20:
		sprintf(buffer, "SHAL    R%d",Rn);
		break;
	case 0x21:
		sprintf(buffer, "SHAR    R%d",Rn);
		break;
	case 0x22:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rn) - 4, EA_UINT32, EA_MEM_WR);
		sprintf(buffer, "STS.L   PR,@-R%d", Rn);
		break;
	case 0x23:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rn) - 4, EA_UINT32, EA_MEM_WR);
		sprintf(buffer, "STC.L   VBR,@-R%d", Rn);
		break;
	case 0x24:
		sprintf(buffer, "ROTCL   R%d",Rn);
		break;
	case 0x25:
		sprintf(buffer, "ROTCR   R%d",Rn);
		break;
	case 0x26:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rn), EA_UINT32, EA_MEM_RD);
		sprintf(buffer, "LDS.L   @R%d+,PR", Rn);
		break;
	case 0x27:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rn), EA_UINT32, EA_MEM_RD);
		sprintf(buffer, "LDC.L   @R%d+,VBR", Rn);
		break;
	case 0x28:
		sprintf(buffer, "SHLL16  R%d",Rn);
		break;
	case 0x29:
		sprintf(buffer, "SHLR16  R%d",Rn);
		break;
	case 0x2a:
		sprintf(buffer, "LDS     R%d,PR",Rn);
		break;
	case 0x2b:
		sprintf(buffer, "JMP     R%d",Rn);
		break;
	case 0x2e:
		sprintf(buffer, "LDC     R%d,VBR",Rn);
		break;
	default:
		if ((opcode & 15) == 15)
		{
			set_ea_info(0, cpu_get_reg(SH2_R0+Rm), EA_UINT32, EA_MEM_RD);
			set_ea_info(1, cpu_get_reg(SH2_R0+Rn), EA_UINT32, EA_MEM_RD);
			sprintf(buffer, "MAC.W   @R%d+,@R%d+",Rm,Rn);
		}
		else
			sprintf(buffer, "??????  $%04X", opcode);
	}
}

void code0101(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	sym = set_ea_info(0, (opcode & 15) * 4, EA_UINT8, EA_VALUE);
	set_ea_info(1, cpu_get_reg(SH2_R0+Rm) + (opcode & 15) * 4, EA_UINT32, EA_MEM_RD);
	sprintf(buffer, "MOV.L   @(%s,R%d),R%d\n", sym, Rm, Rn);
}

void code0110(char *buffer, UINT32 pc, UINT16 opcode)

{
	switch(opcode & 0xF)
	{
	case 0x00:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rm), EA_UINT8, EA_MEM_RD);
		sprintf(buffer, "MOV.B   @R%d,R%d", Rm, Rn);
		break;
	case 0x01:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rm), EA_UINT16, EA_MEM_RD);
		sprintf(buffer, "MOV.W   @R%d,R%d", Rm, Rn);
		break;
	case 0x02:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rm), EA_UINT32, EA_MEM_RD);
		sprintf(buffer, "MOV.L   @R%d,R%d", Rm, Rn);
		break;
	case 0x03:
		sprintf(buffer, "MOV     R%d,R%d", Rm, Rn);
		break;
	case 0x04:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rm), EA_UINT8, EA_MEM_RD);
		sprintf(buffer, "MOV.B   @R%d+,R%d", Rm, Rn);
		break;
	case 0x05:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rm), EA_UINT16, EA_MEM_RD);
		sprintf(buffer, "MOV.W   @R%d+,R%d", Rm, Rn);
		break;
	case 0x06:
		set_ea_info(0, cpu_get_reg(SH2_R0+Rm), EA_UINT32, EA_MEM_RD);
		sprintf(buffer, "MOV.L   @R%d+,R%d", Rm, Rn);
		break;
	case 0x07:
		sprintf(buffer, "NOT     R%d,R%d", Rm, Rn);
		break;
	case 0x08:
		sprintf(buffer, "SWAP.B  R%d,R%d", Rm, Rn);
		break;
	case 0x09:
		sprintf(buffer, "SWAP.W  R%d,R%d", Rm, Rn);
		break;
	case 0x0a:
		sprintf(buffer, "NEGC    R%d,R%d", Rm, Rn);
		break;
	case 0x0b:
		sprintf(buffer, "NEG     R%d,R%d", Rm, Rn);
		break;
	case 0x0c:
		sprintf(buffer, "EXTU.B  R%d,R%d", Rm, Rn);
		break;
	case 0x0d:
		sprintf(buffer, "EXTU.W  R%d,R%d", Rm, Rn);
		break;
	case 0x0e:
		sprintf(buffer, "EXTS.B  R%d,R%d", Rm, Rn);
		break;
	case 0x0f:
		sprintf(buffer, "EXTS.W  R%d,R%d", Rm, Rn);
		break;
	}
}

void code0111(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
	sprintf(buffer, "ADD     #%s,R%d\n", sym, Rn);
}

void code1000(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	switch((opcode >> 8) & 15)
	{
	case  0:
		sym = set_ea_info(0, (opcode & 15), EA_UINT8, EA_VALUE);
		set_ea_info(1, cpu_get_reg(SH2_R0+Rm) + (opcode & 15), EA_UINT8, EA_MEM_WR);
		sprintf(buffer, "MOV.B   R0,@(%s,R%d)", sym, Rm);
		break;
	case  1:
		sym = set_ea_info(0, (opcode & 15) * 2, EA_UINT8, EA_VALUE);
		set_ea_info(1, cpu_get_reg(SH2_R0+Rm) + (opcode & 15)*2, EA_UINT16, EA_MEM_WR);
		sprintf(buffer, "MOV.W   R0,@(%s,R%d)", sym, Rm);
		break;
	case  4:
		sym = set_ea_info(0, (opcode & 15), EA_UINT8, EA_VALUE);
		set_ea_info(1, cpu_get_reg(SH2_R0+Rm) + (opcode & 15), EA_UINT8, EA_MEM_RD);
		sprintf(buffer, "MOV.B   @(%s,R%d),R0", sym, Rm);
		break;
	case  5:
		sym = set_ea_info(0, (opcode & 15), EA_UINT8, EA_VALUE);
		set_ea_info(1, cpu_get_reg(SH2_R0+Rm) + (opcode & 15)*2, EA_UINT16, EA_MEM_RD);
		sprintf(buffer, "MOV.W   @(%s,R%d),R0", sym, Rm);
		break;
	case  8:
		sym = set_ea_info(0, (opcode & 0xff), EA_UINT8, EA_VALUE);
		sprintf(buffer, "CMP/EQ  #%s,R0", sym);
		break;
	case  9:
		sym = set_ea_info(0, pc, SIGNX8(opcode & 0xff) * 2 + 2, EA_REL_PC);
		sprintf(buffer, "BT      %s", sym);
		break;
	case 11:
		sym = set_ea_info(0, pc, SIGNX8(opcode & 0xff) * 2 + 2, EA_REL_PC);
		sprintf(buffer, "BF      %s", sym);
		break;
	case 13:
		sym = set_ea_info(0, pc, SIGNX8(opcode & 0xff) * 2 + 2, EA_REL_PC);
		sprintf(buffer, "BTS     %s", sym);
		break;
	case 15:
		sym = set_ea_info(0, pc, SIGNX8(opcode & 0xff) * 2 + 2, EA_REL_PC);
		sprintf(buffer, "BFS     %s", sym);
		break;
	default :
		sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
		sprintf(buffer, "invalid %s\n", sym);
	}
}

void code1001(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	sym = set_ea_info(0, (opcode & 0xff) * 2, EA_UINT32, EA_VALUE);
	set_ea_info(0, (opcode & 0xff) * 2 + pc + 2, EA_UINT16, EA_MEM_RD);
	sprintf(buffer, "MOV.W   @(%s,PC),R%d", sym, Rn);
}

void code1010(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	sym = set_ea_info(0, SIGNX12(opcode & 0xfff) * 2 + pc + 2, EA_UINT32, EA_ABS_PC);
	sprintf(buffer, "BRA     %s", sym);
}

void code1011(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	sym = set_ea_info(0, SIGNX12(opcode & 0xfff) * 2 + pc + 2, EA_UINT32, EA_ABS_PC);
	sprintf(buffer, "BSR     %s", sym);
}

void code1100(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	switch((opcode >> 8) & 15)
	{
	case  0:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		set_ea_info(1, cpu_get_reg(SH2_GBR) + (opcode & 0xff), EA_UINT8, EA_MEM_WR);
		sprintf(buffer, "MOV.B   R0,@(%s,GBR)", sym);
		break;
	case  1:
		sym = set_ea_info(0, (opcode & 0xff) * 2, EA_UINT8, EA_VALUE);
		set_ea_info(1, cpu_get_reg(SH2_GBR) + (opcode & 0xff) * 2, EA_UINT16, EA_MEM_WR);
		sprintf(buffer, "MOV.W   R0,@(%s,GBR)", sym);
		break;
	case  2:
		sym = set_ea_info(0, (opcode & 0xff) * 4, EA_UINT8, EA_VALUE);
		set_ea_info(1, cpu_get_reg(SH2_GBR) + (opcode & 0xff) * 4, EA_UINT32, EA_MEM_WR);
		sprintf(buffer, "MOV.L   R0,@($%04X,GBR)", (opcode & 0xff) * 4);
		break;
	case  3:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		sprintf(buffer, "TRAPA   #%s", sym);
		break;
	case  4:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		set_ea_info(1, cpu_get_reg(SH2_GBR) + (opcode & 0xff), EA_UINT8, EA_MEM_RD);
		sprintf(buffer, "MOV.B   @(%s,GBR),R0", sym);
		break;
	case  5:
		sym = set_ea_info(0, (opcode & 0xff) * 2, EA_UINT8, EA_VALUE);
		set_ea_info(1, cpu_get_reg(SH2_GBR) + (opcode & 0xff) * 2, EA_UINT16, EA_MEM_RD);
		sprintf(buffer, "MOV.W   @(%s,GBR),R0", sym);
		break;
	case  6:
		sym = set_ea_info(0, (opcode & 0xff) * 4, EA_UINT8, EA_VALUE);
		set_ea_info(1, cpu_get_reg(SH2_GBR) + (opcode & 0xff) * 4, EA_UINT32, EA_MEM_RD);
		sprintf(buffer, "MOV.L   @(%s,GBR),R0", sym);
		break;
	case  7:
		sym = set_ea_info(0, (opcode & 0xff) * 4, EA_UINT8, EA_VALUE);
		set_ea_info(1, (opcode & 0xff) * 4 + pc + 2, EA_UINT32, EA_ABS_PC);
		sprintf(buffer, "MOVA    @(%s,PC),R0", sym);
		break;
	case  8:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		sprintf(buffer, "TST     #%s,R0", sym);
		break;
	case  9:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		sprintf(buffer, "AND     #%s,R0", sym);
		break;
	case 10:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		sprintf(buffer, "XOR     #%s,R0", sym);
		break;
	case 11:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		sprintf(buffer, "OR      #%s,R0", sym);
		break;
	case 12:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		set_ea_info(1, cpu_get_reg(SH2_R0) + cpu_get_reg(SH2_GBR), EA_UINT8, EA_MEM_RD);
		sprintf(buffer, "TST.B   #%s,@(R0,GBR)", sym);
		break;
	case 13:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		set_ea_info(1, cpu_get_reg(SH2_R0) + cpu_get_reg(SH2_GBR), EA_UINT8, EA_MEM_RDWR);
		sprintf(buffer, "AND.B   #%s,@(R0,GBR)", sym);
		break;
	case 14:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		set_ea_info(1, cpu_get_reg(SH2_R0) + cpu_get_reg(SH2_GBR), EA_UINT8, EA_MEM_RDWR);
		sprintf(buffer, "XOR.B   #%s,@(R0,GBR)", sym);
		break;
	case 15:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		set_ea_info(1, cpu_get_reg(SH2_R0) + cpu_get_reg(SH2_GBR), EA_UINT8, EA_MEM_RDWR);
		sprintf(buffer, "OR.B    #%s,@(R0,GBR)", sym);
		break;
	}
}

void code1101(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	sym = set_ea_info(0, (opcode & 0xff) * 4, EA_UINT8, EA_VALUE);
	set_ea_info(1, (opcode & 0xff) * 4 + ((pc + 2) & ~3), EA_UINT32, EA_ABS_PC);
	sprintf(buffer, "MOV.L   @(%s,PC),R%d", sym, Rn);
}

void code1110(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	sym = set_ea_info(0, (UINT32)(INT32)(INT16)(INT8)(opcode & 0xff), EA_UINT8, EA_VALUE);
	sprintf(buffer, "MOV     #%s,R%d", sym, Rn);
}

void code1111(char *buffer, UINT32 pc, UINT16 opcode)
{
	sprintf(buffer, "unknown $%04X", opcode);
}

unsigned DasmSH2(char *buffer, unsigned pc)
{
	UINT16 opcode;
	opcode = cpu_readmem27bew_word(pc & 0x1fffffff);
	pc += 2;

	switch((opcode >> 12) & 15)
	{
	case  0: code0000(buffer,pc,opcode);	break;
	case  1: code0001(buffer,pc,opcode);	break;
	case  2: code0010(buffer,pc,opcode);	break;
	case  3: code0011(buffer,pc,opcode);	break;
	case  4: code0100(buffer,pc,opcode);	break;
	case  5: code0101(buffer,pc,opcode);	break;
	case  6: code0110(buffer,pc,opcode);	break;
	case  7: code0111(buffer,pc,opcode);	break;
	case  8: code1000(buffer,pc,opcode);	break;
	case  9: code1001(buffer,pc,opcode);	break;
	case 10: code1010(buffer,pc,opcode);	break;
	case 11: code1011(buffer,pc,opcode);	break;
	case 12: code1100(buffer,pc,opcode);	break;
	case 13: code1101(buffer,pc,opcode);	break;
	case 14: code1110(buffer,pc,opcode);	break;
	default: code1111(buffer,pc,opcode);	break;
	}
	return 2;
}

#endif

