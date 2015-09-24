#pragma once

#include <cstring> // memset
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdlib.h>

#include "PPU.hpp"
#include "APU.hpp"
#include "ROM.hpp"

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
	using word_t = uint8_t;
	using addr_t = uint16_t;
	
	static constexpr addr_t RAMSize = 0x0800;
	static constexpr size_t ClockRate = 1789773; // Hz
	
	ROM*		rom;
	PPU*		ppu;
	APU*		apu;
	
	CPU() :
		_ram(new word_t[RAMSize])
	{
	}
	
	~CPU()
	{
		delete _ram;
	} 
	
	inline unsigned int getCycles() const
	{
		return _cycles;
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
		_reg_pc = read(0xFFFC) + ((static_cast<addr_t>(read(0xFFFD) & 0xFF) << 8));
		//_reg_pc = 0xC000; /// @TODO Remove (here for nestest.nes)
		_reg_sp = 0xFD; /// @TODO: Check
		_reg_ps = 0b00100000; /// @TODO: Check
		_reg_acc = _reg_x = _reg_y = 0x00;
		std::memset(_ram, 0xFF, RAMSize);
	}
	
	void setTestLog(const std::string& path)
	{
		_test_file.open(path, std::ios::binary);
	}
	
	void step()
	{
		//std::cout << std::hex << _reg_pc << std::endl;
			
		///////////////
		// TESTING
		if(_test_file.is_open())
		{
			static int instr_count = 0;
			static int error_count = 0;
			instr_count++;
			char str[4];
			_test_file.getline(str, 6);
			std::stringstream ss(str);
			int expected_addr = 0;
			ss >> std::hex >> expected_addr;
			
			if(_reg_pc != expected_addr)
			{
				std::cerr << instr_count << " ERROR! Got " << std::hex << (int) _reg_pc << " expected " << std::hex << expected_addr << std::endl;
				std::cerr << " A:" << std::hex << (int) _reg_acc <<
							" X:" << std::hex << (int) _reg_x << 
							" Y:" << std::hex << (int) _reg_y <<
							" PS:" << std::hex << (int) _reg_ps <<
							" SP:" << std::hex << (int) _reg_sp << std::endl;
				_reg_pc = expected_addr;
				error_count++;
				exit(1);
			} //else std::cout << instr_count << " OK" << std::endl;
		}
		
		word_t opcode = read(_reg_pc++);
		execute(opcode);
	}
	
	void execute(word_t opcode);
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	// Memory access
	
	word_t read(addr_t addr) const
	{
		if(addr < RAMSize)
			return _ram[addr];
		else if(addr < 0x2000) // RAM mirrors
			return _ram[addr % RAMSize];
		else if(addr < 0x2008) // PPU registers
			return ppu->read(addr);
		else if(addr < 0x4000) // PPU registers mirrors
			return ppu->read(addr);
		else if(addr < 0x4018) // pAPU
			return apu->read(addr);
		else if(addr < 0x4020) // Controllers registers
			return 0; /// @todo
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
		else if(addr < 0x2008) // PPU registers
			ppu->write(addr, value);
		else if(addr < 0x4000) // PPU registers mirrors
			ppu->write(addr % 0x08, value);
		else if(addr < 0x4018) // pAPU / Controllers registers
			apu->write(addr - 0x4000, value);
		else if(addr < 0x4020) // Controllers registers
			(void) 0; /// @todo
		else {
			std::cerr << "Error: Write addr 0x" << std::hex << addr << " out of bounds." << std::endl;
			exit(1);
		}
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	// Stack
	
	inline void push(word_t value)
	{
		_ram[0x100 + _reg_sp--] = value;
	}
	
	inline word_t pop()
	{
		return _ram[0x100 + ++_reg_sp];
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
		set(StateMask::Negative, operand & 0b10000000);
	}
	
	/// Sets the Zero Flag if the operand is $#00, otherwise clears it.
	inline void set_zero(word_t operand)
	{
		set(StateMask::Zero, operand == 0);
	}
	
	inline void set_neg_zero(word_t operand)
	{
		set_neg(operand);
		set_zero(operand);
	}
	
	inline addr_t get_pc() const { return _reg_pc; }
	inline word_t get_acc() const { return _reg_acc; }
	inline word_t get_x() const { return _reg_x; }
	inline word_t get_y() const { return _reg_y; }
	inline word_t get_sp() const { return _reg_sp; }
	inline word_t get_ps() const { return _reg_ps; }
	
	inline word_t get_next_opcode() const { return read(_reg_pc); }
	inline word_t get_next_operand0() const { return read(_reg_pc + 1); }
	inline word_t get_next_operand1() const { return read(_reg_pc + 2); }
	
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
	
	#include "CPUInstr.inl"
	
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
		++_reg_pc;
		return r;
	}
	
	inline addr_t addr_absX()
	{
		addr_t r = read(_reg_pc) + (read((_reg_pc + 1) & 0xFFFF) << 8) + _reg_x;
		++_reg_pc;
		++_reg_pc;
		return r;
	}
	
	inline addr_t addr_absY()
	{
		addr_t r = read(_reg_pc) + (read((_reg_pc + 1) & 0xFFFF) << 8) + _reg_y;
		++_reg_pc;
		++_reg_pc;
		return r;
	}
	
	inline addr_t addr_zero()
	{
		addr_t r = read(_reg_pc);
		++_reg_pc;
		return r & 0xFF;
	}
	
	inline addr_t addr_zeroX()
	{
		addr_t r = (read(_reg_pc) + _reg_x) & 0xFF;
		++_reg_pc;
		return r;
	}
	
	inline addr_t addr_zeroY()
	{
		addr_t r = (read(_reg_pc) + _reg_y) & 0xFF;
		++_reg_pc;
		return r;
	}
	
	inline addr_t addr_indirect()
	{
		addr_t l = read(_reg_pc);
		addr_t h = read((_reg_pc + 1) & 0xFFFF);
		++_reg_pc;
		++_reg_pc;
		addr_t tmp = (l | (h << 8));
		// This addressing mode is bugged on the actual hardware!
		//                       Here! \/
		return read(tmp) + ((read((((l + 1) & 0xFF) | (h << 8))) & 0xFF) << 8);
		// 'Correct' version:
		// return read(tmp) + ((read(tmp + 1) & 0xFF) << 8);
	}
	
	inline addr_t addr_indirectX()
	{
		addr_t tmp = read(_reg_pc);
		addr_t l = read((_reg_x + tmp) & 0xFF);
		addr_t h = read((_reg_x + tmp + 1) & 0xFF);
		++_reg_pc;
		return (l | (h << 8));
	}
	
	inline addr_t addr_indirectY()
	{
		addr_t tmp = read(_reg_pc);
		addr_t l = read(tmp);
		addr_t h = read((tmp + 1) & 0xFF);
		++_reg_pc;
		return ((l | (h << 8)) + _reg_y) & 0xFFFF;
	}
};
