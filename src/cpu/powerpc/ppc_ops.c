/* PowerPC common opcodes */

// it really seems like this should be elsewhere - like maybe the floating point checks can hang out someplace else
#include <math.h>

static void ppc_null(UINT32 op)
{
    // char    string[256];
    // char    mnem[16], oprs[48];

    // DisassemblePowerPC(op, ppc.pc, mnem, oprs, 1);

	osd_die("ERROR: %08X: unhandled opcode (I need a better error !)\n", activecpu_get_pc());
}

static void ppc_addx(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 rb = REG(RB);

	REG(RT) = ra + rb;

	if( OEBIT ) {
		SET_ADD_OV(REG(RT), ra, rb);
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_addcx(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 rb = REG(RB);

	REG(RT) = ra + rb;

	SET_ADD_CA(REG(RT), ra, rb);

	if( OEBIT ) {
		SET_ADD_OV(REG(RT), ra, rb);
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_addex(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 rb = REG(RB);
	UINT32 carry = (XER >> 29) & 0x1;
	UINT32 tmp;

	tmp = rb + carry;
	REG(RT) = ra + tmp;

	if( ADD_CA(tmp, rb, carry) || ADD_CA(REG(RT), ra, tmp) )
		XER |= XER_CA;
	else
		XER &= ~XER_CA;

	if( OEBIT ) {
		SET_ADD_OV(REG(RT), ra, rb);
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_addi(UINT32 op)
{
	UINT32 i = SIMM16;
	UINT32 a = RA;

	if( a ) 
		i += REG(a);

	REG(RT) = i;
}

static void ppc_addic(UINT32 op)
{
	UINT32 i = SIMM16;
	UINT32 ra = REG(RA);

	REG(RT) = ra + i;

	if( ADD_CA(REG(RT), ra, i) )
		XER |= XER_CA;
	else
		XER &= ~XER_CA;
}

static void ppc_addic_rc(UINT32 op)
{
	UINT32 i = SIMM16;
	UINT32 ra = REG(RA);

	REG(RT) = ra + i;

	if( ADD_CA(REG(RT), ra, i) )
		XER |= XER_CA;
	else
		XER &= ~XER_CA;

	SET_CR0(REG(RT));
}

static void ppc_addis(UINT32 op)
{
	UINT32 i = UIMM16 << 16;
	UINT32 a = RA;

	if( a )
		i += REG(a);

	REG(RT) = i;
}

static void ppc_addmex(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 carry = (XER >> 29) & 0x1;
	UINT32 tmp;

	tmp = ra + carry;
	REG(RT) = tmp + -1;

	if( ADD_CA(tmp, ra, carry) || ADD_CA(REG(RT), tmp, -1) )
		XER |= XER_CA;
	else
		XER &= ~XER_CA;

	if( OEBIT ) {
		SET_ADD_OV(REG(RT), ra, carry - 1);
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_addzex(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 carry = (XER >> 29) & 0x1;

	REG(RT) = ra + carry;

	if( ADD_CA(REG(RT), ra, carry) )
		XER |= XER_CA;
	else
		XER &= ~XER_CA;

	if( OEBIT ) {
		SET_ADD_OV(REG(RT), ra, carry);
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_andx(UINT32 op)
{
	REG(RA) = REG(RS) & REG(RB);

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_andcx(UINT32 op)
{
	REG(RA) = REG(RS) & ~REG(RB);

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_andi_rc(UINT32 op)
{
	UINT32 i = UIMM16;

	REG(RA) = REG(RS) & i;

	SET_CR0(REG(RA));
}

static void ppc_andis_rc(UINT32 op)
{
	UINT32 i = UIMM16 << 16;

	REG(RA) = REG(RS) & i;

	SET_CR0(REG(RA));
}

static void ppc_bx(UINT32 op)
{
	INT32 li = op & 0x3fffffc;
	if( li & 0x2000000 )
		li |= 0xfc000000;

	if( AABIT ) {
		ppc.npc = li;
	} else {
		ppc.npc = ppc.pc + li;
	}

	if( LKBIT ) {
		LR = ppc.pc + 4;
	}

	change_pc(ppc.npc);
}

static void ppc_bcx(UINT32 op)
{
	int condition = check_condition_code(BO, BI);

	if( condition ) {
		if( AABIT ) {
			ppc.npc = SIMM16 & ~0x3;
		} else {
			ppc.npc = ppc.pc + (SIMM16 & ~0x3);
		}

		change_pc(ppc.npc);
	}

	if( LKBIT ) {
		LR = ppc.pc + 4;
	}
}

static void ppc_bcctrx(UINT32 op)
{
	int condition = check_condition_code(BO, BI);

	if( condition ) {
		ppc.npc = CTR & ~0x3;
		change_pc(ppc.npc);
	}

	if( LKBIT ) {
		LR = ppc.pc + 4;
	}
}

static void ppc_bclrx(UINT32 op)
{
	int condition = check_condition_code(BO, BI);

	if( condition ) {
		ppc.npc = LR & ~0x3;
		change_pc(ppc.npc);
	}

	if( LKBIT ) {
		LR = ppc.pc + 4;
	}
}

static void ppc_cmp(UINT32 op)
{
	INT32 ra = REG(RA);
	INT32 rb = REG(RB);
	int d = CRFD;

	if( ra < rb )
		CR(d) = 0x8;
	else if( ra > rb )
		CR(d) = 0x4;
	else
		CR(d) = 0x2;

	if( XER & XER_SO )
		CR(d) |= 0x1;
}

static void ppc_cmpi(UINT32 op)
{
	INT32 ra = REG(RA);
	INT32 i = SIMM16;
	int d = CRFD;

	if( ra < i )
		CR(d) = 0x8;
	else if( ra > i )
		CR(d) = 0x4;
	else
		CR(d) = 0x2;

	if( XER & XER_SO )
		CR(d) |= 0x1;
}

static void ppc_cmpl(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 rb = REG(RB);
	int d = CRFD;

	if( ra < rb )
		CR(d) = 0x8;
	else if( ra > rb )
		CR(d) = 0x4;
	else
		CR(d) = 0x2;

	if( XER & XER_SO )
		CR(d) |= 0x1;
}

static void ppc_cmpli(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 i = UIMM16;
	int d = CRFD;

	if( ra < i )
		CR(d) = 0x8;
	else if( ra > i )
		CR(d) = 0x4;
	else
		CR(d) = 0x2;

	if( XER & XER_SO )
		CR(d) |= 0x1;
}

static void ppc_cntlzw(UINT32 op)
{
	int n = 0;
	int t = RT;
	UINT32 m = 0x80000000;

	while(n < 32)
	{
		if( REG(t) & m )
			break;
		m >>= 1;
		n++;
	}

	REG(RA) = n;

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_crand(UINT32 op)
{
	int bit = RT;
	int b = CRBIT(RA) & CRBIT(RB);
	if( b & 0x1 )
		CR(bit / 4) |= _BIT(3-(bit % 4));
	else
		CR(bit / 4) &= ~_BIT(3-(bit % 4));
}

static void ppc_crandc(UINT32 op)
{
	int bit = RT;
	int b = CRBIT(RA) & ~CRBIT(RB);
	if( b & 0x1 )
		CR(bit / 4) |= _BIT(3-(bit % 4));
	else
		CR(bit / 4) &= ~_BIT(3-(bit % 4));
}

static void ppc_creqv(UINT32 op)
{
	int bit = RT;
	int b = ~(CRBIT(RA) ^ CRBIT(RB));
	if( b & 0x1 )
		CR(bit / 4) |= _BIT(3-(bit % 4));
	else
		CR(bit / 4) &= ~_BIT(3-(bit % 4));
}

static void ppc_crnand(UINT32 op)
{
	int bit = RT;
	int b = ~(CRBIT(RA) & CRBIT(RB));
	if( b & 0x1 )
		CR(bit / 4) |= _BIT(3-(bit % 4));
	else
		CR(bit / 4) &= ~_BIT(3-(bit % 4));
}

static void ppc_crnor(UINT32 op)
{
	int bit = RT;
	int b = ~(CRBIT(RA) | CRBIT(RB));
	if( b & 0x1 )
		CR(bit / 4) |= _BIT(3-(bit % 4));
	else
		CR(bit / 4) &= ~_BIT(3-(bit % 4));
}

static void ppc_cror(UINT32 op)
{
	int bit = RT;
	int b = CRBIT(RA) | CRBIT(RB);
	if( b & 0x1 )
		CR(bit / 4) |= _BIT(3-(bit % 4));
	else
		CR(bit / 4) &= ~_BIT(3-(bit % 4));
}

static void ppc_crorc(UINT32 op)
{
	int bit = RT;
	int b = CRBIT(RA) | ~CRBIT(RB);
	if( b & 0x1 )
		CR(bit / 4) |= _BIT(3-(bit % 4));
	else
		CR(bit / 4) &= ~_BIT(3-(bit % 4));
}

static void ppc_crxor(UINT32 op)
{
	int bit = RT;
	int b = CRBIT(RA) ^ CRBIT(RB);
	if( b & 0x1 )
		CR(bit / 4) |= _BIT(3-(bit % 4));
	else
		CR(bit / 4) &= ~_BIT(3-(bit % 4));
}

static void ppc_dcbf(UINT32 op)
{

}

static void ppc_dcbi(UINT32 op)
{

}

static void ppc_dcbst(UINT32 op)
{

}

static void ppc_dcbt(UINT32 op)
{

}

static void ppc_dcbtst(UINT32 op)
{

}

static void ppc_dcbz(UINT32 op)
{

}

static void ppc_divwx(UINT32 op)
{
	if( REG(RB) == 0 && REG(RA) < 0x80000000 )
	{
		REG(RT) = 0;
		if( OEBIT ) {
			XER |= XER_SO | XER_OV;
		}
	}
	else if( REG(RB) == 0 || (REG(RB) == 0xffffffff && REG(RA) == 0x80000000) )
	{
		REG(RT) = 0xffffffff;
		if( OEBIT ) {
			XER |= XER_SO | XER_OV;
		}
	}
	else 
	{
		REG(RT) = (INT32)REG(RA) / (INT32)REG(RB);
		if( OEBIT ) {
			XER &= ~XER_OV;
		}
	}

	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_divwux(UINT32 op)
{
	if( REG(RB) == 0 )
	{
		REG(RT) = 0;
		if( OEBIT ) {
			XER |= XER_SO | XER_OV;
		}
	}
	else
	{
		REG(RT) = (UINT32)REG(RA) / (UINT32)REG(RB);
		if( OEBIT ) {
			XER &= ~XER_OV;
		}
	}

	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_eieio(UINT32 op)
{

}

static void ppc_eqvx(UINT32 op)
{
	REG(RA) = ~(REG(RS) ^ REG(RB));

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_extsbx(UINT32 op)
{
	REG(RA) = (INT32)(INT8)REG(RS);

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_extshx(UINT32 op)
{
	REG(RA) = (INT32)(INT16)REG(RS);

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_icbi(UINT32 op)
{

}

static void ppc_isync(UINT32 op)
{

}

static void ppc_lbz(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = SIMM16;
	else
		ea = REG(RA) + SIMM16;

	REG(RT) = (UINT32)READ8(ea);
}

static void ppc_lbzu(UINT32 op)
{
	UINT32 ea = REG(RA) + SIMM16;

	REG(RT) = (UINT32)READ8(ea);
	REG(RA) = ea;
}

static void ppc_lbzux(UINT32 op)
{
	UINT32 ea = REG(RA) + REG(RB);

	REG(RT) = (UINT32)READ8(ea);
	REG(RA) = ea;
}

static void ppc_lbzx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	REG(RT) = (UINT32)READ8(ea);
}

static void ppc_lha(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = SIMM16;
	else
		ea = REG(RA) + SIMM16;

	REG(RT) = (INT32)(INT16)READ16(ea);
}

static void ppc_lhau(UINT32 op)
{
	UINT32 ea = REG(RA) + SIMM16;

	REG(RT) = (INT32)(INT16)READ16(ea);
	REG(RA) = ea;
}

static void ppc_lhaux(UINT32 op)
{
	UINT32 ea = REG(RA) + REG(RB);

	REG(RT) = (INT32)(INT16)READ16(ea);
	REG(RA) = ea;
}

static void ppc_lhax(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	REG(RT) = (INT32)(INT16)READ16(ea);
}

static void ppc_lhbrx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	REG(RT) = (UINT32)BYTE_REVERSE16(READ16(ea));
}

static void ppc_lhz(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = SIMM16;
	else
		ea = REG(RA) + SIMM16;

	REG(RT) = (UINT32)READ16(ea);
}

static void ppc_lhzu(UINT32 op)
{
	UINT32 ea = REG(RA) + SIMM16;

	REG(RT) = (UINT32)READ16(ea);
	REG(RA) = ea;
}

static void ppc_lhzux(UINT32 op)
{
	UINT32 ea = REG(RA) + REG(RB);

	REG(RT) = (UINT32)READ16(ea);
	REG(RA) = ea;
}

static void ppc_lhzx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	REG(RT) = (UINT32)READ16(ea);
}

static void ppc_lmw(UINT32 op)
{
	int r = RT;
	UINT32 ea;

	if( RA == 0 )
		ea = SIMM16;
	else
		ea = REG(RA) + SIMM16;

	while( r <= 31 )
	{
		REG(r) = READ32(ea);
		ea += 4;
		r++;
	}
}

static void ppc_lswi(UINT32 op)
{
	osd_die("ppc: lswi unimplemented\n");
}

static void ppc_lswx(UINT32 op)
{
	osd_die("ppc: lswx unimplemented\n");
}

static void ppc_lwarx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	ppc.reserved_address = ea;
	ppc.reserved = 1;

	REG(RT) = READ32(ea);
}

static void ppc_lwbrx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	REG(RT) = BYTE_REVERSE32(READ32(ea));
}

static void ppc_lwz(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = SIMM16;
	else
		ea = REG(RA) + SIMM16;

	REG(RT) = READ32(ea);
}

static void ppc_lwzu(UINT32 op)
{
	UINT32 ea = REG(RA) + SIMM16;

	REG(RT) = READ32(ea);
	REG(RA) = ea;
}

static void ppc_lwzux(UINT32 op)
{
	UINT32 ea = REG(RA) + REG(RB);

	REG(RT) = READ32(ea);
	REG(RA) = ea;
}

static void ppc_lwzx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	REG(RT) = READ32(ea);
}

static void ppc_mcrf(UINT32 op)
{
	CR(RT >> 2) = CR(RA >> 2);
}

static void ppc_mcrxr(UINT32 op)
{
	osd_die("ppc: mcrxr unimplemented\n");
}

static void ppc_mfcr(UINT32 op)
{
	REG(RT) = ppc_get_cr();
}

static void ppc_mfmsr(UINT32 op)
{
	REG(RT) = ppc_get_msr();
}

static void ppc_mfspr(UINT32 op)
{
	REG(RT) = ppc_get_spr(SPR);
}

static void ppc_mtcrf(UINT32 op)
{
	int fxm = FXM;
	int t = RT;

	if( fxm & 0x80 )	CR(0) = (REG(t) >> 28) & 0xf;
	if( fxm & 0x40 )	CR(1) = (REG(t) >> 24) & 0xf;
	if( fxm & 0x20 )	CR(2) = (REG(t) >> 20) & 0xf;
	if( fxm & 0x10 )	CR(3) = (REG(t) >> 16) & 0xf;
	if( fxm & 0x08 )	CR(4) = (REG(t) >> 12) & 0xf;
	if( fxm & 0x04 )	CR(5) = (REG(t) >> 8) & 0xf;
	if( fxm & 0x02 )	CR(6) = (REG(t) >> 4) & 0xf;
	if( fxm & 0x01 )	CR(7) = (REG(t) >> 0) & 0xf;
}

static void ppc_mtmsr(UINT32 op)
{
	ppc_set_msr(REG(RS));
}

static void ppc_mtspr(UINT32 op)
{
	ppc_set_spr(SPR, REG(RS));
}

static void ppc_mulhwx(UINT32 op)
{
	INT64 ra = (INT64)(INT32)REG(RA);
	INT64 rb = (INT64)(INT32)REG(RB);

	REG(RT) = (UINT32)((ra * rb) >> 32);

	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_mulhwux(UINT32 op)
{
	UINT64 ra = (UINT64)REG(RA);
	UINT64 rb = (UINT64)REG(RB);

	REG(RT) = (UINT32)((ra * rb) >> 32);

	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_mulli(UINT32 op)
{
	INT32 ra = (INT32)REG(RA);
	INT32 i = SIMM16;

	REG(RT) = ra * i;
}

static void ppc_mullwx(UINT32 op)
{
	INT64 ra = (INT64)(INT32)REG(RA);
	INT64 rb = (INT64)(INT32)REG(RB);
	INT64 r;

	r = ra * rb;
	REG(RT) = (UINT32)r;

	if( OEBIT ) {
		XER &= ~XER_OV;

		if( r != (INT64)(INT32)r )
			XER |= XER_OV | XER_SO;
	}

	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_nandx(UINT32 op)
{
	REG(RA) = ~(REG(RS) & REG(RB));

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_negx(UINT32 op)
{
	REG(RT) = -REG(RA);

	if( OEBIT ) {
		if( REG(RT) == 0x80000000 )
			XER |= XER_OV | XER_SO;
		else
			XER &= ~XER_OV;
	}

	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_norx(UINT32 op)
{
	REG(RA) = ~(REG(RS) | REG(RB));

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_orx(UINT32 op)
{
	REG(RA) = REG(RS) | REG(RB);

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_orcx(UINT32 op)
{
	REG(RA) = REG(RS) | ~REG(RB);

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_ori(UINT32 op)
{
	REG(RA) = REG(RS) | UIMM16;
}

static void ppc_oris(UINT32 op)
{
	REG(RA) = REG(RS) | (UIMM16 << 16);
}

static void ppc_rfi(UINT32 op)
{
	ppc.npc = ppc_get_spr(SPR_SRR0);
	ppc_set_msr( ppc_get_spr(SPR_SRR1) );

	change_pc(ppc.npc);
}

static void ppc_rlwimix(UINT32 op)
{
	UINT32 r;
	UINT32 mask = GET_ROTATE_MASK(MB, ME);
	UINT32 rs = REG(RS);
	int sh = SH;

	r = (rs << sh) | (rs >> (32-sh));
	REG(RA) = (REG(RA) & ~mask) | (r & mask);

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_rlwinmx(UINT32 op)
{
	UINT32 r;
	UINT32 mask = GET_ROTATE_MASK(MB, ME);
	UINT32 rs = REG(RS);
	int sh = SH;

	r = (rs << sh) | (rs >> (32-sh));
	REG(RA) = r & mask;

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_rlwnmx(UINT32 op)
{
	UINT32 r;
	UINT32 mask = GET_ROTATE_MASK(MB, ME);
	UINT32 rs = REG(RS);
	int sh = REG(RB) & 0x1f;

	r = (rs << sh) | (rs >> (32-sh));
	REG(RA) = r & mask;

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_sc(UINT32 op)
{
	osd_die("ppc: sc unimplemented\n");
}

static void ppc_slwx(UINT32 op)
{
	int sh = REG(RB) & 0x3f;

	if( sh > 31 ) {
		REG(RA) = 0;
	}
	else {
		REG(RA) = REG(RS) << sh;
	}

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_srawx(UINT32 op)
{
	int sh = REG(RB) & 0x3f;

	XER &= ~XER_CA;

	if( sh > 31 ) {
		REG(RA) = (INT32)(REG(RS)) >> 31;
		if( REG(RA) )
			XER |= XER_CA;
	}
	else {
		REG(RA) = (INT32)(REG(RS)) >> sh;
		if( ((INT32)(REG(RS)) < 0) && (REG(RS) & BITMASK_0(sh)) )
			XER |= XER_CA;
	}

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_srawix(UINT32 op)
{
	int sh = SH;

	XER &= ~XER_CA;
	if( ((INT32)(REG(RS)) < 0) && (REG(RS) & BITMASK_0(sh)) )
		XER |= XER_CA;

	REG(RA) = (INT32)(REG(RS)) >> sh;

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_srwx(UINT32 op)
{
	int sh = REG(RB) & 0x3f;

	if( sh > 31 ) {
		REG(RA) = 0;
	}
	else {
		REG(RA) = REG(RS) >> sh;
	}

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_stb(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = SIMM16;
	else
		ea = REG(RA) + SIMM16;

	WRITE8(ea, (UINT8)REG(RS));
}

static void ppc_stbu(UINT32 op)
{
	UINT32 ea = REG(RA) + SIMM16;

	WRITE8(ea, (UINT8)REG(RS));
	REG(RA) = ea;
}

static void ppc_stbux(UINT32 op)
{
	UINT32 ea = REG(RA) + REG(RB);

	WRITE8(ea, (UINT8)REG(RS));
	REG(RA) = ea;
}

static void ppc_stbx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	WRITE8(ea, (UINT8)REG(RS));
}

static void ppc_sth(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = SIMM16;
	else
		ea = REG(RA) + SIMM16;

	WRITE16(ea, (UINT16)REG(RS));
}

static void ppc_sthbrx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	WRITE16(ea, (UINT16)BYTE_REVERSE16(REG(RS)));
}

static void ppc_sthu(UINT32 op)
{
	UINT32 ea = REG(RA) + SIMM16;

	WRITE16(ea, (UINT16)REG(RS));
	REG(RA) = ea;
}

static void ppc_sthux(UINT32 op)
{
	UINT32 ea = REG(RA) + REG(RB);

	WRITE16(ea, (UINT16)REG(RS));
	REG(RA) = ea;
}

static void ppc_sthx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	WRITE16(ea, (UINT16)REG(RS));
}

static void ppc_stmw(UINT32 op)
{
	UINT32 ea;
	int r = RS;

	if( RA == 0 )
		ea = SIMM16;
	else
		ea = REG(RA) + SIMM16;

	while( r <= 31 )
	{
		WRITE32(ea, REG(r));
		ea += 4;
		r++;
	}
}

static void ppc_stswi(UINT32 op)
{
	osd_die("ppc: stswi unimplemented\n");
}

static void ppc_stswx(UINT32 op)
{
	osd_die("ppc: stswx unimplemented\n");
}

static void ppc_stw(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = SIMM16;
	else
		ea = REG(RA) + SIMM16;

	WRITE32(ea, REG(RS));
}

static void ppc_stwbrx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	WRITE32(ea, BYTE_REVERSE32(REG(RS)));
}

static void ppc_stwcx_rc(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	if( ppc.reserved ) {
		WRITE32(ea, REG(RS));

		ppc.reserved = 0;
		ppc.reserved_address = 0;

		CR(0) = 0x2;
		if( XER & XER_SO )
			CR(0) |= 0x1;
	} else {
		CR(0) = 0;
		if( XER & XER_SO )
			CR(0) |= 0x1;
	}
}

static void ppc_stwu(UINT32 op)
{
	UINT32 ea = REG(RA) + SIMM16;

	WRITE32(ea, REG(RS));
	REG(RA) = ea;
}

static void ppc_stwux(UINT32 op)
{
	UINT32 ea = REG(RA) + REG(RB);

	WRITE32(ea, REG(RS));
	REG(RA) = ea;
}

static void ppc_stwx(UINT32 op)
{
	UINT32 ea;

	if( RA == 0 )
		ea = REG(RB);
	else
		ea = REG(RA) + REG(RB);

	WRITE32(ea, REG(RS));
}

static void ppc_subfx(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 rb = REG(RB);
	REG(RT) = rb - ra;

	if( OEBIT ) {
		SET_SUB_OV(REG(RT), rb, ra);
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_subfcx(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 rb = REG(RB);
	REG(RT) = rb - ra;

	SET_SUB_CA(REG(RT), rb, ra);

	if( OEBIT ) {
		SET_SUB_OV(REG(RT), rb, ra);
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_subfex(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 rb = REG(RB);
	UINT32 carry = (XER >> 29) & 0x1;
	UINT32 r;

	r = ~ra + carry;
	REG(RT) = rb + r;

	SET_ADD_CA(r, ~ra, carry);		/* step 1 carry */
	if( REG(RT) < r )				/* step 2 carry */
		XER |= XER_CA;

	if( OEBIT ) {
		SET_SUB_OV(REG(RT), rb, ra);
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_subfic(UINT32 op)
{
	UINT32 i = SIMM16;
	UINT32 ra = REG(RA);

	REG(RT) = i - ra;

	SET_SUB_CA(REG(RT), i, ra);
}

static void ppc_subfmex(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 carry = (XER >> 29) & 0x1;
	UINT32 r;

	r = ~ra + carry;
	REG(RT) = r - 1;

	SET_SUB_CA(r, ~ra, carry);		/* step 1 carry */
	if( REG(RT) < r )
		XER |= XER_CA;				/* step 2 carry */

	if( OEBIT ) {
		SET_SUB_OV(REG(RT), -1, ra);
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_subfzex(UINT32 op)
{
	UINT32 ra = REG(RA);
	UINT32 carry = (XER >> 29) & 0x1;
	
	REG(RT) = ~ra + carry;

	SET_ADD_CA(REG(RT), ~ra, carry);

	if( OEBIT ) {
		SET_SUB_OV(REG(RT), 0, REG(RA));
	}
	if( RCBIT ) {
		SET_CR0(REG(RT));
	}
}

static void ppc_sync(UINT32 op)
{

}

static void ppc_tw(UINT32 op)
{
	osd_die("ppc: tw unimplemented\n");
}

static void ppc_twi(UINT32 op)
{
	osd_die("ppc: twi unimplemented\n");
}

static void ppc_xorx(UINT32 op)
{
	REG(RA) = REG(RS) ^ REG(RB);

	if( RCBIT ) {
		SET_CR0(REG(RA));
	}
}

static void ppc_xori(UINT32 op)
{
	REG(RA) = REG(RS) ^ UIMM16;
}

static void ppc_xoris(UINT32 op)
{
	REG(RA) = REG(RS) ^ (UIMM16 << 16);
}



static void ppc_invalid(UINT32 op)
{
	osd_die("ppc: Invalid opcode %08X PC : %X\n", op, ppc.pc);
}


// Everything below is new from AJG

////////////////////////////
// !here are the 6xx ops! //
////////////////////////////

#define DOUBLE_SIGN		(0x8000000000000000)
#define DOUBLE_EXP		(0x7FF0000000000000)
#define DOUBLE_FRAC		(0x000FFFFFFFFFFFFF)
#define DOUBLE_ZERO		(0)

/*
  Floating point operations.
*/

INLINE int is_nan_FLOAT64(FLOAT64 x){

	return(
		((*(UINT64 *)&(x) & DOUBLE_EXP) == DOUBLE_EXP) &&
		((*(UINT64 *)&(x) & DOUBLE_FRAC) != DOUBLE_ZERO)
	);
}

INLINE int is_qnan_FLOAT64(FLOAT64 x){

	return(
		((*(UINT64 *)&(x) & DOUBLE_EXP) == DOUBLE_EXP) &&
		((*(UINT64 *)&(x) & 0x0007FFFFFFFFFFF) == 0x000000000000000) &&
        ((*(UINT64 *)&(x) & 0x000800000000000) == 0x000800000000000)
    );
}

INLINE int is_snan_FLOAT64(FLOAT64 x){

	return(
		((*(UINT64 *)&(x) & DOUBLE_EXP) == DOUBLE_EXP) &&
        ((*(UINT64 *)&(x) & DOUBLE_FRAC) != DOUBLE_ZERO) &&
        ((*(UINT64 *)&(x) & 0x0008000000000000) == DOUBLE_ZERO)
	);
}

INLINE int is_infinity_FLOAT64(FLOAT64 x)
{
    return (
        ((*(UINT64 *)&(x) & DOUBLE_EXP) == DOUBLE_EXP) &&
        ((*(UINT64 *)&(x) & DOUBLE_FRAC) == DOUBLE_ZERO)
    );
}

INLINE int is_normalized_FLOAT64(FLOAT64 x)
{
    UINT64 exp;

    exp = ((*(UINT64 *) &(x)) & DOUBLE_EXP) >> 52;

    return (exp >= 1) && (exp <= 2046);
}

INLINE int is_denormalized_FLOAT64(FLOAT64 x)
{
    return (
        ((*(UINT64 *)&(x) & DOUBLE_EXP) == 0) &&
        ((*(UINT64 *)&(x) & DOUBLE_FRAC) != DOUBLE_ZERO)
    );
}

INLINE int sign_FLOAT64(FLOAT64 x)
{
    return ((*(UINT64 *)&(x) & DOUBLE_SIGN) != 0);
}

INLINE INT32 round_FLOAT64_to_INT32(FLOAT64 v){

	if(v >= 0)
		return((INT32)(v + 0.5));
	else
		return(-(INT32)(-v + 0.5));
}

INLINE INT32 trunc_FLOAT64_to_INT32(FLOAT64 v){

	if(v >= 0)
		return((INT32)v);
	else
		return(-((INT32)(-v)));
}

INLINE INT32 ceil_FLOAT64_to_INT32(FLOAT64 v){

	// !!! ??? !!!
	FLOAT64 bob = ceil(v) ;
	return (INT32)bob;
}

INLINE INT32 floor_FLOAT64_to_INT32(FLOAT64 v){

	FLOAT64 bob = floor(v) ;
	return (INT32)bob ;
}

INLINE void set_fprf(FLOAT64 f)
{
    UINT32 fprf;

    // see page 3-30, 3-31

    if (is_qnan_FLOAT64(f))
        fprf = 0x11;
    else if (is_infinity_FLOAT64(f))
    {
        if (sign_FLOAT64(f))    // -INF
            fprf = 0x09;
        else                // +INF
            fprf = 0x05;
    }
    else if (is_normalized_FLOAT64(f))
    {
        if (sign_FLOAT64(f))    // -Normalized
            fprf = 0x08;
        else                // +Normalized
            fprf = 0x04;
    }
    else if (is_denormalized_FLOAT64(f))
    {
        if (sign_FLOAT64(f))    // -Denormalized
            fprf = 0x18;
        else                // +Denormalized
            fprf = 0x14;
    }
    else    // Zero
    {
        if (sign_FLOAT64(f))    // -Zero
            fprf = 0x12;
        else                // +Zero
            fprf = 0x02;
    }

    ppc.fpscr &= ~0x0001F000;
    ppc.fpscr |= (fprf << 12);
}



#define SET_VXSNAN(a, b)    if (is_snan_FLOAT64(a) || is_snan_FLOAT64(b)) ppc.fpscr |= 0x80000000
#define SET_VXSNAN_1(c)     if (is_snan_FLOAT64(c)) ppc.fpscr |= 0x80000000




static void ppc_lfs(UINT32 op)
{
	UINT32 ea = SIMM16;
	UINT32 a = RA;
	UINT32 t = RT;
	UINT32 i;

	if(a)
		ea += REG(a);

	i = READ32(ea);
	FPR(t).fd = (FLOAT64)(*((FLOAT32 *)&i));
}

static void ppc_lfsu(UINT32 op)
{
	UINT32 itemp;
	FLOAT32 ftemp;

	UINT32 ea = SIMM16;
	UINT32 a = RA;
	UINT32 t = RT;

	ea += REG(a);

	itemp = READ32(ea);
	ftemp = *(FLOAT32 *)&itemp;

	FPR(t).fd = (FLOAT64)((FLOAT32)ftemp);

	REG(a) = ea;
}

static void ppc_lfd(UINT32 op)
{
	UINT32 ea = SIMM16;
	UINT32 a = RA;
	UINT32 t = RT;

	if(a)
		ea += REG(a);

	FPR(t).id = READ64(ea);
}

static void ppc_lfdu(UINT32 op)
{
	UINT32 ea = SIMM16;
	UINT32 a = RA;
	UINT32 d = RD;

	ea += REG(a);

	FPR(d).id = READ64(ea);

	REG(a) = ea;
}

static void ppc_stfs(UINT32 op)
{
	FLOAT32 ftemp;

	UINT32 ea = SIMM16;
	UINT32 a = RA;
	UINT32 t = RT;

	if(a)
		ea += REG(a);

	ftemp = (FLOAT32)((FLOAT64)FPR(t).fd);

	WRITE32(ea, *(UINT32 *)&ftemp);
}

static void ppc_stfsu(UINT32 op)
{
	FLOAT32 ftemp;

	UINT32 ea = SIMM16;
	UINT32 a = RA;
	UINT32 t = RT;

	ea += REG(a);

	ftemp = (FLOAT32)((FLOAT64)FPR(t).fd);

	WRITE32(ea, *(UINT32 *)&ftemp);

	REG(a) = ea;
}

static void ppc_stfd(UINT32 op)
{
	UINT32 ea = SIMM16;
	UINT32 a = RA;
	UINT32 t = RT;

	if(a)
		ea += REG(a);

	WRITE64(ea, FPR(t).id);
}

static void ppc_stfdu(UINT32 op)
{
	UINT32 ea = SIMM16;
	UINT32 a = RA;
	UINT32 t = RT;

	ea += REG(a);

	WRITE64(ea, FPR(t).id);

	REG(a) = ea;
}

static void ppc_59(UINT32 op)
{
	optable59[_XO](op);
}

static void ppc_63(UINT32 op)
{
	optable63[_XO](op);
}

static void ppc_lfdux(UINT32 op)
{
	UINT32 ea = REG(RB);
	UINT32 a = RA;
	UINT32 d = RD;

	ea += REG(a);

	FPR(d).id = READ64(ea);

	REG(a) = ea;
}

static void ppc_lfdx(UINT32 op)
{
	UINT32 ea = REG(RB);
	UINT32 a = RA;
	UINT32 d = RD;

	if(a)
		ea += REG(a);

	FPR(d).id = READ64(ea);
}

static void ppc_lfsux(UINT32 op)
{
	UINT32 itemp;
	FLOAT32 ftemp;

	UINT32 ea = REG(RB);
	UINT32 a = RA;
	UINT32 t = RT;

	ea += REG(a);

	itemp = READ32(ea);
	ftemp = *(FLOAT32 *)&itemp;

	FPR(t).fd = (FLOAT64)((FLOAT32)ftemp);

	REG(a) = ea;
}

static void ppc_lfsx(UINT32 op)
{
	UINT32 itemp;
	FLOAT32 ftemp;

	UINT32 ea = REG(RB);
	UINT32 a = RA;
	UINT32 t = RT;

	if(a)
		ea += REG(a);

	itemp = READ32(ea);
	ftemp = *(FLOAT32 *)&itemp;

	FPR(t).fd = (FLOAT64)((FLOAT32)ftemp);
}

static void ppc_mfsr(UINT32 op)
{
	UINT32 sr = (op >> 16) & 15;
	UINT32 t = RT;

	CHECK_SUPERVISOR();

	REG(t) = ppc.sr[sr];
}

static void ppc_mfsrin(UINT32 op)
{
	UINT32 b = RB;
	UINT32 t = RT;

	CHECK_SUPERVISOR();

	REG(t) = ppc.sr[REG(b) >> 28];
}

static void ppc_mftb(UINT32 op)
{
	UINT32 t = RT;
	UINT32 x = _SPRF;

	switch(x){

	case 268: REG(t) = (UINT32)(ppc.tb); break;
	case 269: REG(t) = (UINT32)(ppc.tb >> 32); break;

	default:
		logerror("ERROR: invalid mftb\n");
		exit(1);
	}
}

static void ppc_mtsr(UINT32 op)
{
	UINT32 sr = (op >> 16) & 15;
	UINT32 t = RT;

	CHECK_SUPERVISOR();

	ppc.sr[sr] = REG(t);
}

static void ppc_mtsrin(UINT32 op)
{
	UINT32 b = RB;
	UINT32 t = RT;

	CHECK_SUPERVISOR();

	ppc.sr[REG(b) >> 28] = REG(t);
}

static void ppc_dcba(UINT32 op)
{
	ppc_null(op);
}

static void ppc_stfdux(UINT32 op)
{
	UINT32 ea = REG(RB);
	UINT32 a = RA;
	UINT32 t = RT;

	ea += REG(a);

	WRITE64(ea, FPR(t).id);

	REG(a) = ea;
}

static void ppc_stfdx(UINT32 op)
{
	UINT32 ea = REG(RB);
	UINT32 a = RA;
	UINT32 t = RT;

	if(a)
		ea += REG(a);

	WRITE64(ea, FPR(t).id);
}

static void ppc_stfiwx(UINT32 op)
{
	UINT32 ea = REG(RB);
	UINT32 a = RA;
	UINT32 t = RT;

	if(a)
		ea += REG(a);

	WRITE32(ea, FPR(t).iw);
}

static void ppc_stfsux(UINT32 op)
{
	FLOAT32 ftemp;

	UINT32 ea = REG(RB);
	UINT32 a = RA;
	UINT32 t = RT;

	ea += REG(a);

	ftemp = (FLOAT32)((FLOAT64)FPR(t).fd);

	WRITE32(ea, *(UINT32 *)&ftemp);

	REG(a) = ea;
}

static void ppc_stfsx(UINT32 op)
{
	FLOAT32 ftemp;

	UINT32 ea = REG(RB);
	UINT32 a = RA;
	UINT32 t = RT;

	if(a)
		ea += REG(a);

	ftemp = (FLOAT32)((FLOAT64)FPR(t).fd);

	WRITE32(ea, *(UINT32 *)&ftemp);
}

static void ppc_tlbia(UINT32 op)
{
	ppc_null(op);
}

static void ppc_tlbie(UINT32 op)
{

}

static void ppc_tlbsync(UINT32 op)
{

}

static void ppc_eciwx(UINT32 op)
{
	ppc_null(op);
}

static void ppc_ecowx(UINT32 op)
{
	ppc_null(op);
}

static void ppc_fabsx(UINT32 op)
{
	UINT32 b = RB;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

	FPR(t).id = FPR(b).id & ~DOUBLE_SIGN;

	SET_FCR1();
}

static void ppc_faddx(UINT32 op)
{
	UINT32 b = RB;
	UINT32 a = RA;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN(FPR(a).fd, FPR(b).fd);

	FPR(t).fd = (FLOAT64)((FLOAT64)FPR(a).fd + (FLOAT64)FPR(b).fd);

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_fcmpo(UINT32 op)
{
	CHECK_FPU_AVAILABLE();

    ppc_null(op);
}

static void ppc_fcmpu(UINT32 op)
{
	UINT32 b = RB;
	UINT32 a = RA;
	UINT32 t = (RT >> 2);
	UINT32 c;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN(FPR(a).fd, FPR(b).fd);

    if(is_nan_FLOAT64(FPR(a).fd) ||
	   is_nan_FLOAT64(FPR(b).fd)){

		c = 1; /* OX */

		if(is_snan_FLOAT64(FPR(a).fd) ||
		   is_snan_FLOAT64(FPR(b).fd))
			ppc.fpscr |= 0x01000000; /* VXSNAN */

	}else if(FPR(a).fd < FPR(b).fd){

		c = 8; /* FX */

	}else if(FPR(a).fd > FPR(b).fd){

		c = 4; /* FEX */

	}else{

		c = 2; /* VX */
	}

	CR(t) = c;

	ppc.fpscr &= ~0x0001F000;
	ppc.fpscr |= (c << 12);
}

static void ppc_fctiwx(UINT32 op)
{
	UINT32 b = RB;
	UINT32 t = RT;

	// TODO: fix FPSCR flags FX,VXSNAN,VXCVI

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN_1(FPR(b).fd);

    if(FPR(b).fd > (FLOAT64)((INT32)0x7FFFFFFF)){

		FPR(t).iw = 0x7FFFFFFF;

		// FPSCR[FR] = 0
		// FPSCR[FI] = 1
		// FPSCR[XX] = 1

	}else
	if(FPR(b).fd < (FLOAT64)((INT32)0x80000000)){

		FPR(t).iw = 0x80000000;

		// FPSCR[FR] = 1
		// FPSCR[FI] = 1
		// FPSCR[XX] = 1

	}else{

		switch(ppc.fpscr & 3){
		case 0: FPR(t).id = (INT64)(INT32)round_FLOAT64_to_INT32(FPR(b).fd); break;
		case 1: FPR(t).id = (INT64)(INT32)trunc_FLOAT64_to_INT32(FPR(b).fd); break;
		case 2: FPR(t).id = (INT64)(INT32)ceil_FLOAT64_to_INT32(FPR(b).fd); break;
		case 3: FPR(t).id = (INT64)(INT32)floor_FLOAT64_to_INT32(FPR(b).fd); break;
		}

		// FPSCR[FR] = t.iw > t.fd
		// FPSCR[FI] = t.iw == t.fd
		// FPSCR[XX] = ?
	}

	// FPSCR[FPRF] = undefined (leave it as is)

	SET_FCR1();
}

static void ppc_fctiwzx(UINT32 op)
{
	UINT32 b = RB;
	UINT32 t = RT;

	// TODO: fix FPSCR flags FX,VXSNAN,VXCVI

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN_1(FPR(b).fd);

    if(FPR(b).fd > (FLOAT64)((INT32)0x7FFFFFFF)){

		FPR(t).iw = 0x7FFFFFFF;

		// FPSCR[FR] = 0
		// FPSCR[FI] = 1
		// FPSCR[XX] = 1

	}else
	if(FPR(b).fd < (FLOAT64)((INT32)0x80000000)){

		FPR(t).iw = 0x80000000;

		// FPSCR[FR] = 1
		// FPSCR[FI] = 1
		// FPSCR[XX] = 1

	}else{

		FPR(t).iw = trunc_FLOAT64_to_INT32(FPR(b).fd);

		// FPSCR[FR] = t.iw > t.fd
		// FPSCR[FI] = t.iw == t.fd
		// FPSCR[XX] = ?
	}

	// FPSCR[FPRF] = undefined (leave it as is)

	SET_FCR1();
}

static void ppc_fdivx(UINT32 op)
{
	UINT32 b = RB;
	UINT32 a = RA;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN(FPR(a).fd, FPR(b).fd);

    FPR(t).fd = (FLOAT64)((FLOAT64)FPR(a).fd / (FLOAT64)FPR(b).fd);

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_fmrx(UINT32 op)
{
	UINT32 b = RB;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

	FPR(t).fd = FPR(b).fd;

	SET_FCR1();
}

static void ppc_fnabsx(UINT32 op)
{
	UINT32 b = RB;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

	FPR(t).id = FPR(b).id | DOUBLE_SIGN;

	SET_FCR1();
}

static void ppc_fnegx(UINT32 op)
{
	UINT32 b = RB;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

	FPR(t).id = FPR(b).id ^ DOUBLE_SIGN;

	SET_FCR1();
}

static void ppc_frspx(UINT32 op)
{
	UINT32 b = RB;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN_1(FPR(b).fd);

	FPR(t).fd = (FLOAT32)FPR(b).fd;

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_frsqrtex(UINT32 op)
{
	UINT32 b = RB;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN_1(FPR(b).fd);

	FPR(t).fd = 1.0 / sqrt(FPR(b).fd); /* ??? */

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_fsqrtx(UINT32 op)
{
	UINT32 b = RB;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN_1(FPR(b).fd);

	FPR(t).fd = (FLOAT64)(sqrt(FPR(b).fd));

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_fsubx(UINT32 op)
{
	UINT32 b = RB;
	UINT32 a = RA;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN(FPR(a).fd, FPR(b).fd);

	FPR(t).fd = (FLOAT64)((FLOAT64)FPR(a).fd - (FLOAT64)FPR(b).fd);

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_mffsx(UINT32 op)
{
	UINT32 t = RT;

	FPR(t).iw = ppc.fpscr;

	SET_FCR1();
}

static void ppc_mtfsb0x(UINT32 op)
{
    UINT32 crbD;

    crbD = (op >> 21) & 0x1F;

    if (crbD != 1 && crbD != 2) // these bits cannot be explicitly cleared
        ppc.fpscr &= ~(1 << (31 - crbD));

    SET_FCR1();
}

static void ppc_mtfsb1x(UINT32 op)
{
    UINT32 crbD;

    crbD = (op >> 21) & 0x1F;

    if (crbD != 1 && crbD != 2) // these bits cannot be explicitly cleared
        ppc.fpscr |= (1 << (31 - crbD));

    SET_FCR1();
}

static void ppc_mtfsfx(UINT32 op)
{
	UINT32 b = RB;
	UINT32 f = _FM;

	f = ppc_field_xlat[_FM];

	ppc.fpscr &= (~f) | ~(_FEX | _VX);
	ppc.fpscr |= (FPR(b).iw) & ~(_FEX | _VX);

	// FEX, VX

	SET_FCR1();
}

static void ppc_mtfsfix(UINT32 op)
{
    UINT32 crfD = _CRFD;
    UINT32 imm = (op >> 12) & 0xF;

    /*
     * According to the manual:
     *
     * If bits 0 and 3 of FPSCR are to be modified, they take the immediate
     * value specified. Bits 1 and 2 (FEX and VX) are set according to the
     * "usual rule" and not from IMM[1-2].
     *
     * The "usual rule" is not emulated, so these bits simply aren't modified
     * at all here.
     */

    crfD = (7 - crfD) * 4;  // calculate LSB position of field

    if (crfD == 28)         // field containing FEX and VX is special...
    {                       // bits 1 and 2 of FPSCR must not be altered
        ppc.fpscr &= 0x9FFFFFFF;
        ppc.fpscr |= (imm & 0x9FFFFFFF);
    }

    ppc.fpscr &= ~(0xF << crfD);    // clear field
    ppc.fpscr |= (imm << crfD);     // insert new data

	SET_FCR1();
}

static void ppc_mcrfs(UINT32 op)
{
    UINT32 crfS, f;

    crfS = _CRFA;

    f = ppc.fpscr >> ((7 - crfS) * 4);  // get crfS field from FPSCR
    f &= 0xF;

//    LOG("model3.log", "MCRFS field %d\n", crfS);
//    message(0, "MCRFS field %d", crfS);

    switch (crfS)   // determine which exception bits to clear in FPSCR
    {
    case 0: // FX, OX
        ppc.fpscr &= ~0x90000000;
        break;
    case 1: // UX, ZX, XX, VXSNAN
        ppc.fpscr &= ~0x0F000000;
        break;
    case 2: // VXISI, VXIDI, VXZDZ, VXIMZ
        ppc.fpscr &= ~0x00F00000;
        break;
    case 3: // VXVC
        ppc.fpscr &= ~0x00080000;
        break;
    case 5: // VXSOFT, VXSQRT, VXCVI
        ppc.fpscr &= ~0x00000E00;
        break;
    default:
        break;
    }

    CR(_CRFD) = f;
}

static void ppc_faddsx(UINT32 op)
{
	UINT32 b = RB;
	UINT32 a = RA;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN(FPR(a).fd, FPR(b).fd);

    FPR(t).fd = (FLOAT32)((FLOAT64)FPR(a).fd + (FLOAT64)FPR(b).fd);

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_fdivsx(UINT32 op)
{
	UINT32 b = RB;
	UINT32 a = RA;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN(FPR(a).fd, FPR(b).fd);

    FPR(t).fd = (FLOAT32)((FLOAT64)FPR(a).fd / (FLOAT64)FPR(b).fd);

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_fresx(UINT32 op)
{
	UINT32 b = RB;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN_1(FPR(b).fd);

	FPR(t).fd = 1.0 / FPR(b).fd; /* ??? */

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_fsqrtsx(UINT32 op)
{
	UINT32 b = RB;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN_1(FPR(b).fd);

	FPR(t).fd = (FLOAT32)(sqrt(FPR(b).fd));

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_fsubsx(UINT32 op)
{
	UINT32 b = RB;
	UINT32 a = RA;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN(FPR(a).fd, FPR(b).fd);

	FPR(t).fd = (FLOAT32)((FLOAT64)FPR(a).fd - (FLOAT64)FPR(b).fd);

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_fmaddx(UINT32 op)
{
	UINT32 c = RC;
	UINT32 b = RB;
	UINT32 a = RA;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN(FPR(a).fd, FPR(b).fd);
    SET_VXSNAN_1(FPR(c).fd);

	FPR(t).fd = (FLOAT64)((FPR(a).fd * FPR(c).fd) + FPR(b).fd);

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_fmsubx(UINT32 op)
{
	UINT32 c = RC;
	UINT32 b = RB;
	UINT32 a = RA;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN(FPR(a).fd, FPR(b).fd);
    SET_VXSNAN_1(FPR(c).fd);

    FPR(t).fd = (FLOAT64)((FPR(a).fd * FPR(c).fd) - FPR(b).fd);

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_fmulx(UINT32 op)
{
	UINT32 c = RC;
	UINT32 a = RA;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN(FPR(a).fd, FPR(c).fd);

    FPR(t).fd = (FLOAT64)((FLOAT64)FPR(a).fd * (FLOAT64)FPR(c).fd);

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_fnmaddx(UINT32 op)
{
	UINT32 c = RC;
	UINT32 b = RB;
	UINT32 a = RA;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN(FPR(a).fd, FPR(b).fd);
    SET_VXSNAN_1(FPR(c).fd);

    FPR(t).fd = (FLOAT64)(-((FPR(a).fd * FPR(c).fd) + FPR(b).fd));

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_fnmsubx(UINT32 op)
{
	UINT32 c = RC;
	UINT32 b = RB;
	UINT32 a = RA;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN(FPR(a).fd, FPR(b).fd);
    SET_VXSNAN_1(FPR(c).fd);

    FPR(t).fd = (FLOAT64)(-((FPR(a).fd * FPR(c).fd) - FPR(b).fd));

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_fselx(UINT32 op)
{
	UINT32 c = RC;
	UINT32 b = RB;
	UINT32 a = RA;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

	FPR(t).fd = (FPR(a).fd >= 0.0) ? FPR(c).fd : FPR(b).fd;

	SET_FCR1();
}

static void ppc_fmaddsx(UINT32 op)
{
	UINT32 c = RC;
	UINT32 b = RB;
	UINT32 a = RA;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN(FPR(a).fd, FPR(b).fd);
    SET_VXSNAN_1(FPR(c).fd);

    FPR(t).fd = (FLOAT32)((FPR(a).fd * FPR(c).fd) + FPR(b).fd);

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_fmsubsx(UINT32 op)
{
	UINT32 c = RC;
	UINT32 b = RB;
	UINT32 a = RA;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN(FPR(a).fd, FPR(b).fd);
    SET_VXSNAN_1(FPR(c).fd);

    FPR(t).fd = (FLOAT32)((FPR(a).fd * FPR(c).fd) - FPR(b).fd);

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_fmulsx(UINT32 op)
{
	UINT32 c = RC;
	UINT32 a = RA;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN(FPR(a).fd, FPR(c).fd);

	FPR(t).fd = (FLOAT32)((FLOAT64)FPR(a).fd * (FLOAT64)FPR(c).fd);

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_fnmaddsx(UINT32 op)
{
	UINT32 c = RC;
	UINT32 b = RB;
	UINT32 a = RA;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN(FPR(a).fd, FPR(b).fd);
    SET_VXSNAN_1(FPR(c).fd);

    FPR(t).fd = (FLOAT32)(-((FPR(a).fd * FPR(c).fd) + FPR(b).fd));

    set_fprf(FPR(t).fd);
	SET_FCR1();
}

static void ppc_fnmsubsx(UINT32 op)
{
	UINT32 c = RC;
	UINT32 b = RB;
	UINT32 a = RA;
	UINT32 t = RT;

	CHECK_FPU_AVAILABLE();

    SET_VXSNAN(FPR(a).fd, FPR(b).fd);
    SET_VXSNAN_1(FPR(c).fd);

    FPR(t).fd = (FLOAT32)(-((FPR(a).fd * FPR(c).fd) - FPR(b).fd));

    set_fprf(FPR(t).fd);
	SET_FCR1();
}
