#pragma once

#include <cstring> // memset
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdlib.h>

#include "Log.hpp"
#include "PPU.hpp"
#include "APU.hpp"
#include "Cartridge.hpp"

/**
 * NES Central Processing Unit
 *
 * 8bits CPU
 * Ricoh 2A03 (kind of 6502)
 * Little-Endian
 *
 * @see http://nesdev.com/nesdoc1.txt
**/
class CPU
{
public:	
	static constexpr addr_t RAMSize = 0x0800;
	static constexpr size_t ClockRate = 1789773; // Hz
	
	static Log log;
	static Log error;
	
	Cartridge*		cartridge;
	PPU*			ppu;
	APU*			apu;
	
	CPU() :
		_ram(new word_t[RAMSize])
	{
	}
	
	~CPU()
	{
		delete _ram;
	}
	
	void reset();
	
	void set_test_log(const std::string& path);
	
	void step();	
	void execute(word_t opcode);
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	// Memory access
	inline word_t read(addr_t addr) const;
	inline addr_t read16(addr_t addr) const;
	inline void write(addr_t addr, word_t value);
	////////////////////////////////////////////////////////////////////////////////////////////////
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	// Stack
	inline void push(word_t value);
	inline void push16(addr_t value);
	inline word_t pop();
	inline addr_t pop16();
	////////////////////////////////////////////////////////////////////////////////////////////////
	
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
	
	inline bool check(StateMask mask);
	inline void set(StateMask mask);
	inline void clear(StateMask mask);
	inline void set(StateMask mask, bool b);
	/// Clears the Negative Flag if the operand is $#00-7F, otherwise sets it.
	inline void set_neg(word_t operand);
	/// Sets the Zero Flag if the operand is $#00, otherwise clears it.
	inline void set_zero(word_t operand);
	inline void set_neg_zero(word_t operand);
	////////////////////////////////////////////////////////////////////////////////////////////////
	
	inline addr_t get_pc() const { return _reg_pc; }
	inline word_t get_acc() const { return _reg_acc; }
	inline word_t get_x() const { return _reg_x; }
	inline word_t get_y() const { return _reg_y; }
	inline word_t get_sp() const { return _reg_sp; }
	inline word_t get_ps() const { return _reg_ps; }
	
	inline word_t get_next_opcode() const { return read(_reg_pc); }
	inline word_t get_next_operand0() const { return read(_reg_pc + 1); }
	inline word_t get_next_operand1() const { return read(_reg_pc + 2); }
	
	inline size_t get_cycles() const { return _cycles; }
	
	static size_t instr_length[0x100];
	static size_t instr_cycles[0x100];
	
private:
	// Registers
	addr_t		_reg_pc		= 0x0000;	///< Program Counter
	word_t		_reg_acc	= 0x00;		///< Accumulator
	word_t		_reg_x		= 0x00;		///< X Register
	word_t		_reg_y		= 0x00;		///< Y Register
	word_t		_reg_sp		= 0x00;		///< Stack Pointer
	word_t		_reg_ps		= 0x00;		///< Processor Status
	
	unsigned int	_cycles = 0;
	
	// Memory
	word_t*		_ram;		///< RAM
	
	bool		_shutdown = false;
	
	std::ifstream	_test_file;
	
	void oam_dma(word_t value)
	{
		// Takes 513 or 514 (if on an odd cycle) Cycles
		ppu->write(0x2003, 0x00); // Reset OAMADDR
		addr_t start = (value << 8);
		for(addr_t a = 0; a < 256; ++a)
			ppu->write(0x2004, _ram[start + a]);
	}
	
	#include "CPUInstr.inl"
	
	// Addressing Modes
	
	inline addr_t addr_immediate();
	inline addr_t addr_abs();
	inline addr_t addr_absX();
	inline addr_t addr_absY();
	inline addr_t addr_zero();
	inline addr_t addr_zeroX();
	inline addr_t addr_zeroY();
	inline addr_t addr_indirect();
	inline addr_t addr_indirectX();
	inline addr_t addr_indirectY();
};

#include "CPU.inl"
