#pragma once

#include <cstring> // memset
#include <iostream>
#include <iomanip>

#include "ROM.hpp"
#include "PPU.hpp"

/**
 * NES Central Processing Unit
 *
 * 8bits CPU
 * 2A03 (kind of 6502)
 * Little-Endian
 *
 * @see http://nesdev.com/nesdoc1.txt
**/
class CPU
{
public:
	using word_t = uint8_t;
	using addr_t = uint16_t;
	
	static constexpr addr_t RAMSize = 0x07FF;
	static constexpr size_t ClockRate = 1789773; // Hz
	
	ROM*		rom;
	PPU*		ppu;
	
	CPU() :
		_ram(new word_t[RAMSize])
	{
	}
	
	~CPU()
	{
		delete _ram;
	} 
	
	inline void log(const std::string& str)
	{
		std::cout << str << std::endl;
	}
	
	inline void error(const std::string& str)
	{
		std::cerr << str << std::endl;
	}
	
	void reset()
	{
		//_reg_pc = 0x34;
		_reg_pc = read(0xFFFC) + (static_cast<addr_t>(read(0xFFFD)) << 8);
		_reg_sp = 0xFD;
		_reg_acc = _reg_x = _reg_y = _reg_ps = 0x00;
		std::memset(_ram, 0xFF, RAMSize);
	}
	
	void run()
	{
		reset();
		word_t opcode = 0x00;
		_reg_pc = 0xC000; /// @todo TEMP! Remove!
		int inst_count = 0;
		while(inst_count++ < 50)//!_shutdown)
		{
			std::cout << std::hex << _reg_pc << std::endl;
			opcode = read(_reg_pc++);
			execute(opcode);
			// PPU interupt if needed
			
			std::cout << std::dec << inst_count << "\tA:" << std::hex << _reg_acc << " PS:" << std::hex << (int) _reg_ps << std::endl;
		}
	}
	
	#define OP(C,O,A) case C: O(A()); log(#C " " #O " " #A); break;
	#define OPM(C,O,A) case C: { auto tmp = A(); write(tmp, O(tmp)); _reg_pc++; log(#C " " #O " " #A); break; }
	#define OP_(C,O) case C: O(); log(#C " " #O " Implied"); break;
	#define OPM_(C,O) case C: _reg_acc = O(_reg_acc); log(#C " " #O " Implied"); break;
	
	inline void execute(word_t opcode)
	{
		switch(opcode)
		{			
			/// @see http://datacrystal.romhacking.net/wiki/6502_opcodes
			/// @see http://e-tradition.net/bytes/6502/6502_instruction_set.htm
			/// @see http://homepage.ntlworld.com/cyborgsystems/CS_Main/6502/6502.htm
			
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
			OPM_(0x0A, asl);
			OPM(0x06, asl, addr_zero);
			OPM(0x16, asl, addr_zeroX);
			
			OP_(0x90, bcc);
			OP_(0xB0, bcs);
			OP_(0xF0, beq);
			OP(0x2C, bit, addr_abs);
			OP(0x24, bit, addr_zero);
			OP_(0x30, bmi);
			OP_(0xD0, bne);
			OP_(0x10, bpl);
			OP_(0x50, bvc);
			OP_(0x70, bvs);
			OP_(0x18, clc);
			OP_(0xD8, cld);
			OP_(0x58, cli);
			OP_(0xB8, clv);
			
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
			OP(0xB6, ldx, addr_zeroX);
			
			OP(0xAC, ldy, addr_abs);
			OP(0xBC, ldy, addr_absX);
			OP(0xA0, ldy, addr_immediate);
			OP(0xA4, ldy, addr_zero);
			OP(0xB4, ldy, addr_zeroX);
			
			OPM_(0x4A, lsr);
			OPM(0x4E, lsr, addr_abs);
			OPM(0x5E, lsr, addr_absX);
			OPM(0x46, lsr, addr_zero);
			
			OP_(0xEA, nop);
			
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
			
			OPM_(0x2A, rol);
			OPM(0x2E, rol, addr_abs);
			OPM(0x26, rol, addr_zero);
			OPM(0x36, rol, addr_zeroX);
			
			OPM_(0x6A, ror);
			OPM(0x6E, ror, addr_abs);
			OPM(0x7E, ror, addr_absX);
			OPM(0x66, ror, addr_zero);
			OPM(0x76, ror, addr_zeroX);
			
			OP_(0x40, rti);
			OP_(0x60, rts);
			
			// SBC
			OP(0xE1, sbc, addr_indirectX);
			OP(0xF1, sbc, addr_indirectY);
			OP(0xED, sbc, addr_abs);
			OP(0xFD, sbc, addr_absX);
			OP(0xE9, sbc, addr_immediate);
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
			OP(0x96, stx, addr_zeroX);
		
			OP(0x8C, sty, addr_abs);
			OP(0x84, sty, addr_zero);
			OP(0x94, sty, addr_zeroX);
			
			OP_(0xAA, tax);
			OP_(0xA8, tay);
			OP_(0xBA, tsx);
			OP_(0x8A, txa);
			OP_(0x9A, txs);
			OP_(0x98, tya);
			
			case 0x00: // BRK
			default:
				error("Unknown opcode...");
				break;
		}
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	// Memory access
	
	word_t read(addr_t addr)
	{
		if(addr < RAMSize)
			return _ram[addr];
		else if(addr < 0x2000) // RAM mirrors
			return _ram[addr % RAMSize];
		else if(addr < 0x2008) // PPU registers
			return ppu->registers[addr - 0x2000];
		else if(addr < 0x4000) // PPU registers mirrors
			return ppu->registers[(addr - 0x2000) % 8];
		else if(addr < 0x4017) // pAPU / Controllers registers
			return 0;
		else if(addr < 0x5000) // Unused
			return 0;
		else if(addr < 0x6000) // Expansion
			return 0;
		else if(addr < 0x8000) // SRAM
			return 0;
		else if(addr <= 0xFFFF) // ROM
			return rom->read(addr - 0x8000);
		else
			return 0; // Error
	}
	
	void write(addr_t addr, word_t value)
	{
		if(addr < RAMSize)
			_ram[addr] = value;
		else if(addr < 0x2000) // RAM mirrors
			_ram[addr % RAMSize] = value;
	}
	
	// Addressing Modes
	
	inline addr_t addr_immediate()
	{
		auto r = _reg_pc;
		++_reg_pc;
		return r;
	}
	
	inline addr_t addr_abs()
	{
		auto r = read(_reg_pc) + static_cast<addr_t>(read(_reg_pc + 1) << 8);
		++_reg_pc;
		return r;
	}
	
	inline addr_t addr_absX()
	{
		auto r = read(_reg_pc) + _reg_x;
		++_reg_pc;
		return r;
	}
	
	inline addr_t addr_absY()
	{
		auto r = read(_reg_pc) + _reg_y;
		++_reg_pc;
		return r;
	}
	
	inline addr_t addr_zero()
	{
		auto r = read(_reg_pc) & 0xFF;
		++_reg_pc;
		return r;
	}
	
	inline addr_t addr_zeroX()
	{
		auto r = (read(_reg_pc) + _reg_x) & 0xFF;
		++_reg_pc;
		return r;
	}
	
	inline addr_t addr_zeroY()
	{
		auto r = (read(_reg_pc) + _reg_y) & 0xFF;
		++_reg_pc;
		return r;
	}
	
	inline addr_t addr_indirect()
	{
		auto r = static_cast<addr_t>(read(_reg_pc)) | 
					(static_cast<addr_t>(read(_reg_pc + 1)) << 8);
		++_reg_pc;
		++_reg_pc;
		return r;
	}
	
	inline addr_t addr_indirectX()
	{
		auto r = static_cast<addr_t>(read((_reg_pc + _reg_x) & 0xFF)) |
					(static_cast<addr_t>(read((_reg_pc + _reg_x + 1) & 0xFF)) << 8);
		++_reg_pc;
		++_reg_pc;
		return r;
	}
	
	inline addr_t addr_indirectY()
	{
		auto r = ((static_cast<addr_t>(read((_reg_pc) & 0xFF)) |
					(static_cast<addr_t>(read((_reg_pc + 1) & 0xFF)) << 8)) + _reg_y) & 0xFFFF;
		++_reg_pc;
		++_reg_pc;
		return r;
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	// Stack
	
	void push(word_t value)
	{
		_ram[0x100 + _reg_sp--] = value;
	}
	
	word_t pop()
	{
		return _ram[0100 + ++_reg_sp];
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	// Processor Status Flags
	// @todo Use an other form of storage until status is queried ?
	
	enum StateMask : uint8_t
	{
		Carry		= 0b00000001,
		Zero		= 0b00000010,
		Interrupt	= 0b00000100, ///< Interrupt Disable (NMI)
		Decimal		= 0b00001000, ///< Decimal Mode (Unused)
		Break		= 0b00010000, ///< Break Command
		Overflow	= 0b01000000,
		Negative	= 0b10000000
	};
	
	inline bool check(StateMask mask)
	{
		return (_reg_ps & mask);
	}
	
	inline void set(StateMask mask)
	{
		_reg_ps |= mask;
	}
	
	inline void clear(StateMask mask)
	{
		_reg_ps &= ~mask;
	}
	
	inline void set(StateMask mask, bool b)
	{
		b ? set(mask) : clear(mask);
	}
	
	/// Clears the Negative Flag if the operand is $#00-7F, otherwise sets it.
	inline void set_neg(word_t operand)
	{
		_reg_ps = (_reg_ps & ~StateMask::Negative); // Clear Negative flag
		_reg_ps = _reg_ps | (StateMask::Negative & (operand < 0));
	}
	
	/// Sets the Zero Flag if the operand is $#00, otherwise clears it.
	inline void set_zero(word_t operand)
	{
		_reg_ps = (_reg_ps & ~StateMask::Zero); // Clear Zero flag
		_reg_ps = _reg_ps | ((operand == 0) ? StateMask::Zero : 0x0);
	}
	
	inline void set_neg_zero(word_t operand)
	{
		set_neg(operand);
		set_zero(operand);
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	// Instruction Set
	// All addresses are absolute here.
	
	// In alphabetical order.
	
	inline void adc(addr_t addr)
	{
		auto operand = read(addr);
		uint16_t add = _reg_acc + operand + (check(StateMask::Carry)) ? 0xFF : 0;
		if(add > 0xFF)
			set(StateMask::Carry);
		else
			clear(StateMask::Carry);
		
		// Overflow is set if the sum of two inputs with the same sign 
		// produce a result with a different sign.
		// a ^ b	=> 1 if sign bits are differents
		// & 0x80	=> Conserve only sign bit
		if(((~(_reg_acc ^ operand) & (_reg_acc ^ add)) & 0x80) == 0x00)
			clear(StateMask::Overflow);
		else
			set(StateMask::Overflow);
		
		_reg_acc = static_cast<word_t>(add);
		
		set_neg_zero(_reg_acc);
	}
	
	
	inline void and_(addr_t addr)
	{
		_reg_acc &= read(addr);
		set_neg_zero(_reg_acc);
	}
	
	inline word_t asl(addr_t addr)
	{
		auto operand = read(addr);
		set(StateMask::Carry, operand & 0b10000000);
		operand = operand << 1;
		set(StateMask::Negative, operand & 0b10000000);
		set(StateMask::Zero, operand & 0b10000000);
		return operand;
	}
	
	// Helper
	inline void relative_jump(bool b)
	{
		b ? _reg_pc += read(_reg_pc++) :
			_reg_pc++;
	}
	
	inline void bcc()
	{
		relative_jump(!check(StateMask::Carry));
	}
	
	inline void bcs()
	{
		relative_jump(check(StateMask::Carry));
	}
	
	inline void beq()
	{
		relative_jump(check(StateMask::Zero));
	}
	
	inline void bit(addr_t addr)
	{
		word_t tmp = 0b11000000 & (_reg_acc & read(addr));
		_reg_ps |= tmp;
		set(StateMask::Zero, (tmp == 0));
	}
	
	inline void bmi()
	{
		relative_jump(check(StateMask::Negative));
	}
	
	inline void bne()
	{
		relative_jump(!check(StateMask::Zero));
	}
	
	inline void bpl()
	{
		relative_jump(!check(StateMask::Negative));
	}
	
	inline void brk()
	{
		error("BRK not implemented");
	}
	
	inline void bvc()
	{
		relative_jump(!check(StateMask::Overflow));
	}
	
	inline void bvs()
	{
		relative_jump(check(StateMask::Overflow));
	}
	
	inline void clc()
	{
		clear(StateMask::Carry);
	}
	
	inline void cld()
	{
		clear(StateMask::Decimal);
	}
	
	inline void cli()
	{
		clear(StateMask::Interrupt);
	}
	
	inline void clv()
	{
		clear(StateMask::Overflow);
	}
	
	// Helper
	inline void comp(word_t lhs, word_t rhs)
	{
		set(StateMask::Carry, (lhs >= rhs));
		set_neg_zero(lhs - rhs);
	}
	
	inline void cmp(addr_t addr)
	{
		comp(_reg_acc, read(addr));
	}
	
	inline void cpx(addr_t addr)
	{
		comp(_reg_x, read(addr));
	}
	
	inline void cpy(addr_t addr)
	{
		comp(_reg_y, read(addr));
	}
	
	inline word_t dec(addr_t addr)
	{
		word_t operand = read(addr);
		--operand;
		set_neg_zero(operand);
		return operand;
	}
	
	inline void dex()
	{
		--_reg_x;
		set_neg_zero(_reg_x);
	}
	
	inline void dey()
	{
		--_reg_y;
		set_neg_zero(_reg_y);
	}
	
	inline void eor(addr_t addr)
	{
		_reg_acc ^= read(addr);
		set_neg_zero(_reg_acc);
	}
	
	inline word_t inc(addr_t addr)
	{
		word_t operand = read(addr);
		++operand;
		set_neg_zero(operand);
		return operand;
	}
	
	inline void inx()
	{
		++_reg_x;
		set_neg_zero(_reg_x);
	}
	
	inline void iny()
	{
		++_reg_y;
		set_neg_zero(_reg_y);
	}
	
	inline void jmp(addr_t addr)
	{
		_reg_pc = addr;
	}
	
	inline void jsr(addr_t addr)
	{
		auto tmp = _reg_pc - 1;
		push(tmp & 0xFF);
		push((tmp >> 8) & 0xFF);
		_reg_pc = addr;
	}
	
	inline void lda(addr_t addr)
	{
		word_t operand = read(addr);
		set_neg_zero(operand);     
		_reg_acc = operand;
	}
	
	inline void ldx(addr_t addr)
	{
		auto operand = read(addr);
		set_neg_zero(operand);
		_reg_x = operand;
	}
	
	inline void ldy(addr_t addr)
	{
		word_t operand = read(addr);
		set_neg_zero(operand);
		_reg_y = operand;
	}
	
	inline word_t lsr(addr_t addr)
	{
		word_t operand = read(addr);
		clear(StateMask::Negative);
		set(StateMask::Carry, operand & 0b00000001);
		operand = (operand >> 1) & 0x7F;
		set(StateMask::Zero, operand == 0);
		return operand;
	}
	
	inline void nop()
	{
	}
	
	inline void ora(addr_t addr)
	{
		_reg_acc |= read(addr);
		set_neg_zero(_reg_acc);
	}
	
	inline void pha()
	{
		push(_reg_acc);
	}
	
	inline void php()
	{
		push(_reg_ps);
	}
	
	inline void pla()
	{
		_reg_acc = pop();
		set_neg_zero(_reg_acc);
	}
	
	inline void plp()
	{
		_reg_ps = pop();
		set_neg_zero(_reg_ps);
	}
	
	inline word_t rol(addr_t addr)
	{
		word_t operand = read(addr);
		auto t = operand | 0b10000000;
		operand = (operand << 1) & 0xFE;
		operand = operand | (check(StateMask::Carry) ? StateMask::Carry : 0);
		set(StateMask::Carry, t != 0);
		set_neg_zero(operand);
		return operand;
	}
	
	inline word_t ror(addr_t addr)
	{
		word_t operand = read(addr);
		auto t = operand | 0b00000001;
		operand = (operand >> 1) & 0x7F;
		operand = operand | (check(StateMask::Carry) ? 0x80 : 0);
		set(StateMask::Carry, t != 0);
		set_neg_zero(operand);
		return operand;
	}
	
	inline void rti()
	{
		error("RTI not implemented yet");
	}
	
	inline void rts()
	{
		addr_t l = pop();
		addr_t h = pop();
		_reg_pc = (h << 8) + l + 1;
	}
	
	inline void sbc(addr_t addr)
	{
		word_t operand = read(addr);
		uint16_t add = _reg_acc - operand - (!check(StateMask::Carry)) ? 0xFF : 0;
		if(add > 0xFF)
			set(StateMask::Carry);
		else
			clear(StateMask::Carry);
		
		// Overflow is set if the sum of two inputs with the same sign 
		// produce a result with a different sign.
		// a ^ b	=> 1 if sign bits are differents
		// & 0x80	=> Conserve only sign bit
		if(((~(_reg_acc ^ operand) & (_reg_acc ^ add)) & 0x80) == 0x00)
			clear(StateMask::Overflow);
		else
			set(StateMask::Overflow);
		
		_reg_acc = static_cast<word_t>(add);
		
		set_neg_zero(_reg_acc);
	}
	
	inline void sec()
	{
		set(StateMask::Carry);
	}
	
	inline void sed()
	{
		set(StateMask::Decimal);
	}
	
	inline void sei()
	{
		set(StateMask::Interrupt);
	}
	
	inline void sta(addr_t addr)
	{
		write(read(addr), _reg_acc);
	}
	
	inline void stx(addr_t addr)
	{
		write(read(addr), _reg_x);
	}
	
	inline void sty(addr_t addr)
	{
		write(read(addr), _reg_y);
	}
	
	inline void tax()
	{
		set_neg_zero(_reg_acc);
		_reg_x = _reg_acc;
	}
	
	inline void tay()
	{
		set_neg_zero(_reg_acc);
		_reg_y = _reg_acc;
	}
	
	inline void tsx()
	{
		set_neg_zero(_reg_sp);
		_reg_x = _reg_sp;
	}
	
	inline void txa()
	{
		set_neg_zero(_reg_x);
		_reg_acc = _reg_x;
	}
	
	inline void txs()
	{
		set_neg_zero(_reg_x);
		_reg_sp = _reg_x;
	}
	
	inline void tya()
	{
		set_neg_zero(_reg_y);
		_reg_acc = _reg_y;
	}
	
private:
	// Registers
	addr_t		_reg_pc		= 0x0000;	///< Program Counter
	word_t		_reg_acc	= 0x00;		///< Accumulator
	word_t		_reg_x		= 0x00;		///< X Register
	word_t		_reg_y		= 0x00;		///< Y Register
	word_t		_reg_sp		= 0x00;		///< Stack Pointer
	word_t		_reg_ps		= 0x00;		///< Processor Status
	
	// Memory
	word_t*		_ram;		///< RAM
	
	bool		_shutdown = false;
};
