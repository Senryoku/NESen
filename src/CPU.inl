////////////////////////////////////////////////////////////////////////////////////////////////
// CPU Inline functions

word_t CPU::read(addr_t addr)
{
	if(addr < RAMSize)
		return _ram[addr];
	else if(addr < 0x2000) // RAM mirrors
		return _ram[addr % RAMSize];
	else if(addr < 0x2008) // PPU registers
		return ppu->read(addr);
	else if(addr < 0x4000) // PPU registers mirrors
		return ppu->read(addr);
	else if(addr < 0x4016) // pAPU
		return apu->read(addr);
	else if(addr == 0x4016) // Controllers registers
		return read_controller_state();
	else if(addr == 0x4017) // Controllers registers
		return read_controller2_state();
	else if(addr < 0x5000) // Unused
		return 0;
	else if(addr < 0x6000) // Expansion
		return cartridge->read(addr);
	else if(addr < 0x8000) // SRAM
		return cartridge->read(addr);
	else if(addr <= 0xFFFF) // Cartridge
		return cartridge->read(addr);
	else {
		error << "Error: Read addr " << Hexa(addr) << " out of bounds." << std::endl;
		exit(1);
	}
}

addr_t CPU::read16(addr_t addr)
{
	return read(addr) | (read(addr + 1) << 8);
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
	else if(addr == 0x4016) // Controllers registers
		_refresh_controller = value & 1;
	else if(addr < 0x4018) // APU registers
		apu->write(addr - 0x4000, value);
	else
		cartridge->write(addr, value);
}

void CPU::oam_dma(word_t value)
{
	/// @todo Timing: Takes 513 or 514 (if on an odd cycle) Cycles
	ppu->write(0x2003, 0x00); // Reset OAMADDR
	addr_t start = (value << 8);
	for(addr_t a = 0; a < 256; ++a)
		ppu->write(0x2004, _ram[start + a]);
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Stack

inline void CPU::push(word_t value)
{
	_ram[0x100 + _reg_sp--] = value;
}

inline void CPU::push16(addr_t value)
{
	push((value & 0xFF00) >> 8);
	push(value & 0xFF);
}

inline word_t CPU::pop()
{
	return _ram[0x100 + ++_reg_sp];
}

inline addr_t CPU::pop16()
{
	addr_t l = pop();
	addr_t h = pop();
	return (h << 8) | l;
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

////////////////////////////////////////////////////////////////////////////////////////////////
// Addressing modes

inline addr_t CPU::addr_immediate()
{
	auto r = _reg_pc;
	++_reg_pc;
	return r;
}

inline addr_t CPU::addr_abs()
{
	addr_t r = read16(_reg_pc);
	_reg_pc += 2;
	return r;
}

inline addr_t CPU::addr_absX()
{
	addr_t r = read16(_reg_pc) + _reg_x;
	_reg_pc += 2;
	return r;
}

inline addr_t CPU::addr_absY()
{
	addr_t r = read16(_reg_pc) + _reg_y;
	_reg_pc += 2;
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
