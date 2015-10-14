////////////////////////////////////////////////////////////////////////////////////////////////
// CPU Inline functions

void CPU::log(const std::string& str)
{
	std::cout << str << std::endl;
}

void CPU::error(const std::string& str)
{
	std::cerr << str << std::endl;
}

word_t CPU::read(addr_t addr) const
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
		return cartridge->read(addr);
	else if(addr < 0x8000) // SRAM
		return cartridge->read(addr);
	else if(addr <= 0xFFFF) // Cartridge
		return cartridge->read(addr);
	else
		return 0; // Error
}

void CPU::write(addr_t addr, word_t value)
{
	if(addr < RAMSize)
		_ram[addr] = value;
	else if(addr < 0x2000) // RAM mirrors
		_ram[addr % RAMSize] = value;
	else if(addr < 0x2008) // PPU registers
		ppu->write(addr, value);
	else if(addr == 0x4014) // OAMDMA
		oam_dma(value);
	else if(addr < 0x4000) // PPU registers mirrors
		ppu->write(addr, value);
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

inline void CPU::push(word_t value)
{
	_ram[0x100 + _reg_sp--] = value;
}

inline word_t CPU::pop()
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

inline bool CPU::check(StateMask mask)
{
	return (_reg_ps & mask);
}

inline void CPU::set(StateMask mask)
{
	_reg_ps |= mask;
}

inline void CPU::clear(StateMask mask)
{
	_reg_ps &= ~mask;
}

inline void CPU::set(StateMask mask, bool b)
{
	b ? set(mask) : clear(mask);
}

/// Clears the Negative Flag if the operand is $#00-7F, otherwise sets it.
inline void CPU::set_neg(word_t operand)
{
	set(StateMask::Negative, operand & 0b10000000);
}

/// Sets the Zero Flag if the operand is $#00, otherwise clears it.
inline void CPU::set_zero(word_t operand)
{
	set(StateMask::Zero, operand == 0);
}

inline void CPU::set_neg_zero(word_t operand)
{
	set_neg(operand);
	set_zero(operand);
}

inline addr_t CPU::addr_immediate()
{
	auto r = _reg_pc;
	++_reg_pc;
	return r;
}

inline addr_t CPU::addr_abs()
{
	auto r = read(_reg_pc) + static_cast<addr_t>(read(_reg_pc + 1) << 8);
	++_reg_pc;
	++_reg_pc;
	return r;
}

inline addr_t CPU::addr_absX()
{
	addr_t r = read(_reg_pc) + (read((_reg_pc + 1) & 0xFFFF) << 8) + _reg_x;
	++_reg_pc;
	++_reg_pc;
	return r;
}

inline addr_t CPU::addr_absY()
{
	addr_t r = read(_reg_pc) + (read((_reg_pc + 1) & 0xFFFF) << 8) + _reg_y;
	++_reg_pc;
	++_reg_pc;
	return r;
}

inline addr_t CPU::addr_zero()
{
	addr_t r = read(_reg_pc);
	++_reg_pc;
	return r & 0xFF;
}

inline addr_t CPU::addr_zeroX()
{
	addr_t r = (read(_reg_pc) + _reg_x) & 0xFF;
	++_reg_pc;
	return r;
}

inline addr_t CPU::addr_zeroY()
{
	addr_t r = (read(_reg_pc) + _reg_y) & 0xFF;
	++_reg_pc;
	return r;
}

inline addr_t CPU::addr_indirect()
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

inline addr_t CPU::addr_indirectX()
{
	addr_t tmp = read(_reg_pc);
	addr_t l = read((_reg_x + tmp) & 0xFF);
	addr_t h = read((_reg_x + tmp + 1) & 0xFF);
	++_reg_pc;
	return (l | (h << 8));
}

inline addr_t CPU::addr_indirectY()
{
	addr_t tmp = read(_reg_pc);
	addr_t l = read(tmp);
	addr_t h = read((tmp + 1) & 0xFF);
	++_reg_pc;
	return ((l | (h << 8)) + _reg_y) & 0xFFFF;
}
