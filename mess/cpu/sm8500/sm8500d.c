/*******************************************************************

sm8500d.c
Sharp sm8500 CPU disassembly



*******************************************************************/

#include <stdio.h>
#include <string.h>
#ifdef MAME_DEBUG
#include "driver.h"
#include "debugger.h"
#include "eainfo.h"
#include "sm8500.h"

enum e_mnemonics
{
	zADC=0, zADCW, zADD, zADDW, zAND, zANDW, zBAND, zBBC, zBBS,
	zBCLR, zBCMP, zBMOV, zBOR, zBR, zBTST, zBSET, zBXOR, zCALL, zCALS, zCLR,
	zCLRC, zCMP, zCMPW, zCOM, zCOMC, zDA, zDBNZ, zDEC,
	zDECW, zDI, zDIV, zEI, zEXTS, zHALT, zINC, zINCW,
	zIRET, zJMP, zMOV, zMOVM, zMOVW, zMULT, zNEG, zNOP, zOR,
	zORW, zPOP, zPOPW, zPUSH, zPUSHW, zRET, zRL, zRLC,
	zRR, zRRC, zSBC, zSBCW, zSETC, zSLL, zSRA, zSRL, zSTOP,
	zSUB, zSUBW, zSWAP, zXOR, zXORW, zMOVPS0, zINVLD, zDM,
/* unknowns */
z5A, z5B,

/* more complicated instructions */
z1A, z1B, z4F,
};

/* instructions not found:
5A, 5B,
*/

static const char *s_mnemonic[] =
{
	"adc",  "adcw", "add",  "addw", "and",  "andw",  "band", "bbc",  "bbs",
	"bclr", "bcmp", "bmov", "bor",  "br",   "btst", "bset",  "bxor", "call", "cals", "clr",
	"clrc", "cmp",  "cmpw", "com",  "comc",  "da",   "dbnz", "dec",
	"decw", "di",   "div",  "ei",   "exts",  "halt", "inc",  "incw",
	"iret", "jmp",  "mov",  "movm", "movw",  "mult", "neg",  "nop",  "or",
	"orw",  "pop",  "popw", "push", "pushw", "ret",  "rl",   "rlc",
	"rr",   "rrc",  "sbc",  "sbcw", "setc", "sll",   "sra",  "srl",  "stop",
	"sub",  "subw", "swap", "xor",  "xorw", "mov PS0,", "invalid", "dm?", 
/* unknowns */
"unk5A", "unk5B",

/* more complicated instructions */
"comp1A", "comp1B", "comp4F",
};

typedef struct
{
	UINT8	access;
	UINT8	mnemonic;
	UINT8	arguments;
}	sm8500dasm;

/* We can probably get rid of this list of registers */
static char *sm8500_regs_R[256] = {
"00"   , "01"   , "02"   , "03"   , "04"   , "05"   , "06"   , "07"  ,
"08"   , "09"   , "0A"   , "0B"   , "0C"   , "0D"   , "0E"   , "0F" ,
"IE0"  , "IE1"  , "IR0"  , "IR1"  , "P0"   , "P1"   , "P2"   , "P3"   ,
"18"   , "SYS"  , "CKC"  , "1B"   , "SPH"  , "SPL"  , "PS0"  , "PS1"  ,
"P0C"  , "P1C"  , "P2C"  , "P3C"  , "MMU0" , "MMU1" , "MMU2" , "MMU3" ,
"MMU4" , "29"   , "2A"   , "URTT" , "URTR" , "URTS" , "URTC" , "2F" ,
"LDC"  , "LCH"  , "LCV"  , "32"   , "DMC"  , "DMX1" , "DMY1" , "DMDX" ,
"DMDY" , "DMX2" , "DMY2" , "DMPL" , "DMBR" , "DMVP" , "3E"   , "3F" ,
"SGC"  , "41"   , "SG0L" , "43"   , "SG1L" , "45"   , "SG0TH", "SG0TL",
"SG1TH", "SG1TL", "SG2L" , "_4Bh" , "SG2TH", "SG2TL", "SGDA" , "4F" ,
"TM0C" , "TM0D" , "TM1C" , "TM1D" , "CLKT" , "55"   , "56"   , "57" ,
"58"   , "59"   , "5A"   , "5B"   , "5C"   , "5D"   , "WDT"  , "WDTC" ,
"SG0W0", "SG0W1", "SG0W2", "SG0W3", "SG0W4", "SG0W5", "SG0W6", "SG0W7",
"SG0W8", "SG0W9", "SG0WA", "SG0WB", "SG0WC", "SG0WD", "SG0WE", "SG0WF",
"SG1W0", "SG1W1", "SG1W2", "SG1W3", "SG1W4", "SG1W5", "SG1W6", "SG1W7",
"SG1W8", "SG1W9", "SG1WA", "SG1WB", "SG1WC", "SG1WD", "SG1WE", "SG1WF",
"80"   , "81"   , "82"   , "83"   , "84"   , "85"   , "86"   , "87" ,
"88"   , "89"   , "8A"   , "8B"   , "8C"   , "8D"   , "8E"   , "8F" ,
"90"   , "91"   , "92"   , "93"   , "94"   , "95"   , "96"   , "97" ,
"98"   , "99"   , "9A"   , "9B"   , "9C"   , "9D"   , "9E"   , "9F" ,
"A0"   , "A1"   , "A2"   , "A3"   , "A4"   , "A5"   , "A6"   , "A7" ,
"A8"   , "A9"   , "AA"   , "AB"   , "AC"   , "AD"   , "AE"   , "AF" ,
"B0"   , "B1"   , "B2"   , "B3"   , "B4"   , "B5"   , "B6"   , "B7" ,
"B8"   , "B9"   , "BA"   , "BB"   , "BC"   , "BD"   , "BE"   , "BF" ,
"C0"   , "C1"   , "C2"   , "C3"   , "C4"   , "C5"   , "C6"   , "C7" ,
"C8"   , "C9"   , "CA"   , "CB"   , "CC"   , "CD"   , "CE"   , "CF" ,
"D0"   , "D1"   , "D2"   , "D3"   , "D4"   , "D5"   , "D6"   , "D7" ,
"D8"   , "D9"   , "DA"   , "DB"   , "DC"   , "DD"   , "DE"   , "DF" ,
"E0"   , "E1"   , "E2"   , "E3"   , "E4"   , "E5"   , "E6"   , "E7" ,
"E8"   , "E9"   , "EA"   , "EB"   , "EC"   , "ED"   , "EE"   , "EF" ,
"F0"   , "F1"   , "F2"   , "F3"   , "F4"   , "F5"   , "F6"   , "F7" ,
"F8"   , "F9"   , "FA"   , "FB"   , "FC"   , "FD"   , "FE"   , "FF"
};

static char *sm8500_cond[16] = {
	"F", "LT", "LE", "ULE", "OV",  "MI", "Z",  "C",
	"T", "GE", "GT", "UGT", "NOV", "PL", "NZ", "NC"
};

static UINT8 sm8500_b2w[8] = {
	0, 8, 2, 10, 4, 12, 6, 14
};

#define _00	EA_NONE
#define _JP	EA_ABS_PC
#define _JR	EA_REL_PC
#define _RW	EA_MEM_RDWR
#define _WR	EA_MEM_WR

enum e_addrmodes {
	AM_R=1, AM_rr, AM_r1, AM_S, AM_rmb, AM_mbr, AM_Ri, AM_rmw, AM_mwr, AM_smw, AM_mws,
	AM_Sw, AM_iR, AM_rbr, AM_riw, AM_cjp, AM_rib, AM_pi, AM_cbr, AM_i, AM_ii,
	AM_ss, AM_RR, AM_2, AM_SS, AM_bR, AM_Rbr, AM_Rb, AM_rR, AM_Rr, AM_Rii, AM_RiR,
	AM_riB, AM_iS, AM_CALS, AM_bid, AM_1A, AM_1B, AM_4F,
};

static sm8500dasm mnemonic[256] = {
	/* 00 - 0F */
        {_RW,zCLR, AM_R},  {_RW,zNEG,AM_R},   {_RW,zCOM,AM_R},   {_RW,zRR,AM_R},
        {_RW,zRL, AM_R},   {_RW,zRRC,AM_R},  {_RW,zRLC,AM_R},   {_RW,zSRL,AM_R},
        {_RW,zINC, AM_R},  {_RW,zDEC,AM_R},  {_RW,zSRA,AM_R},    {_RW,zSLL,AM_R},
        {_RW,zDA, AM_R},   {_RW,zSWAP,AM_R}, {_RW,zPUSH,AM_R},  {_RW,zPOP,AM_R},
	/* 10 - 1F */
        {_00,zCMP,AM_rr},  {_00,zADD,AM_rr},  {_00,zSUB,AM_rr},   {_00,zADC,AM_rr},
        {_00,zSBC,AM_rr},  {_00,zAND,AM_rr},  {_00,zOR,AM_rr},    {_00,zXOR,AM_rr},
        {_00,zINCW,AM_S}, {_00,zDECW,AM_S}, {_00,z1A,AM_1A},    {_00,z1B,AM_1B},
        {_RW,zBCLR,AM_riB}, {_RW,zBSET,AM_riB},   {_00,zPUSHW,AM_S}, {_00,zPOPW,AM_S},
	/* 20 - 2F */
        {_00,zCMP,AM_rmb},   {_00,zADD,AM_rmb},  {_00,zSUB,AM_rmb},    {_00,zADC,AM_rmb},
        {_00,zSBC,AM_rmb},   {_00,zAND,AM_rmb},   {_00,zOR,AM_rmb},    {_00,zXOR,AM_rmb},
        {_00,zMOV,AM_rmb},  {_00,zMOV,AM_mbr},  {_JR,zBBC,AM_bid},    {_JR,zBBS,AM_bid},
        {_00,zEXTS,AM_R}, {_00,zDM,AM_i},   {_00,zMOVPS0,AM_i},    {_00,zBTST,AM_Ri},
	/* 30 - 3F */
        {_RW,zCMP,AM_rmw},  {_RW,zADD,AM_rmw},  {_RW,zSUB,AM_rmw},   {_RW,zADC,AM_rmw},
        {_RW,zSBC,AM_rmw},  {_RW,zAND,AM_rmw},   {_RW,zOR,AM_rmw},    {_RW,zXOR,AM_rmw},
        {_RW,zMOV,AM_rmw},  {_00,zMOV,AM_mwr},  {_00,zMOVW,AM_smw},  {_00,zMOVW,AM_mws},
        {_00,zMOVW,AM_ss}, {_00,zDM,AM_R},   {_00,zJMP,AM_2},   {_00,zCALL,AM_2},
	/* 40 - 4F */
        {_00,zCMP,AM_RR},  {_00,zADD,AM_RR},  {_00,zSUB,AM_RR},   {_00,zADC,AM_RR},
        {_00,zSBC,AM_RR},  {_00,zAND,AM_RR},  {_00,zOR,AM_RR},    {_00,zXOR,AM_RR},
        {_00,zMOV,AM_RR},  {_00,zCALL,AM_ii}, {_00,zMOVW,AM_SS},  {_WR,zMOVW,AM_Sw},
        {_00,zMULT,AM_RR}, {_00,zMULT,AM_iR}, {_00,zBMOV,AM_bR},  {_00,z4F,AM_4F},
	/* 50 - 5F */
        {_WR,zCMP,AM_iR},  {_WR,zADD,AM_iR},  {_WR,zSUB,AM_iR},   {_WR,zADC,AM_iR},
        {_WR,zSBC,AM_iR},  {_WR,zAND,AM_iR},  {_WR,zOR,AM_iR},    {_WR,zXOR,AM_iR},
        {_WR,zMOV, AM_iR}, {_00,zINVLD,0},   {_00,z5A,AM_ii},    {_00,z5B,AM_ii},
        {_00,zDIV,AM_SS},  {_00,zDIV,AM_iS},   {_00,zMOVM,AM_RiR},  {_00,zMOVM,AM_Rii},
	/* 60 - 6F */
        {_00,zCMPW,AM_SS}, {_00,zADDW,AM_SS}, {_00,zSUBW,AM_SS},  {_00,zADCW,AM_SS},
        {_00,zSBCW,AM_SS}, {_00,zANDW,AM_SS}, {_00,zORW,AM_SS},   {_00,zXORW,AM_SS},
        {_00,zCMPW,AM_Sw}, {_00,zADDW,AM_Sw}, {_00,zSUBW,AM_Sw},  {_00,zADCW,AM_Sw},
        {_00,zSBCW,AM_Sw}, {_00,zANDW,AM_Sw}, {_00,zORW,AM_Sw},   {_00,zXORW,AM_Sw},
	/* 70 - 7F */
        {_JR,zDBNZ,AM_rbr}, {_JR,zDBNZ,AM_rbr}, {_JR,zDBNZ,AM_rbr},  {_JR,zDBNZ,AM_rbr},
        {_JR,zDBNZ,AM_rbr}, {_JR,zDBNZ,AM_rbr}, {_JR,zDBNZ,AM_rbr},  {_JR,zDBNZ,AM_rbr},
        {_WR,zMOVW,AM_riw}, {_WR,zMOVW,AM_riw}, {_WR,zMOVW,AM_riw},  {_WR,zMOVW,AM_riw},
        {_WR,zMOVW,AM_riw}, {_WR,zMOVW,AM_riw}, {_WR,zMOVW,AM_riw},  {_WR,zMOVW,AM_riw},
	/* 80 - 8F */
        {_JR,zBBC,AM_Rbr},  {_JR,zBBC,AM_Rbr},  {_JR,zBBC,AM_Rbr},   {_JR,zBBC,AM_Rbr},
        {_JR,zBBC,AM_Rbr},  {_JR,zBBC,AM_Rbr},  {_JR,zBBC,AM_Rbr},   {_JR,zBBC,AM_Rbr},
        {_JR,zBBS,AM_Rbr},  {_JR,zBBS,AM_Rbr},  {_JR,zBBS,AM_Rbr},   {_JR,zBBS,AM_Rbr},
        {_JR,zBBS,AM_Rbr},  {_JR,zBBS,AM_Rbr},  {_JR,zBBS,AM_Rbr},   {_JR,zBBS,AM_Rbr},
	/* 90 - 9F */
        {_JP,zJMP,AM_cjp},  {_JP,zJMP,AM_cjp},  {_JP,zJMP,AM_cjp},   {_JP,zJMP,AM_cjp},
        {_JP,zJMP,AM_cjp},  {_JP,zJMP,AM_cjp},  {_JP,zJMP,AM_cjp},   {_JP,zJMP,AM_cjp},
        {_JP,zJMP,AM_cjp},  {_JP,zJMP,AM_cjp},  {_JP,zJMP,AM_cjp},   {_JP,zJMP,AM_cjp},
        {_JP,zJMP,AM_cjp},  {_JP,zJMP,AM_cjp},  {_JP,zJMP,AM_cjp},   {_JP,zJMP,AM_cjp},
	/* A0 - AF */
        {_00,zBCLR,AM_Rb}, {_00,zBCLR,AM_Rb}, {_00,zBCLR,AM_Rb},  {_00,zBCLR,AM_Rb},
        {_00,zBCLR,AM_Rb}, {_00,zBCLR,AM_Rb}, {_00,zBCLR,AM_Rb},  {_00,zBCLR,AM_Rb},
        {_00,zBSET,AM_Rb}, {_00,zBSET,AM_Rb}, {_00,zBSET,AM_Rb},  {_00,zBSET,AM_Rb},
        {_00,zBSET,AM_Rb}, {_00,zBSET,AM_Rb}, {_00,zBSET,AM_Rb},  {_00,zBSET,AM_Rb},
	/* B0 - BF */
        {_00,zMOV,AM_rR},  {_00,zMOV,AM_rR},  {_00,zMOV,AM_rR},   {_00,zMOV,AM_rR},
        {_00,zMOV,AM_rR},  {_00,zMOV,AM_rR},  {_00,zMOV,AM_rR},   {_00,zMOV,AM_rR},
        {_00,zMOV,AM_Rr},  {_00,zMOV,AM_Rr},  {_00,zMOV,AM_Rr},   {_00,zMOV,AM_Rr},
        {_00,zMOV,AM_Rr},  {_00,zMOV,AM_Rr},  {_00,zMOV,AM_Rr},   {_00,zMOV,AM_Rr},
	/* C0 - CF */
        {_WR,zMOV,AM_rib},  {_WR,zMOV,AM_rib},  {_WR,zMOV,AM_rib},   {_WR,zMOV,AM_rib},
        {_WR,zMOV,AM_rib},  {_WR,zMOV,AM_rib},  {_WR,zMOV,AM_rib},   {_WR,zMOV,AM_rib},
        {_WR,zMOV,AM_pi},  {_WR,zMOV,AM_pi},  {_WR,zMOV,AM_pi},   {_WR,zMOV,AM_pi},
        {_WR,zMOV,AM_pi},  {_WR,zMOV,AM_pi},  {_WR,zMOV,AM_pi},   {_WR,zMOV,AM_pi},
	/* D0 - DF */
        {_JR,zBR,AM_cbr},   {_JR,zBR,AM_cbr},   {_JR,zBR,AM_cbr},    {_JR,zBR,AM_cbr},
        {_JR,zBR,AM_cbr},   {_JR,zBR,AM_cbr},   {_JR,zBR,AM_cbr},    {_JR,zBR,AM_cbr},
        {_JR,zBR,AM_cbr},   {_JR,zBR,AM_cbr},   {_JR,zBR,AM_cbr},    {_JR,zBR,AM_cbr},
        {_JR,zBR,AM_cbr},   {_JR,zBR,AM_cbr},   {_JR,zBR,AM_cbr},    {_JR,zBR,AM_cbr},
	/* E0 - EF */
        {_00,zCALS,AM_CALS},   {_00,zCALS,AM_CALS},   {_00,zCALS,AM_CALS},    {_00,zCALS,AM_CALS},
        {_00,zCALS,AM_CALS},   {_00,zCALS,AM_CALS},   {_00,zCALS,AM_CALS},    {_00,zCALS,AM_CALS},
        {_00,zCALS,AM_CALS},   {_00,zCALS,AM_CALS},   {_00,zCALS,AM_CALS},    {_00,zCALS,AM_CALS},
        {_00,zCALS,AM_CALS},   {_00,zCALS,AM_CALS},   {_00,zCALS,AM_CALS},    {_00,zCALS,AM_CALS},
	/* F0 - FF */
        {_00,zSTOP,0}, {_00,zHALT,0}, {_00,zINVLD,0},    {_00,zINVLD,0},
        {_00,zINVLD,0},   {_00,zINVLD,0},   {_00,zINVLD,0},    {_00,zINVLD,0},
        {_00,zRET,0},  {_00,zIRET,0}, {_00,zCLRC,0},  {_00,zCOMC,0},
        {_00,zSETC,0}, {_00,zEI,0},   {_00,zDI,0},    {_00,zNOP,0},

};

unsigned dasm_sm8500( char *buffer, unsigned pc )
{
	sm8500dasm *instr;
	const char *symbol, *symbol2;
	char *dst;
	unsigned old_pc = pc;
	UINT8 op;
	INT8 offset = 0;
	UINT16 ea = 0, ea2 = 0;

	dst = buffer;

	op = cpu_readop( pc++ );

	instr = &mnemonic[op];

	if ( instr->arguments )
	{
		if ( instr->arguments != AM_1A || instr->arguments != AM_1B || instr->arguments != AM_4F ) {
			dst += sprintf( dst, "%-4s ", s_mnemonic[ instr->mnemonic ] );
		}
		switch( instr->arguments ) {
		case AM_R:
			ea = cpu_readop_arg( pc++ );
			set_ea_info( 0, ea, EA_UINT8, instr->access );
			dst += sprintf( dst, "R%02Xh", ea );
			break;
		case AM_iR:
			ea = cpu_readop_arg( pc++ );
			symbol = set_ea_info( 1, ea, EA_UINT8, EA_VALUE );
			ea = cpu_readop_arg( pc++ );
			set_ea_info( 0, ea, EA_UINT8, instr->access );
			dst += sprintf( dst, "R%02Xh, %s", ea, symbol );
			break;
		case AM_iS:
			ea = cpu_readop_arg( pc++ );
			symbol = set_ea_info( 1, ea, EA_UINT8, EA_VALUE );
			ea = cpu_readop_arg( pc++ );
			set_ea_info( 0, ea, EA_UINT8, instr->access );
			dst += sprintf( dst, "RR%02Xh, %s", ea, symbol );
			break;
		case AM_Sw:
			ea2 = cpu_readop_arg( pc++ );
			ea = cpu_readop_arg( pc++ ) << 8;
			ea += cpu_readop_arg( pc++ );
			symbol = set_ea_info( 1, ea, EA_UINT16, EA_VALUE );
			set_ea_info( 0, ea2, EA_UINT16, instr->access );
			dst+= sprintf( dst, "RR%02Xh, %s", ea2, symbol );
			break;
		case AM_rib:
			ea = cpu_readop_arg( pc++ );
			symbol = set_ea_info( 1, ea, EA_UINT8, EA_VALUE );
			set_ea_info( 0, (op & 0x07), EA_UINT8, instr->access );
			dst += sprintf( dst, "r%02Xh, %s", op & 0x07, symbol );
			break;
		case AM_riw:
			ea = cpu_readop_arg( pc++ ) << 8;
			ea += cpu_readop_arg( pc++ );
			symbol = set_ea_info( 1, ea, EA_UINT16, EA_VALUE );
			set_ea_info( 0, (op & 0x07), EA_UINT16, instr->access );
			dst += sprintf( dst, "rr%02Xh, %s", sm8500_b2w[op & 0x07], symbol );
			break;
		case AM_rmb:
			ea = cpu_readop_arg( pc++ );
			dst += sprintf( dst, "r%02Xh,", ( ea >> 3 ) & 0x07 );
			switch( ea & 0xC0 ) {
			case 0x00:
				dst += sprintf( dst, "@r%02Xh", ea & 0x07 ); break;
			case 0x40:
				dst += sprintf( dst, "(r%02Xh)+", ea & 0x07 ); break;
			case 0x80:
				ea2 = cpu_readop_arg( pc++ );
				symbol2 = set_ea_info( 0, ea2, EA_UINT8, EA_VALUE );
				if ( ea & 0x07 ) {
					dst += sprintf( dst, "%s(r%02Xh)", symbol2, ea & 0x07 );
				} else {
					dst += sprintf( dst, "@%s", symbol2 );
				}
				break;
			case 0xC0:
				dst += sprintf( dst, "-(r%02Xh)", ea & 0x07 ); break;
			}
			break;
		case AM_mbr:
			ea = cpu_readop( pc++ );
			switch( ea & 0xC0 ) {
			case 0x00:
				dst += sprintf( dst, "@r%02Xh", ea & 0x07 ); break;
			case 0x40:
				dst += sprintf( dst, "(r%02Xh)+", ea & 0x07 ); break;
			case 0x80:
				ea2 = cpu_readop_arg( pc++ );
				symbol2 = set_ea_info( 0, ea2, EA_UINT8, EA_VALUE );
				if ( ea & 0x07 ) {
					dst += sprintf( dst, "%s(r%02Xh)", symbol2, ea & 0x07 );
				} else {
					dst += sprintf( dst, "@%s", symbol2 );
				}
				break;
			case 0xC0:
				dst += sprintf( dst, "-(r%02Xh)", ea & 0x07 ); break;
			}
			dst += sprintf( dst, ",r%02Xh", ( ea >> 3 ) & 0x07 );
			break;
		case AM_rmw:
			ea = cpu_readop_arg( pc++ );
			dst += sprintf( dst, "r%02Xh,", ( ea >> 3 ) & 0x07 );
			switch( ea & 0xC0 ) {
			case 0x00:
				dst += sprintf( dst, "@rr%02Xh", sm8500_b2w[ea & 0x07] ); break;
			case 0x40:
				dst += sprintf( dst, "(rr%02Xh)+", sm8500_b2w[ea & 0x07] ); break;
			case 0x80:
				ea2 = cpu_readop_arg( pc++ ) << 8;
				ea2 += cpu_readop_arg( pc++ );
				symbol2 = set_ea_info( 0, ea2, EA_UINT16, EA_VALUE );
				if ( ea & 0x07 ) {
					dst += sprintf( dst, "%s(rr%02Xh)", symbol2, sm8500_b2w[ea & 0x07] );
				} else {
					dst += sprintf( dst, "@%s", symbol2 );
				}
				break;
			case 0xC0:
				dst += sprintf( dst, "-(rr%02Xh)", sm8500_b2w[ea & 0x07] ); break;
			}
			break;
		case AM_mwr:
			ea = cpu_readop_arg( pc++ );
			switch( ea & 0xC0 ) {
			case 0x00:
				dst += sprintf( dst, "@rr%02Xh", sm8500_b2w[ea & 0x07] ); break;
			case 0x40:
				dst += sprintf( dst, "(rr%02Xh)+", sm8500_b2w[ea & 0x07] ); break;
			case 0x80:
				ea2 = cpu_readop_arg( pc++ ) << 8;
				ea2 += cpu_readop_arg( pc++ );
				symbol2 = set_ea_info( 0, ea2, EA_UINT16, EA_VALUE );
				if ( ea & 0x07 ) {
					dst += sprintf( dst, "%s(rr%02Xh)", symbol2, sm8500_b2w[ea & 0x07] );
				} else {
					dst += sprintf( dst, "@%s", symbol2 );
				}
				break;
			case 0xC0:
				dst += sprintf( dst, "-(rr%02Xh)", sm8500_b2w[ea & 0x07] ); break;
			}
			dst += sprintf( dst, ",r%02Xh", ( ea >> 3 ) & 0x07 );
			break;
		case AM_smw:
			ea = cpu_readop_arg( pc++ );
			dst += sprintf( dst, "rr%02Xh,", sm8500_b2w[( ea >> 3 ) & 0x07] );
			switch( ea & 0xC0 ) {
			case 0x00:
				dst += sprintf( dst, "@rr%02Xh", sm8500_b2w[ea & 0x07] ); break;
			case 0x40:
				dst += sprintf( dst, "(rr%02Xh)+", sm8500_b2w[ea & 0x07] ); break;
			case 0x80:
				ea2 = cpu_readop_arg( pc++ ) << 8;
				ea2 += cpu_readop_arg( pc++ );
				symbol2 = set_ea_info( 0, ea2, EA_UINT16, EA_VALUE );
				if ( ea & 0x07 ) {
					dst += sprintf( dst, "%s(rr%02Xh)", symbol2, sm8500_b2w[ea & 0x07] );
				} else {
					dst += sprintf( dst, "@%s", symbol2 );
				}
				break;
			case 0xC0:
				dst += sprintf( dst, "-(rr%02Xh)", sm8500_b2w[ea & 0x07] ); break;
			}
			break;
		case AM_mws:
			ea = cpu_readop_arg( pc++ );
			switch( ea & 0xC0 ) {
			case 0x00:
				dst += sprintf( dst, "@rr%02Xh", sm8500_b2w[ea & 0x07] ); break;
			case 0x40:
				dst += sprintf( dst, "(rr%02Xh)+", sm8500_b2w[ea & 0x07] ); break;
			case 0x80:
				ea2 = cpu_readop_arg( pc++ ) << 8;
				ea2 += cpu_readop_arg( pc++ );
				symbol2 = set_ea_info( 0, ea2, EA_UINT16, EA_VALUE );
				if ( ea & 0x07 ) {
					dst += sprintf( dst, "%s(rr%02Xh)", symbol2, sm8500_b2w[ea & 0x07] );
				} else {
					dst += sprintf( dst, "@%s", symbol2 );
				}
				break;
			case 0xC0:
				dst += sprintf( dst, "-(rr%02Xh)", sm8500_b2w[ea & 0x07] ); break;
			}
			dst += sprintf( dst, ",rr%02Xh", sm8500_b2w[( ea >> 3 ) & 0x07] );
			break;
		case AM_cbr:
			offset = (INT8) cpu_readop_arg( pc ++ );
			symbol = set_ea_info( 0, old_pc, offset + 2, instr->access );
			dst += sprintf( dst, "%s,%s", sm8500_cond[ op & 0x0F ], symbol );
			break;
		case AM_rbr:
			offset = (INT8) cpu_readop_arg( pc++ );
			symbol = set_ea_info( 0, old_pc, offset + 2, instr->access );
			dst += sprintf( dst, "r%02Xh,%s", op & 0x07, symbol );
			break;
		case AM_cjp:
			ea = cpu_readop_arg( pc++ ) << 8;
			ea += cpu_readop_arg( pc++ );
			symbol = set_ea_info( 0, ea, EA_UINT16, instr->access );
			dst += sprintf( dst, "%s,%s", sm8500_cond[ op & 0x0F], symbol );
			break;
		case AM_rr:
			ea = cpu_readop_arg( pc++ );
			switch( ea & 0xc0 ) {
			case 0x00:
				dst += sprintf( dst, "r%02Xh,r%02Xh", (ea >> 3 ) & 0x07, ea & 0x07 );
				break;
			case 0x40:
			case 0x80:
			case 0xC0:
				dst += sprintf( dst, "undef" );
				break;
			}
			break;
		case AM_r1:
			ea = cpu_readop_arg( pc++ );
			switch( ea & 0xC0 ) {
				dst += sprintf( dst, "@r%02Xh", (ea >> 3 ) & 0x07 );
				break;
			case 0x40:
			case 0x80:
			case 0xC0:
				dst += sprintf( dst, "undef" );
				break;
			}
			break;
		case AM_S:
			ea = cpu_readop_arg( pc++ );
			dst += sprintf( dst, "RR%02Xh", ea );
			break;
		case AM_pi:
			ea = cpu_readop_arg( pc++ );
			symbol = set_ea_info( 1, ea, EA_UINT8, EA_VALUE );
			set_ea_info( 0, 0x10 + (op & 0x07), EA_UINT8, instr->access );
			dst += sprintf( dst, "r%02Xh, %s", 0x10 + (op & 0x07), symbol );
			break;
		case AM_Ri:
			ea = cpu_readop_arg( pc++ );
			ea2 = cpu_readop_arg( pc++ );
			symbol = set_ea_info( 0, ea2, EA_UINT8, EA_VALUE );
			dst += sprintf( dst, "R%02Xh,%s", ea, symbol );
			break;
		case AM_i:
			ea = cpu_readop_arg( pc++ );
			symbol = set_ea_info( 0, ea, EA_UINT8, EA_VALUE );
			dst += sprintf( dst, "%s", symbol );
			break;
		case AM_ii:
			ea = cpu_readop_arg( pc++ ) << 8;
			ea |= cpu_readop_arg( pc++ );
			symbol = set_ea_info( 0, ea, EA_UINT16, EA_VALUE );
			dst += sprintf( dst, "%s", symbol );
			break;
		case AM_ss:
			ea = cpu_readop_arg( pc++ );
			switch( ea & 0xC0 ) {
			case 0x00:
				dst += sprintf( dst, "rr%02Xh,rr%02Xh", sm8500_b2w[( ea >> 3 ) & 0x07], sm8500_b2w[ea & 0x07] ); break;
			case 0x40:
				dst += sprintf( dst, "undef" ); break;
			case 0x80:
				dst += sprintf( dst, "undef" ); break;
			case 0xC0:
				dst += sprintf( dst, "undef" ); break;
			}
			break;
		case AM_RR:
			ea = cpu_readop_arg( pc++ );
			ea2 = cpu_readop_arg( pc++ );
			dst += sprintf( dst, "R%02Xh,R%02Xh", ea2, ea );
			break;
		case AM_2:
			ea = cpu_readop_arg( pc++ );
			switch( ea & 0xC0 ) {
			case 0x00:
				dst += sprintf( dst, "rr%02Xh", sm8500_b2w[ea & 0x07] ); break;
			case 0x40:
				ea2 = cpu_readop_arg( pc++ ) << 8;
				ea2 |= cpu_readop_arg( pc++ );
				symbol = set_ea_info( 0, ea2, EA_UINT16, EA_VALUE );
				if ( ea & 0x38 ) {
					dst += sprintf( dst, "@%s(r%02Xh)", symbol, ( ea >> 3 ) & 0x07 );
				} else {
					dst += sprintf( dst, "@%s", symbol );
				}
				break;
			case 0x80:
				dst += sprintf( dst, "undef" ); break;
			case 0xC0:
				dst += sprintf( dst, "undef" ); break;
			}
			break;
		case AM_SS:
			ea = cpu_readop_arg( pc++ );
			ea2 = cpu_readop_arg( pc++ );
			dst += sprintf( dst, "RR%02Xh,RR%02Xh", ea2, ea );
			break;
		case AM_bR:
			ea = cpu_readop_arg( pc++ );
			ea2 = cpu_readop_arg( pc++ );
			switch( ea & 0xC0 ) {
			case 0x00:
				dst += sprintf( dst, "BF,R%02Xh,#%d", ea2, ea & 0x07 ); break;
			case 0x40:
				dst += sprintf( dst, "R%02Xh,#%d,BF", ea2, ea & 0x07 ); break;
			case 0x80:
				dst += sprintf( dst, "undef" ); break;
			case 0xC0:
				dst += sprintf( dst, "undef" ); break;
			}
			break;
		case AM_Rbr:
			ea = cpu_readop_arg( pc++ );
			offset = (INT8) cpu_readop_arg( pc++ );
			symbol = set_ea_info( 0, pc, offset, instr->access );
			dst += sprintf( dst, "R%02Xh,#%d,%s", ea, op & 0x07, symbol );
			break;
		case AM_Rb:
			ea = cpu_readop_arg( pc++ );
			dst += sprintf( dst, "R%02Xh,#%d", ea, op&0x07 );
			break;
		case AM_rR:
			ea = cpu_readop_arg( pc++ );
			dst += sprintf( dst, "r%02Xh,R%02Xh", op & 0x07, ea );
			break;
		case AM_Rr:
			ea = cpu_readop_arg( pc++ );
			dst += sprintf( dst, "R%02Xh,r%02Xh", ea, op & 0x07 );
			break;
		case AM_Rii:
			ea = cpu_readop_arg( pc++ );
			dst += sprintf( dst, "R%02Xh,", ea );
			ea = cpu_readop_arg( pc++ );
			symbol = set_ea_info( 0, ea, EA_UINT8, EA_VALUE );
			dst += sprintf( dst, "%s,", symbol );
			ea = cpu_readop_arg( pc++ );
			symbol = set_ea_info( 0, ea, EA_UINT8, EA_VALUE );
			dst += sprintf( dst, "%s", symbol );
			break;
		case AM_RiR:
			ea = cpu_readop_arg( pc++ );
			dst += sprintf( dst, "R%02Xh,", ea );
			ea = cpu_readop_arg( pc++ );
			symbol = set_ea_info( 0, ea, EA_UINT8, EA_VALUE );
			dst += sprintf( dst, "%s,", symbol );
			ea = cpu_readop_arg( pc++ );
			dst += sprintf( dst, "R%02Xh", ea );
			break;
		case AM_riB:
			ea = cpu_readop_arg( pc++ );
			ea2 = cpu_readop_arg( pc++ );
			switch( ea & 0xC0 ) {
			case 0x00:
				dst += sprintf( dst, "#%2x(r%02Xh),#%d", ea2, ea >> 3, ea & 0x07 );
				break;
			case 0x40:
				dst += sprintf( dst, "undef" ); break;
			case 0x80:
				dst += sprintf( dst, "undef" ); break;
			case 0xC0:
				dst += sprintf( dst, "undef" ); break;
			}
			break;
		case AM_CALS:
			ea = cpu_readop_arg( pc++ );
			symbol = set_ea_info( 0, 0x1000 | ( ( op & 0x0f ) << 8 ) | ea, EA_UINT16, EA_VALUE );
			dst += sprintf( dst, "%s", symbol );
			break;
		case AM_bid:
			ea = cpu_readop_arg( pc++ );
			ea2 = cpu_readop_arg( pc++ );
			if ( ea & 0x38 ) {
				symbol = set_ea_info( 0, ea2, EA_UINT8, EA_VALUE );
				dst += sprintf( dst, "%s(r%02Xh)", symbol, ( ea >> 3 ) & 0x07 );
			} else {
				symbol = set_ea_info( 0, 0xFF00 + ea2, EA_UINT16, EA_VALUE );
				dst += sprintf( dst, "%s", symbol );
			}
			dst += sprintf( dst, ",#%d,", ea & 0x07 );
			offset = (INT8) cpu_readop_arg( pc++ );
			symbol = set_ea_info( 0, pc, offset, instr->access );
			dst += sprintf( dst, "%s", symbol );
			break;
		case AM_1A:
			ea = cpu_readop_arg( pc++ );
			switch( ea & 0x07 ) {
			case 0x00: dst += sprintf( dst, "%-4s ", s_mnemonic[ zCLR ] ); break;
			case 0x01: dst += sprintf( dst, "%-4s ", s_mnemonic[ zNEG ] ); break;
			case 0x02: dst += sprintf( dst, "%-4s ", s_mnemonic[ zCOM ] ); break;
			case 0x03: dst += sprintf( dst, "%-4s ", s_mnemonic[ zRR ] ); break;
			case 0x04: dst += sprintf( dst, "%-4s ", s_mnemonic[ zRL ] ); break;
			case 0x05: dst += sprintf( dst, "%-4s ", s_mnemonic[ zRRC ] ); break;
			case 0x06: dst += sprintf( dst, "%-4s ", s_mnemonic[ zRLC ] ); break;
			case 0x07: dst += sprintf( dst, "%-4s ", s_mnemonic[ zSRL ] ); break;
			}
			dst += sprintf( dst, "@r%02Xh", ( ea >> 3 ) & 0x07 );
			break;
		case AM_1B:
			ea = cpu_readop_arg( pc++ );
			switch( ea & 0x07 ) {
			case 0x00: dst += sprintf( dst, "%-4s ", s_mnemonic[ zINC ] ); break;
			case 0x01: dst += sprintf( dst, "%-4s ", s_mnemonic[ zDEC ] ); break;
			case 0x02: dst += sprintf( dst, "%-4s ", s_mnemonic[ zSRA ] ); break;
			case 0x03: dst += sprintf( dst, "%-4s ", s_mnemonic[ zSLL ] ); break;
			case 0x04: dst += sprintf( dst, "%-4s ", s_mnemonic[ zDA ] ); break;
			case 0x05: dst += sprintf( dst, "%-4s ", s_mnemonic[ zSWAP ] ); break;
			case 0x06: dst += sprintf( dst, "%-4s ", s_mnemonic[ zPUSH ] ); break;
			case 0x07: dst += sprintf( dst, "%-4s ", s_mnemonic[ zPOP ] ); break;
			}
			dst += sprintf( dst, "@r%02Xh", ( ea >> 3 ) & 0x07 );
			break;
		case AM_4F:
			ea = cpu_readop_arg( pc++ );
			ea2 = cpu_readop_arg( pc++ );
			switch( ea & 0xc0 ) {
			case 0x00: dst += sprintf( dst, "%-4s ", s_mnemonic[ zBCMP ] ); break;
			case 0x40: dst += sprintf( dst, "%-4s ", s_mnemonic[ zBAND ] ); break;
			case 0x80: dst += sprintf( dst, "%-4s ", s_mnemonic[ zBOR ] ); break;
			case 0xC0: dst += sprintf( dst, "%-4s ", s_mnemonic[ zBXOR ] ); break;
			}
			if ( ! ( ea & 0x80 ) ) {
				dst += sprintf( dst, "BF," );
			}
			dst += sprintf( dst, "R%02Xh,", ea2 );
			symbol = set_ea_info( 0, ea & 0x07, EA_UINT8, EA_VALUE );
			dst += sprintf( dst, "%s", symbol );
			if ( ea & 0x80 ) {
				dst += sprintf( dst, ",BF" );
			}
			break;
		}
	}
	else
	{
		dst += sprintf( dst, "%s", s_mnemonic[ instr->mnemonic ] );
	}

	return pc - old_pc;
}

#endif

