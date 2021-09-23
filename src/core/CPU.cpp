#include "CPU.hpp"

// @todo TODO
size_t	CPU::instr_length[0x100] = {
	1, 2, 0, 0, 0, 2, 2, 0, 1, 2, 1, 0, 0, 3, 3, 0, // 0
	2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0, // 1
	3, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0, // 2
	2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 0, 0, 0, 3, 3, 0, // 3
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 4
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 5
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 6
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 7
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 8
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 9
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // A
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // B
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // C
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // D
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // E
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  // F
//  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
};

size_t	CPU::instr_cycles[0x100] = {
	7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6, // 0 
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, // 1 
	6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6, // 2 
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, // 3 
	6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6, // 4 
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, // 5 
	6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6, // 6 
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, // 7 
	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4, // 8 
	2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5, // 9 
	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4, // A 
	2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4, // B 
	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, // C 
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, // D 
	2, 6, 3, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, // E 
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7  // F
//  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
};


CPU::CPU(size_t _RAMSize) :
	RAMSize(_RAMSize),
	_ram(new word_t[RAMSize])
{
}

CPU::~CPU()
{
	delete _ram;
}

void CPU::reset()
{
	_reg_pc = read(0xFFFC) | (read(0xFFFD) << 8);
	_reg_sp = 0xFD; /// @TODO: Check
	_reg_ps = 0x34; /// @TODO: Check
	_reg_acc = _reg_x = _reg_y = 0x00;
	std::memset(_ram, 0xFF, RAMSize);
}

void CPU::step()
{
	if (ppu->check_nmi())
	{
		push16(_reg_pc);
		push(_reg_ps | 0b00100000);
		_reg_ps |= Interrupt;
		_reg_pc = read16(0xFFFA);
	}

	if (!(_reg_ps & Interrupt) && _irq)
	{
		_irq = false;
		push16(_reg_pc);
		push(_reg_ps | 0b00100000);
		_reg_ps |= Interrupt;
		_reg_pc = read16(0xFFFE);
	}

	if (_refresh_controller)
		refresh_controller_states();

	word_t opcode = read(_reg_pc++);
	execute(opcode);
}

void CPU::refresh_controller_states()
{
	_current_controller_read = 0;
	for (size_t i = 0; i < 8; ++i)
	{
		_controller_states[i] = controller_callbacks[i] ? controller_callbacks[i]() : false;
		_controller2_states[i] = controller2_callbacks[i] ? controller2_callbacks[i]() : false;
	}
}

word_t CPU::read_controller_state()
{
	word_t r = 0x40;
	if (_current_controller_read < 8)
		r |= _controller_states[_current_controller_read] ? 1 : 0;
	else if (_current_controller_read == 19)
		r = 0x41;
	else
		r = 0x40;
	++_current_controller_read;
	return r;
}

word_t CPU::read_controller2_state()
{
	word_t r = 0x40;
	if (_current_controller_read < 8)
		r |= _controller2_states[_current_controller_read] ? 1 : 0;
	else if (_current_controller_read == 18)
		r = 0x41;
	else
		r = 0x40;
	++_current_controller_read;
	return r;
}

#define OP(C,O,A) case C: O(A()); /* log << Hexa(_reg_pc) << " " #C " " #O " " #A << std::endl; */ break;
#define OPM(C,O,A) case C: { auto tmp = A(); write(tmp, O(tmp)); /* log << Hexa(_reg_pc) << " " #C " " #O " " #A << std::endl; */ break; }
#define OP_(C,O) case C: O(); /* log << Hexa(_reg_pc) << " " #C " " #O " Implied" << std::endl; */ break;
#define OPA(C,O) case C: _reg_acc = O(_reg_acc); /* log << Hexa(_reg_pc) << " " #C " " #O " ACC" << std::endl; */ break;

void CPU::execute(word_t opcode)
{
	_cycles = instr_cycles[opcode];
	/// @todo Handle these cases (for some instructions):
	/// * add 1 to cycles if page boundery is crossed
	/// ** add 1 to cycles if branch occurs on same page add 2 to cycles if branch occurs to different page
	switch (opcode)
	{
		/// @see http://datacrystal.romhacking.net/wiki/6502_opcodes
		/// @see http://e-tradition.net/bytes/6502/6502_instruction_set.html
		/// @see http://homepage.ntlworld.com/cyborgsystems/CS_Main/6502/6502.htm
		/// @see http://nesdev.com/undocumented_opcodes.txt

		// ADC
		OP(0x61, adc, addr_indirectX);
		OP(0x71, adc, addr_indirectY);
		OP(0x6D, adc, addr_abs);
		OP(0x7D, adc, addr_absX);
		OP(0x69, adc, addr_immediate);
		OP(0x79, adc, addr_absY);
		OP(0x65, adc, addr_zero);
		OP(0x75, adc, addr_zeroX);

		// AND
		OP(0x21, and_, addr_indirectX);
		OP(0x31, and_, addr_indirectY);
		OP(0x2D, and_, addr_abs);
		OP(0x3D, and_, addr_absX);
		OP(0x29, and_, addr_immediate);
		OP(0x39, and_, addr_absY);
		OP(0x25, and_, addr_zero);
		OP(0x35, and_, addr_zeroX);

		// ASL
		OPA(0x0A, asl);
		OPM(0x06, asl, addr_zero);
		OPM(0x16, asl, addr_zeroX);
		OPM(0x0E, asl, addr_abs);
		OPM(0x1E, asl, addr_absX);

		OP_(0x90, bcc);
		OP_(0xB0, bcs);
		OP_(0xF0, beq);
		OP(0x2C, bit, addr_abs);
		OP(0x24, bit, addr_zero);
		OP_(0x30, bmi);
		OP_(0xD0, bne);
		OP_(0x10, bpl);
		OP_(0x00, brk);
		OP_(0x50, bvc);
		OP_(0x70, bvs);
		OP_(0x18, clc);
		OP_(0xD8, cld);
		OP_(0x58, cli);
		OP_(0xB8, clv);

		// DCP (Unofficial)
		OP(0xC7, dcp, addr_zero);
		OP(0xD7, dcp, addr_zeroX);
		OP(0xCF, dcp, addr_abs);
		OP(0xDF, dcp, addr_absX);
		OP(0xDB, dcp, addr_absY);
		OP(0xC3, dcp, addr_indirectX);
		OP(0xD3, dcp, addr_indirectY);


		// CMP
		OP(0xC1, cmp, addr_indirectX);
		OP(0xD1, cmp, addr_indirectY);
		OP(0xCD, cmp, addr_abs);
		OP(0xDD, cmp, addr_absX);
		OP(0xC9, cmp, addr_immediate);
		OP(0xD9, cmp, addr_absY);
		OP(0xC5, cmp, addr_zero);
		OP(0xD5, cmp, addr_zeroX);

		OP(0xEC, cpx, addr_abs);
		OP(0xE0, cpx, addr_immediate);
		OP(0xE4, cpx, addr_zero);

		OP(0xCC, cpy, addr_abs);
		OP(0xC0, cpy, addr_immediate);
		OP(0xC4, cpy, addr_zero);

		OPM(0xCE, dec, addr_abs);
		OPM(0xDE, dec, addr_absX);
		OPM(0xC6, dec, addr_zero);
		OPM(0xD6, dec, addr_zeroX);

		OP_(0xCA, dex);
		OP_(0x88, dey);

		// EOR
		OP(0x41, eor, addr_indirectX);
		OP(0x51, eor, addr_indirectY);
		OP(0x4D, eor, addr_abs);
		OP(0x5D, eor, addr_absX);
		OP(0x49, eor, addr_immediate);
		OP(0x59, eor, addr_absY);
		OP(0x45, eor, addr_zero);
		OP(0x55, eor, addr_zeroX);

		OPM(0xEE, inc, addr_abs);
		OPM(0xFE, inc, addr_absX);
		OPM(0xE6, inc, addr_zero);
		OPM(0xF6, inc, addr_zeroX);

		OP_(0xE8, inx);
		OP_(0xC8, iny);

		OP(0x6C, jmp, addr_indirect);
		OP(0x4C, jmp, addr_abs);

		OP(0x20, jsr, addr_abs); // Some source says Implied, another Absolute...

		// Unofficial
		OP(0xAF, lax, addr_abs);
		OP(0xBF, lax, addr_absY);
		OP(0xA7, lax, addr_zero);
		OP(0xB7, lax, addr_zeroY);
		OP(0xA3, lax, addr_indirectX);
		OP(0xB3, lax, addr_indirectY);

		// LDA  (Load Accumulator With Memory)
		OP(0xA1, lda, addr_indirectX);
		OP(0xB1, lda, addr_indirectY);
		OP(0xAD, lda, addr_abs);
		OP(0xBD, lda, addr_absX);
		OP(0xA9, lda, addr_immediate);
		OP(0xB9, lda, addr_absY);
		OP(0xA5, lda, addr_zero);
		OP(0xB5, lda, addr_zeroX);

		OP(0xAE, ldx, addr_abs);
		OP(0xA2, ldx, addr_immediate);
		OP(0xBE, ldx, addr_absY);
		OP(0xA6, ldx, addr_zero);
		OP(0xB6, ldx, addr_zeroY);

		OP(0xAC, ldy, addr_abs);
		OP(0xBC, ldy, addr_absX);
		OP(0xA0, ldy, addr_immediate);
		OP(0xA4, ldy, addr_zero);
		OP(0xB4, ldy, addr_zeroX);

		OPA(0x4A, lsr);
		OPM(0x4E, lsr, addr_abs);
		OPM(0x5E, lsr, addr_absX);
		OPM(0x46, lsr, addr_zero);
		OPM(0x56, lsr, addr_zeroX);

		OP_(0xEA, nop);

		// Extension NOP
		OP_(0x1A, nop);
		OP_(0x3A, nop);
		OP_(0x5A, nop);
		OP_(0x7A, nop);
		OP_(0xDA, nop);
		OP_(0xFA, nop);
		OP_(0x04, _reg_pc++; nop);
		OP_(0x44, _reg_pc++; nop);
		OP_(0x64, _reg_pc++; nop);
		OP_(0x0C, _reg_pc += 2; nop);
		OP_(0x14, _reg_pc++; nop);
		OP_(0x34, _reg_pc++; nop);
		OP_(0x54, _reg_pc++; nop);
		OP_(0x74, _reg_pc++; nop);
		OP_(0xD4, _reg_pc++; nop);
		OP_(0xF4, _reg_pc++; nop);
		OP_(0x80, _reg_pc++; nop);
		OP_(0x82, _reg_pc++; nop);
		OP_(0x89, _reg_pc++; nop);
		OP_(0x1C, _reg_pc += 2; nop);
		OP_(0x3C, _reg_pc += 2; nop);
		OP_(0x5C, _reg_pc += 2; nop);
		OP_(0x7C, _reg_pc += 2; nop);
		OP_(0xC2, _reg_pc++; nop);
		OP_(0xDC, _reg_pc += 2; nop);
		OP_(0xE2, _reg_pc++; nop);
		OP_(0xFC, _reg_pc += 2; nop);

		// ORA
		OP(0x01, ora, addr_indirectX);
		OP(0x11, ora, addr_indirectY);
		OP(0x0D, ora, addr_abs);
		OP(0x1D, ora, addr_absX);
		OP(0x09, ora, addr_immediate);
		OP(0x19, ora, addr_absY);
		OP(0x05, ora, addr_zero);
		OP(0x15, ora, addr_zeroX);

		OP_(0x48, pha);
		OP_(0x08, php);
		OP_(0x68, pla);
		OP_(0x28, plp);

		OP(0x2F, rla, addr_abs);
		OP(0x3F, rla, addr_absX);
		OP(0x3B, rla, addr_absY);
		OP(0x27, rla, addr_zero);
		OP(0x37, rla, addr_zeroX);
		OP(0x23, rla, addr_indirectX);
		OP(0x33, rla, addr_indirectY);

		OPA(0x2A, rol);
		OPM(0x2E, rol, addr_abs);
		OPM(0x3E, rol, addr_absX);
		OPM(0x26, rol, addr_zero);
		OPM(0x36, rol, addr_zeroX);

		OPA(0x6A, ror);
		OPM(0x6E, ror, addr_abs);
		OPM(0x7E, ror, addr_absX);
		OPM(0x66, ror, addr_zero);
		OPM(0x76, ror, addr_zeroX);

		OP_(0x40, rti);
		OP_(0x60, rts);

		// Unofficial
		OP(0x83, sax, addr_indirectX);
		OP(0x87, sax, addr_zero);
		OP(0x97, sax, addr_zeroY);
		OP(0x8F, sax, addr_abs);

		// SBC
		OP(0xE1, sbc, addr_indirectX);
		OP(0xF1, sbc, addr_indirectY);
		OP(0xED, sbc, addr_abs);
		OP(0xFD, sbc, addr_absX);
		OP(0xE9, sbc, addr_immediate);
		OP(0xEB, sbc, addr_immediate);	// Unofficial
		OP(0xF9, sbc, addr_absY);
		OP(0xE5, sbc, addr_zero);
		OP(0xF5, sbc, addr_zeroX);

		OP_(0x38, sec);
		OP_(0xF8, sed);
		OP_(0x78, sei);

		// STA
		OP(0x81, sta, addr_indirectX);
		OP(0x91, sta, addr_indirectY);
		OP(0x8D, sta, addr_abs);
		OP(0x9D, sta, addr_absX);
		OP(0x99, sta, addr_absY);
		OP(0x85, sta, addr_zero);
		OP(0x95, sta, addr_zeroX);

		OP(0x8E, stx, addr_abs);
		OP(0x86, stx, addr_zero);
		OP(0x96, stx, addr_zeroY);

		OP(0x8C, sty, addr_abs);
		OP(0x84, sty, addr_zero);
		OP(0x94, sty, addr_zeroX);

		OP_(0xAA, tax);
		OP_(0xA8, tay);
		OP_(0xBA, tsx);
		OP_(0x8A, txa);
		OP_(0x9A, txs);
		OP_(0x98, tya);

		OP(0xFF, isc, addr_absX);

	default:
	{
		Log::error("Unknown opcode: ", Hexa8(opcode));
		_reg_pc++; // Assuming at least one operand
		break;
	}
	}
}
