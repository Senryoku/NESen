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
		word_t r = 0, g = 0, b = 0, a = 0;
		
		color_t& operator=(const color_t& v) =default;
		inline bool operator==(const color_t& v)
		{
			return r == v.r && g == v.g && b == v.b && a == v.a;
		}
		
		inline color_t& operator=(word_t val)
		{
			r = g = b = a = val;
			return *this;
		}
		
		inline bool operator==(word_t val)
		{
			return r == g && g == b && b == a && a == val;
		}
	};
	
	const size_t ScreenWidth = 256;
	const size_t ScreenHeight = 240;
	const size_t CyclesPerScanline = 240;
	
	const size_t MemSize = 0x4000;
	
	enum PPUControl : word_t
	{
		NameTableAddress = 0x03,
		VerticalWrite = 0x04,
		SpritePatternTableAddress = 0x18,
		SpriteSize = 0x20,
		PPUMasterSlaveMode = 0x40,
		VBlankEnable = 0x80
	};
	
	enum PPUControl2 : word_t
	{
		ImageMask = 0x02,
		SpriteMask = 0x04,
		ScreenEnable = 0x08,
		SpritesEnable = 0x10,
		BackgroundColor = 0xE0
	};
	
	PPU() :
		_mem(new word_t[MemSize]),
		_screen(new color_t[240 * 256])
	{
		reset();
	}
	
	~PPU()
	{
		delete[] _screen;
	}
	
	void reset()
	{
		std::memset(_screen, 0, 240 * 256);
		std::memset(_mem, 0, MemSize);
	}
	
	inline word_t read(addr_t addr)
	{
		switch(addr)
		{
			case 0x2000: return _ppu_control;
			case 0x2001: return _ppu_control2;
			case 0x2002: return _ppu_status;
		}
		std::cerr << "Read on unsupported PPU register: 0x" << std::hex << addr << std::endl;
		return 0;
	}
	
	inline word_t readVRAM(addr_t addr)
	{
		return _mem[addr];
	}
	
	inline void write(addr_t addr, word_t value)
	{
		switch(addr)
		{
			case 0x2000: _ppu_control = value; break;
			case 0x2001: _ppu_control2 = value; break;
			case 0x2002: _ppu_status = value; break;
			case 0x2005: // Scrolling Register
				if(_scroll_first_read)
					_scroll_x = value;
				else
					_scroll_y = value;
				_scroll_first_read = !_scroll_first_read;
			break;
			case 0x2006:
				if(_access_addr_upper)
					_access_addr = (_access_addr & 0x00FF) | (value << 8);
				else
					_access_addr = (_access_addr & 0xFF00) | value;
				_access_addr_upper = !_scroll_first_read;
			break;
			case 0x2007:
				_mem[_access_addr] = value;
				std::cout << "PPU Write: 0x" << std::hex << _access_addr << std::endl;
				_access_addr += (_ppu_control & VerticalWrite) ? 32 : 1;
			break;
			default:
				std::cerr << "Write on unsupported PPU register: 0x" << std::hex << addr << std::endl;
			break;
		}
	}
	
	inline const color_t* get_screen() const
	{
		return _screen;
	}
	
	void step(size_t cpu_cycles)
	{
		
		_cycles += 3 * cpu_cycles;
		if(_cycles > CyclesPerScanline)
		{
			if(_line < ScreenHeight)
			{
			}
			++_line;
		}
	}
	
	static inline void palette_translation(word_t l, word_t h, word_t& r0, word_t& r1)
	{
		r0 = r1 = 0;
		for(int i = 0; i < 4; i++)
			r0 |= (((l & (1 << (i + 4))) >> (4 + i)) << (2 * i)) | 
					(((h & (1 << (i + 4))) >> (4 + i))  << (2 * i + 1));
		for(int i = 0; i < 4; i++)
			r1 |= ((l & (1 << i)) << (2 * i - i)) | 
					((h & (1 << i)) << (2 * i + 1 - i));
	}
	
private:
	unsigned int _cycles = 0;
	unsigned int _frame = 0;
	unsigned int _line	= 261;
	unsigned int _dot	= 0;
	
	// Registers
	word_t		_ppu_control = 0;			// $2000
	word_t		_ppu_control2 = 0;			// $2001
	word_t		_ppu_status = 0;			// $2002
	bool		_scroll_first_read = true;
	word_t		_scroll_x = 0;				// Access by $2005
	word_t		_scroll_y = 0;				// Access by $2005
	
	bool		_access_addr_upper = true;
	addr_t		_access_addr = 0;			// Access by $2006
	
	word_t*		_mem = nullptr;
	
	color_t*	_screen;
};