void ppc603_exception(int exception)
{
	switch( exception )
	{
		case EXCEPTION_IRQ:		/* External Interrupt */
			if( ppc_get_msr() & MSR_EE ) {
				UINT32 msr = ppc_get_msr();

				SRR0 = ppc.npc;
				SRR1 = msr & 0xff73;

				msr &= ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE | MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI);
				if( msr & MSR_ILE )
					msr |= MSR_LE;
				else
					msr &= ~MSR_LE;
				ppc_set_msr(msr);

				if( msr & MSR_IP )
					ppc.npc = 0xfff00000 | 0x0500;
				else
					ppc.npc = 0x00000000 | 0x0500;
					
				ppc.interrupt_pending &= ~0x1;
				change_pc(ppc.npc);
			}
			break;

		case EXCEPTION_DECREMENTER:		/* Decrementer overflow exception */
			if( ppc_get_msr() & MSR_EE ) {
				UINT32 msr = ppc_get_msr();

				SRR0 = ppc.npc;
				SRR1 = msr & 0xff73;

				msr &= ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE | MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI);
				if( msr & MSR_ILE )
					msr |= MSR_LE;
				else
					msr &= ~MSR_LE;
				ppc_set_msr(msr);

				if( msr & MSR_IP )
					ppc.npc = 0xfff00000 | 0x0900;
				else
					ppc.npc = 0x00000000 | 0x0900;
					
				ppc.interrupt_pending &= ~0x2;
				change_pc(ppc.npc);
			}
			break;

		case EXCEPTION_TRAP:			/* Program exception / Trap */
			{
				UINT32 msr = ppc_get_msr();

				SRR0 = ppc.pc;
				SRR1 = (msr & 0xff73) | 0x20000;	/* 0x20000 = TRAP bit */

				msr &= ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE | MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI);
				if( msr & MSR_ILE )
					msr |= MSR_LE;
				else
					msr &= ~MSR_LE;
				ppc_set_msr(msr);

				if( msr & MSR_IP )
					ppc.npc = 0xfff00000 | 0x0700;
				else
					ppc.npc = 0x00000000 | 0x0700;
				change_pc(ppc.npc);
			}
			break;

		case EXCEPTION_SYSTEM_CALL:		/* System call */
			{
				UINT32 msr = ppc_get_msr();

				SRR0 = ppc.npc;
				SRR1 = (msr & 0xff73);

				msr &= ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE | MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI);
				if( msr & MSR_ILE )
					msr |= MSR_LE;
				else
					msr &= ~MSR_LE;
				ppc_set_msr(msr);

				if( msr & MSR_IP )
					ppc.npc = 0xfff00000 | 0x0c00;
				else
					ppc.npc = 0x00000000 | 0x0c00;
				change_pc(ppc.npc);
			}
			break;

		case EXCEPTION_SMI:
			if( ppc_get_msr() & MSR_EE ) {
				UINT32 msr = ppc_get_msr();

				SRR0 = ppc.npc;
				SRR1 = msr & 0xff73;

				msr &= ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE | MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI);
				if( msr & MSR_ILE )
					msr |= MSR_LE;
				else
					msr &= ~MSR_LE;
				ppc_set_msr(msr);

				if( msr & MSR_IP )
					ppc.npc = 0xfff00000 | 0x1400;
				else
					ppc.npc = 0x00000000 | 0x1400;
					
				ppc.interrupt_pending &= ~0x4;
				change_pc(ppc.npc);
			}
			break;

		default:
			osd_die("ppc: Unhandled exception %d\n", exception);
			break;
	}
}

static void ppc603_set_irq_line(int irqline, int state)
{
	if( state ) {
		ppc.interrupt_pending |= 0x1;
		if (ppc.irq_callback)
		{
			ppc.irq_callback(irqline);
		}
	}
}

static void ppc603_set_smi_line(int state)
{
	if( state ) {
		ppc.interrupt_pending |= 0x4;
	}
}

INLINE void ppc603_check_interrupts(void)
{
	if (MSR & MSR_EE)
	{
		if (ppc.interrupt_pending != 0)
		{
			if (ppc.interrupt_pending & 0x1)
			{
				ppc603_exception(EXCEPTION_IRQ);
			}
			else if (ppc.interrupt_pending & 0x2)
			{
				ppc603_exception(EXCEPTION_DECREMENTER);
			}
			else if (ppc.interrupt_pending & 0x4)
			{
				ppc603_exception(EXCEPTION_SMI);
			}
		}
	}
}

static void ppc603_reset(void *param)
{
	ppc_config *config = param;
	ppc.pc = ppc.npc = 0xfff00100;
	ppc.pvr = config->pvr;

	ppc_set_msr(0x40);
	change_pc(ppc.pc);

	ppc.hid0 = 1;

	ppc.interrupt_pending = 0;
}


static data32_t ppc603_readop_virt(offs_t address);

static int ppc603_execute(int cycles)
{
	UINT32 opcode, dec_old;
	ppc_icount = cycles;
	change_pc(ppc.npc);

	while( ppc_icount > 0 )
	{
		//int cc = (ppc_icount >> 2) & 0x1;
		dec_old = DEC;
		ppc.pc = ppc.npc;
		CALL_MAME_DEBUG;

		ppc.npc = ppc.pc + 4;
		if (MSR & MSR_IR)
			opcode = ppc603_readop_virt(ppc.pc);
		else
			opcode = ROPCODE64(ppc.pc);

		switch(opcode >> 26)
		{
			case 19:	optable19[(opcode >> 1) & 0x3ff](opcode); break;
			case 31:	optable31[(opcode >> 1) & 0x3ff](opcode); break;
			case 59:	optable59[(opcode >> 1) & 0x3ff](opcode); break;
			case 63:	optable63[(opcode >> 1) & 0x3ff](opcode); break;
			default:	optable[opcode >> 26](opcode); break;
		}

		ppc_icount--;

		ppc.tb += 1;

		DEC -= 1;
		if(DEC == 0) {
			ppc.interrupt_pending |= 0x2;
		}
		
		ppc603_check_interrupts();
	}

	return cycles - ppc_icount;
}

/* ----------------------------------------------------------------------- */

static int ppc603_translate_virt(offs_t *addr_ptr, const BATENT *bat)
{
	UINT32 address;
	UINT32 vsid, hash, pteg_address;
	UINT32 target_pte, bl, mask;
	UINT64 pte, *pteg_ptr;
	int i;

	address = *addr_ptr;

	/* first check the block address translation table */
	for (i = 0; i < 4; i++)
	{
		if (bat[i].u & ((MSR & MSR_PR) ? 0x00000001 : 0x00000002))
		{
			bl = bat[i].u & 0x00001FFC;
			mask = (~bl << 15) & 0xFFFE0000;

			if ((address & mask) == (bat[i].u & 0xFFFE0000))
			{
				*addr_ptr = (bat[i].l & 0xFFFE0000)
					| (address & ((bl << 15) | 0x0001FFFF));
				return 1;
			}
		}
	}

	/* now try page address translation */
	vsid = ppc.sr[(address >> 28) & 0x0F];
	hash = (vsid & 0x7FFFF) ^ ((address >> 12) & 0xFFFF);

	pteg_address = (ppc.sdr1 & 0xFFFF0000)
		| (((ppc.sdr1 & 0x01FF) & (hash >> 10)) << 16)
		| ((hash & 0x03FF) << 6);

	target_pte = (vsid << 7) | ((address >> 22) & 0x3F) | 0x80000000;

	pteg_ptr = memory_get_read_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, pteg_address);
	if (pteg_ptr)
	{
		for (i = 0; i < 8; i++)
		{
			pte = pteg_ptr[i];

			/* is valid? */
			if (((pte >> 32) & 0xFFFFFFFF) == target_pte)
			{
				*addr_ptr = ((UINT32) (pte & 0xFFFFF000))
					| (address & 0x0FFF);
				return 1;
			}
		}
	}

	/* lookup failure - exception */
	ppc603_exception(EXCEPTION_TRAP);
	return 0;
}

static data8_t ppc603_read8_virt(offs_t address)
{
	if (ppc603_translate_virt(&address, ppc.dbat))
		return program_read_byte_64be(address);
	return 0;
}

static data16_t ppc603_read16_virt(offs_t address)
{
	if (ppc603_translate_virt(&address, ppc.dbat))
		return program_read_word_64be(address);
	return 0;
}

static data32_t ppc603_read32_virt(offs_t address)
{
	if (ppc603_translate_virt(&address, ppc.dbat))
		return program_read_dword_64be(address);
	return 0;
}

static data64_t ppc603_read64_virt(offs_t address)
{
	if (ppc603_translate_virt(&address, ppc.dbat))
		return program_read_qword_64be(address);
	return 0;
}

static void ppc603_write8_virt(offs_t address, data8_t data)
{
	if (ppc603_translate_virt(&address, ppc.dbat))
		program_write_byte_64be(address, data);
}

static void ppc603_write16_virt(offs_t address, data16_t data)
{
	if (ppc603_translate_virt(&address, ppc.dbat))
		program_write_word_64be(address, data);
}

static void ppc603_write32_virt(offs_t address, data32_t data)
{
	if (ppc603_translate_virt(&address, ppc.dbat))
		program_write_dword_64be(address, data);
}

static void ppc603_write64_virt(offs_t address, data64_t data)
{
	if (ppc603_translate_virt(&address, ppc.dbat))
		program_write_qword_64be(address, data);
}

static data32_t ppc603_readop_virt(offs_t address)
{
	if (ppc603_translate_virt(&address, ppc.ibat))
		return program_read_dword_64be(address);
	return 0;
}


