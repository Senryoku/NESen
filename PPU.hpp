#pragma once

/**
 * NES Picture Processing Unit
 *
 *
**/ 
class PPU
{
public:
	using word_t = uint8_t;
	using addr_t = uint16_t;
	struct color_t
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
	};
	
	PPU() :
		_screen(new color_t[240 * 341])
	{
	}
	
	inline word_t read(addr_t addr)
	{
		return _registers[addr & 0b11];
	}
	
	inline void write(addr_t addr, word_t value)
	{
		_registers[addr & 0b11] = addr;
	}
	
	inline unsigned int getCycles() const
	{
		return _cycles;
	}
	
	void step()
	{
		// Visible lines
		if(_line < 240)
		{
		} else if(_line == 240) {
			// Idle
		} else if(_line == 241) {
			if(_dot == 1)
				_registers[2] |= 0b10000000; // VBlank
		} else if(_line < 260) {
			// Idle too ?
		}
		
		++_dot;
		// "For odd frames, the cycle at the end of the [pre-render] scanline is skipped"
		if((_dot > 339 && _line == 261 && _frame % 2 == 0) || 
			_dot > 340)
		{
			_line = (_line + 1) % 262;
			_dot = 0;
		}
		++_cycles;
	}
	
private:
	unsigned int _cycles = 0;
	unsigned int _frame = 0;
	unsigned int _line	= 261;
	unsigned int _dot	= 0;
	
	word_t		_registers[8];
	
	uint16_t	_shift16[2];
	uint8_t		_shift8[2];
	
	color_t*		_screen;
};